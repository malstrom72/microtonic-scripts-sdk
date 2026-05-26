# Example Selection

Use examples as implementation references, not as generic templates.

- Start from [`examples/EuclideanBeat.mtscript`](../../examples/EuclideanBeat.mtscript)
  for Euclidean rhythm tools, pattern generators, and GUI controls for rhythmic
  parameters.
- Start from [`examples/FMTool.mtscript`](../../examples/FMTool.mtscript) for
  patch manipulation tools and focused sound-design workflows.
- Start from [`examples/MacroTweak.mtscript`](../../examples/MacroTweak.mtscript)
  for macro-style parameter control and randomized variation tools.
- Start from [`examples/MixConsole.mtscript`](../../examples/MixConsole.mtscript)
  for channel utilities, mixer-like interfaces, and drag interactions.
- Start from [`JSConsole.mtscript`](../../JSConsole.mtscript) for console-style
  script package structure and JavaScript runtime interaction.

When adapting an example:

- Preserve the package structure unless the user's task clearly calls for a
  classic GUI-less script.
- Update schema includes and resource paths consistently.
- Run CushyLint when `.cushy` files are touched.
- Keep generated JavaScript compatible with the SDK's JavaScript engine
  constraints.

