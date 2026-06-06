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
- `bridge.json` — `{ ready, protocol, time }`, written by the bridge on `bridge on`.

Requests and replies are paired by a strictly increasing `seq` (epoch-ms based,
so it keeps climbing across restarts). The bridge ignores any `seq` it has
already handled.

> **One Microtonic instance only.** The folder is a single fixed machine-global
> path, so the bridge assumes exactly one live bridge per machine: one running
> Microtonic, one JSConsole, `bridge on`. That's the normal case. Two instances
> with the bridge enabled would race over the same files — enable `bridge on` in
> only one at a time.

## Tools

- **`mt_eval(code, [timeout_ms])`** — evaluate `code` against the live engine.
  Returns the value of the final expression plus any `print()` output. Keep
  snippets short: each eval freezes the UI and is subject to Microtonic's ~20s
  per-call suspension limit. Default timeout `20000` ms.
- **`mt_status()`** — report whether a bridge is attached (via `bridge.json`) and
  the last request/reply sequence numbers.

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

1. Open Microtonic, open `JSConsole.mtscript`, type `bridge on`. Grant the
   folder write-permission prompt when it appears.
2. From the MCP client, call `mt_status` to confirm `attached: yes`, then
   `mt_eval` with a snippet, e.g. `getElement('pattern').steps`.

You'll see each command echo as `BRIDGE> …` in the JSConsole window.

## Reloading the target script over the bridge

The console's `reload` / `reset` are JSConsole *commands*, not globals, so
`mt_eval("reload")` just throws `ReferenceError`. To rerun the script files and
rebuild the GUI from the host — the edit → reload → re-test loop — evaluate the
underlying action instead:

```js
mt_eval("performCushyAction('reload')")
```

A normal reload reruns the JavaScript files but keeps the engine and globals
alive, so **the bridge survives its own reload** and keeps working. The call
returns `true`.

Do **not** drive a full reset (`performCushyAction('reload', 'reset')`) over the
bridge. It is deferred to the next tick, so your current call still gets a reply,
but the reset then wipes JS memory — tearing down JSConsole and the bridge. After
that you must re-enable `bridge on` from the JSConsole window. Do resets from the
GUI.
