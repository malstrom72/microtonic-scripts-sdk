#!/usr/bin/env node
// Install this SDK's bridged JSConsole.mtscript into a user-confirmed Microtonic Scripts folder.

const fs = require('fs');
const path = require('path');

const repoRoot = path.resolve(__dirname, '..');
const source = path.join(repoRoot, 'JSConsole.mtscript');

function fail(message) {
	console.error(message);
	process.exit(1);
}

function verifySource() {
	const main = path.join(source, 'JSConsole_main.js');
	if (!fs.existsSync(main)) {
		fail(`Missing JSConsole source: ${main}`);
	}
	const text = fs.readFileSync(main, 'utf8');
	const markers = ['bridge on', 'bridge off', 'bridgeOn', 'jsConsole.bridge'];
	for (const marker of markers) {
		if (!text.includes(marker)) {
			fail(`Refusing to install: JSConsole source does not contain bridge marker ${JSON.stringify(marker)}.`);
		}
	}
}

function copyDir(from, to) {
	fs.mkdirSync(to, { recursive: true });
	for (const entry of fs.readdirSync(from, { withFileTypes: true })) {
		const src = path.join(from, entry.name);
		const dst = path.join(to, entry.name);
		if (entry.isDirectory()) {
			copyDir(src, dst);
		} else if (entry.isFile()) {
			fs.copyFileSync(src, dst);
		} else {
			fail(`Refusing to copy unsupported filesystem entry: ${src}`);
		}
	}
}

function usage() {
	const platformDefault = process.platform === 'darwin'
		? '/Library/Application Support/Sonic Charge/Microtonic Scripts'
		: process.platform === 'win32'
			? 'Use Microtonic > Open Scripts Folder'
			: 'Use Microtonic > Open Scripts Folder';

	console.log('Install this SDK\'s bridged JSConsole.mtscript into Microtonic Scripts.');
	console.log('');
	console.log(`Source: ${source}`);
	console.log(`Default target hint: ${platformDefault}`);
	console.log('');
	console.log('Usage:');
	console.log('  node tools/install-jsconsole.js "<Microtonic Scripts folder>"');
	console.log('');
	console.log('Find the target with Microtonic > Open Scripts Folder, or DIRS.SCRIPTS over an existing bridge.');
}

verifySource();

const targetRoot = process.argv[2];
if (!targetRoot) {
	usage();
	process.exit(0);
}

const target = path.join(path.resolve(targetRoot), 'JSConsole.mtscript');
if (!fs.existsSync(targetRoot)) {
	fail(`Target Microtonic Scripts folder does not exist: ${targetRoot}`);
}
if (!fs.statSync(targetRoot).isDirectory()) {
	fail(`Target is not a directory: ${targetRoot}`);
}

copyDir(source, target);

const installedMain = path.join(target, 'JSConsole_main.js');
if (!fs.existsSync(installedMain)) {
	fail(`Copy finished but installed console is missing ${installedMain}`);
}

console.log(`Installed bridged JSConsole to: ${target}`);
console.log('Next: open Microtonic, launch JSConsole from the script menu, and type: bridge on');
