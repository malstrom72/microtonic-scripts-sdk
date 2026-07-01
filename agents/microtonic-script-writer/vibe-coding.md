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

## Bootstrap Order

When bootstrapping an empty project, keep the user-facing sequence clear:

1. Create the project scaffold: `AGENTS.md`, `.mcp.json` for Claude Code
   compatibility, `scripts/`, and `references/microtonic-scripts-sdk/`.
2. Identify the exact live scripts folder first: prefer `DIRS.SCRIPTS` via the
   bridge, or have the user open *Open Scripts Folder*. If the standard macOS
   path is missing and `DIRS.SCRIPTS` cannot be confirmed, stop and ask before
   relinking anything. Then ask whether to link Microtonic's live scripts folder
   to project `scripts/`. First verify whether that exact live folder is already
   a symlink or junction to another workspace. Do not call bootstrap complete
   while this decision is pending.
3. Install the SDK `JSConsole.mtscript` into the folder Microtonic will read.
   If it is copied into project `scripts/`, fix the copied schema paths.
4. Register the bridge MCP server for the active assistant.
5. If restart/reload is needed, stop at a handoff checklist and exact resume
   prompt for the bridge smoke test.
6. Only after bridge setup is verified, handed off, or explicitly skipped, show
   the future CushyLint command and wait for the user's script idea.

Do not lead with CushyLint while a live scripts-folder decision or bridge
restart/smoke-test handoff is still pending.

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
`DIRS.SCRIPTS` / *Open Scripts Folder* is authoritative. If a filesystem search
finds a different similarly named folder under Sonic Charge's Application
Support directory, ignore it unless it is exactly the folder Microtonic
reported. Do not relink a similarly named folder or local artifact just because
the standard folder is missing.

## Link A Working Scripts Folder During Development

For quicker round-trips, keep a project-local copy of the entire `Microtonic
Scripts` folder and link Microtonic's scripts folder to it. See
[`README.md`](../../README.md#development-scripts-folder) for macOS and Windows
commands.

An assistant bootstrapping a project should ask before doing this. Linking the
folder changes Microtonic's live scripts installation, moves the original folder
aside as a backup, and may require elevated permissions. Linking is not an
alternative to installing the SDK `JSConsole.mtscript`; it only decides whether
JSConsole should be copied into the live `Microtonic Scripts` folder directly or
into project `scripts/` after Microtonic's scripts folder has been linked there.

Before treating project `scripts/` as live, verify the exact current live
scripts folder target. On macOS, inspect that exact folder with `ls -ld` and
`readlink`. If it already links to a different workspace, do not write through
that link. Ask whether to relink it to this project, copy the package to the
current live folder, or let the user deploy manually.

On macOS, if elevated permission blocks the link step, the assistant should ask
for approval and use `osascript` with administrator privileges to complete the
operation instead of only printing `sudo` commands for the user to run.

When copying this SDK's `JSConsole.mtscript` into project `scripts/`, also fix
the copied `scripts/JSConsole.mtscript/JSConsole_main.schema` paths so they
point back to the SDK checkout:

```schema
include:   ../../references/microtonic-scripts-sdk/Microtonic Resources/microtonic.schema
resources: ../
resources: ../../references/microtonic-scripts-sdk/Microtonic Resources
```

Never copy `Microtonic Resources/` into `scripts/`. If CushyLint or an editor is
looking for `scripts/Microtonic Resources/`, the copied package schema is wrong;
update the schema paths instead.

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

## Bridge Smoke Test

After creating `.mcp.json`, tell the user in product-specific language how the
assistant app loads the `microtonic-bridge` server. Avoid saying only "restart
the MCP client/session"; most users will not know what that means. `.mcp.json`
is a Claude Code project-server convention: for Claude Code, say to exit Claude
Code, start it again in the project, and approve the project MCP server prompt
if it appears.

Do not assume Codex discovers project `.mcp.json` by restarting. Codex uses its
own MCP server configuration; a project `.mcp.json` alone is not enough for
Codex. First check which CLI is being invoked:

```sh
which -a codex
codex --version
codex mcp --help
```

If `codex mcp --help` shows `add`, `list`, `get`, and `remove`, register the
bridge with that CLI using an absolute `server.js` path:

```sh
codex mcp add microtonic-bridge -- node /ABS/PATH/TO/references/microtonic-scripts-sdk/tools/jsconsole-bridge-mcp/server.js
codex mcp list
```

If plain `codex` is an older binary whose `mcp` command only says "run Codex as
an MCP server", but the Codex desktop app is installed, try the app-bundled CLI:

```sh
/Applications/Codex.app/Contents/Resources/codex mcp --help
/Applications/Codex.app/Contents/Resources/codex mcp add microtonic-bridge -- node /ABS/PATH/TO/references/microtonic-scripts-sdk/tools/jsconsole-bridge-mcp/server.js
/Applications/Codex.app/Contents/Resources/codex mcp list
```

If no available Codex CLI supports MCP management, add the bridge directly to
`~/.codex/config.toml`, or ask the user for approval to edit that file because
it is outside the project workspace:

```toml
[mcp_servers.microtonic-bridge]
command = "node"
args = ["/ABS/PATH/TO/references/microtonic-scripts-sdk/tools/jsconsole-bridge-mcp/server.js"]
enabled = true
```

Then restart Codex or reopen the project so it reloads its MCP configuration. In
the resumed Codex session, first verify that `mt_status` and `mt_eval` are
actually exposed as tools. If they are absent, say plainly that Codex has not
loaded the Microtonic bridge tools and do not keep asking for blind restarts. If
the assistant cannot edit `~/.codex/config.toml`, print the exact TOML block
with the resolved absolute `server.js` path and tell the user to add it
manually.

When a restart/reload is known to load the bridge server, treat that restart as
a handoff point: before ending the pre-restart session, give the user a resume
checklist to reload the assistant app, approve the project MCP server if asked,
open Microtonic, open the SDK `JSConsole.mtscript`, type `bridge on`, approve
the folder write-permission prompt if shown, and resume the assistant session
with this exact prompt:

```text
Run the Microtonic bridge smoke test now. Check mt_status, then evaluate 1 + 1 over the bridge.
```

In the resumed session, once the client exposes the bridge tools, call
`mt_status`; it should report `attached: yes`. Finally call `mt_eval` with a
harmless expression such as `1 + 1` and confirm it returns `2` before relying on
the bridge for live debugging.

## Reloading While Iterating

Microtonic caches resources while the GUI window is open, including JavaScript
source files loaded by scripts.

When editing GUI scripts:

- Click the reload button at the top of JSConsole, or type `reload`, to flush
  cached resources, rebuild the open GUI, rerun JavaScript, and keep existing
  globals.
- Shift-click the JSConsole reload button, or type `reset`, for a full
  JavaScript engine reset that clears memory and globals before rebuilding.
- Toggling a script's own window with `toggleCushy(...)` does not reload edited
  resources. Closing Microtonic's main GUI window destroys the whole JavaScript
  environment; reopening it starts clean, like a full reset.

End-user zoom scale changes also force GUI reloads so graphics resources and
exact layout coordinates can be rescaled. GUI scripts must survive their
JavaScript files being run again. See [`cushy-notes.md`](cushy-notes.md)
"Reload And State" for the detailed reload/reset behavior.

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
