#!/usr/bin/env node
//
// JSConsole bridge — MCP server
// =============================
//
// Drives the file bridge built into the product's JS Console script package so an
// MCP client (e.g. Claude Code) can evaluate JavaScript against a *live* engine and
// read the result back, with no GUI automation.
//
// This file is shared verbatim between the Microtonic and Synplant Scripts SDKs.
// Everything that differs between the two products lives in the PRODUCT block
// below; keep the rest of this file (and server.test.js) byte-identical in both
// repos.
//
// Protocol (must match <PRODUCT.consolePackage>/JSConsole_main.js):
//
//   <base>/request.json    we write (temp file + rename):  { seq, code }
//   <base>/response.json   the bridge overwrites:          { seq, ok, value, output, error }
//   <base>/bridge.json     the bridge writes on `bridge on`: { ready, protocol, time, owner }
//                          (`time` is epoch ms; `owner` is a token identifying the
//                          instance that currently holds the bridge)
//
// Server side only (the bridge never reads it):
//
//   <base>/server-owner.json  the newest server writes on startup: { token, pid, time }
//                             (a server that finds another token there stands down)
//
// The bridge never deletes files, so this host owns the directory: it `mkdir -p`s
// <base> on startup, writes requests atomically, and pairs replies by a strictly
// increasing `seq`. We base `seq` on epoch ms so it keeps climbing across restarts
// of this server.
//
// Transport is MCP stdio: newline-delimited JSON-RPC 2.0 on stdin/stdout.
// stdout MUST carry only protocol messages — all diagnostics go to stderr.
//

'use strict';

const crypto = require('crypto');
const fs = require('fs');
const os = require('os');
const path = require('path');

// ---------------------------------------------------------------------------
// BEGIN PRODUCT CONFIGURATION — the only part of this file that differs between
// the Microtonic and Synplant Scripts SDKs.
// ---------------------------------------------------------------------------
const PRODUCT = {
	name: 'Microtonic',
	serverName: 'microtonic-jsconsole-bridge',
	toolPrefix: 'mt',                       // tools are <prefix>_eval / _status / _reload
	consoleName: 'JSConsole',               // how the console window is named in the UI
	consolePackage: 'JSConsole.mtscript',
	scriptExtension: '.mtscript',
	evalExample: "getElement('pattern').steps",
	reloadExample: "typeof polyrhythmChain.newAction !== 'undefined'",
	/*
		Must match jsConsole.bridgeDefaultBase() in JSConsole_main.js. Microtonic's
		script API cannot create folders, so both ends use a fixed, user-writable,
		username-independent shared folder that this server creates. It must never
		resolve to the same folder as another product's bridge: two consoles open at
		once would then share request.json / response.json.
	*/
	defaultBase: function () {
		return process.platform === 'win32'
			? 'C:/Users/Public/Sonic Charge/Microtonic/jsconsole-bridge/'
			: '/Users/Shared/Sonic Charge/Microtonic/jsconsole-bridge/';
	}
};
// ---------------------------------------------------------------------------
// END PRODUCT CONFIGURATION
// ---------------------------------------------------------------------------

const SERVER_NAME = PRODUCT.serverName;
const SERVER_VERSION = '1.0.0';
/*
	MCP protocol versions this server implements, newest first. 2025-03-26 is left out because it
	requires JSON-RPC batches, which this server does not handle.
*/
const PROTOCOL_VERSIONS = ['2025-06-18', '2024-11-05'];
const DEFAULT_TIMEOUT_MS = 20000; // a single eval may run up to ~20s in the engine
const POLL_INTERVAL_MS = 50;
const RELOAD_POLL_MS = 150;
const RELOAD_DEFAULT_TIMEOUT_MS = 10000;

