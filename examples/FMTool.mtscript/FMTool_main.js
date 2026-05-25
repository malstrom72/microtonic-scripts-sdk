if (!this.fmTool) {
	fmTool = {
		windowPosition: '',
		windowZOrder: ''
	};
}

Object.assign(fmTool, {
	MAX_MOD_OCTS: 4,
	NOTE_NAMES: [ 'C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B' ],
	besselI0: function(alpha) {
		var EPSILON = 1e-011, fact = 1, x2 = alpha / 2, p = x2, t = p * p, s = 1 + t;
		for (var k = 2; t > EPSILON; ++k) {
			p *= x2;
			fact *= k;
			t = p / fact;
			t *= t;
			s += t;
		}
		return s;
	},
	sinh: function(x) {
		var y = Math.exp(x);
		return (y - 1 / y) / 2;
	},
	modAmountToOcts: function(amount, velScale) { var x = (amount * 2 - 1) * velScale; return x * Math.abs(x) * fmTool.MAX_MOD_OCTS; },
	modOctsToAmount: function(octs) { var x = Math.sqrt(Math.abs(octs) / fmTool.MAX_MOD_OCTS) * (octs < 0 ? -1 : 1); return (x + 1) / 2; },
	modRateToIndexFuncs: {
		'decay': function(oscFreq, modRate) { return (670.4028639556964 * Math.pow(5, 1 - 3 * oscFreq) * modRate) / (Math.pow(8, oscFreq) * (103 - 100 * modRate)); },
		'sine': function(oscFreq, modRate) { return Math.pow(10, 3.5 * modRate - 3 * oscFreq - 1.5) * modRate; },
		'noise': function(oscFreq, modRate) { return Math.pow(10, 4 * modRate - 3 * oscFreq - 1) * modRate; }
	},
	fmPitchFuncs: {
		'decay': function(fmIndex) { return 0.0; },
		'sine': function(fmIndex) { return (fmIndex < 0) ? fmIndex : Math.log(fmTool.besselI0(Math.LN2 * fmIndex)) / Math.LN2; }, // return negative for negative to make it monotonic for bisect to work
		'noise': function(fmIndex) { return (fmIndex < 0.000001) ? 0 : Math.log((fmTool.sinh(fmIndex * Math.LN2) / (fmIndex * Math.LN2))) / Math.LN2; }, // notice that this is for ideal uniform random noise, filtered noise in Microtonic behave slightly different, but I can't be bothered :)
	},
	GUIParam: createClass({
		constructor: function(undoText, isInteger) {
			this.undoDescription = undoText;
			this.value = 0.0;
			this.isInteger = isInteger;
		},
		set: function(v) {
			saveUndo(this.undoDescription, true);
			this.value = (this.isInteger ? Math.round(+v) : +v);
			fmTool.updatePatch();
		},
		get: function() { return '' + this.value; },
		human: function() { return '' + Math.round(this.value * 100) / 100; },
		touch: function(mouseDown) {
			if (mouseDown) {
				++fmTool.reloadBlocked;
			} else {
				--fmTool.reloadBlocked;
			}
		}
	}),
	SourceParam: createClass({
		constructor: function(paramName) { this.paramName = paramName; },
		get: function() { return getParam(this.paramName + '.' + (selected('channel') + 1)); }
	})
});

