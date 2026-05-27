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
/**
	\file NuXPixels.h
*/

/*
	IDEA:
	
	Everything is a "filler" (but with another term). I.e., even polygon rasterization is done by requesting to fill solid or variable, and the polygon coverage is multiplied into the output...
	Thus we could mask directly by sending a polygon to another polygon and then to a gradient or whatever...
	
	WILDER IDEAS:
	
	Generalize concepts of mask vs. colors, all classes that deal with them are templates so that we can have different mask and color types. Thus, masking is no different than drawing with a
	multiplying color for instance. The "mask" class I have now would be a generic output-raster template class (that could for example hold color images). Even the polygon rasterizer could actually
	work on colors instead of coverage (adding and subtracting the entire argb thing). That would actually enable us to draw adjacing colored polygons with no boundary artifacts!! I don't think
	I've never seen that.
	
	(Actually, thinking about this now again, I am not sure it works, think of 50% red + 50% blue, it will still mix the background there if we do this naively, it seems the only way would be to accumulate coverage PLUS rgb so that we can match, i.e. coverage = 100%, r=50%, b=50% : solid mix between red and blue.
	
	To avoid using classes to wrap ints etc we should have a class with static members that defines the operations on the type. I.e. RGBColor { typedef int PrimitiveType; PrimitiveType add(PrimitiveType a, PrimitiveType b) { blah blah } }
	
	PUSH vs PULL:
	
	I am slowly realizing that a pull design is probably better than push. I.e., top-most you have the blender, which request stuff from the rasterizers instead of the other way around.
	The reason is that we could then combine output from different renderers in the pipe. Like one rasterizer mixing the output from two different rasterizers *before* blending it to the output (this is actually what we do in Microtonic already).
	
	PUSH pros: easier interface, simply call the two different methods for solid or variable coverage / colors, no need for an output-data concept, one renderer can send its output to different places (like on-screen clones of images).
	PUSH cons: tough to combine different inputs, you'll have to render to one or several buffers first and then combine that
	
	PULL pros: one target can combine output from several sources (like masking or mixing before printing to screen buffer)
	PULL cons: need output-compressed data instead of the different methods for solid or variable coverage / colors (this is actually required mainly if we are to combine different sources).

	NEW IDEA 20070517 : colors (ARGB32, Mask8 etc) could be real classes, the classes to render that color solid, i.e. instead of Solid<T>
	
	TODO 20070511:
	
	* We should rewrite the polygon rasterizer to first setup the edges for the entire row, and second render the pixel data for each requested span.
	* Perhaps the fill-rules should be an enum instead.
	* Optimizing of paths.
	* Minimum/maximum operations?
	* Outline renderer
	* Should paths have a traverser class to traverse sub-paths?
	(20071026)	Should Rect<>() and Point<>() really reset its members, doesn't that slow down some stuff? I know it is good OO-practice, but this is a high-performance lib.
	(20131014)	Additive blender could optimize away adding 0x??000000 (e.g. from an opaque source image) with 0xFF?????? as the output will always be 0xFF??????

	DONE:

	* We should create a new faster span merger.
	* Storage of output-compressed data.
	* Gradients should be generalized to take any type of color, we can skip the gamma-correction I think, just to get simpler code.
	* Lookup should be able to use any class that supports [], right?
	* Generalize the different blending classes, they are very similar already.
	* Can unary operations operate directly on the output data? (Not if source / target have different formats, like the converter and lookup, but otherwise?)
	* Can all template methods/functions be in cpp? (nope, but in a separate h)
	* Should renderers have bounds?
		(20071014)	YES! It now has.
	* Bezier-curves.
		(20130106)	Done.

	BUGS:

	(20180923)	raster = background | raster  creates vertical background lines in output ... newRaster = background | raster works fine (rasters were self-contained in the test, background was solid white)
	(20180923)	my miter-limit is apparently a so called "clipped" limit... standard miter-limit is to simply use bevel when miter length exceeds limit and not to clip it like I do... also, the actual limit formula might differ at least from SVG
*/

#ifndef NuXPixels_h
#define NuXPixels_h

#ifndef NUXPIXELS_SIMD
#define NUXPIXELS_SIMD 1
#endif

/**
	Intel-compiler has a very severe optimization bug with construction of arrays. Empty inline constructors are not
	removed in compilation. NuXPixels constructs *a lot* of Span arrays, so this adds up to an extreme CPU overhead.
 
	The workaround is constructing byte-arrays and ugly-casting them to Span arrays instead.
**/
#ifndef NUXPIXELS_ICC_HACK
#define NUXPIXELS_ICC_HACK 1
#endif

/**
	Maximum length of a pixel span expressed as a power of 2. Usually these spans are stored on the stack so they should
	be reasonably small. Using complex rendering pipes can cause a lot of these to be placed on the stack, but 8 (= 256
	pixels) ought to work for normal use. 9 (= 512 pixels) gives slightly better performance. After 10 (= 1024 pixels)
	you usually don't see any performance improvement.
**/
#ifndef NUXPIXELS_MAX_SPAN
#define NUXPIXELS_MAX_SPAN 8
#endif

/**
	64-bit emulation routines are required if int64_t is not supported and even if int64_t is supported by the compiler,
	using the emulation routines may improve performance if 64-bit integers are not supported by native machine-code.
**/
#ifndef NUXPIXELS_EMULATE_INT64
#define NUXPIXELS_EMULATE_INT64 0
#endif

#include "assert.h"
#include <cstdint>
#include <cstdlib>
#include <vector>

#if NUXPIXELS_EMULATE_INT64
#undef NUXPIXELS_INT64_TYPE
#else
#ifndef NUXPIXELS_INT64_TYPE
#define NUXPIXELS_INT64_TYPE int64_t
#endif
#endif

