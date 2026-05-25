if (!this.mixConsole) {
	mixConsole = {
		windowPosition: '',
		windowZOrder: '',
		soloChannel: null,
		savedMutes: getElement('preset').mutes.slice(),
		stripWidth: null,
		dragChannel: null,
		dragReassignment: null
	};
}

Object.assign(mixConsole, {
	HumanParam: createClass({
		constructor: function(param) { this.param = param; },
		get: function() { return this.param.getHuman(); },
		set: function(v) { this.param.setHuman(v); }
	}),
	Param: createClass({
		constructor: function(channel, name) {
			this.channel = channel;
			this.name = name;
			this.paramName = name + '.' + (channel + 1);
			this.human = new mixConsole.HumanParam(this);
		},
		touch: function(down) {
			if (down) {
				saveUndo("Change " + translate("mixConsole.undoLabel." + this.name)
						+ " #" + (this.channel + 1), true);
			}
			editParam(this.paramName, !!down);
		},
		get: function() { return '' + getParam(this.paramName); },
		set: function(v) { setParam(this.paramName, +v); },
		getHuman: function() { return paramText(this.paramName, getParam(this.paramName)); },
		setHuman: function(v) { setParam(this.paramName, paramValue(this.paramName, unescape(v))); }
	}),
	ChannelDrag: createClass({
		constructor: function(channel) {
			this.channel = channel;
		},
		touch: function(down) {
			if (down) {
				mixConsole.dragChannel = this.channel;
				mixConsole.dragReassignment = this.channel;
			} else {
				if (mixConsole.dragChannel !== mixConsole.dragReassignment) {
					saveUndo("Drag Mixer Channel " + (mixConsole.dragChannel + 1)
							+ " to " + (mixConsole.dragReassignment + 1), false);
					mixConsole.reassign(mixConsole.dragChannel, mixConsole.dragReassignment);
				}
				mixConsole.dragChannel = null;
				mixConsole.dragReassignment = null;
			}
		},
		set: function(v) {
			if (mixConsole.dragChannel === this.channel) {
				var xy = v.split(',');
				var newChannel = clamp(Math.round(+xy[0] / +mixConsole.stripWidth), 0, CHANNEL_COUNT - 1);
				mixConsole.dragReassignment = newChannel;
			}
		},
		get: function() {
			return (mixConsole.dragChannel === this.channel
					? '' : mixConsole.reassignedChannel(this.channel) * +mixConsole.stripWidth + ',0');
		}
	}),
	ChannelLabel: createClass({
		constructor: function(channel) { this.channel = channel; },
		get: function() { return '' + (mixConsole.reassignedChannel(this.channel) + 1); }
	}),
	selectChannel: {
		execute: function(channel) { select('channel', +channel); },
		checked: function(channel) { return selected('channel') === +channel; }
	},
	triggerChannel: {
		execute: function(params) {
			var channelAndVelocity = parseNumbstrict(params);
			triggerChannel(+channelAndVelocity[0], +channelAndVelocity[1]);
		},
		checked: function(params) {
			var channelAndVelocity = parseNumbstrict(params);
			return selected('channel') === +channelAndVelocity[0];
		}
	},
	mute: {
		execute: function(cushyParam) {
			var ch = +cushyParam;
			saveUndo(translate("mixConsole.undoLabel.Mute") + " #" + (ch + 1));
			var v = (mixConsole.savedMutes[ch] >= 0.5 ? 0.0 : 1.0);
			mixConsole.savedMutes[ch] = v;
			if (mixConsole.soloChannel === null) {
				var paramName = 'Mute.' + (ch + 1);
				editParam(paramName, true);
				setParam(paramName, v);
				editParam(paramName, false);
			}
		},
		checked: function(cushyParam) {
			return mixConsole.savedMutes[+cushyParam] >= 0.5;
		}
	},
	solo: {
		execute: function(cushyParam) {
			var ch = +cushyParam;
			saveUndo(translate("mixConsole.undoLabel.Solo") + " #" + (ch + 1));
			if (mixConsole.soloChannel !== ch) {
				for (var i = 0; i < CHANNEL_COUNT; ++i) {
					mixConsole.automateSwitch('Mute.' + (i + 1), (i === ch ? 0.0 : 1.0));
				}
			} else {
				for (var i = 0; i < CHANNEL_COUNT; ++i) {
					mixConsole.automateSwitch('Mute.' + (i + 1), mixConsole.savedMutes[i]);
				}
			}
		},
		checked: function(cushyParam) {
			return mixConsole.soloChannel === +cushyParam;
		}
	},
	switch: {
		execute: function(cushyParam) {
			var p = parseNumbstrict(cushyParam);
			saveUndo("Change " + translate("mixConsole.undoLabel." + p.param) + " #" + (p.channel + 1));
			mixConsole.automateSwitch(p.param + '.' + (p.channel + 1), null);
		},
		checked: function(cushyParam) {
			var p = parseNumbstrict(cushyParam);
			return getParam(p.param + '.' + (p.channel + 1)) >= 0.5;
		}
	},
	reassignedChannel: function(originalChannel) {
		if (this.dragChannel === null || this.dragReassignment === null) {
			return originalChannel;
		} else if (this.dragChannel === originalChannel) {
			return this.dragReassignment;
		} else {
			var newChannel = originalChannel;
			if (newChannel > this.dragChannel) {
				--newChannel;
			}
			if (newChannel >= this.dragReassignment) {
				++newChannel;
			}
			return newChannel;
		}
	},
	checkSolo: function() {
		var mutes = getElement('preset').mutes;
		var muteCounter = 0;
		var soloChannel;
		for (var ch = 0; ch < mutes.length; ++ch) {
			if (mutes[ch] < 0.5) {
				soloChannel = ch;
				++muteCounter;
			}
		}
		if (muteCounter !== 1) {
			mixConsole.savedMutes = mutes.slice();
		}
		mixConsole.soloChannel = (muteCounter === 1 ? soloChannel : null);
	},
	extractChannelData: function(preset, channel) {
		var channelData = {
			patch: preset.drumPatches[channel],
			mute: preset.mutes[channel],
			savedMute: mixConsole.savedMutes[channel],
			soloed: mixConsole.soloChannel === channel,
			patterns: [ ]
		};
		for (var p = 0; p < PATTERN_COUNT; ++p) {
			channelData.patterns[p] = preset.patterns[p].channels[channel];
		}
		return channelData;
	},
	injectChannelData: function(preset, channel, channelData) {
		for (var p = 0; p < PATTERN_COUNT; ++p) {
			preset.patterns[p].channels[channel] = channelData.patterns[p];
		}
		preset.drumPatches[channel] = channelData.patch;
		preset.mutes[channel] = channelData.mute;
		mixConsole.savedMutes[channel] = channelData.savedMute;
		if (channelData.soloed) {
			mixConsole.soloChannel = channel;
		} else if (mixConsole.soloChannel === channel) {
			mixConsole.soloChannel = null;
		}
	},
	reassign: function(fromChannel, toChannel) {
		// turn off both channels
		triggerChannel(fromChannel, 0);
		triggerChannel(toChannel, 0);
		
		var preset = getElement('preset');
		var fromChannelData = mixConsole.extractChannelData(preset, fromChannel);
		var d = (fromChannel < toChannel ? 1 : -1);
		for (var ch = fromChannel; ch != toChannel; ch += d) {
			mixConsole.injectChannelData(preset, ch, mixConsole.extractChannelData(preset, ch + d));
		}
		mixConsole.injectChannelData(preset, toChannel, fromChannelData);
		setElement('preset', preset);
		if (selected('channel') === fromChannel) {
			select('channel', toChannel);
		}
	},
	updateVisuals: function() {
		var now = Date.now();
		var visuals = getElement('visuals');
		for (var ch = 0; ch < CHANNEL_COUNT; ++ch) {
			this.triggerIndicators[ch] = clamp(1 - (now - visuals.lastTriggers[ch]) / 500, 0, 1);
		}
	},
	automateSwitch: function(paramName, value) {
		var v = getParam(paramName);
		if (value == null) {
			value = (v >= 0.5 ? 0.0 : 1.0);
		}
		if (v !== value) {
			editParam(paramName, true);
			setParam(paramName, value);
			editParam(paramName, false);
		}
	},
	channelDrags: [ ],
	channelLabels: [ ],
	triggerIndicators: [ ],
	levels: [ ],
	pans: [ ],
	eqFreqs: [ ],
	eqGains: [ ],
	mutes: [ ],
});

(function() {
	for (var ch = 0; ch < CHANNEL_COUNT; ++ch) {
		mixConsole.channelDrags[ch] = new mixConsole.ChannelDrag(ch);
		mixConsole.channelLabels[ch] = new mixConsole.ChannelLabel(ch);
		mixConsole.triggerIndicators[ch] = 0.0;
		mixConsole.levels[ch] = new mixConsole.Param(ch, 'Level');
		mixConsole.pans[ch] = new mixConsole.Param(ch, 'Pan');
		mixConsole.eqFreqs[ch] = new mixConsole.Param(ch, 'EQFreq');
		mixConsole.eqGains[ch] = new mixConsole.Param(ch, 'EQGain');
		mixConsole.mutes[ch] = new mixConsole.Param(ch, 'Mute');
	}
})();
