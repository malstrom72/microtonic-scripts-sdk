var DEBUG = true;

var assert;
if (DEBUG) {
	assert = function(condition, message) {
		if (!condition) {
			message = message || "Unknown assertion failure";
			print("Assertion failure: " + message);
			throw new Error(message);
		}
	};
} else {
	assert = function() { };
}

// Simple (not strictly identical) polyfill for ES6 Object.assign
Object.defineProperty(Object, "assign", {
	value: function(target, varArgs) {
		for (var i = 1; i < arguments.length; ++i) {
			var o = arguments[i];
			for (var p in o) {
				if (o.hasOwnProperty(p)) {
					target[p] = o[p];
				}
			}
		}
		return target;
	},
    writable: true,
    configurable: true
});

// Polyfill for ES6 Date.now
Date.now = function() { return new Date().getTime() }

Math.sign = function sign(x) { return (+(x > 0) - +(x < 0)) || +x; };
Math.cbrt = function cbrt(x) { return x < 0 ? -Math.pow(-x, 1 / 3) : Math.pow(x, 1 / 3); };
Math.log10 = function log10(x) { return Math.log(x) * Math.LOG10E; };

function clamp(x, mini, maxi) { return (x < mini ? mini : (x > maxi ? maxi : x)); }
function square(x) { return x * x; }
function cube(x) { return x * x * x; }
function cbrt(x) { return x < 0 ? -Math.pow(-x, 1/3) : Math.pow(x, 1/3); }
function lerp(y0, y1, x) { return y0 + (y1 - y0) * x; }
function bounce(x, min, max) { var d = max - min; return min + Math.abs(Math.abs(x - max) % (2 * d) - d); }
function converge(x, y) { return x + (0.5 - x) * (1 - y); }
function scale(x, x0, x1, y0, y1) { return y0 + (y1 - y0) * (x - x0) / (x1 - x0); }
function fract(x) { return x - Math.floor(x); }
function unescape(s) { return ((s[0] === '"' || s[0] === "'") ? parseNumbstrict(s) : s); }

var random = {
	uniform: Math.random,
	integer: function integer(ceiling) {
		if (typeof ceiling == 'undefined') {
			ceiling = 4294967296;
		}
		return (Math.random() * ceiling) >>> 0;
	},
	normal: function normal(mu, sigma) {
		mu = mu || 0;
		sigma = sigma || 1;
		do { var x = Math.random() } while (x === 0);
		do { var y = Math.random() } while (y === 0);
		return mu + sigma * Math.sqrt(-2 * Math.log(x)) * Math.cos(2 * Math.PI * y);
	}
};

// Utility for class creation with inheritance. Use like this:
//
//  MyClass = createClass({
//      constructor: function(x) { this.field = x },
//      getField: function() { return this.field }
//  })
//
// `super` can be used to access inherited prototype:
//
//  MySubClass = createClass({
//      // no constructor defined = will use MyClass constructor
//      getField: function() { return this.super.getField.call(this) * 100 }
//  }, MyClass)

function createClass(definition, inherit) {
	// Create a new object for this class prototype, which in turn inherits from `inherit` or `Object` prototype.
	var ProtoConstructor = function() { };
	ProtoConstructor.prototype = (inherit || Object).prototype;
	var proto = new ProtoConstructor();

	// Copy definitions to the new class prototype
	Object.assign(proto, definition);

	// Setup super to point to the inherited prototype and create a default constructor if necessary.
	proto.super = ProtoConstructor.prototype;
	if (!proto.hasOwnProperty('constructor')) {
		proto.constructor = (inherit ? function() { inherit.apply(this, arguments) } : function() { });
	}

	// The new prototype object is assigned to the constructor function we return.
	proto.constructor.prototype = proto;
	return proto.constructor;
}


// StringBuilder improves performance of building *really long* strings by maintaining a number of internal buffers
// of increasing maximum lengths. Concats ripple upwards in buffer sizes when the resulting string overflows current
// buffer size.

StringBuilder = createClass({
    constructor: function(s) {
	    var i = 20, b = this.buffers = [ ];
	    do {
		    b[--i] = ''
	    } while (i > 0);
	    if (s) {
		    this.append(s)
	    }
    },
    append: function(s) {
	    for (var i = 0, n = 256, b = this.buffers; (b[i] += s).length >= n && i < 20; n <<= 1, ++i) {
		    s = b[i];
		    b[i] = ''
	    }
    },
    build: function() {
    	var i = 19, b = this.buffers, s = b[i];
	    do {
    		s += b[--i]
    	} while (i > 0);
	    return s
    }
});

this.devLayout = (this.devLayout ? this.devLayout : '');
function displayCushy(name) { performCushyAction('modal.open', name); }
function toggleCushy(name) { performCushyAction('modal.toggle', name); }
function closeCushy() { performCushyAction('modal.close', ''); }

/** Setup GUI Control Backends **/

var cachedMidiConfig = null;
function getCachedMidiConfig() {
	return (!cachedMidiConfig ? (cachedMidiConfig = getElement('midiConfig')) : cachedMidiConfig);
}
function updateMidiConfig(mc) {
	setElement('midiConfig', mc);
	cachedMidiConfig = mc;
}
function flushCachedMidiConfig() { cachedMidiConfig = null; }
function midiConfigId() { return getElementId('midiConfig'); }

MidiConfigSwitch = createClass({
	constructor: function(id) {
		this.id = id;
		this.toggle = new this.Toggler(id);
	},
	get: function() {
		return getCachedMidiConfig()[this.id];
	},
	set: function(v) {
		var mc = getCachedMidiConfig();
		mc[this.id] = !!v;
		updateMidiConfig(mc);
	},
	Toggler: createClass({
		constructor: function(id) { this.id = id; },
		execute: function() {
			var mc = getCachedMidiConfig();
			mc[this.id] = !mc[this.id];
			updateMidiConfig(mc);
		},
		checked: function() {
			return getCachedMidiConfig()[this.id];
		}
	})
})

SelectWithMidi = new MidiConfigSwitch('midiNotesSelectChannel');
PitchedMode = new MidiConfigSwitch('pitchedMode');
MidiNoteOff = new MidiConfigSwitch('supportMidiNoteOff');

ProgramNumber = [
	function() { return ((selected('program') + 1) >= 10 ? 1 : 11); }, 	// first digit
	function() { return (selected('program') + 1) % 10; }, 				// second digit
	10																	// arrow icon
];

// Temporary solution for checkmarks in script popup. Notice that only .checked is used, not .execute etc.
runScript = {
	checked: function(s) {
		s += '.mtscript/';
		return (getCushyVariable('modal.current').substring(0, s.length) === s
				|| devLayout.substring(0, s.length) === s);
	}
};

function callPika(func, args) {
	performCushyAction('jsPikaBackend.call', composeNumbstrict(Array.prototype.slice.call(arguments)));
	return parseNumbstrict(getCushyVariable('jsPikaBackend.result'));
};

function evalPika(code) {
	performCushyAction('jsPikaBackend.eval', composeNumbstrict('' + code));
	return parseNumbstrict(getCushyVariable('jsPikaBackend.result'));
};
