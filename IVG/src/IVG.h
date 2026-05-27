/**
	IVG is released under the BSD 2-Clause License.

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

/*
	Object diagram, example scenario:
 
			+-----------------+		+-----------------+
	   +----+ sub Interpreter +----->	 FontParser	  |
	   |	|  (font parser)  |		|	 (Executor)	  |
	   |	+-----------------+		+--------+--------+
	   |									 |
	   |	+-----------------+				 |				+--------------+	 +------------+
	   +----> sub Interpreter +--+			 |			 +--> mask Context +-----> MaskMaker  |
	   |	|	  (call)	  |	 |			 |			 |	|  (current)   |	 |	(Canvas)  |
	   |	+--------+--------+	 |		  parent		 |	+--------------+	 +------------+
	 var			 |			 |		 executor		 |
	access	  +------v------+	 |	   (tracing etc)	 |	+--------------+
	   |	  |	   local	|	 |			 |			 +--> sub Context  +--+
	   |	  |	 Variables	|	 |			 |			 |	|			   |  |
	   |	  +-------------+	 |			 |			 |	+--------------+  |
	   |						 |			 |			 |					  |
	   |	+-----------------+	 |	+--------v--------+	 |	+--------------+  |	 +------------+
	   +---->	 root IMPD	  +--+-->	IVGExecutor	  +--+--> root Context +--+-->	 Canvas	  |
			|	Interpreter	  |		|				  |		|			   |	 |			  |
			+--------+--------+		+-----------------+		+--------------+	 +------------+
					 |				| inherit and	  |
			  +------v------+		| overload for	  |
			  |	 Variables	|		| custom tracing, |
			  +-------------+		| loading etc	  |
			  | inherit for |		\-----------------/
			  | custom		|
			  | handling	|
			  \-------------/
*/

#ifndef IVG_h
#define IVG_h

#include "assert.h" // Note: I always include assert.h like this so that you can override it with a "local" file.
#include "IMPD.h"
#include <cmath>
#include <memory>
#include <NuX/NuXPixels.h>

namespace IVG {

using NuXPixels::Rect; // Rect is a typedef in Carbon which can confuse the compiler so we do an explicit using for it.

inline double square(double d) { return d * d; }

void checkBounds(const NuXPixels::IntRect& bounds);

/**
	Small helper that wraps a pointer which might live on the heap.
	- If you assign a freshly created object it is kept in a std::unique_ptr
	  and deleted automatically when the Inheritable instance goes away.
	- If you only pass in an existing object the class just stores the pointer
	  and never attempts to free it.
	
	IVG keeps optional gamma tables, painters and masks in stack classes using
	this wrapper so dynamic helpers are cleaned up through RAII without extra
	code.
**/
template<class T> class Inheritable {
	public:		Inheritable() : inherited(0) { }
	public:		Inheritable(const Inheritable<T>& c) : inherited(c.inherited) { }
	public:		Inheritable(const T* o) : inherited(o), owned(const_cast<T*>(o)) { }
	public:		Inheritable& operator=(T* o) { inherited = o; owned.reset(o); return *this; }
	public:		Inheritable& operator=(const Inheritable<T>& c) {
					inherited = c.inherited;
					if (inherited != owned.get()) owned.reset();
					return *this;
				}
	public:		bool operator==(T* o) const { return inherited == o; }
	public:		bool operator!=(T* o) const { return inherited != o; }
	public:		operator const T*() const { return inherited; }
	public:		virtual ~Inheritable() { }
	protected:	const T* inherited;
	protected:	std::unique_ptr<T> owned;
};

/**
	   Global rendering options controlling gamma and quality settings.
**/
class Options {
	public:		Options() : gamma(1.0), curveQuality(1.0), patternResolution(1.0) { }
	public:		void setGamma(double newGamma);
	public:		double gamma;
	public:		double curveQuality;
	public:		double patternResolution;
	public:		Inheritable<NuXPixels::GammaTable> gammaTable;
};

class Paint;
class Context;

/**
	   Interface implemented by all painting helpers used to draw with a mask.
**/
class Painter {
	public:		virtual bool isVisible(const Paint& withPaint) const = 0;
	public:		virtual void doPaint(Paint& withPaint, Context& inContext, const Rect<double>& sourceBounds
						, const NuXPixels::Renderer<NuXPixels::Mask8>& mask) const = 0; ///< It is not legal to call doPaint if isVisible returns false. \p sourceBounds should only be used for relative paint.
	public:		virtual ~Painter() { }
};

/**
	   Describes how something is painted, including transform, opacity and
	   painter implementation.
**/
class Paint {
	public:		Paint() : relative(false), opacity(255) { }
	public:		NuXPixels::AffineTransformation transformation;
	public:		bool relative;
	public:		NuXPixels::Mask8::Pixel opacity;
	public:		Inheritable<Painter> painter;
	public:		bool isVisible() const {
					return (opacity != 0 && static_cast<const Painter*>(painter) != 0
							&& static_cast<const Painter*>(painter)->isVisible(*this));
				}
	public:		void doPaint(Context& inContext, const Rect<double>& sourceBounds
						, const NuXPixels::Renderer<NuXPixels::Mask8>& mask) {
					assert(isVisible());
					const Painter* p = painter;
					if (p != 0) p->doPaint(*this, inContext, sourceBounds, mask);
				}
};

/**
	   Holds outline properties such as width, cap and join style as well as
	   dash pattern and paint.
**/
class Stroke {
	public:		Stroke() : width(1.0), caps(NuXPixels::Path::BUTT), joints(NuXPixels::Path::MITER), miterLimit(2.0)
						, dash(0.0), gap(0.0), dashOffset(0.0) { }
	public:		Paint paint;
	public:		double width;
	public:		NuXPixels::Path::EndCapStyle caps;
	public:		NuXPixels::Path::JointStyle joints;
	public:		double miterLimit;
	public:		double dash;
	public:		double gap;
	public:		double dashOffset;
};

/**
	   Simple font containing glyph paths and metrics used for text rendering.
**/
class Font {
	public:		struct Glyph {
					IMPD::UniChar character;
					std::string svgPath;
					double advance;
				};
	
