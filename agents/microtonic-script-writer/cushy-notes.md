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

## Timed Actions / Animation Loop

A GUI script gets periodic callbacks by registering a repeating action in the
root `autoexecs` block. Use this for animation, polling, games, sequenced GUI
updates, or any other time-based behavior. Each JavaScript action should return
quickly; the UI freezes while script code runs, and a single execution that
runs longer than 20 seconds is suspended.

```cushy
autoexecs: {
    { action: "@script.tick", repeat: 1/@fps }
}
```

Use `Date.now()` deltas for work that needs a stable rate, so behavior does not
depend on the exact callback cadence:

```javascript
tick: function() {
    var now = Date.now();
    if (now - this.lastStep >= this.intervalMs) {
        this.lastStep = now;
        this.step();
    }
    this.render();
}
```

Other useful `autoexecs` triggers in the same schema block include
`onChanged: <var>`, `onClose`, `onReload`, `onInit`, and one-shot `delay`.

## Performance

Measure performance in Microtonic. CushyLint and ESLint can catch syntax and
schema problems, but they do not predict runtime cost. `JSConsole.mtscript`
shows an FPS counter; treat FPS drops, visible UI lag, delayed redraw, the
20-second suspension prompt, or memory termination as real warning signs. For
script-side profiling, use `Date.now()` around action bodies and print
occasional averages rather than tracing every tick.

### Actions And Timing

- Script actions run synchronously on the UI thread. They block repaint and
  input until they return, except when they call modal UI functions such as
  alerts or file dialogs. Those modal waits may process the event loop while
  waiting for user input and do not count toward the 20-second suspension timer.
- The 20-second suspension timer applies per JavaScript invocation, not
  cumulatively across repeated `autoexecs`.
- Repeated `autoexecs` queue if the previous invocation is still running. They
  run as often as possible, but they do not take priority over the entire UI.
- Use repeat rates that communicate intent. On Windows, the practical maximum
  is limited by the Win32 15.6 ms timer resolution, around 64 Hz. On Mac, the
  maximum is around 200 Hz. In normal scripts, target 50 Hz or lower.
- Avoid "as often as possible" repeats such as `repeat: 0.001` unless there is a
  very specific reason. For reacting to state changes, use `onChanged` instead.
  `onChanged` actions run asynchronously on the next tick after the current
  action returns, and only when the value actually changes.

### GUI Variables And Polling

- Cushy does not maintain a dependency graph. Views and native code poll GUI
  variables regularly. Poll rates adapt per lookup: variables that rarely
  change are polled less often, down to one fifth of the normal tick rate. In
  Microtonic, full rate is normally 50 fps.
- Updating a variable every tick is acceptable when the view needs full-rate
  updates. If a variable is read by views, changing it frequently keeps those
  reads active at the full rate.
- Keep GUI-variable getters short and fast. Variable data is polled a lot in
  Cushy, not only by IVG views. Prefer simple JavaScript or Cushy variables for
  values that are read often. If a getter must compute derived text or data,
  cache the result based on its inputs.
- JavaScript variables shadow Cushy variables with the same name. Account for
  that when naming GUI state or debugging why a view reads a particular value.
- For dynamic IVG data, compact list strings are usually fine for payloads of a
  few hundred characters, or even around a thousand characters. If the IVG only
  needs a tiny part of a very large structure, do not serialize the whole thing
  every tick; `guiVariables: true` can be a better fit.
- There is no simple "bindings are faster" or "guiVariables is faster" rule.
  Choose based on clarity and update pattern.

### View Redraw And Vector Caching

- Cushy uses dirty-rectangle redraws and occlusion culling. Small independent
  invalidated regions can redraw separately; overlapping regions may merge into
  one larger redraw. Views covered by non-transparent views can often be partly
  or fully skipped.
- Bounds and clipping affect both correctness and performance: drawing outside a
  view's bounds is clipped, and the clipped-away work is not drawn.
- Cushy can handle thousands of views. The cost depends on update patterns,
  variable refreshes, redraw area, and what each view does.
- Hidden or invisible views usually do not carry meaningful cost, unless their
  state depends on heavy variable refreshes. Exactly what updates while hidden
  depends on the native view implementation.
- `hover` and `mousePosition` updates are not inherently expensive, but avoid
  doing unnecessary work in their actions or getters.
- A `cluster` is often the right UX for many repeated cells because it tracks a
  mouse-down gesture across cells. Individual buttons are still fine when the
  interaction is one button at a time.

Vector views have additional caching:

- For the first five ticks after creation or cache flush, a vector view draws
  directly to the destination buffer when it uses normal blending and 100%
  opacity.
- On the fifth redraw with no changes, it draws into an RLE-compressed offscreen
  buffer. Later redraws can reuse that cache until source material changes.
- Static or rarely updated vector views are therefore usually cheap after the
  initial redraws.
- A vector view tracks changes to the IVG source, bound variables that were
  accessed during the last update, standard native parameters such as `$width`
  and `$height` when accessed, and, with `guiVariables: true`, global GUI
  variables accessed during the last update.
- `file:` vector sources are cached by the resource manager until a reload, such
  as JSConsole reload or a zoom-scale reload.
- `defines:` are IVG variables with constant values. They are available during
  rendering, not a one-time textual substitution pass.
- `bindings:` are available during rendering too. How often the vector actually
  rerenders depends on the change tracking and cache behavior above.

### IVG And Drawing Cost

- IVG/IMPD is interpreted when it renders. There is no GPU acceleration; drawing
  is CPU-rendered.
- IVG itself is not a fast language, but large pixel areas are often the bigger
  cost than individual language constructs. Very large UI areas, high zoom
  levels, and Retina-scale buffers are common reasons to notice FPS drops.
- Basic shapes, paths, masks, gradients, patterns, opacity, blend modes, image
  transforms, and text can all be practical. Do not assume a feature is too
  expensive without measuring it in the actual UI size and update pattern.
- Static IVG source with `defines:` and `bindings:` is usually preferable for
  clarity, separation of presentation from data, and static regression testing.
  Generated IVG source is also fine when it is the clearest expression of the
  drawing.
- Splitting graphics into multiple layered vector views can improve performance
  when some layers are static or rarely updated. It can also be unnecessary.
  Measure the actual result.

### JavaScript Data And Memory

- The memory cap is checked after garbage collection. There is no script-visible
  explicit GC call in this version of Microtonic.
- Returning from an action is enough to make large temporary data collectible if
  no global or retained object still references it.
- Avoid keeping large transient data in persistent globals. Shift-reload in
  JSConsole resets the JavaScript engine and is useful for clean memory tests.
- `StringBuilder` is useful when building very long strings or appending many
  pieces. Repeated `+=` one character at a time creates many intermediate
  strings and heavy GC pressure.
- `Array.prototype.join` uses `StringBuilder` internally. Ordinary `+`
  concatenation is fine for a small number of short strings; use `StringBuilder`
  or `join` when combining hundreds or thousands of pieces.
- Arrays store full 16-byte JavaScript values for elements, including numbers
  and booleans. Dense arrays use continuous storage; sparse arrays with large
  holes become object-like internally.
- Object maps and many small objects are normal JavaScript tools, but they carry
  object/value overhead. Compact strings can be much cheaper for dense payloads
  such as IVG list data.

### Microtonic API Calls

- `getElement` returns structured Microtonic data as JavaScript objects:
  preset, current drum patch, current pattern, MIDI config, or visuals.
  `setElement` writes supported structured data back: preset, current drum
  patch, current pattern, or MIDI config.
- Use `getElement` / `setElement` when the operation is naturally about one of
  those structured elements, such as editing pattern steps, copying drum-patch
  properties, or changing MIDI configuration.
- Use `getElementId` instead of `getElement` when the script only needs to know
  whether a preset, drum patch, pattern, or MIDI config has changed.
- Use `setParam` when changing only a few parameters. It creates less JavaScript
  structure and, more importantly, does not disturb real-time DAW parameter
  updates the way replacing the whole program can.
- `saveUndo(..., collapse)` is a UX choice, not a performance optimization. Use
  `collapse: true` when repeated actions with the same description should become
  one undo item instead of many.
- `triggerChannel` is cheap enough for normal GUI interaction.

## Easy `.cushy` Mistakes

- `@define` captures everything to the end of the line unless the value is
  wrapped as a raw value (`@<...@>`). A trailing comment becomes part of the
  macro value and can break later Numbstrict parsing. Put comments on their own
  line when possible:

  ```makaron
  // width of the main area
  @define viewWidth = 360
  ```

  Raw values are useful when a same-line comment is genuinely convenient:

  ```makaron
  @define viewWidth = @<360@> // comment is emitted as output, but not part of @viewWidth
  ```

  Makaron does not discard the trailing comment; it still appears in the
  expanded Cushy source as a `//` comment line. That is normally accepted by
  Numbstrict/Cushy parsing, but it is not the same as removing the comment.

