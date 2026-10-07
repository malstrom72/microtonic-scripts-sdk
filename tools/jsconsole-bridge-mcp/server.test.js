'use strict';
//
// Integration tests for the JSConsole bridge MCP server.
//
// Shared verbatim between the Microtonic and Synplant Scripts SDKs: all
// product-specific names (server name, tool prefix, console name) come from the
// PRODUCT block exported by server.js.
//
// Run with:  node --test   (from this directory)
//        or: node --test tools/jsconsole-bridge-mcp/
//
// Each test spawns the real server.js over stdio, talks newline-delimited
// JSON-RPC to it, and uses a tiny in-process "fake bridge" (a setInterval that
// watches request.json and writes response.json) to stand in for the product.
// BRIDGE_BASE points the server at a throwaway temp folder so nothing touches
// the real bridge directory.
//

const test = require('node:test');
const assert = require('node:assert');
const { spawn } = require('node:child_process');
const fs = require('node:fs');
const os = require('node:os');
const path = require('node:path');

const SERVER = path.join(__dirname, 'server.js');
const PRODUCT = require('./server.js').PRODUCT;
const EVAL = PRODUCT.toolPrefix + '_eval';
const STATUS = PRODUCT.toolPrefix + '_status';
const RELOAD = PRODUCT.toolPrefix + '_reload';
const READY = PRODUCT.toolPrefix.toUpperCase() + '_READY';
const WAIT = PRODUCT.toolPrefix.toUpperCase() + '_WAIT';

function escapeRegExp(s) {
	return s.replace(/[.*+?^${}()|[\]\\]/g, '\\$&');
}

// Every <prefix>_<word> mentioned in a message must be a tool this server provides.
function assertOnlyRealTools(text) {
	const mentioned = text.match(new RegExp('\\b' + PRODUCT.toolPrefix + '_[a-z]+', 'g')) || [];
	mentioned.forEach(function (name) {
		assert.ok([EVAL, STATUS, RELOAD].indexOf(name) >= 0, 'mentions nonexistent tool ' + name);
	});
}

function freshBase() {
	return fs.mkdtempSync(path.join(os.tmpdir(), PRODUCT.toolPrefix + '-bridge-test-'));
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

// Stand-in for the product's JS Console bridge: watch request.json, and when a new
// seq appears, write a response built by `handler(req)`. Records seqs seen.
// Polls every 50ms like the real console's bridge tick.
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
	}, 50);
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
	assert.equal(init.result.serverInfo.name, PRODUCT.serverName);
	assert.equal(init.result.protocolVersion, '2024-11-05');

	const list = await s.request('tools/list');
	const names = list.result.tools.map(function (x) { return x.name; }).sort();
	assert.deepEqual(names, [EVAL, RELOAD, STATUS]);
});

test(EVAL + ' round-trips a value and captured output', async function (t) {
	const base = freshBase();
	const s = startServer(base);
	const fb = startFakeBridge(base, function (req) {
		return { value: 'echo:' + req.code, output: 'traced\n' };
	});
	t.after(function () { fb.stop(); s.kill(); cleanup(base); });

	await s.request('initialize', { capabilities: {} });
	const r = await s.request('tools/call', { name: EVAL, arguments: { code: '6*7' } });
	assert.equal(r.result.isError, false);
	const text = r.result.content[0].text;
	assert.match(text, /value: echo:6\*7/);
	assert.match(text, /traced/);
});

test(EVAL + ' surfaces engine errors as isError', async function (t) {
	const base = freshBase();
	const s = startServer(base);
	const fb = startFakeBridge(base, function () {
		return { ok: false, value: '', error: 'ReferenceError: x is not defined' };
	});
	t.after(function () { fb.stop(); s.kill(); cleanup(base); });

	await s.request('initialize', { capabilities: {} });
	const r = await s.request('tools/call', { name: EVAL, arguments: { code: 'x' } });
	assert.equal(r.result.isError, true);
	assert.match(r.result.content[0].text, /error: ReferenceError/);
});

