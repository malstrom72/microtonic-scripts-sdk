#!/usr/bin/env node
//
// Microtonic JSConsole bridge — MCP server
// ========================================
//
// Drives the file bridge built into JSConsole.mtscript so an MCP client (e.g.
// Claude Code) can evaluate JavaScript against a *live* Microtonic engine and
// read the result back, with no GUI automation.
//
// Protocol (must match JSConsole.mtscript/JSConsole_main.js):
//
//   <base>/request.json    we write (temp file + rename):  { seq, code }
//   <base>/response.json   the bridge overwrites:          { seq, ok, value, output, error }
//   <base>/bridge.json     the bridge writes on `bridge on`: { ready, protocol, time }
//
// The bridge can neither create folders nor delete files, so this host owns the
// directory: it `mkdir -p`s <base> on startup, writes requests atomically, and
// pairs replies by a strictly increasing `seq`. We base `seq` on epoch ms so it
// keeps climbing across restarts of this server.
//
// Transport is MCP stdio: newline-delimited JSON-RPC 2.0 on stdin/stdout.
// stdout MUST carry only protocol messages — all diagnostics go to stderr.
//

'use strict';

const fs = require('fs');
const path = require('path');

const SERVER_NAME = 'microtonic-jsconsole-bridge';
const SERVER_VERSION = '1.0.0';
const DEFAULT_PROTOCOL = '2024-11-05';
const DEFAULT_TIMEOUT_MS = 20000; // a single eval may run up to ~20s in Microtonic
const POLL_INTERVAL_MS = 50;

function log() {
	console.error('[' + SERVER_NAME + ']', ...arguments);
}

function withSlash(p) {
	return p.charAt(p.length - 1) === '/' ? p : p + '/';
}

function bridgeBase() {
	if (process.env.BRIDGE_BASE) {
		return withSlash(process.env.BRIDGE_BASE.replace(/\\/g, '/'));
	}
	if (process.platform === 'win32') {
		return 'C:/Users/Public/Sonic Charge/Microtonic/jsconsole-bridge/';
	}
	return '/Users/Shared/Sonic Charge/Microtonic/jsconsole-bridge/';
}

const BASE = bridgeBase();
const REQUEST_PATH = path.join(BASE, 'request.json');
const RESPONSE_PATH = path.join(BASE, 'response.json');
const PRESENCE_PATH = path.join(BASE, 'bridge.json');

let lastSeq = 0;

function ensureBase() {
	fs.mkdirSync(BASE, { recursive: true });
	// Continue numbering above any request left from a previous run.
	try {
		const prev = JSON.parse(fs.readFileSync(REQUEST_PATH, 'utf8'));
		if (prev && typeof prev.seq === 'number') {
			lastSeq = prev.seq;
		}
	} catch (e) { /* no prior request, fine */ }
}

function nextSeq() {
	let s = Math.floor(Date.now());
	if (s <= lastSeq) {
		s = lastSeq + 1;
	}
	lastSeq = s;
	return s;
}

function sleep(ms) {
	return new Promise(function (resolve) { setTimeout(resolve, ms); });
}

function readJson(p) {
	try {
		return JSON.parse(fs.readFileSync(p, 'utf8'));
	} catch (e) {
		return null;
	}
}

//
// Tool: mt_eval — write a request atomically, poll for the matching reply.
//
async function mtEval(args) {
	const code = args && typeof args.code === 'string' ? args.code : null;
	if (code === null) {
		throw new Error('mt_eval requires a string "code" argument');
	}
	const timeout = args && typeof args.timeout_ms === 'number' ? args.timeout_ms : DEFAULT_TIMEOUT_MS;
	const seq = nextSeq();

	// Atomic publish: write a temp file in the same dir, then rename over request.json.
	const tmp = path.join(BASE, 'request.' + process.pid + '.' + seq + '.tmp');
	fs.writeFileSync(tmp, JSON.stringify({ seq: seq, code: code }));
	fs.renameSync(tmp, REQUEST_PATH);

	const deadline = Date.now() + timeout;
	while (Date.now() < deadline) {
		const resp = readJson(RESPONSE_PATH);
		if (resp && resp.seq === seq) {
			return resp;
		}
		await sleep(POLL_INTERVAL_MS);
	}
	throw new Error('timed out after ' + timeout + 'ms with no reply — the bridge is not '
		+ 'responding. Check, in order of likelihood: '
		+ '1) the JSConsole window is open in Microtonic; '
		+ '2) you typed `bridge on` in it this session (a leftover bridge.json does not mean it is live); '
		+ '3) Microtonic is running. '
		+ 'Only if it was working and just stopped: a modal dialog may be blocking the bridge tick — '
		+ 'dismiss it in Microtonic, then `bridge off` / `bridge on`. '
		+ 'Run mt_status to probe the connection.');
}

