//
// BeatDrop for Microtonic.
//
// Tetris reimagined as a step sequencer. The well is 8 columns wide (one per drum
// channel) and 22 rows tall. The bottom 16 rows ARE the Microtonic 16-step pattern,
// read bottom -> top (bottom row = step that plays first); the 6 rows above are play
// space for maneuvering.
//
// Each falling piece's four cells are a mix of states assigned at spawn:
//   1 = inert   - drawn almost transparent; stacks but writes no trigger
//   2 = trigger - writes a trigger on that channel/step
//   3 = accent  - writes an accented trigger
// When a piece locks, the bottom-16 grid is recomputed into the currently selected
// pattern, so the sound comes from Microtonic's own sequencer (no triggerChannel
// sound effects). The script never touches transport -- press play in Microtonic /
// your DAW yourself. A playhead row shows the playing step while it runs.
//
// A row full across all 8 channels clears (with a white strobe) and the rows above
// drop down; it's game over when a new piece can't spawn.
//
// Controls (mouse-only): move = steer, left-click = hard drop, right-click = rotate.
//
(function() {

	var COLS = 8;					// drum channels 0..7
	var STEP_ROWS = 16;				// Microtonic pattern steps
	var HEADROOM = 6;				// play space above the step grid
	var ROWS = STEP_ROWS + HEADROOM;
	var CELL = 18;					// must match @cell in BeatDrop_main.cushy
	var SPAWN_X = 2;				// (COLS - 4) / 2
	var FLASH_HALF = 80;			// line-clear strobe: ms per on/off half-blink
	var FLASH_TOTAL = 480;			// total strobe (~3 blinks) before the rows collapse

	var EMPTY = 0, INERT = 1, TRIG = 2, ACCENT = 3;

	// Step <-> board-row mapping (bottom row = step 0).
	function rowForStep(s) { return ROWS - 1 - s; }

	// Tetromino spawn shapes + bounding-box size. Rotations are COMPUTED by rotating
	// cells within the box, so cell index k stays tied to the same physical mino in
	// every rotation -- which means a cell's state (inert/trigger/accent) rotates
	// along with it.
	var BASE = [
		{ n: 4, cells: [ [0,1],[1,1],[2,1],[3,1] ] },	// I
		{ n: 2, cells: [ [0,0],[1,0],[0,1],[1,1] ] },	// O
		{ n: 3, cells: [ [1,0],[0,1],[1,1],[2,1] ] },	// T
		{ n: 3, cells: [ [1,0],[2,0],[0,1],[1,1] ] },	// S
		{ n: 3, cells: [ [0,0],[1,0],[1,1],[2,1] ] },	// Z
		{ n: 3, cells: [ [0,0],[0,1],[1,1],[2,1] ] },	// J
		{ n: 3, cells: [ [2,0],[0,1],[1,1],[2,1] ] }	// L
	];

	var KICKS = [ 0, -1, 1, -2, 2 ];

	//
	// Persistent, rerun-safe state.
	//
	var doInit = false;
	if (!this.beatdrop) {
		this.beatdrop = {
			board: [],				// ROWS*COLS cells of EMPTY/INERT/TRIG/ACCENT
			cur: null,				// { type, rot, x, y, flags:[4] }
			nextType: 0,
			nextFlags: [ TRIG, TRIG, TRIG, TRIG ],
			bag: [],

			gameOver: false,
			paused: false,
			piecesPlaced: 0,
			lines: 0,
			trigCount: 0,
			loopSteps: 1,			// dynamic pattern length (steps that actually play)
			patternIndex: 0,		// pattern slot being written / played

			dirty: true,
			lastDrop: 0,
			mouseX: -1,
			playRowNum: -1,			// playhead board row (numeric), -1 = not playing

			// bound render strings
			cells: '[]',
			ghostCells: '[]',
			nextCells: '[]',
			playRow: '-1',			// bound to the board vector (string)
			patternText: 'A',
			trigText: '0',
			linesText: '0',
			loopText: '1',
			statusText: '',
			hintVisible: true,		// "PRESS PLAY" footer: hidden while playing, blinks while stopped + running
			flashRows: '[]',		// rows flashing white during a line-clear strobe (bound)

			clearing: false,		// a line-clear strobe is in progress
			clearStart: 0,			// Date.now() when the strobe began
			clearRowsStr: '[]',		// the full-row list being flashed
			clearOver: false,		// the locking piece also topped out

			windowPosition: '',
			windowZOrder: ''
		};
		doInit = true;
	}
	var beatdrop = this.beatdrop;

	// Cells of a piece at a rotation, in stable per-mino order: cell k is base cell k
	// rotated `rot` quarter-turns clockwise within the piece's box ((x,y) -> (n-1-y,x)).
	function cellsOf(type, rot) {
		var base = BASE[type], n = base.n, src = base.cells, out = [];
		for (var i = 0; i < src.length; ++i) {
			var x = src[i][0], y = src[i][1];
			for (var r = 0; r < rot; ++r) {
				var nx = n - 1 - y;
				y = x;
				x = nx;
			}
			out.push([ x, y ]);
		}
		return out;
	}

	function collides(type, rot, px, py) {
		var c = cellsOf(type, rot);
		for (var i = 0; i < c.length; ++i) {
			var x = px + c[i][0];
			var y = py + c[i][1];
			if (x < 0 || x >= COLS || y >= ROWS) { return true; }
			if (y >= 0 && beatdrop.board[y * COLS + x]) { return true; }
		}
		return false;
	}

	function dropIntervalMs() {
		var ms = 620 - beatdrop.piecesPlaced * 5;
		return (ms < 300 ? 300 : ms);
	}

	// Each cell of a piece is inert / trigger / accent (~60 / 20 / 20), with at
	// least one active cell so every piece can contribute to the beat.
	function randomFlags() {
		var f = [], anyActive = false;
		for (var i = 0; i < 4; ++i) {
			var r = random.uniform();
			var s = (r < 0.6 ? INERT : (r < 0.8 ? TRIG : ACCENT));
			f[i] = s;
			if (s >= TRIG) { anyActive = true; }
		}
		if (!anyActive) { f[random.integer(3)] = (random.uniform() < 0.5 ? TRIG : ACCENT); }
		return f;
	}

	function nextFromBag() {
		if (beatdrop.bag.length === 0) {
			var a = [ 0, 1, 2, 3, 4, 5, 6 ];
			for (var i = a.length - 1; i > 0; --i) {
				var j = random.integer(i);
				var t = a[i]; a[i] = a[j]; a[j] = t;
			}
			beatdrop.bag = a;
		}
		return beatdrop.bag.pop();
	}

	function spawn() {
		var c = { type: beatdrop.nextType, rot: 0, x: SPAWN_X, y: 0, flags: beatdrop.nextFlags };
		beatdrop.nextType = nextFromBag();
		beatdrop.nextFlags = randomFlags();
		beatdrop.cur = c;
		beatdrop.dirty = true;
		beatdrop.lastDrop = Date.now();
		if (collides(c.type, c.rot, c.x, c.y)) { endGame(); }
	}

	function endGame() {
		beatdrop.gameOver = true;
		beatdrop.dirty = true;
	}

	//
	// Recompute the bottom-16 grid into the selected pattern and (re)start playback.
	// Only called on lock / new game, never per frame.
	//
	function writePattern() {
		var preset = getElement('preset');
		var pi = selected('pattern');
		beatdrop.patternIndex = pi;
		var pat = preset.patterns[pi];
		// Break the chain around this pattern so it loops in isolation, even after
		// loading a preset whose patterns are chained: unset `chained` on the previous
		// pattern (so it won't chain INTO the next one) and on this one (so it won't
		// chain OUT). `chained` only exists on patterns 0..10.
		if (pi > 0 && preset.patterns[pi - 1].chained !== undefined && preset.patterns[pi - 1].chained !== null) {
			preset.patterns[pi - 1].chained = false;
		}
		if (pi < PATTERN_COUNT - 1 && pat.chained !== undefined && pat.chained !== null) {
			pat.chained = false;
		}

		var b = beatdrop.board, count = 0, maxStep = -1;
		for (var ch = 0; ch < COLS; ++ch) {
			var chan = pat.channels[ch];
			for (var s = 0; s < STEP_ROWS; ++s) {
				var v = b[rowForStep(s) * COLS + ch];
				if (v && s > maxStep) { maxStep = s; }		// any occupied cell extends the loop
				var trig = (v >= TRIG);
				chan.triggers[s] = trig;
				chan.accents[s] = (v === ACCENT);
				chan.fills[s] = false;
				if (trig) { ++count; }
			}
		}
		beatdrop.trigCount = count;

		// Loop exactly at the last placed row: 3 stacked rows -> 3 steps. Clamped 1..16.
		var loop = maxStep + 1;
		if (loop < 1) { loop = 1; }
		if (loop > STEP_ROWS) { loop = STEP_ROWS; }
		pat.steps = loop;
		beatdrop.loopSteps = loop;

		preset.modified = true;
		setElement('preset', preset);
	}

	// Classic Tetris row clear: a row full across all 8 channels clears and the rows
	// above shift down (carrying their cell states). This is the relief valve so the
	// well doesn't fill up as fast; it also keeps the beat evolving.
	function clearLines() {
		var b = beatdrop.board, cleared = 0;
		for (var y = ROWS - 1; y >= 0; --y) {
			var full = true;
			for (var x = 0; x < COLS; ++x) {
				if (!b[y * COLS + x]) { full = false; break; }
			}
			if (full) {
				for (var yy = y; yy > 0; --yy) {
					for (x = 0; x < COLS; ++x) {
						b[yy * COLS + x] = b[(yy - 1) * COLS + x];
					}
				}
				for (x = 0; x < COLS; ++x) { b[x] = EMPTY; }
				++cleared;
				++y;								// recheck this row index after the shift
			}
		}
		if (cleared > 0) {
			beatdrop.lines += cleared;
			beatdrop.dirty = true;
		}
		return cleared;
	}

	// Rows completely filled across all channels (detection only; clearLines does
	// the actual removal once the strobe finishes).
	function fullRows() {
		var b = beatdrop.board, rows = [];
		for (var y = 0; y < ROWS; ++y) {
			var full = true;
			for (var x = 0; x < COLS; ++x) { if (!b[y * COLS + x]) { full = false; break; } }
			if (full) { rows.push(y); }
		}
		return rows;
	}

	function lockPiece() {
		// Save an undo point before every drop, with collapse:true (saveUndo runs
		// before the document write -- it snapshots the state to return to).
		// Consecutive drops on the same pattern share the label and merge into one
		// "Undo BeatDrop on Pattern X" item. Re-saving on every drop is what keeps us
		// in sync with the document: after you undo and keep playing, the next drop
		// starts a fresh item so you can always get back. (A script-side "already
		// saved this pattern" flag would go stale, since undo rewinds the document
		// but not our JS state.)
		saveUndo('BeatDrop on Pattern ' + String.fromCharCode(65 + selected('pattern')), true);
		var c = beatdrop.cur, sh = cellsOf(c.type, c.rot), b = beatdrop.board, over = false;
		for (var i = 0; i < sh.length; ++i) {
			var x = c.x + sh[i][0];
			var y = c.y + sh[i][1];
			if (y < 0) { over = true; } else { b[y * COLS + x] = c.flags[i]; }
		}
		beatdrop.piecesPlaced += 1;

		// If this lock completed any rows, kick off the white-strobe clear and let
		// the tick finish it (collapse + writePattern + spawn). Otherwise resolve now.
		var fr = fullRows();
		if (fr.length > 0) {
			beatdrop.clearing = true;
			beatdrop.clearStart = Date.now();
			beatdrop.clearRowsStr = '[' + fr.join(',') + ']';
			beatdrop.clearOver = over;
			beatdrop.flashRows = beatdrop.clearRowsStr;	// start on the "on" frame
			beatdrop.cur = null;						// hide the just-locked piece during the flash
			beatdrop.dirty = true;
			return;
		}

		beatdrop.dirty = true;
		writePattern();
		if (over) { beatdrop.cur = null; endGame(); return; }
		spawn();
	}

	// Drives the line-clear strobe from the tick; finishes by collapsing the rows.
	function tickClear() {
		var elapsed = Date.now() - beatdrop.clearStart;
		if (elapsed >= FLASH_TOTAL) {
			beatdrop.clearing = false;
			beatdrop.flashRows = '[]';
			clearLines();				// actually remove the full rows + bump LINES
			writePattern();				// pattern now reflects the collapsed board
			beatdrop.dirty = true;
			if (beatdrop.clearOver) { beatdrop.cur = null; endGame(); } else { spawn(); }
			return;
		}
		var on = (Math.floor(elapsed / FLASH_HALF) % 2 === 0);
		var s = on ? beatdrop.clearRowsStr : '[]';
		if (s !== beatdrop.flashRows) { beatdrop.flashRows = s; beatdrop.dirty = true; }
	}

	function desiredX() {
		if (beatdrop.mouseX < 0 || !beatdrop.cur) { return beatdrop.cur ? beatdrop.cur.x : SPAWN_X; }
		var sh = cellsOf(beatdrop.cur.type, beatdrop.cur.rot);
		var minDx = 99, maxDx = -99;
		for (var i = 0; i < sh.length; ++i) {
			if (sh[i][0] < minDx) { minDx = sh[i][0]; }
			if (sh[i][0] > maxDx) { maxDx = sh[i][0]; }
		}
		var mx = COLS * CELL - beatdrop.mouseX;	// playfield is mirrored: channel 1 right, channel 8 left
		return Math.round(mx / CELL - (minDx + maxDx + 1) / 2);
	}

	function updatePlayhead() {
		var visuals = getElement('visuals');
		var playing = visuals.isPlaying;
		var row = (playing ? rowForStep(visuals.currentStep) : -1);
		if (row !== beatdrop.playRowNum) {
			beatdrop.playRowNum = row;
			beatdrop.playRow = '' + row;
			beatdrop.dirty = true;
		}
		// "PRESS PLAY" footer: hidden once transport runs; blinks (~1s) while the
		// game is actively running but stopped; steady otherwise (paused / over).
		if (playing) {
			beatdrop.hintVisible = false;
		} else if (beatdrop.cur && !beatdrop.gameOver && !beatdrop.paused) {
			beatdrop.hintVisible = (Math.floor(Date.now() / 500) % 2 === 0);
		} else {
			beatdrop.hintVisible = true;
		}
	}

	function tick() {
		updatePlayhead();

		if (beatdrop.clearing) {
			tickClear();
		} else if (beatdrop.cur && !beatdrop.gameOver && !beatdrop.paused) {
			var c = beatdrop.cur;

			var want = desiredX();
			var moved = false;
			while (want !== c.x && !collides(c.type, c.rot, c.x + (want > c.x ? 1 : -1), c.y)) {
				c.x += (want > c.x ? 1 : -1);
				moved = true;
			}
			if (moved) { beatdrop.dirty = true; }

			var now = Date.now();
			if (now - beatdrop.lastDrop >= dropIntervalMs()) {
				beatdrop.lastDrop = now;
				if (!collides(c.type, c.rot, c.x, c.y + 1)) {
					c.y += 1;
					beatdrop.dirty = true;
				} else {
					lockPiece();
				}
			}
		}

		if (beatdrop.dirty) {
			render();
			beatdrop.dirty = false;
		}
	}

	function rotate() {
		if (!beatdrop.cur || beatdrop.gameOver || beatdrop.paused) { return; }
		var c = beatdrop.cur, nr = (c.rot + 1) % 4;
		for (var i = 0; i < KICKS.length; ++i) {
			if (!collides(c.type, nr, c.x + KICKS[i], c.y)) {
				c.x += KICKS[i];
				c.rot = nr;
				beatdrop.dirty = true;
				return;
			}
		}
	}

	function hardDrop() {
		if (!beatdrop.cur || beatdrop.gameOver || beatdrop.paused) { return; }
		var c = beatdrop.cur;
		while (!collides(c.type, c.rot, c.x, c.y + 1)) { c.y += 1; }
		beatdrop.dirty = true;
		lockPiece();
	}

	function newGame() {
		var n = ROWS * COLS;
		beatdrop.board = [];
		for (var i = 0; i < n; ++i) { beatdrop.board[i] = EMPTY; }
		beatdrop.gameOver = false;
		beatdrop.paused = false;
		beatdrop.piecesPlaced = 0;
		beatdrop.lines = 0;
		beatdrop.clearing = false;	// cancel any in-flight line-clear strobe
		beatdrop.flashRows = '[]';
		beatdrop.bag = [];
		beatdrop.nextType = nextFromBag();
		beatdrop.nextFlags = randomFlags();
		beatdrop.dirty = true;
		writePattern();				// clears the pattern (you control play/stop in Microtonic)
		spawn();
	}

	function togglePause() {
		if (beatdrop.gameOver) { return; }
		beatdrop.paused = !beatdrop.paused;
		if (!beatdrop.paused) { beatdrop.lastDrop = Date.now(); }
		beatdrop.dirty = true;					// only freezes the falling pieces; transport untouched
	}

	function setMousePosition(s) {
		var xy = ('' + s).split(',');
		var mx = +xy[0];
		if (!isNaN(mx)) { beatdrop.mouseX = mx; }
	}

	function cellString(x, y, st) { return '[' + x + ',' + y + ',' + st + ']'; }

	function render() {
		var b = beatdrop.board, parts = [];
		for (var i = 0; i < b.length; ++i) {
			if (b[i]) {
				var x = i % COLS;
				var y = (i - x) / COLS;
				parts.push(cellString(x, y, b[i]));
			}
		}

		var ghosts = [];
		if (beatdrop.cur && !beatdrop.gameOver) {
			var c = beatdrop.cur, sh = cellsOf(c.type, c.rot);
			var gy = c.y;
			while (!collides(c.type, c.rot, c.x, gy + 1)) { gy += 1; }
			for (var k = 0; k < sh.length; ++k) {
				var gyy = gy + sh[k][1];
				if (gyy >= 0) { ghosts.push('[' + (c.x + sh[k][0]) + ',' + gyy + ']'); }
			}
			for (k = 0; k < sh.length; ++k) {
				var ay = c.y + sh[k][1];
				if (ay >= 0) { parts.push(cellString(c.x + sh[k][0], ay, c.flags[k])); }
			}
		}

		var np = [], ns = cellsOf(beatdrop.nextType, 0), maxnx = 0, m;
		for (m = 0; m < ns.length; ++m) { if (ns[m][0] > maxnx) { maxnx = ns[m][0]; } }
		for (m = 0; m < ns.length; ++m) {
			np.push(cellString(maxnx - ns[m][0], ns[m][1], beatdrop.nextFlags[m]));	// mirror to match the flipped board
		}

		beatdrop.cells = '[' + parts.join(',') + ']';
		beatdrop.ghostCells = '[' + ghosts.join(',') + ']';
		beatdrop.nextCells = '[' + np.join(',') + ']';
		beatdrop.patternText = String.fromCharCode(65 + beatdrop.patternIndex);	// 0..11 -> A..L (Microtonic pattern labels)
		beatdrop.trigText = '' + beatdrop.trigCount;
		beatdrop.linesText = '' + beatdrop.lines;
		beatdrop.loopText = '' + beatdrop.loopSteps;
		beatdrop.statusText = beatdrop.gameOver ? 'GAME OVER' : (beatdrop.paused ? 'PAUSED' : '');
	}

	function startup() {
		if (doInit || beatdrop.board.length === 0) {
			newGame();
		} else {
			beatdrop.lastDrop = Date.now();
			beatdrop.clearing = false;	// drop any half-finished strobe across a reload
			beatdrop.flashRows = '[]';
			writePattern();			// resync Microtonic with the current grid
		}
		render();
		beatdrop.dirty = false;
	}

	Object.assign(beatdrop, {
		startup: startup,
		tick: tick,
		rotate: rotate,
		hardDrop: hardDrop,
		newGame: newGame,
		pause: {
			execute: togglePause,
			checked: function() { return beatdrop.paused; }
		},
		mousePosition: { set: setMousePosition }
	});
})();
