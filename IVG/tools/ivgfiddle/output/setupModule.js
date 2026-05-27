function initializeFiddle() {
	let initSource = localStorage.getItem("ivgSource");
	if (initSource == null || initSource === '') {
		initSource = Module.FS.readFile('demoSource.ivg', { encoding: 'utf8' });
	}
	ace.edit("editor").setValue(initSource);
	if (localStorage.getItem("runOnStartup") === 'false') {
		trace("Last execution was terminated. Modify IVG code to run again.");
	} else {
		runIVG();
	}
}

const moduleConfig = {
	print: function(text) { trace(text); },
	printErr: function(text) { trace(text); }
};
var Module = moduleConfig;
let moduleStarted = false;
function startModuleWhenReady() {
	if (moduleStarted) {
		return;
	}
	if (typeof Module !== 'function' || typeof runIVG !== 'function') {
		setTimeout(startModuleWhenReady, 0);
		return;
	}
	moduleStarted = true;
	Module(moduleConfig).then(function(instance) {
		Module = instance;
		initializeFiddle();
	});
}
if (document.readyState === 'loading') {
	window.addEventListener('load', startModuleWhenReady);
} else {
	startModuleWhenReady();
}