const EVAL_TOOL = PRODUCT.toolPrefix + '_eval';
const STATUS_TOOL = PRODUCT.toolPrefix + '_status';
const RELOAD_TOOL = PRODUCT.toolPrefix + '_reload';
const READY_TOKEN = PRODUCT.toolPrefix.toUpperCase() + '_READY';
const WAIT_TOKEN = PRODUCT.toolPrefix.toUpperCase() + '_WAIT';
const CONSOLE = PRODUCT.consoleName;
const PRODUCT_NAME = PRODUCT.name;

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
	return withSlash(PRODUCT.defaultBase().replace(/\\/g, '/'));
}

const BASE = bridgeBase();
const REQUEST_PATH = path.join(BASE, 'request.json');
const RESPONSE_PATH = path.join(BASE, 'response.json');
const PRESENCE_PATH = path.join(BASE, 'bridge.json');
const OWNER_PATH = path.join(BASE, 'server-owner.json');

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

/*
	On Windows, renaming over or deleting a file that another process has open (the bridge polls
	request.json) fails with EPERM/EACCES/EBUSY. Retry with a short, jittered backoff (like
	graceful-fs) so retries can't lock onto the poller's timing, until `limitMs` runs out.
*/
const RENAME_RETRY_CODES = ['EPERM', 'EACCES', 'EBUSY'];
const RENAME_RETRY_LIMIT_MS = 1000;

async function renameWithRetry(from, to, rename, limitMs) {
	rename = rename || fs.renameSync;
	return retryWhileLocked(function () { rename(from, to); }, limitMs);
}

async function retryWhileLocked(op, limitMs) {
	const deadline = Date.now() + (limitMs || RENAME_RETRY_LIMIT_MS);
	let delay = 5;
	for (;;) {
		try {
			op();
			return;
		} catch (e) {
			if (RENAME_RETRY_CODES.indexOf(e.code) < 0 || Date.now() >= deadline) {
				throw e;
			}
		}
		await sleep(Math.min(delay / 2 + Math.random() * delay, Math.max(0, deadline - Date.now())));
		delay = Math.min(delay * 2, 100);
	}
}

// Atomic publish: write `value` to a temp file in the bridge folder, then rename it over `dest`.
async function writeJsonAtomic(tmpName, dest, value, limitMs) {
	const tmp = path.join(BASE, tmpName);
	fs.writeFileSync(tmp, JSON.stringify(value));
	try {
		await renameWithRetry(tmp, dest, null, limitMs);
	} catch (e) {
		try { fs.unlinkSync(tmp); } catch (e2) { /* already gone */ }
		throw e;
	}
}

/*
	request.json and response.json hold one message each, so bridge calls run one at a time and
	`timeout_ms` includes the wait in this queue. A call that times out withdraws its request, so a
	blocked bridge does not run it later; one the bridge already picked up still runs.
*/
let bridgeQueue = Promise.resolve();
let callsPending = 0;

// Set by notifications/cancelled. A cancelled call stops waiting and withdraws its request.
function cancelToken() {
	const token = { cancelled: false };
	token.whenCancelled = new Promise(function (resolve) {
		token.cancel = function () {
			token.cancelled = true;
			resolve(false);
		};
	});
	return token;
}

function throwIfCancelled(token) {
	if (token.cancelled) {
		throw new Error('cancelled by the client');
	}
}

// Marked so callers that retry, like the reload probes, can tell a timeout from a real failure.
function timeoutError(message) {
	const e = new Error(message);
	e.timedOut = true;
	return e;
}

// Resolves true once `turn` settles, or false if `ms` runs out or the call is cancelled first.
function waitAtMost(turn, ms, token) {
	let timer = null;
	const expired = new Promise(function (resolve) { timer = setTimeout(resolve, ms, false); });
	return Promise.race([turn.then(function () { return true; }), expired, token.whenCancelled])
		.finally(function () {
			clearTimeout(timer);
		});
}

// Deletes our request unless the bridge replaced it. False only if it stayed locked.
async function withdrawRequest(seq) {
	const req = readJson(REQUEST_PATH);
	if (!req || req.seq !== seq) {
		return true;
	}
	try {
		await retryWhileLocked(function () {
			try {
				fs.unlinkSync(REQUEST_PATH);
			} catch (e) {
				if (e.code !== 'ENOENT') {
					throw e;
				}
			}
		});
		return true;
	} catch (e) {
		log('could not withdraw request', seq + ':', e.message);
		return false;
	}
}

