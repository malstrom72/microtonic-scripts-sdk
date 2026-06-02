# Validation

## JavaScript Syntax

Microtonic's JavaScript engine is based on
[`NuXJS`](https://github.com/malstrom72/NuXJS), an ECMAScript 3 engine with a
small set of runtime additions such as string indexing, `JSON`, `Object.assign`,
and `Date.now`. Generated `.js` files should therefore be checked as
Microtonic scripts, not as modern browser or Node.js code.

The simplest lightweight check is `tools/validate-js.sh`, which runs ESLint with
the Microtonic config in `tools/eslint.microtonic.config.mjs`. The config uses
ECMAScript 5 script syntax. Microtonic is ES3-based, but the shipped examples use
some ES5-era syntax that NuXJS accepts in practice, such as trailing commas in
object literals and reserved words as property names. ESLint with `ecmaVersion:
5` is therefore a better practical syntax gate than `ecmaVersion: 3`: it rejects
modern ES6+ syntax while accepting the syntax style already used by this SDK.

Validate the built-in examples and JSConsole package with:

```sh
tools/validate-js.sh
```

Validate a specific script package, directory, or file with:

```sh
tools/validate-js.sh path/to/MyScript.mtscript
```

This catches modern syntax such as `let`, `const`, arrow functions, classes,
template literals, destructuring, modules, and async functions. It is still only
a syntax check. It does not prove that a script behaves correctly inside
Microtonic, and it may not catch every NuXJS-specific runtime difference. For an
exact engine check, run the code in Microtonic with `JSConsole.mtscript` or
build and test against the NuXJS repository directly.

Use the local `CushyLint` tool in this repository for `.cushy` validation.

## Single File

On Unix-like systems:

```sh
CushyLint/CushyLint "$(pwd)/<path-to-file.cushy>" "$(pwd)/Microtonic Resources"
```

On Windows:

```bat
CushyLint\CushyLint.bat "%cd%\<path-to-file.cushy>" "%cd%\Microtonic Resources"
```

The second argument is the Microtonic resource directory. Use an absolute path
because the CushyLint wrapper runs from the `CushyLint` directory internally.
The resource directory contains `microtonic.schema`, Makaron support files, and
GUI resources needed by Microtonic `.mtscript` packages.

## Directory

Directories must end with a slash:

```sh
CushyLint/CushyLint "$(pwd)/examples/" "$(pwd)/Microtonic Resources"
```

## Expected Practice

- Run CushyLint before returning a generated `.mtscript` package that includes a
  `.cushy` file whenever the workspace can execute the validator.
- If validation fails, fix unsupported `.cushy` syntax, missing resources,
  schema path problems, or invented actions/views before changing the script
  concept.
- Do not treat a CushyLint pass as proof that the script works inside
  Microtonic. It validates Cushy syntax and schema compatibility, not runtime
  behavior inside the plugin.
- CushyLint does not validate or rasterize IVG drawing code. Check IVG syntax
  and behavior against `docs/IVG Documentation.md` and the vendored `IVG/`
  source when exact shipped behavior matters.
- For static `.ivg` files, build and run the vendored `IVG2PNG` renderer:

  ```sh
  tools/validate-static-ivg.sh
  ```

  This renders SDK `.ivg` resources to PNG files under
  `/tmp/microtonic-static-ivg-validation` by default and fails if any static
  IVG cannot be parsed or rasterized. IVG files that depend on Cushy or GUI
  variables are reported as dynamic and skipped by this static renderer pass.
  The validator passes `--fonts IVG/fonts` to `IVG2PNG`, so static IVG files can
  use the bundled external font faces `sans-serif`, `serif`, and `monospace`.
- Inline Cushy vector snippets still need separate review or extraction before
  `IVG2PNG` can render them.

## Testing Dynamic IVG Designs

Dynamic IVG files (those that use `$variable` references bound from Cushy) are
skipped by the static renderer. To verify a dynamic IVG design before running
it in Microtonic, create a companion `_test.ivg` file with all `$variable`
references replaced by representative hardcoded values, and render it directly:

```sh
IVG/output/IVG2PNG --fonts IVG/fonts "path/to/MyFile_test.ivg" "/tmp/MyFile_test.png"
```

The test IVG must open with a `bounds` declaration so the renderer knows the
canvas size:

```
format IVG-2 requires:IMPD-1
bounds 0,0,300,300
```

Colors in IVG are ARGB pre-multiplied (`#AARRGGBB`). All of R, G, B must be
≤ A — the renderer will error on invalid values. Fully opaque 6-digit colors
(`#RRGGBB`) are always safe.

Keep `_test.ivg` files out of the shipped package (`.gitignore` or delete after
verification).

IVG text rendering always requires an explicit font face before drawing text.
For local render tests, pass `--fonts IVG/fonts` when using one of the bundled
external font faces:

```
font sans-serif size:18 color:white
TEXT at:20,40 "Label"
```

The external font names are the `.ivgfont` filenames without the extension.
Use printable ASCII unless you have checked that the chosen `.ivgfont` includes
the needed glyphs.
