# Cushy Notes

Use this file for practical Cushy details, gotchas, and patterns that are
useful when writing or debugging `.mtscript` GUI packages but are too specific
or workflow-oriented for the main instruction file.

These notes do not replace the schema, JS reference, or example packages. When
exact syntax or supported fields matter, verify against the references listed in
[`source-map.md`](source-map.md).

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

## IVG Colors

- `none` can be used for transparency.
- `rgb(r,g,b,a)` uses normalized component values, not `0` to `255` values.
- Use `rgb(1,1,1,0.35)`, not `rgb(255,255,255,0.35)`.
- CSS-style `rgba(...)` is not valid IVG color syntax.
