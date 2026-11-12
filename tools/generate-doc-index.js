#!/usr/bin/env node
/*
	Generates llms.txt, a section index of the SDK's docs, agent notes and schemas, so an agent with a small context
	window can read the index and then open only the lines it needs instead of whole references.

	Usage: node tools/generate-doc-index.js [--check]

	Each indexed section gets one line: its line range, an estimated size in tokens (characters / 4) and its heading.
	`levels` is the deepest heading level indexed for a file; with levels 0 the file gets only its own line. `notes`
	adds a short description after headings that do not say what they cover; a note whose heading is gone is an error.
	Schema files are indexed by rule definition (`<name> = ...`). With --check, nothing is written and the exit code is
	1 if the checked-in index differs from what would be generated, so CI catches a stale index.

	Shared between the Microtonic and Synplant script SDKs. Only the product configuration block below differs
	between the two copies.
*/

const fs = require("fs");
const path = require("path");

// ---- Product configuration -----------------------------------------------------------------------------------------
const TITLE = "Microtonic Scripts SDK";
const SUMMARY = "Docs, agent notes and schemas for writing Microtonic 3.3.4 scripts (JavaScript, Cushy GUI, IVG"
		+ " graphics). Read core-card.md first, then open only the line ranges you need. Paths are relative to the SDK"
		+ " root, which in a project is usually references/microtonic-scripts-sdk/.";
const CORE_CARD = "agents/microtonic-script-writer/core-card.md";
const GROUPS = [
	{
		name: "Agent notes",
		files: [
			{ path: "agents/microtonic-script-writer/instructions.md", levels: 3 },
			{ path: "agents/microtonic-script-writer/cushy-notes.md", levels: 3, notes: {
				"Easy .cushy Mistakes": "@define comments, color spellings and ranges, ASCII, button captions"
			} },
			{ path: "agents/microtonic-script-writer/validation.md", levels: 3, notes: {
				"All Examples At Once": "tools/validate-examples (.sh or .cmd) on every bundled package",
				"JavaScript Syntax": "run tools/validate-js (.sh or .cmd), ESLint ES5 rules, globals",
				"Single File": "CushyLint on one .cushy file",
				"Directory": "CushyLint on a package folder",
				"Package Schema Header": "the package .schema include: and resources: lines",
				"Expected Practice": "when to validate, and what a CushyLint pass does not prove"
			} },
			{ path: "agents/microtonic-script-writer/packaging.md", levels: 3 },
			{ path: "agents/microtonic-script-writer/examples.md", levels: 3 },
			{ path: "agents/microtonic-script-writer/source-map.md", levels: 2 },
			{ path: "agents/microtonic-script-writer/vibe-coding.md", levels: 2 }
		]
	},
	{
		name: "Reference",
		files: [
			{ path: "docs/Microtonic JS Reference.md", levels: 3, notes: {
				"Engine": "NuXJS, ES3 with partial ES5, the Math.round string bug",
				"GUI Variables": "get, set and touch objects, the reload-safe state guard",
				"GUI Actions": "execute, checked and enabled action objects"
			} },
			{ path: "docs/Cushy Documentation.md", levels: 3 },
			{ path: "CushyLint/cushy.schema", schema: true },
			{ path: "Microtonic Resources/microtonic.schema", schema: true },
			{ path: "docs/Makaron Documentation.md", levels: 2 },
			{ path: "docs/IVG Documentation.md", levels: 3, notes: {
				"Intro": "what IVG is, a sample file, the format line"
			} },
			{ path: "docs/Development Scripts Folder.md", levels: 2 },
			{ path: "README.md", levels: 2 }
		]
	},
	{
		name: "Optional",
		files: [
			{ path: "docs/ImpD Documentation.md", levels: 0 },
			{ path: "docs/ivgfont Documentation.md", levels: 0 },
			{ path: "docs/Microtonic User Guide.md", levels: 0 }
		]
	}
];
const OUTPUT = "llms.txt";
// ---- End of product configuration ----------------------------------------------------------------------------------

const sdkRoot = path.resolve(__dirname, "..");

function readLines(relativePath) {
	return fs.readFileSync(path.join(sdkRoot, relativePath), "utf8").replace(/\r\n?/g, "\n").replace(/\n$/, "")
			.split("\n");
}

function tokens(lines, first, last) {
	let chars = 0;
	for (let i = first; i <= last; ++i) {
		chars += lines[i].length + 1;
	}
	return chars / 4;
}

// Thousands with one decimal, at least 0.1k.
function formatTokens(count) {
	return Math.max(1, Math.round(count / 100)) / 10 + "k";
}