				struct KerningPair {
					std::pair<IMPD::UniChar, IMPD::UniChar> characters;
					double adjust;
				};

				struct Metrics {
					Metrics();
					double upm;		// Units Per EM. Scale down paths by this factor for font size 1.
					double ascent;
					double descent; // normally negative
					double linegap;
				};

	public:		Font();
	public:		Font(const Metrics& metrics, const std::vector<Glyph>& glyphs
						, const std::vector<KerningPair>& kerningPairs);	// glyphs and kerning pairs must be sorted!
	public:		const Glyph* findGlyph(IMPD::UniChar forCharacter) const;
	public:		const double findKerningAdjust(IMPD::UniChar characterA, IMPD::UniChar characterB) const;
	public:		const Metrics& getMetrics() const;
	protected:	Metrics metrics;
	protected:	std::vector<Glyph> glyphs;
	protected:	std::vector<KerningPair> kerningPairs;
};

/**
	   IMPD executor used for parsing and creating fonts embedded in IVG files.
**/
class FontParser : public IMPD::Executor {
	public:		FontParser(IMPD::Executor* parentExecutor = 0);
	public:		virtual bool format(IMPD::Interpreter& interpreter, const IMPD::FormatInfo& formatInfo);
	public:		virtual bool execute(IMPD::Interpreter& interpreter, const IMPD::String& instruction
						, const IMPD::String& arguments);
	public:		virtual bool progress(IMPD::Interpreter& interpreter, int maxStatementsLeft);
	public:		virtual bool load(IMPD::Interpreter& interpreter, const IMPD::WideString& filename
						, IMPD::String& contents);
	public:		virtual void trace(IMPD::Interpreter& interpreter, const IMPD::WideString& s);
	public:		virtual bool meta(IMPD::Interpreter& interpreter, const IMPD::String& key, const IMPD::String& arguments);
	public:		Font finalizeFont() const;
	protected:	IMPD::Executor* const parentExecutor;
	protected:	Font::Metrics metrics;
	protected:	typedef std::map<IMPD::UniChar, Font::Glyph> GlyphsMap;
	protected:	typedef std::map< std::pair<IMPD::UniChar, IMPD::UniChar>, double > KerningPairsMap;
	protected:	GlyphsMap glyphs;
	protected:	KerningPairsMap kerningPairs;
};

/**
	   Holds font name and painting settings for drawing text.
**/
class TextStyle {
	public:		TextStyle() : size(20.0), letterSpacing(0.0) { }
	public:		IMPD::WideString fontName;
	public:		Paint fill;
	public:		Stroke outline;
	public:		NuXPixels::AffineTransformation glyphTransform;
	public:		double size;
	public:		double letterSpacing;
};

/**
	   Snapshot of all painting state used when rendering.
**/
class State {
	public:		State() : evenOddFillRule(false) { }
	public:		NuXPixels::AffineTransformation transformation;
	public:		Options options;
	public:		bool evenOddFillRule;
	public:		Paint fill;
	public:		Stroke pen;
	public:		TextStyle textStyle;
	public:		NuXPixels::Vertex textCaret;
	public:		Inheritable< NuXPixels::RLERaster<NuXPixels::Mask8> > mask;
};

class IVGExecutor;
/**
	   Abstract drawing surface that accepts blended pixel data.
**/
class Canvas {
	protected:	template<class PIXEL_TYPE> void parsePaintOfType(IMPD::Interpreter& impd, IVGExecutor& executor, Context& context, IMPD::ArgumentsContainer& args, Paint& paint) const;
	public:		virtual void parsePaint(IMPD::Interpreter& impd, IVGExecutor& executor, Context& context, IMPD::ArgumentsContainer& args, Paint& paint) const = 0;
	public:		virtual void blendWithARGB32(const NuXPixels::Renderer<NuXPixels::ARGB32>& source) { (void)source; assert(0); }
	public:		virtual void blendWithMask8(const NuXPixels::Renderer<NuXPixels::Mask8>& source) { (void)source; assert(0); }
	public:		virtual void defineBounds(const NuXPixels::IntRect& newBounds) = 0;			///< Defines the physical boundaries of the canvas (i.e. outer bounds disregarding any current transformations). Never called more than once.
	public:		virtual NuXPixels::IntRect getBounds() const = 0;							///< Returns the outer boundaries of the canvas. A canvas may throw if bounds has not been set yet.
	public:		virtual ~Canvas() { }
	public:		template<class PIXEL_TYPE> void blend(const NuXPixels::Renderer<PIXEL_TYPE>& source);
};

/**
	   Canvas implementation used to build a mask raster.
**/
class MaskMakerCanvas : public Canvas {
	public:		MaskMakerCanvas(const NuXPixels::IntRect& bounds);
	public:		virtual void parsePaint(IMPD::Interpreter& impd, IVGExecutor& executor, Context& context, IMPD::ArgumentsContainer& args, Paint& paint) const;
	public:		virtual void blendWithARGB32(const NuXPixels::Renderer<NuXPixels::ARGB32>& source);
	public:		virtual void blendWithMask8(const NuXPixels::Renderer<NuXPixels::Mask8>& source);
	public:		virtual void defineBounds(const NuXPixels::IntRect& newBounds);
	public:		virtual NuXPixels::IntRect getBounds() const;
	public:		NuXPixels::RLERaster<NuXPixels::Mask8>* finish(bool invert);
	protected:	std::unique_ptr< NuXPixels::RLERaster<NuXPixels::Mask8> > mask8RLE;
};

/**
	   Provides drawing state and helpers while rendering on a canvas.
**/
class Context {
	friend class PatternBase;
	
