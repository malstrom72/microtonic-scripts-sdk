# Cushy Notes

Use this file for practical Cushy details, gotchas, and patterns that are
useful when writing or debugging `.mtscript` GUI packages but are too specific
or workflow-oriented for the main instruction file.

These notes do not replace the schema, JS reference, or example packages. When
exact syntax or supported fields matter, verify against the references listed in
[`source-map.md`](source-map.md).

## scriptRoot Convention

Always define `@scriptRoot` at the top of every `.cushy` file and use it
wherever the package folder name appears (e.g. `file:` references). This means
renaming the package requires only a one-line change.

```makaron
@define scriptRoot=MyScript.mtscript
@define script=myScript
```

Then reference IVG files as:

```
file: "@scriptRoot/MyScript_someGraphic"
```

Note: `@scriptRoot` is a Makaron define and only expands in `.cushy` files.
The entry-point `.js` file (`toggleCushy(...)`) is plain JavaScript and must
still use the literal folder name.

## CushyLint

See [`validation.md`](validation.md) for the full command. Easy mistake: passing a
directory path **without** a trailing slash silently skips the companion
`.schema` file and produces false "rule missing" errors. Always add the slash:

```sh
CushyLint/CushyLint "$(pwd)/MyScript.mtscript/" "$(pwd)/Microtonic Resources"
#                                             ^
```

## Cluster Views

A `cluster` is not only a display primitive. By default it is an editable paint
surface tied to its `array`.

If `array` points to a normal JavaScript array of plain values, the cluster can
write directly into that array while the user clicks or drags. This is useful
for editable step lanes, but dangerous when the array is meant to be derived
display state. One failure mode is dragging through a display lane and writing
image or marker values back into cells that should be controlled by script
state.

Safer patterns:

1. Display-only cluster

   Use `readonly: true` when the cluster should only visualize state.

2. Editable cluster with controlled state

   Use custom per-cell GUI variable objects with `get` and `set` methods
   instead of plain values when writes need to be intercepted.

3. Separate display and interaction

   For advanced controls, layer clusters:

   - A readonly visual cluster for drawing.
   - A transparent or invisible interaction cluster for `mouseIndex` and
     `clickActions`.

Practical rule of thumb: use a plain array only when it is okay for the cluster
to modify it directly. Use `readonly: true` or per-cell `get` / `set` objects
when the displayed values are derived from script state.

## `mouseIndex`

- `mouseIndex` is useful for tracking which rect the pointer is over.
- It may become an empty string during pointer transitions or release.
- Do not blindly coerce it to a number, because `""` can behave like `0` and
  cause controls to snap to the first cell.
- Keep a last valid index or ignore empty updates during drag.

## Reload And State

- Cushy reruns the matching `_main.js` on GUI reload, but existing globals
  survive normal JSConsole reload.
- If object instances are kept across reloads, they may retain old methods and
  old action closures.
- For development, recreate interaction objects on each reload and copy over
  only persistent user state.
- Shift-reload resets the whole JavaScript engine and clears stale instances.

## Slider Slit Inset

The slit `start` and `end` coordinates define where the **center** of the cap
travels, not where its edges travel. Cushy clips hard at view bounding boxes
with no exceptions, so both the cap fill and its frame stroke must stay inside
the slider's bounds at the extremes.

Inset each end by `cap_half_width + frame_stroke`:

```
// cap width = 12, frame stroke = 1 → inset = 6 + 1 = 7
cap:  { size: { 12, 20 }, frame: { color: "...", stroke: 1 }, ... }
slit: { start: { 7, 14 }, end: { 113, 14 }, ... }   // slider bounds width = 120
```

## All Text Requires an Explicit Font

Cushy has no default font. Any text — `caption` views, button `caption` fields
(via `font` in `<buttonStyle>`), bubble styles, or any other text-bearing view —
will not be drawn at all unless an explicit `font` is specified. There is no
fallback rendering.

```
// caption view
{ type: "caption", text: "...", font: { ivgfont: "sans-serif", size: 13, color: "..." } }

// button caption
standard: { fill: "...", font: { ivgfont: "sans-serif", size: 14, color: "..." } }
```

