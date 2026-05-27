# Shipping Microtonic IVG Snapshot

This directory is a curated vendored copy of the IVG source tree used as the
reference for the shipping Microtonic IVG implementation.

Imported from `malstrom72/IVG` commit:

```text
c016809d2f120e74eca4d64d61b134ee8dfb563d
```

Do not update this directory to latest upstream IVG unless Microtonic's shipped
IVG implementation is updated too.

## Curated Subset

This is intentionally not a full copy of the upstream Git repository. It keeps
only the pieces needed to document and verify the shipped IVG implementation:

- Core IVG and ImpD sources in `src/`.
- Required `IVG2PNG` dependencies in `externals/NuX`, `externals/libpng`, and
  `externals/zlib`.
- The `IVG2PNG` source and build helper in `tools/`.
- The generated standalone IVGFiddle output in `tools/ivgfiddle/output/`.
- The bundled IVGFontConverter command-line tool in `tools/IVGFontConverter/`.
- Core documentation and generated example images in `docs/`.
- Upstream license and README.

Upstream CI files, IDE projects, fuzzing artifacts, full test corpora,
IVGFiddle source/build scaffolding, SVG conversion tools, font bundles, and
unrelated helper projects were omitted to keep this SDK snapshot small and
focused.

## Local Patches

- `tools/ivgfiddle/output/setupModule.js` waits for the modularized Emscripten
  runtime instance before initializing the editor and running IVG. This avoids
  calling `runIVG()` while the global `Module` value still points at the
  generated factory function, which can otherwise fail with
  `Module.lengthBytesUTF8 is not a function`.
