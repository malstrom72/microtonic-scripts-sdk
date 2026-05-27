/**
	NuXPixels is released under the BSD 2-Clause License.

	Copyright (c) 2013-2025, Magnus Lidström

	Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
	following conditions are met:

	1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following
	disclaimer.

	2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following
	disclaimer in the documentation and/or other materials provided with the distribution.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
	INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
	DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
	SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
	SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
	WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
	OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**/
#include <math.h>
#include <algorithm>
#include "NuXPixels.h"
#include "NuXPixelsImpl.h"
#if (NUXPIXELS_SIMD)
#include "NuXSIMD.h"
#endif

namespace NuXPixels {

#if (NUXPIXELS_SIMD)
using namespace NuXSIMD;
#endif

static const int POLYGON_FRACTION_BITS = 8;
static const int FRACT_BITS = POLYGON_FRACTION_BITS;
static const int FRACT_MASK = ((1 << FRACT_BITS) - 1);
static const int FRACT_ONE = (1 << FRACT_BITS);
static const int COVERAGE_BITS = 8;

template class Point<int>;
template class Point<double>;
template class Rect<int>;
template class Rect<double>;
template class Raster<ARGB32>;
template class Raster<Mask8>;
template class SelfContainedRaster<ARGB32>;
template class SelfContainedRaster<Mask8>;
template class Span<ARGB32>;
template class Span<Mask8>;
template class RLERaster<ARGB32>;
template class RLERaster<Mask8>;
template class Renderer<ARGB32>;
template class Renderer<Mask8>;
template class Solid<ARGB32>;
template class Solid<Mask8>;
template class SolidRect<ARGB32>;
template class SolidRect<Mask8>;
template class Clipper<ARGB32>;
template class Clipper<Mask8>;
template class Offsetter<ARGB32>;
template class Offsetter<Mask8>;
template class Blender<ARGB32>;
template class Blender<Mask8>;
template class Adder<ARGB32>;
template class Adder<Mask8>;
template class Multiplier<ARGB32, ARGB32>;
template class Multiplier<ARGB32, Mask8>;
template class Multiplier<Mask8, Mask8>;
template class Inverter<ARGB32>;
template class Inverter<Mask8>;
template class Converter<ARGB32, Mask8>;
template class Converter<Mask8, ARGB32>;
template class Texture<ARGB32>;
template class Texture<Mask8>;
template class LookupTable<ARGB32>;
template class LookupTable<Mask8>;
template class Lookup< ARGB32, LookupTable<ARGB32> >;
template class Lookup< Mask8, LookupTable<Mask8> >;
template class Gradient<ARGB32>;
template class Gradient<Mask8>;
template class Optimizer<ARGB32>;
template class Optimizer<Mask8>;

const IntRect EMPTY_RECT(0, 0, 0, 0);
const IntRect FULL_RECT(-0x40000000, -0x40000000, 0x7FFFFFFF, 0x7FFFFFFF);

/**
	Number of divisions of a circle for rounded joints etc. Notice that going beyond 200 divisions
	doesn't make much difference, even if the end-result is magnified considerably. This is
	because the angle between the segments is independent of the scaling of the circle.
**/
const double MAX_CIRCLE_DIVISIONS = 200.0;
const double MIN_CIRCLE_DIVISIONS = 8.0;	// smallest circle-like shape: octagon

const int MAX_SPLINE_SEGMENTS = 200;

/* --- Utility routines --- */

static inline void sort(int& a, int& b) {
	const int x = a;
	const int y = b - a;
	const int z = (y >> 31);
	a = x + (y & z);
	b = x + (y & ~z);
}

static double calcCircleRotationVector(double curveQuality, double diameter, double& rx, double& ry) {
	const double t = (diameter < EPSILON ? PI2
			: minValue(maxValue(1.0 / sqrt(curveQuality * diameter), PI2 / MAX_CIRCLE_DIVISIONS)
			, PI2 / MIN_CIRCLE_DIVISIONS));
	rx = cos(t);
	ry = sin(t);
	return t;
}

#ifndef NUXPIXELS_INT64_TYPE

inline Fixed32_32 divide(Int32 v1, Int32 v2) throw()
{
	const UInt32 av1 = std::abs(v1);
	const UInt32 av2 = std::abs(v2);
	UInt32 i = 0;
	UInt32 f = 0;
	UInt32 r = av1;

	// Integer part first, keep the remainder.

	if (av1 >= av2) {
		i = av1 / av2;
		r -= i * av2;
	}
	
	// We will now divide the remainer with av2, try to utilize integer division first.

	int l = 32;
	int c = 16; // Start with 16-bit precision, since we will need at least two iterations anyhow.
	do {
		while (r >> (32 - c) != 0) {
			--c;
		}
		r <<= c;
		const UInt32 x = r / av2;
		r -= x * av2;
		f = (f << c) | x;
		l -= c;
		c = minValue(l, c);
	} while (c >= 4);
	
	// When precision is less than 4 bits, we consider it quicker to do bit by bit.

	while (l > 0) {
		r += r - av2;
		const int x = (static_cast<int>(r) >> 31); // x is now -1 if r is < 0 and 0 if r is >= 0
		f += f + 1 + x; // Shift f one bit left and add 1 if x r is >= 0.
		r += av2 & x; // Restore previous r if r < 0 by adding av2.
		--l;
	}
	
	// If one (and only one) of v1 or v2 is negative, result is negative.

	return ((v1 ^ v2) < 0) ? negate(Fixed32_32(i, f)) : Fixed32_32(i, f);
}

#endif // #ifndef NUXPIXELS_INT64_TYPE

/* --- ARGB32 -- */

#if (NUXPIXELS_SIMD)
ARGB32::Pixel* ARGB32::allocate(int count) { assert(count >= 0); return NuXSIMD::allocateAligned<ARGB32::Pixel>(count); }
void ARGB32::free(Pixel* pixels) { assert(pixels != 0); return NuXSIMD::freeAligned(pixels); }
#else
ARGB32::Pixel* ARGB32::allocate(int count) { assert(count >= 0); return new ARGB32::Pixel[count]; }
void ARGB32::free(Pixel* pixels) { assert(pixels != 0); delete [] pixels; }
#endif

ARGB32::Pixel ARGB32::fromFloatRGB(double r, double g, double b, double a) {
	assert(0.0 <= r && r <= 1.0);
	assert(0.0 <= g && g <= 1.0);
	assert(0.0 <= b && b <= 1.0);
	assert(0.0 <= a && a <= 1.0);

	const UInt32 rgb
			= (static_cast<UInt32>(r * 255.99999999) << 16)
			| (static_cast<UInt32>(g * 255.99999999) << 8)
			| (static_cast<UInt32>(b * 255.99999999) << 0);
	return multiply(0xFF000000 | rgb, static_cast<Mask8::Pixel>(a * 255.99999999));
}

ARGB32::Pixel ARGB32::fromFloatHSV(double h, double s, double v, double a) {
	assert(0.0 <= h && h <= 1.0);
	assert(0.0 <= s && s <= 1.0);
	assert(0.0 <= v && v <= 1.0);
	assert(0.0 <= a && a <= 1.0);

	/* TODO : find more optimal algorithm */
	Pixel c;
	if (v == 0.0) {
		c = 0;
	} else if (s == 0.0) {
		c = static_cast<int>(v * 255.99999999);
		c |= (c << 16) | (c << 8);
	} else {
		const double h6 = ((h == 1.0) ? 0.0 : (h * 6.0));
		const int i = static_cast<int>(h6);
		const double f = h6 - i;
		const UInt32 w = static_cast<UInt32>(v * 255.99999999);
		const UInt32 p = static_cast<UInt32>(v * (1.0 - s) * 255.99999999);
		const UInt32 q = static_cast<UInt32>(v * (1.0 - s * f) * 255.99999999);
		const UInt32 t = static_cast<UInt32>(v * (1.0 - s * (1.0 - f)) * 255.99999999);
		switch (i) {
			case 0: c = ((w << 16) | (t << 8)) | p; break;
			case 1: c = ((q << 16) | (w << 8)) | p; break;
			case 2: c = ((p << 16) | (w << 8)) | t; break;
			case 3: c = ((p << 16) | (q << 8)) | w; break;
			case 4: c = ((t << 16) | (p << 8)) | w; break;
			case 5: c = ((w << 16) | (p << 8)) | q; break;
			default: assert(0); return 0;
		}
	}
	return multiply(0xFF000000 | c, static_cast<Mask8::Pixel>(a * 255.99999999));
}

/* --- Mask8 -- */

#if (NUXPIXELS_SIMD)
Mask8::Pixel* Mask8::allocate(int count) { assert(count >= 0); return NuXSIMD::allocateAligned<Mask8::Pixel>(count); }
void Mask8::free(Pixel* pixels) { assert(pixels != 0); return NuXSIMD::freeAligned(pixels); }
#else
Mask8::Pixel* Mask8::allocate(int count) { assert(count >= 0); return new Mask8::Pixel[count]; }
void Mask8::free(Pixel* pixels) { assert(pixels != 0); delete [] pixels; }
#endif

/* --- AffineTransformation --- */

AffineTransformation::AffineTransformation()
{
	matrix[0][0] = 1.0f;
	matrix[0][1] = 0.0f;
	matrix[0][2] = 0.0f;
	matrix[1][0] = 0.0f;
	matrix[1][1] = 1.0f;
	matrix[1][2] = 0.0f;
}

AffineTransformation::AffineTransformation(const double matrix[2][3])
{
	std::copy(&matrix[0][0], &matrix[2][3], &this->matrix[0][0]);
}

AffineTransformation::AffineTransformation(double m00, double m01, double m02, double m10, double m11, double m12)
{
	matrix[0][0] = m00;
	matrix[0][1] = m01;
	matrix[0][2] = m02;
	matrix[1][0] = m10;
	matrix[1][1] = m11;
	matrix[1][2] = m12;
}

AffineTransformation AffineTransformation::translate(double x, double y) const
{
	return AffineTransformation
			( matrix[0][0], matrix[0][1], matrix[0][2] + x
			, matrix[1][0], matrix[1][1], matrix[1][2] + y
			);
}

AffineTransformation AffineTransformation::scale(double x, double y) const
{
	return AffineTransformation
			( matrix[0][0] * x, matrix[0][1] * x, matrix[0][2] * x
			, matrix[1][0] * y, matrix[1][1] * y, matrix[1][2] * y
			);
}

AffineTransformation AffineTransformation::scale(double v) const
{
	return scale(v, v);
}

AffineTransformation AffineTransformation::rotate(double t) const
{
	double c = cos(t);
	double s = sin(t);
	return AffineTransformation
			( c * matrix[0][0] - s * matrix[1][0], c * matrix[0][1] - s * matrix[1][1], c * matrix[0][2] - s * matrix[1][2]
			, s * matrix[0][0] + c * matrix[1][0], s * matrix[0][1] + c * matrix[1][1], s * matrix[0][2] + c * matrix[1][2]
			);
}

AffineTransformation AffineTransformation::rotate(double t, double x, double y) const
{
	double c = cos(t);
	double s = sin(t);
	return AffineTransformation
			( c * matrix[0][0] - s * matrix[1][0], c * matrix[0][1] - s * matrix[1][1], c * matrix[0][2] - s * matrix[1][2] + x - x * c + y * s
			, s * matrix[0][0] + c * matrix[1][0], s * matrix[0][1] + c * matrix[1][1], s * matrix[0][2] + c * matrix[1][2] + y - x * s - y * c
			);
}

AffineTransformation AffineTransformation::shear(double x, double y) const
{
	return AffineTransformation
			( matrix[0][0] + matrix[1][0] * x, matrix[0][1] + matrix[1][1] * x, matrix[0][2] + matrix[1][2] * x
			, matrix[0][0] * y + matrix[1][0], matrix[0][1] * y + matrix[1][1], matrix[0][2] * y + matrix[1][2]
			);
}

AffineTransformation AffineTransformation::transform(const AffineTransformation& transformation) const
{
	return AffineTransformation
			( transformation.matrix[0][0] * matrix[0][0] + transformation.matrix[0][1] * matrix[1][0]
			, transformation.matrix[0][0] * matrix[0][1] + transformation.matrix[0][1] * matrix[1][1]
			, transformation.matrix[0][0] * matrix[0][2] + transformation.matrix[0][1] * matrix[1][2] + transformation.matrix[0][2]
			, transformation.matrix[1][0] * matrix[0][0] + transformation.matrix[1][1] * matrix[1][0]
			, transformation.matrix[1][0] * matrix[0][1] + transformation.matrix[1][1] * matrix[1][1]
			, transformation.matrix[1][0] * matrix[0][2] + transformation.matrix[1][1] * matrix[1][2] + transformation.matrix[1][2]
			);
}

bool AffineTransformation::invert()
{
	double m = (matrix[0][0] * matrix[1][1] - matrix[0][1] * matrix[1][0]);
	if (fabs(m) < EPSILON) {
		return false;
	} else {
		m = 1.0 / m;
		(*this) = AffineTransformation
				( matrix[1][1] * m, matrix[0][1] * -m, (matrix[0][1] * matrix[1][2] - matrix[0][2] * matrix[1][1]) * m
				, matrix[1][0] * -m, matrix[0][0] * m, (matrix[0][0] * matrix[1][2] - matrix[0][2] * matrix[1][0]) * -m
				);
		return true;
	}
}

Vertex AffineTransformation::transform(const Vertex& xy) const
{
	return Vertex(xy.x * matrix[0][0] + xy.y * matrix[0][1] + matrix[0][2]
			, xy.x * matrix[1][0] + xy.y * matrix[1][1] + matrix[1][2]);
}

/* --- Path --- */

Rect<double> Path::calcFloatBounds() const {
	Rect<double> r;
	const_iterator it = instructions.begin();
	const const_iterator e = instructions.end();
	if (it != e) {
		r.left = it->second.x;
		r.top = it->second.y;
		double right = r.left;
		double bottom = r.top;
		++it;
		while (it != e) {
			r.left = minValue(r.left, it->second.x);
			r.top = minValue(r.top, it->second.y);
			right = maxValue(right, it->second.x);
			bottom = maxValue(bottom, it->second.y);
			++it;
		}
		r.width = right - r.left;
		r.height = bottom - r.top;
	}
	return r;
}

IntRect Path::calcIntBounds() const {
	const Rect<double> b = calcFloatBounds();
	IntRect r;
	r.left = static_cast<int>(floor(b.left));
	r.top = static_cast<int>(floor(b.top));
	r.width = static_cast<int>(ceil(b.left + b.width)) - r.left;
	r.height = static_cast<int>(ceil(b.top + b.height)) - r.top;
	return r;
}

Path& Path::append(const Path& p) {
	instructions.insert(instructions.end(), p.instructions.begin(), p.instructions.end());
	if (p.openIndex != static_cast<size_type>(-1)) openIndex = p.openIndex + instructions.size() - p.instructions.size();
	return *this;
}

Path& Path::close() {
	instructions.push_back(Instruction(CLOSE
			, (openIndex == static_cast<size_type>(-1) ? Vertex(0.0, 0.0) : instructions[openIndex].second)));
	return *this;
}

Path& Path::quadraticTo(double controlPointX, double controlPointY, double x, double y, double curveQuality) {
	const Vertex p0 = getPosition();
	double px = p0.x;
	double py = p0.y;
	
	const double c1x = 2.0 * (controlPointX - px);
	const double c1y = 2.0 * (controlPointY - py);
	const double c2x = 2.0 * (px - 2.0 * controlPointX + x);
	const double c2y = 2.0 * (py - 2.0 * controlPointY + y);

	const double d = sqrt(c2x * c2x + c2y * c2y);	// Norm of second derivative is a measure of how direction (=first derivative) twist. We use this info to decide the segment count.
	const int n = minValue(static_cast<int>(sqrt(d * 0.707 * curveQuality) + 0.5) + 1, MAX_SPLINE_SEGMENTS);

	const double m = 1.0 / n;
	const double px2 = c2x * m * m;
	const double py2 = c2y * m * m;
	double px1 = c1x * m + (1.0 / 2.0) * px2;
	double py1 = c1y * m + (1.0 / 2.0) * py2;

	for (int i = 0; i < n; ++i) {
		px += px1;
		py += py1;
		px1 += px2;
		py1 += py2; 
		lineTo(px, py);
	}
	
	return *this;
}

Path& Path::cubicTo(double cpBeginX, double cpBeginY, double cpEndX, double cpEndY, double x, double y, double curveQuality) {
	const Vertex p0 = getPosition();
	double px = p0.x;
	double py = p0.y;

	const double c1x = 3.0 * (cpBeginX - px);
	const double c1y = 3.0 * (cpBeginY - py); 
	const double c2x = 6.0 * (px - 2.0 * cpBeginX + cpEndX);
	const double c2y = 6.0 * (py - 2.0 * cpBeginY + cpEndY);
	const double c3x = 6.0 * (x - px + 3.0 * (cpBeginX - cpEndX));
	const double c3y = 6.0 * (y - py + 3.0 * (cpBeginY - cpEndY));

	/**
		Norm of second derivative is a measure of how direction (=first derivative) twist.
		For a cubic (where second derivative also twists) we use the maximum which happens
		to be either at the very beginning or very end of the curve. We use this info to
		decide the segment count.
	**/
	const double k2x = 6.0 * (cpBeginX - 2.0 * cpEndX + x);
	const double k2y = 6.0 * (cpBeginY - 2.0 * cpEndY + y);
	const double d = sqrt(maxValue(c2x * c2x + c2y * c2y, k2x * k2x + k2y * k2y));
	const int n = minValue(static_cast<int>(sqrt(d * 0.707 * curveQuality) + 0.5) + 1, MAX_SPLINE_SEGMENTS);

	const double m = 1.0 / n;
	const double px3 = c3x * m * m * m;
	const double py3 = c3y * m * m * m;
	double px2 = c2x * m * m + px3;
	double py2 = c2y * m * m + py3;
	double px1 = c1x * m + (1.0 / 2.0) * px2 - (1.0 / 3.0) * px3;
	double py1 = c1y * m + (1.0 / 2.0) * py2 - (1.0 / 3.0) * py3;

	for (int i = 0; i < n; ++i) {
		px += px1;
		py += py1;
		px1 += px2;
		py1 += py2;
		px2 += px3;
		py2 += py3;
		lineTo(px, py);
	}

	return *this;
}

Path& Path::addLine(double startX, double startY, double endX, double endY) {
	moveTo(startX, startY);
	lineTo(endX, endY);
	return *this;
}

Path& Path::addRect(double left, double top, double width, double height) {
	addLine(left, top, left + width, top);
	lineTo(left + width, top + height);
	lineTo(left, top + height);
	close();
	return *this;
}

/**
	Makes an arc by rotating a point around the center of the arc.
**/
Path& Path::arcSweep(double centerX, double centerY, double sweepRadians, double aspectRatio, double curveQuality) {
	assert(-PI2 <= sweepRadians && sweepRadians <= PI2);
	assert(0.0 < aspectRatio && aspectRatio < 10000000000.0);
	assert(0.0 < curveQuality);

	const Vertex pos(getPosition());
	const double sx = (pos.x - centerX) / aspectRatio;
	const double sy = pos.y - centerY;
	const double diameter = maxValue(2.0 * fabs(aspectRatio), 2.0) * sqrt(sx * sx + sy * sy);
	double rx;
	double ry;
	const double t = calcCircleRotationVector(curveQuality, diameter, rx, ry);
	double s = sweepRadians;
	if (s < 0) {
		s = -s;
		ry = -ry;
	}
	double px = sx;
	double py = sy;
	double r = t;
	while (r < s - EPSILON) {
		const double nx = px * rx - py * ry;
		const double ny = px * ry + py * rx;
		px = nx;
		py = ny;
		r += t;
		lineTo(centerX + px * aspectRatio, centerY + py);
	}
	rx = cos(sweepRadians);
	ry = sin(sweepRadians);
	px = sx * rx - sy * ry;
	py = sx * ry + sy * rx;
	lineTo(centerX + px * aspectRatio, centerY + py);

	return *this;
}

Path& Path::addEllipse(double centerX, double centerY, double radiusX, double radiusY, double curveQuality) {
	assert(0.0 < curveQuality);
	if (fabs(radiusX) < EPSILON) addLine(centerX, centerY - radiusY, centerX, centerY + radiusY);
	else if (fabs(radiusY) < EPSILON) addLine(centerX - radiusX, centerY, centerX + radiusX, centerY);
	else {
		double sweepSign = ((radiusX < 0.0) != (radiusY < 0.0) ? -1.0 : 1.0);
		moveTo(centerX + radiusX, centerY);
		arcSweep(centerX, centerY, sweepSign * PI2, sweepSign * radiusX / radiusY, curveQuality);
	}
	close();
	return *this;
}

Path& Path::addCircle(double centerX, double centerY, double radius, double curveQuality) {
	assert(0.0 < curveQuality);
	moveTo(centerX + radius, centerY);
	arcSweep(centerX, centerY, PI2, 1.0, curveQuality);
	close();
	return *this;
}

Path& Path::addRoundedRect(double left, double top, double width, double height, double cornerWidth
		, double cornerHeight, double curveQuality) {
	if (cornerWidth < EPSILON || cornerHeight < EPSILON) {
		addRect(left, top, width, height);
	} else {
		double ratio = cornerWidth / cornerHeight;
		double right = left + width;
		double bottom = top + height;
		addLine(left + cornerWidth, top, right - cornerWidth, top);
		arcSweep(right - cornerWidth, top + cornerHeight, PI * 0.5, ratio, curveQuality);
		lineTo(right, top + cornerHeight);
		lineTo(right, bottom - cornerHeight);
		arcSweep(right - cornerWidth, bottom - cornerHeight, PI * 0.5, ratio, curveQuality);
		lineTo(right - cornerWidth, bottom);
		lineTo(left + cornerWidth, bottom);
		arcSweep(left + cornerWidth, bottom - cornerHeight, PI * 0.5, ratio, curveQuality);
		lineTo(left, bottom - cornerHeight);
		lineTo(left, top + cornerHeight);
		arcSweep(left + cornerWidth, top + cornerHeight, PI * 0.5, ratio, curveQuality);
		close();
	}
	return *this;
}

Path& Path::addStar(double centerX, double centerY, int points, double radius1, double radius2, double rotation) {
	assert(0 < points);
	double px = sin(rotation);
	double py = -cos(rotation);
	double t = PI2 / points;
	double rx = cos(t);
	double ry = sin(t);
	double s = radius1;
	moveTo(centerX + px * s, centerY + py * s);
	double r = t;
	while (r < PI2 - EPSILON) {
		s = (radius1 + radius2) - s;
		double nx = px * rx - py * ry;
		double ny = px * ry + py * rx;
		px = nx;
		py = ny;
		r += t;
		lineTo(centerX + px * s, centerY + py * s);
	}
	close();
	
	return *this;
}

struct StrokeSegment {
	StrokeSegment(const Vertex& v = Vertex(), const Vertex& d = Vertex(), double l = 0.0)
			: v(v), d(d), l(l) { }
	Vertex v; /// Start vertex.
	Vertex d; /// Delta vector per "width unit" (i.e. delta vector / length * width).
	double l; /// Length in "width units" (i.e. length / width).
};

/**
	Makes an arc by rotating a point around the center of the arc.
	The end-point is rotated to the horizontal plane so that we can easily check when we reach it.
	(This trick works since the arc is always less than 180 degrees.)
**/
static void strokeRounded(Path& stroked, double ax1, double ay1, double bx0, double by0, double bdx, double bdy
		, double rx, double ry) {
	double px = ax1 - bx0 + bdy;
	double py = ay1 - by0 - bdx;
	double ex = bdy * bdx - bdx * bdy;
	double tx;
	do {
		stroked.lineTo(bx0 - bdy + px, by0 + bdx + py);
		double nx = px * rx - py * ry;
		double ny = px * ry + py * rx;
		px = nx;
		py = ny;
		tx = px * bdx + py * bdy;
	} while (tx < ex);
	stroked.lineTo(bx0, by0);
}

static void strokeEnd(Path& stroked, double direction, const StrokeSegment* seg, Path::EndCapStyle endCaps
		, double rx, double ry) {
	int o = (direction >= 0) ? 0 : 1;
	double adx = seg[0].d.x * direction;
	double ady = seg[0].d.y * direction;
	double ax1 = seg[1 - o].v.x + ady;
	double ay1 = seg[1 - o].v.y - adx;
	
	double bx0 = ax1 - ady * 2;
	double by0 = ay1 + adx * 2;
	if (endCaps == Path::ROUND) {
		strokeRounded(stroked, ax1, ay1, bx0, by0, -adx, -ady, rx, ry);
	} else { // Squared end-caps were already extended before stroking begun, so same code as butt here.
		stroked.lineTo(ax1, ay1);
		stroked.lineTo(bx0, by0);
	}
}

/**
	Offsets two consecutive segments and emits the outline for one side of the stroke.
	`direction` is +1 for the left side and -1 for the right side when following the path.
	The segments are offset by their perpendiculars; inner joins collapse while outer
	joins are expanded according to `joints`, with miters clipped by `miterLimitW`.
**/
static void strokeOneSide(Path& stroked, double direction, const StrokeSegment* segA, const StrokeSegment* segB
		, Path::JointStyle joints, double miterLimitW, double rx, double ry) {
	int o = (direction >= 0) ? 0 : 1;			// select start/end index depending on traversal direction
	
	double al = segA[0].l;						// length of A measured in stroke widths
	double adx = segA[0].d.x * direction;		// normalized delta for segment A
	double ady = segA[0].d.y * direction;
	double ax0 = segA[o].v.x + ady;				// offset point A at start of join
	double ay0 = segA[o].v.y - adx;
	double ax1 = segA[1 - o].v.x + ady;			// offset point A at end of join
	double ay1 = segA[1 - o].v.y - adx;
	double bl = segB[0].l;						// length of B in stroke widths
	double bdx = segB[0].d.x * direction;		// normalized delta for segment B
	double bdy = segB[0].d.y * direction;
	double bx0 = segB[o].v.x + bdy;				// offset point B at start of join
	double by0 = segB[o].v.y - bdx;
	const double cross = bdx * ady - adx * bdy;	// determinant of direction vectors
	const double dot = adx * bdx + ady * bdy;	// dot product of direction vectors
	const bool zeroCross = (fabs(cross) < EPSILON);

	/*
		Inner joint if B is inside half-plane of A (or if A and B are virtually
		collinear, which is checked by adding EPSILON * 2), unless they are
		virtually collinear but opposite directions.
	*/
	const bool insideHalfPlane = (bx0 - ax1) * bdx < (ay1 - by0) * bdy + EPSILON * 2;
	const bool oppositeDirs = zeroCross && dot <= -1.0 + EPSILON;
	if (insideHalfPlane && !oppositeDirs) {
		// --- Inner joint ---
		
		double v = 0.0;										// param along segment A
		double w = 0.0;										// param along segment B
		if (!zeroCross) {
			v = (bdy * (ax0 - bx0) - bdx * (ay0 - by0)) / cross;
			w = (ady * (ax0 - bx0) - adx * (ay0 - by0)) / cross;
		}
		if (!zeroCross && v >= 0.0 && v <= al && w >= 0.0 && w <= bl) {	// Do the offset lines cross before segment ends?
			stroked.lineTo(ax0 + adx * v, ay0 + ady * v);
		} else {											// If lines do not cross, resort to a safe romb that fills correctly
			stroked.lineTo(ax1, ay1);
			// stroked.lineTo(bx0 - bdy, by0 + bdx);		// would produce a slimmer join
			stroked.lineTo(bx0, by0);
		}	
	} else {
		// --- Outer joint ---
		switch (joints) {
			case Path::MITER: {
				if (!zeroCross) {
					const double w = (ady * (ax0 - bx0) - adx * (ay0 - by0)) / cross;	// param along B
					if (w > miterLimitW) {	// Intersection within miter limit?
						stroked.lineTo(bx0 + bdx * w, by0 + bdy * w);
						break;
					} 
				}
				stroked.lineTo(ax1 - adx * miterLimitW, ay1 - ady * miterLimitW);	// Clip to miter limit
				stroked.lineTo(bx0 + bdx * miterLimitW, by0 + bdy * miterLimitW);
				break;
			}
			
			case Path::BEVEL: {
				stroked.lineTo(ax1, ay1);
				stroked.lineTo(bx0, by0);
				break;
			}
			
			case Path::CURVE: {
				strokeRounded(stroked, ax1, ay1, bx0, by0, bdx, bdy, rx, ry);
				break;
			}
			
			default: assert(0);
		}
	}
}

Path& Path::stroke(double width, EndCapStyle endCaps, JointStyle joints, double miterLimit, double curveQuality) {
	assert(0.0 <= width);
	assert(BUTT <= endCaps && endCaps <= SQUARE);
	assert(BEVEL <= joints && joints <= MITER);
	assert(1.0 <= miterLimit);
	assert(0.0 < curveQuality);
	
	Path stroked;
	stroked.instructions.reserve(instructions.size() * 3);
	width = maxValue(width, EPSILON);

	double rcpWidth = 2.0 / width;
	double miterLimitW = (joints == MITER ? -sqrt(miterLimit * miterLimit - 1.0) : 0.0);
	double rx = 0.0;
	double ry = 0.0;
	if (joints == CURVE || endCaps == ROUND) {
		calcCircleRotationVector(curveQuality, width, rx, ry);
	}

	Vertex lv(0.0, 0.0);
	std::vector<StrokeSegment> segs;
	segs.reserve(instructions.size() + 2);
	
	for (const_iterator it = instructions.begin(), e = instructions.end(); it != e;) {
		segs.clear();
		for (; it != e && it->first != LINE; ++it) {
			lv = it->second;
		}
		bool isClosed = false;
		for (; it != e && it->first != MOVE && !isClosed; ++it) {
			isClosed = (it->first == CLOSE);
			Vertex nv = it->second;
			double dx = nv.x - lv.x;
			double dy = nv.y - lv.y;
			double l = dx * dx + dy * dy;
			if (l >= EPSILON) {
				l = sqrt(l) * rcpWidth;
				segs.push_back(StrokeSegment(lv, Vertex(dx / l, dy / l), l));
				lv = nv;
			}
		}
		
		// Special case for empty path to draw a square or circle end-cap.
		if (segs.empty()) {
			segs.push_back(StrokeSegment(lv, Vertex(width * 0.5, 0.0), 1.0));
		}
		
		// Stroke sub-path by tracing each of it's sides.
		size_type count = segs.size();
		segs.push_back(StrokeSegment(lv));						// Append terminal segment.
		
		size_type firstVertexIndex = stroked.size();
		stroked.instructions.push_back(Instruction());			// We fill in the first vertex later.
		
		if (isClosed) {
			for (size_type i = 0; i < count - 1; ++i) {
				strokeOneSide(stroked, 1.0, &segs[i], &segs[i + 1], joints, miterLimitW, rx, ry);
			}
			strokeOneSide(stroked, 1.0, &segs[count - 1], &segs[0], joints, miterLimitW, rx, ry);
			stroked.instructions.back().first = CLOSE;			// Close first output path, begin a new one.
			stroked.instructions[firstVertexIndex] = Instruction(MOVE, stroked.getPosition());
			firstVertexIndex = stroked.size();
			stroked.instructions.push_back(Instruction());
			for (size_type i = count - 1; i > 0; --i) {
				strokeOneSide(stroked, -1.0, &segs[i], &segs[i - 1], joints, miterLimitW, rx, ry);
			}
			strokeOneSide(stroked, -1.0, &segs[0], &segs[count - 1], joints, miterLimitW, rx, ry);
		} else {
			if (endCaps == SQUARE) {							// Extend beginning and ending if square end-caps. (Doing this "up-front" may improve the "look" of the first and last inner joint.)
				segs[0].v.x -= segs[0].d.x;
				segs[0].v.y -= segs[0].d.y;
				++segs[0].l;
				segs[count].v.x += segs[count - 1].d.x;
				segs[count].v.y += segs[count - 1].d.y;
				++segs[count - 1].l;
			}
			for (size_type i = 0; i < count - 1; ++i) {
				strokeOneSide(stroked, 1.0, &segs[i], &segs[i + 1], joints, miterLimitW, rx, ry);
			}
			strokeEnd(stroked, 1.0, &segs[count - 1], endCaps, rx, ry);
			for (size_type i = count - 1; i >= 1; --i) {
				strokeOneSide(stroked, -1.0, &segs[i], &segs[i - 1], joints, miterLimitW, rx, ry);
			}
			strokeEnd(stroked, -1.0, &segs[0], endCaps, rx, ry);
		}
		
		stroked.instructions.back().first = CLOSE;				// Close first output path, begin a new one.
		stroked.instructions[firstVertexIndex] = Instruction(MOVE, stroked.getPosition());
	}

	instructions.swap(stroked.instructions);
	openIndex = stroked.openIndex;
	return *this;
}

Path& Path::dash(double dashLength, double gapLength, double dashOffset) {
	assert(0.0 <= dashLength);
	assert(0.0 <= gapLength);
	assert(0.0 <= dashOffset && dashOffset <= (dashLength + gapLength));

	if (gapLength >= EPSILON) {
		InstructionsVector dashed;
		double initR = fmod(dashLength - dashOffset, (dashLength + gapLength));		
		Vertex lv(0.0, 0.0);
		for (const_iterator it = instructions.begin(), e = instructions.end(); it != e;) {
			for (; it != e && it->first != LINE; ++it) lv = it->second;
			if (it != e) {
				size_type firstDashIndex = dashed.size();
				size_type lastDashIndex = firstDashIndex;
				dashed.push_back(Instruction(MOVE, lv));
				bool firstPenDown = true;
				double r = initR;
				if (r < 0.0) {
					firstPenDown = false;
					r += gapLength;
				}
				bool penDown = firstPenDown;
				bool isClosed = false;
				for (; it != e && it->first != MOVE && !isClosed; ++it) {
					isClosed = (it->first == CLOSE);
					double dx = it->second.x - lv.x;
					double dy = it->second.y - lv.y;
					double l = dx * dx + dy * dy;
					if (l >= EPSILON) {
						l = sqrt(l);
						dx /= l;
						dy /= l;
						do {
							double n = minValue(l, r);
							lv.x += n * dx;
							lv.y += n * dy;
							l -= n;
							r -= n;
							if (penDown) {
								dashed.push_back(Instruction(LINE, lv));
								if (r <= 0.0) {
									penDown = false;
									r += gapLength;
								}
							} else if (r <= 0.0) {
								penDown = true;
								lastDashIndex = dashed.size();
								dashed.push_back(Instruction(MOVE, lv));
								r += dashLength;
							}
						} while (l > 0.0);
					}
				}
				if (firstDashIndex != lastDashIndex && isClosed && penDown && firstPenDown) {	// If original sub-path was closed and we currently have "pen down", we should rotate the vertex data so that we begin the new sub-path at "pen-down-point".
					(dashed.begin() + firstDashIndex)->first = LINE;
					std::rotate(dashed.begin() + firstDashIndex, dashed.begin() + lastDashIndex, dashed.end());
				}
			}
		}
		
		instructions.swap(dashed);
		openIndex = instructions.size() - 1;
	}
	
	return *this;
}

Path& Path::closeAll() {
	InstructionsVector closed;
	
	Vertex openCoordinates(0.0, 0.0);
	for (const_iterator it = instructions.begin(), e = instructions.end(); it != e;) {
		const_iterator b = it;
		do {
			if (it->first != LINE) openCoordinates = it->second;
			++it;
		} while (it != e && !((it - 1)->first == LINE && it->first == MOVE));
		closed.insert(closed.end(), b, it);
		if ((it - 1)->first != CLOSE) closed.push_back(Instruction(CLOSE, openCoordinates));
	} 

	instructions.swap(closed);
	openIndex = instructions.size() - 1;

	return *this;
}

Path& Path::transform(const AffineTransformation& transformation) {
	if (transformation != AffineTransformation()) {
		for (InstructionsVector::iterator it = instructions.begin(), e = instructions.end(); it != e; ++it)
			it->second = transformation.transform(it->second);
	}
	return *this;
}

/* --- GammaTable --- */

GammaTable::GammaTable(double gamma)
{
	assert(0.0 < gamma);
	for (int i = 0; i < 256; ++i) {
		table[i] = Mask8::Pixel(floor(pow(i / 255.0, 1.0 / gamma) * 255) + 0.5);
	}
}

/* --- LinearAscend --- */

LinearAscend::LinearAscend(double startX, double startY, double endX, double endY)
{
	double dx0 = endX - startX;
	double dy0 = endY - startY;
	double l = sqrt(dx0 * dx0 + dy0 * dy0);
	if (l != 0) {
		l = 1.0 / l;
	}
	l *= l * (1 << 16);
	dx = roundToInt(dx0 * l);
	dy = roundToInt(dy0 * l);
	start = roundToInt(-startX * dx - startY * dy);
}

IntRect LinearAscend::calcBounds() const {
	return FULL_RECT; // FIX : optimize, we should calculate the real rect here, but how actually?
}

void LinearAscend::render(int x, int y, int length, SpanBuffer<Mask8>& output) const
{
	assert(0 < length && length <= MAX_RENDER_LENGTH);

	// FIX : increase x resolution from 16 bits?

	int ki = start + x * dx + y * dy;
	int dk = dx;

	int i = 0;
	while (i < length) {
		if (ki <= 0 || ki >= (1 << 16) || dk == 0) {
			// Quickly fill left and right surrounding of gradient transition.
			
			int edge = length;
			if (ki <= 0 && dk > 0) {
				edge = minValue(i + 1 - ki / dk, length);
			} else if (ki >= (1 << 16) && dk < 0) {
				edge = minValue(i + 1 + (ki - (1 << 16)) / -dk, length);
			}
			assert(i < edge);
			output.addSolid(edge - i, minValue(maxValue(ki >> 8, 0), 256 - 1));
			ki += dk * (edge - i);
			i = edge;
		} else {
			int leftEdge = i;
			int rightEdge = length;
			if (dk > 0) {
				rightEdge = minValue(i + ((1 << 16) - ki + (dk - 1)) / dk, length);
			} else if (dk < 0) {
				rightEdge = minValue(i + (ki + (-dk - 1)) / -dk, length);
			}
			Mask8::Pixel* pixels = output.addVariable(rightEdge - leftEdge, false);
			assert(i < rightEdge);
			while (i < rightEdge) {
				assert(0 <= (ki >> 8) && (ki >> 8) <= 255);
				// FIX : i should be 0 based
				pixels[i - leftEdge] = (ki >> 8);
				ki += dk;
				++i;
			}
			assert(i >= length || ki <= 0 || ki >= (1 << 16));
		}
	}
}

/* --- RadialAscend --- */

Mask8::Pixel RadialAscend::sqrtTable[1 << RADIAL_SQRT_BITS];

RadialAscend::RadialAscend(double centerX, double centerY, double width, double height)
	: centerX(centerX)
	, centerY(centerY)
	, width(fabs(width))
	, height(fabs(height))
	, hk((1U << 30) / (height * height))
	, wk((1U << 30) / (width * width))
{
	assert(width != 0.0 && height != 0.0);
	if (sqrtTable[0] == 0) { // sqrtTable[0] should be 255 when initialized.
		{ for (int i = 0; i < (1 << RADIAL_SQRT_BITS); ++i) {
			// Notice: output is 255 at the center so that we'll have full transparency surroundings.
			// The entire table is therefore inversed.
			sqrtTable[i] = 255 - roundToInt(sqrt(double(i) / ((1 << RADIAL_SQRT_BITS) - 1)) * 255);
		} }
	}
}

IntRect RadialAscend::calcBounds() const
{
	const int left = static_cast<int>(floor(centerX - width));
	const int top = static_cast<int>(floor(centerY - height));
	return IntRect(left, top, static_cast<int>(ceil(centerX + width)) - left
			, static_cast<int>(ceil(centerY + height)) - top);
}

void RadialAscend::render(int x, int y, int length, SpanBuffer<Mask8>& output) const
{
	assert(0 < length && length <= MAX_RENDER_LENGTH);

	// Calculate left and right edge of inner circle for this row.

	const double dy = y + 0.5 - centerY;
	const double a = 1.0 - dy * dy / (height * height);
	const double rowWidth = (a > EPSILON) ? width * sqrt(a) : 0;
	const double rowStart = (centerX - rowWidth);
	const int rowStartInt = roundToInt(rowStart);
	const int leftEdge = minValue(maxValue(rowStartInt - x, 0), length);
	const int rightEdge = minValue(roundToInt(rowStart + rowWidth * 2 - x), length);
	
	int i = 0;
	while (i < length) {
		if (i < leftEdge || i >= rightEdge) {
			assert(i == 0 || i == rightEdge);
			const int edge = (i < leftEdge) ? leftEdge : length;
			output.addTransparent(edge - i);
			i = edge;
		} else {
			assert(i == leftEdge);
						
			const int steps = x + i - rowStartInt;
			assert(steps >= 0);
			const double dx = rowStartInt - centerX;
			const double dpp = 2.0 * wk;
			const double dp = (2.0 * dx - 1.0) * wk + dpp * 0.5;
			const double d = dy * dy * hk + dx * dx * wk + dp * 0.5;
			assert(dpp >= 0.0);
			const unsigned int dppi = roundToInt(dpp);
			assert(steps < (1 << 16));
			const int dp0 = roundToInt(dp);
			
			// Calculate steps * (steps + 1) / 2 in a way that avoids overflow.
			const int tri = ((steps & 1) != 0) ? steps * ((steps + 1) >> 1) : (steps >> 1) * (steps + 1);
			int dpi = dp0 + steps * dppi;
			int di = roundToInt(d) + steps * dp0 + dppi * tri;			
			
			Mask8::Pixel* pixels = output.addVariable(rightEdge - leftEdge, false);
			
			// Lead up to next absolute x divisible by 4, to enforce identical output regardless of span length limits
			while (((i + x) & 3) != 0 && i < rightEdge) {
				const int z = minValue(maxValue(di, 0), (1 << 30) - 1);								// Clamp di to valid range.
				const int precision = (z < (1 << (30 - 8))) << 2;									// Shift input and output (by 8 and 4 respectively) if z is small to attain 256 times higher resolution for the relatively small sqrt table lookup.
				const int sqrtShift = ((30 - RADIAL_SQRT_BITS) - precision - precision);			// Input is "up-shifted" twice as much (8) as the output is down-shifted (4), since the output multiplier should be the square-root of the input multiplier.
				*pixels++ = ((255 << precision) - 255 + sqrtTable[z >> sqrtShift]) >> precision;	// Since the table is inversed (see constructor), we use an algebraic trick to perform: 255 - (255 - table) >> 4.
				dpi += dppi;																		// Perform run-time integration of the derivate of di * di to avoid the integer multiplication.
				di += dpi;
				++i;
			}
			while (i + 4 <= rightEdge) {
				int z0 = di;
				dpi += dppi;
				di += dpi;
				int z1 = di;
				dpi += dppi;
				di += dpi;
				int z2 = di;
				dpi += dppi;
				di += dpi;
				int z3 = di;
				dpi += dppi;
				di += dpi;

				int allZ = z0 | z1 | z2 | z3;														// allZ is used to determine the resolution for the table lookup later on.
				if ((allZ & ~((1 << 30) - 1)) != 0) {												// Check if any z was outside 0 <= z < (1 << 30) range, if so, clamp them all.
					z0 = minValue(maxValue(z0, 0), (1 << 30) - 1);
					z1 = minValue(maxValue(z1, 0), (1 << 30) - 1);
					z2 = minValue(maxValue(z2, 0), (1 << 30) - 1);
					z3 = minValue(maxValue(z3, 0), (1 << 30) - 1);
					allZ = z0 | z1 | z2 | z3;
				}

				if (allZ < (1 << (30 - 8))) {														// Shift input and output if maximum z is small to attain 256 times higher resolution for the relatively small sqrt table lookup.
					const int sqrtShift = ((30 - RADIAL_SQRT_BITS) - 8);							// Input is "up-shifted" twice as much (8) as the output is down-shifted (4), since the output multiplier should be the square-root of the input multiplier.
					pixels[0] = ((255 << 4) - 255 + sqrtTable[z0 >> sqrtShift]) >> 4;				// Since the table is inversed (see constructor), we use an algebraic trick to perform: 255 - (255 - table) >> 4.
					pixels[1] = ((255 << 4) - 255 + sqrtTable[z1 >> sqrtShift]) >> 4;
					pixels[2] = ((255 << 4) - 255 + sqrtTable[z2 >> sqrtShift]) >> 4;
					pixels[3] = ((255 << 4) - 255 + sqrtTable[z3 >> sqrtShift]) >> 4;
				} else {
					const int sqrtShift = (30 - RADIAL_SQRT_BITS);
					pixels[0] = sqrtTable[z0 >> sqrtShift];
					pixels[1] = sqrtTable[z1 >> sqrtShift];
					pixels[2] = sqrtTable[z2 >> sqrtShift];
					pixels[3] = sqrtTable[z3 >> sqrtShift];
				}

				pixels += 4;
				i += 4;
			}

			while (i < rightEdge) {
				const int z = minValue(maxValue(di, 0), (1 << 30) - 1);								// Clamp di to valid range.
				const int precision = (z < (1 << (30 - 8))) << 2;									// Shift input and output (by 8 and 4 respectively) if z is small to attain 256 times higher resolution for the relatively small sqrt table lookup.
				const int sqrtShift = ((30 - RADIAL_SQRT_BITS) - precision - precision);			// Input is "up-shifted" twice as much (8) as the output is down-shifted (4), since the output multiplier should be the square-root of the input multiplier.
				*pixels++ = ((255 << precision) - 255 + sqrtTable[z >> sqrtShift]) >> precision;	// Since the table is inversed (see constructor), we use an algebraic trick to perform: 255 - (255 - table) >> 4.
				dpi += dppi;																		// Perform run-time integration of the derivate of di * di to avoid the integer multiplication.
				di += dpi;
				++i;
			}
		}
	}
}

/* --- FillRule --- */

FillRule::~FillRule() { }

/* --- NonZeroFillRule --- */

void NonZeroFillRule::processCoverage(int count, const Int32* source, Mask8::Pixel* destination) const
{
	for (int i = 0; i < count; ++i) {
		destination[i] = minValue(abs(source[i]) >> ((COVERAGE_BITS + POLYGON_FRACTION_BITS) - 8), 0xFF);
	}
}

/* --- EvenOddFillRule --- */

void EvenOddFillRule::processCoverage(int count, const Int32* source, Mask8::Pixel* destination) const
{
	for (int i = 0; i < count; ++i) {
		int c = source[i];
		const int k = (1 << (COVERAGE_BITS + POLYGON_FRACTION_BITS));
		c = ((c & k) != 0) ? ((~c & (k - 1)) + 1) : (c & (k - 1));
		destination[i] = minValue(c >> ((COVERAGE_BITS + POLYGON_FRACTION_BITS) - 8), 0xFF);
	}
}

/* --- PolygonMask --- */

// Notice: this compares through pointers, so we can't implement this as operator< for Segment.
struct PolygonMask::Segment::Order {
	bool operator()(const PolygonMask::Segment* a, const PolygonMask::Segment* b) {
		// Sort by starting row then left edge.
		return ((a->topY >> FRACT_BITS) < (b->topY >> FRACT_BITS)
				|| ((a->topY >> FRACT_BITS) == (b->topY >> FRACT_BITS) && a->leftEdge < b->leftEdge));
	}
};

bool PolygonMask::isValid() const { return valid; }

PolygonMask::PolygonMask(const Path& path, const IntRect& clipBounds, const FillRule& fillRule)
	: segments(), fillRule(fillRule), row(0), engagedStart(0), engagedEnd(0), coverageDelta(), valid(true)
{
	// Clamp the clip rectangle to the numeric limits handled by the rasterizer.
	IntRect cb = clipBounds;
	assert(0 <= cb.width && 0 <= cb.height);
	const int limit = (0x7FFFFFFF >> FRACT_BITS);
	cb.left = maxValue(-limit, minValue(cb.left, limit));
	cb.top = maxValue(-limit, minValue(cb.top, limit));
	int rightBound = maxValue(-limit, minValue(cb.calcRight(), limit));
	int bottomBound = maxValue(-limit, minValue(cb.calcBottom(), limit));
	cb.width = maxValue(0, rightBound - cb.left);
	cb.height = maxValue(0, bottomBound - cb.top);

	// Reserve space for all edges plus a sentinel segment.
	segments.reserve(path.size() + 1);
	const double vertexLimit = static_cast<double>(0x7FFFFFFF >> POLYGON_FRACTION_BITS);
	int minY = 0x3FFFFFFF;
	int minX = 0x3FFFFFFF;
	int maxY = -0x3FFFFFFF;
	int maxX = -0x3FFFFFFF;
	int top = cb.top << FRACT_BITS;
	int right = rightBound << FRACT_BITS;
	int bottom = bottomBound << FRACT_BITS;
	int lx = 0;
	int ly = 0;

	// Parse the path, converting each edge to a Segment.
	for (Path::const_iterator it = path.begin(), e = path.end(); it != e;) {
		while (it != path.end() && it->first == Path::MOVE) {
			// Begin a new contour.
			const double x = it->second.x;
			const double y = it->second.y;
			if (!isfinite(x) || !isfinite(y) || fabs(x) > vertexLimit || fabs(y) > vertexLimit) {
				valid = false;
				segments.clear();
				bounds = IntRect();
				return;
			}
			lx = roundToInt(x * FRACT_ONE);
			ly = roundToInt(y * FRACT_ONE);
			++it;
		}
		while (it != path.end() && it->first != Path::MOVE) {
			int x0 = lx;
			int y0 = ly;
			const double x = it->second.x;
			const double y = it->second.y;
			if (!isfinite(x) || !isfinite(y) || fabs(x) > vertexLimit || fabs(y) > vertexLimit) {
				valid = false;
				segments.clear();
				bounds = IntRect();
				return;
			}
			int x1 = roundToInt(x * FRACT_ONE);
			int y1 = roundToInt(y * FRACT_ONE);
			lx = x1;
			ly = y1;
			bool reversed = false;
			if (y0 > y1) {
				// Ensure segment runs from top to bottom.
				std::swap(y0, y1);
				std::swap(x0, x1);
				reversed = true;
			}
			
			// Skip horizontal edges and those completely outside the clip rectangle.
			if (y0 != y1 && y1 > top && y0 < bottom && minValue(x0, x1) < right) {
				segments.push_back(Segment());
				Segment& seg = segments.back();
				seg.topY = y0;
				seg.bottomY = y1;
				seg.x = toFixed32_32(x0, 0);
				seg.leftEdge = (x0 >> FRACT_BITS);
				seg.dx = toFixed32_32(0, 0);
				int coverageByX = 1 << (COVERAGE_BITS + FRACT_BITS);
				const int dx = x1 - x0;
				if (dx != 0) {
					const int dy = y1 - y0;
					seg.dx = divide(dx, dy);
					assert(dy >= 0);
					const Fixed32_32 dyByDx = divide(dy, abs(dx));
					// if dy/|dx| < 1, use floor(2^T * dy/|dx|); else keep the saturated default
					if (high32(dyByDx) == 0) {
						coverageByX = high32(shiftLeft(dyByDx, COVERAGE_BITS + FRACT_BITS));
					}
				}
				seg.coverageByX = (reversed ? -coverageByX : coverageByX);
				if (top > seg.topY) { // Oops, we've passed the first y segment, catch-up!
					seg.x = add(seg.x, multiply(static_cast<UInt32>(top - seg.topY), seg.dx));
					seg.topY = top;
					seg.leftEdge = (high32(seg.x) >> FRACT_BITS);
				}
				seg.currentY = seg.topY;
				seg.rightEdge = seg.leftEdge;
			}
			
			// Track overall bounds of the path in fixed-point space.
			minY = minValue(minY, y0);
			maxY = maxValue(maxY, y1);
			sort(x0, x1);
			minX = minValue(minX, x0);
			maxX = maxValue(maxX, x1);
			++it;
		}
	}
	
	// Append a sentinel segment to simplify iteration logic.
	segments.push_back(Segment());
	Segment& segSentinel = segments.back();
	segSentinel.topY = 0x7FFFFFFF; // "Sentinel" value, so we don't have to check the count.
	segSentinel.currentY = segSentinel.topY;

	// Finalize bounds in pixel space and allocate coverage buffer.
	bounds.left = minX >> FRACT_BITS;
	bounds.top = minY >> FRACT_BITS;
	bounds.width = ((maxX + FRACT_MASK) >> FRACT_BITS) - bounds.left;
	bounds.height = ((maxY + FRACT_MASK) >> FRACT_BITS) - bounds.top;
	bounds = bounds.calcIntersection(cb);
	coverageDelta.assign(minValue(bounds.width + 1, MAX_RENDER_LENGTH + 1), 0);

	// Prepare for the first rendering pass.
	rewind();
}

void PolygonMask::rewind() const {
	assert(valid);
	if (!valid) return;
	
	// Reset state so rendering can start from the top row again.
	row = bounds.top;
	engagedStart = 0;
	engagedEnd = 0;
	std::fill(coverageDelta.begin(), coverageDelta.end(), 0);
	for (size_t i = 0, n = segments.size(); i < n; ++i) {
		Segment* seg = const_cast<Segment*>(&segments[i]);
		if (seg->currentY != seg->topY) {
			int dy = seg->currentY - seg->topY;
			seg->x = add(seg->x, multiply(-dy, seg->dx));
			seg->currentY = seg->topY;
		}
		seg->leftEdge = seg->rightEdge = high32(seg->x) >> FRACT_BITS;
	}
	
	// Build a list of pointers sorted vertically by topY.
	segsVertically.resize(segments.size());
	for (size_t segIndex = 0; segIndex < segments.size(); ++segIndex) {
		segsVertically[segIndex] = const_cast<Segment*>(&segments[segIndex]);
	}
	std::sort(segsVertically.begin(), segsVertically.end(), Segment::Order());
	// Horizontal list starts identical; it will be maintained in x-order during rendering.
	segsHorizontally = segsVertically;
}

IntRect PolygonMask::calcBounds() const {
	assert(valid);
	return (valid ? bounds : IntRect());
}

void PolygonMask::render(int x, int y, int length, SpanBuffer<Mask8>& output) const {
	assert(valid);
	if (!valid) {
		output.addTransparent(length);
		return;
	}
	assert(0 < length && length <= MAX_RENDER_LENGTH);
	const int clipLeft = bounds.left;
	const int clipRight = bounds.calcRight();
	if (x + length <= clipLeft || x >= clipRight) {
		// Entire request lies outside horizontal clip bounds.
		output.addTransparent(length);
		return;
	}
	int rightClip = 0;
	if (x < clipLeft) {
		// Clip span on the left.
		const int leftClip = clipLeft - x;
		output.addTransparent(leftClip);
		x = clipLeft;
		length -= leftClip;
	}
	if (x + length > clipRight) {
		// Clip span on the right.
		rightClip = x + length - clipRight;
		length -= rightClip;
	}
	const int clipTop = bounds.top;
	const int clipBottom = clipTop + bounds.height;
	if (y < clipTop || y >= clipBottom) {
		// Outside vertical bounds: emit transparent pixels.
		output.addTransparent(length);
		if (rightClip > 0) {
			output.addTransparent(rightClip);
		}
		return;
	}

	if (y < row) {
		// Requested row is above last rendered one: restart rasterizer.
		rewind();
	}

	if (y > row) {
		/*
			Advance the active edge list to the requested row.
			Already-engaged edges simply step forward `rowCount` rows.
			Newly-engaged edges adjust from their topY.
			This may leave the horizontal list unsorted, requiring
			extra work later when reordering.
		*/
		const int yFixed = y << FRACT_BITS;
		int segIndex = engagedStart;
		while (segsVertically[segIndex]->topY < yFixed) {
			Segment* seg = segsVertically[segIndex];
			int dy = yFixed - seg->currentY;
			if (dy > 0) {
				seg->x = add(seg->x, multiply(static_cast<UInt32>(dy), seg->dx));
				seg->currentY = yFixed;
			}
			++segIndex;
		}
		row = y;
	}
	
	const int rowFixed = row << FRACT_BITS;
	
	int includeIndex = engagedEnd;
	while (segsVertically[includeIndex]->topY < rowFixed + FRACT_ONE) {
		++includeIndex;
	}

	// Merge-sort newly activated segments into the x-ordered list.
	
	int insertIndex = includeIndex - 1;
	int hIndex = engagedEnd - 1;
	int vIndex = insertIndex;
	while (insertIndex >= engagedStart && (vIndex >= engagedEnd || hIndex != insertIndex)) {
		if (vIndex < engagedEnd || (hIndex >= engagedStart
				&& segsHorizontally[hIndex]->leftEdge > segsVertically[vIndex]->leftEdge - x)) {
			segsHorizontally[insertIndex] = segsHorizontally[hIndex];
			--hIndex;
		} else {
			segsHorizontally[insertIndex] = segsVertically[vIndex];
			--vIndex;
		}
		--insertIndex;
	}
	
	// Rasterize active segments into coverage deltas.
	
	engagedEnd = includeIndex;
	int integrateIndex = engagedStart;
	for (int drawIndex = engagedStart; drawIndex < engagedEnd; ++drawIndex) {
		Segment* seg = segsVertically[drawIndex];
		
		if (rowFixed >= seg->bottomY) {
			// Mark retired line for horizontal removal.
			seg->leftEdge = -0x7FFFFFFF;
			// Swap out directly from vertical list.
			std::swap(segsVertically[integrateIndex], segsVertically[drawIndex]);
			++integrateIndex;
		} else {
			const int coverageByX = seg->coverageByX;
			int remaining;	// Signed total area this segment contributes in THIS row (fixed-point; sign follows winding)
			Fixed32_32 dx;
			if (rowFixed < seg->topY || rowFixed + FRACT_ONE > seg->bottomY) {
				// Partial row (entering/exiting): compute subpixel dy, scale 'remaining' by dy, advance x by dy*dx.
				const unsigned short dy = minValue(seg->bottomY - rowFixed, FRACT_ONE) - maxValue(seg->topY - rowFixed, 0);
				remaining = ((coverageByX < 0) ? -(1 << COVERAGE_BITS) : (1 << COVERAGE_BITS)) * dy;
				dx = multiply(dy, seg->dx);
			} else {
				// Full row: use +/-(1 << (COVERAGE_BITS+FRACT_BITS)) area and advance x by (dx << FRACT_BITS).
				remaining = (coverageByX < 0)
						? -(1 << (COVERAGE_BITS + FRACT_BITS)) : (1 << (COVERAGE_BITS + FRACT_BITS));
				dx = shiftLeft(seg->dx, FRACT_BITS);
			}
			int leftX = high32(seg->x);
			int rightX = high32(add(seg->x, dx));
			sort(leftX, rightX);	// Ensure leftX <= rightX regardless of edge direction
			int leftCol = (leftX >> FRACT_BITS) - x;
			const int rightCol = (rightX >> FRACT_BITS) - x;
			const int leftSub = leftX & FRACT_MASK;
			const int rightSub = rightX & FRACT_MASK;				
			
			if (leftCol >= length) {
				// Entirely to the RIGHT of the requested span -> nothing to accumulate inside; set edges to length.
				seg->leftEdge = length;
				seg->rightEdge = length;
			} else if (rightCol < 0) {
				// Entirely to the LEFT of the requested span -> deposit all signed area at boundary 0.
				seg->leftEdge = 0;
				seg->rightEdge = 0;
				coverageDelta[0] += remaining;
			} else if (leftCol == rightCol) {
				// Both endpoints land in the SAME column -> split 'remaining' between boundaries col and col+1 (trapezoid in x).
				seg->leftEdge = leftCol;
				const int coverage = (2 * FRACT_ONE - leftSub - rightSub) * remaining >> (FRACT_BITS + 1);
				coverageDelta[leftCol + 0] += coverage;
				coverageDelta[leftCol + 1] += remaining - coverage;
				// One-past-the-rightmost column
				seg->rightEdge = leftCol + 1;
			} else {
				int covered;	// signed area already spent on the left this row (clip-left + left partial); subtracted before the right edge
				if (leftCol < 0) {
					// Enters from CLIP-LEFT: precharge boundary 0 with area up to it, then start at column 0.
					seg->leftEdge = 0;
					covered = (minValue(rightCol, 0) - leftCol) * coverageByX;
					covered += -leftSub * coverageByX >> FRACT_BITS;
					coverageDelta[0] += covered;
					leftCol = 0;
				} else {
					// Left edge INSIDE span: record leftEdge; split the left PARTIAL pixel between boundaries; advance to first interior column.
					seg->leftEdge = leftCol;
					int lx = FRACT_ONE - leftSub;
					covered = lx * coverageByX >> FRACT_BITS;
					int coverage = lx * covered >> (FRACT_BITS + 1);
					coverageDelta[leftCol + 0] += coverage;
					coverageDelta[leftCol + 1] += covered - coverage;
					++leftCol;
				}
				const int colCount = rightCol - leftCol;
				if (colCount > 0) {
					// Interior columns: uniform slope contribution -> boundary deltas follow 1/2,1,...,1,1/2 pattern.
					coverageDelta[leftCol + 0] += (coverageByX >> 1);
					const int end = minValue(leftCol + colCount, length);
					for (int col = leftCol + 1; col < end; ++col) {
						coverageDelta[col] += coverageByX;
					}
					coverageDelta[end] += coverageByX - (coverageByX >> 1);
				}
				if (rightCol < length) {
					// Right edge INSIDE span: spend what's left ('remaining - covered - interiors') in the right PARTIAL pixel.
					remaining -= covered + colCount * coverageByX;
					const int coverage = (2 * FRACT_ONE - rightSub) * remaining >> (FRACT_BITS + 1);
					coverageDelta[rightCol + 0] += coverage;
					coverageDelta[rightCol + 1] += remaining - coverage;
					// One-past-the-rightmost column
					seg->rightEdge = rightCol + 1;
				} else {
					// Exits past CLIP-RIGHT: mark rightEdge at span end; no right-partial deposit inside buffer.
					seg->rightEdge = length;
				}
			}
		}
	}
	
	// Drop segments that end at this row and restore horizontal order for remaining ones.

	int orderIndex = engagedEnd - 1;
	int sortIndex = orderIndex;
	while (orderIndex >= integrateIndex) {
		if (segsHorizontally[sortIndex]->leftEdge != -0x7FFFFFFF) {
			Segment* v = segsHorizontally[sortIndex];
			int seekIndex = orderIndex;
			while (seekIndex < engagedEnd - 1 && segsHorizontally[seekIndex + 1]->leftEdge < v->leftEdge) {
				segsHorizontally[seekIndex] = segsHorizontally[seekIndex + 1];
				++seekIndex;
			}
			segsHorizontally[seekIndex] = v;
			--orderIndex;
		}
		--sortIndex;
	}
	engagedStart = integrateIndex;

	// Integrate coverage and emit mask pixels.
	
	int coverageAcc = 0;
	int col = 0;
	while (col < length) {

		// Go to the next left-edge (first round this may be 0 if first left-edge < 0)

		int nx = ((integrateIndex < engagedEnd) ? segsHorizontally[integrateIndex]->leftEdge : length);
		if (nx > col) {
			coverageAcc += coverageDelta[col];
			const Int32 sourceCoverage[1] = { coverageAcc };
			Mask8::Pixel pixel;
			fillRule.processCoverage(1, sourceCoverage, &pixel);
			coverageDelta[col] = 0;
			output.addSolid(nx - col, pixel);
			col = nx;
		}

		// Find the end of this span by extending as long as right-edge overlaps next left-edge (with 4 pixels margin).

		if (integrateIndex < engagedEnd) {
			nx = segsHorizontally[integrateIndex]->rightEdge;
			while (integrateIndex + 1 < engagedEnd && nx + 4 >= segsHorizontally[integrateIndex + 1]->leftEdge) {
				++integrateIndex;
				nx = maxValue(segsHorizontally[integrateIndex]->rightEdge, nx);
			}
			++integrateIndex;
		}
	
		if (nx > col) {
			int spanLength = nx - col;
			for (int i = 0; i < spanLength; ++i) {
				coverageAcc += coverageDelta[col + i];
				coverageDelta[col + i] = coverageAcc;
			}
			Mask8::Pixel* pixels = output.addVariable(spanLength, false);
			fillRule.processCoverage(spanLength, &coverageDelta[col], pixels);
			for (int i = 0; i < spanLength; ++i) {
			}
			for (int i = 0; i < spanLength; ++i) {
				coverageDelta[col + i] = 0;
			}
			col = nx;
		}
	}

	coverageDelta[length] = 0; // Need to clear the extra margin element.
	if (rightClip > 0) {
		output.addTransparent(rightClip);
	}
}

NonZeroFillRule PolygonMask::nonZeroFillRule;
EvenOddFillRule PolygonMask::evenOddFillRule;

// Optimized specializations

#if (NUXPIXELS_SIMD)

inline NuXSIMD::QInt scaleSIMD(NuXSIMD::QInt argb, NuXSIMD::QInt scale) {
	using namespace NuXSIMD;
	QInt ag = zeroShiftRight16<8>(argb);							// 00aa00gg
	QInt rb = bitAnd(argb, CONST_QINT(0x00FF00FFU));				// 00rr00bb
	ag = mulLow16(ag, scale);										// aaaagggg
	rb = mulLow16(rb, scale);										// rrrrbbbb
	ag = bitAnd(ag, CONST_QINT(0xFF00FF00U));						// aa00gg00
	rb = zeroShiftRight16<8>(rb);									// 00rr00bb
	return bitOr(ag, rb);											// aarrggbb
}

inline NuXSIMD::QInt blendSIMD(NuXSIMD::QInt dest, NuXSIMD::QInt color) {
	using namespace NuXSIMD;
	QInt scale = zeroShiftRight<24>(color);							// 000000aa
	scale = sub(scale, greater(scale, CONST_QINT(0)));				// 000001aa
	scale = bitOr(NuXSIMD::shiftLeft<16>(scale), scale);			// 01aa01aa
	return add8(color, sub8(dest, scaleSIMD(dest, scale)));
}

inline NuXSIMD::QInt interpolateSIMD(NuXSIMD::QInt from, NuXSIMD::QInt to, NuXSIMD::QInt x)
{
	using namespace NuXSIMD;
	const QInt scale = bitOr(NuXSIMD::shiftLeft<16>(x), x);			// 01xx01xx
	const QInt toAG = zeroShiftRight16<8>(to);						// 00aa00gg
	const QInt toRB = bitAnd(to, CONST_QINT(0x00FF00FFU));			// 00rr00bb
	const QInt fromAG = zeroShiftRight16<8>(from);					// 00aa00gg
	const QInt fromRB = bitAnd(from, CONST_QINT(0x00FF00FFU));		// 00rr00bb
	QInt diffAG = sub16(toAG, fromAG);								// +-aa+-gg
	QInt diffRB = sub16(toRB, fromRB);								// +-rr+-bb
	diffAG = mulLow16(diffAG, scale);								// aaaagggg
	diffRB = mulLow16(diffRB, scale);								// rrrrbbbb
	diffAG = bitAnd(diffAG, CONST_QINT(0xFF00FF00U));				// aa00gg00
	diffRB = zeroShiftRight16<8>(diffRB);							// 00rr00bb
	QInt diff = bitOr(diffAG, diffRB);								// aarrggbb
	QInt sum = add8(from, diff);									// aarrggbb
	return sum;
}

// const NuXSIMD::QInt& here because MSVC (at least 2015) is buggy and can't seem to take more than 3 QInt parameters (or we get error C2719)
inline NuXSIMD::QInt interpolateSIMD(const NuXSIMD::QInt& p00, const NuXSIMD::QInt& p10, const NuXSIMD::QInt& p01
		, const NuXSIMD::QInt& p11, const NuXSIMD::QInt& x, const NuXSIMD::QInt& y)
{
	using namespace NuXSIMD;

	const QInt eq00_10 = equal8(p00, p10);
	const QInt eq10_01 = equal8(p10, p01);
	const QInt eq01_11 = equal8(p01, p11);
	const QInt combined = bitAnd(bitAnd(eq00_10, eq10_01), eq01_11);
	
#if (NUXSIMD_NEON)
	if (vminvq_u8(combined) == 0xFF) {
		return p00;
	}
#else
	if (getSigns8(combined) == 0xFFFF) {
		return p00;
	}
#endif

	NuXSIMD::QInt xs = bitOr(NuXSIMD::shiftLeft<16>(x), x);			// 01xx01xx
	NuXSIMD::QInt ys = bitOr(NuXSIMD::shiftLeft<16>(y), y);			// 01xx01xx
	
	const QInt p10AG = zeroShiftRight16<8>(p10);					// 00aa00gg
	const QInt p10RB = bitAnd(p10, CONST_QINT(0x00FF00FFU));		// 00rr00bb
	const QInt p00AG = zeroShiftRight16<8>(p00);					// 00aa00gg
	const QInt p00RB = bitAnd(p00, CONST_QINT(0x00FF00FFU));		// 00rr00bb
	QInt d_0AG = sub16(p10AG, p00AG);								// +-aa+-gg
	QInt d_0RB = sub16(p10RB, p00RB);								// +-rr+-bb
	d_0AG = mulLow16(d_0AG, xs);									// aaaagggg
	d_0RB = mulLow16(d_0RB, xs);									// rrrrbbbb
	d_0AG = zeroShiftRight16<8>(d_0AG);								// 00aa00gg
	d_0RB = zeroShiftRight16<8>(d_0RB);								// 00rr00bb
	const QInt p_0AG = add8(p00AG, d_0AG);							// 00aa00gg
	const QInt p_0RB = add8(p00RB, d_0RB);							// 00rr00bb

	const QInt p11AG = zeroShiftRight16<8>(p11);					// 00aa00gg
	const QInt p11RB = bitAnd(p11, CONST_QINT(0x00FF00FFU));		// 00rr00bb
	const QInt p01AG = zeroShiftRight16<8>(p01);					// 00aa00gg
	const QInt p01RB = bitAnd(p01, CONST_QINT(0x00FF00FFU));		// 00rr00bb
	QInt d_1AG = sub16(p11AG, p01AG);								// +-aa+-gg
	QInt d_1RB = sub16(p11RB, p01RB);								// +-rr+-bb
	d_1AG = mulLow16(d_1AG, xs);									// aaaagggg
	d_1RB = mulLow16(d_1RB, xs);									// rrrrbbbb
	d_1AG = zeroShiftRight16<8>(d_1AG);								// 00aa00gg
	d_1RB = zeroShiftRight16<8>(d_1RB);								// 00rr00bb
	const QInt p_1AG = add8(p01AG, d_1AG);							// 00aa00gg
	const QInt p_1RB = add8(p01RB, d_1RB);							// 00rr00bb
	
	QInt dAG = sub16(p_1AG, p_0AG);									// +-aa+-gg
	QInt dRB = sub16(p_1RB, p_0RB);									// +-rr+-bb
	dAG = mulLow16(dAG, ys);										// aaaagggg
	dRB = mulLow16(dRB, ys);										// rrrrbbbb
	dAG = zeroShiftRight16<8>(dAG);									// 00aa00gg
	dRB = zeroShiftRight16<8>(dRB);									// 00rr00bb
	const QInt pAG = add8(p_0AG, dAG);								// 00aa00gg
	const QInt pRB = add8(p_0RB, dRB);								// 00rr00bb
	
	return bitOr(shiftLeft16<8>(pAG), pRB);							// aarrggbb
}

template<> void copyPixels<ARGB32>(int count, ARGB32::Pixel* target, const ARGB32::Pixel* source) {
	using namespace NuXSIMD;

	if (target == source) {
		return;
	}
	
	ARGB32::Pixel* t = target;
	const ARGB32::Pixel* s = source;
	const ARGB32::Pixel* e = target + count;
	ptrdiff_t n = minValue(count, static_cast<int>((-reinterpret_cast<intptr_t>(target) & 15) >> 2));
	while (--n >= 0) {
		*t = *s;
		++t;
		++s;
	}
	if (t < e) {
		assert(isAligned(t));
		QInt* qt = reinterpret_cast<QInt*>(t);
		const QInt* qs = reinterpret_cast<const QInt*>(s);
		if (isAligned(qs)) {
			n = (e - t) >> 4;
			while (--n >= 0) {
				const QInt p0 = loadAligned(qs + 0);
				const QInt p1 = loadAligned(qs + 1);
				const QInt p2 = loadAligned(qs + 2);
				const QInt p3 = loadAligned(qs + 3);
				storeAligned(p0, qt + 0);
				storeAligned(p1, qt + 1);
				storeAligned(p2, qt + 2);
				storeAligned(p3, qt + 3);
				qt += 4;
				qs += 4;
			}
			t = reinterpret_cast<ARGB32::Pixel*>(qt);
			n = (e - t) >> 2;
			while (--n >= 0) {
				const QInt p0 = loadAligned(qs);
				storeAligned(p0, qt);
				++qt;
				++qs;
			}
			t = reinterpret_cast<ARGB32::Pixel*>(qt);
		} else {
			n = (e - t) >> 4;
			while (--n >= 0) {
				const QInt p0 = loadUnaligned(qs + 0);
				const QInt p1 = loadUnaligned(qs + 1);
				const QInt p2 = loadUnaligned(qs + 2);
				const QInt p3 = loadUnaligned(qs + 3);
				storeAligned(p0, qt + 0);
				storeAligned(p1, qt + 1);
				storeAligned(p2, qt + 2);
				storeAligned(p3, qt + 3);
				qt += 4;
				qs += 4;
			}
			t = reinterpret_cast<ARGB32::Pixel*>(qt);
			n = (e - t) >> 2;
			while (--n >= 0) {
				const QInt p0 = loadUnaligned(qs);
				storeAligned(p0, qt);
				++qt;
				++qs;
			}
			t = reinterpret_cast<ARGB32::Pixel*>(qt);
		}
		s = reinterpret_cast<const ARGB32::Pixel*>(qs);
		n = e - t;
		while (--n >= 0) {
			*t = *s;
			++t;
			++s;
		}
	}
}

template<> void fillPixels<ARGB32>(int count, ARGB32::Pixel* target, ARGB32::Pixel color) {
	using namespace NuXSIMD;
	
	ARGB32::Pixel* t = target;
	const ARGB32::Pixel* e = target + count;
	ptrdiff_t n = minValue(count, static_cast<int>((-reinterpret_cast<intptr_t>(target) & 15) >> 2));
	while (--n >= 0) {
		*t = color;
		++t;
	}
	if (t < e) {
		assert(isAligned(t));
		const QInt c = load4(color);
		QInt* qt = reinterpret_cast<QInt*>(t);
		n = (e - t) >> 6;
		while (--n >= 0) {
			storeAligned(c, qt + 0);
			storeAligned(c, qt + 1);
			storeAligned(c, qt + 2);
			storeAligned(c, qt + 3);
			storeAligned(c, qt + 4);
			storeAligned(c, qt + 5);
			storeAligned(c, qt + 6);
			storeAligned(c, qt + 7);
			storeAligned(c, qt + 8);
			storeAligned(c, qt + 9);
			storeAligned(c, qt + 10);
			storeAligned(c, qt + 11);
			storeAligned(c, qt + 12);
			storeAligned(c, qt + 13);
			storeAligned(c, qt + 14);
			storeAligned(c, qt + 15);
			qt += 16;
		}
		t = reinterpret_cast<ARGB32::Pixel*>(qt);
		n = (e - t) >> 4;
		while (--n >= 0) {
			storeAligned(c, qt + 0);
			storeAligned(c, qt + 1);
			storeAligned(c, qt + 2);
			storeAligned(c, qt + 3);
			qt += 4;
		}
		t = reinterpret_cast<ARGB32::Pixel*>(qt);
		n = (e - t) >> 2;
		while (--n >= 0) {
			storeAligned(c, qt + 0);
			++qt;
		}
		t = reinterpret_cast<ARGB32::Pixel*>(qt);
		n = e - t;
		while (--n >= 0) {
			*t = color;
			++t;
		}
	}
}

template<> void blendSolidToPixels<ARGB32>(int count, ARGB32::Pixel* target, ARGB32::Pixel foreground
		, const ARGB32::Pixel* background) {
	using namespace NuXSIMD;
	
	ARGB32::Pixel* t = target;
	const ARGB32::Pixel* e = target + count;
	const ARGB32::Pixel* b = background;
	ptrdiff_t n = minValue(count, static_cast<int>((-reinterpret_cast<intptr_t>(target) & 15) >> 2));
	while (--n >= 0) {
		*t = ARGB32::blend(*b, foreground);
		++t;
		++b;
	}
	if (t < e) {
		assert(isAligned(t));
		const QInt f = load4(foreground);
		QInt scale;
		/*{
			UInt32 s = alphaToScale((foreground >> 24) & 255);
			s |= (s << 16);
			scale = load4(s);
		}*/
		scale = zeroShiftRight<24>(f);								// 000000aa
		scale = sub(scale, greater(scale, CONST_QINT(0)));				// 000001aa
		scale = bitOr(NuXSIMD::shiftLeft<16>(scale), scale);			// 01aa01aa
		QInt* qt = reinterpret_cast<QInt*>(t);
		const QInt* qb = reinterpret_cast<const QInt*>(b);
		if (isAligned(qb)) {
			n = (e - t) >> 4;
			while (--n >= 0) {
				const QInt b0 = loadAligned(qb + 0);
				const QInt b1 = loadAligned(qb + 1);
				const QInt b2 = loadAligned(qb + 2);
				const QInt b3 = loadAligned(qb + 3);
				const QInt p0 = add8(f, sub8(b0, scaleSIMD(b0, scale)));
				const QInt p1 = add8(f, sub8(b1, scaleSIMD(b1, scale)));
				const QInt p2 = add8(f, sub8(b2, scaleSIMD(b2, scale)));
				const QInt p3 = add8(f, sub8(b3, scaleSIMD(b3, scale)));
				storeAligned(p0, qt + 0);
				storeAligned(p1, qt + 1);
				storeAligned(p2, qt + 2);
				storeAligned(p3, qt + 3);
				qt += 4;
				qb += 4;
			}
			t = reinterpret_cast<ARGB32::Pixel*>(qt);
			n = (e - t) >> 2;
			while (--n >= 0) {
				const QInt b0 = loadAligned(qb);
				const QInt p0 = add8(f, sub8(b0, scaleSIMD(b0, scale)));
				storeAligned(p0, qt);
				++qt;
				++qb;
			}
			t = reinterpret_cast<ARGB32::Pixel*>(qt);
		} else {
			n = (e - t) >> 2;
			while (--n >= 0) {
				const QInt b0 = loadUnaligned(qb);
				const QInt p0 = add8(f, sub8(b0, scaleSIMD(b0, scale)));
				storeAligned(p0, qt);
				++qt;
				++qb;
			}
			t = reinterpret_cast<ARGB32::Pixel*>(qt);
		}
		b = reinterpret_cast<const ARGB32::Pixel*>(qb);
		n = e - t;
		while (--n >= 0) {
			*t = ARGB32::blend(*b, foreground);
			++t;
			++b;
		}
	}
}

template<> void blendPixelsToPixels<ARGB32>(int count, ARGB32::Pixel* target, const ARGB32::Pixel* foreground
		, const ARGB32::Pixel* background) {
	int preroll = static_cast<int>((-reinterpret_cast<intptr_t>(target) & 15) >> 2);
	int i = 0;
	while (i < preroll && i < count) {
		target[i] = ARGB32::blend(background[i], foreground[i]);
		++i;
	}
	if (i < count) {
		assert(isAligned(target + i));
		if (isAligned(background + i) && isAligned(foreground + i)) {
			while (i + 4 <= count) {
				storeAligned(blendSIMD(loadAligned(&background[i]), loadAligned(&foreground[i])), &target[i]);
				i += 4;
			}
		} else if (isAligned(background + i)) {
			while (i + 4 <= count) {
				storeAligned(blendSIMD(loadAligned(&background[i]), loadUnaligned(&foreground[i])), &target[i]);
				i += 4;
			}
		} else if (isAligned(foreground + i)) {
			while (i + 4 <= count) {
				storeAligned(blendSIMD(loadUnaligned(&background[i]), loadAligned(&foreground[i])), &target[i]);
				i += 4;
			}
		} else {
			while (i + 4 <= count) {
				storeAligned(blendSIMD(loadUnaligned(&background[i]), loadUnaligned(&foreground[i])), &target[i]);
				i += 4;
			}
		}
		while (i < count) {
			target[i] = ARGB32::blend(background[i], foreground[i]);
			++i;
		}
	}
}

template<> void interpolatePixels<ARGB32>(int count, typename ARGB32::Pixel* target
		, const typename ARGB32::Pixel* source, int imageStride, Fixed32_32& sx, Fixed32_32& sy, const Fixed32_32 dxx
		, const Fixed32_32 dxy, int hop) {
	const int preroll = minValue(count, static_cast<int>((-reinterpret_cast<intptr_t>(target) & 15) >> 2));
	standardInterpolatePixels<ARGB32>(preroll, target, source, imageStride, sx, sy, dxx, dxy, hop);
	count -= preroll;
	while (count >= 4) {
		assert(isAligned(target));
		SIMD_ALIGN(UInt32 colFractions[4]);
		SIMD_ALIGN(UInt32 rowFractions[4]);
		SIMD_ALIGN(ARGB32::Pixel sources00[4]);
		SIMD_ALIGN(ARGB32::Pixel sources10[4]);
		SIMD_ALIGN(ARGB32::Pixel sources01[4]);
		SIMD_ALIGN(ARGB32::Pixel sources11[4]);

		for (int i = 0; i < 4; ++i) {
			colFractions[i] = low32(sx) >> 24;
			rowFractions[i] = low32(sy) >> 24;
			sources00[i] = source[0];
			sources10[i] = source[1];
			sources01[i] = source[imageStride];
			sources11[i] = source[imageStride + 1];
			source += hop + addCarry(sx, dxx) + (-addCarry(sy, dxy) & imageStride);
		}
		
		storeAligned(interpolateSIMD(loadAligned(sources00), loadAligned(sources10)
				, loadAligned(sources01), loadAligned(sources11)
				, loadAligned(colFractions), loadAligned(rowFractions)), target);
		count -= 4;
		target += 4;
	}
	standardInterpolatePixels<ARGB32>(count, target, source, imageStride, sx, sy, dxx, dxy, hop);
}

template<> void interpolatePixelsXOnly<ARGB32>(int count, typename ARGB32::Pixel* target
		, const typename ARGB32::Pixel* source, Fixed32_32& sx, const Fixed32_32 dxx, int hop) {
	const int preroll = minValue(count, static_cast<int>((-reinterpret_cast<intptr_t>(target) & 15) >> 2));
	standardInterpolatePixelsXOnly<ARGB32>(preroll, target, source, sx, dxx, hop);
	count -= preroll;
	while (count >= 4) {
		assert(isAligned(target));
		SIMD_ALIGN(UInt32 colFractions[4]);
		SIMD_ALIGN(ARGB32::Pixel sources00[4]);
		SIMD_ALIGN(ARGB32::Pixel sources10[4]);

		for (int i = 0; i < 4; ++i) {
			colFractions[i] = low32(sx) >> 24;
			sources00[i] = source[0];
			sources10[i] = source[1];
			source += hop + addCarry(sx, dxx);
		}
		
		storeAligned(interpolateSIMD(loadAligned(sources00), loadAligned(sources10), loadAligned(colFractions)), target);
		count -= 4;
		target += 4;
	}
	standardInterpolatePixelsXOnly<ARGB32>(count, target, source, sx, dxx, hop);
}

template<> void interpolatePixelsYOnly<ARGB32>(int count, typename ARGB32::Pixel* target
		, const typename ARGB32::Pixel* source, int imageStride, Fixed32_32& sy, const Fixed32_32 dxy, int hop) {
	const int preroll = minValue(count, static_cast<int>((-reinterpret_cast<intptr_t>(target) & 15) >> 2));
	standardInterpolatePixelsYOnly<ARGB32>(preroll, target, source, imageStride, sy, dxy, hop);
	count -= preroll;
	while (count >= 4) {
		assert(isAligned(target));
		SIMD_ALIGN(UInt32 rowFractions[4]);
		SIMD_ALIGN(ARGB32::Pixel sources00[4]);
		SIMD_ALIGN(ARGB32::Pixel sources01[4]);

		for (int i = 0; i < 4; ++i) {
			rowFractions[i] = low32(sy) >> 24;
			sources00[i] = source[0];
			sources01[i] = source[imageStride];
			source += hop + (-addCarry(sy, dxy) & imageStride);
		}
		
		storeAligned(interpolateSIMD(loadAligned(sources00), loadAligned(sources01), loadAligned(rowFractions)), target);
		count -= 4;
		target += 4;
	}
	standardInterpolatePixelsYOnly<ARGB32>(count, target, source, imageStride, sy, dxy, hop);
}

#endif

} // namespace NuXPixels
