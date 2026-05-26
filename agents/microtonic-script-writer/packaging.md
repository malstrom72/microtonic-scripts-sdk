# Packaging Guidance

The SDK documents two practical script shapes.

## Classic GUI-Less Scripts

Use a single JavaScript file when the user wants a direct utility, generation
script, patch operation, pattern operation, or other simple command without a
custom interface.

Classic scripts are the default for simple requests.

## `.mtscript` Packages

Use a `.mtscript` package when the user asks for:

- A custom GUI.
- A persistent tool window.
- Cushy controls, GUI variables, or GUI actions.
- A reusable tool that benefits from stateful controls.
- A script modeled on one of the repository `.mtscript` examples.

Treat `.mtscript` examples as directories/packages. Inspect their internal
files before adapting them.

## GUI Startup Pattern

For GUI scripts, prefer rerun-safe startup code:

- Create one guarded top-level script object for first-run state.
- Refresh functions, action handlers, and GUI variable handlers on subsequent
  startup runs.
- Assume startup may run again when the GUI reopens or rebuilds. End-user zoom
  scale changes also force GUI reloads so resources and layout coordinates can
  be rescaled.
- Wire close controls to an actual close action.

## Generated Pattern Tools

For generators that derive from the current pattern and then overwrite pattern
data, capture a stable source-pattern snapshot when the script opens. Generate
from that snapshot, not from partially overwritten live pattern data.
