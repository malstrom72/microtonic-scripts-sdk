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

For the recommended external project layout, run the SDK wrapper from the
project root and pass the user script folder:

```sh
references/microtonic-scripts-sdk/tools/validate-js.sh scripts
```

The wrapper keeps the caller's working directory when paths are supplied, so
files outside the SDK checkout are validated instead of being ignored by
ESLint's flat-config base path. With no arguments it changes to the SDK root and
validates `JSConsole.mtscript` plus `examples`.

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

## Package Schema Header

Every `.mtscript` package should include a package-local `.schema` file next to
the main `.cushy` file. The schema should declare the Microtonic schema include
and the resource roots needed by both CushyLint and editor integrations. Paths
are relative to the `.schema` file:

```schema
include:   <rel>/Microtonic Resources/microtonic.schema
resources: ../
resources: <rel>/Microtonic Resources

<actions> = <microtonicAction>
        | <yourCustomActions>

<view> = <microtonicView>
```

For packages inside this SDK's `examples/` directory, `<rel>` is usually
`../..`. For a package in a normal external project such as
`scripts/MyTool.mtscript/`, `<rel>` should point from
`scripts/MyTool.mtscript/MyTool_main.schema` to the SDK checkout, for example
`../../references/microtonic-scripts-sdk`.

The same rule applies to SDK packages copied into the external project's
`scripts/` folder. For example, if `JSConsole.mtscript` is copied from
`references/microtonic-scripts-sdk/JSConsole.mtscript` to
`scripts/JSConsole.mtscript`, rewrite the copied
`scripts/JSConsole.mtscript/JSConsole_main.schema` header to:

```schema
include:   ../../references/microtonic-scripts-sdk/Microtonic Resources/microtonic.schema
resources: ../
resources: ../../references/microtonic-scripts-sdk/Microtonic Resources
```

Do not make `scripts/Microtonic Resources/` to satisfy the copied schema's
original SDK-relative paths. That directory should not exist in the project
scripts folder; the correct fix is to point the copied `.schema` file back to
the SDK checkout under `references/`.

The `include:` line brings in Microtonic-specific view and action definitions,
including `closeCushy`. The `resources: ../` line lets file references such as
`file: "@scriptRoot/MyGraphic"` resolve through the package's parent directory.
The `resources: <rel>/Microtonic Resources` line lets `@include
scriptSupport.makaron` and built-in resources such as `rectDropShadow` resolve.

Declare these resource roots in the schema, not only through the second
CushyLint command-line argument. CushyLint can use the CLI resource argument,
but editor integrations need the schema's own `resources:` lines to resolve
includes and file references consistently. A self-contained schema like the
example above also lints cleanly without the second CLI resource argument.

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

For tiny glyphs or cell-sized artwork, render a scaled test version that shows
the glyph in context. Put `scale N` after `bounds` and size the bounds
proportionally, for example `bounds 0,0,288,64` plus `scale 4` for a 72×16
layout. Stroke widths scale too, so use this to judge proportions rather than
exact pixel thickness.

When testing transparent or stroke-only glyphs, pass a realistic background
color to the renderer:

```sh
IVG/output/IVG2PNG --fonts IVG/fonts --background "#202020" "path/to/MyGlyph_test.ivg" "/tmp/MyGlyph_test.png"
```

For comparing variants, render each variant as its own `_test.ivg` strip with
neighboring cells or surrounding UI color included. Inspecting the generated PNG
inside the assistant only verifies it for the assistant; if the user needs to
see the result, open or share the rendered PNG explicitly.

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
