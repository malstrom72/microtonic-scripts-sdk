# Microtonic Scripts SDK Agent Instructions

For Microtonic scripting work, follow the package in
[`agents/microtonic-script-writer/`](agents/microtonic-script-writer/).

Start with [`instructions.md`](agents/microtonic-script-writer/instructions.md), then use:

- [`source-map.md`](agents/microtonic-script-writer/source-map.md) for authoritative references.
- [`validation.md`](agents/microtonic-script-writer/validation.md) for the SDK tooling: CushyLint,
  live-bridge checks, and rendering `.ivg` files to PNG with IVG2PNG.
- [`packaging.md`](agents/microtonic-script-writer/packaging.md) for classic scripts vs `.mtscript` packages.
- [`cushy-notes.md`](agents/microtonic-script-writer/cushy-notes.md) for Cushy details, gotchas, tricks, and tips.
- [`vibe-coding.md`](agents/microtonic-script-writer/vibe-coding.md) for recommended project setup and AI-assisted workflow.

Working on a `.ivg` icon or vector asset? Render it with `tools/IVG2PNG/IVG2PNG` while designing it,
not only as a final validation step. Do not approximate the glyph in SVG or rely on generic image
tools to preview it; ImageMagick installations without a reliable SVG delegate can silently drop
stroked paths. If the source is SVG, use the vendored `IVG/tools/svg2ivg.js` only as a draft
generator, then tidy the IVG and render the result with IVG2PNG.