namespace NuXPixels {

typedef uint8_t UInt8;
typedef uint16_t UInt16;
typedef int32_t Int32;
typedef uint32_t UInt32;

const double PI = 3.141592653589793238462643383279502884197;
const double PI2 = (PI * 2.0);
const double EPSILON = 0.00000001;
const int MAX_SPAN_BITS = NUXPIXELS_MAX_SPAN;
const int MAX_RENDER_LENGTH = (1 << MAX_SPAN_BITS);
const int RADIAL_SQRT_BITS = 12;

/* --- Utility routines --- */

/**
	Microsoft still have problems with std::min and std::max (the old C macros sometimes get in the
	way). Besides, in MSVC 8 they are a bit slow due to debugging stuff, even in release builds.
**/
template<typename T> T minValue(T a, T b) { return (a < b) ? a : b; }
template<typename T> T maxValue(T a, T b) { return (a > b) ? a : b; }
inline int roundToInt(double d) { return int(floor(d + 0.5)); }
inline int wrap(int x, int y) { return (x >= 0) ? (x % y) : (y - 1 - (-x - 1) % y); }

#ifdef NUXPIXELS_INT64_TYPE

typedef NUXPIXELS_INT64_TYPE Fixed32_32;

inline Fixed32_32 toFixed32_32(Int32 high, UInt32 low) throw() { return (Fixed32_32(high) << 32) | low; }
inline Fixed32_32 toFixed32_32(double d) throw() { return Fixed32_32(floor(d * 4294967296.0 + 0.5)); }
inline Fixed32_32 add(Fixed32_32 v1, Fixed32_32 v2) throw() { return v1 + v2; }
inline Int32 addCarry(Fixed32_32& v1, Fixed32_32 v2) throw() { Int32 carry = Int32((Fixed32_32((UInt32)(v1)) + (UInt32)(v2)) >> 32); v1 += v2; return carry; }
inline Fixed32_32 shiftLeft(Fixed32_32 v, Int32 s) throw() { return v << s; }
inline Fixed32_32 shiftRight(Fixed32_32 v, Int32 s) throw() { return v >> s; }
inline Int32 high32(Fixed32_32 v) throw() { return static_cast<Int32>(v >> 32); }
inline UInt32 low32(Fixed32_32 v) throw() { return static_cast<UInt32>(v); }
inline Fixed32_32 divide(Int32 v1, Int32 v2) throw() { return (Fixed32_32(v1) << 32) / v2; }
inline Fixed32_32 multiply(Int32 v1, Fixed32_32 v2) throw() { return v1 * v2; }

#else // #ifdef NUXPIXELS_INT64_TYPE

/**
	Fixed32_32 represents a 32.32 fixed-point value for precision math.

	example:
	Fixed32_32 half = toFixed32_32(0, 0x80000000);
	Fixed32_32 one = multiply(2, half);
**/
class Fixed32_32 {
	public:		Fixed32_32() { }
	public:		Fixed32_32(Int32 high, UInt32 low) : high(high), low(low) { }
	public:		Int32 high;
	public:		UInt32 low;
};

Fixed32_32 divide(Int32 v1, Int32 v2) throw();

inline Fixed32_32 toFixed32_32(Int32 high, UInt32 low) throw() { return Fixed32_32(high, low); }
inline Fixed32_32 toFixed32_32(double d) throw() { d += (0.5 / 4294967296.0); return Fixed32_32(Int32(floor(d)), (UInt32)((d - floor(d)) * 4294967296.0)); }
inline Fixed32_32 add(Fixed32_32 v1, Fixed32_32 v2) throw() { return Fixed32_32(v1.high + v2.high + (v1.low + v2.low < v1.low), v1.low + v2.low); }
inline Int32 addCarry(Fixed32_32& v1, Fixed32_32 v2) throw() { Int32 carry = (v1.low + v2.low < v1.low); v1 = Fixed32_32(v1.high + v2.high + carry, v1.low + v2.low); return carry; }
inline Fixed32_32 shiftLeft(Fixed32_32 v, Int32 s) throw() { Int32 h = (v.high << s) | (v.low >> (32 - s) & -(s != 0)); UInt32 l = v.low << s; return Fixed32_32(h, l); }
inline Fixed32_32 shiftRight(Fixed32_32 v, Int32 s) throw() { Int32 h = v.high >> s; UInt32 l = (v.high << (32 - s)) | (v.low >> s & -(s != 0)); return Fixed32_32(h, l); }
inline Int32 high32(Fixed32_32 v) throw() { return v.high; }
inline UInt32 low32(Fixed32_32 v) throw() { return v.low; }
inline Fixed32_32 negate(Fixed32_32 v) throw() { return Fixed32_32(~v.high + (v.low == 0), ~v.low + 1); }

inline Fixed32_32 multiply(UInt32 v1, Fixed32_32 v2) throw() {
	UInt32 a = v2.low >> 16;
	UInt32 b = v2.low & 0xFFFF;
	UInt32 c = v1 >> 16;
	UInt32 d = v1 & 0xFFFF;
	UInt32 e = b * d;
	UInt32 f = (e >> 16) + a * d + c * b;
	Fixed32_32 r(v1 * v2.high + a * c + (f >> 16), (f << 16) | (e & 0xFFFF));
	return r;
}

inline Fixed32_32 multiply(UInt16 v1, Fixed32_32 v2) throw() {
	UInt32 a = (v2.low & 0xFFFF) * v1;
	UInt32 b = (a >> 16) + (v2.low >> 16) * v1;
	Fixed32_32 r(v1 * v2.high + (b >> 16), (b << 16) | (a & 0xFFFF));
	return r;
}

#endif // #ifdef NUXPIXELS_INT64_TYPE

template<typename T> class Point {
	public:		Point();
	public:		Point(T x, T y);
	public:		bool operator==(const Point<T>& other) const;
	public:		bool operator!=(const Point<T>& other) const;
	public:		void swap(Point<T>& other);
	public:		T x;
	public:		T y;
};

template<typename T> class Rect {
	public:		Rect();
	public:		Rect(T left, T top, T width, T height);
	public:		T calcRight() const;
	public:		T calcBottom() const;
	public:		bool isEmpty() const;
	public:		Rect<T> offset(T x, T y) const;
	public:		Rect<T> calcUnion(const Rect<T>& other) const;
	public:		Rect<T> calcIntersection(const Rect<T>& other) const;
	public:		bool operator==(const Rect<T>& other) const;
	public:		bool operator!=(const Rect<T>& other) const;
	public:		void swap(Rect<T>& other);
	public:		T left;
	public:		T top;
	public:		T width;
	public:		T height;
};

typedef Point<int> IntPoint;
typedef Rect<int> IntRect;
typedef Point<double> Vertex;

extern const IntRect EMPTY_RECT;
extern const IntRect FULL_RECT; // top:-0x40000000, left:-0x40000000, width:0x7FFFFFFF, height:0x7FFFFFFF

// FIX : I don't like the incosistency here, Path methods are destructive, but AffineTransformation methods return new transformations. (Except invert!!)
// FIX : transformation concatenation is done in different order in different APIs, but the most common in 2D (e.g. Quartz, Java) seems to be the opposite of what I have chosen here. We should perhaps create a new matrix class with the opposite concat order and a backwards compatibility class (derived) for my personal old projects.

/**
	AffineTransformation stores a 2x3 matrix used to transform vertices.

	example:
	AffineTransformation t = AffineTransformation().translate(10, 5).rotate(0.5);
**/
class AffineTransformation {
	public:		AffineTransformation();
	public:		AffineTransformation(const double matrix[2][3]);
	public:		AffineTransformation(double m00, double m01, double m02, double m10, double m11, double m12);
	public:		AffineTransformation translate(double x, double y) const;
	public:		AffineTransformation scale(double v) const; // FIX : add scale around a center point
	public:		AffineTransformation scale(double x, double y) const;
	public:		AffineTransformation rotate(double t) const;
	public:		AffineTransformation rotate(double t, double x, double y) const;
	public:		AffineTransformation shear(double x, double y) const;
	public:		AffineTransformation transform(const AffineTransformation& transformation) const;
	public:		bool invert();
	public:		Vertex transform(const Vertex& xy) const;
	public:		bool operator==(const AffineTransformation& other) const { return std::equal(&matrix[0][0], &matrix[0][0] + 2 * 3, &other.matrix[0][0]); }
	public:		bool operator!=(const AffineTransformation& other) const { return !(*this == other); }
	public:		double matrix[2][3];
	
// FIX : add + / -, * etc...
// FIX : add isIdentity
};

/**
	Path records drawing commands for shapes that can be filled or stroked.

	example:
	Path p;
	p.moveTo(0,0).lineTo(10,10).close();
**/
class Path {
	public:		enum EndCapStyle { BUTT, ROUND, SQUARE };
	public:		enum JointStyle { BEVEL, CURVE, MITER };
	public:		enum Operation { MOVE, LINE, CLOSE };
	
