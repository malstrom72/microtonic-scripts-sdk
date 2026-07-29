#!/usr/bin/env node
"use strict";

const fs = require("fs");

let outputString = "";
let indent = 0;
function output(line) {
	if (line === "]") {
		indent--;
	}
	outputString += "\t".repeat(indent) + line + "\n";
	if (line.endsWith("[")) {
		indent++;
	}
}

function warning(msg) {
	console.warn("Warning! " + msg);
}

function parseRect(str) {
	const nums = str.trim().split(/[ ,]+/);
	if (nums.length !== 4 || nums.some((n) => isNaN(parseFloat(n)))) {
		throw new Error("Invalid rect: " + str);
	}
	return {
		left: parseFloat(nums[0]),
		top: parseFloat(nums[1]),
		width: parseFloat(nums[2]),
		height: parseFloat(nums[3]),
	};
}

function quoteIMPD(value) {
	return /^[A-Za-z0-9_-]+$/.test(value) ? value : `[${value.replace(/]/g, "\\]")}]`;
}

function checkRequiredAttributes(attribs, ...names) {
	for (const name of names) {
		if (!(name in attribs)) {
			throw new Error("Missing required attribute: " + name);
		}
	}
}

const KNOWN_PRESENTATION_ATTRIBUTES = new Set([
	"stroke",
	"stroke-width",
	"stroke-linejoin",
	"stroke-linecap",
	"stroke-miterlimit",
	"stroke-dasharray",
	"stroke-dashoffset",
	"fill",
	"opacity",
	"fill-opacity",
	"stroke-opacity",
	"fill-rule",
]);

function clamp(value, min, max) {
	return Math.max(min, Math.min(max, value));
}

function rgbToHex(r, g, b) {
	const toHex = (n) => clamp(Math.round(n), 0, 255).toString(16).padStart(2, "0");
	return "#" + toHex(r) + toHex(g) + toHex(b);
}

function hslToRgb(h, s, l) {
	h = (((h % 360) + 360) % 360) / 360;
	s = clamp(s, 0, 1);
	l = clamp(l, 0, 1);
	const c = (1 - Math.abs(2 * l - 1)) * s;
	const x = c * (1 - Math.abs(((h * 6) % 2) - 1));
	const m = l - c / 2;
	let r, g, b;
	if (h < 1 / 6) {
		r = c;
		g = x;
		b = 0;
	} else if (h < 2 / 6) {
		r = x;
		g = c;
		b = 0;
	} else if (h < 3 / 6) {
		r = 0;
		g = c;
		b = x;
	} else if (h < 4 / 6) {
		r = 0;
		g = x;
		b = c;
	} else if (h < 5 / 6) {
		r = x;
		g = 0;
		b = c;
	} else {
		r = c;
		g = 0;
		b = x;
	}
	return { r: (r + m) * 255, g: (g + m) * 255, b: (b + m) * 255 };
}

function hsvToRgb(h, s, v) {
	h = (((h % 360) + 360) % 360) / 60;
	s = clamp(s, 0, 1);
	v = clamp(v, 0, 1);
	const c = v * s;
	const x = c * (1 - Math.abs((h % 2) - 1));
	const m = v - c;
	let r, g, b;
	if (h < 1) {
		r = c;
		g = x;
		b = 0;
	} else if (h < 2) {
		r = x;
		g = c;
		b = 0;
	} else if (h < 3) {
		r = 0;
		g = c;
		b = x;
	} else if (h < 4) {
		r = 0;
		g = x;
		b = c;
	} else if (h < 5) {
		r = x;
		g = 0;
		b = c;
	} else {
		r = c;
		g = 0;
		b = x;
	}
	return { r: (r + m) * 255, g: (g + m) * 255, b: (b + m) * 255 };
}

function parseRgbComponent(str) {
	const percentage = str.trim().endsWith("%");
	let num = parseFloat(str);
	if (percentage) {
		num = (255 * num) / 100;
	} else if (num <= 1) {
		num *= 255;
	}
	return num;
}

function parseAlpha(str) {
	if (str.trim().endsWith("%")) {
		return parseFloat(str) / 100;
	}
	const num = parseFloat(str);
	return num > 1 ? num / 255 : num;
}

function formatFloat(num) {
	return parseFloat(num.toFixed(6)).toString();
}

const SVG_COLORS = {
	aliceblue: "#f0f8ff",
	antiquewhite: "#faebd7",
	aquamarine: "#7fffd4",
	azure: "#f0ffff",
	beige: "#f5f5dc",
	bisque: "#ffe4c4",
	blanchedalmond: "#ffebcd",
	blueviolet: "#8a2be2",
	brown: "#a52a2a",
	burlywood: "#deb887",
	cadetblue: "#5f9ea0",
	chartreuse: "#7fff00",
	chocolate: "#d2691e",
	coral: "#ff7f50",
	cornflowerblue: "#6495ed",
	cornsilk: "#fff8dc",
	crimson: "#dc143c",
	cyan: "#00ffff",
	darkblue: "#00008b",
	darkcyan: "#008b8b",
	darkgoldenrod: "#b8860b",
	darkgray: "#a9a9a9",
	darkgreen: "#006400",
	darkgrey: "#a9a9a9",
	darkkhaki: "#bdb76b",
	darkmagenta: "#8b008b",
	darkolivegreen: "#556b2f",
	darkorange: "#ff8c00",
	darkorchid: "#9932cc",
	darkred: "#8b0000",
	darksalmon: "#e9967a",
	darkseagreen: "#8fbc8f",
	darkslateblue: "#483d8b",
	darkslategray: "#2f4f4f",
	darkslategrey: "#2f4f4f",
	darkturquoise: "#00ced1",
	darkviolet: "#9400d3",
	deeppink: "#ff1493",
	deepskyblue: "#00bfff",
	dimgray: "#696969",
	dimgrey: "#696969",
	dodgerblue: "#1e90ff",
	firebrick: "#b22222",
	floralwhite: "#fffaf0",
	forestgreen: "#228b22",
	gainsboro: "#dcdcdc",
	ghostwhite: "#f8f8ff",
	gold: "#ffd700",
	goldenrod: "#daa520",
	grey: "#808080",
	greenyellow: "#adff2f",
	honeydew: "#f0fff0",
	hotpink: "#ff69b4",
	indianred: "#cd5c5c",
	indigo: "#4b0082",
	ivory: "#fffff0",
	khaki: "#f0e68c",
	lavender: "#e6e6fa",
	lavenderblush: "#fff0f5",
	lawngreen: "#7cfc00",
	lemonchiffon: "#fffacd",
	lightblue: "#add8e6",
	lightcoral: "#f08080",
	lightcyan: "#e0ffff",
	lightgoldenrodyellow: "#fafad2",
	lightgray: "#d3d3d3",
	lightgreen: "#90ee90",
	lightgrey: "#d3d3d3",
	lightpink: "#ffb6c1",
	lightsalmon: "#ffa07a",
	lightseagreen: "#20b2aa",
	lightskyblue: "#87cefa",
	lightslategray: "#778899",
	lightslategrey: "#778899",
	lightsteelblue: "#b0c4de",
	lightyellow: "#ffffe0",
	limegreen: "#32cd32",
	linen: "#faf0e6",
	magenta: "#ff00ff",
	mediumaquamarine: "#66cdaa",
	mediumblue: "#0000cd",
	mediumorchid: "#ba55d3",
	mediumpurple: "#9370db",
	mediumseagreen: "#3cb371",
	mediumslateblue: "#7b68ee",
	mediumspringgreen: "#00fa9a",
	mediumturquoise: "#48d1cc",
	mediumvioletred: "#c71585",
	midnightblue: "#191970",
	mintcream: "#f5fffa",
	mistyrose: "#ffe4e1",
	moccasin: "#ffe4b5",
	navajowhite: "#ffdead",
	oldlace: "#fdf5e6",
	olivedrab: "#6b8e23",
	orange: "#ffa500",
	orangered: "#ff4500",
	orchid: "#da70d6",
	palegoldenrod: "#eee8aa",
	palegreen: "#98fb98",
	paleturquoise: "#afeeee",
	palevioletred: "#db7093",
	papayawhip: "#ffefd5",
	peachpuff: "#ffdab9",
	peru: "#cd853f",
	pink: "#ffc0cb",
	plum: "#dda0dd",
	powderblue: "#b0e0e6",
	rosybrown: "#bc8f8f",
	royalblue: "#4169e1",
	saddlebrown: "#8b4513",
	salmon: "#fa8072",
	sandybrown: "#f4a460",
	seagreen: "#2e8b57",
	seashell: "#fff5ee",
	sienna: "#a0522d",
	skyblue: "#87ceeb",
	slateblue: "#6a5acd",
	slategray: "#708090",
	slategrey: "#708090",
	snow: "#fffafa",
	springgreen: "#00ff7f",
	steelblue: "#4682b4",
	tan: "#d2b48c",
	thistle: "#d8bfd8",
	tomato: "#ff6347",
	turquoise: "#40e0d0",
	violet: "#ee82ee",
	wheat: "#f5deb3",
	whitesmoke: "#f5f5f5",
	yellowgreen: "#9acd32",
};

