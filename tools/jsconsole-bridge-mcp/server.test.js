'use strict';
//
// Integration tests for the JSConsole bridge MCP server.
//
// Run with:  node --test   (from this directory)
//        or: node --test tools/jsconsole-bridge-mcp/
//
// Each test spawns the real server.js over stdio, talks newline-delimited
// JSON-RPC to it, and uses a tiny in-process "fake bridge" (a setInterval that
// watches request.json and writes response.json) to stand in for Microtonic.
// BRIDGE_BASE points the server at a throwaway temp folder so nothing touches
// the real machine-global bridge directory.
//

const test = require('node:test');
const assert = require('node:assert');
const { spawn } = require('node:child_process');
const fs = require('node:fs');
const os = require('node:os');
const path = require('node:path');

const SERVER = path.join(__dirname, 'server.js');

function freshBase() {
	return fs.mkdtempSync(path.join(os.tmpdir(), 'mt-bridge-test-'));
}

// Spawn server.js and return a small JSON-RPC client over its stdio.
function startServer(base) {
	const child = spawn(process.execPath, [SERVER], {
		env: Object.assign({}, process.env, { BRIDGE_BASE: base }),
		stdio: ['pipe', 'pipe', 'pipe']
	});
	child.stdout.setEncoding('utf8');

	const pending = new Map();
	let buffer = '';
	child.stdout.on('data', function (chunk) {
		buffer += chunk;
		let nl;
		while ((nl = buffer.indexOf('\n')) >= 0) {
			const line = buffer.slice(0, nl).trim();
			buffer = buffer.slice(nl + 1);
			if (!line) continue;
			let msg;
			try { msg = JSON.parse(line); } catch (e) { continue; }
			if (msg.id != null && pending.has(msg.id)) {
				const entry = pending.get(msg.id);
				pending.delete(msg.id);
				entry(msg);
			}
		}
	});

	let nextId = 1;
	function request(method, params, timeoutMs) {
		const id = nextId++;
		return new Promise(function (resolve, reject) {
			const timer = setTimeout(function () {
				pending.delete(id);
				reject(new Error('timed out waiting for response to ' + method));
			}, timeoutMs || 5000);
			pending.set(id, function (msg) { clearTimeout(timer); resolve(msg); });
			child.stdin.write(JSON.stringify({ jsonrpc: '2.0', id: id, method: method, params: params || {} }) + '\n');
		});
	}

	return {
		child: child,
		request: request,
		endStdin: function () { child.stdin.end(); },
		kill: function () { try { child.kill(); } catch (e) {} },
		waitExit: function () { return new Promise(function (r) { child.on('exit', function (code) { r(code); }); }); }
	};
}

// Stand-in for Microtonic's JSConsole bridge: watch request.json, and when a new
// seq appears, write a response built by `handler(req)`. Records seqs seen.
function startFakeBridge(base, handler, delayMs) {
	let last = 0;
	const seqs = [];
	const reqPath = path.join(base, 'request.json');
	const resPath = path.join(base, 'response.json');
	const timer = setInterval(function () {
		let req;
		try { req = JSON.parse(fs.readFileSync(reqPath, 'utf8')); } catch (e) { return; }
		if (!req || typeof req.seq !== 'number' || req.seq <= last) return;
		last = req.seq;
		seqs.push(req.seq);
		const write = function () {
			const base2 = { seq: req.seq, ok: true, value: '', output: '', error: '' };
			fs.writeFileSync(resPath, JSON.stringify(Object.assign(base2, handler(req))));
		};
		if (delayMs) setTimeout(write, delayMs); else write();
	}, 10);
	return { stop: function () { clearInterval(timer); }, seqs: seqs };
}

function cleanup(base) {
	try { fs.rmSync(base, { recursive: true, force: true }); } catch (e) {}
}

test('initialize echoes protocol version and advertises tools', async function (t) {
	const base = freshBase();
	const s = startServer(base);
	t.after(function () { s.kill(); cleanup(base); });

	const init = await s.request('initialize', { protocolVersion: '2024-11-05', capabilities: {} });
	assert.equal(init.result.serverInfo.name, 'microtonic-jsconsole-bridge');
	assert.equal(init.result.protocolVersion, '2024-11-05');

	const list = await s.request('tools/list');
	const names = list.result.tools.map(function (x) { return x.name; }).sort();
	assert.deepEqual(names, ['mt_eval', 'mt_status']);
});