	public:		typedef std::pair<Operation, Vertex> Instruction;
	public:		typedef std::vector<Instruction> InstructionsVector;
	public:		typedef InstructionsVector::size_type size_type;
	public:		typedef InstructionsVector::const_iterator const_iterator;
	public:		Path();
	public:		Path& clear();
	public:		IntRect calcIntBounds() const;
	public:		Rect<double> calcFloatBounds() const;
	public:		Vertex getPosition() const;
	public:		Path& moveTo(double x, double y);
	public:		Path& lineTo(double x, double y);
	public:		Path& quadraticTo(double controlPointX, double controlPointY, double x, double y, double curveQuality = 1.0);
	public:		Path& cubicTo(double cpBeginX, double cpBeginY, double cpEndX, double cpEndY, double x, double y, double curveQuality = 1.0);
	public:		Path& arcSweep(double centerX, double centerY, double sweepRadians, double aspectRatio = 1.0, double curveQuality = 1.0);
	public:		Path& append(const Path& p);
	public:		Path& addLine(double startX, double startY, double endX, double endY);
	public:		Path& addRect(double left, double top, double width, double height); // FIX : <- phase out? only have addRect(Rect<double>)?
	public:		template<class T> Path& addRect(const Rect<T>& rect) { addRect(rect.left, rect.top, rect.width, rect.height); return *this; }
	public:		Path& addEllipse(double centerX, double centerY, double radiusX, double radiusY, double curveQuality = 1.0);
	public:		Path& addCircle(double centerX, double centerY, double radius, double curveQuality = 1.0);
	public:		Path& addRoundedRect(double left, double top, double width, double height, double cornerWidth, double cornerHeight, double curveQuality = 1.0);
	public:		Path& addStar(double centerX, double centerY, int points, double radius1, double radius2, double rotation); // Equation to map a star polygon with p and q to a description with two radii: r2 == r1 * cos(q * (PI / p)) / cos((q - 1) * (PI / p))
	public:		Path& close();
	public:		Path& closeAll();
	public:		Path& stroke(double width, EndCapStyle endCaps = BUTT, JointStyle joints = BEVEL, double miterLimit = 2.0, double curveQuality = 1.0);
	public:		Path& dash(double dashLength, double gapLength, double dashOffset = 0.0);
	public:		Path& transform(const AffineTransformation& transformation);
	public:		bool empty() const;
	public:		size_type size() const;
	public:		const_iterator begin() const;
	public:		const_iterator end() const;
	protected:	InstructionsVector instructions;
	protected:	size_type openIndex;
};
inline Path::Path() : openIndex(static_cast<size_type>(-1)) { }
inline Path& Path::clear() { instructions.clear(); openIndex = static_cast<size_type>(-1); return *this; }
inline Vertex Path::getPosition() const { return (instructions.empty() ? Vertex(0.0, 0.0) : instructions.back().second); }
inline Path& Path::moveTo(double x, double y) { instructions.push_back(Instruction(MOVE, Vertex(x, y))); openIndex = size() - 1; return *this; }
inline Path& Path::lineTo(double x, double y) { instructions.push_back(Instruction(LINE, Vertex(x, y))); return *this; }
inline bool Path::empty() const { return instructions.empty(); }
inline Path::size_type Path::size() const { return instructions.size(); }
inline Path::const_iterator Path::begin() const { return instructions.begin(); }
inline Path::const_iterator Path::end() const  { return instructions.end(); }

inline UInt32 alphaToScale(UInt8 alpha) { return alpha + (alpha != 0 ? 1 : 0); }	// 0 = 0, 1..255 = 2..256
// FIX: not correct (should be 0..1 = 0, 2..256 = 1..255
inline UInt8 scaleToAlpha(UInt32 scale) { assert(0 <= scale && scale <= 256); return scale - (scale >> 8); }

/**
	ARGB32 is a 32-bit pixel with alpha, red, green and blue components.
**/
class ARGB32 {
	public:		enum { COMPONENT_COUNT = 4 };
	public:		typedef UInt32 Pixel;
	public:		static Pixel* allocate(int count);	// use these to allocate and free with SIMD alignment on Windows 32-bit
	public:		static void free(Pixel* pixels);	// use these to allocate and free with SIMD alignment on Windows 32-bit
	public:		static bool isValid(Pixel color);
	public:		static bool isOpaque(Pixel color);
	public:		static bool isTransparent(Pixel color);
	public:		static bool isMaximum(Pixel color);
	public:		static Pixel transparent();
	public:		static Pixel maximum();
	public:		static Pixel invert(Pixel color);
	public:		static Pixel add(Pixel a, Pixel b);		// FIX : it would actually be better if these were located outside of classes so that we could introduce new pixel-types and make new routines for operating against existing types (without having to modify this class).
	public:		static Pixel multiply(Pixel c1, Pixel c2);
	public:		static Pixel scale(Pixel c1, UInt32 scale);
	public:		static Pixel multiply(Pixel a, UInt8 alpha);
	public:		static Pixel blend(Pixel dest, Pixel color);
	public:		static Pixel interpolate(Pixel from, Pixel to, UInt32 x);
	public:		static Pixel interpolate(Pixel p00, Pixel p10, Pixel p01, Pixel p11, UInt32 x, UInt32 y);
	public:		static Pixel fromFloatRGB(double r, double g, double b, double a);
	public:		static Pixel fromFloatHSV(double h, double s, double v, double a);
	public:		static void split(Pixel c, UInt8 components[COMPONENT_COUNT]);
	public:		static Pixel join(const UInt8 components[COMPONENT_COUNT]);
};

inline bool ARGB32::isValid(Pixel color)
{
	UInt32 a = (color >> 24);
	return ((color & 0x00FF0000) <= (a << 16) && (color & 0x0000FF00) <= (a << 8) && (color & 0x000000FF) <= a);
}

inline bool ARGB32::isOpaque(Pixel color) { return (color >= 0xFF000000); }
inline bool ARGB32::isTransparent(Pixel color) { return (color == 0); }
inline bool ARGB32::isMaximum(ARGB32::Pixel color) { return (color == 0xFFFFFFFF); }
inline ARGB32::Pixel ARGB32::transparent() { return 0; }
inline ARGB32::Pixel ARGB32::maximum() { return 0xFFFFFFFF; }
inline ARGB32::Pixel ARGB32::invert(ARGB32::Pixel color) { return ~color; }

inline ARGB32::Pixel ARGB32::add(ARGB32::Pixel a, ARGB32::Pixel b)
{
	ARGB32::Pixel c = b & 0x80808080;
	ARGB32::Pixel d = a & 0x80808080;
	ARGB32::Pixel e = (b & 0x7F7F7F7F) + (a & 0x7F7F7F7F);
	ARGB32::Pixel f = c | d;
	return (e | f) | (0x7F7F7F7F ^ (0x7F7F7F7F + (((c & d) | (e & f)) >> 7)));
}

inline ARGB32::Pixel ARGB32::multiply(ARGB32::Pixel c1, ARGB32::Pixel c2)
{
	const UInt32 a = ((c1 >> 24) & 0xFF) * alphaToScale(static_cast<UInt8>(c2 >> 24)) >> 8;
	const UInt32 r = ((c1 >> 16) & 0xFF) * alphaToScale(static_cast<UInt8>(c2 >> 16)) >> 8;
	const UInt32 g = ((c1 >> 8) & 0xFF) * alphaToScale(static_cast<UInt8>(c2 >> 8)) >> 8;
	const UInt32 b = ((c1 >> 0) & 0xFF) * alphaToScale(static_cast<UInt8>(c2)) >> 8;
	return (a << 24) | (r << 16) | (g << 8) | b;
}

inline ARGB32::Pixel ARGB32::scale(ARGB32::Pixel a, UInt32 scale)
{
	const UInt32 rb = static_cast<UInt32>(a & 0x00FF00FFU) * scale >> 8;
	const UInt32 ag = static_cast<UInt32>((a & 0xFF00FF00U) >> 8) * scale;
	return (rb & 0xFF00FF) + (ag & 0xFF00FF00U);
}

inline ARGB32::Pixel ARGB32::multiply(ARGB32::Pixel a, UInt8 alpha) { return scale(a, alphaToScale(alpha)); }

inline ARGB32::Pixel ARGB32::blend(ARGB32::Pixel dest, ARGB32::Pixel color) {
	return color + dest - scale(dest, alphaToScale(color >> 24));
}

inline ARGB32::Pixel ARGB32::interpolate(ARGB32::Pixel from, ARGB32::Pixel to, UInt32 x) {
	assert(0 <= x && x <= 256);
	UInt32 fromAG = (from & 0xFF00FF00);
	UInt32 fromRB = (from & 0x00FF00FF);
	UInt32 toAG = (to & 0xFF00FF00);
	UInt32 toRB = (to & 0x00FF00FF);
	UInt32 ag = fromAG + static_cast<UInt32>((toAG >> 8) - (fromAG >> 8)) * x;
	UInt32 rb = fromRB + ((toRB - fromRB) * x >> 8);
	ARGB32::Pixel argb = (ag & 0xFF00FF00) + (rb & 0x00FF00FF);
	return argb;
}

inline ARGB32::Pixel ARGB32::interpolate(Pixel p00, Pixel p10, Pixel p01, Pixel p11, UInt32 x, UInt32 y) {
	assert(0 <= x && x <= 256);
	
	if (p00 == p10 && p10 == p01 && p01 == p11) {
		return p00;
	}
	
	const UInt32 p00AG = (p00 & 0xFF00FF00U);
	const UInt32 p00RB = (p00 & 0x00FF00FFU);
	const UInt32 p10AG = (p10 & 0xFF00FF00U);
	const UInt32 p10RB = (p10 & 0x00FF00FFU);
	const UInt32 p01AG = (p01 & 0xFF00FF00U);
	const UInt32 p01RB = (p01 & 0x00FF00FFU);
	const UInt32 p11AG = (p11 & 0xFF00FF00U);
	const UInt32 p11RB = (p11 & 0x00FF00FFU);

	const UInt32 ag_0 = (p00AG + static_cast<UInt32>((p10AG >> 8) - (p00AG >> 8)) * x) & 0xFF00FF00U;
	const UInt32 rb_0 = (p00RB + ((p10RB - p00RB) * x >> 8)) & 0x00FF00FFU;
	const UInt32 ag_1 = (p01AG + static_cast<UInt32>((p11AG >> 8) - (p01AG >> 8)) * x) & 0xFF00FF00U;
	const UInt32 rb_1 = (p01RB + ((p11RB - p01RB) * x >> 8)) & 0x00FF00FFU;

	const UInt32 ag = (ag_0 + static_cast<UInt32>((ag_1 >> 8) - (ag_0 >> 8)) * y) & 0xFF00FF00U;
	const UInt32 rb = (rb_0 + ((rb_1 - rb_0) * y >> 8)) & 0x00FF00FFU;
	const ARGB32::Pixel argb = ag | rb;

	return argb;
}

inline void ARGB32::split(Pixel c, UInt8 components[COMPONENT_COUNT]) {
	components[0] = ((c >> 24) & 0xFF);
	components[1] = ((c >> 16) & 0xFF);
	components[2] = ((c >> 8) & 0xFF);
	components[3] = ((c >> 0) & 0xFF);
}

inline ARGB32::Pixel ARGB32::join(const UInt8 components[COMPONENT_COUNT]) {
	return (components[0] << 24) | (components[1] << 16) | (components[2] << 8) | components[3];
}

/**
	Mask8 holds 8-bit coverage values for masking and blending.
**/
class Mask8 {
	public:		enum { COMPONENT_COUNT = 1 };
	public:		typedef UInt8 Pixel;
	public:		static Pixel* allocate(int count);	// use these to allocate and free with SIMD alignment on Windows 32-bit
	public:		static void free(Pixel* pixels);	// use these to allocate and free with SIMD alignment on Windows 32-bit
	public:		static bool isValid(Pixel color);
	public:		static bool isOpaque(Pixel color);
	public:		static bool isTransparent(Pixel color);
	public:		static bool isMaximum(Pixel color);
	public:		static Pixel transparent();
	public:		static Pixel maximum();
	public:		static Pixel invert(Pixel color);
	public:		static Pixel add(Pixel a, Pixel b);
	public:		static Pixel multiply(Pixel a, UInt8 b);
	public:		static Pixel blend(Pixel dest, Pixel color);
	public:		static Pixel interpolate(Pixel from, Pixel to, UInt32 x);
	public:		static Pixel interpolate(Pixel p00, Pixel p10, Pixel p01, Pixel p11, UInt32 x, UInt32 y);
	public:		static void split(Pixel c, UInt8 components[COMPONENT_COUNT]);
	public:		static Pixel join(const UInt8 components[COMPONENT_COUNT]);
};
inline bool Mask8::isValid(Mask8::Pixel /*color*/) { return true; } // Invalid pixel value is impossible.
inline bool Mask8::isOpaque(Mask8::Pixel color) { return (color == 0xFF); }
inline bool Mask8::isTransparent(Mask8::Pixel color) { return (color == 0); }
inline bool Mask8::isMaximum(Mask8::Pixel color) { return (color == 0xFF); }
inline Mask8::Pixel Mask8::transparent() { return 0; }
inline Mask8::Pixel Mask8::maximum() { return 0xFF; }
inline Mask8::Pixel Mask8::invert(Mask8::Pixel color) { return ~color; }
inline Mask8::Pixel Mask8::add(Mask8::Pixel a, Mask8::Pixel b) { return minValue(a + b, 0xFF); }
inline Mask8::Pixel Mask8::multiply(Mask8::Pixel a, UInt8 b) { return a * alphaToScale(b) >> 8; }
inline Mask8::Pixel Mask8::blend(Mask8::Pixel dest, Mask8::Pixel color) { return add(dest, color); }
inline Mask8::Pixel Mask8::interpolate(Mask8::Pixel from, Mask8::Pixel to, UInt32 x) { return from + ((to - from) * x >> 8); }
inline Mask8::Pixel Mask8::interpolate(Pixel p00, Pixel p10, Pixel p01, Pixel p11, UInt32 x, UInt32 y) {
	return interpolate(interpolate(p00, p10, x), interpolate(p01, p11, x), y);
}
inline void Mask8::split(Pixel c, UInt8 components[COMPONENT_COUNT]) { components[0] = c; }
inline Mask8::Pixel Mask8::join(const UInt8 components[COMPONENT_COUNT]) { return components[0]; }

inline Mask8::Pixel convert(const ARGB32&, const Mask8&, ARGB32::Pixel source) { return source >> 24; }
inline ARGB32::Pixel convert(const Mask8&, const ARGB32&, Mask8::Pixel source) { return (source << 24) | (source << 16) | (source << 8) | source; }

/**
	   Span models a run of consecutive pixels. The run length and the "solid"
	   and "opaque" flags are packed into a 32-bit field. When the span is solid,
	   `pixels` points to a single color repeated for the entire run; otherwise
	   it addresses an array containing one pixel per position.
**/
template<class T> class Span {
	public:		Span();
	public:		Span(int length, bool solid, bool opaque, const typename T::Pixel* pixels);
	public:		bool isSolid() const;
	public:		bool isOpaque() const;
	public:		bool isTransparent() const;
	public:		bool isMaximum() const;
	public:		int getLength() const;
	public:		typename T::Pixel getSolidPixel() const;
	public:		const typename T::Pixel* getVariablePixels() const;
	public:		const typename T::Pixel* getPixelPointer() const;
	protected:	UInt32 lengthAndFlags;
	protected:	const typename T::Pixel* pixels;
};

// FIX : special SpanBuffer class for owned spans / pixels?
#if (NUXPIXELS_ICC_HACK)
	#define NUXPIXELS_SPAN_ARRAY(T, n) char (n)[NuXPixels::MAX_RENDER_LENGTH * sizeof (NuXPixels::Span<T>)];
#elif (!NUXPIXELS_ICC_HACK)
	#define NUXPIXELS_SPAN_ARRAY(T, n) NuXPixels::Span<T> (n)[NuXPixels::MAX_RENDER_LENGTH];
#endif

/**
	   SpanBuffer stores runs of pixels in two parallel arrays. When a span of
	   length `n` is added, `n` entries are reserved in the span array. The
	   first entry holds the span itself while the last entry duplicates it so
	   the iterator can read the previous span's length when stepping
	   backwards. Entries in between are unused but make pointer arithmetic work
	   for both forward and backward iteration. Pixel data are appended to the
	   pixel array in tandem—a solid span stores one color, whereas a variable
	   span stores `n` colors.
**/
template<class T> class SpanBuffer {
	public:		class iterator;
#if (NUXPIXELS_ICC_HACK)
	public:		SpanBuffer(char* spans, typename T::Pixel* pixels);
#elif (!NUXPIXELS_ICC_HACK)
	public:		SpanBuffer(Span<T>* spans, typename T::Pixel* pixels);
#endif
	public:		iterator begin() const;
	public:		iterator end() const;
	public:		void add(int length, const Span<T>& span);
	public:		void addSpan(const Span<T>& span);
	public:		void addTransparent(int length);
	public:		void addSolid(int length, const typename T::Pixel& pixel);
	public:		typename T::Pixel* preallocatePixels() const;
	public:		typename T::Pixel* addVariable(int length, bool opaque);
	public:		void addReference(int length, const typename T::Pixel* pixels, bool opaque);
	public:		void split(iterator it, int splitPoint); /// This method is a low-level method used for "merging" two SpanBuffers. It only splits for "forward iteration", but if you would iterate backwards the array would still be invariant since the split doesn't modify any pixel contents.
	protected:	Span<T>* spans;
	protected:	typename T::Pixel* pixels;
	protected:	Span<T>* endSpan;
	protected:	typename T::Pixel* endPixel;
};

template<class A, class B> void merge(SpanBuffer<A>& spansA, SpanBuffer<B>& spansB, typename SpanBuffer<A>::iterator itA, typename SpanBuffer<B>::iterator itB);
template<class T> class Blender;
template<class T> class Adder;
template<class A, class B> class Multiplier;
template<class S, class T> class Converter;
template<class T> class Inverter;
template<class T> class Offsetter;

/**
	Renderer is the abstract base for anything that can produce pixel spans.

	example:
	myRaster.fill(Solid<ARGB32>(0xff0000ff), area);
**/
template<class T> class Renderer {
	public:		virtual IntRect calcBounds() const = 0;
	public:		virtual void render(int x, int y, int length, SpanBuffer<T>& output) const = 0;
	public:		Blender<T> operator|(const Renderer<T>& b) const { return Blender<T>(*this, b); }
	public:		Adder<T> operator+(const Renderer<T>& b) const { return Adder<T>(*this, b); }
	public:		template<class B> Multiplier<T, B> operator*(const Renderer<B>& b) const { return Multiplier<T, B>(*this, b); }
	public:		Inverter<T> operator~() const { return Inverter<T>(*this); }
	public:		Offsetter<T> operator+(IntPoint offset) const { return Offsetter<T>(*this, offset.x, offset.y); }
	public:		Offsetter<T> operator-(IntPoint offset) const { return Offsetter<T>(*this, -offset.x, -offset.y); }
	public:		virtual ~Renderer();
};

// FIX : no inlines here (?)
/**
	Raster represents an in-memory pixel buffer that can be rendered to. It does not own the memory.
**/
template<class T> class Raster : public Renderer<T> {
	public:		Raster(typename T::Pixel* pixels, int stride, const IntRect& bounds, bool opaque);	/// Warning! If opaque is true you must never have transparent pixels in this raster.
	public:		virtual IntRect calcBounds() const;
	public:		virtual void render(int x, int y, int length, SpanBuffer<T>& output) const;
	public:		void fill(const Renderer<T>& source, const IntRect& area);
	public:		typename T::Pixel& operator[](int index) const { return pixels[index]; }
	public:		typename T::Pixel getPixel(int x, int y) const;
	public:		void setPixel(int x, int y, const typename T::Pixel& p);
	public:		Raster<T>& operator=(const Renderer<T>& source);
	public:		Raster<T>& operator|=(const Renderer<T>& source);
	public:		Raster<T>& operator+=(const Renderer<T>& source);
	// FIX : check that transparent * transparent == transparent before unioning
	public:		template<class B> Raster<T>& operator*=(const Renderer<B>& source) { (*this) = (*this) * source; return (*this); }
	public:		typename T::Pixel* getPixelPointer() const { return pixels; }
	public:		int getStride() const { return stride; }
	public:		bool isOpaque() const { return opaque; }
	protected:	Raster(); // for SelfContainedRaster
	protected:	typename T::Pixel* pixels;	/// The address of the topmost scanline. The 0,0 coordinate should point to this pixel.
	protected:	int stride;					/// The stride with which one should offset the pointer for every increasing row (often referred to as "row bytes", although this value is not a byte count but an int count). Can be negative in case the offscreen orientation is upside down (in this case the base address should actually point to the last scanline in memory, which is now the topmost line).
	protected:	IntRect bounds;				/// Access outside this rect is illegal as it might be outside allocated memory bounds.
	protected:	bool opaque;
};
template<class T> typename T::Pixel Raster<T>::getPixel(int x, int y) const {
	assert(bounds.left <= x && bounds.top <= y && x < bounds.calcRight() && y < bounds.calcBottom());
	return pixels[y * stride + x];
}
template<class T> void Raster<T>::setPixel(int x, int y, const typename T::Pixel& p) {
	assert(bounds.left <= x && bounds.top <= y && x < bounds.calcRight() && y < bounds.calcBottom());
	pixels[y * stride + x] = p;
}

/**
	SelfContainedRaster owns its memory and behaves like a Raster.
**/
template<class T> class SelfContainedRaster : public Raster<T> {
	public:		SelfContainedRaster();
	public:		SelfContainedRaster(const IntRect& bounds, bool opaque = false);	/// Warning! If opaque is true you must never have transparent pixels in this raster.
	public:		SelfContainedRaster(const SelfContainedRaster& that);
	public:		SelfContainedRaster& operator=(const SelfContainedRaster& that);
	public:		Raster<T>& operator=(const Renderer<T>& source) { return Raster<T>::operator=(source); }
	public:		virtual ~SelfContainedRaster();
	protected:	typename T::Pixel* allocated;
};

/**
	Solid renders a single constant pixel value for all spans.

	example:
	Solid<ARGB32> blue(0xff0000ff);
**/
template<class T> class Solid : public Renderer<T> {
	public:		Solid(const typename T::Pixel& pixel);
	public:		virtual IntRect calcBounds() const;
	public:		virtual void render(int x, int y, int length, SpanBuffer<T>& output) const;
	protected:	typename T::Pixel pixel;
};

/**
	RLERaster stores spans in run-length encoded form to save memory.

	example:
	RLERaster<Mask8> cache(area, mask);
**/
template<class T> class RLERaster : public Renderer<T> {
	public:		RLERaster(const IntRect& bounds, const Renderer<T>& source = Solid<T>(T::transparent()));
	public:		virtual IntRect calcBounds() const;
	public:		virtual void render(int x, int y, int length, SpanBuffer<T>& output) const;
	public:		void fill(const Renderer<T>& source);
	public:		RLERaster<T>& operator=(const Renderer<T>& source);
	public:		RLERaster<T>& operator|=(const Renderer<T>& source);
	public:		RLERaster<T>& operator+=(const Renderer<T>& source);
	public:		template<class B> RLERaster<T>& operator*=(const Renderer<B>& source) { (*this) = (*this) * source; return (*this); }
	public:		void swap(RLERaster<T>& other);
	public:		bool isOpaque() const;
	protected:	RLERaster();
	protected:	void rewind();
	protected:	IntRect bounds;
	protected:	std::vector<UInt16> spans;
	protected:	std::vector<typename T::Pixel> pixels;
	protected:	std::vector< std::pair<size_t, size_t> > rows;
	protected:	mutable int lastX;
	protected:	mutable int lastY;
	protected:	mutable size_t lastSpanIndex;
	protected:	mutable size_t lastPixelIndex;
	protected:	bool opaque;
};

/**
	SolidRect quickly fills a rectangle with a solid color.

	example:
	SolidRect<ARGB32> box(0xffffffff, IntRect(10,10,50,20));
**/
template<class T> class SolidRect : public Renderer<T> {
	public:		SolidRect(const typename T::Pixel& pixel, const IntRect& rect);
	public:		virtual IntRect calcBounds() const;
	public:		virtual void render(int x, int y, int length, SpanBuffer<T>& output) const;
	protected:	typename T::Pixel pixel;
	protected:	IntRect rect;
};

// FIX : common super-class for all filters? (i.e. renderers having a source)
/**
	Clipper confines a renderer's output to a rectangular area.

	example:
	Clipper<ARGB32> clipped(texture, IntRect(0,0,32,32));
**/
template<class T> class Clipper : public Renderer<T> {
	public:		Clipper(const Renderer<T>& source, const IntRect& rect);
	public:		virtual IntRect calcBounds() const;
	public:		virtual void render(int x, int y, int length, SpanBuffer<T>& output) const;
	protected:	const Renderer<T>& source;
	protected:	IntRect rect;
};

/**
	Offsetter translates the coordinates of another renderer. Operator + and - (with a IntPoint) in Renderer<T>
	returns an Offsetter<T>.

	example:
	texture + IntPoint(2, 2);
**/
template<class T> class Offsetter : public Renderer<T> {
	public:		Offsetter(const Renderer<T>& source, int offsetX, int offsetY);
	public:		virtual IntRect calcBounds() const;
	public:		virtual void render(int x, int y, int length, SpanBuffer<T>& output) const;
	protected:	const Renderer<T>& source;
	protected:	int offsetX;
	protected:	int offsetY;
};

// FIX : a unary operators calcBounds is always either FULL or source's bounds, depending on the result of converting a transparent pixel, so why not just test a converting a transparent pixel?
/**
	UnaryOperator processes each pixel from a source renderer individually.
**/
template<class S, class T> class UnaryOperator : public Renderer<T> {
	public:		UnaryOperator(const Renderer<S>& source);
	public:		virtual void process(int count, const typename S::Pixel* source, typename T::Pixel* target, bool& opaque) const = 0;
	protected:	void render(int x, int y, int length, SpanBuffer<S>& inputBuffer, SpanBuffer<T>& output) const;
	protected:	template<class U> void render(int x, int y, int length, const Renderer<U>&, SpanBuffer<T>& output) const;
	protected:	void render(int x, int y, int length, const Renderer<T>&, SpanBuffer<T>& output) const;
	public:		virtual void render(int x, int y, int length, SpanBuffer<T>& output) const;
	protected:	const Renderer<S>& source;
};

/**
	Lookup applies a lookup table to mask values to produce pixels.

	example:
	Lookup<ARGB32, LookupTable<ARGB32>> shaded(mask, table);
**/
template<class T, class L> class Lookup : public UnaryOperator<Mask8, T> {
	public:		typedef UnaryOperator<Mask8, T> super;
	public:		Lookup(const Renderer<Mask8>& source, const L& table);
	public:		virtual IntRect calcBounds() const;
	public:		virtual void process(int count, const Mask8::Pixel* source, typename T::Pixel* target, bool& opaque) const;
	protected:	const L& table;
};

/**
	Inverter flips the color components of its source renderer. operator ~ in Renderer<T> returns an Inverter<T>.

	example:
	Inverter<ARGB32> neg(image);
**/
template<class T> class Inverter : public UnaryOperator<T, T> {
	public:		Inverter(const Renderer<T>& source);
	public:		virtual IntRect calcBounds() const;
	public:		virtual void process(int count, const typename T::Pixel* source, typename T::Pixel* target, bool& opaque) const;
};

/**
	Converter transforms pixels from one format to another.

	example:
	Converter<Mask8, ARGB32> toColor(maskRenderer);
**/
template<class S, class T> class Converter : public UnaryOperator<S, T> {
	public:		typedef UnaryOperator<S, T> super;
	public:		Converter(const Renderer<S>& source);
	public:		virtual IntRect calcBounds() const;
	public:		virtual void process(int count, const typename S::Pixel* source, typename T::Pixel* target, bool& opaque) const;
};

/**
	LinearAscend produces a linear gradient mask between two points.
	
	example:
	Gradient<ARGB32> grad(0xff0000ff, 0xffffffff);
	canvas |= grad[LinearAscend(0, 0, 0, 100)];
**/
class LinearAscend : public Renderer<Mask8> { // FIX : name? LinearMask, LinearAscent?
	public:		LinearAscend(double startX, double startY, double endX, double endY);
	public:		virtual IntRect calcBounds() const;
	public:		virtual void render(int x, int y, int length, SpanBuffer<Mask8>& output) const;
	protected:	int start;
	protected:	int dx;
	protected:	int dy;
};

/**
	RadialAscend creates a radial gradient mask around a center point.
	
	example:
	Gradient<ARGB32> grad(0xffff0000, 0xff00ff00);
	canvas |= grad[RadialAscend(50, 50, 40, 40)];
**/
class RadialAscend : public Renderer<Mask8> {
	public:		RadialAscend(double centerX, double centerY, double width, double height);	// width and height must be non-zero
	public:		virtual IntRect calcBounds() const;
	public:		virtual void render(int x, int y, int length, SpanBuffer<Mask8>& output) const;
	protected:	double centerX;
	protected:	double centerY;
	protected:	double width;
	protected:	double height;
	protected:	double hk;
	protected:	double wk;
	protected:	static Mask8::Pixel sqrtTable[1 << RADIAL_SQRT_BITS];
};

/**
	Texture samples from an image raster with an affine transformation.

	example:
	Texture<ARGB32> tex(image, true, AffineTransformation().scale(2));
**/
template<class T> class Texture : public Renderer<T> {
	public:		Texture(const Raster<T>& image, bool wrap = true, const AffineTransformation& transformation = AffineTransformation(), const IntRect& sourceRect = FULL_RECT);
	public:		virtual IntRect calcBounds() const;
	public:		virtual void render(int x, int y, int length, SpanBuffer<T>& output) const;
	public:		virtual ~Texture();
	protected:	class Impl;
	protected:	Impl* impl;
};

/**
	BinaryOperator combines output from two renderers pixel by pixel.
**/
template<class A, class B> class BinaryOperator : public Renderer<A> {
	public:		BinaryOperator(const Renderer<A>& rendererA, const Renderer<B>& rendererB);
	protected:	const Renderer<A>& rendererA;
	protected:	const Renderer<B>& rendererB;
	protected:	mutable typename B::Pixel pixelsB[MAX_RENDER_LENGTH];
};

/**
	Blender overlays one renderer on top of another using alpha blending. Operator | in Renderer<T> returns a Blender<T>.

	example:
	result.fill(Blender<ARGB32>(background, overlay), area);
**/
template<class T> class Blender : public BinaryOperator<T, T> {
	public:		typedef BinaryOperator<T, T> super;
	public:		Blender(const Renderer<T>& rendererA, const Renderer<T>& rendererB); /// rendererA is background, rendererB is overlay
	public:		virtual IntRect calcBounds() const;
	public:		void render(int x, int y, int length, SpanBuffer<T>& output) const;
	protected:	IntRect boundsA;
	protected:	IntRect boundsB;
};

/**
	Adder simply adds pixel values from two renderers. Operator + in Renderer<T> returns an Adder<T>.

	example:
	Adder<Mask8> combined(maskA, maskB);
**/
template<class T> class Adder : public BinaryOperator<T, T> {
	public:		typedef BinaryOperator<T, T> super;
	public:		Adder(const Renderer<T>& rendererA, const Renderer<T>& rendererB);
	public:		virtual IntRect calcBounds() const;
	public:		void render(int x, int y, int length, SpanBuffer<T>& output) const;
};

/**
	Multiplier multiplies pixel values from two renderers.

	example:
	Multiplier<ARGB32, Mask8> masked(image, mask);
**/
template<class A, class B> class Multiplier : public BinaryOperator<A, B> {
	public:		typedef BinaryOperator<A, B> super;
	public:		Multiplier(const Renderer<A>& rendererA, const Renderer<B>& rendererB);
	public:		virtual IntRect calcBounds() const;
	public:		void render(int x, int y, int length, SpanBuffer<A>& output) const;
};

/**
	Optimizer analyzes spans from a renderer to minimize redundant output.
**/
template<class T> class Optimizer : public Renderer<T> {
	public:		Optimizer(const Renderer<T>& source);
	public:		virtual IntRect calcBounds() const;
	public:		virtual void render(int x, int y, int length, SpanBuffer<T>& output) const;
	protected:	static const typename T::Pixel* outputVariable(const typename T::Pixel* b, const typename T::Pixel* e, bool opaque, SpanBuffer<T>& output);
	protected:	static const typename T::Pixel* analyzeSolid(const typename T::Pixel* b, const typename T::Pixel* e, SpanBuffer<T>& output);
	protected:	static const typename T::Pixel* analyzeOpaque(const typename T::Pixel* b, const typename T::Pixel* e, SpanBuffer<T>& output);
	protected:	static const typename T::Pixel* analyzeNonOpaque(const typename T::Pixel* b, const typename T::Pixel* e, SpanBuffer<T>& output);
	protected:	const Renderer<T>& source;
};

// FIX : switch on these instead, class bloating...
/**
	FillRule determines how polygon winding produces coverage.
**/
class FillRule {
	public:		virtual void processCoverage(int span, const Int32* source, Mask8::Pixel* destination) const = 0;
	public:		virtual ~FillRule() = 0;
};

/**
	NonZeroFillRule counts winding direction to determine fill.
**/
class NonZeroFillRule : public FillRule {
	public:		virtual void processCoverage(int span, const Int32* source, Mask8::Pixel* destination) const;
};

/**
	EvenOddFillRule toggles fill state every time an edge is crossed.
**/
class EvenOddFillRule : public FillRule {
	public:		virtual void processCoverage(int span, const Int32* source, Mask8::Pixel* destination) const;
};

/**
	PolygonMask rasterizes a path into a coverage mask using a fill rule.

	It works by:

		1. Converting each path edge into a vertical segment and sorting them by their top Y.
		2. Scanning rows from top to bottom while maintaining an active list of segments ordered by X.
		3. For each row, accumulating edge coverage per column to build a span buffer.
		4. Applying the selected fill rule to turn accumulated coverage into mask pixels.
	
	The clip rectangle is clamped to the numeric limits handled by the rasterizer. After construction callers may
	query `isValid()`; it returns `false` if any vertex falls outside the fixed-point range and the mask will produce
	no coverage.

	Rendering rows in ascending order is most efficient. Calling `render` with a `y` lower than a prior call rewinds
	the mask so scanning restarts from the top.
**/
class PolygonMask : public Renderer<Mask8> {
	public:		static NonZeroFillRule nonZeroFillRule;
	public:		static EvenOddFillRule evenOddFillRule;
	public:		PolygonMask(const Path& path,
						const IntRect& clipBounds = FULL_RECT,
						const FillRule& fillRule = nonZeroFillRule);   /// `clipBounds` is clamped; must cover destination raster.
	public:		virtual IntRect calcBounds() const;
	public:		virtual void render(int x, int y, int length, SpanBuffer<Mask8>& output) const;
	public:		void rewind() const;
	public: 	bool isValid() const;	/// false if path had out-of-range vertices
	