const BASIC_COLORS = {
	aqua: "#00ffff",
	black: "#000000",
	blue: "#0000ff",
	fuchsia: "#ff00ff",
	gray: "#808080",
	green: "#008000",
	lime: "#00ff00",
	maroon: "#800000",
	navy: "#000080",
	olive: "#808000",
	purple: "#800080",
	red: "#ff0000",
	silver: "#c0c0c0",
	teal: "#008080",
	white: "#ffffff",
	yellow: "#ffff00",
};
let gradients = new Map();
let patterns = new Map();
let cssRules = [];
let cssRuleIndex = 0;
let elementStack = [];
let styleStack = [{}];

const INHERITED_PROPERTIES = new Set([...KNOWN_PRESENTATION_ATTRIBUTES]);
INHERITED_PROPERTIES.delete("opacity");
INHERITED_PROPERTIES.add("color");

function convertPaint(sourcePaint) {
	sourcePaint = sourcePaint.trim().toLowerCase();
	if (sourcePaint === "currentcolor" || sourcePaint === "inherit") {
		return { paint: currentColor, opacity: 1 };
	}
	if (sourcePaint.startsWith("url(")) {
		let id = sourcePaint.slice(4, -1).trim();
		if (id.startsWith("#")) id = id.slice(1);
		if (gradients.has(id)) {
			return { paint: buildGradient(gradients.get(id)), opacity: 1 };
		}
		if (patterns.has(id)) {
			return { paint: buildPattern(patterns.get(id)), opacity: 1 };
		}
		throw new Error("Unrecognized paint reference: " + sourcePaint);
	}
	if (sourcePaint === "none") {
		return { paint: "none", opacity: 1 };
	}
	if (/^#[0-9a-f]{6}$/.test(sourcePaint)) {
		return { paint: sourcePaint, opacity: 1 };
	}
	let m = sourcePaint.match(/^#([0-9a-f])([0-9a-f])([0-9a-f])$/i);
	if (m) {
		return { paint: `#${m[1]}${m[1]}${m[2]}${m[2]}${m[3]}${m[3]}`, opacity: 1 };
	}
	m = sourcePaint.match(/^#([0-9a-f]{8})$/);
	if (m) {
		const r = parseInt(m[1].slice(0, 2), 16);
		const g = parseInt(m[1].slice(2, 4), 16);
		const b = parseInt(m[1].slice(4, 6), 16);
		const a = parseInt(m[1].slice(6, 8), 16) / 255;
		return { paint: rgbToHex(r, g, b), opacity: a };
	}
	m = sourcePaint.match(/^#([0-9a-f])([0-9a-f])([0-9a-f])([0-9a-f])$/i);
	if (m) {
		const r = parseInt(m[1] + m[1], 16);
		const g = parseInt(m[2] + m[2], 16);
		const b = parseInt(m[3] + m[3], 16);
		const a = parseInt(m[4] + m[4], 16) / 255;
		return { paint: rgbToHex(r, g, b), opacity: a };
	}
	m = sourcePaint.match(/^rgba\(([^)]+)\)$/);
	if (m) {
		const nums = m[1].split(/[ ,]+/);
		if (nums.length !== 4) throw new Error("Invalid rgba color: " + sourcePaint);
		const r = +(parseRgbComponent(nums[0]) / 255).toFixed(6);
		const g = +(parseRgbComponent(nums[1]) / 255).toFixed(6);
		const b = +(parseRgbComponent(nums[2]) / 255).toFixed(6);
		return { paint: `rgb(${r},${g},${b})`, opacity: parseAlpha(nums[3]) };
	}
	m = sourcePaint.match(/^rgb\(([^)]+)\)$/);
	if (m) {
		const nums = m[1].split(/[ ,]+/);
		if (nums.length !== 3) throw new Error("Invalid rgb color: " + sourcePaint);
		const r = +(parseRgbComponent(nums[0]) / 255).toFixed(6);
		const g = +(parseRgbComponent(nums[1]) / 255).toFixed(6);
		const b = +(parseRgbComponent(nums[2]) / 255).toFixed(6);
		return { paint: `rgb(${r},${g},${b})`, opacity: 1 };
	}
	m = sourcePaint.match(/^hsla\(([^)]+)\)$/);
	if (m) {
		const nums = m[1].split(/[ ,]+/);
		if (nums.length !== 4) throw new Error("Invalid hsla color: " + sourcePaint);
		const rgb = hslToRgb(parseFloat(nums[0]), parseFloat(nums[1]) / 100, parseFloat(nums[2]) / 100);
		const r = +(rgb.r / 255).toFixed(6);
		const g = +(rgb.g / 255).toFixed(6);
		const b = +(rgb.b / 255).toFixed(6);
		return { paint: `rgb(${r},${g},${b})`, opacity: parseAlpha(nums[3]) };
	}
	m = sourcePaint.match(/^hsl\(([^)]+)\)$/);
	if (m) {
		const nums = m[1].split(/[ ,]+/);
		if (nums.length !== 3) throw new Error("Invalid hsl color: " + sourcePaint);
		const rgb = hslToRgb(parseFloat(nums[0]), parseFloat(nums[1]) / 100, parseFloat(nums[2]) / 100);
		const r = +(rgb.r / 255).toFixed(6);
		const g = +(rgb.g / 255).toFixed(6);
		const b = +(rgb.b / 255).toFixed(6);
		return { paint: `rgb(${r},${g},${b})`, opacity: 1 };
	}
	m = sourcePaint.match(/^hsva\(([^)]+)\)$/);
	if (m) {
		const nums = m[1].split(/[ ,]+/);
		if (nums.length !== 4) throw new Error("Invalid hsva color: " + sourcePaint);
		const h = +(parseFloat(nums[0]) / 360).toFixed(6);
		const s = +(parseFloat(nums[1]) / 100).toFixed(6);
		const v = +(parseFloat(nums[2]) / 100).toFixed(6);
		return { paint: `hsv(${h},${s},${v})`, opacity: parseAlpha(nums[3]) };
	}
	m = sourcePaint.match(/^hsv\(([^)]+)\)$/);
	if (m) {
		const nums = m[1].split(/[ ,]+/);
		if (nums.length !== 3) throw new Error("Invalid hsv color: " + sourcePaint);
		const h = +(parseFloat(nums[0]) / 360).toFixed(6);
		const s = +(parseFloat(nums[1]) / 100).toFixed(6);
		const v = +(parseFloat(nums[2]) / 100).toFixed(6);
		return { paint: `hsv(${h},${s},${v})`, opacity: 1 };
	}
	if (sourcePaint in BASIC_COLORS) {
		return { paint: sourcePaint, opacity: 1 };
	}
	if (sourcePaint in SVG_COLORS) {
		return { paint: SVG_COLORS[sourcePaint], opacity: 1 };
	}
	throw new Error("Unrecognized color: " + sourcePaint);
}

let definitions = {};
let viewportWidth = 100;
let viewportHeight = 100;
let defaultWidth = 800;
let defaultHeight = 800;
let defaultFontFamily = "serif";
let defaultFontSize = 16;
let rootFontSize = 16;
let currentColor = "black";
let firstSVG = true;

function resetState() {
	outputString = "";
	indent = 0;
	gradients = new Map();
	patterns = new Map();
	cssRules = [];
	cssRuleIndex = 0;
	elementStack = [];
	styleStack = [{}];
	definitions = {};
	viewportWidth = 100;
	viewportHeight = 100;
	defaultWidth = 800;
	defaultHeight = 800;
	defaultFontFamily = "serif";
	defaultFontSize = 16;
	rootFontSize = 16;
	currentColor = "black";
	firstSVG = true;
}

function convertUnits(value, axis) {
	const str = value.trim().toLowerCase();
	const match = str.match(/^([+-]?\d*\.?\d+)([a-z%]*)$/);
	if (!match) {
		throw new Error("Invalid unit: " + value);
	}
	const num = parseFloat(match[1]);
	switch (match[2]) {
		case "":
		case "px":
			return num;
		case "cm":
			return (num * 96) / 2.54;
		case "mm":
			return (num * 96) / 25.4;
		case "in":
			return num * 96;
		case "pt":
			return (num * 96) / 72;
		case "pc":
			return num * 16;
		case "em":
			return num * defaultFontSize;
		case "ex":
			return num * (defaultFontSize / 2);
		case "rem":
			return num * rootFontSize;
		case "vh":
			return ((viewportHeight || 100) * num) / 100;
		case "vw":
			return ((viewportWidth || 100) * num) / 100;
		case "vmin":
			return ((Math.min(viewportWidth, viewportHeight) || 100) * num) / 100;
		case "vmax":
			return ((Math.max(viewportWidth, viewportHeight) || 100) * num) / 100;
		case "%":
			if (axis === "y") {
				return ((viewportHeight || 100) * num) / 100;
			}
			return ((viewportWidth || 100) * num) / 100;
		default:
			warning("Unsupported unit: " + match[2]);
			return num;
	}
}

function convertOpacity(value) {
	const percentage = value.trim().endsWith("%");
	let num = parseFloat(value);
	if (percentage) {
		num /= 100;
	}
	if (num < 0 || num > 1) {
		throw new Error("Invalid opacity: " + num);
	}
	return num;
}

