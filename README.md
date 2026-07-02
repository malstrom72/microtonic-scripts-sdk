# Microtonic Scripts SDK

This repository contains all the necessary documentation and resources to create GUI scripts for Microtonic. The
scripting engine in Microtonic uses a proprietary JavaScript engine based on ECMAScript 3. The engine only works when
the Microtonic window is open. There is currently no way to create real-time scripts that process MIDI or audio. Apart
from these restrictions, a script can do pretty much anything with the data in Microtonic, and the user interface can
look and behave in any way you can imagine.

While script code is running, Microtonic's user interface will freeze. Scripts that run for more than 20 seconds are
suspended, and the user gets the option of aborting or continuing. Each Microtonic instance has a memory limit of around
64MB after garbage collection, and scripts that exceed it are terminated. Memory usage can grow faster than raw data size
suggests: the smallest allocated JavaScript value, such as a number, uses 16 bytes before any additional array, object,
or string storage overhead.

Disclaimer: many proprietary technologies, formats, and languages are involved in creating user interfaces for
Sonic Charge plugins. These technologies have evolved organically over time and will continue to do so in the future.
There is functional overlap and inconsistencies, and there is no guarantee that a script that works in the current
version of Microtonic will work in the next (even though good compatibility is something that we strive for at
Sonic Charge).

The documentation in this repository was written for Microtonic version 3.3.4 (build 1048).

## Prerequisites

For everyday scripting work — writing `.cushy` files, running CushyLint, and validating JavaScript — no extra tools are required beyond what is bundled in this repository. The CushyLint binaries (`PikaCmd`, `MakaronCmd`) are prebuilt for both macOS and Windows and will be used as-is.