function formatEval(resp) {
	const parts = ['value: ' + resp.value];
	if (resp.output && resp.output.length) {
		parts.push('output:\n' + resp.output.replace(/\n$/, ''));
	}
	if (!resp.ok) {
		parts.push('error: ' + resp.error);
	}
	return { text: parts.join('\n'), isError: !resp.ok };
}

//
// Tool: mt_status — report whether the bridge is actually responding.
//
// The bridge.json presence file only proves the bridge was enabled at *some* point:
// it is written once on `bridge on` and never updated, so it lingers after JSConsole
// is closed or Microtonic quits. Presence is therefore NOT liveness. To report the
// truth we actively probe — send a trivial eval and see if a reply comes back.
//
const PROBE_TIMEOUT_MS = 1500;

async function mtStatus() {
	const lines = ['base: ' + BASE];
	if (!fs.existsSync(BASE)) {
		lines.push('folder: missing (will be created on first mt_eval)');
		return { text: lines.join('\n'), isError: false };
	}
	const presence = readJson(PRESENCE_PATH);

	let live = false;
	try {
		await mtEval({ code: '1', timeout_ms: PROBE_TIMEOUT_MS });
		live = true;
	} catch (e) { /* no reply within the probe window */ }

	if (live) {
		lines.push('bridge: LIVE — responded to a probe.');
	} else if (presence && presence.ready) {
		// Use the presence file's mtime (OS wall clock) for "announced ago" — the
		// bridge measures time with getMonotonicTime(), not a wall clock, so it does
		// not write a comparable epoch timestamp.
		let announced = '';
		try {
			const ageMs = Date.now() - fs.statSync(PRESENCE_PATH).mtimeMs;
			announced = ' (bridge.json announced ' + Math.round(ageMs / 1000) + 's ago)';
		} catch (e) { /* mtime unavailable, omit */ }
		lines.push('bridge: NOT RESPONDING' + announced + '.');
		lines.push('  A presence file exists but no reply came back. Most likely, in order: '
			+ '1) the JSConsole window is not open; 2) `bridge on` was not typed in it this session; '
			+ '3) Microtonic is not running; 4) a modal dialog is blocking the bridge tick (dismiss it, '
			+ 'then `bridge off` / `bridge on`).');
	} else {
		lines.push('bridge: NOT RESPONDING and no presence file — open JSConsole in Microtonic '
			+ 'and type `bridge on`.');
	}
	const req = readJson(REQUEST_PATH);
	const resp = readJson(RESPONSE_PATH);
	lines.push('last request seq: ' + (req && typeof req.seq === 'number' ? req.seq : '(none)'));
	lines.push('last reply seq: ' + (resp && typeof resp.seq === 'number' ? resp.seq : '(none)'));
	return { text: lines.join('\n'), isError: false };
}