function parseDashArray(value) {
	value = value.trim();
	if (value === "" || value === "none") {
		return "none";
	}
	const parts = value.split(/[ ,]+/).filter((p) => p.length);
	if (parts.length === 0) {
		return "none";
	}
	const nums = parts.map((p) => formatFloat(convertUnits(p, "x")));
	if (nums.length > 2) {
		warning("Too many dash values; using first two");
	}
	return nums.slice(0, 2).join(",");
}

function parsePoints(str) {
	const nums = str.trim().split(/[ ,]+/);
	if (nums.length < 2 || nums.length % 2 !== 0) {
		throw new Error("Invalid points: " + str);
	}
	const points = [];
	for (let i = 0; i < nums.length; i += 2) {
		points.push([convertUnits(nums[i], "x"), convertUnits(nums[i + 1], "y")]);
	}
	return points;
}

function outputMarker(ref, x, y, angle, kind) {
	const id = parseUrlRef(ref);
	if (!id || !(id in definitions)) {
		warning("Unrecognized marker reference: " + ref);
		return;
	}
	const marker = definitions[id];
	const attrs = marker.attributes || {};
	const refX = convertUnits(attrs.refX || "0", "x");
	const refY = convertUnits(attrs.refY || "0", "y");
	const width = convertUnits(attrs.markerWidth || "3", "x");
	const height = convertUnits(attrs.markerHeight || "3", "y");
	let scaleX = 1;
	let scaleY = 1;
      if (attrs.viewBox) {
	      const vb = parseRect(attrs.viewBox);
	      scaleX = width / vb.width;
	      scaleY = height / vb.height;
      }
      let rotateAngle = NaN;
      const orient = attrs.orient || "0";
      if (orient === "auto" || orient === "auto-start-reverse") {
	      rotateAngle = angle;
	      if (orient === "auto-start-reverse" && kind === "start") rotateAngle += 180;
      } else {
	      rotateAngle = parseFloat(orient);
      }
	output("context [");
	output("pen none");
	output(`offset ${x},${y}`);
      if (!isNaN(rotateAngle) && rotateAngle !== 0) {
	      output(`rotate ${formatFloat(rotateAngle)}`);
      }
      if (scaleX !== 1 || scaleY !== 1) {
	      output(`scale ${scaleX},${scaleY}`);
      }
      if (refX || refY) {
	      output(`offset -${refX},-${refY}`);
      }
      for (const item of marker.contents || []) {
	      if (item.element) convertSVGElement(item.element);
      }
      output("]");
}

function processMarkers(attribs, pts) {
	const getAngle = (p1, p2) => (Math.atan2(p2[1] - p1[1], p2[0] - p1[0]) * 180) / Math.PI;
	if ("marker-start" in attribs && pts.length >= 2) {
	       outputMarker(attribs["marker-start"], pts[0][0], pts[0][1], getAngle(pts[0], pts[1]), "start");
	}
	if ("marker-mid" in attribs && pts.length >= 3) {
	       for (let i = 1; i < pts.length - 1; i++) {
	               outputMarker(attribs["marker-mid"], pts[i][0], pts[i][1], getAngle(pts[i - 1], pts[i + 1]), "mid");
	       }
	}
	if ("marker-end" in attribs && pts.length >= 2) {
	       const n = pts.length - 1;
	       outputMarker(attribs["marker-end"], pts[n][0], pts[n][1], getAngle(pts[n - 1], pts[n]), "end");
	}
}

function parseGradientCoord(value, axis) {
	value = value.trim();
	if (value.endsWith("%")) {
		return parseFloat(value) / 100;
	}
	return convertUnits(value, axis);
}

function parseGradientStops(element) {
	const stops = [];
	for (const item of element.contents || []) {
	       if (item.element && item.element.type === "stop") {
	                const a = item.element.attributes;
	                expandStyle(a);
	                const cs = resolveStyles(item.element, a, styleStack[styleStack.length - 1]);
	                Object.assign(a, cs);
	                if (!("offset" in a) || !("stop-color" in a)) continue;
			let o = a.offset.trim();
			o = o.endsWith("%") ? parseFloat(o) / 100 : parseFloat(o);
			const p = convertPaint(a["stop-color"]);
			let op = p.opacity;
			if ("stop-opacity" in a) {
				op *= convertOpacity(a["stop-opacity"]);
			}
			let color = p.paint;
			if (op !== 1) {
				if (color.startsWith("#")) {
					const alpha = Math.round(op * 255)
						.toString(16)
						.padStart(2, "0");
					color += alpha;
				} else if (color.startsWith("rgb(") || color.startsWith("hsv(")) {
					color = color.replace(/\)$/u, `,${formatFloat(op)})`);
				} else if (color in BASIC_COLORS || color in SVG_COLORS) {
					const hex = BASIC_COLORS[color] || SVG_COLORS[color];
					const r = +(parseInt(hex.slice(1, 3), 16) / 255).toFixed(6);
					const g = +(parseInt(hex.slice(3, 5), 16) / 255).toFixed(6);
					const b = +(parseInt(hex.slice(5, 7), 16) / 255).toFixed(6);
					color = `rgb(${r},${g},${b},${formatFloat(op)})`;
				} else {
					const alpha = Math.round(op * 255)
						.toString(16)
						.padStart(2, "0");
					color += alpha;
				}
			}
			stops.push({ offset: o, color });
		}
	}
	return stops;
}

function buildGradient(g) {
	let s;
	if (g.type === "linear") {
		s = `gradient:[linear ${g.x1},${g.y1},${g.x2},${g.y2}`;
	} else {
		s = `gradient:[radial ${g.cx},${g.cy},${g.r}`;
	}
	if (g.stops.length === 2 && g.stops[0].offset === 0 && g.stops[1].offset === 1) {
		s += ` from:${g.stops[0].color} to:${g.stops[1].color}`;
	} else if (g.stops.length) {
		s += " stops:[" + g.stops.map((st) => `${st.offset},${st.color}`).join(",") + "]";
	}
	s += "]";
	if (g.transform) {
		s += ` transform:[${transformToCommandString(g.transform)}]`;
	}
	if (g.relative) s += " relative:yes";
	return s;
}

function buildPattern(p) {
	// Patterns require integer bounds too; ceil fractional sizes and warn
	const pw = Math.ceil(p.width);
	const ph = Math.ceil(p.height);
	if (pw !== p.width || ph !== p.height) {
		warning(`Non-integer pattern bounds (${formatFloat(p.width)},${formatFloat(p.height)}); using ceil (${pw},${ph}).`);
	}
	let s = `pattern:[bounds 0,0,${pw},${ph}`;
	if (p.body) {
		s += "; " + p.body;
	}
	s += "]";
	const transforms = [];
	if (p.x !== 0 || p.y !== 0) {
		transforms.push(`offset ${p.x},${p.y}`);
	}
	if (p.transform) {
		transforms.push(transformToCommandString(p.transform));
	}
	if (transforms.length) {
		s += ` transform:[${transforms.join("; ")}]`;
	}
	if (p.relative) s += " relative:yes";
	return s;
}

function transformToCommands(str) {
	const re = /(translate|scale|rotate|skewX|skewY|matrix)\(([^)]*)\)/g;
	const cmds = [];
	let m;
	while ((m = re.exec(str)) !== null) {
		const type = m[1];
		const params = m[2]
			.trim()
			.split(/[ ,]+/)
			.filter((p) => p.length);
		switch (type) {
			case "translate": {
				const tx = convertUnits(params[0] || "0", "x");
				const ty = params.length > 1 ? convertUnits(params[1], "y") : 0;
				cmds.push(`offset ${tx},${ty}`);
				break;
			}
			case "scale": {
				const sx = parseFloat(params[0] || "1");
				const sy = params.length > 1 ? parseFloat(params[1]) : sx;
				if (params.length > 1) {
					cmds.push(`scale ${sx},${sy}`);
				} else {
					cmds.push(`scale ${sx}`);
				}
				break;
			}
			case "rotate": {
				const angle = parseFloat(params[0] || "0");
				let cmd = `rotate ${angle}`;
				if (params.length > 2) {
					const cx = convertUnits(params[1], "x");
					const cy = convertUnits(params[2], "y");
					cmd += ` anchor:${cx},${cy}`;
				}
				cmds.push(cmd);
				break;
			}
			case "skewX": {
				const angle = parseFloat(params[0] || "0");
				const sx = Math.tan((angle * Math.PI) / 180);
				cmds.push(`shear ${sx},0`);
				break;
			}
			case "skewY": {
				const angle = parseFloat(params[0] || "0");
				const sy = Math.tan((angle * Math.PI) / 180);
				cmds.push(`shear 0,${sy}`);
				break;
			}
			case "matrix": {
				if (params.length !== 6) {
					warning("matrix requires 6 parameters");
					break;
				}
				const a = parseFloat(params[0]);
				const b = parseFloat(params[1]);
				const c = parseFloat(params[2]);
				const d = parseFloat(params[3]);
				const e = convertUnits(params[4], "x");
				const f = convertUnits(params[5], "y");
				cmds.push(`matrix ${a},${b},${c},${d},${e},${f}`);
				break;
			}
			default:
				warning("Unsupported transform: " + type);
		}
	}
	return cmds;
}

