# SVG Support in svg2ivg

## Supported SVG Features

### Elements

-   `svg` root element with `width`, `height` (`px`, `%`, `cm`, `mm`, `in`, `pt`, `pc`) and `viewBox` for basic scaling/offset.
-   `g` groups, emitted as `context` blocks to propagate presentation attributes.
-   Shapes: `path` (`d`), `circle` (`cx`, `cy`, `r`), `ellipse` (`cx`, `cy`, `rx`, `ry`), `line` (`x1`, `y1`, `x2`, `y2` as a path), `rect` (`x`, `y`, `width`, `height`, optional `rx`/`ry` rounded corners), `polygon` (`points`), and `polyline` (`points`).
-   Gradient fills and strokes via `linearGradient` and `radialGradient` definitions referenced with `url(#id)`, including `gradientTransform`.
-   Pattern fills and strokes via `pattern` definitions referenced with `url(#id)`.
-   Clipping paths via `clipPath` definitions referenced with `clip-path`.
-   Masking via `mask` definitions referenced with `mask`.
-   Basic `text` elements (`x`, `y`, `font-size`, `font-family`, `text-anchor`, `fill`, `stroke`).
-   Reuse via `defs`/`use` elements.

### Units

-   Percentage lengths resolved against the root `svg` dimensions.

### Presentation Attributes

-   `stroke`, `stroke-width`, `stroke-linejoin`, `stroke-linecap`, `stroke-miterlimit`, `stroke-dasharray`, `stroke-dashoffset`, `fill`.
-   `stroke-linejoin` values: `bevel`, `round`, `miter`, `miter-clip`.
-   `stroke-linecap` values: `butt`, `round`, `square`.
-   `opacity`, `stroke-opacity`, `fill-opacity`, `visibility`.
-   Inline `style` attribute for presentation properties.

### Transforms

-   `transform` attribute with `translate`, `scale`, `rotate`, `skewX`, `skewY`, and `matrix` operations.

### Colors

-   Hex color literals `#rrggbb` or `#rgb`.
-   Named colors from the CSS/SVG color list (e.g. `lightblue`, `darkgreen`).
-   `rgba()` and `hsla()` color functions.
-   `none` to disable stroke or fill.

## Unsupported or Partial Features

-   Additional SVG elements (`image`, etc.).
-   `preserveAspectRatio` handling.
-   CSS class selectors and `<style>` elements for styling.
-   Color functions like `rgb()`.
-   `stroke-linejoin="arcs"` (undefined in SVG 1.x).
-   Any `viewBox` behavior beyond a simple uniform scale and top-left offset.
-   Error recovery for missing attributes—many attributes treated as required even though the SVG spec provides defaults.

This list reflects the current state of `tools/svg2ivg.pika` and may change as the converter evolves.