- Cushy and IVG use different transparency spellings. Cushy's `<color>` accepts
  `transparent`; IVG's `<color>` uses `none` for invisible paint. Use
  `fill: "transparent"` in `.cushy` view fields, and use `fill none` / `pen
  none` inside `.ivg` or inline `ivgCode`.

- A `button` caption can be a plain text value, or an object with both `text`
  and `offset`. If you use object form, the offset is required:

  ```cushy
  caption: "GO"
  caption: { text: "GO", offset: { 0, 0 } }
  ```

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

## View Bounds Clip Drawing And Input

Cushy view hierarchy clips hard at every view's `bounds`. Nothing outside a
view's bounds is drawn or clickable, even when a child view declares larger or
negative bounds.

Use `dragArea` only when the parent view itself is the thing being moved, and
there is larger surrounding space for it to move in. This is the MixConsole
channel-strip pattern: the strip/group moves within a wider parent, and the
`dragArea` updates a `positionVariable` for that moving parent.

Do not use `dragArea` for "click anywhere in a fixed pad and move a marker
there." A `dragArea` moves its parent. Making the parent as large as the whole
pad gives it no useful room to move, and putting a larger `dragArea` inside a
smaller handle group is clipped by the handle group's bounds.

For fixed pads, use the BeatSpace pattern instead: layer a full-size `click`
view with `mousePosition`, `press`, and `release` actions, plus a `hover` view
that updates the same mouse-position variable. Let JavaScript store a
`dragging` flag, convert the reported mouse coordinates into the control value,
and move the visual marker with group `offset` variables or another
script-controlled visual state.

## Driving IVG From Script State

`vector` views can render dynamic data by combining static `defines:` with live
`bindings:`. Static settings such as sizes or palette colors belong in
`defines:`. Data that JavaScript updates while the GUI is open belongs in
`bindings:` and is re-read when it changes.

```cushy
{
    type: "vector"
    file: "@scriptRoot/Points"
    defines: {
        "pointSize": "@pointSize"
        "color": "#E0D0D0E0"
    }
    bindings: {
        "points": @script.points
        "selected": @script.selectedChannel
    }
}
```

The script can keep a binding as an IVG/IMPD list string. BeatSpace uses this
pattern for point, constellation, and flare rendering:

```javascript
script.points = "[[0,120,80,yes,no],[1,160,96,yes,no]]";
```

Then IVG can iterate the list and split each row:

```ivg
for p in:$points [
    $split $p into:i,x,y,enabled,muted
    context [
        offset $x,$y
        ellipse 0,0,$pointSize
    ]
]
```

For more structured state, a `vector` view can set `guiVariables: true`. This
allows IVG source to read GUI variables directly with `$<variable-name>` and
refreshes when variables touched during the last repaint change. Prefer explicit
`defines:` plus `bindings:` when the data can be expressed cleanly; it makes the
view's input contract visible in the `.cushy` file and avoids giving the drawing
layer access to the full GUI-variable namespace.

When JavaScript needs to generate the whole IVG source string, use the
`variable:` source form:

```cushy
{ type: "vector", variable: @script.ivgSource }
```

`variable: <var>` is the direct form for this pattern: the named GUI variable
contains the complete IVG source. Use `code:` when the source is written inline
in the `.cushy` file; use `variable:` when JavaScript owns and updates the
source string.

Generated IVG source is flexible but shifts drawing text generation into
JavaScript. IVG/IMPD is interpreted when it renders, so the cost is not a
separate compile step; the tradeoff is that JavaScript may need to rebuild and
store more source text, and static `_test.ivg` validation cannot cover every
generated path. NuXJS stores string character data as UTF-16, so a large source
string or compact text payload is usually much cheaper than an equivalent
JavaScript array of numbers, though normal string object and allocation overhead
still applies.

## Input Model

There is no documented Cushy model for binding arbitrary real-time keyboard
keys such as arrows or WASD to script actions. Design live interaction around
mouse hover, click, drag, context click, and modifier masks. Text input is
available through:

- `ask(question, [default])` in JavaScript: a modal text dialog returning a
  string or `null` on cancel.
- The Cushy `edit` built-in action: a modal editor bound to a variable, with
  optional `default` and `reaction`.
- The Cushy `console` view: TTY-style input/output with live `inputVariable`,
  `inputAction` on Enter, and `outputVariable` or `outputArray`. See
  `JSConsole.mtscript`.

