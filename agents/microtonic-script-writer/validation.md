# Validation

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
- Inline Cushy vector snippets still need separate review or extraction before
  `IVG2PNG` can render them.

## Testing Dynamic IVG Designs

Dynamic IVG files (those that use `$variable` references bound from Cushy) are
skipped by the static renderer. To verify a dynamic IVG design before running
it in Microtonic, create a companion `_test.ivg` file with all `$variable`
references replaced by representative hardcoded values, and render it directly:

```sh
IVG/output/IVG2PNG "path/to/MyFile_test.ivg" "/tmp/MyFile_test.png"
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
