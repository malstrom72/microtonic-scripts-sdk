if (!this.polyChain) {
    polyChain = {
        windowPosition: "",
        windowZOrder: "",
        uiOpen: false,
        sourcePatternIndex: 0,
        chainStart: 0,
        chainLength: 8,
        sourcePattern: null,
        loopRows: [ ],
        zero: 0,
        lastPresetId: 0,
        lastSelected: -1,
        syncedOn: false,
        outOfSyncOn: true,
        stateHistory: { },
        historyKeys: [ ],
        historyCap: 200
    };
}

Object.assign(polyChain, {
    startup: function() {
        if (this.uiOpen) {
            return;
        }

        this.uiOpen = true;

        if (!this.sourcePattern) {
            this.captureCurrentSource(true);
        }

        this.lastPresetId = 0;
        this.lastSelected = -1;
    },
    shutdown: function() {
        this.uiOpen = false;
    },
    capture: {
        execute: function() {
            saveUndo(polyChain.captureCaption.get());
            polyChain.captureCurrentSource(true);
            polyChain.generate();
        }
    },
    captureCurrentSource: function(resetLengthControls) {
        this.sourcePatternIndex = selected("pattern");
        this.updateChainStart();
        this.sourcePattern = this.captureSourcePattern();

        if (resetLengthControls) {
            this.resetLengths();
        }
    },
    updateChainStart: function() {
        this.chainStart = Math.min(this.sourcePatternIndex, PATTERN_COUNT - this.chainLength);
    },
    setChainLength: function(length) {
        var next = clamp(Math.round(+length), 1, PATTERN_COUNT);

        if (next !== this.chainLength) {
            saveUndo("Set Chain Length", true);
            this.chainLength = next;
            this.updateChainStart();
            this.generate();
        }
    },
    resetLengths: function() {
        var i;
        var sourceLength = this.sourcePattern ? this.sourcePattern.steps : PATTERN_STEP_COUNT;

        for (i = 0; i < CHANNEL_COUNT; ++i) {
            this.loopRows[i].reset(sourceLength);
        }
    },
    captureSourcePattern: function() {
        var preset = getElement("preset");
        var source = preset.patterns[this.sourcePatternIndex];
        var captured = {
            steps: source.steps,
            channels: [ ]
        };
        var ch;
        var step;

        for (ch = 0; ch < CHANNEL_COUNT; ++ch) {
            var sourceChannel = source.channels[ch];
            var channel = captured.channels[ch] = {
                triggers: [ ],
                accents: [ ],
                fills: [ ]
            };

            for (step = 0; step < PATTERN_STEP_COUNT; ++step) {
                var active = (step < source.steps);
                channel.triggers[step] = (active ? !!sourceChannel.triggers[step] : false);
                channel.accents[step] = (active ? !!sourceChannel.accents[step] : false);
                channel.fills[step] = (active ? !!sourceChannel.fills[step] : false);
            }
        }

        return captured;
    },
    hashChainRegion: function(preset, start, length) {
        // Fingerprint of a chained pattern block [start, start+length), used as
        // the memo key in stateHistory to recognise whether Microtonic currently
        // holds a chain this script generated.
        //
        // Mixing must stay inside 32-bit integer range. A classic FNV
        // `hash = (hash + v) * 16777619` overflows 2^53 within a couple of
        // iterations, and over ~1000 step values the rounding noise swamps the
        // content (early steps stop affecting the result). The `* 31` form below
        // keeps every intermediate an exact int32 via `<< 5` + `| 0`.
        var hash = 0;
        var patternOffset;
        var ch;
        var step;

        for (patternOffset = 0; patternOffset < length; ++patternOffset) {
            var pattern = preset.patterns[start + patternOffset];

            hash = (((hash << 5) - hash) + pattern.steps) | 0;
            hash = (((hash << 5) - hash) + (pattern.chained ? 1 : 0)) | 0;

            for (ch = 0; ch < CHANNEL_COUNT; ++ch) {
                var channel = pattern.channels[ch];

                for (step = 0; step < PATTERN_STEP_COUNT; ++step) {
                    var bits = (channel.triggers[step] ? 1 : 0)
                        | (channel.accents[step] ? 2 : 0)
                        | (channel.fills[step] ? 4 : 0);
                    hash = (((hash << 5) - hash) + bits) | 0;
                }
            }
        }

        return hash >>> 0;
    },
    walkChain: function(preset, c) {
        // The "current chain" is the maximal run of `chained` patterns containing
        // the current pattern. (`chained` on pattern i means it plays into i+1.)
        // Returns [start, end] inclusive.
        var patterns = preset.patterns;
        var s = c;
        var e = c;

        while (s > 0 && patterns[s - 1].chained) {
            --s;
        }
        while (e < PATTERN_COUNT - 1 && patterns[e].chained) {
            ++e;
        }

        return [ s, e ];
    },
    snapshotLoops: function() {
        var loops = [ ];
        var i;

        for (i = 0; i < CHANNEL_COUNT; ++i) {
            loops[i] = [ this.loopRows[i].start, this.loopRows[i].length ];
        }

        return loops;
    },
    rememberState: function(hash) {
        // Memoise the inputs (source buffer + loop windows) that produced this
        // chain, keyed by the chain's content hash, so undo/redo onto it can be
        // recognised and the UI restored. The source is held by reference (it
        // only changes on Capture and is never mutated after), so all the
        // loop/length states between two captures share one object.
        if (!this.stateHistory[hash]) {
            this.historyKeys.push(hash);

            if (this.historyKeys.length > this.historyCap) {
                delete this.stateHistory[this.historyKeys.shift()];
            }
        }

        this.stateHistory[hash] = {
            source: this.sourcePattern,
            loops: this.snapshotLoops()
        };
    },
    restore: function(entry, s, e) {
        var length = e - s + 1;
        var changed = (this.sourcePattern !== entry.source)
            || (this.chainStart !== s)
            || (this.chainLength !== length);
        var i;

        this.sourcePattern = entry.source;
        this.sourcePatternIndex = s;
        this.chainStart = s;
        this.chainLength = length;

        for (i = 0; i < CHANNEL_COUNT; ++i) {
            var row = this.loopRows[i];

            if (changed || row.start !== entry.loops[i][0] || row.length !== entry.loops[i][1]) {
                row.start = entry.loops[i][0];
                row.length = entry.loops[i][1];
                row.refreshCells();
            }
        }
    },
    checkSync: function() {
        // Cheap gate: only work when the preset id (any document change) or the
        // current pattern selection has moved since last time.
        var id = getElementId("preset");
        var c = selected("pattern");

        if (id === this.lastPresetId && c === this.lastSelected) {
            return;
        }

        this.lastPresetId = id;
        this.lastSelected = c;

        var preset = getElement("preset");
        var range = this.walkChain(preset, c);
        var s = range[0];
        var e = range[1];
        var entry = this.stateHistory[this.hashChainRegion(preset, s, e - s + 1)];

        if (entry) {
            // Recognised: a chain we generated. Restore the source + loops that
            // produced it so the display matches the document.
            this.restore(entry, s, e);
            this.syncedOn = true;
            this.outOfSyncOn = false;
        } else {
            this.syncedOn = false;
            this.outOfSyncOn = true;
        }
    },
    generate: function() {
        var preset = getElement("preset");
        var patternOffset;
        var ch;
        var step;

        if (this.chainStart > 0) {
            preset.patterns[this.chainStart - 1].chained = false;
        }

        for (patternOffset = 0; patternOffset < this.chainLength; ++patternOffset) {
            var patternIndex = this.chainStart + patternOffset;
            var pattern = preset.patterns[patternIndex];
            pattern.steps = PATTERN_STEP_COUNT;

            // The `chained` field exists only on patterns 0..PATTERN_COUNT - 2.
            // The final preset pattern has no next pattern to chain into, so it
            // is already a natural loop boundary and must not be written.
            if (patternIndex < PATTERN_COUNT - 1) {
                pattern.chained = (patternOffset < this.chainLength - 1);
            }

            for (ch = 0; ch < CHANNEL_COUNT; ++ch) {
                var sourceChannel = this.sourcePattern.channels[ch];
                var targetChannel = pattern.channels[ch];
                var loopRow = this.loopRows[ch];
                var patternBaseStep = patternOffset * PATTERN_STEP_COUNT;

                for (step = 0; step < PATTERN_STEP_COUNT; ++step) {
                    var sourceStep = loopRow.start + ((patternBaseStep + step) % loopRow.length);
                    targetChannel.triggers[step] = sourceChannel.triggers[sourceStep];
                    targetChannel.accents[step] = sourceChannel.accents[sourceStep];
                    targetChannel.fills[step] = sourceChannel.fills[sourceStep];
                }
            }
        }

        preset.modified = true;
        setElement("preset", preset);

        // Record the inputs that produced this chain (for undo/redo restore) and
        // mark our own write so checkSync() does not flag it as external.
        this.rememberState(this.hashChainRegion(preset, this.chainStart, this.chainLength));
        this.lastPresetId = getElementId("preset");
        this.lastSelected = selected("pattern");
        this.syncedOn = true;
        this.outOfSyncOn = false;
    },
    updatePlayheads: function() {
        // Refresh the play-position markers once per tick (registered as a repeating
        // autoexecs action). One getElement("visuals") drives all rows; each row's
        // marker is a single overlay group moved via its `playX` offset and shown via
        // `playOn`, so there is no per-cell state to write.
        //
        // The sync check rides on this same tick rather than registering its own
        // `repeat` autoexec, so there is a single per-frame entry point.
        this.checkSync();

        var visuals = getElement("visuals");
        var playing = visuals.isPlaying
            && this.syncedOn
            && visuals.currentPattern >= this.chainStart
            && visuals.currentPattern < this.chainStart + this.chainLength;
        var chainPosition = playing
            ? (visuals.currentPattern - this.chainStart) * PATTERN_STEP_COUNT + visuals.currentStep
            : -1;
        var ch;
        var row;

        for (ch = 0; ch < CHANNEL_COUNT; ++ch) {
            row = this.loopRows[ch];

            if (chainPosition >= 0) {
                row.playX = (row.start + (chainPosition % row.length) + 1) * 12;
                row.playOn = true;
            } else {
                row.playOn = false;
            }
        }
    },
    captureCaption: {
        get: function() {
            return "Capture Pattern " + String.fromCharCode(65 + selected("pattern"));
        }
    },
    chainLengthControl: {
        get: function() {
            return "Repeat " + polyChain.chainLength + " Bars";
        }
    },
    chainLengthValue: {
        get: function() {
            return polyChain.chainLength;
        },
        set: function(v) {
            polyChain.setChainLength(v);
        }
    },
    LoopMouseIndex: createClass({
        constructor: function(row) {
            this.row = row;
            this.value = "";
        },
        get: function() {
            return this.value;
        },
        set: function(v) {
            this.value = v;

            if (v !== "") {
                this.row.updateDrag(this.row.pointerIndex());
            }
        }
    }),
    LoopRow: createClass({
        constructor: function(channel) {
            var self = this;
            var i;

            this.channel = channel;
            this.start = 0;
            this.length = PATTERN_STEP_COUNT;
            this.dragging = "";
            this.savedUndo = false;
            this.lastPointerIndex = 0;
            this.cells = [ ];
            this.playX = 0;
            this.playOn = false;
            this.mouseIndex = new polyChain.LoopMouseIndex(this);
            this.pointerDown = {
                execute: function() {
                    self.beginDrag();
                }
            };
            this.pointerUp = {
                execute: function() {
                    if (self.dragging === "pending" && self.mouseIndex.value !== "") {
                        self.updateDrag(self.pointerIndex());
                    }

                    self.dragging = "";
                    self.refreshCells();
                }
            };

            for (i = 0; i < 18; ++i) {
                this.cells[i] = 0;
            }

            this.refreshCells();
        },
        reset: function(length) {
            this.start = 0;
            this.length = clamp(Math.round(+length), 1, PATTERN_STEP_COUNT);
            this.refreshCells();
        },
        get: function() {
            return (this.start + 1) + "-" + this.endBoundary();
        },
        startVisualIndex: function() {
            return this.start;
        },
        endBoundary: function() {
            return this.start + this.length;
        },
        endVisualIndex: function() {
            return this.endBoundary() + 1;
        },
        refreshCells: function() {
            var i;
            var step;
            var inLoop;
            var trigger;
            var accent;

            for (i = 0; i < 18; ++i) {
                this.cells[i] = 0;
            }

            if (polyChain.sourcePattern) {
                for (step = 0; step < PATTERN_STEP_COUNT; ++step) {
                    inLoop = (step >= this.start && step < this.endBoundary());
                    trigger = polyChain.sourcePattern.channels[this.channel].triggers[step];
                    accent = polyChain.sourcePattern.channels[this.channel].accents[step];

                    this.cells[step + 1] = inLoop
                        ? (trigger ? (accent ? 6 : 5) : 4)
                        : (trigger ? (accent ? 3 : 2) : 1);
                }
            }

            this.cells[this.startVisualIndex()] = 7;
            this.cells[this.endVisualIndex()] = 8;
        },
        setStartBoundary: function(index) {
            var next = clamp(Math.round(+index), 0, this.endBoundary() - 1);

            if (next !== this.start) {
                this.saveLoopUndo();
                this.length = this.endBoundary() - next;
                this.start = next;
                this.refreshCells();
                polyChain.generate();
            }
        },
        setEndBoundary: function(index) {
            var next = clamp(Math.round(+index), this.start + 1, PATTERN_STEP_COUNT);

            if (next !== this.endBoundary()) {
                this.saveLoopUndo();
                this.length = next - this.start;
                this.refreshCells();
                polyChain.generate();
            }
        },
        saveLoopUndo: function() {
            // One undo entry per drag gesture: snapshot on the first change only.
            if (!this.savedUndo) {
                saveUndo("Adjust Channel " + (this.channel + 1) + " Loop", true);
                this.savedUndo = true;
            }
        },
        beginDrag: function() {
            this.dragging = "pending";
            this.savedUndo = false;
        },
        chooseDragTarget: function(index) {
            var startDistance = Math.abs(index - this.startVisualIndex());
            var endDistance = Math.abs(index - this.endVisualIndex());

            this.dragging = (startDistance <= endDistance) ? "start" : "end";
        },
        updateDrag: function(index) {
            var next;

            if (!this.dragging) {
                return;
            }

            if (this.dragging === "pending") {
                this.chooseDragTarget(index);
            }

            if (this.dragging === "start") {
                this.setStartBoundary(index);
            } else {
                next = clamp(Math.round(+index), 0, 17);
                this.setEndBoundary(next - 1);
            }
        },
        pointerIndex: function() {
            var index = Math.round(+this.mouseIndex.value);

            if (isNaN(index)) {
                return this.lastPointerIndex;
            }

            this.lastPointerIndex = clamp(index, 0, 17);

            return this.lastPointerIndex;
        }
    })
});

(function() {
    var ch;

    for (ch = 0; ch < CHANNEL_COUNT; ++ch) {
        var previous = polyChain.loopRows[ch];
        var row = new polyChain.LoopRow(ch);

        if (previous) {
            row.start = previous.start;
            row.length = previous.length;
            row.lastPointerIndex = previous.lastPointerIndex || 0;
            row.refreshCells();
        }

        polyChain.loopRows[ch] = row;
    }
})();