function transformToCommandString(str) {
	return transformToCommands(str).join("; ");
}

function outputTransforms(str) {
	for (const cmd of transformToCommands(str)) {
		output(cmd);
	}
}

const LINEJOINS_TO_JOINTS = {
	bevel: "bevel",
	round: "curve",
	miter: "miter",
	"miter-clip": "miter",
};

const SUPPORTED_LINECAPS = new Set(["butt", "round", "square"]);

function outputPresentationAttributes(attribs) {
	const hasStroke = "stroke" in attribs && attribs.stroke.trim() !== "inherit";
	const hasStrokeWidth = "stroke-width" in attribs;
	const hasStrokeLineJoin = "stroke-linejoin" in attribs;
	const hasStrokeLineCap = "stroke-linecap" in attribs;
	const hasStrokeMiterlimit = "stroke-miterlimit" in attribs;
	const hasStrokeDasharray = "stroke-dasharray" in attribs;
	const hasStrokeDashoffset = "stroke-dashoffset" in attribs;
	let baseOpacity = 1.0;
	if ("opacity" in attribs) {
		baseOpacity = convertOpacity(attribs.opacity);
	}
	let strokeOpacity = baseOpacity;
	let fillOpacity = baseOpacity;
	let strokePaint = null;
	if (hasStroke) {
		const s = attribs.stroke.trim();
		if (s !== "inherit") {
			if (s.startsWith("gradient:[") || /^[-+]?\d*\.?\d+(e[-+]?\d+)?$/i.test(s)) {
				strokePaint = { paint: s, opacity: 1 };
			} else {
				strokePaint = convertPaint(s);
				strokeOpacity *= strokePaint.opacity;
			}
		}
	}
	if ("stroke-opacity" in attribs) {
		strokeOpacity *= convertOpacity(attribs["stroke-opacity"]);
	}
	let fillPaint = null;
	if ("fill" in attribs) {
		const f = attribs.fill.trim();
		if (f !== "inherit") {
			if (f.startsWith("gradient:[") || /^[-+]?\d*\.?\d+(e[-+]?\d+)?$/i.test(f)) {
				fillPaint = { paint: f, opacity: 1 };
			} else {
				fillPaint = convertPaint(f);
				fillOpacity *= fillPaint.opacity;
			}
		}
	}
	if ("fill-opacity" in attribs) {
		fillOpacity *= convertOpacity(attribs["fill-opacity"]);
	}
	let fillRule = null;
	if ("fill-rule" in attribs) {
		const fr = attribs["fill-rule"].trim().toLowerCase();
		if (fr === "evenodd") {
			fillRule = "even-odd";
		} else if (fr !== "nonzero") {
			throw new Error("Unrecognized fill-rule: " + attribs["fill-rule"]);
		}
	}
	const hasStrokeAttrs =
		hasStroke ||
		hasStrokeWidth ||
		hasStrokeLineJoin ||
		hasStrokeLineCap ||
		hasStrokeMiterlimit ||
		hasStrokeDasharray ||
		hasStrokeDashoffset;
	if (hasStrokeAttrs) {
		let s = "pen";
		if (strokePaint) {
			s += " " + strokePaint.paint;
		}
		if (hasStrokeWidth) {
			s += " width:" + convertUnits(attribs["stroke-width"], "x");
		}
		if (hasStrokeLineJoin) {
			const lj = attribs["stroke-linejoin"];
			if (!(lj in LINEJOINS_TO_JOINTS)) {
				throw new Error("Unrecognized stroke-linejoin: " + lj);
			}
			s += " joints:" + LINEJOINS_TO_JOINTS[lj];
		}
		if (hasStrokeLineCap) {
			const lc = attribs["stroke-linecap"];
			if (!SUPPORTED_LINECAPS.has(lc)) {
				throw new Error("Unrecognized stroke-linecap: " + lc);
			}
			s += " caps:" + lc;
		}
		if (hasStrokeMiterlimit) {
			s += " miter-limit:" + attribs["stroke-miterlimit"];
		}
		if (hasStrokeDasharray) {
			const dash = parseDashArray(attribs["stroke-dasharray"]);
			s += " dash:" + dash;
		}
		if (hasStrokeDashoffset) {
			s += " dash-offset:" + convertUnits(attribs["stroke-dashoffset"], "x");
		}
		if (strokeOpacity !== 1) {
			s += " opacity:" + strokeOpacity;
		}
		output(s);
	}
	if (fillPaint || fillOpacity !== 1 || fillRule) {
		let s = "fill";
		if (fillPaint) {
			s += " " + fillPaint.paint;
		}
		if (fillRule) {
			s += " rule:" + fillRule;
		}
		if (fillOpacity !== 1) {
			s += " opacity:" + fillOpacity;
		}
		output(s);
	}
}

function gotKnownPresentationAttributes(attribs) {
	for (const attr of KNOWN_PRESENTATION_ATTRIBUTES) {
		if (attr in attribs) return true;
	}
	return false;
}

function expandStyle(attribs) {
	if (!("style" in attribs)) return;
	const styles = {};
	const decls = attribs.style.split(";");
	for (const decl of decls) {
	        if (!decl.trim()) continue;
	        const parts = decl.split(":");
	        if (parts.length < 2) continue;
	        const name = parts.shift().trim().toLowerCase();
	        let value = parts.join(":").trim();
	        let important = false;
	        if (/!important$/i.test(value)) {
	                important = true;
	                value = value.replace(/!important$/i, "").trim();
	        }
	        if (name) styles[name] = { value, important };
	}
	attribs.__style = styles;
	delete attribs.style;
}