## Radio Button / Checked State Pattern

Use `action: "set"` with a **plain string variable** for radio-button groups
(e.g. scale pickers, channel selectors). Per the Cushy spec, `set` is checked
when `<var>` is already exactly `<value>` — so the `checked:` button style
applies automatically with no JS code required.

```
// JS: plain string property on the state object
scaleGenerator = { scaleIndex: '0', ... };

// Cushy: each option button
{
    type: "button"
    action: "set"
    params: { scaleGenerator.scaleIndex, "2" }
    standard:    { fill: "...", font: ... }
    checked:     { fill: "..." }        // shown when scaleIndex == "2"
    checkedDown: { fill: "..." }
}
```

This is the pattern MacroTweak's channel-choice buttons use. For custom actions,
the equivalent is an action object with both `execute` and `checked` functions —
omitting `checked` means the checked style never applies.

## Drum Patch Property Copy

`JSON.stringify` / `JSON.parse` fail on Microtonic's internal drum-patch
objects (they are native C++ proxies). The error surfaces as
`TypeError: Cannot convert undefined or null to object`.

To copy a drum patch, snapshot its properties into a plain JS object using
`DRUM_PATCH_PARAMS`, then write back to the **existing** internal object
in-place. Do not assign a new plain object to `preset.drumPatches[ch]`.

```javascript
// Snapshot before the loop so in-place writes don't corrupt later reads
var snap = {};
for (var i = 0; i < DRUM_PATCH_PARAMS.length; i++) {
    snap[DRUM_PATCH_PARAMS[i].NAME] = src[DRUM_PATCH_PARAMS[i].NAME];
}
snap.name = src.name;

for (var ch = 0; ch < CHANNEL_COUNT; ch++) {
    var dp = preset.drumPatches[ch];         // existing internal object
    for (var j = 0; j < DRUM_PATCH_PARAMS.length; j++) {
        dp[DRUM_PATCH_PARAMS[j].NAME] = snap[DRUM_PATCH_PARAMS[j].NAME];
    }
    dp.name = snap.name;
    dp.modified = true;
    // apply any per-channel adjustments to dp here
}
setElement('preset', preset);
```

## Window Default Position

Microtonic's main canvas is `{ 0, 0, 740, 550 }` (from `main.cushy`). Script
windows must fit within this — clipping is hard, not graceful.

The `@window(left, top, width, height, ...)` macro's `height` **includes** the
24px title bar (defined as `@titleBarHeight` in `common.makaron`). To stay
within bounds: `left + width ≤ 740` and `top + height ≤ 550`.

To center a window:

```
left = (740 - width) / 2
top  = (550 - height) / 2
```

`windowPosition` is restored between window close/re-open because the script
stores it on its state object inside the `if (!this.script)` initialization
guard — so the value survives the window being closed and the JS file re-running.
The `dragArea` reads the variable on open and writes it on drag, but persistence
is entirely the script's responsibility. Without the guard (or without
`windowPosition` on the persisted object), position resets to the default on
every open.

## IVG: Prefer External Files Over Inline Code

Use `file: "name"` (no extension) in `vector` and `image` views rather than
`code: "..."` inline strings when the IVG content is static (no GUI-variable
references). External `.ivg` files can be validated and rasterized by the
vendored `IVG2PNG` renderer:

```sh
tools/validate-static-ivg.sh
```

This script renders every static `.ivg` under `Microtonic Resources/` and
`examples/` to PNG and fails if any file cannot be parsed or rasterized. Files
that reference GUI variables are reported as dynamic and skipped — they need
separate review.

Inline `code:` snippets cannot be fed to `IVG2PNG` without manual extraction.

## IVG Colors

- `none` can be used for transparency.
- `rgb(r,g,b,a)` uses normalized component values, not `0` to `255` values.
- Use `rgb(1,1,1,0.35)`, not `rgb(255,255,255,0.35)`.
- CSS-style `rgba(...)` is not valid IVG color syntax.