test(EVAL + ' times out when no bridge replies', async function (t) {
	const base = freshBase();
	const s = startServer(base);
	t.after(function () { s.kill(); cleanup(base); });

	await s.request('initialize', { capabilities: {} });
	const r = await s.request('tools/call', { name: EVAL, arguments: { code: '1', timeout_ms: 300 } });
	assert.equal(r.result.isError, true);
	assert.match(r.result.content[0].text, /timed out/);
	assert.match(r.result.content[0].text, new RegExp('Run ' + STATUS + ' to probe'));
	assertOnlyRealTools(r.result.content[0].text);
});

test(EVAL + ' timeout reports a stale reply seq', async function (t) {
	const base = freshBase();
	const s = startServer(base);
	t.after(function () { s.kill(); cleanup(base); });

	await s.request('initialize', { capabilities: {} });
	fs.writeFileSync(path.join(base, 'response.json'),
		JSON.stringify({ seq: 1, ok: true, value: '', output: '', error: '' }));
	const r = await s.request('tools/call', { name: EVAL, arguments: { code: '1', timeout_ms: 300 } });
	assert.equal(r.result.isError, true);
	assert.match(r.result.content[0].text, /Last reply seq is still 1 while this request seq is \d+\./);
});

test(RELOAD + ' polls until the third probe reports ready', async function (t) {
	const base = freshBase();
	let probes = 0;
	const fb = startFakeBridge(base, function (req) {
		if (req.code === "performCushyAction('reload')") {
			return { value: 'true' };
		}
		probes++;
		return { value: probes === 3 ? '"' + READY + '"' : '"' + WAIT + '"' };
	});
	const s = startServer(base);
	t.after(function () { fb.stop(); s.kill(); cleanup(base); });

	await s.request('initialize', { capabilities: {} });
	const r = await s.request('tools/call', {
		name: RELOAD,
		arguments: { until: 'myScript.version === 2', timeout_ms: 2000 }
	});
	assert.equal(r.result.isError, false);
	assert.equal(probes, 3);
	assert.match(r.result.content[0].text, /reload complete after \d+ms/);
	assert.match(r.result.content[0].text, /predicate satisfied/);
});

