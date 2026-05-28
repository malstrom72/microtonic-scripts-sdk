(function() {
	function parseNuXNN(bytes) {
		var offset = 0;

		function readUInt32() {
			var v = (bytes[offset] | (bytes[offset + 1] << 8) | (bytes[offset + 2] << 16) | (bytes[offset + 3] << 24)) >>> 0;
			offset += 4;
			return v;
		}

		function readFloat16s(n) {
			var floats = [ ];
			floats.length = n;
			var o = offset, b = bytes;
			for (var i = 0; i < n; ++i) {
				var v = b[o] | (b[o + 1] << 8);
				o += 2;
				var exponent = (v >> 10) & 31;
				floats[i] = ((v & 0x8000) ? -1.0 : 1.0)
						* (exponent ? ((v & 0x3FF) + 0x400) * 2.9802322387695312e-8 * (1 << exponent)
						: (v & 0x3FF) * 5.960464477539063e-8);
			}
			offset = o;
			return floats;
		}
		
		function readFloat32s(n) {
			var floats = [ ];
			floats.length = n;
			for (var i = 0; i < n; ++i) {
				var v = (bytes[offset] | (bytes[offset + 1] << 8)
						| (bytes[offset + 2] << 16) | (bytes[offset + 3] << 24)) >>> 0;
				offset += 4;
				var exponent = ((v >> 23) & 0xff) - 127;
				floats[i] = ((v & 0x80000000) ? -1.0 : 1.0)
						* (((v & 0x7fffff) / (1 << 23))
						+ ((exponent > -127) ? 0x800000 / (1 << 23) : 0))
						* Math.pow(2, (exponent > -127 ? exponent : exponent + 1));
			}
			return floats;
		}
		
		return (function() {
			var parsers = [ ];

			// 	0xd9fd8e7b 	Tanh
			parsers[0xd9fd8e7b] = (function(tag, inputSize) {
				return [ function(input) {
					var exp = Math.exp;
					for (var i = input.length; --i >= 0;) {
						var y = exp(2 * input[i]);
						input[i] = (y - 1) / (y + 1);
					}
					return input;
				}, inputSize ];
			});

			// 	0xd5b8e08e 	Sigmoid
			parsers[0xd5b8e08e] = (function(tag, inputSize) {
				return [ function(input) {
					var exp = Math.exp;
					for (var i = input.length; --i >= 0;) {
						input[i] = 1 / (1 + exp(-input[i]));
					}
					return input;
				}, inputSize ];
			});
			
			// 	0xf36cdc69 	LeakyReLU
			//				float32 alpha
			parsers[0xf36cdc69] = (function(tag, inputSize) {
				var alpha = readFloat32s(1)[0];
				return [ function(input) {
					var a = alpha;
					for (var i = input.length; --i >= 0;) {
						input[i] = Math.max(input[i] * a, input[i]);
					}
					return input;
				}, inputSize ];
			});

			// 	0x9cb138bc 	16-bit dense
			// 				uint32 outputSize
			// 				float16[outputSize*inputSize] kernel
			//				float16[outputSize] bias
			parsers[0x9cb138bc] = (function(tag, inputSize) {
				var outputSize = readUInt32();
				var kernel = [ ];
				var reader = (tag === 0x9cb138bc ? readFloat16s : readFloat32s);
				for (var i = 0; i < outputSize; ++i) {
					kernel[i] = reader(inputSize);
				}
				var bias = reader(outputSize);
				return [ function(input) {
					var output = [ ];
					var n = outputSize;
					var k = kernel;
					for (var i = n; --i >= 0;) {
						var ki = k[i];
						var s = bias[i];
						for (var j = input.length; --j >= 0;) {
							s += input[j] * ki[j];
						}
						output[i] = s;
					}
					return output;
				}, outputSize ];
			});

			// 	0x5a5591eb 	32-bit dense
			// 				uint32 outputSize
			// 				float32[outputSize*inputSize] kernel
			//				float32[outputSize] bias
			parsers[0x5a5591eb] = parsers[0x9cb138bc];

			//	0xa7fb7d64 	Sequential
			//				layer[ ] layers
			parsers[0xa7fb7d64] = (function(tag, inputSize) {
				var layers = [ ];
				var currentSize = inputSize;
				for (;;) {
					var parseInfo = parseLayer(currentSize);
					if (parseInfo === null) {
						break;
					}
					layers.push(parseInfo[0]);
					currentSize = parseInfo[1];
				}
				return [ function(input) {
					for (var i = 0; i < layers.length; ++i) {
						input = layers[i](input);
					}
					return input;
				} , currentSize ];
			});

			function parseLayer(inputSize) {
				var tag = readUInt32();
				if (tag === 0) {
					return null;
				}
				if (!parsers[tag]) {
					throw TypeError("Unknown layer tag in NuXNN");
				}
				return parsers[tag](tag, inputSize);
			}

			// 	NuXNN file format:
			//
			//	0x8d77306f magic
			//	uint32 inputSize
			//	layer rootLayer
			var magic = readUInt32();
			if (magic !== 0x8d77306f && magic !== 0x8d773070) {
				throw TypeError("Invalid NuXNN format");
			}
			var name = '';
			var date = null;
			if (magic === 0x8d773070) {
				var nameLength = bytes[offset++];
				for (var i = 0; i < nameLength; ++i) {
					name += String.fromCharCode(bytes[offset++]);
				}
				date = new Date(readUInt32() * 1000);
			}
			var inputSize = readUInt32();
			var rootLayer = parseLayer(inputSize);
			if (rootLayer === null) {
				throw TypeError("Missing NuXNN root layer");
			}

			return { name: name, date: date, inputSize: inputSize, outputSize: rootLayer[1], rootLayer: rootLayer[0] };
		})();
	}

	function decodeBase64(s) {
		var BASE64 = {
			'A': 0, 'B': 1, 'C': 2, 'D': 3, 'E': 4, 'F': 5, 'G': 6, 'H': 7,
			'I': 8, 'J': 9, 'K': 10, 'L': 11, 'M': 12, 'N': 13, 'O': 14, 'P': 15,
			'Q': 16, 'R': 17, 'S': 18, 'T': 19, 'U': 20, 'V': 21, 'W': 22, 'X': 23,
			'Y': 24, 'Z': 25, 'a': 26, 'b': 27, 'c': 28, 'd': 29, 'e': 30, 'f': 31,
			'g': 32, 'h': 33, 'i': 34, 'j': 35, 'k': 36, 'l': 37, 'm': 38, 'n': 39,
			'o': 40, 'p': 41, 'q': 42, 'r': 43, 's': 44, 't': 45, 'u': 46, 'v': 47,
			'w': 48, 'x': 49, 'y': 50, 'z': 51, '0': 52, '1': 53, '2': 54, '3': 55,
			'4': 56, '5': 57, '6': 58, '7': 59, '8': 60, '9': 61, '+': 62, '/': 63
		};
		var l = s.length, c, a = [ ], n = -1;
		while (l > 0 && ((c = s[l - 1]) === '=' || c === ' ' || c === '\n' || c === '\r' || c === '\t')) {
			--l;
		}
		l -= 4;
		for (var i = 0; i <= l; i += 4) {
			a[++n] = (BASE64[s[i]] << 2) | ((c = BASE64[s[i + 1]]) >> 4);
			a[++n] = ((c & 15) << 4) | ((c = BASE64[s[i + 2]]) >> 2);
			a[++n] = ((c & 3) << 6) | BASE64[s[i + 3]];
		}
		if (i - 2 <= l) {
			a[++n] = (BASE64[s[i]] << 2) | ((c = BASE64[s[i + 1]]) >> 4);
			if (i - 1 <= l) {
				a[++n] = ((c & 15) << 4) | (BASE64[s[i + 2]] >> 2);
			}
		}
		return a;
	}

	var DRUM_PATCH_PARAMS_COUNT = DRUM_PATCH_PARAMS.length;
	var DRUM_PATCH_PARAM_STEPS = (function() {
		var a = [ ];
		for (var i = 0; i < DRUM_PATCH_PARAMS_COUNT; ++i) a[i] = DRUM_PATCH_PARAMS[i].STEPS;
		return a;
	})();
	var DRUM_PATCH_PARAM_NAMES = (function() {
		var a = [ ];
		for (var i = 0; i < DRUM_PATCH_PARAMS_COUNT; ++i) a[i] = DRUM_PATCH_PARAMS[i].NAME;
		return a;
	})();
	var DICE_STRINGS = [
		"123", "135", "154", "142", "246", "263", "231", "214", "312", "326", "365", "351", "421", "415", "456"
		, "462", "513", "536", "564", "541", "632", "624", "645", "651"
	];

	var doInit = false;
	if (!this.beatspace) {
		this.beatspace = {
			// Config stuff
			spaceSize: 0,
			pointSize: 0,
			clickRadius: 0,
			flareLifetime: 0,
			maxFlares: 0,
			
			// UI state
			windowPosition: '',
			windowZOrder: '',
			diceRoll: 0,
			diceNumbers: DICE_STRINGS[0],
			constellation: 'M0,0',	// svg path

			// The rest
			loadedNetName: '',		// on a change the decoder will be reloaded
			decoder: null,
			selectedChannel: 0,
			hoveringChannel: -1,
			hoveringPosition: [ 0, 0 ],
			channelEnabled: [ ],
			latentPoints: [ ],
			dirty: false,			// selectedChannel has new latent point and Microtonic params / pattern need updating
			lastPresetId: null,		// to track any update in Microtonic and convert back to latentPoints if found in history
			pointHistory: { },		// key: processed (quantized) vector, value: point
			flaresArray: [ ],
			lastTriggers: [ ],
		};
		doInit = true;
	}

	var beatspace = this.beatspace;
	var typeOfPress = '';	// work-around because Microtonic 3.4 has a bug where no more than one key-modifier is ever flagged
	var lastMousePosition = [ 0, 0 ];
	var keyModifiers = { };
	var held = false;
	var heldPoint;
	var savedUndo = false;
	var solo = false;
	var previouslyMuted = [ ];

	function findShortestNonCrossingPath(points) {
		function lineSegmentCrossesAny(x0, y0, x1, y1, points, n) {
			var dx = x1 - x0;
			var dy = y1 - y0;
			var x2, y2, x3, y3;
			for (var i = n; --i >= 0;) {
				var p0 = points[i];
				var p1 = points[i + 1];
				var denom = ((y3 = p1[1]) - (y2 = p0[1])) * dx - ((x3 = p1[0]) - (x2 = p0[0])) * dy;
				if (denom !== 0) {
					var ua = ((x3 - x2) * (y0 - y2) - (y3 - y2) * (x0 - x2)) / denom;
					var ub = (dx * (y0 - y2) - dy * (x0 - x2)) / denom;
					if (ua > 0 && ua < 1 && ub > 0 && ub < 1) {
						return true;
					}
				}
			}
			return false;
		}

		function distance(p0, p1) {
			var dx = p0[0] - p1[0];
			var dy = p0[1] - p1[1];
			return Math.sqrt(dx * dx + dy * dy);
		}

		var shortestLength = +Infinity;
		var shortestPath = null;
		function examineBranch(points, depth, length) {
			for (var unusedIndex = depth; unusedIndex < points.length; ++unusedIndex) {
				var last = points[depth - 1];
				var next = points[unusedIndex];
				var thisLength = length + distance(last, next);
				if (thisLength < shortestLength && (depth < 3 ||
						!lineSegmentCrossesAny(last[0], last[1], next[0], next[1], points, depth - 3))) {
					var swap = points[depth]; points[depth] = points[unusedIndex]; points[unusedIndex] = swap;
					if (depth + 2 === points.length) { // shortest path is always non-crossing, no need to check for crossing on the last two points
						var totalLength = thisLength + distance(points[depth], points[depth + 1])
								+ distance(points[depth + 1], points[0]);
						if (totalLength < shortestLength) {
							shortestPath = points.slice();
							shortestLength = totalLength;
						}
					} else {
						examineBranch(points, depth + 1, thisLength);
					}
					swap = points[depth]; points[depth] = points[unusedIndex]; points[unusedIndex] = swap;
				}
			}
		}
		examineBranch(points, 1, 0);
		// assert(shortestPath !== null, "shortestPath !== null");
		return shortestPath;
	}

	function postProcessVector(vector) {
		var processed = [ ];
		var steps = DRUM_PATCH_PARAM_STEPS;
		var floor = Math.floor;
		var n;
		var paramIndex = 0;
		while (paramIndex < DRUM_PATCH_PARAMS_COUNT) {
			var v = vector[paramIndex];
			if ((n = steps[paramIndex]) !== null) v = floor(v * n) / (n - 1);
			processed[paramIndex++] = (v < 0 ? 0 : (v > 1 ? 1 : v));
		}
		var maxWeight = 0.001;
		for (var stepIndex = 0; stepIndex < 16; ++stepIndex) {
			var v = vector[paramIndex + stepIndex * 3];
			if (v > maxWeight) maxWeight = v;
			if (v >= 0.5001) break;
		}
		var norm = (maxWeight < 0.5001 ? 0.5001 / maxWeight : 1);
		for (var stepIndex = 0; stepIndex < 16; ++stepIndex) {
			var trig = (vector[paramIndex] * norm >= 0.5);
			processed[paramIndex] = +trig;
			processed[paramIndex + 1] = +(trig && vector[paramIndex + 1] >= 0.5);
			processed[paramIndex + 2] = +(trig && vector[paramIndex + 2] >= 0.5);
			paramIndex += 3;
		}
		// assert(paramIndex == 73, "paramIndex == 73");
		return processed;
	}

	function vectorToDrumChannel(vector, preset, drumChannelIndex, patternIndex, point) {
		var paramIndex = 0;
		var names = DRUM_PATCH_PARAM_NAMES;
		var channel = preset.drumPatches[drumChannelIndex];
		channel.name = 'BeatSpace ' + point[0] + ';' + point[1];
		channel.modified = false;
		while (paramIndex < DRUM_PATCH_PARAMS_COUNT) {
			channel[names[paramIndex]] = vector[paramIndex];
			++paramIndex;
		}
		var pattern = preset.patterns[patternIndex].channels[drumChannelIndex];
		for (var stepIndex = 0; stepIndex < 16; ++stepIndex) {
			pattern.triggers[stepIndex] = !!vector[paramIndex];
			pattern.accents[stepIndex] = !!vector[paramIndex + 1];
			pattern.fills[stepIndex] = !!vector[paramIndex + 2];
			paramIndex += 3;
		}
		// assert(paramIndex == 73);
	}
	
	function drumChannelToVector(preset, drumChannelIndex, patternIndex) {
		var vector = [ ];
		var paramIndex = 0;
		var names = DRUM_PATCH_PARAM_NAMES;
		var channel = preset.drumPatches[drumChannelIndex];
		while (paramIndex < DRUM_PATCH_PARAMS_COUNT) {
			vector[paramIndex] = channel[names[paramIndex]];
			++paramIndex;
		}
		var pattern = preset.patterns[patternIndex].channels[drumChannelIndex];
		for (var stepIndex = 0; stepIndex < 16; ++stepIndex) {
			var trig = pattern.triggers[stepIndex];
			vector[paramIndex++] = +trig;
			vector[paramIndex++] = +(trig && pattern.accents[stepIndex]);
			vector[paramIndex++] = +(trig && pattern.fills[stepIndex]);
		}
		// assert(paramIndex == 73);
		return vector;
	}

	function hashVector(vector) {
		var hash = 2166136261, round = Math.round;
		for (var i = vector.length; --i >= 0;) {
			var v = round(vector[i] * 10000);
			hash = (hash << 3) | (hash >>> (32 - 3));
			hash = (hash + (v + 1)) * 16777619;
		}
		return hash >>> 0;
	}

	function updateSolo(newSolo) {
		if (solo !== newSolo) {
			solo = newSolo;
			if (solo) {
				for (var i = CHANNEL_COUNT; --i >= 0;) { 
					previouslyMuted[i] = (getParam('Mute.' + (i + 1)) >= 0.5);
				}
				for (var i = CHANNEL_COUNT; --i >= 0;) { 
					setParam('Mute.' + (i + 1), +(i !== beatspace.selectedChannel));
				}
			} else {
				for (var i = CHANNEL_COUNT; --i >= 0;) { 
					setParam('Mute.' + (i + 1), +previouslyMuted[i]);
				}
			}
		}
	}

	function setPreset(preset) {
		setElement('preset', preset);
		beatspace.lastPresetId = getElementId('preset');
	}

	function decodeOneChannel(preset, channelIndex, patternIndex) {
		var point = beatspace.latentPoints[channelIndex];
		var vector = (beatspace.decoder.rootLayer)([ (point[0] / 999) * 2 - 1, (point[1] / 999) * 2 - 1 ]);
		var processed = postProcessVector(vector);
		beatspace.pointHistory[hashVector(processed)] = point;
		vectorToDrumChannel(processed, preset, channelIndex, patternIndex, point);
		beatspace.channelEnabled[channelIndex] = true;
	}

	function selectChannel(newChannel) {
		if (beatspace.dirty) {
			update();
		}
		beatspace.selectedChannel = newChannel;
		select('channel', newChannel);
	}

	function updateConstellation() {
		var s = '';
		var constellation = findShortestNonCrossingPath(beatspace.latentPoints);
		for (var i = 0; i < constellation.length; ++i) {
			var pixelPoint = spaceToPixelPoint(constellation[i]);
			s += (s === '' ? 'M' : 'L') + pixelPoint[0] + ',' + pixelPoint[1];
		}
		beatspace.constellation = s + 'z';
	}

	function update() {
		if (beatspace.dirty) {
			var channelIndex = beatspace.selectedChannel;
			var preset = getElement('preset');
			var patternIndex = selected('pattern');
			decodeOneChannel(preset, channelIndex, patternIndex);
			if (preset.name.substr(0, 10) === 'BeatSpace ') {
				preset.name = makeHashedName();
			}
			preset.modified = true;
			setPreset(preset);
			updateConstellation();
			beatspace.dirty = false;
		}
		if (!held) {
			selectChannel(selected('channel'));
		}
	}

	function pixelToChannel(x, y) {
		var minD = +Infinity;
		var pressChannel = 0;
		for (var channel = beatspace.latentPoints.length - 1; channel >= 0; --channel) {
			var e = spaceToPixelPoint(beatspace.latentPoints[channel]);
			var dx = (x - e[0]);
			var dy = (y - e[1]);
			var d = dx * dx + dy * dy;
			if (d < minD) {
				minD = d;
				pressChannel = channel;
			}
		}
		if (minD < beatspace.clickRadius * beatspace.clickRadius) {
			return pressChannel;
		}
		return null;
	}

	function press(param) {
		typeOfPress = unescape(param);
		var x = lastMousePosition[0], y = lastMousePosition[1];
		var pressChannel = pixelToChannel(x, y);
		if (pressChannel !== null) {
			selectChannel(pressChannel);

			var muteParam = 'Mute.' + (pressChannel + 1);
			if (typeOfPress === 'mute') {
				var doMute = (getParam(muteParam) < 0.5);
				saveUndo((doMute ? 'Mute' : 'Unmute') + ' BeatSpace Point ' + (pressChannel + 1));
				setParam(muteParam, +doMute);
			} else {
				held = true;
				savedUndo = false;
				heldPoint = spaceToPixelPoint(beatspace.latentPoints[pressChannel]);
				var enabled = beatspace.channelEnabled[pressChannel];
				if (!enabled || getParam(muteParam) >= 0.5) {
					saveUndo((!enabled ? 'Enable' : 'Unmute') + ' BeatSpace Point ' + (pressChannel + 1));
					setParam(muteParam, 0);
					if (!enabled) {
						beatspace.channelEnabled[pressChannel] = true;
						beatspace.dirty = true;
					}
				}
				updateSolo(typeOfPress === 'solo' || ('alt' in keyModifiers));
				update();
			}
		}
	}

	function release() {
		if (held) {
			held = false;
			updateSolo(false);
			update();
		}
	}
	
	function coordPress(paramsText) {
		var params = parseNumbstrict(paramsText);
		selectChannel(params.index);
		var muteParam = 'Mute.' + (params.index + 1);
		if (params.type === 'mute') {
			var doMute = (getParam(muteParam) < 0.5);
			saveUndo((doMute ? 'Mute' : 'Unmute') + ' BeatSpace Point ' + (params.index + 1));
			setParam(muteParam, +doMute);
		} else if (params.type === 'solo') {
			updateSolo(true);
			held = true;
		}
	}

	function coordRelease() {
		updateSolo(false);
		held = false;
	}

	function coordEnter(paramsText) {
		updateHoveringChannel(+paramsText);
	}

	function coordLeave() {
		updateHoveringChannel(null);
	}

	function setKeyModifiers(s) {
		var mods = s.split('+');
		keyModifiers = { };
		for (var i = mods.length; --i >= 0; ) {
			keyModifiers[mods[i]] = true;
		}
	}

	function pixelToSpacePoint(xy) {
		var s = +beatspace.spaceSize;
		return [ clamp(Math.round(xy[0] / s * 999), 0, 999), clamp(Math.round(xy[1] / s * 999), 0, 999) ];
	}

	function spaceToPixelPoint(xy) {
		var s = +beatspace.spaceSize;
		return [ xy[0] / 999 * s, xy[1]/ 999 * s ];
	}

	function updateHoveringChannel(newChannel) {
		beatspace.hoveringChannel = -1;
		if (newChannel !== null) {
			beatspace.hoveringChannel = newChannel;
			beatspace.hoveringPosition = spaceToPixelPoint(beatspace.latentPoints[newChannel]);
		}
	}

	function setMousePosition(s) {
		var xy = s.split(',');
		xy[0] = +xy[0];
		xy[1] = +xy[1];
		var mouseChannel;
		if (held) {
			mouseChannel = beatspace.selectedChannel;
			if (!savedUndo) {
				updateSolo(false);
				saveUndo('Move BeatSpace Point ' + (mouseChannel + 1));
				savedUndo = true;
			}
			updateSolo(typeOfPress === 'solo' || ('alt' in keyModifiers));
			var rate = ((typeOfPress === 'fine' || ('shift' in keyModifiers)) ? 0.25 : 1);
			var newX = heldPoint[0] + (xy[0] - lastMousePosition[0]) * rate;
			var newY = heldPoint[1] + (xy[1] - lastMousePosition[1]) * rate;
			heldPoint = [ newX, newY ];
			beatspace.latentPoints[mouseChannel] = pixelToSpacePoint(heldPoint);
			beatspace.dirty = true;
		} else {
			mouseChannel = pixelToChannel(xy[0], xy[1]);
		}
		lastMousePosition = xy;
		updateHoveringChannel(mouseChannel);
	}

	function makeHashedName() {
		var hash = 2166136261;
		var points = beatspace.latentPoints;
		for (var channelIndex = 0; channelIndex < CHANNEL_COUNT; ++channelIndex) {
			var p;
			hash = (hash << 3) | (hash >>> (32 - 3));
			hash = (hash + ((p = points[channelIndex])[0] + 1)) * 16777619;
			hash = (hash << 3) | (hash >>> (32 - 3));
			hash = (hash + (p[1] + 1)) * 16777619;
		}
		return 'BeatSpace ' + (hash >>> 0).toString(16).toUpperCase();
	}

	function decodeAllChannels(turnOffMutes) {
		var preset = getElement('preset');
		var patternIndex = selected('pattern');
		for (var channelIndex = 0; channelIndex < CHANNEL_COUNT; ++channelIndex) {
			decodeOneChannel(preset, channelIndex, patternIndex);
		}
		if (patternIndex > 0) {
			preset.patterns[patternIndex - 1].chained = false;
		}
		if (patternIndex < PATTERN_COUNT - 1) {
			preset.patterns[patternIndex].chained = false;
		}
		preset.patterns[patternIndex].steps = 16;
		preset.name = makeHashedName();
		if (turnOffMutes) {
			preset.mutes = [ false, false, false, false, false, false, false, false ];
		}
		preset.modified = true;
		setPreset(preset);
		updateConstellation();
		if (beatspace.hoveringChannel >= 0) {
			beatspace.hoveringPosition = spaceToPixelPoint(beatspace.latentPoints[beatspace.hoveringChannel]);
		}
		beatspace.dirty = false;
	}

	function randomize(turnOffMutes) {
		saveUndo('Randomize BeatSpace');
		for (var channelIndex = 0; channelIndex < CHANNEL_COUNT; ++channelIndex) {
			beatspace.latentPoints[channelIndex] = [ Math.round(random.uniform() * 999), Math.round(random.uniform() * 999) ];
		}
		beatspace.diceRoll = (beatspace.diceRoll + random.integer(DICE_STRINGS.length - 1) + 1) % DICE_STRINGS.length;
		beatspace.diceNumbers = DICE_STRINGS[beatspace.diceRoll];
		decodeAllChannels(turnOffMutes);
	}
	
	function presetIdChanged() {
		var currentId = getElementId('preset');
		if (currentId !== beatspace.lastPresetId) {
			// Preset state differs from beatspace
			var p = getElement('preset');
			var enabled = beatspace.channelEnabled;	
			var patternIndex = selected('pattern');
			var updated = false;
			for (var channelIndex = CHANNEL_COUNT; --channelIndex >= 0;) {
				var hash = hashVector(postProcessVector(drumChannelToVector(p, channelIndex, patternIndex)));
				var point = beatspace.pointHistory[hash];
				if (point) {
					enabled[channelIndex] = true;
					beatspace.latentPoints[channelIndex] = point;
					updated = true;
				} else {
					enabled[channelIndex] = false;
				}
			}
			if (updated) {
				updateConstellation();				
			}
			beatspace.lastPresetId = currentId;
		}
	}

	function animate() {
		var points = beatspace.latentPoints;
		var enabled = beatspace.channelEnabled;
		var mutes = getElement('preset').mutes;
		var selected = [];
		var pointStrings = [];
		for (var i = 0; i < CHANNEL_COUNT; ++i) {
			var point = points[i];
			var pixelPoint = spaceToPixelPoint(point);
			var selected = (i === beatspace.selectedChannel);
			var isMuted = (mutes[i] ? 'yes' : 'no');
			var isEnabled = (enabled[i] ? 'yes' : 'no');
			var str = '[' + i + ',' + pixelPoint[0] + ',' + pixelPoint[1] + ','
					+ isEnabled + ',' + isMuted + ']';
			if (selected) {
				pointStrings.push(str);
			} else {
				pointStrings.unshift(str);
			}
			beatspace.coords[i] = {
				text: ("  " + point[0]).slice(-3) + ":" + point[1],
				muted: isMuted,
				enabled: isEnabled
			}
		}
		beatspace.points = '[' + pointStrings.join(",") + ']';
		
		var lastTriggers = beatspace.lastTriggers;
		var flares = beatspace.flaresArray;

		var visuals = getElement('visuals');
		var triggers = visuals.lastTriggers;

		for (var i = CHANNEL_COUNT; --i >= 0;) {
			var lastTrigger = triggers[i];
			if (lastTriggers[i] != lastTrigger) {
				lastTriggers[i] = lastTrigger;
				if (flares.length < beatspace.maxFlares) {
					flares.push([ i, lastTrigger ]);
				}
			}
		}

		var now = Date.now();
		var s = '';
		for (var i = flares.length; --i >= 0;) {
			var flare = flares[i];
			var age = (now - flare[1]) / beatspace.flareLifetime;
			if (age < 0 || age >= 1) {
				flares.splice(i, 1);
			} else {
				var point = spaceToPixelPoint(points[flare[0]]);
				s += (s !== '' ? ',[' : '[') + point[0] + ',' + point[1] + ',' + age + ']';
			}
		}
		beatspace.flares = '[' + s + ']';
	}

	function startup(netName) {
		try {
			netName = unescape(netName);
			if (beatspace.loadedNetName !== netName) {
				beatspace.loadedNetName = netName;
				beatspace.decoder = parseNuXNN(decodeBase64(load('BeatSpace.mtscript/'+netName+'_decoder.nuxnn.base64')));
				if (beatspace.latentPoints.length === 0) {
					randomize(true);
				} else {
					decodeAllChannels(false);
				}
			}
			selectChannel(selected('channel'));
		}
		catch (error) {
			closeCushy();
			display("Failed initializing BeatSpace:\n" + error, "error");
		}
	}

	function copy() {
		var text = 'beatspacev1:';
		var points = beatspace.latentPoints;
		for (var i = 0; i < CHANNEL_COUNT; ++i) {
			text += (i > 0 ? '|' : '[') + points[i][0] + ':' + points[i][1]
					+ (getParam('Mute.' + (i + 1)) >= 0.5 ? 'm' : '');
		}
		writeClipboard(text + ']');
	}

	function parseClipboard(s) {
		if (s === null) {
			return '';
		}
		for (var b = 0; s[b] <= ' '; ++b) ;
		for (var e = s.length; s[e - 1] <= ' '; --e) ;
		return (s.substr(b, 12) === 'beatspacev1:' ? s.substring(b + 12, e) : '');
	}
	
	function pastePossible() {
		return parseClipboard(readClipboard());
	}

	function paste() {
		var text = parseClipboard(readClipboard());
		if (text) {
			saveUndo("Paste BeatSpace Constellation");
			if (text[0] === '[' && text[text.length - 1] === ']') {
				text = text.slice(1, -1);
			}
			var textPoints = text.split('|', CHANNEL_COUNT);
			for (var i = 0; i < textPoints.length; ++i) {
				var s = textPoints[i], c, isMuted = false;
				if ((c = s[s.length - 1]) === 'm' || c === 'M') {
					s = s.substr(0, s.length - 1);
					isMuted = true;
				}
				setParam('Mute.' + (i + 1), +isMuted);
				var xy = s.split(':', 2);
				if (!isNaN(xy[0] = +xy[0]) && !isNaN(xy[1] = +xy[1])) {
					beatspace.latentPoints[i] = pixelToSpacePoint(spaceToPixelPoint(xy));
				}
			}
			decodeAllChannels(false);
		}
	}

	function enableAllChecked() {
		var enabled = beatspace.channelEnabled;
		for (var channelIndex = CHANNEL_COUNT; --channelIndex >= 0;) {
			if (!enabled[channelIndex] || getParam('Mute.' + (channelIndex + 1)) >= 0.5) {
				return false;
			}
		}
		return true;
	}

	function enableAll() {
		if (!enableAllChecked()) {
			saveUndo("Enable All BeatSpace Points");
			decodeAllChannels(true);
		}
	}

	// public interface
	Object.assign(beatspace, {
		press: press,
		release: release,
		coordPress: coordPress,
		coordRelease: coordRelease,
		coordEnter: coordEnter,
		coordLeave: coordLeave,
		update: update,
		randomize: randomize,
		presetId: function() { return getElementId('preset'); },
		presetIdChanged: presetIdChanged,
		animate: animate,
		startup: startup,
		copy: copy,
		paste: {
			execute: paste,
			enabled: pastePossible
		},
		enableAll: {
			execute: enableAll,
			checked: enableAllChecked
		},
		keyModifiers: { set: setKeyModifiers },
		mousePosition: { set: setMousePosition },
		points: '[]',		// [ [ index, x, y, enabled, muted ], ... ]
		flares: '[]',		// [ [ x, y, age ], ... ]
		coords: [],			// { text: string, muted: 'yes'|'no', enabled: 'yes'|'no' } * 8
		decoderName: (function() { return beatspace.decoder.name; }),
		decoderDate: (function() { return beatspace.decoder.date.toISOString(); }),
	});
})();
