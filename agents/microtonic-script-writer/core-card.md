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
  at the top of each file that uses it. Keep only changing state in that object; constants, methods and GUI variable
  handlers go outside the guard and are reassigned on every run.
- A reload must leave everything the user sees unchanged: startup code must not re-randomize, re-capture or write to
  the document.
- In JSConsole, `reload` reruns the scripts and keeps globals; `reset` starts a clean engine. When the state object's
  shape changes during development, use `reset` instead of writing migration code.

## Changing the document

- Call `saveUndo(label)` immediately before each write. With `collapse` `true` and the same label as the last undo
  entry, the write joins that entry, even from a separate gesture; for one undo step per drag, see `saveUndo` in the
  JS Reference.
- Before `setElement('preset', p)`, set `p.modified = true`, as the examples do.

## Code and comments

- Comments describe the code as it is now: no revision history ("renamed from"), version counters or "verified"
  claims.
- Write each rule once. When two functions need the same decision, call one shared function instead of keeping two
  copies in sync.

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
- Every script action the `.cushy` names, `autoexecs` included, must be listed in the package `.schema`. Remove the
  entry when you remove the action.
- Lay out from a few `@define`s for margins, gutters and columns, not hand-typed coordinates. Before calling a GUI
  done, look at a screenshot of it: every caption fits its longest value, and edges and gaps are consistent. Button
  labels say what the button does; explanations go in the `hint`.
- To start a GUI package, copy `examples/MacroTweak.mtscript`, the smallest example (about 2.5k tokens: knobs,
  captions, a button and a package schema). The other examples are 3k to 9k tokens; open only the parts you need.

## Graphics (IVG)

- Check every IVG instruction against `docs/IVG Documentation.md`; CushyLint does not validate IVG.
- Fixed vector art: an image from IVG (`image: { ivgFile: "name" }`). Drawings that follow variables: a `vector` view
  with `file:` and `bindings:`.
- IVG colors are normalized too, and `none` is transparent.

## Validate before you answer

Run every check below again after each round of edits, not only the first time; work is not done while a check fails
or was skipped. Run them from the project root, with the SDK in `references/microtonic-scripts-sdk/`; from the SDK
root, drop that prefix. On Windows, use the `.cmd` and `.bat` versions.

- `.js` files: `references/microtonic-scripts-sdk/tools/validate-js.sh scripts`. Run it from the project root, or
  ESLint skips files outside the SDK.
- `.cushy` files: CushyLint on the package folder, with a trailing slash:
  `references/microtonic-scripts-sdk/CushyLint/CushyLint "$(pwd)/scripts/MyScript.mtscript/"`. If the package
  `.schema` has no `resources:` lines, add the resource folder as a second argument. On Windows, Git Bash included,
  call `CushyLint/CushyLint.bat` with the same argument, forward slashes and a trailing `/`: the file without an
  extension is the Unix script and tries to compile with g++.
- `.ivg` files: `references/microtonic-scripts-sdk/tools/validate-static-ivg.sh "scripts/MyScript.mtscript"`, and
  render icons with `references/microtonic-scripts-sdk/tools/IVG2PNG/IVG2PNG` while designing them. On Windows that is
  `IVG2PNG.exe`; `IVG2PNG` without an extension is the macOS binary.
- Never claim code was tested inside Microtonic unless it was.