/*
	Only one server may drive the bridge folder at a time, or two would overwrite each other's
	request.json. A starting server takes over by writing its token to server-owner.json. A server
	that finds another token there has been taken over and stands down for good: it answers tool
	calls with an error instead of writing requests. This mirrors the console's owner token in
	bridge.json and leaves the console protocol unchanged.
*/
const OWNER_TOKEN = crypto.randomBytes(8).toString('hex');
let ownerClaimed = Promise.resolve();
let stoodDownFor = null;	// the owner record that replaced ours

function claimOwnership() {
	return writeJsonAtomic('server-owner.' + process.pid + '.tmp', OWNER_PATH,
		{ token: OWNER_TOKEN, pid: process.pid, time: Date.now() });
}

// Throws once another server has taken over. A missing or unreadable owner file is claimed again.
async function checkOwnership() {
	await ownerClaimed;
	if (stoodDownFor === null) {
		const owner = readJson(OWNER_PATH);
		if (owner === null || typeof owner.token !== 'string') {
			await claimOwnership();
		} else if (owner.token !== OWNER_TOKEN) {
			stoodDownFor = owner;
			log('another server took over the bridge folder; standing down');
		}
	}
	if (stoodDownFor !== null) {
		const who = typeof stoodDownFor.pid === 'number' ? ' (server pid ' + stoodDownFor.pid + ')' : '';
		throw new Error('another session took over the bridge' + who + '; nothing was sent. Restart or '
			+ 'reconnect this session\'s ' + SERVER_NAME + ' MCP server to take it back.');
	}
}

//
// Tool: <prefix>_eval — write a request atomically, poll for the matching reply.
//
async function bridgeEval(args, token) {
	const code = args && typeof args.code === 'string' ? args.code : null;
	if (code === null) {
		throw new Error(EVAL_TOOL + ' requires a string "code" argument');
	}
	const timeout = args && typeof args.timeout_ms === 'number' ? args.timeout_ms : DEFAULT_TIMEOUT_MS;
	const deadline = Date.now() + timeout;

	const previous = bridgeQueue;
	let release = null;
	const mine = new Promise(function (resolve) { release = resolve; });
	bridgeQueue = previous.then(function () { return mine; });
	callsPending++;
	try {
		if (!await waitAtMost(previous, deadline - Date.now(), token)) {
			throwIfCancelled(token);
			throw timeoutError('timed out after ' + timeout + 'ms waiting for an earlier bridge call to '
				+ 'finish; nothing was sent.');
		}
		return await bridgeEvalNow(code, timeout, deadline, token);
	} finally {
		callsPending--;
		release();
	}
}

