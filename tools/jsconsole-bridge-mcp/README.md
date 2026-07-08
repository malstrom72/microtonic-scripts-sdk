# Microtonic JSConsole bridge — MCP server

A tiny [Model Context Protocol](https://modelcontextprotocol.io) server that lets
an MCP client (e.g. Claude Code) evaluate JavaScript against a **live** Microtonic
engine and read the result back — no GUI automation, no screen reading.

It drives the file bridge built into `JSConsole.mtscript`. You type `bridge on`
in JSConsole; this server writes JS requests to a shared folder and reads the
replies the bridge writes back.

## How it fits together

```
MCP client  ──mt_eval(code)──▶  this server  ──request.json──▶  JSConsole bridge
                                                                 (eval in live engine,
                                                                  in the shared JS globals)
MCP client  ◀──value/output──   this server  ◀─response.json──  JSConsole bridge
```

Because every Microtonic script shares one JS global space, code you `mt_eval`
can read and drive a script running in the **main GUI layer** while JSConsole
runs in the dev layer — it's a real debugger into the running instrument.

## Shared folder

Both ends agree on a fixed, user-writable, username-independent path:

| OS      | Folder |
| ------- | ------ |
| macOS   | `/Users/Shared/Sonic Charge/Microtonic/jsconsole-bridge/` |
| Windows | `C:/Users/Public/Sonic Charge/Microtonic/jsconsole-bridge/` |

The server `mkdir -p`s it on startup (Microtonic's script API cannot create
folders). Override with the `BRIDGE_BASE` environment variable if needed — it
must match `jsConsole.bridgeBase()` in `JSConsole_main.js`.

Files (all JSON):

- `request.json` — `{ seq, code }`, written by this server (temp file + atomic rename).
- `response.json` — `{ seq, ok, value, output, error }`, written by the bridge.
- `bridge.json` — `{ ready, protocol, time, owner }`, written by the bridge on `bridge on`
  (`owner` is a token identifying the instance that currently holds the bridge).

Requests and replies are paired by a strictly increasing `seq` (epoch-ms based,
so it keeps climbing across restarts). The bridge ignores any `seq` it has
already handled.

> **One active bridge at a time (single owner).** The folder is a single fixed
> machine-global path, so only one Microtonic instance can serve the bridge at a
> time. `bridge on` records an `owner` token in `bridge.json`. If another instance
> already owns it, `bridge on` pops an OK/Cancel dialog offering to take over;
> taking over writes the new owner, and the previous owner sees the changed token
> on its next tick and stands down — so two engines never handle the same request.
> To move the bridge to a different instance, run `bridge on` (and click OK) in that
> instance's JSConsole window.

## Tools

- **`mt_eval(code, [timeout_ms])`** — evaluate `code` against the live engine.
  Returns the value of the final expression plus any `print()` output. Keep
  snippets short: each eval freezes the UI and is subject to Microtonic's ~20s
  per-call suspension limit. Wrap multi-statement snippets in an IIFE so local
  `var`s do not leak into the shared global space or shadow host names like
  `save`, `load`, or `print`. Default timeout `20000` ms.
- **`mt_status()`** — check whether the bridge is actually **responding**. It probes
  (a trivial eval with a short timeout) and reports `bridge: LIVE` or `bridge: NOT
  RESPONDING`, rather than trusting the `bridge.json` presence file, which lingers
  after the console is closed. See [When the bridge doesn't respond](#when-the-bridge-doesnt-respond).

## Install

Requires Node ≥ 18 (no dependencies, no build step).

### Claude Code

This repo ships a project-scoped [`.mcp.json`](../../.mcp.json) at its root, so
opening the project in Claude Code offers the server automatically — approve the
one-time prompt and you're done. No per-developer setup; the `server.js` path is
resolved relative to the repo root.

To register it manually instead (e.g. from outside the repo, or for a single
user), use:

```sh
claude mcp add microtonic-bridge -- node /ABS/PATH/TO/microtonic-scripts-sdk/tools/jsconsole-bridge-mcp/server.js
```

### Other MCP clients

`.mcp.json` is Claude Code's convention. The bridge itself is client-agnostic —
the `request.json` / `response.json` protocol is just files. Point any MCP client
at `node tools/jsconsole-bridge-mcp/server.js`, or write your own host against
the file protocol described above.

## Usage

1. Install this SDK's bridged `JSConsole.mtscript` into Microtonic's scripts folder, then open Microtonic, open
   `JSConsole.mtscript`, and type `bridge on`. Grant the folder write-permission prompt when it appears.
2. From the MCP client, call `mt_status` to confirm `bridge: LIVE`, then
   `mt_eval` with a snippet, e.g. `getElement('pattern').steps`.

You'll see each command echo as `BRIDGE> …` in the JSConsole window.

The quickest way to find the scripts folder is the script menu in Microtonic →
**Open Scripts Folder** (it is `DIRS.SCRIPTS`). On Windows, copying the console may need elevation; for repeated
development, consider linking the live scripts folder to a project `scripts` folder first.

Once the target is confirmed, copy the SDK's bridged console with:

```sh
node tools/install-jsconsole.js "<Microtonic Scripts folder>"
```

## Running or toggling a script over the bridge

Use Microtonic's `run()` function with the script's `.js` entry point path
relative to the live `Microtonic Scripts` folder:

```js
mt_eval("run('MyScript.mtscript/MyScript.js')")
```

For GUI packages, the entry `.js` normally calls `toggleCushy(...)`, so this
usually toggles the package window, not just launches it. If the package GUI is
closed, `run(...)` opens it; if the same package GUI is already open,
`run(...)` closes it. For example:

```js
mt_eval("run('PolyrhythmChain.mtscript/PolyrhythmChain.js')")
```

To force a GUI package open without toggling, call the modal action directly
with the package's layout path:

```js
mt_eval("performCushyAction('modal.open', 'MyScript.mtscript/MyScript_main')")
```

## Checking the currently open script

The current modal Cushy script window is stored in `modal.current`:

```js
mt_eval("getCushyVariable('modal.current')")
```

It returns a layout path such as
`PolyrhythmChain.mtscript/PolyrhythmChain_main`, or an empty value when no modal
script window is open. To test whether a specific package is open, use the same
prefix check as Microtonic's built-in script popup:

```js
mt_eval("getCushyVariable('modal.current').substring(0, 'MyScript.mtscript/'.length) === 'MyScript.mtscript/'")
```

`JSConsole` itself is usually hosted through the development layout
(`devLayout`), not `modal.current`, so `modal.current` is the useful value for
the user script window you are debugging.

## Reloading the target script over the bridge

The console's `reload` / `reset` are JSConsole *commands*, not globals, so
`mt_eval("reload")` just throws `ReferenceError`. To rerun the script files and
rebuild the GUI from the host — the edit → reload → re-test loop — evaluate the
underlying action instead:

```js
mt_eval("performCushyAction('reload')")
```

A normal reload reruns the JavaScript files but keeps the engine and globals
alive. It does not unload or close the current script window, so
**the bridge survives its own reload** and keeps working. The call returns
`true`.

Do **not** drive a full reset (`performCushyAction('reload', 'reset')`) over the
bridge. It is deferred to the next tick, so your current call still gets a reply,
but the reset then wipes JS memory — tearing down JSConsole and the bridge. After
that you must re-enable `bridge on` from the JSConsole window. Do resets from the
GUI.

## When the bridge doesn't respond

`mt_eval` timing out, or `mt_status` reporting `bridge: NOT RESPONDING`, means the
bridge isn't answering. **Diagnose in order of likelihood — a modal dialog is the
*least* common cause, not the first thing to check:**

1. **Is the JSConsole window open in Microtonic?** If it was closed, reopen it.
2. **Was `bridge on` typed in it _this session_?** A `bridge.json` presence file is
   written once on `bridge on` and lingers afterward, so it does **not** prove the
   bridge is live — that is exactly why `mt_status` probes instead of trusting it.
   Re-type `bridge on`.
3. **Is Microtonic running at all?**
4. **Only if the bridge _was_ working and just stopped** is a modal dialog the likely
   cause. The bridge tick runs on Microtonic's UI thread, so a synchronous modal
   (`displayCushy(...)` from a startup/reload path, or a Cushy/IVG load-error dialog
   such as an invalid pre-multiplied `#AARRGGBB` color) freezes the tick until
   dismissed. The tell is `last reply seq` frozen *below* `last request seq` after it
   had been advancing. Dismiss the dialog in Microtonic, then run `bridge off` /
   `bridge on`.

`mt_status` distinguishes these cases: it probes the bridge (a trivial eval with a
short timeout) and reports `LIVE` or `NOT RESPONDING` rather than trusting the
presence file. Re-run it before sending another eval.