	public:		Context(Canvas& canvas, const NuXPixels::AffineTransformation& initialTransform);
	public:		Context(Canvas& canvas, Context& parentContext);
	public:		void resetState();
	public:		double calcCurveQuality() const;
	public:		State& accessState() { return state; }
	public:		Canvas& accessCanvas() const { return canvas; }
	public:		NuXPixels::AffineTransformation getTransformation() const { return state.transformation; }
	public:		int calcPatternScale() const;
	public:		void stroke(const NuXPixels::Path& path, Stroke& stroke, const Rect<double>& paintSourceBounds, double widthMultiplier);
	public:		void fill(const NuXPixels::Path& path, Paint& fill, bool evenOddFillRule, const Rect<double>& paintSourceBounds);
	public:		void draw(const NuXPixels::Path& path);

	protected:	Canvas& canvas;
	protected:	State initState;
	protected:	State state;
};

/**
	   Loaded image data with resolution information.
**/
struct Image {
	Image() : raster(0), xResolution(1.0), yResolution(1.0) { }
	const NuXPixels::Raster<NuXPixels::ARGB32>* raster;
	double xResolution;
	double yResolution;
};

/**
	   Executes IVG drawing instructions within a rendering context.
**/
class IVGExecutor : public IMPD::Executor {
	public:		IVGExecutor(Canvas& canvas, const NuXPixels::AffineTransformation& initialTransform
						= NuXPixels::AffineTransformation());
	public:		virtual bool format(IMPD::Interpreter& interpreter, const IMPD::FormatInfo& formatInfo);
	public:		virtual bool execute(IMPD::Interpreter& interpreter, const IMPD::String& instruction
						, const IMPD::String& arguments);
	public:		virtual void trace(IMPD::Interpreter& interpreter, const IMPD::WideString& s);
	public:		virtual bool progress(IMPD::Interpreter& interpreter, int maxStatementsLeft);
	public:		virtual bool load(IMPD::Interpreter& interpreter, const IMPD::WideString& filename
						, IMPD::String& contents);
	public:		virtual bool meta(IMPD::Interpreter& interpreter, const IMPD::String& key, const IMPD::String& arguments);
	