**[Node.js](https://nodejs.org/)** is the only tool you may need to install, and only for two tasks:

- **JavaScript validation** (`tools/validate-js.sh`) — runs ESLint via `npx` to catch syntax and style errors in `.js` and `.mtscript` files. ESLint understands the ECMAScript 3 subset used by Microtonic's scripting engine, so it catches mistakes that would only surface at runtime inside the plugin.
- **IVGFontConverter** — converts TrueType/OpenType fonts to the `.ivgfont` format used by Cushy and IVG. Requires Node.js to run `IVGFontConverter.node.js`.

The following tools are only needed for **SDK maintenance** tasks, not for everyday scripting:

- **C++ compiler** (Xcode/`g++` on macOS, MSVC on Windows) — only needed if the prebuilt `IVG2PNG` binary in `tools/IVG2PNG/` cannot run on your platform (e.g. a Linux machine, or after an upstream update). The validation scripts auto-build from source in that case. `IVG2PNG` renders `.ivg` files to PNG for visual validation and is sourced from the upstream [IVG repository](https://github.com/malstrom72/IVG).
- **Python 3** — required to run `tools/bootstrap-docling.sh`, which installs [Docling](https://github.com/DS4SD/docling) into a local virtual environment. Docling is used for the one-off task of converting the Microtonic PDF user guide to Markdown (`tools/convert-user-guide.sh`).
- **Pandoc** — required to regenerate the checked-in HTML copies of Markdown reference docs.

## Technology Overview

Here is a brief list of the technologies used in Microtonic GUIs:

- _Cushy_: the layout engine and file format for describing layouts, based on _Numbstrict_ with _Makaron_ support.
- _ImpD_: a simple imperative computer language disguised as a data format, or the other way around.
- _IVG_ (Imperative Vector Graphics): a compact 2D vector format and renderer written in standard C++, based on _ImpD_
  and the _NuXPixels_ rasterizer.
- _Makaron_: a macro expansion syntax, used to make `.cushy` files easier to write. Sourced from the
  [Numbstrict repository](https://github.com/malstrom72/Numbstrict).
- _Numbstrict_: an object notation format similar to (but not compatible with) JSON, from the
  [Numbstrict repository](https://github.com/malstrom72/Numbstrict).
- _NuXJScript_: our JavaScript engine, fully ECMAScript 3 compliant with features from ECMAScript 5.
- _PikaScript_: our legacy script language used by older Microtonic scripts and offline tools.

## Repository Structure

- `agents`:
    - Agent instruction packages for AI assistants working with this SDK. See
      [`agents/microtonic-script-writer`](agents/microtonic-script-writer/).
      For a project setup and iteration workflow, see
      [`vibe-coding.md`](agents/microtonic-script-writer/vibe-coding.md).

- `CushyLint`:
    1. Command-line syntax checker for `.cushy` files.
    2. Contains [`cushy.schema`](CushyLint/cushy.schema), the official reference for the `.cushy` format.

- `docs`:
    - [Cushy Documentation](docs/Cushy%20Documentation.md)
    - [ImpD Documentation](docs/ImpD%20Documentation.md)
    - [IVG Documentation](docs/IVG%20Documentation.md)
    - [ivgfont Documentation](docs/ivgfont%20Documentation.md)
    - [Makaron Documentation](docs/Makaron%20Documentation.md)
    - [Microtonic JS Reference](docs/Microtonic%20JS%20Reference.md)
    - [Microtonic User Guide](docs/Microtonic%20User%20Guide.md)

- `IVG`: curated vendored snapshot of the IVG source, renderer tools, IVGFiddle output, IVGFontConverter, dependencies,
  and documentation used for the shipping Microtonic IVG implementation. The IVG/ImpD/ivgfont docs are mirrored into
  the top-level `docs/` by [`tools/sync-ivg-docs.sh`](tools/sync-ivg-docs.sh).

- `JSConsole.mtscript`: an interactive Javascript console for Microtonic.

- `legacy`: contains documentation for the legacy scripting engine (based on _PikaScript_).

- `tools`: utilities for maintaining generated documentation and repository support files.

- `tmLanguages`: syntax highlighting support for Sonic Charge formats and languages.

## Maintaining Documentation

Most reference documentation is edited as Markdown under `docs/`. Some files also have checked-in `.html` copies for
offline viewing. After editing a Markdown file with a matching HTML file, regenerate the HTML with:

```sh
tools/regenerate-doc-html.sh "docs/Microtonic JS Reference.md"
```

Run `tools/regenerate-doc-html.sh` with no arguments to regenerate every Markdown file in `docs/` that already has a
checked-in HTML target. The script uses `pandoc -s --no-highlight` so regenerated output stays close to the existing
plain code-block style.

### Cushy

_Cushy_ is the GUI / layout engine used in all Sonic Charge products. `.cushy` files define the layout of views and
configure how the user can interact with the plugin through _GUI variables_ and _GUI actions_. The format is based
on _Numbstrict_, the object notation format used in all Sonic Charge products. _Numbstrict_ is similar to JSON with
the following differences:

1. C-style comments are supported.
2. You use curly brackets (`{` and `}`) for both structures and arrays.
3. To differentiate empty structures from empty arrays, you may use this syntax: `{ : }`.
4. You may express integer values as hexadecimal numbers in this format: `0xABCD`.
5. Real values include infinity (`inf`) and the NaN value (`nan`).
6. Free-form text values without quotes are allowed in many cases.
7. You can use `\U` inside quoted strings to escape a 32-bit Unicode character, e.g., `\U0001F9FF`.

Here is an "outline" of the Cushy file structure: 

    bounds: { <left>, <top>, <width>, <height> }
    autoexecs: {
        ... actions to run on open, close, or regularly on a timer
    }
    transitions: {
        ... visual transition effects applied when this Cushy is opened or closed
    }
    translations: {
        ... special string translations used for this Cushy.
    }
    views: {
        ... view definitions
    }

In Cushy, it's often possible to write mathematical expressions where numerical constants are expected. In these places,
`$` may be used to insert the default value for the field. E.g., `updateRate: $*2` would set `updateRate` to twice the
default. For rectangles such as view bounds you can also use the following variables: `t`, `l`, `w`, `h`, `r`, `b`
for _top_, _left_, _width_, _height_, _right_ and _bottom_ of the default rectangle. E.g.
`bounds: { l+10,t+10,h-20,w-20 }`. The default rectangle for a view bound is the full bounding box of the parent view. 

The _Numbstrict_ format can be challenging to write correctly, especially when containing deeply nested hierarchical
views and actions. Therefore we created _CushyLint_, a command-line tool to check the syntax of a `.cushy` file against
the official "schema" specification. Simple run `CushyLint` (Mac) or `CushyLint.bat` (Windows) from a command-line
prompt with the single argument specifying a single `.cushy` file path to check or a directory if you want to check
multiple files. Directories must end with a slash (`/` or `\`).

The files are checked against the official `cushy.schema` file in the `CushyLint` directory, plus any other `.schema`
files existing in the directory next to the `.cushy` file that you are checking. The `cushy.schema` file is designed to
be readable and contains lots of comments, thus also serving as a kind of official reference documentation on `.cushy`
files, available view types, and built-in actions.

Furthermore, you can use macros when writing `.cushy` files for easier development and maintenance. See
[Makaron Documentation](docs/Makaron%20Documentation.md) for documentation on the macro expansion language we use. Macros
you write are expanded when `.cushy` files are loaded inside the plugin, before they are parsed.

For an orientation to the Cushy system — the mental model, coordinates and scaling, what is static versus dynamic, and a
map of where each part is documented — see [Cushy Documentation](docs/Cushy%20Documentation.md). See also
[`cushy.schema`](CushyLint/cushy.schema) and [Microtonic JS Reference](docs/Microtonic%20JS%20Reference.md) for more
information on how to write Cushy.

### IVGFiddle

Included in this distribution is a standalone .html application called _IVGFiddle_. You can run it simply by opening the
[`IVG/tools/ivgfiddle/output/ivgfiddle.html`](https://htmlpreview.github.io/?https://github.com/malstrom72/microtonic-scripts-sdk/blob/main/IVG/tools/ivgfiddle/output/ivgfiddle.html)
file in your favorite browser (Google Chrome). It will let you experiment with IVG code and see the graphical output in
real-time.

See [IVG Documentation](docs/IVG%20Documentation.md) for more information on IVG.

### IVG2PNG

The `IVG` directory is a curated vendored snapshot of the upstream [IVG repository](https://github.com/malstrom72/IVG). A prebuilt `IVG2PNG` binary for macOS and Windows is included in `tools/IVG2PNG/` so no compiler is needed for normal use.

To render all static `.ivg` resources in this SDK:

```sh
tools/validate-static-ivg.sh        # macOS / Linux
tools\validate-static-ivg.cmd       # Windows
```

The rendered PNGs are written to a temp directory by default (`/tmp/microtonic-static-ivg-validation` on macOS/Linux, `%TEMP%\microtonic-static-ivg-validation` on Windows). IVG files that depend on Cushy or GUI variables are reported as dynamic and skipped by this static renderer pass.

If the prebuilt binary cannot run on your platform, the scripts automatically rebuild `IVG2PNG` from source using `tools/build-ivg2png.sh` (macOS/Linux) or `tools\build-ivg2png.cmd` (Windows, requires MSVC).

### IVGFontConverter

Cushy (and IVG) uses a proprietary file format for fonts: `.ivgfont`. You can use _IVGFontConverter_ to convert
a _TrueType_ or _OpenType_ font to this format. To run, you must install [node.js](https://nodejs.org/en/). Then use
it like this:

    node IVG/tools/IVGFontConverter/IVGFontConverter.node.js <input> [ ?|-|<feature>[,<feature>,...] ] [ <charset>[,<charset>,...] ] > <output>
    
      ?          List all GSUB features
      -          No extra GSUB feature
      <feature>  Enable GSUB feature by [<script>.[<language>.]]<feature>
      <charset>  Convert Unicode characters [<hex>[-<hex>]] (default is ISO-8859-1)
    
    Example: node IVG/tools/IVGFontConverter/IVGFontConverter.node.js font.otf >font.ivgfont
    Example: node IVG/tools/IVGFontConverter/IVGFontConverter.node.js font.otf ss01 >font.ivgfont
             node IVG/tools/IVGFontConverter/IVGFontConverter.node.js font.ttf latn.ROM.locl,latn.ss01 >font.ivgfont
             node IVG/tools/IVGFontConverter/IVGFontConverter.node.js font.ttf - 0020-007f,a0-cf >font.ivgfont

See [ivgfont Documentation](docs/ivgfont%20Documentation.md) for details on the generated font format.

### JSConsole

A console (`JSConsole`) is available for developing scripts. It runs inside Microtonic and allows you to enter
JavaScript code interactively, see traces, reload all resources and see script performance (as frames per second).
Microtonic scripts do not have a step-through debugger or browser-style developer tools, so the normal debugging
workflow is to install and open `JSConsole.mtscript` and use `print("message")` from script code to write trace output
to the console. For blocking checkpoints or user-visible diagnostics, use `display("message")`, which shows a modal
alert and pauses script flow until the user dismisses it.

Install it by copying `JSConsole.mtscript` to the `Microtonic Scripts` folder. (You can easily find this folder by
choosing `Open Scripts Folder` from the "puzzle menu" in Microtonic.)

To copy this SDK's bridged JSConsole into a confirmed scripts folder:

```sh
node tools/install-jsconsole.js "/path/to/Microtonic Scripts"
```

With no argument, the helper prints the SDK source path and platform-specific target hints. The helper refuses to run
unless the source console contains the bridge commands, which avoids accidentally installing a plain JSConsole copy.

Microtonic caches resources while the GUI window is open, including JavaScript source files loaded by scripts. When
editing a GUI script, use the reload button at the top of the JSConsole window to flush cached resources and rebuild the
open GUI, or type `reload` in JSConsole. A normal reload reruns JavaScript files but keeps the current JavaScript engine
and existing globals alive. Shift-clicking the JSConsole reload button, or typing `reset`, performs a full JavaScript
engine reset, which clears memory and starts from a clean scripting environment. Closing the Microtonic GUI window also
destroys the entire JavaScript environment, so reopening the GUI starts with no previous script globals, like a full
reset.

### Development Scripts Folder

For quicker round-trips during development, keep a project-local copy of the entire `Microtonic Scripts` folder and
link Microtonic's scripts folder to it. See [Development Scripts Folder](docs/Development%20Scripts%20Folder.md) for
macOS and Windows commands, including re-linking an existing symlink and using macOS' administrator dialog for elevated
link steps.

On Windows the live folder may require elevation for every copy. In that case, consider the development-link workflow
before the first manual install so later edits are elevation-free.

### Syntax Highlighting

_TextMate Language Grammars_ are available for most of the custom languages and formats used by Sonic Charge. You find
them in the `tmLanguages` folder. We have tested them in [Sublime Text](https://www.sublimetext.com/) and
[Visual Studio Code](https://code.visualstudio.com/). Installation instructions:

- Sublime Text
    
  Use the menu `Preferences > Browse Packages...` to open `Packages` and copy the `soniccharge` folder into this folder.

- Visual Studio Code

  Copy the `soniccharge` folder into the `.vscode/extensions/` directory under your "home folder".

  - Windows: `%USERPROFILE%\.vscode\extensions`
  - Mac/Linux: `$HOME/.vscode/extensions`
