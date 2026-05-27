const moduleConfig = {
print: function(text) { trace(text); },
printErr: function(text) { trace(text); },
onRuntimeInitialized: function() {
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