// Heading text without inline markup: links keep their text, code spans lose their backticks.
function plainHeading(text) {
	return text.replace(/\[([^\]]*)\]\([^)]*\)/g, "$1").replace(/[`*]/g, "").replace(/\s+#+\s*$/, "").trim();
}

// Headings outside code fences, both ATX (## Title) and setext (Title over === or ---).
function findHeadings(lines) {
	const headings = [];
	let inFence = false;
	for (let i = 0; i < lines.length; ++i) {
		const line = lines[i];
		if (/^\s*(```|~~~)/.test(line)) {
			inFence = !inFence;
			continue;
		}
		if (inFence) {
			continue;
		}
		const atx = /^(#{1,6})\s+(.*\S)\s*$/.exec(line);
		if (atx) {
			headings.push({ line: i, level: atx[1].length, text: atx[2] });
		} else if (i > 0 && /^(=+|-+)\s*$/.test(line) && line.trim().length >= 3 && /\S/.test(lines[i - 1])
				&& !/^\s*([-*+>|]|\d+\.)\s/.test(lines[i - 1]) && !/^\s/.test(lines[i - 1])) {
			headings.push({ line: i - 1, level: line[0] === "=" ? 1 : 2, text: lines[i - 1].trim() });
		}
	}
	return headings;
}

function indexMarkdown(file) {
	const lines = readLines(file.path);
	const headings = findHeadings(lines);
	const notes = file.notes || {};
	const used = {};
	const out = [];
	for (let h = 0; h < headings.length; ++h) {
		const heading = headings[h];
		const text = plainHeading(heading.text);
		if (heading.level === 1 || heading.level > file.levels || /^(table of )?contents$/i.test(text)) {
			continue;
		}
		let end = lines.length - 1;
		for (let n = h + 1; n < headings.length; ++n) {
			if (headings[n].level <= heading.level) {
				end = headings[n].line - 1;
				break;
			}
		}
		const indent = heading.level === 3 ? "  " : "";
		let note = "";
		if (Object.prototype.hasOwnProperty.call(notes, text)) {
			note = ": " + notes[text];
			used[text] = true;
		}
		out.push(indent + "- L" + (heading.line + 1) + "-" + (end + 1) + " "
				+ formatTokens(tokens(lines, heading.line, end)) + " " + text + note);
	}
	for (const text of Object.keys(notes)) {
		if (!used[text]) {
			throw new Error(file.path + ": no indexed heading \"" + text + "\" for its note");
		}
	}
	return { lines: lines, entries: out };
}

function indexSchema(file) {
	const lines = readLines(file.path);
	const starts = [];
	for (let i = 0; i < lines.length; ++i) {
		const rule = /^<([A-Za-z][A-Za-z0-9]*)>\s*=/.exec(lines[i]);
		if (rule) {
			starts.push({ line: i, name: rule[1] });
		}
	}
	const out = [];
	for (let r = 0; r < starts.length; ++r) {
		let end = (r + 1 < starts.length ? starts[r + 1].line : lines.length) - 1;
		while (end > starts[r].line && !/\S/.test(lines[end])) {
			--end;
		}
		out.push("L" + (starts[r].line + 1) + "-" + (end + 1) + " <" + starts[r].name + ">");
	}
	return { lines: lines, entries: out.length > 0 ? ["- " + out.join(", ")] : [] };
}

function generate() {
	const out = [];
	out.push("# " + TITLE);
	out.push("");
	out.push("> " + SUMMARY);
	out.push("");
	out.push("Generated by tools/generate-doc-index.js; do not edit. Each line is: line range, size in tokens (chars/4),"
			+ " section. Read only the lines you need, for example with `sed -n 120,180p <file>`.");
	out.push("");
	const coreCard = readLines(CORE_CARD);
	out.push("- Core card, " + CORE_CARD + ": rules for every task, "
			+ formatTokens(tokens(coreCard, 0, coreCard.length - 1)) + ". Always load it.");
	for (const group of GROUPS) {
		out.push("");
		out.push("## " + group.name);
		for (const file of group.files) {
			const result = file.schema ? indexSchema(file) : indexMarkdown(file);
			out.push("");
			out.push("### " + file.path + " (" + result.lines.length + " lines, "
					+ formatTokens(tokens(result.lines, 0, result.lines.length - 1)) + ")");
			if (result.entries.length > 0) {
				out.push("");
				for (const entry of result.entries) {
					out.push(entry);
				}
			}
		}
	}
	return out.join("\n") + "\n";
}

function main() {
	const args = process.argv.slice(2);
	const check = args.length === 1 && args[0] === "--check";
	if (args.length > 1 || (args.length === 1 && !check)) {
		console.error("usage: generate-doc-index.js [--check]");
		process.exit(2);
	}
	const text = generate();
	const outputPath = path.join(sdkRoot, OUTPUT);
	if (check) {
		const current = fs.existsSync(outputPath) ? fs.readFileSync(outputPath, "utf8").replace(/\r\n?/g, "\n") : "";
		if (current !== text) {
			console.error(OUTPUT + " is stale; run: node tools/generate-doc-index.js");
			process.exit(1);
		}
		console.log(OUTPUT + " is up to date");
		return;
	}
	fs.writeFileSync(outputPath, text, "utf8");
	console.log("wrote " + OUTPUT + " (" + formatTokens(text.length / 4) + " tokens)");
}

main();
