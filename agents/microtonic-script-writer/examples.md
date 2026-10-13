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
- Start from [`examples/PolyChain.mtscript`](../../examples/PolyChain.mtscript)
  for generators that write chained patterns, and for keeping a GUI in sync with
  the document after undo and outside edits (hash history, synced / out-of-sync
  state).
- Start from [`examples/BeatSpace.mtscript`](../../examples/BeatSpace.mtscript)
  for generated drum patches and patterns, press-drag-release pads, and
  recognizing generated output after undo.
- Start from [`examples/BeatDrop.mtscript`](../../examples/BeatDrop.mtscript)
  for timer-driven, game-like interaction that writes pattern triggers.
- Start from [`examples/Pick New Patches.js`](../../examples/Pick%20New%20Patches.js)
  for GUI-less scripts that browse the factory drum patches and replace channel
  patches.
- Start from [`JSConsole.mtscript`](../../JSConsole.mtscript) for console-style
  script package structure and JavaScript runtime interaction.
- Use [`examples/screenshots/`](../../examples/screenshots) as visual references
  when designing or reviewing GUI scripts. These screenshots show Microtonic and
  the official example scripts in context, and are useful for matching spacing,
  scale, contrast, saturation, window treatment, control density, and overall
  visual alignment with the bundled scripts. Do not copy hues rigidly; the
  official examples use different accent colors for different tools. There are
  window screenshots for BeatSpace, FMTool, MacroTweak and MixConsole; BeatDrop,
  EuclideanBeat and PolyChain have none yet.

When adapting an example:

- Preserve the package structure unless the user's task clearly calls for a
  classic GUI-less script.
- Update schema includes and resource paths consistently.
- Run CushyLint when `.cushy` files are touched.
- Keep generated JavaScript compatible with the SDK's JavaScript engine
  constraints.