async function bridgeEvalNow(code, timeout, deadline, token) {
	await checkOwnership();
	const seq = nextSeq();
	await writeJsonAtomic('request.' + process.pid + '.' + seq + '.tmp', REQUEST_PATH, { seq: seq, code: code },
		Math.max(1, deadline - Date.now()));

	while (Date.now() < deadline && !token.cancelled) {
		const resp = readJson(RESPONSE_PATH);
		if (resp && resp.seq === seq) {
			return resp;
		}
		await sleep(POLL_INTERVAL_MS);
	}
	const withdrawn = await withdrawRequest(seq);
	throwIfCancelled(token);
	const resp = readJson(RESPONSE_PATH);
	if (resp && resp.seq === seq) {
		return resp;	// the reply landed while we were withdrawing
	}
	let detail = '';
	if (resp && typeof resp.seq === 'number' && resp.seq < seq) {
		detail = ' Last reply seq is still ' + resp.seq + ' while this request seq is ' + seq + '.';
	}
	detail += withdrawn
		? ' The request was withdrawn, so it will not run later unless the bridge had already '
			+ 'started it.'
		: ' The request could not be withdrawn (request.json stayed locked) and may still run later.';
	throw timeoutError('timed out after ' + timeout + 'ms with no reply — the bridge is not '
		+ 'responding.' + detail + ' Check, in order of likelihood: '
		+ '1) the ' + CONSOLE + ' window is open in ' + PRODUCT_NAME + '; '
		+ '2) you typed `bridge on` in it this session (a leftover bridge.json does not mean it is live); '
		+ '3) ' + PRODUCT_NAME + ' is running. '
		+ 'Only if it was working and just stopped: a modal dialog may be blocking the bridge tick — '
		+ 'dismiss it in ' + PRODUCT_NAME + ', then `bridge off` / `bridge on`. '
		+ 'Run ' + STATUS_TOOL + ' to probe the connection.');
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
// Tool: <prefix>_reload — invoke the asynchronous reload action, then poll an
// observable effect until the edited scripts are actually live.
//
// performCushyAction itself is synchronous; `reload` is the asynchronous part. Its
// boolean result is only an invocation result (and reload always succeeds), so it
// cannot tell callers when the script rerun has finished.
//
async function bridgeReload(args, token) {
	const until = args && typeof args.until === 'string' && args.until !== '' ? args.until : null;
	const timeout = args && typeof args.timeout_ms === 'number'
		? args.timeout_ms
		: RELOAD_DEFAULT_TIMEOUT_MS;

	const issued = await bridgeEval({ code: "performCushyAction('reload')" }, token);
	if (!issued.ok) {
		throw new Error('reload could not be invoked: ' + issued.error);
	}
	if (until === null) {
		return {
			text: 'reload invoked. WARNING: the reload action is asynchronous and no `until` '
				+ 'predicate was supplied, so the new code may not be live yet. Pass `until` '
				+ '(e.g. "typeof myScript.newAction !== \'undefined\'") to wait for it properly.',
			isError: false
		};
	}

	/*
		A predicate that touches a not-yet-defined global may throw. Treat that as "not ready" so
		callers do not have to make every natural probe defensive.
	*/
	const probe = '(function(){try{return (' + until + ') ? "' + READY_TOKEN + '" : "'
		+ WAIT_TOKEN + '";}catch(e){return "' + WAIT_TOKEN + '";}})()';
	const started = Date.now();
	const deadline = started + timeout;
	while (Date.now() < deadline) {
		await sleep(RELOAD_POLL_MS);
		const remaining = deadline - Date.now();
		if (remaining <= 0) {
			break;
		}
		let r = null;
		try {
			r = await bridgeEval({ code: probe, timeout_ms: Math.min(5000, remaining) }, token);
		} catch (e) {
			// One probe timing out is not the end; keep polling until the deadline.
			if (!e.timedOut) {
				throw e;
			}
		}
		if (r && r.ok && String(r.value).indexOf(READY_TOKEN) >= 0) {
			return {
				text: 'reload complete after ' + (Date.now() - started)
					+ 'ms (predicate satisfied).',
				isError: false
			};
		}
	}
	throw new Error('reload was invoked but the `until` predicate did not become true within '
		+ timeout + 'ms. The predicate may be wrong, or the script may have failed to parse — '
		+ 'check the ' + CONSOLE + ' output before assuming the reload did not happen.');
}

//
// Tool: <prefix>_status — report whether the bridge is actually responding.
//
// The bridge.json presence file only proves the bridge was enabled at *some* point:
// it is written on `bridge on` and not refreshed while running, so it lingers after
// the console is closed or the product quits. Presence is therefore NOT liveness.
// To report the truth we actively probe — send a trivial eval and see if a reply
// comes back.
//
const PROBE_TIMEOUT_MS = 1500;

async function bridgeStatus(token) {
	const lines = ['base: ' + BASE];
	if (!fs.existsSync(BASE)) {
		lines.push('folder: missing (will be created on first ' + EVAL_TOOL + ')');
		return { text: lines.join('\n'), isError: false };
	}
	try {
		await checkOwnership();
	} catch (e) {
		lines.push('server: STOOD DOWN, ' + e.message);
		return { text: lines.join('\n'), isError: true };
	}
	const presence = readJson(PRESENCE_PATH);

	// A probe would queue behind the call in progress; report that instead of waiting.
	const busy = callsPending > 0;
	let live = false;
	if (!busy) {
		try {
			await bridgeEval({ code: '1', timeout_ms: PROBE_TIMEOUT_MS }, token);
			live = true;
		} catch (e) { /* no reply within the probe window */ }
	}

	if (busy) {
		lines.push('bridge: BUSY, another call is in progress, so no probe was sent. Run '
			+ STATUS_TOOL + ' again after it returns.');
	} else if (live) {
		lines.push('bridge: LIVE — responded to a probe.');
	} else if (presence && presence.ready) {
		// bridge.json carries the bridge's own epoch-ms `time` stamp.
		let announced = '';
		if (typeof presence.time === 'number') {
			const ageMs = Date.now() - presence.time;
			announced = ' (bridge.json announced ' + Math.round(ageMs / 1000) + 's ago)';
		}
		lines.push('bridge: NOT RESPONDING' + announced + '.');
		lines.push('  A presence file exists but no reply came back. Most likely, in order: '
			+ '1) the ' + CONSOLE + ' window is not open; 2) `bridge on` was not typed in it this session; '
			+ '3) ' + PRODUCT_NAME + ' is not running; 4) a modal dialog is blocking the bridge tick (dismiss it, '
			+ 'then `bridge off` / `bridge on`).');
	} else {
		lines.push('bridge: NOT RESPONDING and no presence file — open the ' + CONSOLE + ' window in '
			+ PRODUCT_NAME + ' and type `bridge on`.');
	}
	// Our own last seq, since a timed-out request.json has been withdrawn.
	const resp = readJson(RESPONSE_PATH);
	lines.push('last request seq: ' + (lastSeq ? lastSeq : '(none)'));
	lines.push('last reply seq: ' + (resp && typeof resp.seq === 'number' ? resp.seq : '(none)'));
	return { text: lines.join('\n'), isError: false };
}

const TOOLS = [
	{
		name: EVAL_TOOL,
		description: 'Evaluate JavaScript against the live ' + PRODUCT_NAME + ' engine via the '
			+ CONSOLE + ' file bridge and return the result. The ' + CONSOLE + ' window must be '
			+ 'open with the bridge enabled (type `bridge on` in it). Code runs in the shared JS '
			+ 'global space, so it can read and drive scripts running in the main GUI layer. Keep '
			+ 'snippets short: each eval freezes the UI and is subject to ' + PRODUCT_NAME + '\'s '
			+ '~20s suspension limit. Wrap multi-statement snippets in an IIFE to avoid leaking '
			+ 'vars or shadowing host globals such as save, load, or print. Avoid evals that may '
			+ 'open modal dialogs during reload or startup; a modal blocks the bridge tick until '
			+ 'dismissed.',
		inputSchema: {
			type: 'object',
			properties: {
				code: {
					type: 'string',
					description: 'JavaScript to evaluate, e.g. "' + PRODUCT.evalExample + '". '
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
		name: STATUS_TOOL,
		description: 'Check whether the ' + CONSOLE + ' bridge is actually responding. It probes '
			+ 'live (sends a trivial eval and waits briefly), reporting LIVE or NOT RESPONDING '
			+ 'rather than trusting the bridge.json presence file, which lingers after the console '
			+ 'is closed. Use it before evaluating, and when an ' + EVAL_TOOL + ' times out: NOT '
			+ 'RESPONDING almost always means the ' + CONSOLE + ' window is closed or `bridge on` '
			+ 'was not typed this session, not a modal dialog. While another call is in progress it '
			+ 'reports BUSY without probing.',
		inputSchema: { type: 'object', properties: {} }
	},
	{
		name: RELOAD_TOOL,
		description: 'Re-run edited script files in the live ' + PRODUCT_NAME + ' engine and wait '
			+ 'until the new code is actually live. Use this after editing a '
			+ PRODUCT.scriptExtension + ' instead of evaluating performCushyAction(\'reload\') '
			+ 'yourself: the reload action is asynchronous, so an eval sent straight after a bare '
			+ 'reload can still see the old code. Pass `until` with a JavaScript expression that '
			+ 'becomes true once your change is loaded. A normal reload keeps the engine, globals, '
			+ 'and this bridge alive.',
		inputSchema: {
			type: 'object',
			properties: {
				until: {
					type: 'string',
					description: 'JavaScript expression polled until truthy, e.g. "'
						+ PRODUCT.reloadExample + '". Strongly recommended; '
						+ 'without it the tool cannot tell when the reload finished.'
				},
				timeout_ms: {
					type: 'number',
					description: 'How long to poll. Default ' + RELOAD_DEFAULT_TIMEOUT_MS + '.'
				}
			}
		}
	}
];

async function handleToolCall(name, args, token) {
	if (name === EVAL_TOOL) {
		const resp = await bridgeEval(args || {}, token);
		return formatEval(resp);
	}
	if (name === STATUS_TOOL) {
		return await bridgeStatus(token);
	}
	if (name === RELOAD_TOOL) {
		return await bridgeReload(args || {}, token);
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

// Cancel tokens of the tools/call requests in progress, by request id.
const inFlight = new Map();

async function dispatch(msg) {
	if (msg === null || typeof msg !== 'object' || Array.isArray(msg)) {
		sendError(null, -32600, 'invalid request: expected a single JSON-RPC message object');
		return;
	}
	const id = msg.id;
	const method = msg.method;

	// Notifications (no id) get no response.
	if (id === undefined || id === null) {
		if (method === 'notifications/cancelled' && msg.params) {
			const token = inFlight.get(msg.params.requestId);
			if (token) {
				token.cancel();
			}
		}
		return;
	}

	switch (method) {
		case 'initialize': {
			const requested = msg.params && msg.params.protocolVersion;
			sendResult(id, {
				protocolVersion: PROTOCOL_VERSIONS.indexOf(requested) >= 0 ? requested : PROTOCOL_VERSIONS[0],
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
			const token = cancelToken();
			inFlight.set(id, token);
			let result;
			try {
				const out = await handleToolCall(params.name, params.arguments, token);
				result = {
					content: [{ type: 'text', text: out.text }],
					isError: !!out.isError
				};
			} catch (e) {
				// Tool-level failure is reported as a result with isError, per MCP.
				result = {
					content: [{ type: 'text', text: String(e && e.message ? e.message : e) }],
					isError: true
				};
			} finally {
				inFlight.delete(id);
			}
			// The client has dropped a cancelled request, so it gets no reply.
			if (!token.cancelled) {
				sendResult(id, result);
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
	ownerClaimed = claimOwnership().catch(function (e) {
		log('could not claim the bridge folder:', e.message);
	});
	log('ready. bridge folder:', BASE);

	/*
		Don't exit while a tool call is still in flight (an eval may be mid-poll when
		stdin closes). Real clients keep stdin open; this matters for graceful
		shutdown and for piped/test invocations.
	*/
	let pending = 0;
	let endReceived = false;
	let exiting = false;
	function maybeExit() {
		if (endReceived && pending === 0 && !exiting) {
			exiting = true;
			/*
				Flush any buffered stdout (e.g. the final reply) before exiting; a
				bare process.exit() can truncate a pending write on a pipe.
			*/
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
				sendError(null, -32700, 'parse error: ' + e.message);
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

/*
	Exported so server.test.js can be shared verbatim and parameterised by PRODUCT.
	Requiring this file does not start the server or touch the bridge folder.
*/
module.exports = { PRODUCT: PRODUCT, PROTOCOL_VERSIONS: PROTOCOL_VERSIONS, renameWithRetry: renameWithRetry };

if (require.main === module) {
	main();
}
