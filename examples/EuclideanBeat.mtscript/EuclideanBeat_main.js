if (!this.euclideanBeat) {
    euclideanBeat = {
        windowPosition: '',
        windowZOrder: ''
    };
}

Object.assign(euclideanBeat, {
    rotateArray: function(a, r) {
        return a.slice(-r).concat(a.slice(0, -r));
    },
    generateEuclidean: function generateEuclidean(k, n) {
        var leftCount = k;
        var rightCount = Math.max(n - k, 0);
        var leftPattern = [ true ];
        var rightPattern = [ false ];
        while (leftCount >= 1 && rightCount > 1) {
            var previousLeftCount = leftCount;
            var previousLeftPattern = leftPattern;
            leftPattern = leftPattern.concat(rightPattern);
            if (leftCount >= rightCount) {
                leftCount = rightCount;
                rightCount = previousLeftCount - rightCount;
                rightPattern = previousLeftPattern;
            } else {
                rightCount = rightCount - leftCount;
            }
        }
        var a = [ ];
        for (var i = 0; i < leftCount; ++i) {
            a = a.concat(leftPattern);
        }
        for (var i = 0; i < rightCount; ++i) {
            a = a.concat(rightPattern);
        }
        return a;
    },
    generateBeat: function() {
        var stepCount = this.stepCount;
        var trigCount = this.triggers.onsets.value = Math.min(this.triggers.onsets.value, stepCount);
        var accentCount = this.accents.onsets.value = Math.min(this.accents.onsets.value, trigCount);

        var triggers = this.generateEuclidean(trigCount, stepCount);
        var packedAccents = this.generateEuclidean(accentCount, trigCount);
        packedAccents = this.rotateArray(packedAccents, this.accents.rotation.value);
        
        var accents = [ ];
        accents.length = stepCount;
        var accentIndex = 0;
        for (var i = 0; i < stepCount; ++i) {
            var accented = false;
            if (triggers[i]) {
                accented = packedAccents[accentIndex];
                ++accentIndex;
            }
            accents[i] = accented;
        }

        triggers = this.rotateArray(triggers, this.triggers.rotation.value);
        accents = this.rotateArray(accents, this.triggers.rotation.value);
        this.currentBeat = { triggers: triggers, accents: accents };

        var preset = getElement('preset');
        var offset = 0;
        var patternIndex = this.firstPattern;
        do {
            var pattern = preset.patterns[patternIndex];
            var steps = pattern.steps;
            var patternChannel = pattern.channels[this.currentChannel];
            for (var i = 0; i < steps; ++i) {
                patternChannel.triggers[i] = triggers[offset + i];
                patternChannel.accents[i] = accents[offset + i];
            }
            offset += steps;
            ++patternIndex;
            if (patternIndex < PATTERN_COUNT) {
                pattern.chained = offset < stepCount;
            }
        } while (pattern.chained);
        this.chainLength = patternIndex - this.firstPattern;

        preset.modified = true;
        setElement('preset', preset);
        this.lastPresetId = getElementId('preset');
    },
    Parameter: createClass({
        constructor: function(lane, variable) {
            this.value = 0;
            this.lane = lane;
            this.variable = variable;
        },
        saveUndo: function() {
            saveUndo('Change ' + this.lane[0].toUpperCase() + this.lane.substr(1)
                    + ' ' + this.variable[0].toUpperCase() + this.variable.substr(1), true);
        },
        max: function max() {
            return (this.lane === "triggers" ? euclideanBeat.stepCount : euclideanBeat.triggers.onsets.value)
                    - (this.variable === "rotation" ? 1 : 0);
        },
        get: function get() { return this.value; },
        set: function set(v) {
            var y = clamp(Math.round(+v), 0, this.max());
            if (y !== this.value) {
                this.value = y;
                this.saveUndo();
                euclideanBeat.generateBeat();
            }
        },
        enabled: function enabled(p) {
            p = +unescape(p);
            return (p > 0 ? this.value < this.max() : this.value > 0);
        },
        execute: function execute(p) {
            var x = this.value;
            var y = clamp(x + (+unescape(p)), 0, this.max());
            if (x !== y) {
                this.value = y;
                this.saveUndo();
                euclideanBeat.generateBeat();
            }
        }
    }),
    patternChainStart: function patternChainStart(patternsArray, ofPatternIndex) {
        while (ofPatternIndex > 0 && patternsArray[ofPatternIndex - 1].chained) {
            --ofPatternIndex;
        }
        return ofPatternIndex;
    },
    presetIdentity: function presetIdentity() { return getElementId('preset'); },
    selectedChannel: function selectedChannel() { return selected('channel'); },
    playPosition: function playPosition() {
        this.syncFromPreset();

        var visuals = getElement('visuals');
        var currentPattern = visuals.currentPattern;
        if (visuals.isPlaying && currentPattern >= this.firstPattern
                && currentPattern < this.firstPattern + this.chainLength) {
            var stepToPosition = this.patternAndStepToPosition[currentPattern];
            if (stepToPosition) {
                if (visuals.currentStep < stepToPosition.length) {
                    return stepToPosition[visuals.currentStep];
                }
            }
        }
        return -1;
    },
    stepColors: function stepColors() {
        this.syncFromPreset();

        var triggers = this.currentBeat.triggers;
        var accents = this.currentBeat.accents;
        var s = '';
        for (var i = 0; i < this.stepCount; ++i) {
            if (s !== '') {
                s += ',';
            }
            s += (triggers[i] ? (accents[i] ? 2 : 1) : 0);
        }
        return s;
    },
    findRotation: function findRotation(target, source) {
        var bestScore = -1;
        var bestRotation = 0;
        for (var thisRotation = 0; thisRotation < source.length; ++thisRotation) {
            var thisScore = 0;
            for (var sourceIndex = 0; sourceIndex < source.length; ++sourceIndex) {
                thisScore += (source[sourceIndex] === target[(sourceIndex + thisRotation) % source.length] ? 1 : 0);
            }
            if (thisScore > bestScore) {
                bestScore = thisScore;
                bestRotation = thisRotation;
            }
        }
        return [ bestRotation, bestScore ];
    },
    syncFromPreset: function syncFromPreset() {
        var currentPresetId = getElementId('preset');
        var selectedChannel = selected('channel');
        if (this.lastPresetId !== currentPresetId || this.currentChannel !== selectedChannel) {
            this.lastPresetId = currentPresetId;
            this.currentChannel = selectedChannel;

            var patterns = getElement('preset').patterns;
            var currentPattern = selected('pattern');
            var firstPattern = this.firstPattern = this.patternChainStart(patterns, currentPattern);

            var currentBeat = this.currentBeat = { triggers: [ ], accents: [ ] };
            var patternIndex = firstPattern;
            var patternAndStepToPosition = this.patternAndStepToPosition = [ ];
            var stepCount = 0;
            var trigCount = 0;
            do {
                var pattern = patterns[patternIndex];
                var steps = pattern.steps;
                var patternChannel = pattern.channels[this.currentChannel];
                var triggers = patternChannel.triggers;
                currentBeat.triggers = currentBeat.triggers.concat(triggers.slice(0, steps));
                currentBeat.accents = currentBeat.accents.concat(patternChannel.accents.slice(0, steps));
                var stepToPosition = patternAndStepToPosition[patternIndex] = [ ];
                for (var i = 0; i < steps; ++i) {
                    stepToPosition[i] = stepCount;
                    if (triggers[i]) {
                        ++trigCount;
                    }
                    ++stepCount;
                }
                ++patternIndex;
            } while (patternIndex < PATTERN_COUNT && pattern.chained);
            this.chainLength = patternIndex - firstPattern;
            this.stepCount = stepCount;

            var euclideanTriggers = this.generateEuclidean(trigCount, this.stepCount);
            var bestRotation = this.findRotation(this.currentBeat.triggers, euclideanTriggers);
            var trigRotation = bestRotation[0];
            var perfectScore = (bestRotation[1] === this.stepCount);

            var packedAccents = [ ];
            packedAccents.length = trigCount;
            var accentCount = 0;
            var packedIndex = 0;
            for (var i = 0; i < this.stepCount; ++i) {
                if (euclideanTriggers[i]) {
                    var accented = (this.currentBeat.accents[(i + trigRotation) % this.stepCount]);
                    accentCount += (accented ? 1 : 0);
                    packedAccents[packedIndex] = accented;
                    ++packedIndex;
                }
            }
            var euclideanAccents = this.generateEuclidean(accentCount, trigCount);
            bestRotation = this.findRotation(packedAccents, euclideanAccents);
            var accentRotation = bestRotation[0];
            perfectScore = (perfectScore && bestRotation[1] === trigCount);

            this.triggers.onsets.value = trigCount;
            this.triggers.rotation.value = trigRotation;
            this.accents.onsets.value = accentCount;
            this.accents.rotation.value = accentRotation;

            this.disabled = !perfectScore;
        }
    },
    enable: function enable() {
        saveUndo('Enable EuclideanBeat');
        this.generateBeat();
        this.disabled = false;
    },
    disabled: false,
    firstPattern: 0,
    chainLength: 0,
    currentChannel: 0,
    stepCount: 0,
    currentBeat: null,
    lastPresetId: null,
    patternAndStepToPosition: [ ]
});

Object.assign(euclideanBeat, {
    triggers: {
        onsets: new euclideanBeat.Parameter('triggers', 'onsets'),
        rotation: new euclideanBeat.Parameter('triggers', 'rotation')
    },
    accents: {
        onsets: new euclideanBeat.Parameter('accents', 'onsets'),
        rotation: new euclideanBeat.Parameter('accents', 'rotation')
    }
});

euclideanBeat.syncFromPreset();
