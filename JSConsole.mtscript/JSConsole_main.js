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
		knownGlobals: { }
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
				+ "reset: reinits JS engine and restarts UI\n");
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
		'copy': function() { writeClipboard(jsConsole.output.substr(1)); }
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
			print = jsConsole.printNoMore;
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