	protected:	struct Segment {
					int topY;			/// Starting y in fixed fraction format (fraction precision = POLYGON_FRACTION_BITS).
					int bottomY;		/// Ending y in fixed fraction format (fraction precision = POLYGON_FRACTION_BITS).
					int currentY;		/// Current y in fixed fraction format (fraction precision = POLYGON_FRACTION_BITS).
					Fixed32_32 x;		/// Current x in fixed super-fractional format (fraction precision = POLYGON_FRACTION_BITS + 32).
					Fixed32_32 dx;		/// Delta x for each row (fraction precision = POLYGON_FRACTION_BITS + 32).
					int coverageByX;	/// Absolute coverage delta for each column (precision = renderCoverFractionBits).
					int leftEdge;		/// Last left edge pixel.
					int rightEdge;		/// Last right edge pixel.
					struct Order;
				};

	protected:	std::vector<Segment> segments;
	protected:	IntRect bounds;
	protected:	const FillRule& fillRule;
	protected:	mutable int row;
	protected:	mutable int engagedStart;
	protected:	mutable int engagedEnd;
	protected:	mutable std::vector<Int32> coverageDelta;
	protected:	mutable std::vector<Segment*> segsVertically;
	protected:	mutable std::vector<Segment*> segsHorizontally;
	protected:	bool valid;
};


/**
	LookupTable holds 256 entries for mapping mask values to colors.
**/
template<class T> class LookupTable {
	protected:	LookupTable();
	protected:	LookupTable(const typename T::Pixel* table, bool opaque);
	public:		bool isOpaque() const { return opaque; }
	public:		typename T::Pixel operator[](int index) const { assert(0 <= index && index < 256); return table[index]; }
	public:		Lookup<T, LookupTable> operator[](const Renderer<Mask8>& source) const { return Lookup<T, LookupTable>(source, *this); }
	protected:	typename T::Pixel table[256];
	protected:	bool opaque;
};

/**
	GammaTable precomputes gamma corrected values for mask pixels.
**/
class GammaTable : public LookupTable<Mask8> {
	public:		GammaTable(double gamma);
};

/**
	Gradient interpolates between color stops to fill pixels.

	example:
	Gradient<ARGB32>::Stop stops[] = {{0.0, 0xff0000ff}, {1.0, 0xffffffff}};
	Gradient<ARGB32> grad(2, stops);
**/
template<class T> class Gradient : public LookupTable<T> {
	public:		class Stop {
					public:		double position;
					public:		typename T::Pixel color;
				};
	public:		typedef LookupTable<T> super;
	public:		Gradient(int count, const Stop* points);
	public:		Gradient(typename T::Pixel start, typename T::Pixel end);
	protected:	void init(int count, const Stop* points);
};

} // namespace NuXPixels
					
#endif