test(RELOAD + ' timeout points to diagnostics, not a reset', async function (t) {
	const base = freshBase();
	const fb = startFakeBridge(base, function (req) {
		return { value: req.code === "performCushyAction('reload')" ? 'true' : '"' + WAIT + '"' };
	});
	const s = startServer(base);
	t.after(function () { fb.stop(); s.kill(); cleanup(base); });

	await s.request('initialize', { capabilities: {} });
	const r = await s.request('tools/call', {
		name: RELOAD,
		arguments: { until: 'false', timeout_ms: 350 }
	});
	assert.equal(r.result.isError, true);
	assert.match(r.result.content[0].text, /did not become true within 350ms/);
	assert.match(r.result.content[0].text, new RegExp(escapeRegExp(PRODUCT.consoleName) + ' output'));
	assertOnlyRealTools(r.result.content[0].text);
	assert.doesNotMatch(r.result.content[0].text, /full reset|performCushyAction\(.+reset/);
});

test(RELOAD + ' without until warns that the new code may not be live', async function (t) {
	const base = freshBase();
	const fb = startFakeBridge(base, function () { return { value: 'true' }; });
	const s = startServer(base);
	t.after(function () { fb.stop(); s.kill(); cleanup(base); });

	await s.request('initialize', { capabilities: {} });
	const r = await s.request('tools/call', { name: RELOAD, arguments: {} });
	assert.equal(r.result.isError, false);
	assert.match(r.result.content[0].text, /asynchronous/);
	assert.match(r.result.content[0].text, /may not be live yet/);
	assert.match(r.result.content[0].text, /Pass `until`/);
});

test(RELOAD + ' wraps a throwing until predicate and treats it as not ready', async function (t) {
	const base = freshBase();
	let sawWrappedProbe = false;
	const fb = startFakeBridge(base, function (req) {
		if (req.code === "performCushyAction('reload')") {
			return { value: 'true' };
		}
		sawWrappedProbe = /try\{return \(missingScript\.ready\)/.test(req.code)
			&& req.code.indexOf('catch(e){return "' + WAIT + '";}') >= 0;
		return { value: '"' + WAIT + '"' };
	});
	const s = startServer(base);
	t.after(function () { fb.stop(); s.kill(); cleanup(base); });

	await s.request('initialize', { capabilities: {} });
	const r = await s.request('tools/call', {
		name: RELOAD,
		arguments: { until: 'missingScript.ready', timeout_ms: 350 }
	});
	assert.equal(sawWrappedProbe, true);
	assert.equal(r.result.isError, true);
	assert.match(r.result.content[0].text, /did not become true/);
	assert.doesNotMatch(r.result.content[0].text, /ReferenceError/);
});

test(STATUS + ' probes liveness, not just the presence file', async function (t) {
	const base = freshBase();
	const s = startServer(base);
	t.after(function () { s.kill(); cleanup(base); });

	await s.request('initialize', { capabilities: {} });

	// Nothing answering and no presence file → NOT RESPONDING, no presence.
	const none = await s.request('tools/call', { name: STATUS }, 4000);
	assert.match(none.result.content[0].text, /NOT RESPONDING and no presence file/);

	// A stale presence file with nothing answering → still NOT RESPONDING. (The old
	// code wrongly reported "attached: yes" here — exactly the bug that misled us.)
	fs.writeFileSync(path.join(base, 'bridge.json'),
		JSON.stringify({ ready: true, protocol: 1, time: Date.now() - 60000, owner: 'jc-test' }));
	const stale = await s.request('tools/call', { name: STATUS }, 4000);
	assert.match(stale.result.content[0].text, /NOT RESPONDING/);
	// Age comes from bridge.json's own `time` stamp, not the file's mtime.
	assert.match(stale.result.content[0].text, /announced 6\ds ago/);
	assert.doesNotMatch(stale.result.content[0].text, /LIVE/);

	// A live bridge answering the probe → LIVE.
	const fb = startFakeBridge(base, function (req) { return { value: '' + req.code }; });
	t.after(function () { fb.stop(); });
	const live = await s.request('tools/call', { name: STATUS }, 4000);
	assert.match(live.result.content[0].text, /LIVE/);
});

test('seq strictly increases across calls', async function (t) {
	const base = freshBase();
	const s = startServer(base);
	const fb = startFakeBridge(base, function (req) { return { value: '' + req.seq }; });
	t.after(function () { fb.stop(); s.kill(); cleanup(base); });

	await s.request('initialize', { capabilities: {} });
	await s.request('tools/call', { name: EVAL, arguments: { code: 'a' } });
	await s.request('tools/call', { name: EVAL, arguments: { code: 'b' } });

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
	const pending = s.request('tools/call', { name: EVAL, arguments: { code: 'slow' } });
	s.endStdin(); // close stdin immediately; server must wait for the in-flight eval

	const r = await pending;
	assert.equal(r.result.isError, false);
	assert.match(r.result.content[0].text, /late:slow/);

	const code = await s.waitExit();
	assert.equal(code, 0);
});

test('overlapping calls run one at a time and each gets its own reply', async function (t) {
	const base = freshBase();
	const s = startServer(base);
	const fb = startFakeBridge(base, function (req) { return { value: 'r:' + req.code }; }, 100);
	t.after(function () { fb.stop(); s.kill(); cleanup(base); });

	await s.request('initialize', { capabilities: {} });
	// Sent back to back, so without serialization the second request would overwrite
	// the first before the bridge's next poll, and the first call would time out.
	const results = await Promise.all(['a', 'b', 'c'].map(function (code) {
		return s.request('tools/call', { name: EVAL, arguments: { code: code, timeout_ms: 3000 } }, 8000);
	}));
	results.forEach(function (r, i) {
		assert.equal(r.result.isError, false, r.result.content[0].text);
		assert.match(r.result.content[0].text, new RegExp('value: r:' + 'abc'.charAt(i) + '$', 'm'));
	});
	assert.equal(fb.seqs.length, 3, 'the bridge should have seen every request');
});

test(EVAL + ' timeout withdraws the request so a blocked bridge does not run it later', async function (t) {
	const base = freshBase();
	const s = startServer(base);
	let fb = null;
	t.after(function () { if (fb) fb.stop(); s.kill(); cleanup(base); });

	await s.request('initialize', { capabilities: {} });
	const r = await s.request('tools/call', { name: EVAL, arguments: { code: 'late', timeout_ms: 300 } });
	assert.equal(r.result.isError, true);
	assert.match(r.result.content[0].text, /request was withdrawn/);
	assert.equal(fs.existsSync(path.join(base, 'request.json')), false);

	// The bridge comes back (e.g. a modal dialog was dismissed): nothing to run.
	fb = startFakeBridge(base, function () { return { value: 'ran' }; });
	await new Promise(function (resolve) { setTimeout(resolve, 300); });
	assert.deepEqual(fb.seqs, []);
});

test(STATUS + ' still reports the last request seq after its probe is withdrawn', async function (t) {
	const base = freshBase();
	const s = startServer(base);
	t.after(function () { s.kill(); cleanup(base); });

	await s.request('initialize', { capabilities: {} });
	const r = await s.request('tools/call', { name: STATUS }, 4000);
	assert.match(r.result.content[0].text, /NOT RESPONDING/);
	assert.match(r.result.content[0].text, /last request seq: \d+/);
});

test(STATUS + ' reports BUSY at once instead of queueing behind a call in progress', async function (t) {
	const base = freshBase();
	const s = startServer(base);
	t.after(function () { s.kill(); cleanup(base); });

	await s.request('initialize', { capabilities: {} });
	const slow = s.request('tools/call', { name: EVAL, arguments: { code: 'slow', timeout_ms: 3000 } }, 8000);
	await new Promise(function (resolve) { setTimeout(resolve, 100); });
	const started = Date.now();
	const r = await s.request('tools/call', { name: STATUS }, 4000);
	assert.ok(Date.now() - started < 1000, 'status took ' + (Date.now() - started) + 'ms');
	assert.match(r.result.content[0].text, /bridge: BUSY/);
	assert.doesNotMatch(r.result.content[0].text, /NOT RESPONDING/);
	await slow;
});

test('a queued call times out on its own timeout_ms without sending anything', async function (t) {
	const base = freshBase();
	const s = startServer(base);
	t.after(function () { s.kill(); cleanup(base); });

	await s.request('initialize', { capabilities: {} });
	const first = s.request('tools/call', { name: EVAL, arguments: { code: 'a', timeout_ms: 2000 } }, 8000);
	const started = Date.now();
	const r = await s.request('tools/call', { name: EVAL, arguments: { code: 'b', timeout_ms: 300 } }, 8000);
	assert.ok(Date.now() - started < 1500, 'queued call took ' + (Date.now() - started) + 'ms');
	assert.equal(r.result.isError, true);
	assert.match(r.result.content[0].text, /waiting for an earlier bridge call/);
	await first;
});

const renameWithRetry = require('./server.js').renameWithRetry;

function renameError(code) {
	const e = new Error(code + ': operation not permitted, rename');
	e.code = code;
	return e;
}

test('renameWithRetry retries EPERM (a reader has request.json open on Windows)', async function () {
	let calls = 0;
	await renameWithRetry('a', 'b', function () {
		if (++calls < 3) throw renameError('EPERM');
	});
	assert.equal(calls, 3);
});

test('renameWithRetry does not retry other errors', async function () {
	let calls = 0;
	await assert.rejects(renameWithRetry('a', 'b', function () {
		++calls;
		throw renameError('ENOENT');
	}), /ENOENT/);
	assert.equal(calls, 1);
});

test('renameWithRetry gives up when its time limit runs out', async function () {
	const start = Date.now();
	await assert.rejects(renameWithRetry('a', 'b', function () {
		throw renameError('EBUSY');
	}, 300), /EBUSY/);
	const elapsed = Date.now() - start;
	assert.ok(elapsed >= 250 && elapsed < 2000, 'elapsed ' + elapsed + 'ms');
});