BeatSpace carries a Microtonic 3.4 workaround comment for modifier detection:
no more than one key modifier is flagged there, so modifier combinations should
be treated cautiously. If modifiers matter, give each modifier its own click
mask and choose an explicit priority order.

### Click Mask Dispatch

For `click` and `button` views, an `actions` block is an ordered dispatch table,
not a list of callbacks. For each mouse event, Cushy scans masks **bottom up**,
so later entries have higher priority, and chooses one matching action.

Consequences:

- Only one action runs for a given event.
- Later masks can steal events from earlier masks when their type/modifiers
  match.
- Do not add extra masks speculatively. An apparently harmless `{ "down",
  "nop" }` at the bottom of the table can take priority for hit-tracking entry
  events.
- Put more specific modifier masks after less specific masks when both could
  match, e.g. put `press+shift` below `press` if shift should win.
- `context` handles right-click / control-click and can bind directly to a
  script action, not only to the built-in `popup` action.
- `down` / `up` are hit-tracking enter/leave masks. They are not extra
  press-drag callbacks.

For a press-drag-release pad, follow BeatSpace and use the masks that define
that interaction:

```cushy
{
    type: "click"
    actions: {
        { "press", "@script.press" }
        { "release", "@script.release" }
    }
    mousePosition: { xy: @script.mousePosition, integer: false }
}
```

## Cushy Action Binding

Cushy JavaScript actions called as methods on the script singleton bind `this`
to that script object. For example, inside `morphSquare.padPress`,
`this === morphSquare` is true.

Referencing the explicit script singleton is still acceptable when it makes
callbacks or moved helper functions clearer, but do not diagnose action failure
as a `this` binding problem without checking it directly:

```javascript
// Valid when called as a Cushy action on morphSquare.
padPress: function() {
    this.dragging = true;
    this.setFromMouse(this.mouseXY);
}

// Also valid, and sometimes clearer for callbacks/helper reuse.
padPress: function() {
    morphSquare.dragging = true;
    morphSquare.setFromMouse(morphSquare.mouseXY);
}
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

## IVG Font Character Coverage

The built-in IVG `sans-serif` font does not cover all Unicode characters.
Use only printable ASCII (U+0020–U+007E) in GUI variable return values and
caption fallback strings. Characters outside that range — including the em-dash
`—` (U+2014) and similar typographic characters — render as one replacement
box per code unit, which typically appears as `???` in the UI. Use plain ASCII
alternatives such as `-` or `...` instead.

## File Extensions And `dir()` Scanning

### Microtonic file extensions

| Type | Current extension | Legacy extension (v2) |
|---|---|---|
| Preset | `.mtpreset` | `.mtpg` |
| Drum patch | `.mtdrum` | `.mtdp` |
| MIDI config | `.scmc` | — |
| Script package | `.mtscript` (directory) | — |

Both current and legacy formats are accepted by `isMarshaledFormat` /
`unmarshal` / `load` in Microtonic's open dialogs. When scanning for files to
load programmatically, matching only the current extension is usually fine
since factory content uses the current format. Add the legacy extension to the
pattern when user-created files from older versions may be present.

### `DIRS.PRESETS` layout

`DIRS.PRESETS` points to the platform preset folder (e.g.
`/Library/Audio/Presets/Sonic Charge/Microtonic Presets/` on macOS). It
contains two subdirectories:

- `All/` — flat list of all factory presets as `.mtpreset` files
- `By Package/` — the same presets organised into per-pack subdirectories

Scanning `DIRS.PRESETS + 'All/'` is the simplest way to access the full
factory preset library from a script.

### `dir()` extensionFilter excludes directories

When an `extensionFilter` argument is passed to `dir()`, the JS Reference
states that *"only files with the given extension(s) are returned"* — and in
practice **directories are excluded** from the result. This silently breaks
recursive scanning: the subdirectories are never seen, so recursion never
happens and the file list stays empty.

**Rule:** when a scan must recurse into subdirectories, always call `dir(path)`
with **no filter**, then test the extension manually on non-directory entries:

```javascript
function scanDir(path) {
    var entries = dir(path);          // no filter — directories are included
    for (var i = 0; i < entries.length; ++i) {
        var e = entries[i];
        if (e.isDirectory) {
            scanDir(path + e.name + '/');
        } else if (/\.mtpreset$/i.test(e.name)) {
            // handle file
        }
    }
}
```

Using an extension filter — e.g. `dir(path, 'mtdrum')` — is safe only for
known-flat directories (such as `DIRS.DRUM_PATCHES + 'All/'`) where no
subdirectories need to be traversed.
