#!/usr/bin/env node
// Install this SDK's bridged JS console package into a user-confirmed Scripts folder.

// ==== BEGIN PRODUCT CONFIGURATION ====
// This block is the only part of this file that differs between the Microtonic and Synplant SDKs.
// Everything below the END marker is shared and must stay byte-identical in both repositories.
const PRODUCT = 'Microtonic';
const CONSOLE_NAME = 'JSConsole';
const CONSOLE_PACKAGE = 'JSConsole.mtscript';
const SCRIPTS_FOLDER = 'Microtonic Scripts';
const MAC_SCRIPTS_PATH = '/Library/Application Support/Sonic Charge/Microtonic Scripts';
const WINDOWS_SCRIPTS_PATH = 'C:\\Program Files\\Sonic Charge\\Microtonic Scripts';
// Why the folder has to be created by hand on a fresh install (one terminal line per entry).
const MISSING_FOLDER_REASON = [
	'Microtonic shows the puzzle (script) menu but keeps it greyed out and inactive,',
	'including Open Scripts Folder, until this folder exists, so on a fresh install you',
	'must create it before the console can be installed.',
];
// ==== END PRODUCT CONFIGURATION ====

const fs = require('fs');
const path = require('path');

const repoRoot = path.resolve(__dirname, '..');
const source = path.join(repoRoot, CONSOLE_PACKAGE);
const mainFile = 'JSConsole_main.js';
const locator = path.join(__dirname, 'locate-scripts-folder.ps1');

function fail(message) {
	console.error(message);
	process.exit(1);
}

