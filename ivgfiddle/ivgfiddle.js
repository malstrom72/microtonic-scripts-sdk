"use strict";

const MAX_LOG_SIZE = 64 * 1024;
const MAX_LOG_LINES = 1000;

const leftPanelElement = document.getElementById("leftPanel");
const leftRightSplitElement = document.getElementById('leftRightSplit');
const traceElement = document.getElementById('trace');
const traceDiv = document.getElementById('traceDiv');
const ivgCanvas = document.getElementById('ivgCanvas');
const ivgContext = ivgCanvas.getContext("2d");

let allLogLines = '';
let traceLinesCount = 0;
let lastLogLine = null;
let repeatingLogLineCount = 0;

function clearTrace() {
	allLogLines = '';
	traceLinesCount = 0;
	lastLogLine = null;
	repeatingLogLineCount = 0;
	traceElement.textContent = '';
	traceDiv.scrollTop = 0;
}

function trace(message) {
	const allLines = traceElement.textContent;
	while (allLogLines.length > MAX_LOG_SIZE || traceLinesCount >= MAX_LOG_LINES) {
		const offset = allLogLines.indexOf("\n");
		if (offset < 0) {
			break;
		}
		allLogLines = allLogLines.substr(offset + 1);
		--traceLinesCount;
	}
	if (lastLogLine === message) {
		if (repeatingLogLineCount > 1) {
			const offset = allLogLines.lastIndexOf(' *');
			if (offset >= 0) {
				allLogLines = allLogLines.substr(0, offset);
			}
		} else {
			allLogLines = allLogLines.substr(0, allLogLines.length - 1); // remove last LF
		}
		++repeatingLogLineCount;
		allLogLines += " *" + repeatingLogLineCount + "\n";
	} else {
		allLogLines += message + "\n";
		++traceLinesCount;
		lastLogLine = message;
		repeatingLogLineCount = 1;
	}
	traceElement.textContent = allLogLines;
	traceDiv.scrollTop = traceDiv.scrollHeight;
}

// Can't use cwrap to pass very long strings, as they are placed on the stackm and not the heap.
const rasterizeIVG = function(source, scaling) {
	const size = Module.lengthBytesUTF8(source) + 1;
	const stringPointer = Module._malloc(size);
	Module.stringToUTF8(source, stringPointer, size);
	const result = Module._rasterizeIVG(stringPointer, scaling);
	Module._free(stringPointer);
	return result;
};
const deallocatePixels = Module._deallocatePixels;

function runIVG() {
	clearTrace();
	trace("Running IVG");
	const linesCountWas = traceLinesCount;
	const start = Date.now();
	const sourceCode = aceEditor.getValue();
	localStorage.setItem("ivgSource", sourceCode);
	localStorage.setItem("runOnStartup", false);
	let ok = false;
	try {
		const pixelRatio = window.devicePixelRatio;
		const rasterPointer = rasterizeIVG(sourceCode, pixelRatio);
		const end = Date.now();
		if (rasterPointer !== 0) {
			let dimensions = new Int32Array(Module.HEAPU32.buffer, rasterPointer, 4);
			const left = dimensions[0];
			const top = dimensions[1];
			const width = dimensions[2];
			const height = dimensions[3];
			dimensions = null;
			let pixelData = new Uint8Array(Module.HEAPU32.buffer, rasterPointer + 4 * 4, width * height * 4);
			deallocatePixels(rasterPointer);
			ivgCanvas.width = width;
			ivgCanvas.height = height;
			const imageData = ivgContext.createImageData(width, height);
			imageData.data.set(pixelData);
			pixelData = null;
			ivgCanvas.style.width = width / pixelRatio + "px";
			ivgCanvas.style.height = height / pixelRatio + "px";
			ivgCanvas.style.transform = "translate(" + left / pixelRatio + "px," + top / pixelRatio + "px)";
			ivgContext.putImageData(imageData, 0, 0);
			trace("Completed IVG");
			trace("Time spent: " + (end - start) + "ms");
			ok = true;
		} else {
			trace("Aborted IVG");
		}
		localStorage.setItem("runOnStartup", true);
	}
	catch (e) {
		trace("Rasterization crashed");
		trace(e);
	}
	if (!ok) {
		ivgContext.beginPath();
		ivgContext.moveTo(0, 0);
		ivgContext.lineTo(ivgCanvas.width, ivgCanvas.height);
		ivgContext.moveTo(0, ivgCanvas.height);
		ivgContext.lineTo(ivgCanvas.width, 0);
		ivgContext.strokeStyle = "red";
		ivgContext.lineWidth = 10;
		ivgContext.stroke();
	}
}

const aceEditor = ace.edit("editor");
aceEditor.setTheme("ace/theme/twilight");
const aceSession = aceEditor.getSession();
aceSession.setUseSoftTabs(false);
aceSession.setMode("ace/mode/ivg");
let recompileTimer = null;
aceSession.on('change', function(e) {
	if (recompileTimer !== null) {
		clearTimeout(recompileTimer);
		recompileTimer = null;
	};
	recompileTimer = setTimeout(runIVG, 500);
});

let isDragging = false;
let currentX;
let currentPanelWidth;
leftRightSplitElement.addEventListener('mousedown', function(e) {
  isDragging = true;
  currentX = e.clientX;
  currentPanelWidth = leftPanelElement.offsetWidth;
  e.preventDefault();
});
document.addEventListener('mousemove', function(e) {
  if (isDragging) {
	leftPanelElement.style.width = (currentPanelWidth + e.clientX - currentX) + 'px';
  }
});
document.addEventListener('mouseup', function(e) {
  isDragging = false;
});
