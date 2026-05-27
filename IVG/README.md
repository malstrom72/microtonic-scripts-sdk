# IVG

IVG (Imperative Vector Graphics) is a compact, dependency-free 2D vector format and renderer
written in standard C++. Graphics are described using the small imperative language **ImpD**, which
supports variables and control flow for defining procedural images.

The renderer is built on the included **NuXPixels** rasterizer and provides high-quality
gamma-correct anti-aliasing. The format is concise and designed for both hand-written and generated
vector graphics.

## Features

- Graphics are described using **ImpD**, a minimal imperative language for image construction.  
- Built-in **NuXPixels** rasterizer provides high-quality, gamma-correcting anti-aliasing.  
- Renderer written in **portable, dependency-free C++**, with no reliance on third-party libraries.  
- Supports **paths, shapes, images, text, styling, transformations**, and nesting.
- SVG path commands are fully supported.
- Simple `.ivgfont` format for embedded vector fonts, converted from standard font formats.
- **Standalone HTML editor** (IVGFiddle) for live editing and previewing IVG code.  
- Built-in **test suite** with regression output compared to golden PNGs.  
- Self-contained format and tools designed for experimentation and integration.

## Mini Examples

Here are a couple of quick snippets written in ImpD. Each one produces the image right below it.

```ImpD
format IVG-2 requires:ImpD-1
bounds 0,0,300,80

fill #406080
RECT 60,10,180,60

fill #80C0E0
pen #C0E0E0
RECT 80,20,140,40 rounded:30,20
```
![Rounded rectangle example](docs/images/rectExample.png)

```ImpD
WIPE #E0E0E0

// Fill a red circle.
fill #802020
ELLIPSE 150,100,80

// Outline a flat ellipse, rotated 10 degrees clockwise.
fill none
pen #802020 width:5
rotate 10 anchor:150,100
ELLIPSE 150,100,140,30
```
![Ellipse example](docs/images/ellipseExample.png)

## Prerequisites

You will need a standard C++ compiler.

- On **macOS** or **Linux**, use `g++` or `clang++`.
- On **Windows**, the build requires Microsoft Visual C++. Any version from Visual Studio 2008
	(VC9.0) onward should work. The build scripts locate the compiler automatically using
	`vswhere.exe`, falling back to known versions if needed.
- Node.js 16+ is required for the SVG conversion tests. If Node.js is unavailable, set
	`SKIP_SVG=1` before running `./build.sh` to skip them.

## Build & Test

Run `bash build.sh` (or `build.cmd` on Windows) from the repository root. This builds the renderer
tools and runs the regression tests.

Both the **beta** and **release** targets are compiled with optimizations enabled. The **beta**
build additionally has assertions turned on.

On **macOS** and **Linux**, the build script also compiles a SIMD-enabled variant using SSE or NEON
instructions when available.

## Helper Scripts

- `build.sh` / `build.cmd` – build both the **beta** and **release** targets and run all tests
- `tools/updateIVGTests.sh` / `.cmd` – regenerate golden PNGs from all `.ivg` test files
- `tools/updateDocumentation.sh` – rebuild HTML documentation using Pandoc and PikaScript
	(Mac / Linux only)

## svg2ivg

`svg2ivg` converts a subset of SVG into IVG's ImpD language. The Node.js script
resides at `tools/svg2ivg.js` and is invoked as:

```
node tools/svg2ivg.js input.svg [output.ivg] [defaultWidth,defaultHeight]
```

Omitting `output.ivg` prints the converted ImpD to stdout. See
[docs/SVG Support.md](docs/SVG%20Support.md) for supported features and
`tests/svg` for sample inputs.

## IVGFiddle

Included in this repository is a standalone HTML application called **IVGFiddle**. You can open it
in your browser to write IVG code and see the output rendered in real time.

- File location: `tools/ivgfiddle/output/ivgfiddle.html`

You can also try it live without cloning the repo:  
[IVGFiddle](https://htmlpreview.github.io/?https://github.com/malstrom72/IVG/blob/main/tools/ivgfiddle/output/ivgfiddle.html)

## Fonts

The repository includes several `.ivgfont` files converted from the following open-source fonts:

- Source Sans Pro
- Source Serif Pro
- Source Code Pro

These fonts are licensed under the [SIL Open Font License 1.1](https://scripts.sil.org/OFL). The
full license text is available in `fonts/OFL.txt`.

See [ivgfont Documentation](docs/ivgfont%20Documentation.md) for details on the font format and
how to convert other fonts.

Font parsing for the converter relies on the [Typr.ts](https://github.com/fredli74/Typr.ts) library
by Fredrik Lidström, a TypeScript wrapper around the legacy
[Typr.js](https://github.com/photopea/Typr.js) from 2020, both MIT-licensed.

## Documentation

- [IVG Documentation](docs/IVG%20Documentation.md)
- [ImpD Documentation](docs/ImpD%20Documentation.md)
- [NuXPixels Documentation](docs/NuXPixels%20Documentation.md)
- [ivgfont Documentation](docs/ivgfont%20Documentation.md)
- [Developer Guide](docs/Developer%20Guide.md)
- [Fuzzing](docs/Fuzzing.md)

## AI Usage

AI tools (such as OpenAI Codex) have been used to assist with documentation, code comments, test
generation, and repetitive edits. All core source code, except svg2ivg.js, has been written and
refined by hand over many years. AI was used extensively to complete svg2ivg.js (which began as
a "PikaScript").

## License

This project is released under the [BSD 2-Clause License](LICENSE).
Fonts are distributed under the [SIL Open Font License 1.1](https://scripts.sil.org/OFL); see
`fonts/OFL.txt` for the full license text.