function parseStyleSheet(text) {
	text = text.replace(/\/\*[^]*?\*\//g, "");
	for (const block of text.split("}")) {
	        const parts = block.split("{");
	        if (parts.length < 2) continue;
	        const selectors = parts[0]
	                .split(",")
	                .map((s) => s.trim())
	                .filter(Boolean);
	        const decls = [];
	        for (const decl of parts[1].split(";")) {
	                if (!decl.trim()) continue;
	                const dparts = decl.split(":");
	                if (dparts.length < 2) continue;
	                const name = dparts.shift().trim().toLowerCase();
	                let value = dparts.join(":").trim();
	                let important = false;
	                if (/!important$/i.test(value)) {
	                        important = true;
	                        value = value.replace(/!important$/i, "").trim();
	                }
	                if (name) decls.push({ name, value, important });
	        }
	        for (const sel of selectors) {
	                const tokens = parseSelector(sel);
	                cssRules.push({
	                        tokens,
	                        specificity: calcSpecificity(tokens),
	                        order: cssRuleIndex++,
	                        declarations: decls,
	                });
	        }
	}
}


function parseUrlRef(value) {
	const m = /^url\(#([^\)]+)\)$/.exec(value.trim());
	return m ? m[1] : null;
}

function outputClipPath(ref, bbox) {
	const id = parseUrlRef(ref);
	if (!id || !(id in definitions)) {
		warning("Unrecognized clip-path reference: " + ref);
		return;
	}
	const clip = definitions[id];
	const units = clip.attributes.clipPathUnits || "userSpaceOnUse";
	output("mask [");
	const needsContext = units === "objectBoundingBox" || "transform" in clip.attributes;
	if (needsContext) {
		output("context [");
		if (units === "objectBoundingBox") {
			if (bbox) {
				output(`offset ${bbox.left},${bbox.top}`);
				output(`scale ${bbox.width},${bbox.height}`);
			} else {
				warning('clipPathUnits="objectBoundingBox" requires known bounds');
			}
		}
		if ("transform" in clip.attributes) {
			outputTransforms(clip.attributes.transform);
		}
	}
	for (const item of clip.contents || []) {
		if (!item.element) continue;
		const child = JSON.parse(JSON.stringify(item.element));
		child.attributes = child.attributes || {};
		child.attributes.fill = "1";
		child.attributes.stroke = "0";
		delete child.attributes["clip-path"];
		convertSVGElement(child);
	}
	if (needsContext) output("]");
	output("]");
}

function colorToMask(color) {
	color = color.toLowerCase();
	if (color in BASIC_COLORS) color = BASIC_COLORS[color];
	if (color in SVG_COLORS) color = SVG_COLORS[color];
	let r = 0,
		g = 0,
		b = 0;
	if (color.startsWith("#")) {
		if (color.length === 7) {
			r = parseInt(color.slice(1, 3), 16) / 255;
			g = parseInt(color.slice(3, 5), 16) / 255;
			b = parseInt(color.slice(5, 7), 16) / 255;
		} else if (color.length === 9) {
			r = parseInt(color.slice(1, 3), 16) / 255;
			g = parseInt(color.slice(3, 5), 16) / 255;
			b = parseInt(color.slice(5, 7), 16) / 255;
		}
	} else if (color.startsWith("rgb(")) {
		const nums = color.slice(4, -1).split(",").map(parseFloat);
		r = nums[0];
		g = nums[1];
		b = nums[2];
	} else {
		throw new Error("Unsupported mask color: " + color);
	}
	const lum = 0.2126 * r + 0.7152 * g + 0.0722 * b;
	return formatFloat(lum);
}

function buildGradientMask(g) {
	function extractAlpha(color) {
		if (color.startsWith("#") && color.length === 9) {
			return parseInt(color.slice(7, 9), 16) / 255;
		}
		if (color.startsWith("rgb(")) {
			const parts = color.slice(4, -1).split(",");
			if (parts.length === 4) return parseFloat(parts[3]);
		}
		return 1;
	}
	function extract(color) {
		return {
			lum: parseFloat(colorToMask(color)),
			alpha: extractAlpha(color),
		};
	}
	const src = g.stops.map((st) => ({ offset: st.offset, ...extract(st.color) }));
	const stops = [];
	function addStop(o, lum, alpha) {
		stops.push({ offset: o, color: formatFloat(lum * alpha) });
	}
	function subdivide(a, b, depth) {
		const v0 = a.lum * a.alpha;
		const v1 = b.lum * b.alpha;
		const mid = {
			offset: (a.offset + b.offset) / 2,
			lum: (a.lum + b.lum) / 2,
			alpha: (a.alpha + b.alpha) / 2,
		};
		const vMid = mid.lum * mid.alpha;
		const linearMid = (v0 + v1) / 2;
		if (depth < 6 && Math.abs(vMid - linearMid) > 0.02) {
			subdivide(a, mid, depth + 1);
			addStop(mid.offset, mid.lum, mid.alpha);
			subdivide(mid, b, depth + 1);
		}
	}
	addStop(src[0].offset, src[0].lum, src[0].alpha);
	for (let i = 0; i < src.length - 1; i++) {
		const a = src[i];
		const b = src[i + 1];
		subdivide(a, b, 0);
		addStop(b.offset, b.lum, b.alpha);
	}
	const clone = JSON.parse(JSON.stringify(g));
	clone.stops = stops;
	return buildGradient(clone);
}

function convertMaskPaint(sourcePaint) {
	sourcePaint = sourcePaint.trim();
	if (sourcePaint.startsWith("url(")) {
		const id = parseUrlRef(sourcePaint);
		if (id && gradients.has(id)) {
			return buildGradientMask(gradients.get(id));
		}
		throw new Error("Unrecognized mask paint reference: " + sourcePaint);
	}
	const p = convertPaint(sourcePaint);
	return formatFloat(parseFloat(colorToMask(p.paint)) * p.opacity);
}

function outputMask(ref, bbox) {
	const id = parseUrlRef(ref);
	if (!id || !(id in definitions)) {
		warning("Unrecognized mask reference: " + ref);
		return;
	}
	const mask = definitions[id];
	const units = mask.attributes.maskContentUnits || "userSpaceOnUse";
	output("mask [");
	const needsContext = units === "objectBoundingBox";
	if (needsContext) {
		if (bbox) {
			output("context [");
			output(`offset ${bbox.left},${bbox.top}`);
			output(`scale ${bbox.width},${bbox.height}`);
		} else {
			warning('maskContentUnits="objectBoundingBox" requires known bounds');
		}
	}
	if (mask.body) {
		for (const line of mask.body) {
			output(line);
		}
	} else {
		for (const item of mask.contents || []) {
			if (!item.element) continue;
			const child = JSON.parse(JSON.stringify(item.element));
			child.attributes = child.attributes || {};
			if (!("fill" in child.attributes)) {
				child.attributes.fill = "1";
			} else {
				child.attributes.fill = convertMaskPaint(child.attributes.fill);
			}
			if ("stroke" in child.attributes) {
				child.attributes.stroke = convertMaskPaint(child.attributes.stroke);
			} else {
				child.attributes.stroke = "0";
			}
			delete child.attributes.mask;
			delete child.attributes["clip-path"];
			convertSVGElement(child);
		}
	}
	if (needsContext) output("]");
	output("]");
}

function createContextMaybe(attribs, bbox) {
	const needs =
		gotKnownPresentationAttributes(attribs) ||
		"transform" in attribs ||
		"clip-path" in attribs ||
		"mask" in attribs;
	const hasColor = "color" in attribs;
	const oldColor = currentColor;
	if (hasColor) {
	       const c = attribs.color.trim().toLowerCase();
	       if (c !== "inherit" && c !== "currentcolor") {
	               currentColor = convertPaint(c).paint;
	       }
	}
	const hasFont = "font-size" in attribs || "font-family" in attribs;
	const oldFontSize = defaultFontSize;
	const oldFontFamily = defaultFontFamily;
	if ("font-size" in attribs) {
	       defaultFontSize = convertUnits(attribs["font-size"], "y");
	}
	if ("font-family" in attribs) {
	       defaultFontFamily = attribs["font-family"]
	               .split(",")[0]
	               .trim()
	               .replace(/^['"]|['"]$/g, "");
	}
	if (needs) {
		output("context [");
		if ("transform" in attribs) {
			outputTransforms(attribs.transform);
		}
		outputPresentationAttributes(attribs);
		if ("clip-path" in attribs) {
			outputClipPath(attribs["clip-path"], bbox);
		}
		if ("mask" in attribs) {
			outputMask(attribs.mask, bbox);
		}
	}
	return { needs, oldColor, hasColor, hasFont, oldFontSize, oldFontFamily };
}

function registerDefinition(element) {
	if (element.attributes && element.attributes.id) {
		definitions[element.attributes.id] = JSON.parse(JSON.stringify(element));
	}
}

const converters = {};

converters.marker = function () {};

converters.svg = function (element, attribs) {
	let width, height;
	if ("width" in attribs) {
		width = convertUnits(attribs.width, "x");
	} else {
		warning(`Missing 'width' attribute. Assuming a width of ${defaultWidth}.`);
		width = defaultWidth;
	}
	if ("height" in attribs) {
		height = convertUnits(attribs.height, "y");
	} else {
		warning(`Missing 'height' attribute. Assuming a height of ${defaultHeight}.`);
		height = defaultHeight;
	}
	viewportWidth = width;
	viewportHeight = height;
	const oldColor = currentColor;
	if ("color" in attribs) {
		const c = attribs.color.trim().toLowerCase();
		if (c !== "inherit" && c !== "currentcolor") {
			currentColor = convertPaint(c).paint;
		}
	}
    if (!firstSVG) {
        output("reset");
    }
    firstSVG = false;
    // Ensure integer bounds for IVG; ceil fractional sizes and warn
    const widthCeil = Math.ceil(width);
    const heightCeil = Math.ceil(height);
    if (widthCeil !== width || heightCeil !== height) {
        warning(`Non-integer bounds (${formatFloat(width)},${formatFloat(height)}); using ceil (${widthCeil},${heightCeil}).`);
    }
    output(`bounds 0,0,${widthCeil},${heightCeil}`);
    output("fill black");
    output("pen miter-limit:4");
	const oldFamily = defaultFontFamily;
	const oldSize = defaultFontSize;
	if ("font-family" in attribs) {
		defaultFontFamily = attribs["font-family"]
			.split(",")[0]
			.trim()
			.replace(/^['"]|['"]$/g, "");
	}
	if ("font-size" in attribs) {
		defaultFontSize = convertUnits(attribs["font-size"], "y");
	}
	if (!rootFontSize || rootFontSize === 16) {
		rootFontSize = defaultFontSize;
	}
	if ("transform" in attribs) {
		outputTransforms(attribs.transform);
	}
	if ("viewBox" in attribs) {
		const vb = parseRect(attribs.viewBox);
		if (vb.left !== 0 || vb.top !== 0) {
			output(`offset -${vb.left},-${vb.top}`);
		}
		output(`scale ${Math.min(width / vb.width, height / vb.height)}`);
	}
	convertSVGContainer(element);
	defaultFontFamily = oldFamily;
	defaultFontSize = oldSize;
	currentColor = oldColor;
};

converters.g = function (element, attribs) {
	const ctx = createContextMaybe(attribs);
	convertSVGContainer(element);
	if (ctx.needs) output("]");
	if (ctx.hasColor) currentColor = ctx.oldColor;
	if (ctx.hasFont) {
	       defaultFontSize = ctx.oldFontSize;
	       defaultFontFamily = ctx.oldFontFamily;
	}
};

converters.path = function (element, attribs) {
	const ctx = createContextMaybe(attribs);
	if ("d" in attribs) {
	       output("path svg:[" + attribs.d + "]");
	} else {
	       warning("Missing 'd' attribute in 'path' element.");
	}
	if (ctx.needs) output("]");
	if (ctx.hasColor) currentColor = ctx.oldColor;
	if (ctx.hasFont) {
	       defaultFontSize = ctx.oldFontSize;
	       defaultFontFamily = ctx.oldFontFamily;
	}
};

converters.circle = function (element, attribs) {
	checkRequiredAttributes(attribs, "cx", "cy", "r");
	const cx = convertUnits(attribs.cx, "x");
	const cy = convertUnits(attribs.cy, "y");
	const r = convertUnits(attribs.r, "x");
	const bbox = { left: cx - r, top: cy - r, width: r * 2, height: r * 2 };
	const ctx = createContextMaybe(attribs, bbox);
	output(`ellipse ${cx},${cy},${r}`);
	if (ctx.needs) output("]");
	if (ctx.hasColor) currentColor = ctx.oldColor;
	if (ctx.hasFont) {
	       defaultFontSize = ctx.oldFontSize;
	       defaultFontFamily = ctx.oldFontFamily;
	}
};

converters.ellipse = function (element, attribs) {
	checkRequiredAttributes(attribs, "cx", "cy", "rx", "ry");
	const cx = convertUnits(attribs.cx, "x");
	const cy = convertUnits(attribs.cy, "y");
	const rx = convertUnits(attribs.rx, "x");
	const ry = convertUnits(attribs.ry, "y");
	const bbox = { left: cx - rx, top: cy - ry, width: rx * 2, height: ry * 2 };
	const ctx = createContextMaybe(attribs, bbox);
	output(`ellipse ${cx},${cy},${rx},${ry}`);
	if (ctx.needs) output("]");
	if (ctx.hasColor) currentColor = ctx.oldColor;
	if (ctx.hasFont) {
	       defaultFontSize = ctx.oldFontSize;
	       defaultFontFamily = ctx.oldFontFamily;
	}
};

converters.line = function (element, attribs) {
	checkRequiredAttributes(attribs, "x1", "y1", "x2", "y2");
	const x1 = convertUnits(attribs.x1, "x");
	const y1 = convertUnits(attribs.y1, "y");
	const x2 = convertUnits(attribs.x2, "x");
	const y2 = convertUnits(attribs.y2, "y");
	const bbox = {
		left: Math.min(x1, x2),
		top: Math.min(y1, y2),
		width: Math.abs(x2 - x1),
		height: Math.abs(y2 - y1),
	};
	const ctx = createContextMaybe(attribs, bbox);
	output(`path svg:[M${x1},${y1}L${x2},${y2}]`);
	processMarkers(attribs, [
		[x1, y1],
		[x2, y2],
	]);
	if (ctx.needs) output("]");
	if (ctx.hasColor) currentColor = ctx.oldColor;
	if (ctx.hasFont) {
	       defaultFontSize = ctx.oldFontSize;
	       defaultFontFamily = ctx.oldFontFamily;
	}
};

converters.rect = function (element, attribs) {
	checkRequiredAttributes(attribs, "x", "y", "width", "height");
	const x = convertUnits(attribs.x, "x");
	const y = convertUnits(attribs.y, "y");
	const w = convertUnits(attribs.width, "x");
	const h = convertUnits(attribs.height, "y");
	const bbox = { left: x, top: y, width: w, height: h };
	const ctx = createContextMaybe(attribs, bbox);
	let s = `rect ${x},${y},${w},${h}`;
	const hasRX = "rx" in attribs;
	const hasRY = "ry" in attribs;
	if (hasRX && hasRY) {
		s += ` rounded:${convertUnits(attribs.rx, "x")},${convertUnits(attribs.ry, "y")}`;
	} else if (hasRX) {
		s += ` rounded:${convertUnits(attribs.rx, "x")}`;
	} else if (hasRY) {
		s += ` rounded:${convertUnits(attribs.ry, "y")}`;
	}
	output(s);
	processMarkers(attribs, [
		[x, y],
		[x + w, y + h],
	]);
	if (ctx.needs) output("]");
	if (ctx.hasColor) currentColor = ctx.oldColor;
	if (ctx.hasFont) {
	       defaultFontSize = ctx.oldFontSize;
	       defaultFontFamily = ctx.oldFontFamily;
	}
};

converters.polygon = function (element, attribs) {
	checkRequiredAttributes(attribs, "points");
	const pts = parsePoints(attribs.points);
	let bbox = null;
	if (pts.length >= 1) {
		let minX = pts[0][0];
		let minY = pts[0][1];
		let maxX = pts[0][0];
		let maxY = pts[0][1];
		for (let i = 1; i < pts.length; i++) {
			minX = Math.min(minX, pts[i][0]);
			minY = Math.min(minY, pts[i][1]);
			maxX = Math.max(maxX, pts[i][0]);
			maxY = Math.max(maxY, pts[i][1]);
		}
		bbox = { left: minX, top: minY, width: maxX - minX, height: maxY - minY };
	}
	const ctx = createContextMaybe(attribs, bbox);
	if (pts.length < 2) {
		warning("Not enough points in 'polygon'.");
	} else {
		let s = `M${pts[0][0]},${pts[0][1]}`;
		for (let i = 1; i < pts.length; i++) {
			s += `L${pts[i][0]},${pts[i][1]}`;
		}
		s += "Z";
		output("path svg:[" + s + "]");
	}
	processMarkers(attribs, pts);
	if (ctx.needs) output("]");
	if (ctx.hasColor) currentColor = ctx.oldColor;
	if (ctx.hasFont) {
	       defaultFontSize = ctx.oldFontSize;
	       defaultFontFamily = ctx.oldFontFamily;
	}
};

converters.polyline = function (element, attribs) {
	checkRequiredAttributes(attribs, "points");
	const pts = parsePoints(attribs.points);
	let bbox = null;
	if (pts.length >= 1) {
		let minX = pts[0][0];
		let minY = pts[0][1];
		let maxX = pts[0][0];
		let maxY = pts[0][1];
		for (let i = 1; i < pts.length; i++) {
			minX = Math.min(minX, pts[i][0]);
			minY = Math.min(minY, pts[i][1]);
			maxX = Math.max(maxX, pts[i][0]);
			maxY = Math.max(maxY, pts[i][1]);
		}
		bbox = { left: minX, top: minY, width: maxX - minX, height: maxY - minY };
	}
	const ctx = createContextMaybe(attribs, bbox);
	if (pts.length < 2) {
		warning("Not enough points in 'polyline'.");
	} else {
		let s = `M${pts[0][0]},${pts[0][1]}`;
		for (let i = 1; i < pts.length; i++) {
			s += `L${pts[i][0]},${pts[i][1]}`;
		}
		output("path svg:[" + s + "]");
	}
	processMarkers(attribs, pts);
	if (ctx.needs) output("]");
	if (ctx.hasColor) currentColor = ctx.oldColor;
	if (ctx.hasFont) {
	       defaultFontSize = ctx.oldFontSize;
	       defaultFontFamily = ctx.oldFontFamily;
	}
};

function applyTextAttributes(base, attribs) {
	const out = Object.assign({}, base);
	if ("font-family" in attribs) {
		out.fontName = attribs["font-family"]
			.split(",")[0]
			.trim()
			.replace(/^['"]|['"]$/g, "");
	}
	if ("font-size" in attribs) {
		out.size = convertUnits(attribs["font-size"], "y");
	}
	if ("fill" in attribs && attribs.fill.trim() !== "inherit") {
		const fillPaint = convertPaint(attribs.fill);
		out.fill = fillPaint.paint;
		out.fillOpacity = fillPaint.opacity;
	}
	if ("opacity" in attribs) {
		out.fillOpacity *= convertOpacity(attribs.opacity);
	}
	if ("fill-opacity" in attribs) {
		out.fillOpacity *= convertOpacity(attribs["fill-opacity"]);
	}
	if ("stroke" in attribs && attribs.stroke !== "none" && attribs.stroke.trim() !== "inherit") {
		const strokePaint = convertPaint(attribs.stroke);
		out.stroke = strokePaint.paint;
		out.strokeOpacity = strokePaint.opacity;
	}
	if ("stroke-opacity" in attribs && out.stroke) {
		out.strokeOpacity *= convertOpacity(attribs["stroke-opacity"]);
	}
	if ("stroke-width" in attribs && out.stroke) {
		out.strokeWidth = convertUnits(attribs["stroke-width"], "x");
	}
	if ("stroke-linejoin" in attribs && out.stroke) {
		const lj = attribs["stroke-linejoin"];
		if (!(lj in LINEJOINS_TO_JOINTS)) {
			throw new Error("Unrecognized stroke-linejoin: " + lj);
		}
		out.strokeJoin = LINEJOINS_TO_JOINTS[lj];
	}
	if ("stroke-linecap" in attribs && out.stroke) {
		const lc = attribs["stroke-linecap"];
		if (!SUPPORTED_LINECAPS.has(lc)) {
			throw new Error("Unrecognized stroke-linecap: " + lc);
		}
		out.strokeCap = lc;
	}
	if ("stroke-miterlimit" in attribs && out.stroke) {
		out.strokeMiter = parseFloat(attribs["stroke-miterlimit"]);
	}
	if ("stroke-dasharray" in attribs && out.stroke) {
		out.strokeDash = parseDashArray(attribs["stroke-dasharray"]);
	}
	if ("stroke-dashoffset" in attribs && out.stroke) {
		out.strokeDashOffset = convertUnits(attribs["stroke-dashoffset"], "x");
	}
	return out;
}

function collectTextSegments(element, baseAttribs) {
	const segments = [];
	const attribs = applyTextAttributes(baseAttribs, element.attributes || {});
	let prevEndsWithSpace = false;
	for (const item of element.contents || []) {
		if (item.text) {
			let text = item.text.replace(/\s+/g, " ");
			if (!text) continue;
			if (segments.length && !prevEndsWithSpace && !/^\s/.test(text)) {
				text = " " + text;
			}
			prevEndsWithSpace = /\s$/.test(text);
			segments.push({ text, attribs });
		} else if (item.element && item.element.type === "tspan") {
			const childSegs = collectTextSegments(item.element, attribs);
			if (childSegs.length) {
				if (segments.length && !prevEndsWithSpace && !childSegs[0].text.startsWith(" ")) {
					childSegs[0].text = " " + childSegs[0].text;
				}
				prevEndsWithSpace = childSegs[childSegs.length - 1].text.endsWith(" ");
				segments.push(...childSegs);
			}
		} else if (item.element) {
			warning("Unsupported nested element in text");
		}
	}
	if (segments.length) {
		segments[0].text = segments[0].text.replace(/^\s+/, "");
		segments[segments.length - 1].text = segments[segments.length - 1].text.replace(/\s+$/, "");
	}
	return segments.filter((s) => s.text !== "");
}

converters.text = function (element, attribs) {
	const separate = "transform" in attribs;
	if (separate) {
		output("context [");
		outputTransforms(attribs.transform);
	}
	const hasColor = "color" in attribs;
	const oldColor = currentColor;
	if (hasColor) {
		const c = attribs.color.trim().toLowerCase();
		if (c !== "inherit" && c !== "currentcolor") {
			currentColor = convertPaint(c).paint;
		}
	}
	const baseAttribs = applyTextAttributes(
		{
			fontName: defaultFontFamily,
			size: defaultFontSize,
			fill: currentColor,
			fillOpacity: 1,
		},
		attribs,
	);
	const segments = collectTextSegments(element, baseAttribs);
	if (!segments.length) {
		if (separate) output("]");
		if (hasColor) currentColor = oldColor;
		return;
	}
	let fontKey = "";
	let x = "x" in attribs ? convertUnits(attribs.x, "x") : 0;
	let y = "y" in attribs ? convertUnits(attribs.y, "y") : 0;
	let anchor = "";
	if ("text-anchor" in attribs) {
		switch (attribs["text-anchor"]) {
			case "middle":
				anchor = " anchor:center";
				break;
			case "end":
				anchor = " anchor:right";
				break;
		}
	}
	let needAt = true;
	for (const seg of segments) {
		const key = JSON.stringify(seg.attribs);
		if (key !== fontKey) {
			let color = seg.attribs.fill;
			if (/[:\s]/.test(color) && color !== "none") {
				color = `[${color}]`;
			}
			let fontCmd = `font ${quoteIMPD(seg.attribs.fontName)} size:${seg.attribs.size} color:${color}`;
			if (seg.attribs.fillOpacity !== 1) {
				fontCmd += ` opacity:${seg.attribs.fillOpacity}`;
			}
			if (seg.attribs.stroke) {
				let outline = seg.attribs.stroke;
				let opts = "";
				if (seg.attribs.strokeWidth) {
					opts += ` width:${seg.attribs.strokeWidth}`;
				}
				if (seg.attribs.strokeJoin) {
					opts += ` joints:${seg.attribs.strokeJoin}`;
				}
				if (seg.attribs.strokeCap) {
					opts += ` caps:${seg.attribs.strokeCap}`;
				}
				if (seg.attribs.strokeOpacity && seg.attribs.strokeOpacity !== 1) {
					opts += ` opacity:${seg.attribs.strokeOpacity}`;
				}
				if (seg.attribs.strokeMiter) {
					opts += ` miter:${seg.attribs.strokeMiter}`;
				}
				if (seg.attribs.strokeDash) {
					if (seg.attribs.strokeDash === "none") {
						opts += " dash:none";
					} else {
						opts += ` dash:${seg.attribs.strokeDash}`;
					}
				}
				if (seg.attribs.strokeDashOffset) {
					opts += ` dash-offset:${seg.attribs.strokeDashOffset}`;
				}
				if (opts || /[:\s]/.test(outline)) {
					fontCmd += ` outline:[${outline}${opts}]`;
				} else {
					fontCmd += ` outline:${outline}`;
				}
			}
			output(fontCmd);
			fontKey = key;
		}
		const t = seg.text.replace(/"/g, '\\"');
		if (needAt) {
			output(`TEXT at:${x},${y}${anchor} "${t}"`);
			needAt = false;
		} else {
			output(`TEXT "${t}"`);
		}
	}
	if (separate) output("]");
	if (hasColor) currentColor = oldColor;
};

converters.defs = function (element) {
        for (const item of element.contents || []) {
                if (!item.element) continue;
                const child = item.element;
                if (
                        child.type === "linearGradient" ||
                        child.type === "radialGradient" ||
                        child.type === "pattern" ||
                        child.type === "defs" ||
                        child.type === "mask" ||
                        child.type === "style"
                ) {
                        convertSVGElement(child);
                } else {
                        registerDefinition(child);
                }
        }
};

converters.use = function (element, attribs) {
	let ref = attribs.href || attribs["xlink:href"];
	if (!ref) {
		warning("Missing 'href' in use");
		return;
	}
	if (ref.startsWith("#")) {
		ref = ref.slice(1);
	}
	if (!(ref in definitions)) {
		warning("Unrecognized reference: " + ref);
		return;
	}
	const clone = JSON.parse(JSON.stringify(definitions[ref]));
	for (const [k, v] of Object.entries(attribs)) {
		if (k === "href" || k === "xlink:href" || k === "x" || k === "y" || k === "transform") {
			continue;
		}
		clone.attributes[k] = v;
	}
	const needsContext = "x" in attribs || "y" in attribs || "transform" in attribs;
	if (needsContext) {
		output("context [");
		if ("transform" in attribs) {
			outputTransforms(attribs.transform);
		}
		const tx = "x" in attribs ? convertUnits(attribs.x, "x") : 0;
		const ty = "y" in attribs ? convertUnits(attribs.y, "y") : 0;
		if (tx !== 0 || ty !== 0) {
			output(`offset ${tx},${ty}`);
		}
	}
	delete clone.attributes.id;
	convertSVGElement(clone);
	if (needsContext) {
		output("]");
	}
};

converters.clipPath = function () {};

converters.mask = function (element, attribs) {
	if (!("id" in attribs)) {
		warning("Missing 'id' in mask");
		return;
	}
	const savedOut = outputString;
	const savedIndent = indent;
	outputString = "";
	indent = 0;
	for (const item of element.contents || []) {
		if (!item.element) continue;
		const child = JSON.parse(JSON.stringify(item.element));
		child.attributes = child.attributes || {};
		if (!("fill" in child.attributes)) {
			child.attributes.fill = "1";
		} else {
			child.attributes.fill = convertMaskPaint(child.attributes.fill);
		}
		if ("stroke" in child.attributes) {
			child.attributes.stroke = convertMaskPaint(child.attributes.stroke);
		} else {
			child.attributes.stroke = "0";
		}
		delete child.attributes.mask;
		delete child.attributes["clip-path"];
		convertSVGElement(child);
	}
	const body = outputString
		.trim()
		.split("\n")
		.map((s) => s.trim())
		.filter(Boolean);
	outputString = savedOut;
	indent = savedIndent;
	definitions[attribs.id] = { attributes: attribs, body };
};

converters.linearGradient = function (element, attribs) {
	if (!("id" in attribs)) {
		warning("Missing 'id' in linearGradient");
		return;
	}
	const g = {
		type: "linear",
		x1: parseGradientCoord(attribs.x1 || "0%", "x"),
		y1: parseGradientCoord(attribs.y1 || "0%", "y"),
		x2: parseGradientCoord(attribs.x2 || "100%", "x"),
		y2: parseGradientCoord(attribs.y2 || "0%", "y"),
		stops: parseGradientStops(element),
		relative: attribs.gradientUnits !== "userSpaceOnUse",
	};
	if ("gradientTransform" in attribs) {
		g.transform = attribs.gradientTransform;
	}
	gradients.set(attribs.id, g);
};

converters.radialGradient = function (element, attribs) {
	if (!("id" in attribs)) {
		warning("Missing 'id' in radialGradient");
		return;
	}
	const g = {
		type: "radial",
		cx: parseGradientCoord(attribs.cx || "50%", "x"),
		cy: parseGradientCoord(attribs.cy || "50%", "y"),
		r: parseGradientCoord(attribs.r || "50%", "x"),
		stops: parseGradientStops(element),
		relative: attribs.gradientUnits !== "userSpaceOnUse",
	};
	if ("gradientTransform" in attribs) {
		g.transform = attribs.gradientTransform;
	}
	gradients.set(attribs.id, g);
};

converters.pattern = function (element, attribs) {
	if (!("id" in attribs)) {
		warning("Missing 'id' in pattern");
		return;
	}
	if (!("width" in attribs) || !("height" in attribs)) {
		warning("Missing 'width' or 'height' in pattern");
		return;
	}
	const p = {
		x: convertUnits(attribs.x || "0", "x"),
		y: convertUnits(attribs.y || "0", "y"),
		width: convertUnits(attribs.width, "x"),
		height: convertUnits(attribs.height, "y"),
		relative: attribs.patternUnits !== "userSpaceOnUse",
	};
	if ("patternTransform" in attribs) {
		p.transform = attribs.patternTransform;
	}
	const savedOut = outputString;
	const savedIndent = indent;
	outputString = "";
	indent = 0;
	for (const item of element.contents || []) {
		if (item.element) convertSVGElement(item.element);
	}
	p.body = outputString
		.trim()
		.split("\n")
		.map((s) => s.trim())
		.filter(Boolean)
		.join("; ");
	outputString = savedOut;
	indent = savedIndent;
	patterns.set(attribs.id, p);
};

function convertSVGElement(element) {
	if (element.type === "style") {
		let cssText = "";
		for (const item of element.contents || []) {
			if (item.text) cssText += item.text;
		}
		parseStyleSheet(cssText);
		return;
	}
	if (element.attributes) expandStyle(element.attributes);
	const parentStyle = styleStack[styleStack.length - 1];
	const computed = resolveStyles(element, element.attributes || {}, parentStyle);
	if (element.attributes) Object.assign(element.attributes, computed); else element.attributes = computed;
	registerDefinition(element);
	if (element.attributes && element.attributes.visibility === "hidden") {
	        return;
	}
	elementStack.push(element);
	styleStack.push(computed);
	const type = element.type;
	if (converters[type]) {
	        converters[type](element, element.attributes);
	} else {
	        warning("Can't convert type: " + type);
	}
	styleStack.pop();
	elementStack.pop();
}

function convertSVGContainer(container) {
	for (const item of container.contents || []) {
		if (item.element) {
			convertSVGElement(item.element);
		}
	}
}

function parseXML(src) {
	let pos = 0;
	const len = src.length;

	function skipWS() {
		while (pos < len && /[\s]/.test(src[pos])) pos++;
	}

	function parseName() {
		const start = pos;
		while (pos < len && /[-.0-9:_A-Za-z]/.test(src[pos])) pos++;
		return src.slice(start, pos);
	}

	function parseAttributes() {
		const attrs = {};
		while (true) {
			skipWS();
			if (pos >= len || src[pos] === "/" || src[pos] === ">") break;
			const name = parseName();
			skipWS();
			if (src[pos] === "=") {
				pos++;
				skipWS();
				const quote = src[pos++];
				const start = pos;
				while (pos < len && src[pos] !== quote) pos++;
				attrs[name] = src.slice(start, pos);
				pos++;
			} else {
				attrs[name] = "";
			}
		}
		return attrs;
	}

	function parseNode() {
		if (src.startsWith("<!--", pos)) {
			pos = src.indexOf("-->", pos);
			if (pos < 0) pos = len;
			else pos += 3;
			return null;
		}
		if (src.startsWith("<?", pos)) {
			pos = src.indexOf("?>", pos);
			if (pos < 0) pos = len;
			else pos += 2;
			return null;
		}
		if (src.startsWith("<!", pos)) {
			pos = src.indexOf(">", pos);
			if (pos < 0) pos = len;
			else pos += 1;
			return null;
		}
		return parseElement();
	}

	function parseElement() {
		if (src[pos] !== "<") return null;
		pos++;
		const name = parseName();
		const attrs = parseAttributes();
		let selfClose = false;
		if (src[pos] === "/") {
			selfClose = true;
			pos++;
		}
		if (src[pos] !== ">") throw new Error("Malformed XML");
		pos++;
		const contents = [];
		if (!selfClose) {
			while (true) {
				skipWS();
				if (src.startsWith("</" + name, pos)) {
					pos += name.length + 2;
					skipWS();
					if (src[pos] === ">") pos++;
					else throw new Error("Malformed XML");
					break;
				} else if (src[pos] === "<") {
					const child = parseNode();
					if (child) contents.push({ element: child });
				} else {
					const start = pos;
					while (pos < len && src[pos] !== "<") pos++;
					const text = src.slice(start, pos);
					if (text) contents.push({ text });
				}
			}
		}
		return { type: name, attributes: attrs, contents };
	}

	const contents = [];
	while (pos < len) {
		skipWS();
		if (pos >= len) break;
		if (src[pos] === "<") {
			const el = parseNode();
			if (el) contents.push({ element: el });
		} else {
			pos++;
		}
	}
	return { contents };
}

function convertFile(svgPath, ivgPath, defaultDimArg) {
	resetState();
	if (defaultDimArg) {
		const parts = defaultDimArg.split(",");
	       if (parts.length === 2) {
		       defaultWidth = parseFloat(parts[0]);
		       defaultHeight = parseFloat(parts[1]);
	       } else {
		       throw new Error("Invalid default dimensions: " + defaultDimArg);
	       }
	}
	let svgSource;
	try {
	       svgSource = fs.readFileSync(svgPath, "utf8");
	} catch (err) {
	       throw new Error("Failed to read " + svgPath + ": " + err.message);
	}
	const svg = parseXML(svgSource);
	output("format IVG-1 requires:IMPD-1");
	convertSVGContainer(svg);
	if (ivgPath) {
	       try {
		       fs.writeFileSync(ivgPath, outputString, "utf8");
	       } catch (err) {
		       throw new Error("Failed to write " + ivgPath + ": " + err.message);
	       }
	       console.log("Converted " + svgPath + " to " + ivgPath);
	} else {
	       console.log("------");
	       console.log(outputString);
	}
}

try {
	const args = process.argv.slice(2);
	if (args.length < 1) {
	       throw new Error("Usage: node svg2ivg.js input.svg [output.ivg] [defaultWidth,defaultHeight]");
	}
	let ivgPath;
	let defaultDimArg;
	if (args[1]) {
	       if (args[1].includes(",")) {
		       defaultDimArg = args[1];
	       } else {
		       ivgPath = args[1];
		       defaultDimArg = args[2];
	       }
	}
	convertFile(args[0], ivgPath, defaultDimArg);
} catch (err) {
	console.error(err.message);
	process.exitCode = 1;
}

function parseSelector(sel) {
	return sel.trim().split(/\s+/).map(parseCompound);
}
function parseCompound(part) {
	if (part === "*") return { tag: "*", id: null, classes: [], attrs: [] };
	const comp = { tag: null, id: null, classes: [], attrs: [] };
	let i = 0;
	while (i < part.length) {
		const ch = part[i];
		if (ch === "#") {
			i++;
			let j = i;
			while (j < part.length && /[A-Za-z0-9_-]/.test(part[j])) j++;
			comp.id = part.slice(i, j);
			i = j;
		} else if (ch === ".") {
			i++;
			let j = i;
			while (j < part.length && /[A-Za-z0-9_-]/.test(part[j])) j++;
			comp.classes.push(part.slice(i, j));
			i = j;
		} else if (ch === "[") {
			let j = part.indexOf("]", i);
			if (j === -1) break;
			const attr = part.slice(i + 1, j).split("=")[0].trim();
			comp.attrs.push(attr);
			i = j + 1;
		} else {
			let j = i;
			while (j < part.length && /[A-Za-z0-9_-]/.test(part[j])) j++;
			comp.tag = part.slice(i, j);
			i = j;
		}
	}
	return comp;
}
function calcSpecificity(tokens) {
	let s = 0;
	for (const t of tokens) {
		if (t.id) s += 100;
		s += 10 * (t.classes.length + t.attrs.length);
		if (t.tag && t.tag !== "*") s += 1;
	}
	return s;
}
function matchCompound(el, comp) {
	const attrs = el.attributes || {};
	if (comp.tag && comp.tag !== "*" && el.type !== comp.tag) return false;
	if (comp.id && attrs.id !== comp.id) return false;
	for (const cls of comp.classes) {
		if (!attrs.class || !attrs.class.split(/\s+/).includes(cls)) return false;
	}
	for (const a of comp.attrs) {
		if (!(a in attrs)) return false;
	}
	return true;
}
function selectorMatches(element, tokens) {
	if (!tokens.length) return false;
	if (!matchCompound(element, tokens[tokens.length - 1])) return false;
	let ancestorIndex = elementStack.length - 1;
	for (let i = tokens.length - 2; i >= 0; i--) {
		let matched = false;
		for (; ancestorIndex >= 0; ancestorIndex--) {
			if (matchCompound(elementStack[ancestorIndex], tokens[i])) {
				matched = true;
				ancestorIndex--;
				break;
			}
		}
		if (!matched) return false;
	}
	return true;
}
function resolveStyles(element, attribs, parentStyle) {
        const style = {};
        for (const prop of INHERITED_PROPERTIES) {
                if (parentStyle && prop in parentStyle) style[prop] = parentStyle[prop];
        }
	for (const prop of KNOWN_PRESENTATION_ATTRIBUTES) {
		if (prop in attribs) style[prop] = attribs[prop];
	}
	const matches = [];
	for (const rule of cssRules) {
		if (!selectorMatches(element, rule.tokens)) continue;
		for (const decl of rule.declarations) {
			matches.push({
				prop: decl.name,
				value: decl.value,
				important: decl.important ? 1 : 0,
				specificity: rule.specificity,
				order: rule.order,
			});
		}
	}
	matches.sort((a, b) => {
		if (a.important !== b.important) return a.important - b.important;
		if (a.specificity !== b.specificity) return a.specificity - b.specificity;
		return a.order - b.order;
	});
	for (const m of matches) {
		style[m.prop] = m.value;
	}
	if (attribs.__style) {
		for (const [name, decl] of Object.entries(attribs.__style)) {
			style[name] = decl.value;
		}
		delete attribs.__style;
	}
	return style;
}
