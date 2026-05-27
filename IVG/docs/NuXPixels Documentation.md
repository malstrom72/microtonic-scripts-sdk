# NuXPixels

## Table of Contents
- [Intro](#intro)
- [Quickstart](#quickstart)
- [Core Data Types](#core-data-types)
	- [Points and Rectangles](#points-and-rectangles)
	- [Pixel Formats](#pixel-formats)
	- [Color Math](#color-math)
	- [AffineTransformations](#affinetransformations)
- [Rendering Pipeline](#rendering-pipeline)
	- [Raster and Renderer](#raster-and-renderer)
	- [PolygonMask](#polygonmask)
	- [Solid and Texture](#solid-and-texture)
	- [Gradients](#gradients)
	- [RLERaster](#rleraster)
- [Operator Overloading](#operator-overloading)
- [Pull Model](#pull-model)
- [Lifetime of Renderers](#lifetime-of-renderers)
- [Path Construction](#path-construction)
- [Limits and Safety](#limits-and-safety)
- [Examples and Recipes](#examples-and-recipes)
## Intro

_NuXPixels_ is a small C++ library for 2D graphics rendering. It is designed to be self contained
 with no operating system dependencies and focuses on a minimal API with high quality
 anti‑aliasing. The renderer operates entirely in memory.

## Quickstart

```cpp
#include "NuXPixels.h"
using namespace NuXPixels;

int main() {
       SelfContainedRaster<ARGB32> canvas(IntRect(0, 0, 64, 64));
       Path rect;
       rect.addRect(IntRect(8, 8, 48, 48));
       PolygonMask mask(rect, canvas.calcBounds());
       canvas |= Solid<ARGB32>(0xFFFF0000) * mask; // fill red square

       FILE* f = fopen("out.ppm", "wb");
       fprintf(f, "P6\n64 64\n255\n");
       const ARGB32::Pixel* p = canvas.getPixelPointer();
       for (int i = 0; i < 64 * 64; ++i) {
               unsigned char rgb[3] = {
                       static_cast<unsigned char>(p[i] >> 16),
                       static_cast<unsigned char>(p[i] >> 8),
                       static_cast<unsigned char>(p[i])
               };
               fwrite(rgb, 1, 3, f);
       }
       fclose(f);
       return 0;
}
```

Compile with `g++ example.cpp -std=c++17` (or a similar C++17 compiler command).

## Core Data Types

### Points and Rectangles

`NuXPixels` defines generic `Point<T>` and `Rect<T>` templates for integer and floating point
coordinates. Convenience typedefs such as `IntPoint`, `IntRect` and `Vertex` (a double precision
point) are available for common use.

`Rect<T>` uses half‑open bounds `[left, left+width) × [top, top+height)` and expects non‑negative
`width` and `height`. When rectangles only touch at an edge the intersection is empty. `IntRect`
stores 31‑bit signed coordinates (`FULL_RECT` spans ±0x40000000) while floating variants rely on
`double`. `Rect<T>` provides helpers like `offset`, `calcUnion` and `calcIntersection` to
manipulate regions. These operations simplify clipping logic:

```cpp
IntRect a(0, 0, 50, 50);
IntRect b(20, 20, 10, 10);
IntRect clipped = a.calcIntersection(b);
```

### Pixel Formats

The library ships with a few pixel formats. `ARGB32` stores premultiplied 8‑bit channels in
`0xAARRGGBB` order. A lightweight `Mask8` type carries 8‑bit coverage when rendering masks.

### Color Math

Pixels are interpreted as sRGB and all arithmetic happens in that gamma space; no linear conversion
is performed. Blending follows the conventional `dst * (255 - src.a) / 255 + src` equation on
premultiplied channels, where `src.a` is the source alpha (0–255). Intermediate arithmetic uses
integers with truncation, so channel multiplications round toward zero. `ARGB32` provides utilities
such as `add`, `multiply` and `interpolate` for pixel arithmetic. A color can be constructed from
floats and then modulated:

```cpp
ARGB32::Pixel p = ARGB32::fromFloatRGB(1.0, 0.0, 0.0, 0.5); // 50% red
ARGB32::Pixel dark = ARGB32::multiply(p, 128);               // scale alpha & rgb by 128/255
// If using a literal, red opaque is 0xFFFF0000, blue is 0xFF0000FF.
```

### AffineTransformations

`AffineTransformation` represents 2×3 matrices for translation, scaling, rotation and shearing.
Transformations can be composed with the `transform()` function and applied to vertices or paths.

Chained calls build complex transforms in a readable manner:

```cpp
AffineTransformation t = AffineTransformation()
	.translate(20, 10)
	.scale(2)
	.rotate(0.25 * M_PI);
```

Transforms are pre‑multiplied (points are column vectors), so calls execute left‑to‑right:
`translate().scale().rotate()` means translate, then scale, then rotate in object space. Angles are
specified in radians.

## Rendering Pipeline

### Raster and Renderer

All drawing is expressed through `Renderer` templates that generate spans of pixel data. A span
models a contiguous horizontal run. When marked *solid* the span stores one pixel value repeated
for the entire run; otherwise it carries an array of per-pixel values so coverage or color may vary
across the span. Each span also flags opaque or transparent runs to enable culling. Spans longer
than `MAX_RENDER_LENGTH` are split automatically. `Raster<T>` collects the result in a client
supplied buffer while `SelfContainedRaster<T>` manages its own memory. A typical pipeline blends
color data from `Renderer<ARGB32>` through coverage masks produced by `Renderer<Mask8>` sources:

```cpp
ARGB32::Pixel pixels[1024 * 1024];
Raster<ARGB32> view(pixels, 1024, IntRect(0, 0, 1024, 1024), false);
Path rect;
rect.addRect(IntRect(100, 100, 200, 200));
PolygonMask mask(rect, view.calcBounds());
view |= Solid<ARGB32>(0xFFFF0000) * mask;
```

### PolygonMask

`PolygonMask` is the workhorse renderer for filling paths. Given a vector `Path` and an optional
fill rule, it converts the geometry into a scanline coverage mask(`Renderer<Mask8>`). Paint sources
such as solid colors or gradients are then blended through this mask onto the destination raster.
Scanlines are processed sequentially from top to bottom. Coverage values range 0–255 with 8‑bit
subpixel precision, and both even‑odd and non‑zero winding rules are supported. The constructor
accepts an optional clip rectangle (defaulting to `FULL_RECT`), which is clamped to the maximum
coordinate range and applied before rasterization.

Further details on the algorithm, design trade‑offs and pseudo‑code are available in `PolygonMask Rasterizer.md`.

### Gradients

A `Gradient` lookup table produces color values for linear or radial fills. `LinearAscend` and
`RadialAscend` generate a 0–255 coverage ramp that can index into a gradient: `gradient
[LinearAscend(x0, y0, x1, y1)]` or `gradient[RadialAscend(cx, cy, rx, ry)]` yield a color renderer.
The library also provides a `GammaTable` for simple tone adjustments.

Stops must be supplied in ascending order and are interpolated in premultiplied space. The ramp
spans indices `0–255` inclusive and clamps at the ends; gradients neither repeat nor reflect. Large
color steps may band since no dithering is applied.

```cpp
Gradient<ARGB32>::Stop stops[] = {
	{0.0, 0xFF0000FF},// blue
	{1.0, 0xFFFFFFFF}// white
};
Gradient<ARGB32> grad(2, stops);
canvas |= grad[LinearAscend(0, 0, 0, 100)];

Gradient<ARGB32>::Stop stops2[] = {
{0.0, 0xFFFF0000},// red
{1.0, 0xFFFFFFFF}
};
```

See [Lifetime of Renderers](#lifetime-of-renderers) for notes on gradient lookups referencing their
ramps.


### Solid and Texture

`Solid<T>` outputs a constant pixel value. `Texture<T>` samples from a raster using an affine
transformation and optional wrapping (repeat when `wrap=true`, otherwise clamp to transparent
black). Sampling uses 16.16 fixed-point coordinates with bilinear filtering.

```cpp
Texture<ARGB32> tex(image, true, AffineTransformation().scale(0.5));
canvas |= tex * mask; // Defaults: bilinear filter, wrap repeat
// Clamp with wrap=false to sample transparent black; sampling happens in premultiplied space.
```

### RLERaster

`RLERaster<T>` stores spans in run-length encoded form for reuse. It is handy for caching masks so
that complex paths need not be rasterized repeatedly.

```cpp
RLERaster<Mask8> cache(area, mask);
canvas |= Solid<ARGB32>(color) * cache;
```

Runs encode either a solid pixel or a block of per‑pixel data. Each span stores a 16‑bit header plus
optional pixel payload, preserving partial alpha. Compression ratio depends on image
coherence—solid regions compress heavily while noisy images approach raw size.

## Operator Overloading

Renderers can be combined with `*`, `+`, `|`, `+=`, `*=`, and `|=` operators. Each operator returns
another `Renderer` that lazily requests spans from its inputs. An expression like `canvas |=
Solid<ARGB32>(color) * mask` forms a small pipeline. Operator precedence follows C++ rules: `*`
binds tighter than `|`. The `|` operator blends the right renderer over the left; it is not a
bitwise OR.

## Pull Model

As the canvas renders, it pulls spans from the expression and each stage only computes what the next
stage requires. Because drawing is demand driven, NuXPixels can optimize away work in real time.
Opaque spans automatically block processing of any renderers beneath them since those pixels are
invisible. This culling happens per span and keeps the renderer efficient even with many layers.

## Lifetime of Renderers

Most renderer types store references to the objects passed into their constructors or operators. C++
destroys temporary objects at the end of the statement, so a renderer built from temporaries must
also be used in that same statement. To keep a renderer for later, create and store every component
separately so their lifetimes extend as needed.

```cpp
Gradient<ARGB32>::Stop stops[] = {{0.0, 0xff0000ff}, {1.0, 0xffffffff}};
Gradient<ARGB32> grad(2, stops);
LinearAscend ramp(x0, y0, x1, y1);
Lookup<ARGB32, LookupTable<ARGB32> > lookup = grad[ramp];
canvas |= lookup; /// `grad` and `ramp` must outlive `lookup`

canvas |= Gradient<ARGB32>(2, stops)[LinearAscend(x0, y0, x1, y1)]; /// safe: everything is temporary
```

This rule applies to all expressions in NuXPixels—`PolygonMask`, `Texture`, `Solid`, gradients and
more. Either chain the full expression in a single statement or keep each renderer alive for as
long as any derived renderer uses it.

Even types that are trivially copyable still reference external data; treat renderers as views
rather than owning objects.

| Renderer | References | Ownership |
|---|---|---|
| Solid<T> | none | value |
| Texture<T> | source `Raster<T>` | no |
| PolygonMask | `Path`, `FillRule` | no |
| Gradient<T> | copy of stops | table |
| Gradient<T>::Lookup | gradient, ramp | no |
| RLERaster<T> | none | owns span/pixel arrays |

## Path Construction

`Path` records drawing commands such as `moveTo`, `lineTo`, `quadraticTo`, `cubicTo`, `arcSweep` and
`close`. Convenience helpers like `addRect`, `addEllipse`, `addCircle`, `addRoundedRect` and
`addStar` append common shapes. `stroke` replaces the path with its stroked outline while `dash`
rewrites segments to alternate drawn and skipped portions. All operations modify the path in place
but return `*this` for chaining. Paths operate in double precision and can be transformed with an
`AffineTransformation` before rendering.

```cpp
Path star;
star.addStar(40, 40, 5, 20, 10, 0);
star.stroke(3.0, Path::ROUND, Path::MITER);
star.dash(5.0, 2.0);
```

`stroke(width, endCaps, joints, miterLimit, curveQuality)` defaults to a miter limit of `2.0`.
`EndCapStyle` may be `BUTT`, `ROUND`, or `SQUARE`; `JointStyle` is `BEVEL`, `CURVE`, or `MITER`.
`dash(dashLength, gapLength, dashOffset)` alternates drawn and skipped segments starting at
`dashOffset`; fractional lengths accumulate along each sub-path and the pattern resets at every
`moveTo`.

## Limits and Safety

- Maximum span length is 256 pixels (`MAX_RENDER_LENGTH`); longer runs are split automatically.
- `IntRect` uses 31‑bit signed coordinates (`FULL_RECT`); floating types use `double` values.
- Texture sampling outside the source repeats when `wrap=true` and returns transparent black
  (`0x00000000`) when `wrap=false`.
- Renderers and rasters are not thread‑safe; use separate instances on different threads.
- Requesting scanlines out of order forces `PolygonMask` to rewind and resort edges, which is slower
  than sequential rendering.
- `RLERaster` compresses runs; memory usage varies with image content.
- Paths with fewer than two points or zero-length segments yield zero coverage.
- Color and coverage calculations use 8‑bit integer arithmetic with truncation.
- Many routines assume coordinates roughly within -32768 to 32767; exceeding that range can overflow
  internal 16-bit accumulators or lose precision. `IntRect` stores 31-bit signed coordinates
  (`FULL_RECT` spans ±0x40000000); exceeding this may overflow.
- Functions do not guarantee `noexcept` and may fail on allocation.

## Examples and Recipes

A short example shows how the operator overloads work together:

```cpp
using namespace NuXPixels;

SelfContainedRaster<ARGB32> canvas(IntRect(0, 0, 64, 64));

Path rect;
rect.addRect(IntRect(8, 8, 48, 48));
PolygonMask mask(rect, canvas.calcBounds());

// Multiply the coverage mask with a solid color and alpha-blend onto the canvas
canvas |= Solid<ARGB32>(0xFFFF0000) * mask;
```

Here `*` multiplies the color renderer with the `PolygonMask`, producing `Renderer<ARGB32>` spans
masked by the polygon coverage. The resulting renderer is then blended onto `canvas` with `|=`. The
entire expression becomes a pull pipeline: the canvas requests pixels, which asks the mask for
coverage, which in turn iterates the path only for the visible spans. Similar expressions can chain
gradients or multiple masks together.

This program (including file output) is available as `tests/red_square.cpp`.

Another example uses a texture and gradient:

```cpp
SelfContainedRaster<ARGB32> texCanvas(IntRect(0, 0, 64, 64));
Gradient<ARGB32> grad(ARGB32::transparent(), 0xFF00FF00); // 0xAARRGGBB: green
texCanvas |= grad[RadialAscend(32, 32, 32, 32)];

Texture<ARGB32> tex(texCanvas, true);
canvas |= tex * mask;
```

