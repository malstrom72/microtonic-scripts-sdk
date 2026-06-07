# Source Map

Use these repository files as the grounding map for Microtonic scripting work.

## Core SDK References

- [`README.md`](../../README.md): repository structure, terminology, technology
  boundaries, and high-level constraints.
- [`docs/Microtonic JS Reference.md`](../../docs/Microtonic%20JS%20Reference.md):
  JavaScript API details, engine limits, built-in functions, objects,
  constants, GUI variables, and GUI actions.
- [`Microtonic Resources/main.js`](../../Microtonic%20Resources/main.js):
  implementation source for documented JavaScript utility globals and helper
  objects such as `assert`, `Object.assign`, `Date.now`, `Math.sign`,
  `Math.cbrt`, `Math.log10`, `clamp`, `square`, `cube`, `cbrt`, `lerp`,
  `bounce`, `converge`, `scale`, `fract`, `unescape`, `random`,
  `createClass`, `StringBuilder`, `displayCushy`, `toggleCushy`, and
  `closeCushy`.
- [`docs/Microtonic User Guide.md`](../../docs/Microtonic%20User%20Guide.md):
  Microtonic product behavior, terminology, pattern behavior, patch concepts,
  workflow, and user-facing feature descriptions.
- [`docs/Microtonic User Guide.pdf`](../../docs/Microtonic%20User%20Guide.pdf):
  original user guide source.

## Cushy And GUI References

- [`docs/Cushy Documentation.md`](../../docs/Cushy%20Documentation.md):
  orientation to the Cushy system — mental model, minimal window walkthrough,
  and a map of where each part of Cushy is documented. Start here, then drill
  into the schema for exact syntax.
- [`CushyLint/cushy.schema`](../../CushyLint/cushy.schema): official `.cushy`
  format reference, view types, built-in actions, schema rules, and comments.
- [`Microtonic Resources/microtonic.schema`](../../Microtonic%20Resources/microtonic.schema):
  Microtonic-specific schema additions.
- [`Microtonic Resources/`](../../Microtonic%20Resources/): Microtonic GUI
  resources, Makaron files, built-in resources, and schema support.
- [`IVG/`](../../IVG/): vendored IVG source, tools, tests, fonts, and
  documentation matching the shipping Microtonic IVG implementation. Prefer
  this copy over live upstream when exact shipped IVG behavior matters.
- [`docs/Makaron Documentation.md`](../../docs/Makaron%20Documentation.md):
  Makaron macro syntax.
- [`docs/IVG Documentation.md`](../../docs/IVG%20Documentation.md): IVG vector
  graphics language.
- [`docs/ImpD Documentation.md`](../../docs/ImpD%20Documentation.md): ImpD
  language reference.
- [`docs/ivgfont Documentation.md`](../../docs/ivgfont%20Documentation.md):
  IVG font format reference.
- [`cushy-notes.md`](cushy-notes.md): practical Cushy gotchas, UI behavior
  notes, and implementation patterns discovered while building scripts.

## Examples

- [`JSConsole.mtscript`](../../JSConsole.mtscript): JavaScript console package
  and a useful reference for script package structure.
- [`tools/jsconsole-bridge-mcp/README.md`](../../tools/jsconsole-bridge-mcp/README.md):
  MCP bridge setup, `mt_eval` usage, launching scripts with `run(...)`, and
  reload/reset behavior over the live JSConsole bridge.
- [`examples/EuclideanBeat.mtscript`](../../examples/EuclideanBeat.mtscript):
  Euclidean pattern generation and GUI package structure.
- [`examples/FMTool.mtscript`](../../examples/FMTool.mtscript): patch editing
  tool structure.
- [`examples/MacroTweak.mtscript`](../../examples/MacroTweak.mtscript): macro
  control workflow.
- [`examples/MixConsole.mtscript`](../../examples/MixConsole.mtscript): channel
  utility UI and drag interactions.
- [`examples/screenshots/`](../../examples/screenshots): screenshots of
  Microtonic and official example script windows for visual alignment,
  proportions, control density, contrast, saturation, accent treatment, and
  window treatment.

Inspect package contents instead of assuming `.mtscript` examples are flat
files.

## Legacy References

- [`legacy/`](../../legacy/): use only when the user explicitly asks for legacy
  PikaScript behavior or when repository material clearly indicates the legacy
  path is the correct fit.