test('mt_eval round-trips a value and captured output', async function (t) {
	const base = freshBase();
	const s = startServer(base);
	const fb = startFakeBridge(base, function (req) {
		return { value: 'echo:' + req.code, output: 'traced\n' };
	});
	t.after(function () { fb.stop(); s.kill(); cleanup(base); });

	await s.request('initialize', { capabilities: {} });
	const r = await s.request('tools/call', { name: 'mt_eval', arguments: { code: '6*7' } });
	assert.equal(r.result.isError, false);
	const text = r.result.content[0].text;
	assert.match(text, /value: echo:6\*7/);
	assert.match(text, /traced/);
});

test('mt_eval surfaces engine errors as isError', async function (t) {
	const base = freshBase();
	const s = startServer(base);
	const fb = startFakeBridge(base, function () {
		return { ok: false, value: '', error: 'ReferenceError: x is not defined' };
	});
	t.after(function () { fb.stop(); s.kill(); cleanup(base); });

	await s.request('initialize', { capabilities: {} });
	const r = await s.request('tools/call', { name: 'mt_eval', arguments: { code: 'x' } });
	assert.equal(r.result.isError, true);
	assert.match(r.result.content[0].text, /error: ReferenceError/);
});

test('mt_eval times out when no bridge replies', async function (t) {
	const base = freshBase();
	const s = startServer(base);
	t.after(function () { s.kill(); cleanup(base); });

	await s.request('initialize', { capabilities: {} });
	const r = await s.request('tools/call', { name: 'mt_eval', arguments: { code: '1', timeout_ms: 300 } });
	assert.equal(r.result.isError, true);
	assert.match(r.result.content[0].text, /timed out/);
});

test('mt_status reflects bridge.json presence', async function (t) {
	const base = freshBase();
	const s = startServer(base);
	t.after(function () { s.kill(); cleanup(base); });

	await s.request('initialize', { capabilities: {} });

	const before = await s.request('tools/call', { name: 'mt_status' });
	assert.match(before.result.content[0].text, /attached: no/);

	fs.writeFileSync(path.join(base, 'bridge.json'),
		JSON.stringify({ ready: true, protocol: 1, time: Date.now() }));

	const after = await s.request('tools/call', { name: 'mt_status' });
	assert.match(after.result.content[0].text, /attached: yes/);
});

test('seq strictly increases across calls', async function (t) {
	const base = freshBase();
	const s = startServer(base);
	const fb = startFakeBridge(base, function (req) { return { value: '' + req.seq }; });
	t.after(function () { fb.stop(); s.kill(); cleanup(base); });

	await s.request('initialize', { capabilities: {} });
	await s.request('tools/call', { name: 'mt_eval', arguments: { code: 'a' } });
	await s.request('tools/call', { name: 'mt_eval', arguments: { code: 'b' } });

	assert.ok(fb.seqs.length >= 2, 'bridge should have seen two requests');
	assert.ok(fb.seqs[1] > fb.seqs[0], 'second seq must be strictly greater');
});

test('does not exit (or truncate the reply) while an eval is in flight', async function (t) {
	const base = freshBase();
	const s = startServer(base);
	// Bridge replies only after 250ms, well after we close stdin.
	const fb = startFakeBridge(base, function (req) { return { value: 'late:' + req.code }; }, 250);
	t.after(function () { fb.stop(); s.kill(); cleanup(base); });

	await s.request('initialize', { capabilities: {} });
	const pending = s.request('tools/call', { name: 'mt_eval', arguments: { code: 'slow' } });
	s.endStdin(); // close stdin immediately; server must wait for the in-flight eval

	const r = await pending;
	assert.equal(r.result.isError, false);
	assert.match(r.result.content[0].text, /late:slow/);

	const code = await s.waitExit();
	assert.equal(code, 0);
});
