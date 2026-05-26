# Vibe-Coding Microtonic Scripts

This guide is for using an AI coding assistant with the Microtonic Scripts SDK.
The SDK should be treated as the source of truth for documentation, examples,
schemas, resources, and validation tools.

## Starter Prompt For Codex, Claude, Or ChatGPT

Copy this into a fresh agent session to bootstrap a Microtonic script project:

```text
I want to build Sonic Charge Microtonic scripts with this SDK:

https://github.com/malstrom72/microtonic-scripts-sdk

Before writing any script logic, clone the latest SDK from GitHub into:

references/microtonic-scripts-sdk/

Then read and follow the latest project bootstrap instructions in:

references/microtonic-scripts-sdk/agents/microtonic-script-writer/instructions.md

Set up the project exactly as those instructions describe, then wait for my
script idea.
```

## Recommended Project Layout

Keep your own scripts in a separate project and use this SDK as a reference:

```text
my-microtonic-scripts/
  AGENTS.md
  scripts/
    MyUtility.js
    MyTool.mtscript/
      MyTool.js
      MyTool_main.cushy
      MyTool_main.js
      MyTool_main.schema
  references/
    microtonic-scripts-sdk/
```

The assistant should write project scripts under your own `scripts/` folder, not
inside the SDK, unless you are deliberately contributing SDK examples.

## Installing Scripts In Microtonic

Microtonic finds runnable scripts in the `Microtonic Scripts` folder. Open it
from Microtonic with `Open Scripts Folder` in the puzzle menu.

Install classic GUI-less scripts as single `.js` files directly in that folder:

```text
Microtonic Scripts/
  MyUtility.js
```

Install GUI scripts as `.mtscript` package folders:

```text
Microtonic Scripts/
  MyTool.mtscript/
    MyTool.js
    MyTool_main.cushy
    MyTool_main.js
    MyTool_main.schema
```

Inside Microtonic, `DIRS.SCRIPTS` is the absolute path to this folder and is the
default directory used by relative script file operations.

## Link A Working Scripts Folder During Development

For quicker round-trips, keep a project-local copy of the entire `Microtonic
Scripts` folder and link Microtonic's scripts folder to it. See
[`README.md`](../../README.md#development-scripts-folder) for macOS and Windows
commands.

## Minimal Project `AGENTS.md`

In your project root, add an `AGENTS.md` like this:

```md
# Project Agent Instructions

For Microtonic scripting work, follow:

`references/microtonic-scripts-sdk/agents/microtonic-script-writer/instructions.md`

Use the SDK checkout as the source of truth for docs, examples, schemas,
resources, and CushyLint validation.
```

Claude and ChatGPT can use the same starter prompt. The SDK also includes thin
tool-specific wrappers under `agents/microtonic-script-writer/`, but the shared
`instructions.md` file is the source of truth.

## Choosing Script Type

Use a classic GUI-less `.js` script for direct utilities:

- Pattern generation.
- Patch manipulation.
- Channel operations.
- One-shot randomization or cleanup scripts.

Use a `.mtscript` package for GUI tools:

- Custom windows.
- Cushy controls.
- Reusable tools with knobs, buttons, sliders, or persistent UI state.
- Workflows modeled on the SDK `.mtscript` examples.

## Ask The Assistant To Ground Its Work

Good prompts are explicit about SDK grounding:

```text
Create a GUI-less Microtonic JavaScript script. Use the SDK JS reference and
examples as source of truth. Keep it ECMAScript 3-compatible.
```

```text
Create a .mtscript GUI tool. Inspect the closest SDK example first, use
Microtonic Resources for schema/resources, and run CushyLint before finishing.
```

## Validation

For `.mtscript` packages with `.cushy` files, validate with CushyLint from the
SDK root. Use absolute paths because the CushyLint wrapper runs from inside the
`CushyLint` directory:

```sh
references/microtonic-scripts-sdk/CushyLint/CushyLint \
  "$(pwd)/scripts/MyTool.mtscript/MyTool_main.cushy" \
  "$(pwd)/references/microtonic-scripts-sdk/Microtonic Resources"
```

A CushyLint pass means the Cushy syntax and schema references are valid. It does
not prove the script has been tested inside Microtonic.

## Reloading While Iterating

Microtonic caches resources while the GUI window is open, including JavaScript
source files loaded by scripts.

When editing GUI scripts:

- Click the reload button at the top of JSConsole to flush cached resources and
  rebuild the open GUI.
- Close and reopen the GUI window to force a reload.
- Shift-click the JSConsole reload button for a full JavaScript engine reset,
  clearing memory and testing from a clean scripting environment.

End-user zoom scale changes also force GUI reloads so graphics resources and
exact layout coordinates can be rescaled. GUI scripts must survive their
JavaScript files being run again.

## JavaScript Style

Microtonic uses a proprietary JavaScript environment based on ECMAScript 3 with
some ECMAScript 5 support and SDK-provided utilities.

Prefer:

- `var`, function declarations, and plain objects.
- One script-named global state object when persistent script-local state is
  needed.
- A main IIFE for local scope in GUI-less scripts.
- Rerun-safe setup for GUI scripts.

Do not use:

- `let`, `const`, arrow functions, classes, destructuring, modules, promises,
  async functions, and template literals.

## Script State Pattern

If a script needs persistent state while the Microtonic window remains open, use
one global named after the script and initialize its fields clearly:

```js
if (!MyScript) {
    var MyScript = {
        inited: false,
        cache: {}
    };
}

(function () {
    if (!MyScript.inited) {
        MyScript.cache = {};
        MyScript.inited = true;
    }

    // Script body here.
}());
```

Avoid scattering globals or creating aliases that make state harder to trace.
