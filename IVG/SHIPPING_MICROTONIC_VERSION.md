# Vendored IVG Snapshot

This directory is a curated vendored copy of the IVG source tree. It follows the IVG-2
freeze on IVG main (the IVG-1/2 line that Microtonic and Synplant use, not IVG-3), so
the SDK validates scripts with the renderer the next product releases use.

Imported from `malstrom72/IVG` commit:

```text
651fdabd4384007738d66266898d55b77242babe
```

The pinned freeze may be newer than the IVG in released Microtonic and Synplant builds.
Differences are expected to be small, such as a few anti-aliased pixels along curve edges.
Move the pin only to a new IVG-2 freeze from IVG main, and update both script SDKs together.

## Curated Subset

This is intentionally not a full copy of the upstream Git repository. It keeps
only the pieces needed to document and verify the IVG implementation:

- Core IVG and ImpD sources in `src/`.
- Required `IVG2PNG` dependencies in `externals/NuX`, `externals/libpng`, and
  `externals/zlib`.
- The `IVG2PNG` source and build helper in `tools/`.
- The generated standalone IVGFiddle output in `tools/ivgfiddle/output/`.
- The bundled IVGFontConverter command-line tool in `tools/IVGFontConverter/`.
- The `svg2ivg` converter in `tools/svg2ivg.js` and its supported-feature
  reference in `docs/SVG Support.md`.
- Core documentation and generated example images in `docs/`.
- Upstream license and README.

Upstream CI files, IDE projects, fuzzing artifacts, full test corpora,
IVGFiddle source/build scaffolding, font bundles, and unrelated helper projects
were omitted to keep this SDK snapshot small and focused. The upstream
`tests/svg/` corpus remains omitted; the converter and its feature reference are
included.

## Local Patches

- `tools/ivgfiddle/output/setupModule.js` waits for the modularized Emscripten
  runtime instance before initializing the editor and running IVG. This avoids
  calling `runIVG()` while the global `Module` value still points at the
  generated factory function, which can otherwise fail with
  `Module.lengthBytesUTF8 is not a function`.
- `README.md` does not link to the omitted `tests/svg/` sample corpus or the omitted
  `docs/CodingStyle.md`.
