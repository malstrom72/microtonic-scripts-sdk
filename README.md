# Microtonic Scripts SDK

This repository contains all the necessary documentation and resources to create GUI scripts for Microtonic. The
scripting engine in Microtonic uses a proprietary JavaScript engine based on ECMAScript 3. The engine only works when
the Microtonic window is open. There is currently no way to create real-time scripts that process MIDI or audio. Apart
from these restrictions, a script can do pretty much anything with the data in Microtonic, and the user interface can
look and behave in any way you can imagine.

Disclaimer: many proprietary technologies, formats, and languages are involved in creating user interfaces for
Sonic Charge plugins. These technologies have evolved organically over time and will continue to do so in the future.
There is functional overlap and inconsistencies, and there is no guarantee that a script that works in the current
version of Microtonic will work in the next (even though good compatibility is something that we strive for at
Sonic Charge).

The documentation in this repository was written for Microtonic version 3.3.4 (build 1048).

## Technology Overview

Here is a brief list of the technologies used in Microtonic GUIs:

- _Cushy_: the layout engine and file format for describing layouts, based on Numbstrict with Makaron support.
- _ImpD_: a simple imperative computer language disguised as a data format, or the other way around.
- _IVG_ (Imperative Vector Graphics): a language and file-format for 2D vector graphics, based on _ImpD_.
- _Makaron_: a macro expansion syntax, used to make `.cushy` files easier to write.
- _Numbstrict_: an object notation format similar to (but not compatible with) JSON.
- _NuXJScript_: our JavaScript engine, fully EcmaScript 3 compliant with features from EcmaScript 5.
- _PikaScript_: our legacy script language used by older Microtonic scripts and offline tools.

## Repository Structure

- `CushyLint`:
	1. Command-line syntax checker for `.cushy` files.
	2. Contains [`cushy.schema`](CushyLint/cushy.schema), the official reference for the `.cushy` format.

- `docs`:
	- [ImpD Documentation](docs/ImpD%20Documentation.md)
	- [IVG Documentation](docs/IVG%20Documentation.md)
	- [Makaron Documentation](docs/Makaron%20Documentation.md)
	- [Microtonic JS Reference](docs/Microtonic%20JS%20Reference.md)

- `ivgfiddle`: a browser "playground" for experimenting with _IVG_ (compiled with _emscripten_).

- `IVGFontConverter`: a converter from `.ttf` and `.otf` to `.ivgfont` (requires _node.js_).

- `JSConsole.mtscript`: an interactive Javascript console for Microtonic.

- `legacy`: contains documentation for the legacy scripting engine (based on _PikaScript_).

- `tmLanguages`: syntax highlighting support for Sonic Charge formats and languages.

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

In Cushy, it's often possible to write mathematical expressions where numeric constants are expected. In these places,
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

See [`cushy.schema`](CushyLint/cushy.schema) and [Microtonic JS Reference](docs/Microtonic%20JS%20Reference.md) for more
information on how to write Cushy.

### IVGFiddle

Included in this distribution is a standalone .html application called _IVGFiddle_. You can run it simply by opening the
`ivgfiddle.html` file in your favorite browser (Google Chrome). It will let you experiment with IVG code and see the
graphical output in real-time.

See [IVG Documentation](docs/IVG%20Documentation.md) for more information on IVG.

### IVGFontConverter

Cushy (and IVG) uses a proprietary file format for fonts: `.ivgfont`. You can use _IVGFontConverter_ to convert
a _TrueType_ or _OpenType_ font to this format. To run, you must install [node.js](https://nodejs.org/en/). Then use
it like this:

	node IVGFontConverter.node.js <input> [ ?|-|<feature>[,<feature>,...] ] [ <charset>[,<charset>,...] ] > <output>
 	
	  ?          List all GSUB features
	  -          No extra GSUB feature
	  <feature>  Enable GSUB feature by [<script>.[<language>.]]<feature>
	  <charset>  Convert Unicode characters [<hex>[-<hex>]] (default is ISO-8859-1)
 	
	Example: node IVGFontConverter.node.js font.otf >font.ivgfont
	Example: node IVGFontConverter.node.js font.otf ss01 >font.ivgfont
	         node IVGFontConverter.node.js font.ttf latn.ROM.locl,latn.ss01 >font.ivgfont
	         node IVGFontConverter.node.js font.ttf - 0020-007f,a0-cf >font.ivgfont

### JSConsole

A console (`JSConsole`) is available for developing scripts. It runs inside Microtonic and allows you to enter
JavaScript code interactively, see traces, reload all resources and see script performance (as frames per second).

Install it by copying `JSConsole.mtscript` to the `Microtonic Scripts` folder. (You can easily find this folder by
choosing `Open Scripts Folder` from the "puzzle menu" in Microtonic.)

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
