# Cushy

_Cushy_ is the GUI / layout engine used in all Sonic Charge products. A `.cushy` file describes a window's layout as a
tree of _views_, and wires those views to your script through _GUI variables_ and _GUI actions_. This document is an
orientation: it explains the mental model and shows where each piece is documented in full. It does not duplicate the
field-by-field reference — that lives in [`cushy.schema`](../CushyLint/cushy.schema).

> Microtonic 3.x is only partially ported to Cushy. Most standard knobs and buttons are legacy "hard-wired" elements,
> with a layer of Cushy on top. You write scripts against that Cushy layer.

## Where Cushy is documented

There is no single exhaustive Cushy manual, by design. The format is large and validated against a schema, so the
schema is the canonical syntax reference and everything else routes to it:

| If you need...                                              | Read                                                                                            |
| ----------------------------------------------------------- | ----------------------------------------------------------------------------------------------- |
| The mental model and a reading order (this page)            | this document                                                                                   |
| Exact fields for a view, action, color, font, or style      | [`CushyLint/cushy.schema`](../CushyLint/cushy.schema) — the official reference                  |
| How `.cushy` connects to JavaScript (variables and actions) | [Microtonic JS Reference](Microtonic%20JS%20Reference.md) → _Cushy Interface_                   |
| The macro syntax used in `.cushy` files (`@define`, etc.)   | [Makaron Documentation](Makaron%20Documentation.md)                                             |
| Vector graphics drawn inside views                          | [IVG Documentation](IVG%20Documentation.md) and [ImpD Documentation](ImpD%20Documentation.md)   |
| Working code to copy from                                   | the packages under [`examples/`](../examples) and [`JSConsole.mtscript`](../JSConsole.mtscript) |

The schema is written to be read: it carries inline comments alongside every rule and "serves as a kind of official
reference documentation on `.cushy` files, available view types, and built-in actions." When exact syntax or supported
fields matter, the schema is the source of truth.

## The mental model

A `.cushy` file is a _Numbstrict_ document — a JSON-like notation with C-style comments and a few extensions — that is
expanded by Makaron macros before parsing. Conceptually it has five top-level sections:

```
bounds:      { left, top, width, height }   // the window rectangle (main .cushy only)
autoexecs:   { ... }                        // actions run on open, close, a timer, or a variable change
transitions: { ... }                        // open/close animations
translations:{ ... }                        // string translations for this Cushy
views:       { ... }                        // the view tree — the actual UI
```

Three ideas tie the system together:

1. **Views nest into a tree.** Each view has `bounds` relative to its parent, and Cushy **clips hard at every view's
   bounding box** — nothing outside a view's bounds is drawn or clickable. Common view types include `group`, `caption`,
   `rectangle`, `button`, `knob`, `slider`, `click`, `hover`, `vector`, and `cluster`; the full list and every field is
   in the schema.

2. **GUI variables synchronize view state with your script.** A knob bound to `variable: myScript.cutoff` writes
   `myScript.cutoff` when the user turns it, and continuously reads it to redraw. Variables are always strings. A bound
   name can be a plain JS string, a getter function, an object with `get`/`set`/`touch`, or a Cushy-only variable. The
   exact semantics and built-in variable names are in the JS Reference's _Cushy Interface_.

3. **GUI actions run script code from the UI.** A clicked button or menu item performs an _action_ — a built-in
   (`set`, `popup`, `edit`, `reload`, ...) or a JavaScript function you provide (optionally an object with
   `execute`/`enabled`/`checked`). Actions can also be triggered by `autoexecs`: on open, on close, on a timer, or when
   a variable changes.

### Startup and reload

A GUI script is a `<Name>.mtscript/` directory. Its entry `<Name>.js` calls `toggleCushy("<Name>.mtscript/<Name>_main")`
to open the window. When Cushy opens `<Name>_main.cushy`, it first runs the same-named `<Name>_main.js`, which defines
the script's variables and action functions.

Cushy **reruns that JavaScript on every reload** — and reload is not only a developer action: changing Microtonic's zoom
scale also forces a rebuild. So scripts must survive their JS being re-run: keep persistent state under a single object
named after the script and guard its initialization (`if (!myScript) { ... }`) so existing globals are not reset. The
reload/reset/close lifecycle is detailed in the JS Reference's _Cushy Interface_.

## Coordinates, expressions, and scaling

**Coordinates are always integers.** View bounds and offsets are whole pixels, and they stay whole pixels after the
user's zoom level and high-DPI scaling are applied — Cushy rescales to integer positions and never places a view on a
fractional pixel.

