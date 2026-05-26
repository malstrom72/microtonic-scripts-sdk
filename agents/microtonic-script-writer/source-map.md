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

- [`CushyLint/cushy.schema`](../../CushyLint/cushy.schema): official `.cushy`
  format reference, view types, built-in actions, schema rules, and comments.
- [`Microtonic Resources/microtonic.schema`](../../Microtonic%20Resources/microtonic.schema):
  Microtonic-specific schema additions.
- [`Microtonic Resources/`](../../Microtonic%20Resources/): Microtonic GUI
  resources, Makaron files, built-in resources, and schema support.
- [`docs/Makaron Documentation.md`](../../docs/Makaron%20Documentation.md):
  Makaron macro syntax.
- [`docs/IVG Documentation.md`](../../docs/IVG%20Documentation.md): IVG vector
  graphics language.
- [`docs/ImpD Documentation.md`](../../docs/ImpD%20Documentation.md): ImpD
  language reference.

## Examples

- [`JSConsole.mtscript`](../../JSConsole.mtscript): JavaScript console package
  and a useful reference for script package structure.
- [`examples/EuclideanBeat.mtscript`](../../examples/EuclideanBeat.mtscript):
  Euclidean pattern generation and GUI package structure.
- [`examples/FMTool.mtscript`](../../examples/FMTool.mtscript): patch editing
  tool structure.
- [`examples/MacroTweak.mtscript`](../../examples/MacroTweak.mtscript): macro
  control workflow.
- [`examples/MixConsole.mtscript`](../../examples/MixConsole.mtscript): channel
  utility UI and drag interactions.

Inspect package contents instead of assuming `.mtscript` examples are flat
files.

## Legacy References

- [`legacy/`](../../legacy/): use only when the user explicitly asks for legacy
  PikaScript behavior or when repository material clearly indicates the legacy
  path is the correct fit.
