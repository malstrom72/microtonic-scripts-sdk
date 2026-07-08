if (!this.jsConsole) {
	jsConsole = {
		realPrint: print,
		realSave: save,
		realLoad: load,
		lastCushyTracer: this.handleCushyTrace,
		windowPosition: '',
		windowZOrder: '',
		input: '',
		output: '',
		minimized: false,
		moreBuffered: '',
		noMoreBuffered: '',
		bufferMax: 65536,
		prompt: "JS> ",
		multiline: null,
		history: '',
		opacity: 100,
		globals: this,
		knownGlobals: { },
		bridge: { on: false, lastSeq: 0, base: '', token: '', isOwner: false }
	}
}

Object.assign(jsConsole, {
	columns: 80,
	rows: 25,
	moreCounter: 0,
	tabSize: 4,
	outputLimit: 20000,
	commands: {
		'?': function() {
			print("\nEnter any expression to evaluate, result will be stored in _\n"
				+ "Use single ` to enter multi-line code. End with another `\n"
				+ "End line with ; to prevent printing the statement result.\n"
				+ "\n"
				+ "cls: clear screen\n"
				+ "copy: copies console buffer to clipboard\n"
				+ "exit: closes console\n"
				+ "reload: reloads UI\n"
				+ "reset: reinits JS engine and restarts UI\n"
				+ "bridge: show external file-bridge status\n"
				+ "bridge on / bridge off: toggle the external file bridge\n");
		},
		'`': function() {
			if (jsConsole.multiline === null) {
				jsConsole.multiline = '';
			} else {
				var s = jsConsole.multiline;
				jsConsole.multiline = null;
				eval.call(null, s);
			}
		},
		'exit': function() { jsConsole.close(); },
		'reload': function() { jsConsole.reload(); },
		'reset': function() { jsConsole.reset(); },
		'cls': function() { jsConsole.clearScreen(); },
		'copy': function() { writeClipboard(jsConsole.output.substr(1)); },
		'bridge': function() { jsConsole.bridgeStatus(); },
		'bridge on': function() { jsConsole.bridgeOn(); },
		'bridge off': function() { jsConsole.bridgeOff(); }
	},
	clearScreen: function() {
		this.output = '\f';
	},
	updateConfig: function(config) {
		config = config.split(',');
		this.columns = +config[0];
		this.rows = +config[1];
		this.tabSize = +config[2];
		var s = "more (y)? ";
		while (s.length < this.columns - 2) {
			s = ' ' + s;
		}
		this.morePrompt = s;
	},
	startup: function(dims) {
		print = handleCushyTrace = jsConsole.printNoMore;
		this.output = '\f';
		this.input = '';
		this.printNoLF("Javascript Console. Type ? for help.\n\n");
		_ = '';
		for (var key in this.globals) this.knownGlobals[key] = true;
	},
	isWaitingForMore: function() {
		return (this.moreCounter == 0 && this.moreBuffered !== '');
	},
	printNoLF: function(s) {
		var jc = jsConsole;
		jc.output += s;
		if (jc.output.length >= jc.outputLimit) {
			jc.output = "\f" + jc.output.substr(jc.outputLimit / 2);
		}
	},
	printMoreNoLF: function(s) {
		var jc = jsConsole;
		var line = '';
		for (var i = 0; i < s.length && jc.moreCounter > 0; ++i) {
			var c = s[i];
			if (c == '\t') {
				var stop = Math.min(line.length + jc.tabSize - line.length % jc.tabSize, jc.columns);
				while (line.length < stop) {
					line += ' ';
				}
			} else if (c == '\n' || c >= ' ') {
				line += c;
			}
			if (c == '\n' || line.length >= jc.columns) {
				jc.printNoLF(line);
				line = '';
				if (--jc.moreCounter == 0) {
					s = s.substr(i + 1);
					jc.prompt = jc.morePrompt;
				}
			}
		}
		if (jc.moreCounter == 0 && jc.moreBuffered.length < jc.bufferMax) {
			jc.moreBuffered += s;
			if (jc.moreBuffered.length >= jc.bufferMax - 28) {
				jc.moreBuffered = jc.moreBuffered.substr(0, jc.bufferMax - 28) + "\n!!! output buffer overflow\n";
			}
		}
	},
	printMore: function(s) {
		jsConsole.printMoreNoLF(s + '\n');
		jsConsole.realPrint(s);
	},
	printNoMore: function(s) {
		var jc = jsConsole;
		if (jc.isWaitingForMore() || jc.multiline !== null) {
			if (jc.noMoreBuffered.length < jc.bufferMax) {
				jc.noMoreBuffered += s + '\n';
				if (jc.noMoreBuffered.length >= jc.bufferMax - 28) {
					jc.noMoreBuffered = jc.noMoreBuffered.substr(0, jc.bufferMax - 28) + "\n!!! output buffer overflow\n";
				}
			}
		} else {
			jc.printNoLF(s + '\n');
		}
		jc.realPrint(s);
	},
	paste: function() {
		jsConsole.input += readClipboard();
	},
	hitEnter: function() {
		var jc = jsConsole;
		var input = jc.input;
		jc.input = '';
		if (jc.isWaitingForMore()) {
			jc.moreCounter = jc.rows - 1;
			var s = jc.moreBuffered;
			jc.moreBuffered = '';
			if (input == '' || input == 'y') {
				jc.printMoreNoLF(s);
			}
		} else {
			jc.printNoLF(jc.prompt + input + '\n');
			jc.prompt = '';
			print = jsConsole.printMore;
			try {
				jc.moreCounter = jc.rows - 1;
				if (jc.commands[input]) {
					jc.commands[input]();
				} else if (jc.multiline !== null) {
					jc.multiline += input + '\n';
				} else {
					_ = eval.call(null, input);
					if (input.length > 0 && input[input.length - 1] !== ';') {
						var type = typeof _;
						var s = _;
						if (type == 'string') {
							s = JSON.stringify(_);
						} else if (type == 'object') {
							var gotObject = false;
							s = JSON.stringify(_, function(key, value) {
								if (typeof value == 'object' && !Array.isArray(value)) {
									gotObject = true;
								}
								return value;
							});
							if (gotObject) {
								s = JSON.stringify(_, null, 2).replace(/\n/g, '\n    ');
							}
						}
						print('  = ' + s);
					}
				}
			}
			catch (e) {
				print("!!! " + e);
			}
			finally {
				print = jsConsole.printNoMore;
			}
		}
		if (jc.multiline !== null) {
			jc.prompt = '..> ';
		} else if (jc.isWaitingForMore()) {
			jc.prompt = jc.morePrompt;
		} else {
			jc.printNoLF(jc.noMoreBuffered);
			jc.noMoreBuffered = '';
			jc.prompt = "JS> ";
		}
	},
	checkLitter: function() {
		var known = this.knownGlobals;
		for (var key in this.globals) {
			if (!(key in known)) {
				print("New global: " + key);
				known[key] = true;
			}
		}
	},
	/*
		External file bridge. Lets a host process (e.g. an MCP server) side-load
		JavaScript into the live engine through files and read back the result,
		without driving the GUI. The host owns the folder (creates it, writes
		request.json atomically, deletes when done); the bridge only ever reads
		request.json and overwrites response.json inside it. Requests are paired
		with the host by a monotonic `seq`; a request is evaluated only when its
		seq advances past the last one handled.

		Only one Microtonic instance may serve the bridge at a time. bridge.json
		records an `owner` token identifying the instance that currently holds it. If
		`bridge on` finds the file already owned by another instance, it asks the user
		(OK/Cancel) whether to take over; taking over just writes this instance's token
		as the new owner. Each tick, an owner that no longer sees its own token in
		bridge.json stands down -- so the instance that was taken over stops serving,
		and requests are never handled by two engines at once.
	*/
	bridgeDefaultBase: function() {
		return (PLATFORM.OS == 'mac')
			? '/Users/Shared/Sonic Charge/Microtonic/jsconsole-bridge/'
			: 'C:/Users/Public/Sonic Charge/Microtonic/jsconsole-bridge/';
	},
	bridgeToken: function() {
		var jc = this;
		if (!jc.bridge.token) {
			jc.bridge.token = 'jc-' + Math.floor(Date.now()).toString(36) + '-'
				+ Math.floor(Math.random() * 0x7FFFFFFF).toString(36);
		}
		return jc.bridge.token;
	},
	bridgeReadPresence: function() {
		try {
			return JSON.parse(this.realLoad(this.bridge.base + 'bridge.json'));
		} catch (e) {
			return null;
		}
	},
	bridgeClaim: function() {
		var jc = this;
		try {
			jc.realSave(jc.bridge.base + 'bridge.json', JSON.stringify({
				ready: true, protocol: 1, time: Math.floor(Date.now()), owner: jc.bridge.token
			}));
		} catch (e) { }
	},
	bridgeOwnedByOther: function(info) {
		// True when bridge.json names a DIFFERENT instance as the current owner.
		return !!(info && info.ready && info.owner && info.owner !== this.bridge.token);
	},
	bridgeOn: function() {
		var jc = this;
		var base = jc.bridgeDefaultBase();
		jc.bridge.base = base;
		jc.bridgeToken();
		/*
			Single-owner: only one instance serves the bridge. If bridge.json is
			already owned by another instance, ask the user whether to take it over
			rather than silently co-handling every request (which would run each eval
			in both engines and race over response.json).
		*/
		if (jc.bridgeOwnedByOther(jc.bridgeReadPresence())) {
			var answer = display(
				"Another Microtonic instance is currently serving the JSConsole bridge.\n\n"
					+ "Take it over? The other instance will stop serving the bridge.",
				'warning', 'ok cancel', 'cancel');
			if (answer != 'ok') {
				print("Bridge left with the other instance.");
				return;
			}
			print("Taking over the bridge from the other instance.");
		}
		jc.bridge.lastSeq = 0;
		try {
			var req = JSON.parse(jc.realLoad(base + 'request.json'));
			if (req && typeof req.seq == 'number') {
				jc.bridge.lastSeq = req.seq;	// skip any stale request from a previous session
			}
		} catch (e) { }
		/*
			Claim ownership (this also provokes Microtonic's one-time folder
			write-permission prompt now, at the user's explicit command, rather than
			mid-session on the first reply).
		*/
		jc.bridgeClaim();
		jc.bridge.isOwner = true;
		jc.bridge.on = true;
		print("Bridge ON.");
		print("Folder: " + base);
	},
	bridgeRelease: function() {
		var jc = this;
		// If we currently own the bridge, clear the owner in bridge.json so another
		// instance can take over cleanly. Used by both `bridge off` and window
		// shutdown (closing the window must release the claim, otherwise the stale
		// token makes the next `bridge on` elsewhere prompt to take over a dead owner).
		if (jc.bridge.on && jc.bridge.isOwner && jc.bridge.base) {
			try {
				jc.realSave(jc.bridge.base + 'bridge.json', JSON.stringify({
					ready: false, protocol: 1, time: Math.floor(Date.now()), owner: null
				}));
			} catch (e) { }
		}
		jc.bridge.on = false;
		jc.bridge.isOwner = false;
	},
	bridgeOff: function() {
		this.bridgeRelease();
		print("Bridge OFF.");
	},
	bridgeStatus: function() {
		var jc = this;
		print("Bridge is " + (jc.bridge.on ? "ON" : "OFF") + ".");
		print("Base: " + (jc.bridge.base || jc.bridgeDefaultBase()));
		if (jc.bridge.on) {
			print("Owner: this instance (" + jc.bridge.token + ").");
			print("Last handled seq: " + jc.bridge.lastSeq);
		} else {
			var info = jc.bridgeReadPresence();
			if (jc.bridgeOwnedByOther(info)) {
				print("Another instance currently owns the bridge (owner " + info.owner + ").");
			}
		}
	},
	bridgeStringify: function(v) {
		var type = typeof v;
		if (type == 'string') {
			return JSON.stringify(v);
		} else if (type == 'object' && v !== null) {
			var gotObject = false;
			JSON.stringify(v, function(key, value) {
				if (typeof value == 'object' && value !== null && !Array.isArray(value)) {
					gotObject = true;
				}
				return value;
			});
			return gotObject ? JSON.stringify(v, null, 2) : JSON.stringify(v);
		}
		return '' + v;
	},
	bridgeEval: function(code) {
		var jc = this;
		var captured = '';
		var result = { ok: true, value: '', output: '', error: '' };
		var savedPrint = print;
		/*
			Echo the incoming command into the console so the user sees what the
			host is doing, then capture print() output while still showing it.
		*/
		try {
			jc.printNoLF("BRIDGE> " + code + '\n');
			print = function(s) {
				captured += s + '\n';
				jc.printNoLF(s + '\n');
				jc.realPrint(s);
			};
			_ = eval.call(null, code);
			result.value = jc.bridgeStringify(_);
			jc.printNoLF("  = " + result.value + '\n');
		} catch (e) {
			result.ok = false;
			result.error = '' + e;
			jc.printNoLF("!!! " + result.error + '\n');
		}
		finally {
			print = savedPrint;
		}
		result.output = captured;
		return result;
	},
	bridgeTick: function() {
		try {
			var jc = this;
			if (!jc.bridge.on) {
				return;
			}
			var base = jc.bridge.base;
			if (!base) {
				return;
			}
			/*
				If another instance has taken ownership (its token is now in
				bridge.json), stand down instead of co-handling requests.
			*/
			if (jc.bridgeOwnedByOther(jc.bridgeReadPresence())) {
				print("Bridge stood down: another instance has taken over.");
				jc.bridge.isOwner = false;
				jc.bridge.on = false;
				return;
			}
			var text;
			try {
				text = jc.realLoad(base + 'request.json');
			} catch (e) {
				return;		// folder / file not present yet, keep waiting
			}
			var req;
			try {
				req = JSON.parse(text);
			} catch (e) {
				return;		// partial or invalid write, try again next tick
			}
			if (!req || typeof req.seq != 'number' || req.seq <= jc.bridge.lastSeq) {
				return;
			}
			jc.bridge.lastSeq = req.seq;
			var response;
			try {
				var r = jc.bridgeEval('' + req.code);
				response = {
					seq: req.seq,
					ok: r.ok,
					value: r.value,
					output: r.output,
					error: r.error
				};
			} catch (e) {
				response = {
					seq: req.seq,
					ok: false,
					value: '',
					output: '',
					error: '' + e
				};
			}
			try {
				jc.realSave(base + 'response.json', JSON.stringify(response));
			} catch (e) {
				print("!!! Failed to write bridge response: " + e);
			}
		} catch (e) { }
	},
	minimize: {
		execute: function() { jsConsole.minimized = !jsConsole.minimized; },
		checked: function() { return !jsConsole.minimized; }
	},
	fpsLastTick: Date.now(),
	fpsCounter: 0,
	fpsTick: function() {
		++this.fpsCounter;
		var now = Date.now();
		if (now >= this.fpsLastTick + 1000) {
			this.fps = Math.round(this.fpsCounter * 1000 / (now - this.fpsLastTick));
			this.fpsLastTick = now;
			this.fpsCounter = 0;
		}
	},
	fps: 0.0,
	close: function() { devLayout = ''; },
	reload: function() { performCushyAction('reload'); print("ok"); },
	reset: function() { performCushyAction('reload', 'reset'); },
	shutdown: function() {
		this.bridgeRelease();	// release any bridge claim we own so another instance can take it cleanly
		print = this.realPrint;
		handleCushyTrace = this.lastCushyTracer;
		print("SHUTTING DOWN");
		delete jsConsole;
	}
});