Object.assign(fmTool, {
	modRateToIndexFunc: fmTool.fmPitchFuncs.decay,
	fmPitchFunc: fmTool.fmPitchFuncs.decay,
	
	clipWarning: false,
	updatePatch: function() {
		var dp = getElement('drumPatch');

		fmTool.clipWarning = false;        
		function clip(x) {
			if (x < 0) {
				if (x < -0.00001) {
					fmTool.clipWarning = true;
				}
				x = 0;
			} else if (x > 1) {
				if (x > 1.00001) {
					fmTool.clipWarning = true;
				}
				x = 1;
			}
			return x;
		}
		
		// bisect is a fairly slow root finder but it is guaranteed to find a root if ((fn(low) < y) != (fn(high) < y)) (otherwise it is a game of luck).
		function bisect(fn, y, low, high, maxSteps) {
			maxSteps = maxSteps || 50;
			if (fn(high) < fn(low)) {
				var temp = low;
				low = high;
				high = temp;
			}
			var x = low + (high - low) * 0.5;
			for (var i = 0; i < maxSteps && x != low && x != high; ++i) {
				if (fn(x) < y) {
					low = x;
				} else {
					high = x;
				}
				x = low + (high - low) * 0.5;
			}
			return x;
		}

		var centerPitch = (fmTool.pitchOct.value + fmTool.pitchSemi.value / 12 + fmTool.pitchCent.value / 1200);
		var fmPitch = fmTool.fmPitchFunc(fmTool.index.value);
		dp.OscFreq = (centerPitch - fmPitch) * OCTAVE_STEP + FREQ_VALUE_C4;
		dp.ModAmt = fmTool.modOctsToAmount(fmTool.index.value * fmTool.indexPolarity);
		var ratio = fmTool.ratioCoarse.value + fmTool.ratioFine.value * Math.abs(fmTool.ratioFine.value * 2);
		ratio = (ratio < 0 ? Math.pow(2, ratio) : (ratio + 1));
		var centerFreq = centerPitch * OCTAVE_STEP + FREQ_VALUE_C4;
		dp.ModRate = bisect(function(x) { return fmTool.modRateToIndexFunc(centerFreq, x); }, ratio, -0.02, 1.02);	// max beyond 1.02 doesn't work for decay (not monotonic)
		if (fmTool.unaccentedTransposition.value == 0) {
			dp.ModVel = 0;
		} else {
			var unaccentedFMIndex = bisect(fmTool.fmPitchFunc, fmTool.unaccentedTransposition.value / 12 + fmPitch, -0.02, fmTool.MAX_MOD_OCTS); // max fm index is same as max mod octs
			var biAmount = dp.ModAmt * 2 - 1;
			dp.ModVel = 1 - (fmTool.modOctsToAmount(unaccentedFMIndex) * 2 - 1) / Math.abs(biAmount);
			dp.ModVel = clip(dp.ModVel);
		}
		dp.OscFreq = clip(dp.OscFreq);
		dp.ModAmt = clip(dp.ModAmt);
		dp.ModRate = clip(dp.ModRate);
		setElement('drumPatch', dp);
	},

	reloadBlocked: 0,
	indexPolarity: 1,
	
	reloadPatch: function() {
		if (fmTool.reloadBlocked === 0) {
			var dp = getElement('drumPatch');
			
			var modeString = paramText('ModMode.1', dp.ModMode).toLowerCase();
			fmTool.fmPitchFunc = fmTool.fmPitchFuncs[modeString];
			fmTool.modRateToIndexFunc = fmTool.modRateToIndexFuncs[modeString];
			var modOcts = fmTool.modAmountToOcts(dp.ModAmt, 1);
			fmTool.indexPolarity = (modOcts >= 0 ? 1 : -1);
			var index = Math.abs(modOcts);
			var fmPitch = fmTool.fmPitchFunc(index);
			var centerPitch = (dp.OscFreq - FREQ_VALUE_C4) / OCTAVE_STEP + fmPitch;
			var pitchOct = Math.floor(centerPitch + 0.5 / 12);
			var pitchSemi = Math.round((centerPitch - pitchOct) * 12);
			var pitchCent = (centerPitch - (pitchOct + pitchSemi / 12)) * 1200;
			var unaccentedFMIndex = Math.abs(fmTool.modAmountToOcts(dp.ModAmt, 1 - dp.ModVel));
			var unaccentedTransposition = (fmTool.fmPitchFunc(unaccentedFMIndex) - fmPitch) * 12;
			var ratioCoarse = -Infinity;
			var ratioFine = 0.0;
			var ratio = fmTool.modRateToIndexFunc(centerPitch * OCTAVE_STEP + FREQ_VALUE_C4, dp.ModRate);
			if (ratio >= 0.000001) {
				ratio = (ratio < 1 ? Math.log(ratio) / Math.LN2 : ratio - 1);
				ratioCoarse = Math.round(ratio);
				var v = (ratio - ratioCoarse) * 2;
				ratioFine = 0.5 * Math.sqrt(Math.abs(v)) * (v < 0 ? -1 : 1);
			}

			fmTool.index.value = index;
			fmTool.pitchOct.value = pitchOct;
			fmTool.pitchSemi.value = pitchSemi;
			fmTool.pitchCent.value = pitchCent;
			fmTool.ratioCoarse.value = ratioCoarse;
			fmTool.ratioFine.value = ratioFine;
			fmTool.unaccentedTransposition.value = unaccentedTransposition;
			fmTool.clipWarning = false;
		}
	},
	
	pitchOct: new fmTool.GUIParam("Change Pitch", true),
	pitchSemi: Object.assign(new fmTool.GUIParam("Change Pitch", true), {
		human: function() {
			return fmTool.GUIParam.prototype.human.call(this) + ' (' + fmTool.NOTE_NAMES[this.value] + ')';
		}
	}),
	pitchCent: Object.assign(new fmTool.GUIParam("Change Pitch", false), {
		human: function() { return this.value.toFixed(2); }
	}),
	index: new fmTool.GUIParam("Change FM Index", false), // FIX : get: precision(fmTool.index, 4) };
	ratioCoarse: new fmTool.GUIParam("Change FM Ratio", true),
	ratioFine: new fmTool.GUIParam("Change FM Ratio", false),
	unaccentedTransposition: Object.assign(new fmTool.GUIParam("Change Unaccented Transposition", false), {
		human: function() { return this.value.toFixed(2); }
	}),
	toggleIndexPolarity: {
		execute: function(params) {
			saveUndo("Change FM Index Polarity", true);
			fmTool.indexPolarity = -fmTool.indexPolarity;
			fmTool.updatePatch();
		},
		checked: function() {
			return fmTool.indexPolarity < 0;
		}
	},
	sourceOscFreq: new fmTool.SourceParam("OscFreq"),
	sourceModAmount: new fmTool.SourceParam("ModAmt"),
	sourceModRate: new fmTool.SourceParam("ModRate"),
	sourceModVel: new fmTool.SourceParam("ModVel"),
	sourceModMode: new fmTool.SourceParam("ModMode")
});

fmTool.reloadPatch();
