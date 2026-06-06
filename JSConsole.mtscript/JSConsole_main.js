if (!this.jsConsole) {
	jsConsole = {
		realPrint: print,
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
		bridge: { on: false, lastSeq: 0 }
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
	*/
	bridgeBase: function() {
		return (PLATFORM.OS == 'mac')
			? '/Users/Shared/Sonic Charge/Microtonic/jsconsole-bridge/'
			: 'C:/Users/Public/Sonic Charge/Microtonic/jsconsole-bridge/';
	},
	bridgeOn: function() {
		var jc = this;
		var base = jc.bridgeBase();
		jc.bridge.lastSeq = 0;
		try {
			var req = JSON.parse(load(base + 'request.json'));
			if (req && typeof req.seq == 'number') {
				jc.bridge.lastSeq = req.seq;	// skip any stale request from a previous session
			}
		} catch (e) { }
		jc.bridge.on = true;
		/*
			Write a presence file. When the host has already created the folder this
			also provokes Microtonic's one-time folder write-permission prompt now,
			at the user's explicit command, rather than mid-session on the first reply.
		*/
		try {
			save(base + 'bridge.json', JSON.stringify({ ready: true, protocol: 1, time: Math.floor(Date.now()) }));
		} catch (e) { }
		print("Bridge ON.");
		print("Folder: " + base);
	},
	bridgeOff: function() {
		this.bridge.on = false;
		print("Bridge OFF.");
	},
	bridgeStatus: function() {
		var jc = this;
		print("Bridge is " + (jc.bridge.on ? "ON" : "OFF") + ".");
		print("Base: " + jc.bridgeBase());
		if (jc.bridge.on) {
			print("Last handled seq: " + jc.bridge.lastSeq);
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
		/*
			Echo the incoming command into the console so the user sees what the
			host is doing, then capture print() output while still showing it.
		*/
		jc.printNoLF("BRIDGE> " + code + '\n');
		var savedPrint = print;
		print = function(s) {
			captured += s + '\n';
			jc.printNoLF(s + '\n');
			jc.realPrint(s);
		};
		try {
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
		var jc = this;
		if (!jc.bridge.on) {
			return;
		}
		var base = jc.bridgeBase();
		var text;
		try {
			text = load(base + 'request.json');
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
		var r = jc.bridgeEval('' + req.code);
		var response = {
			seq: req.seq,
			ok: r.ok,
			value: r.value,
			output: r.output,
			error: r.error
		};
		try {
			save(base + 'response.json', JSON.stringify(response));
		} catch (e) {
			print("!!! bridge: failed to write response: " + e);
		}
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
		print = this.realPrint;
		handleCushyTrace = this.lastCushyTracer;
		print("SHUTTING DOWN");
		delete jsConsole;
	}
});
