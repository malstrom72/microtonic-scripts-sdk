if (!this.macroTweak) {
	macroTweak = {
		pitchSetting: 0.0,
		timeSetting: 0.0,
		dynamicsSetting: 0.0,
		channelsChoice: 'all',
		lastElementId: null,
		windowPosition: '',
		windowZOrder: ''
	};
}

Object.assign(macroTweak, {
	resetBlocked: 0,
	undoPending: false,
	dragAborted: false,
	checkAndReset: function() {
		if (this.resetBlocked === 0) {
			var elementId = getElementId(this.channelsChoice === 'selected' ? 'drumPatch' : 'preset');
			if (this.lastElementId !== elementId) {
				this.lastElementId = elementId;
				this.tweakedPreset = getElement('preset');
				this.sourcePreset = getElement('preset');	// must be separate copy

				this.pitchSetting = 0.0;
				this.timeSetting = 0.0;
				this.dynamicsSetting = 0.0;

				var count = 0;
				this.meanLevel = 0.0;
				for (var i = 0; i < CHANNEL_COUNT; ++i) {
					if (this.sourcePreset.mutes[i] < 0.5) {
						this.meanLevel += this.sourcePreset.drumPatches[i].Level;
						++count;
					}
				}
				if (count > 0) {
					this.meanLevel /= count;
				}
			}
		}
	},

	identity: function() {
		return getElementId(this.channelsChoice === 'selected' ? 'drumPatch' : 'preset');
	},

	sameDrumPatch: function(a, b) {
		for (var i = 0; i < DRUM_PATCH_PARAMS.length; ++i) {
			var name = DRUM_PATCH_PARAMS[i].NAME;
			if (a[name] !== b[name]) {
				return false;
			}
		}
		return true;
	},

	/*
		Each tweak starts from the current document, not from tweakedPreset, so changes made elsewhere during a drag
		(transport, pattern chains, mutes) are not written back over. If the drum patches themselves were changed
		elsewhere (undo, edits, another program), the rest of the drag is ignored and the knobs reset on release.
	*/
	tweak: function(undoName, tweaker) {
		if (this.dragAborted) {
			return;
		}
		var channel = selected('channel');
		var selectedOnly = (this.channelsChoice === 'selected');
		var current = getElement(selectedOnly ? 'drumPatch' : 'preset');
		if (this.resetBlocked > 0 && this.identity() !== this.lastElementId) {
			var unchanged = true;
			if (selectedOnly) {
				unchanged = this.sameDrumPatch(this.tweakedPreset.drumPatches[channel], current);
			} else {
				for (var i = 0; i < CHANNEL_COUNT && unchanged; ++i) {
					unchanged = this.sameDrumPatch(this.tweakedPreset.drumPatches[i], current.drumPatches[i]);
				}
			}
			if (!unchanged) {
				this.dragAborted = true;
				return;
			}
		}

		saveUndo("Change " + undoName, !this.undoPending);
		this.undoPending = false;

		if (selectedOnly) {
			tweaker(this.sourcePreset.drumPatches[channel], current);
			setElement('drumPatch', current);
			this.tweakedPreset.drumPatches[channel] = getElement('drumPatch');
		} else {
			for (var i = 0; i < CHANNEL_COUNT; ++i) {
				if (this.channelsChoice === 'all' || this.sourcePreset.mutes[i] < 0.5) {
					tweaker(this.sourcePreset.drumPatches[i], current.drumPatches[i]);
				}
			}
			current.modified = true;
			setElement('preset', current);
			this.tweakedPreset = getElement('preset');
		}
		this.lastElementId = this.identity();
	},

	touch: function(mouseDown) {
		if (mouseDown) {
			if (this.resetBlocked++ === 0) {
				this.undoPending = true;
				this.dragAborted = false;
			}
		} else if (--this.resetBlocked === 0) {
			this.checkAndReset();
		}
	},
	
	pitch: {
		set: function(v) {
			v = +v;
			if (isNaN(v)) {		// text from "Enter Exact Value..."; writing NaN would throw a RangeError
				return;
			}
			v = clamp(v, -1, 1);
			macroTweak.pitchSetting = v;
			var d = OCTAVE_STEP * 2 * v;
			macroTweak.tweak('Pitch', function(sp, dp) {
				dp.EQFreq = clamp(sp.EQFreq + d, 0, 1);
				dp.NFilFrq = clamp(sp.NFilFrq + d, 0, 1);
				dp.OscFreq = clamp(sp.OscFreq + d, 0, 1);
			});
		},
		get: function() { return '' + macroTweak.pitchSetting; },
		human: {
			get: function() { return (macroTweak.pitchSetting * 24).toFixed(2); },
			set: function(v) { macroTweak.pitch.set(+v / 24.0); }
		},
		touch: function(mouseDown) { macroTweak.touch(mouseDown); }
	},

	time: {
		set: function(v) {
			v = +v;
			if (isNaN(v)) {		// text from "Enter Exact Value..."; writing NaN would throw a RangeError
				return;
			}
			v = clamp(v, -1, 1);
			macroTweak.timeSetting = v;
			var timeScale = Math.pow(2, v * 4);

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

			function paramToAttackTime(v) { return v * Math.pow(10, (12 * v - 7) / 5); }
			function paramToDecayTime(v) { return Math.pow(10, 3 * v - 2); }
			function decayTimeToParam(t) { return Math.log(100 * t) / Math.log(1000); }

			macroTweak.tweak('Time', function(sp, dp) {
				dp.NEnvAtk = bisect(paramToAttackTime, paramToAttackTime(sp.NEnvAtk) * timeScale, 0.0, 1.0);
				dp.NEnvDcy = clamp(decayTimeToParam(paramToDecayTime(sp.NEnvDcy) * timeScale), 0.0, 1.0);
				dp.OscAtk = bisect(paramToAttackTime, paramToAttackTime(sp.OscAtk) * timeScale, 0.0, 1.0);
				dp.OscDcy = clamp(decayTimeToParam(paramToDecayTime(sp.OscDcy) * timeScale), 0.0, 1.0);
			});
		},
		get: function() { return '' + macroTweak.timeSetting; },
		human: {
			get: function() { return Math.pow(2, macroTweak.timeSetting * 4).toFixed(2); },
			set: function(v) { macroTweak.time.set(Math.log(+v) * Math.LOG2E / 4.0); }
		},
		touch: function(mouseDown) { macroTweak.touch(mouseDown); }
	},

	dynamics: {
		set: function(v) {
			v = +v;
			if (isNaN(v)) {		// text from "Enter Exact Value..."; writing NaN would throw a RangeError
				return;
			}
			v = clamp(v, -1, 1);
			macroTweak.dynamicsSetting = v;
			macroTweak.tweak('Dynamics', function(sp, dp) {
				dp.Level = clamp(lerp(sp.Level, macroTweak.meanLevel, v), 0, 1);
				dp.OscVel = clamp(sp.OscVel + v, 0, 1);
				dp.ModVel = clamp(sp.ModVel + v, 0, 1);
				dp.NVel = clamp(sp.NVel + v, 0, 1);
			});
		},
		get: function() { return '' + macroTweak.dynamicsSetting; },
		human: {
			get: function() { return ((macroTweak.dynamicsSetting + 1) * 100.0).toFixed(2); },
			set: function(v) { macroTweak.dynamics.set(+v / 100.0 - 1); }
		},
		touch: function(mouseDown) { macroTweak.touch(mouseDown); }
	}
});

macroTweak.checkAndReset();
