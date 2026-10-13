const moduleConfig = {
print: function(text) { trace(text); },
printErr: function(text) { trace(text); },
onRuntimeInitialized: function() {
// Runs before the factory's promise resolves, so take the instance from `this` rather than waiting for the `then` below.
const runtimeModule = this;
Module = runtimeModule;
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
};
var Module = moduleConfig;
window.addEventListener('load', function() {
if (typeof Module === 'function') {
Module(moduleConfig).then(function(instance) {
Module = instance;
});
}
});
