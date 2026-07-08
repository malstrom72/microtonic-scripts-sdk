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
			? 'Use tools\\locate-scripts-folder.ps1 -Verify or Microtonic > Open Scripts Folder'
			: 'Use Microtonic > Open Scripts Folder';

	console.log('Install this SDK\'s bridged JSConsole.mtscript into Microtonic Scripts.');
	console.log('');
	console.log(`Source: ${source}`);
	console.log(`Default target hint: ${platformDefault}`);
	console.log('');
	console.log('Usage:');
	console.log('  node tools/install-jsconsole.js "<Microtonic Scripts folder>"');
	console.log('');
	console.log('Find the target with Microtonic > Open Scripts Folder, DIRS.SCRIPTS over an existing bridge,');
	console.log('or on Windows: powershell -ExecutionPolicy Bypass -File tools\\locate-scripts-folder.ps1 -Verify');
}

function failMissingFolder(resolvedTarget) {
	const lines = [
		`Target Microtonic Scripts folder does not exist: ${resolvedTarget}`,
		'',
		'Microtonic shows the puzzle (script) menu but keeps it greyed out and inactive',
		'until this folder exists, so on a fresh install you must create it before the',
		'console can be installed. On a stock Mac the folder lives under root-owned',
		'/Library, so creating it and copying into it needs one elevated step. Stage the',
		'console through /tmp first so the elevated copy never has to read the SDK checkout',
		'(macOS TCC blocks that when the checkout is under ~/Documents, Desktop, or',
		'Downloads):',
	];
	if (process.platform === 'darwin') {
		lines.push(
			'',
			`  rm -rf "/tmp/JSConsole.mtscript"`,
			`  cp -R ${JSON.stringify(source)} /tmp/`,
			`  osascript -e 'do shell script "mkdir -p \\"${resolvedTarget}\\" && cp -R \\"/tmp/JSConsole.mtscript\\" \\"${resolvedTarget}/\\"" with administrator privileges'`,
			'',
			'That is a one-time setup — ongoing iteration goes over the bridge, not this folder.',
			'',
			'Alternatively, link the standard location to your project\'s scripts/ folder so later',
			'installs and edits need no elevation (see "development scripts folder" in the README).',
			'Run from your project root; scripts/ becomes Microtonic\'s live installation:',
			'',
			`  mkdir -p scripts`,
			`  cp -R ${JSON.stringify(source)} scripts/`,
			`  osascript -e "do shell script \\"ln -s '$PWD/scripts' '${resolvedTarget}'\\" with administrator privileges"`,
			'',
			'If the folder already exists and is writable (e.g. such a development symlink), just',
			're-run this installer.',
		);
	} else {
		lines.push(
			'',
			'Create the Microtonic Scripts folder shown by Microtonic > Open Scripts Folder (it',
			'may be under an elevated location such as C:\\Program Files\\Sonic Charge), then',
			're-run this installer against it.',
		);
	}
	fail(lines.join('\n'));
}

verifySource();

const targetRoot = process.argv[2];
if (!targetRoot) {
	usage();
	process.exit(0);
}

const target = path.join(path.resolve(targetRoot), 'JSConsole.mtscript');
if (!fs.existsSync(targetRoot)) {
	failMissingFolder(path.resolve(targetRoot));
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