**Microtonic scales the whole UI for the end user.** The zoom menu offers 50%, 75%, 100%, 125%, 150%, 175%, and 200%,
and the display may be high-DPI / Retina (2x) or standard (1x). Each of these is a scale factor applied to your integer
layout. Changing zoom forces a full GUI rebuild — the same reload that reruns your `_main.js` — so the layout must
survive reloads (see [Startup and reload](#startup-and-reload)).

> **The 4-pixel grid.** The zoom steps are quarter increments, so the common denominator is 4. The one alignment that
> survives *every* zoom level and both DPI modes without rounding is a 4-pixel grid: a feature placed on a 4-pixel
> boundary rescales to an exact integer at 50%, 75%, 125%, 175%, and so on, while a 1-, 2-, or 3-pixel detail can land
> between pixels at some zoom levels and shift by a pixel. For tightly aligned, abutting views, align on multiples of 4.

**Math expressions are evaluated by Cushy, at construction.** Where a numeric constant is expected you may write an
expression — for example `bounds: { l+10, t+10, w-20, h-20 }` or `updateRate: $*2`, using `$` for the field's default
value and `l`/`t`/`w`/`h`/`r`/`b` for the parent rectangle (see the [README](../README.md#cushy)). These are evaluated
by **Cushy when it builds the view tree** — *not* by Makaron (which only does text substitution before parsing) and
*not* by JavaScript. The result is a fixed number baked into the view at construction time.

**Most properties are static between rebuilds.** Cushy reads view properties once when it builds the tree; they do not
change again until the next reload/rebuild. Beyond that, dynamism comes in two grades, and it is worth deliberately
choosing the more stable one:

- **Live updates without recreating views (preferred).** Fields explicitly designed to be dynamic accept a GUI variable
  (`<var>`) in the schema and update in place while the window is open. A `group`, for example, can move and show or hide
  its contents via `offset: { <var>, <var> }` and `visibility: <var>` (the schema notes these "do not recreate views on
  change"). This is cheap and stable. By contrast, a `caption`'s position or a `knob`'s `bounds` have no variable form
  and stay static.

- **Rebuilding views from a source variable (`varExpansion`, least stable).** A `group` with `varExpansion: 'true'`
  treats its `views:` as a source string with `[var]myDynamicSource[/var]` interpolation, letting a script generate
  arbitrary view configurations at run time. The catch is that those views are torn down and recreated *every time the
  source variable changes* — the schema even notes CushyLint cannot check views under `varExpansion`. It is the most
  flexible and the least stable option; reserve it for cases that genuinely need a script-generated view tree.

  **`[var]` expansion does not nest.** Expansion is literally a single pass that rebakes the whole group's `views:` text
  and reparses it as a Cushy subtree — there is no second pass. So a child inside a `varExpansion` group cannot itself
  rely on `[var]…[/var]` (a knob `hint`, a `meta` text, an indirect `execute` action, etc.); the inner markers are
  consumed or broken by the one outer expansion. If you need `[var]`-driven content *inside* a var-expanding group, move
  that content out of the expanded subtree, or have the script bake the inner value into the generated source string
  directly instead of leaving a `[var]` marker for a second pass that never happens.

When something must change live, look for an in-place `<var>` field first and design around a view that has one (often a
`group`). Fall back to `varExpansion` only when you truly need a script-generated tree, and expect a full rebuild only
when nothing else fits.

**Raster resources ship at 2x.** PNG resources are referenced by base name in `.cushy` (`name: "myGraphic"` or
`file: "myGraphic"`); Microtonic resolves the file and scales it to the current zoom and DPI. Provide a high-resolution
master named `<base>_x2.png` — the convention the shipped resources use, where nearly every built-in PNG exists only as
`_x2`. **`_x4` masters are a later-Cushy addition and are not used by Microtonic 3.3.4**, so target `_x2` for Microtonic
scripts. Vector (`vector` / IVG) graphics and `caption` text scale cleanly at any zoom and sidestep the resolution
question entirely; prefer them where crisp graphics matter.

## A minimal window

The smallest useful `.cushy` includes the shared support macros and uses the `@window` macro for standard window chrome
(shadow, frame, title, close button), then declares its content views. The fragment below — one knob bound to a script
variable, with a caption — mirrors the [`MacroTweak`](../examples/MacroTweak.mtscript) example:

```makaron
@include scriptSupport.makaron
@define scriptRoot=MyScript.mtscript
@define script=myScript

@window(280, 200, 180, 120, "My Script", @backgroundColor, @frameColor, white, @script, @<
    {
        type: "knob"
        bounds: { 60, 20, 60, 40 }
        variable: @script.cutoff
        range: { 0, 1 }
        cap: { size: 40, color: "white", frame: { color: "white", stroke: 1 } }
        hint: "Cutoff = [var]@script.cutoff.human[/var]"
    }
    {
        type: "caption"
        bounds: { 60, 65, 60, 15 }
        text: "Cutoff"
        font: { ivgfont: "sans-serif", size: 14, color: "white" }
        align: "center"
    }
@>)
```

The matching `MyScript_main.js` would define `myScript` with a `cutoff` value (guarded so it survives reloads). For the
full window structure, `transitions:`/`autoexecs:` blocks, and how `@window` lays out its content, read the example
packages end to end — they are the canonical worked references.

A few things that trip up first-time authors, all expanded in the schema or example packages:

- **All text needs an explicit `font`** — Cushy has no default font, so text with no `font` is simply not drawn.
- **Coordinates are clipped at view bounds** — a child cannot draw or receive clicks outside its parent.
- **Colors use normalized components** — `rgb(1,1,1,0.35)`, not `rgb(255,255,255,...)`.
- **Keep `.cushy` source ASCII** unless you have tested the exact character path.

## Validating what you write

Numbstrict is easy to get subtly wrong in deeply nested view trees, so run **CushyLint** against your files before
loading them in Microtonic:

```sh
CushyLint/CushyLint "$(pwd)/MyScript.mtscript/" "$(pwd)/Microtonic Resources"
```

Pass a directory path **with a trailing slash** so the companion `.schema` files are picked up. CushyLint checks syntax
and schema conformance; it does not predict runtime performance — measure that in Microtonic. See the README's
[Cushy](../README.md#cushy) section for CushyLint details and [`cushy.schema`](../CushyLint/cushy.schema) for what it
validates against.