function verifySource() {
	const main = path.join(source, mainFile);
	if (!fs.existsSync(main)) {
		fail(`Missing ${CONSOLE_NAME} source: ${main}`);
	}
	const text = fs.readFileSync(main, 'utf8');
	const markers = ['bridge on', 'bridge off', 'bridgeOn', 'jsConsole.bridge'];
	for (const marker of markers) {
		if (!text.includes(marker)) {
			fail(`Refusing to install: ${CONSOLE_NAME} source does not contain bridge marker ${JSON.stringify(marker)}.`);
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

function realPathOrNull(p) {
	try {
		return fs.realpathSync(p);
	} catch (e) {
		return null;
	}
}

// Replace any existing install instead of copying over it, so files that a newer console no longer
// ships do not linger. The copy is staged next to the target first, so a failed copy leaves the
// previous install untouched.
function installPackage(target) {
	const staging = `${target}.installing`;
	fs.rmSync(staging, { recursive: true, force: true });
	copyDir(source, staging);
	if (!fs.existsSync(path.join(staging, mainFile))) {
		fail(`Copy finished but the staged console is missing ${path.join(staging, mainFile)}`);
	}
	// rmSync does not follow symlinks: a linked package is unlinked, never its target emptied.
	fs.rmSync(target, { recursive: true, force: true });
	fs.renameSync(staging, target);
}

function usage() {
	const platformDefault = process.platform === 'darwin'
		? MAC_SCRIPTS_PATH
		: process.platform === 'win32'
			? `${WINDOWS_SCRIPTS_PATH} (confirm with tools\\locate-scripts-folder.ps1 -Verify)`
			: `Use ${PRODUCT} > Open Scripts Folder`;

	console.log(`Install this SDK's bridged ${CONSOLE_PACKAGE} into ${SCRIPTS_FOLDER}.`);
	console.log('');
	console.log(`Source: ${source}`);
	console.log(`Default target hint: ${platformDefault}`);
	console.log('');
	console.log('Usage:');
	console.log(`  node tools/install-jsconsole.js "<${SCRIPTS_FOLDER} folder>"`);
	console.log('');
	console.log('An existing console package in the target is replaced, not merged.');
	console.log(`Find the target with ${PRODUCT} > Open Scripts Folder (once the folder exists), DIRS.SCRIPTS`);
	console.log('over an existing bridge, or on Windows:');
	console.log('  powershell -ExecutionPolicy Bypass -File tools\\locate-scripts-folder.ps1 -Verify');
	console.log('If the folder does not exist yet, run this installer against the expected path anyway;');
	console.log('it prints the one-time commands that create or link the folder.');
}

function failMissingFolder(resolvedTarget) {
	const lines = [
		`Target ${SCRIPTS_FOLDER} folder does not exist: ${resolvedTarget}`,
		'',
		...MISSING_FOLDER_REASON,
	];
	if (process.platform === 'darwin') {
		const stagedCopy = `/tmp/${CONSOLE_PACKAGE}`;
		lines.push(
			'',
			'On a stock Mac the folder lives under root-owned /Library, so creating it and copying',
			'into it needs one elevated step. Stage the console through /tmp first so the elevated',
			'copy never has to read the SDK checkout (macOS TCC blocks that when the checkout is',
			'under ~/Documents, Desktop, or Downloads):',
			'',
			`  rm -rf "${stagedCopy}"`,
			`  cp -R ${JSON.stringify(source)} /tmp/`,
			`  osascript -e 'do shell script "mkdir -p \\"${resolvedTarget}\\" && cp -R \\"${stagedCopy}\\" \\"${resolvedTarget}/\\"" with administrator privileges'`,
			'',
			'That is a one-time setup — ongoing iteration goes over the bridge, not this folder.',
			'',
			'Alternatively, link the standard location to your project\'s scripts/ folder so later',
			'installs and edits need no elevation (see "development scripts folder" in the README).',
			`Run from your project root; scripts/ becomes ${PRODUCT}'s live installation:`,
			'',
			'  mkdir -p scripts',
			`  cp -R ${JSON.stringify(source)} scripts/`,
			`  osascript -e "do shell script \\"ln -s '$PWD/scripts' '${resolvedTarget}'\\" with administrator privileges"`,
			'',
			'If the folder already exists and is writable (e.g. such a development symlink), just',
			're-run this installer.',
		);
	} else if (process.platform === 'win32') {
		lines.push(
			'',
			`On Windows the folder is normally ${WINDOWS_SCRIPTS_PATH}. Confirm the`,
			'exact path first (it reads the same registry key the engine does):',
			'',
			`  powershell -ExecutionPolicy Bypass -File "${locator}"`,
			'',
			'That location is under C:\\Program Files, so creating it needs one elevated step. Open',
			'Command Prompt (cmd.exe) with "Run as administrator" and choose one of:',
			'',
			'A - Create the folder at the standard location and install into it:',
			'',
			`  mkdir "${resolvedTarget}"`,
			`  node "${__filename}" "${resolvedTarget}"`,
			'',
			'  The folder stays admin-only, so later installs need an elevated prompt too, but',
			'  ongoing iteration goes over the bridge, not this folder.',
			'',
			'B - Link the standard location to your project\'s scripts\\ folder so later installs and',
			'  edits need no elevation (see "development scripts folder" in the README).',
			`  scripts\\ becomes ${PRODUCT}'s live installation:`,
			'',
			'  cd /d "<your project root>"',
			'  mkdir scripts',
			`  xcopy "${source}" "scripts\\${CONSOLE_PACKAGE}\\" /E /I`,
			`  mklink /J "${resolvedTarget}" "%CD%\\scripts"`,
			'',
			'Either way this is a one-time setup. Once the folder exists, re-run this installer to',
			'refresh the console.',
		);
	} else {
		lines.push(
			'',
			`Create the ${SCRIPTS_FOLDER} folder, then re-run this installer against it.`,
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

const resolvedRoot = path.resolve(targetRoot);
const target = path.join(resolvedRoot, CONSOLE_PACKAGE);
if (!fs.existsSync(resolvedRoot)) {
	failMissingFolder(resolvedRoot);
}
if (!fs.statSync(resolvedRoot).isDirectory()) {
	fail(`Target is not a directory: ${resolvedRoot}`);
}
if (realPathOrNull(target) === realPathOrNull(source)) {
	fail(`Target already is this SDK's console package (same folder or a link to it): ${target}`);
}

installPackage(target);

const installedMain = path.join(target, mainFile);
if (!fs.existsSync(installedMain)) {
	fail(`Copy finished but installed console is missing ${installedMain}`);
}

console.log(`Installed bridged ${CONSOLE_NAME} to: ${target}`);
console.log(`Next: open ${PRODUCT}, launch ${CONSOLE_NAME} from the script menu, and type: bridge on`);