				/**
					Return Image with null pointer in `raster` if image can't be loaded. Otherwise point to a raster
					whose lifetime is guaranteed until the next call of this function. IVG will *not* cache this
					pointer.
				 
					The `sourceRectangle` is optional and if null the entire image should always be returned, otherwise
					you *may* return a different clipped version of the image (which might be necessary to prevent
					"bleeding" if you supply an image in a different resolution). Regardless, IVG will always clip the
					source image (taking the resolution into consideration).
				 
					`forXSize` and `forYSize` can be used to supply different pre-scaled images depending on desired
					resolution (but it is not required). If `xSizeIsRelative` is true, `forXSize` is a ratio (1.0 being
					default), otherwise it is a pixel count (and likewise for `ySizeIsRelative` and `forYSize`).
				*/
	public:		virtual Image loadImage(IMPD::Interpreter& interpreter, const IMPD::WideString& imageSource
						, const NuXPixels::IntRect* sourceRectangle, bool forStretching, double forXSize
						, bool xSizeIsRelative, double forYSize, bool ySizeIsRelative);
				
				/**
					Returns a vector of pointers to fonts containing the necessary glyphs to display the specified string.
					Fonts are searched in the order provided in the vector. If a glyph is not found in a font, the search continues with the next font.
					Returns an empty vector if no suitable font is found. The returned font pointers must remain valid until the next call to lookupFonts().
					If `forString` is empty, just return the main font.
					If a glyph cannot be found in any font, the main font's missing glyph is used (character #0).
				**/
	public:		virtual std::vector<const Font*> lookupFonts(IMPD::Interpreter& interpreter, const IMPD::WideString& fontName
						, const IMPD::UniString& forString);
	public:		void runInNewContext(IMPD::Interpreter& impd, Context& context, const IMPD::String& source);
	public:		virtual ~IVGExecutor();
	protected:	void executeImage(IMPD::Interpreter& impd, IMPD::ArgumentsContainer& args);
	protected:	void executeDefine(IMPD::Interpreter& impd, IMPD::ArgumentsContainer& args);
	protected:	void parseStroke(IMPD::Interpreter& impd, IMPD::ArgumentsContainer& args, Stroke& stroke);
	protected:	std::vector<const Font*> lookupExternalOrInternalFonts(IMPD::Interpreter& impd
						, const IMPD::WideString& name, const IMPD::UniString& forString);
	protected:	Context rootContext;
	protected:	Context* currentContext;
	protected:	typedef std::map<IMPD::WideString, Font> FontMap;
	protected:	FontMap embeddedFonts;
	protected:	IMPD::WideString lastFontName;
	protected:	std::vector<const Font*> lastFontPointers; // empty = must look up
	protected:	typedef std::map<IMPD::WideString, Image> ImageMap;
	protected:	ImageMap definedImages;
};

/**
	   Utility that multiplies a mask by an opacity value when used.
**/
class FadedMask {
	public:		FadedMask(const NuXPixels::Renderer<NuXPixels::Mask8>& mask, NuXPixels::Mask8::Pixel opacity);
	public:		operator const NuXPixels::Renderer<NuXPixels::Mask8>&() const;
	protected:	NuXPixels::Solid<NuXPixels::Mask8> solid;
	protected:	NuXPixels::Multiplier<NuXPixels::Mask8, NuXPixels::Mask8> multiplier;
	protected:	const NuXPixels::Renderer<NuXPixels::Mask8>& output;
};

/**
	   Painter that fills using a solid color.
**/
template<class PIXEL_TYPE> class ColorPainter : public Painter {
	public:		ColorPainter(typename PIXEL_TYPE::Pixel color) : color(color) { }
	public:		virtual bool isVisible(const Paint& withPaint) const { (void)withPaint; return color != 0; }
	public:		virtual void doPaint(Paint& withPaint, Context& inContext, const Rect<double>& sourceBounds
						, const NuXPixels::Renderer<NuXPixels::Mask8>& mask) const {
					(void)sourceBounds;
					inContext.accessCanvas().blend(NuXPixels::Solid<PIXEL_TYPE>(withPaint.opacity != 255
							? PIXEL_TYPE::multiply(color, withPaint.opacity) : color) * mask);
				}
	protected:	typename PIXEL_TYPE::Pixel color;
};

/**
	   Base class for gradient painters handling stop visibility and transforms.
**/
template<class PIXEL_TYPE> class GradientPainter : public Painter {
	public:		GradientPainter(int count, const typename NuXPixels::Gradient<PIXEL_TYPE>::Stop* points)
						: gradient(count, points), visibleStops(false) {
					for (int i = 0; i < count; ++i) {
						if (!PIXEL_TYPE::isTransparent(points[i].color)) {
							visibleStops = true;
							break;
						}
					}
				}
	public:		virtual bool isVisible(const Paint& withPaint) const { (void)withPaint; return visibleStops; }
	protected:	NuXPixels::AffineTransformation calcXF(const Paint& withPaint, const Context& inContext
						, const Rect<double>& sourceBounds) const {
					if (withPaint.relative) {
						return withPaint.transformation.transform(NuXPixels::AffineTransformation()
								.scale(sourceBounds.width, sourceBounds.height)
								.translate(sourceBounds.left, sourceBounds.top)
								.transform(inContext.getTransformation()));
					} else {
						return withPaint.transformation.transform(inContext.getTransformation());
					}
				}
	protected:	NuXPixels::Gradient<PIXEL_TYPE> gradient;
	protected:	bool visibleStops;
};

/**
	   Painter that renders a linear gradient between two points.
**/
template<class PIXEL_TYPE> class LinearGradientPainter : public GradientPainter<PIXEL_TYPE> {
	public:		LinearGradientPainter(double startX, double startY, double endX, double endY, int count
						, const typename NuXPixels::Gradient<PIXEL_TYPE>::Stop* points)
						: GradientPainter<PIXEL_TYPE>(count, points), start(startX, startY), end(endX, endY) {
				}
	public:		virtual void doPaint(Paint& withPaint, Context& inContext, const Rect<double>& sourceBounds
						, const NuXPixels::Renderer<NuXPixels::Mask8>& mask) const {
					using NuXPixels::Vertex; // G++ needs this
					NuXPixels::AffineTransformation xf(this->calcXF(withPaint, inContext, sourceBounds));
					NuXPixels::Vertex xfStart = xf.transform(start);
					NuXPixels::Vertex xfEnd = xf.transform(end);
					
					// The following complicated transforms are necessary to support non-uniform scaling and shearing etc...
					
					NuXPixels::Vertex xfEnd90 = xf.transform(Vertex(start.x - end.y + start.y, start.y + end.x - start.x));
					double dx = xfEnd90.x - xfStart.x;
					double dy = xfEnd90.y - xfStart.y;
					double l = fabs((xfEnd.y - xfStart.y) * dx - (xfEnd.x - xfStart.x) * dy) / (dx * dx + dy * dy);
					xfEnd = NuXPixels::Vertex(xfStart.x + dy * l, xfStart.y - dx * l);
					
					inContext.accessCanvas().blend(this->gradient[NuXPixels::LinearAscend(xfStart.x, xfStart.y, xfEnd.x, xfEnd.y)]
							* static_cast<const NuXPixels::Renderer<NuXPixels::Mask8>&>(FadedMask(mask, withPaint.opacity)));
				}
	protected:	NuXPixels::Vertex start;
	protected:	NuXPixels::Vertex end;
};

// Sorry, the radial gradient painter does not care about any other transformations than width / height
// (rotation doesn't affect the output unless unproportionally scaled)
/**
	   Painter that draws a radial gradient based on an elliptical shape.
**/
template<class PIXEL_TYPE> class RadialGradientPainter : public GradientPainter<PIXEL_TYPE> {
	public:		RadialGradientPainter(double centerX, double centerY
						, double width, double height, int count, const typename NuXPixels::Gradient<PIXEL_TYPE>::Stop* points)
						: GradientPainter<PIXEL_TYPE>(count, points), center(centerX, centerY), size(width, height) {
				}
	public:		virtual void doPaint(Paint& withPaint, Context& inContext, const Rect<double>& sourceBounds
						, const NuXPixels::Renderer<NuXPixels::Mask8>& mask) const {
					using NuXPixels::Vertex; // G++ needs this
					NuXPixels::AffineTransformation xf(this->calcXF(withPaint, inContext, sourceBounds));
					NuXPixels::Vertex xfCenter = xf.transform(center);
					NuXPixels::Vertex xfHSize = xf.transform(Vertex(center.x + size.x, center.y));
					NuXPixels::Vertex xfVSize = xf.transform(Vertex(center.x, center.y + size.y));
					double hSize = sqrt(square(xfHSize.x - xfCenter.x) + square(xfHSize.y - xfCenter.y));
					double vSize = sqrt(square(xfVSize.x - xfCenter.x) + square(xfVSize.y - xfCenter.y));
					if (hSize == 0 || vSize == 0) {
						inContext.accessCanvas().blend(NuXPixels::Solid<PIXEL_TYPE>
								(PIXEL_TYPE::multiply(this->gradient[0], withPaint.opacity)) * mask);
					} else {
						inContext.accessCanvas().blend(this->gradient[NuXPixels::RadialAscend(xfCenter.x, xfCenter.y, hSize, vSize)]
								* static_cast<const NuXPixels::Renderer<NuXPixels::Mask8>&>(FadedMask(mask, withPaint.opacity)));
					}
				}
	protected:	NuXPixels::Vertex center;
	protected:	NuXPixels::Vertex size;
};

/**
	   Base helper for pattern painters providing a canvas to draw the pattern.
**/
class PatternBase : public Painter, public Canvas {
	public:		PatternBase(int scale);
	public:		void makePattern(IMPD::Interpreter& impd, IVGExecutor& executor, Context& parentContext, const IMPD::String& source);
	protected:	int scale;
};

/**
	   Concrete pattern painter storing the drawn image for repeated use.
**/
template<class PIXEL_TYPE> class PatternPainter : public PatternBase {
	public:		PatternPainter(int scale) : PatternBase(scale) { }
	public:		virtual void parsePaint(IMPD::Interpreter& impd, IVGExecutor& executor, Context& context, IMPD::ArgumentsContainer& args, Paint& paint) const {
					parsePaintOfType<PIXEL_TYPE>(impd, executor, context, args, paint);
				}
	public:		virtual void blendWithARGB32(const NuXPixels::Renderer<NuXPixels::ARGB32>& source) {
					if (image.get() == 0) IMPD::Interpreter::throwRunTimeError("Undeclared bounds");
					(*image) |= source;
				}
	public:		virtual void blendWithMask8(const NuXPixels::Renderer<NuXPixels::Mask8>& source) {
					if (image.get() == 0) IMPD::Interpreter::throwRunTimeError("Undeclared bounds");
					(*image) |= source;
				}
	public:		virtual bool isVisible(const Paint& withPaint) const { (void)withPaint; return (image.get() != 0); }
	public:		virtual void doPaint(Paint& withPaint, Context& inContext, const Rect<double>& sourceBounds
						, const NuXPixels::Renderer<NuXPixels::Mask8>& mask) const {
					assert(image.get() != 0);
					NuXPixels::IntRect myBounds = getBounds();
					NuXPixels::AffineTransformation xf;
					if (withPaint.relative) {
						xf = NuXPixels::AffineTransformation().scale(1.0 / scale).transform(
								withPaint.transformation.transform(NuXPixels::AffineTransformation()									
								.translate(myBounds.left, myBounds.top)
								.scale(static_cast<double>(sourceBounds.width * scale) / myBounds.width
										, static_cast<double>(sourceBounds.height * scale) / myBounds.height)
								.translate(sourceBounds.left, sourceBounds.top)
								.transform(inContext.getTransformation())));
					} else {
						xf = NuXPixels::AffineTransformation().scale(1.0 / scale)
								.transform(withPaint.transformation.transform(inContext.getTransformation()));
					}

					inContext.accessCanvas().blend(NuXPixels::Texture<PIXEL_TYPE>(*image, true
							, NuXPixels::AffineTransformation().transform(xf))
							* static_cast<const NuXPixels::Renderer<NuXPixels::Mask8>&>(FadedMask(mask, withPaint.opacity)));
				}
	public:		virtual void defineBounds(const NuXPixels::IntRect& newBounds) {
					NuXPixels::IntRect physicalBounds(newBounds.left * scale, newBounds.top * scale
						, newBounds.width * scale, newBounds.height * scale);
					if (image.get() != 0) IMPD::Interpreter::throwRunTimeError("Multiple bounds declarations");
					checkBounds(physicalBounds);
					image.reset(new NuXPixels::SelfContainedRaster<PIXEL_TYPE>(physicalBounds));
					(*image) = NuXPixels::Solid<PIXEL_TYPE>(PIXEL_TYPE::transparent());
				}
	public:		virtual NuXPixels::IntRect getBounds() const {
					if (image.get() == 0) IMPD::Interpreter::throwRunTimeError("Undeclared bounds");
					return image->calcBounds();
				}
	protected:	std::unique_ptr< NuXPixels::SelfContainedRaster<PIXEL_TYPE> > image;
};

/**
	   Canvas that renders directly into an existing ARGB32 raster.
**/
class ARGB32Canvas : public Canvas {
	public:		ARGB32Canvas(NuXPixels::Raster<NuXPixels::ARGB32>& output);
	public:		virtual void parsePaint(IMPD::Interpreter& impd, IVGExecutor& executor, Context& context, IMPD::ArgumentsContainer& args, Paint& paint) const;
	public:		virtual void blendWithARGB32(const NuXPixels::Renderer<NuXPixels::ARGB32>& source);
	public:		virtual void defineBounds(const NuXPixels::IntRect& newBounds);
	public:		virtual NuXPixels::IntRect getBounds() const;
	protected:	NuXPixels::Raster<NuXPixels::ARGB32>& argb32Raster;
};

// FIX : if we templetize this one as a generic offscreenCanvas it is virtually identical to the one in the PatternPainter
// it could also expand to an RLEARGB32Canvas, but that is probably always slower than drawing to a bitmap canvas and then compressing
/**
	   Canvas with its own ARGB32 buffer allocated internally.
**/
class SelfContainedARGB32Canvas : public Canvas {
	public:		SelfContainedARGB32Canvas(const double rescaleBounds = 1.0); // rescaleBounds can be used to create a canvas for a different target resolution (just supply the same scale for the initial transform of the root context).
	public:		virtual void parsePaint(IMPD::Interpreter& impd, IVGExecutor& executor, Context& context, IMPD::ArgumentsContainer& args, Paint& paint) const;
	public:		virtual void blendWithARGB32(const NuXPixels::Renderer<NuXPixels::ARGB32>& source);
	public:		virtual void defineBounds(const NuXPixels::IntRect& newBounds);
	public:		virtual NuXPixels::IntRect getBounds() const;
	public:		NuXPixels::SelfContainedRaster<NuXPixels::ARGB32>* accessRaster();
	public:		NuXPixels::SelfContainedRaster<NuXPixels::ARGB32>* relinquishRaster();
	protected:	void checkBoundsDeclared() const;
	protected:	std::unique_ptr< NuXPixels::SelfContainedRaster<NuXPixels::ARGB32> > raster;
	protected:	const double rescaleBounds;
};

NuXPixels::ARGB32::Pixel parseColor(const IMPD::String& color);

bool buildPathFromSVG(const IMPD::String& svgSource, double curveQuality, NuXPixels::Path& path, const char*& errorString);
bool buildPathForString(const IMPD::UniString& string, const std::vector<const Font*>& fonts, double size
				, const NuXPixels::AffineTransformation& glyphTransform, double letterSpacing, double curveQuality
				, NuXPixels::Path& path, double& advance, const char*& errorString, IMPD::UniChar lastCharacter = 0);

} // namespace IVG

#endif