const TOOLS = [
	{
		name: 'mt_eval',
		description: 'Evaluate JavaScript against the live Microtonic engine via the JSConsole '
			+ 'file bridge and return the result. The JSConsole window must be open with the '
			+ 'bridge enabled (type `bridge on` in it). Code runs in the shared JS global space, '
			+ 'so it can read and drive a script running in the main GUI layer. Keep snippets '
			+ 'short: each eval freezes the UI and is subject to Microtonic\'s ~20s suspension '
			+ 'limit. Wrap multi-statement snippets in an IIFE to avoid leaking vars or '
			+ 'shadowing host globals such as save, load, or print.',
		inputSchema: {
			type: 'object',
			properties: {
				code: {
					type: 'string',
					description: 'JavaScript to evaluate, e.g. "getElement(\'pattern\').steps". '
						+ 'The value of the final expression is returned; print() output is captured too.'
				},
				timeout_ms: {
					type: 'number',
					description: 'How long to wait for a reply before giving up. Default ' + DEFAULT_TIMEOUT_MS + '.'
				}
			},
			required: ['code']
		}
	},
	{
		name: 'mt_status',
		description: 'Check whether the JSConsole bridge is actually responding. It probes live '
			+ '(sends a trivial eval and waits briefly), reporting LIVE or NOT RESPONDING rather '
			+ 'than trusting the bridge.json presence file, which lingers after the console is '
			+ 'closed. Use it before evaluating, and when an mt_eval times out: NOT RESPONDING '
			+ 'almost always means JSConsole is closed or `bridge on` was not typed this '
			+ 'session, not a modal dialog.',
		inputSchema: { type: 'object', properties: {} }
	}
];

async function handleToolCall(name, args) {
	if (name === 'mt_eval') {
		const resp = await mtEval(args || {});
		return formatEval(resp);
	}
	if (name === 'mt_status') {
		return await mtStatus();
	}
	throw new Error('unknown tool: ' + name);
}

//
// JSON-RPC plumbing
//

function send(msg) {
	process.stdout.write(JSON.stringify(msg) + '\n');
}

function sendResult(id, result) {
	send({ jsonrpc: '2.0', id: id, result: result });
}

function sendError(id, code, message) {
	send({ jsonrpc: '2.0', id: id, error: { code: code, message: message } });
}

async function dispatch(msg) {
	const id = msg.id;
	const method = msg.method;

	// Notifications (no id) get no response.
	if (id === undefined || id === null) {
		return;
	}

	switch (method) {
		case 'initialize': {
			const requested = msg.params && msg.params.protocolVersion;
			sendResult(id, {
				protocolVersion: requested || DEFAULT_PROTOCOL,
				capabilities: { tools: {} },
				serverInfo: { name: SERVER_NAME, version: SERVER_VERSION }
			});
			return;
		}
		case 'ping':
			sendResult(id, {});
			return;
		case 'tools/list':
			sendResult(id, { tools: TOOLS });
			return;
		case 'tools/call': {
			const params = msg.params || {};
			try {
				const out = await handleToolCall(params.name, params.arguments);
				sendResult(id, {
					content: [{ type: 'text', text: out.text }],
					isError: !!out.isError
				});
			} catch (e) {
				// Tool-level failure is reported as a result with isError, per MCP.
				sendResult(id, {
					content: [{ type: 'text', text: String(e && e.message ? e.message : e) }],
					isError: true
				});
			}
			return;
		}
		default:
			sendError(id, -32601, 'method not found: ' + method);
			return;
	}
}

function main() {
	ensureBase();
	log('ready. bridge folder:', BASE);

	// Don't exit while a tool call is still in flight (an mt_eval may be mid-poll
	// when stdin closes). Real clients keep stdin open; this matters for graceful
	// shutdown and for piped/test invocations.
	let pending = 0;
	let endReceived = false;
	let exiting = false;
	function maybeExit() {
		if (endReceived && pending === 0 && !exiting) {
			exiting = true;
			// Flush any buffered stdout (e.g. the final reply) before exiting; a
			// bare process.exit() can truncate a pending write on a pipe.
			process.stdout.write('', function () { process.exit(0); });
		}
	}

	let buffer = '';
	process.stdin.setEncoding('utf8');
	process.stdin.on('data', function (chunk) {
		buffer += chunk;
		let nl;
		while ((nl = buffer.indexOf('\n')) >= 0) {
			const line = buffer.slice(0, nl).trim();
			buffer = buffer.slice(nl + 1);
			if (line === '') {
				continue;
			}
			let msg;
			try {
				msg = JSON.parse(line);
			} catch (e) {
				log('failed to parse line:', line);
				continue;
			}
			// dispatch is async; errors inside are handled per-message.
			pending++;
			Promise.resolve(dispatch(msg)).catch(function (e) {
				log('dispatch error:', e);
			}).then(function () {
				pending--;
				maybeExit();
			});
		}
	});
	process.stdin.on('end', function () {
		endReceived = true;
		maybeExit();
	});
}

main();
