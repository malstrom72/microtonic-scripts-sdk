# Microtonic Script Writer Core Card

The rules that apply to every Microtonic scripting task, for agents that cannot load the full instructions. Everything
else is in [`llms.txt`](../../llms.txt), a section index with line ranges: find the section there and read only those
lines. The full instructions are [`instructions.md`](instructions.md).

## JavaScript

- Microtonic runs ECMAScript 3 with partial ECMAScript 5. No arrow functions, `let`, `const`, classes, template
  literals, destructuring, promises, modules or async code. JSON, string index access, `Object.assign` and `Date.now`
  are available.
- Use only functions, objects, actions and schema fields documented in this SDK. Never invent API.
- Scripts run only while the Microtonic window is open. Closing it destroys the JavaScript environment; all scripts
  share one environment while it is open.
- A script that runs for 20 seconds is suspended, and the UI freezes while any script runs. For timed or repeated
  work use short repeating Cushy `autoexecs`, not loops.
- Memory is limited to about 64 MB per instance after garbage collection; every value costs at least 16 bytes.

## Reload-safe GUI scripts

- GUI scripts rerun on every reload, including the one a zoom change causes. Keep state in one global object named
  after the script and guard it at top level: `if (!this.myScript) { myScript = { inited: false }; }`. Reading an
  undeclared global throws, so test `this.myScript`, not `myScript`. For ESLint, add `/* global myScript:writable */`
  at the top of each file that uses it. Then reassign methods and GUI variable handlers on every run.
- In JSConsole, `reload` reruns the scripts and keeps globals; `reset` starts a clean engine.

## Cushy GUI (`.cushy`)

- All text needs an explicit `font`, for example `font: { ivgfont: "sans-serif", size: 13, color: "..." }`. Cushy has
  no default font, so text without one is not drawn.
- Colors use normalized components: `rgb(1,1,1,0.35)`, not `rgb(255,255,255,0.35)`.
- Views clip hard at their bounds: nothing outside a view's bounds is drawn or clickable.
- Keep `.cushy` source and the strings getters return ASCII.
- Makaron expands macros everywhere, comments included: never write `@name` of a macro in a `//` comment.
- Cushy polls every bound variable and action state 10 to 50 times a second, hidden or not. Keep getters, `enabled`
  and `checked` cheap: return values computed when their inputs changed.
- Never modify anything in `Microtonic Resources/`; it is a copy of the shipped files.
- `@window(left, top, width, height, ...)`: `height` includes the 24 px title bar. Initialize the script's
  `windowPosition` to `''`.
- To start a GUI package, copy `examples/MacroTweak.mtscript`, the smallest example (about 2.5k tokens: knobs,
  captions, a button and a package schema). The other examples are 3k to 9k tokens; read them by section.

## Graphics (IVG)

- Check every IVG instruction against `docs/IVG Documentation.md`; CushyLint does not validate IVG.
- Fixed vector art: an image from IVG (`image: { ivgFile: "name" }`). Drawings that follow variables: a `vector` view
  with `file:` and `bindings:`.
- IVG colors are normalized too, and `none` is transparent.

## Validate before you answer

Run these from the project root, with the SDK in `references/microtonic-scripts-sdk/`; from the SDK root, drop that
prefix. On Windows, use the `.cmd` and `.bat` versions.

- `.js` files: `references/microtonic-scripts-sdk/tools/validate-js.sh scripts`. Run it from the project root, or
  ESLint skips files outside the SDK.
- `.cushy` files: CushyLint on the package folder, with a trailing slash:
  `references/microtonic-scripts-sdk/CushyLint/CushyLint "$(pwd)/scripts/MyScript.mtscript/"`. If the package
  `.schema` has no `resources:` lines, add the resource folder as a second argument.
- `.ivg` files: `references/microtonic-scripts-sdk/tools/validate-static-ivg.sh "scripts/MyScript.mtscript"`, and
  render icons with `references/microtonic-scripts-sdk/tools/IVG2PNG/IVG2PNG` while designing them. On Windows that is
  `IVG2PNG.exe`; `IVG2PNG` without an extension is the macOS binary.
- Never claim code was tested inside Microtonic unless it was.
