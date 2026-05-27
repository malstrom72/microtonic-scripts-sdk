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

#include <algorithm>
#include <iostream>
#include <cstring>
#include <locale>
#include <cmath>
#include "IVG.h"

namespace IVG {

// Undefine horrible, horrible Microsoft macros.
#undef min
#undef max

using namespace std;
using namespace IMPD;
using namespace NuXPixels;

using NuXPixels::PI;
using NuXPixels::PI2;
using IMPD::WideChar;
using IMPD::UniChar;

const double DEGREES = PI2 / 360.0;
const double MIN_CURVE_QUALITY = 0.001;
const double MAX_CURVE_QUALITY = 100.0;
const double COORDINATE_LIMIT = 1000000.0;

void checkBounds(const IntRect& bounds) {
	if (bounds.left < -32768 || bounds.left >= 32768) {
		Interpreter::throwRunTimeError(String("bounds left out of range [-32768..32767]: ")
				+ Interpreter::toString(bounds.left));
	}
	if (bounds.top < -32768 || bounds.top >= 32768) {
		Interpreter::throwRunTimeError(String("bounds top out of range [-32768..32767]: ")
				+ Interpreter::toString(bounds.top));
	}
	if (bounds.width <= 0 || bounds.width >= 32768) {
		Interpreter::throwRunTimeError(String("bounds width out of range [1..32767]: ")
				+ Interpreter::toString(bounds.width));
	}
	if (bounds.height <= 0 || bounds.height >= 32768) {
		Interpreter::throwRunTimeError(String("bounds height out of range [1..32767]: ")
				+ Interpreter::toString(bounds.height));
	}
}
static StringIt eatSpace(StringIt p, const StringIt& e) {
	while (p != e && (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')) ++p;
	return p;
}

static StringIt eatSpaceAndComma(StringIt p, const StringIt& e) {
	p = eatSpace(p, e);
	if (p != e && *p == ',') p = eatSpace(p + 1, e);
	return p;
}

static Vertex toAbsoluteVertex(const Path& path, bool sourceIsRelative, const Vertex& sourceVertex) {
	if (!sourceIsRelative) return sourceVertex;
	else {
		Vertex pos(path.getPosition());
		return Vertex(pos.x + sourceVertex.x, pos.y + sourceVertex.y);
	}
}

static bool parseInt(StringIt& p, const StringIt& e, int32_t& v) {
	assert(p <= e);
	StringIt q = p;
	bool negative = (e - q > 1 && (*q == '+' || *q == '-') ? (*q++ == '-') : false);
	int32_t i = 0;
	if (q == e || *q < '0' || *q > '9') return false;
	else {
		p = q;
		for (; p != e && *p >= '0' && *p <= '9'; ++p) i = i * 10 + (*p - '0');
		v = negative ? -i : i;
		return true;
	}
}

static bool parseSingleCoordinate(StringIt& p, const StringIt& e, double& v) {
	assert(p <= e);
	StringIt q = Interpreter::parseDouble(p, e, v);
	if (q == p || !isfinite(v) || fabs(v) > COORDINATE_LIMIT) return false;
	p = q;
	return true;
}

static bool parseCoordinatePair(StringIt& p, const StringIt& e, Vertex& vertex, bool acceptLeadingComma) {
	StringIt q = (acceptLeadingComma ? eatSpaceAndComma(p, e) : eatSpace(p, e));
	if (!parseSingleCoordinate(q, e, vertex.x)) return false;
	q = eatSpaceAndComma(q, e);
	if (!parseSingleCoordinate(q, e, vertex.y)) return false;
	p = q;
	return true;
}

static int parseNumberList(const Interpreter& impd, const StringRange& r, double numbers[], int minElems, int maxElems) {
	StringVector elems;
	int count = impd.parseList(r, elems, true, false, minElems, maxElems);
	for (int i = 0; i < count; ++i) numbers[i] = impd.toDouble(elems[i]);
	return count;
}

static Mask8::Pixel parseOpacity(const Interpreter& impd, const StringRange& r) {
	unsigned int i;
	if (r.e != r.b && *r.b == '#') {
		StringIt p = Interpreter::parseHex(r.b + 1, r.e, i);
		if (p - (r.b + 1) != 2) impd.throwBadSyntax(String("Invalid opacity: ") + String(r.b + 1, r.e));
	} else {
		double d = impd.toDouble(r);
		if (d < 0.0 || d > 1.0) impd.throwRunTimeError(String("opacity out of range [0..1]: ") + impd.toString(d));
		i = min(static_cast<int>(d * 256), 255);
	}
	assert(0 <= i && i < 256);
	return static_cast<Mask8::Pixel>(i);
}

bool buildPathFromSVG(const String& svgSource, double curveQuality, Path& path, const char*& errorString) {
	assert(curveQuality > 0.0);
	StringIt p = svgSource.begin();
	StringIt e = svgSource.end();
	Vertex quadraticReflectionPoint;
	Vertex cubicReflectionPoint;

	p = eatSpace(p, e);
	if (p == e) return true;
	if (*p != 'M' && *p != 'm') {
		errorString = "SVG path must begin with 'M'";
		return false;
	}

	while (p != e) {
		p = eatSpace(p, e);
		if (p != e) {
			Char c = *p++;
			const bool isRelative = (c >= 'a' && c <= 'z');
			c = (isRelative ? c - ('a' - 'A') : c);
			if (c != 'T') {
				quadraticReflectionPoint = Vertex(0.0, 0.0);
			}
			if (c != 'S') {
				cubicReflectionPoint = Vertex(0.0, 0.0);
			}
			bool first = true;
			switch (c) {
				case 'M': {
					Vertex v;
					if (!parseCoordinatePair(p, e, v, false)) {
						errorString = "Invalid M syntax in svg path data";
						return false;
					}
					v = toAbsoluteVertex(path, isRelative, v);
					path.moveTo(v.x, v.y);
					while (parseCoordinatePair(p, e, v, true)) {
						v = toAbsoluteVertex(path, isRelative, v);
						path.lineTo(v.x, v.y);
					}
					break;
				}
				
				case 'L': {
					Vertex v;
					if (!parseCoordinatePair(p, e, v, false)) {
						errorString = "Invalid L syntax in svg path data";
						return false;
					}
					do {
						v = toAbsoluteVertex(path, isRelative, v);
						path.lineTo(v.x, v.y);
					} while (parseCoordinatePair(p, e, v, true));
					break;
				}

				case 'H': case 'V': { // FIX : is H and V without arguments allowed here?
					Vertex pos(path.getPosition());
					double v;
					StringIt q = eatSpace(p, e);
					while (parseSingleCoordinate(q, e, v)) {
						p = q;
						if (c == 'H') {
							if (isRelative) pos.x += v;
							else pos.x = v;
						} else {
							if (isRelative) pos.y += v;
							else pos.y = v;
						}
						path.lineTo(pos.x, pos.y);
						q = eatSpaceAndComma(p, e);
					}
					break;
				}

				case 'C': { // FIX : is C without arguments allowed here?
					Vertex bcp;
					Vertex ecp;
					Vertex v;
					StringIt q = p;
					while (parseCoordinatePair(q, e, bcp, !first)
							&& parseCoordinatePair(q, e, ecp, true)
							&& parseCoordinatePair(q, e, v, true)) {
						first = false;
						p = q;
						bcp = toAbsoluteVertex(path, isRelative, bcp);
						ecp = toAbsoluteVertex(path, isRelative, ecp);
						v = toAbsoluteVertex(path, isRelative, v);
						cubicReflectionPoint = Vertex(v.x - ecp.x, v.y - ecp.y);
						path.cubicTo(bcp.x, bcp.y, ecp.x, ecp.y, v.x, v.y, curveQuality);
					}
					break;
				}

				case 'S': { // FIX : is S without arguments allowed here?
					Vertex ecp;
					Vertex v;
					StringIt q = p;
					while (parseCoordinatePair(q, e, ecp, !first)
							&& parseCoordinatePair(q, e, v, true)) {
						first = false;
						p = q;
						Vertex pos(path.getPosition());
						Vertex bcp(pos.x + cubicReflectionPoint.x, pos.y + cubicReflectionPoint.y);
						ecp = toAbsoluteVertex(path, isRelative, ecp);
						v = toAbsoluteVertex(path, isRelative, v);
						cubicReflectionPoint = Vertex(v.x - ecp.x, v.y - ecp.y);
						path.cubicTo(bcp.x, bcp.y, ecp.x, ecp.y, v.x, v.y, curveQuality);
					}
					break;
				}

				case 'Q': { // FIX : is Q without arguments allowed here?
					Vertex cp;
					Vertex v;
					StringIt q = p;
					while (parseCoordinatePair(q, e, cp, !first)
							&& parseCoordinatePair(q, e, v, true)) {
						first = false;
						p = q;
						cp = toAbsoluteVertex(path, isRelative, cp);
						v = toAbsoluteVertex(path, isRelative, v);
						quadraticReflectionPoint = Vertex(v.x - cp.x, v.y - cp.y);
						path.quadraticTo(cp.x, cp.y, v.x, v.y, curveQuality);
					}
					break;
				}

				case 'T': { // FIX : is T without arguments allowed here?
					Vertex v;
					StringIt q = p;
					while (parseCoordinatePair(q, e, v, !first)) {
						first = false;
						p = q;
						Vertex pos(path.getPosition());
						Vertex cp(pos.x + quadraticReflectionPoint.x, pos.y + quadraticReflectionPoint.y);
						v = toAbsoluteVertex(path, isRelative, v);
						quadraticReflectionPoint = Vertex(v.x - cp.x, v.y - cp.y);
						path.quadraticTo(cp.x, cp.y, v.x, v.y, curveQuality);
					}
					break;
				}
				
				case 'A': { // FIX : is A without arguments allowed here?
					Vertex radii;
					double xAxisRotation;
					int32_t largeArcFlag;
					int32_t sweepFlag;
					Vertex v;
					StringIt q = p;
					while (parseCoordinatePair(q, e, radii, !first)
							&& ((void)(q = eatSpaceAndComma(q, e)), parseSingleCoordinate(q, e, xAxisRotation))
							&& ((void)(q = eatSpaceAndComma(q, e)), parseInt(q, e, largeArcFlag))
							&& ((void)(q = eatSpaceAndComma(q, e)), parseInt(q, e, sweepFlag))
							&& parseCoordinatePair(q, e, v, true)) {
						first = false;
						p = q;
						v = toAbsoluteVertex(path, isRelative, v);
						radii.x = fabs(radii.x);
						radii.y = fabs(radii.y);
						if (radii.x >= EPSILON && radii.y >= EPSILON) {
							Vertex startPos(path.getPosition());
							Vertex endPos(v);
							AffineTransformation affineReverse;
							if (xAxisRotation != 0.0) {
								affineReverse = AffineTransformation().rotate(xAxisRotation * (PI2 / 360.0));
								AffineTransformation affineForward = affineReverse;
								bool success = affineForward.invert();
								(void)success;
								assert(success);
								startPos = affineForward.transform(startPos);
								endPos = affineForward.transform(endPos);
							}
							double dx = endPos.x - startPos.x;
							double dy = endPos.y - startPos.y;
							if (fabs(dx) >= EPSILON || fabs(dy) >= EPSILON) {
								double largeArcSign = (largeArcFlag != 0 ? 1.0 : -1.0);
								double sweepSign = (sweepFlag != 0 ? largeArcSign : -largeArcSign);
								double aspectRatio = radii.x / radii.y;
								double l = dx * dx + (aspectRatio * dy) * (aspectRatio * dy);
								double b = max(4.0 * radii.x * radii.x / l - 1.0, EPSILON);
								double a = sweepSign * sqrt(b * 0.25);
								double centerX = startPos.x + dx * 0.5 + a * dy * aspectRatio;
								double centerY = startPos.y + dy * 0.5 - a * dx / aspectRatio;
								double sweepRadians = sweepSign * (largeArcSign * PI + PI - acos((b - 1.0) / (1.0 + b)));
								if (xAxisRotation != 0.0) {
									Path tempPath;
									tempPath.lineTo(startPos.x, startPos.y);
									tempPath.arcSweep(centerX, centerY, sweepRadians, aspectRatio, curveQuality);
									tempPath.transform(affineReverse);
									path.append(tempPath);
								} else {
									path.arcSweep(centerX, centerY, sweepRadians, aspectRatio, curveQuality);
								}
							}
						}
						path.lineTo(v.x, v.y);
					}
					break;
				}
				
				case 'Z': path.close(); break;
				
				default: {
					errorString = "Invalid command in svg path data";
					return false;
				}
			}
		}
	}
	return true;
}

/* --- MaskMakerCanvas --- */

MaskMakerCanvas::MaskMakerCanvas(const IntRect& bounds) : mask8RLE(new RLERaster<Mask8>(bounds)) { }

void MaskMakerCanvas::parsePaint(Interpreter& impd, IVGExecutor& executor, Context& context, ArgumentsContainer& args
		, Paint& paint) const {
	parsePaintOfType<Mask8>(impd, executor, context, args, paint);
}

void MaskMakerCanvas::blendWithARGB32(const Renderer<ARGB32>& source) {
	(*mask8RLE) |= Converter<ARGB32, Mask8>(source);
}

void MaskMakerCanvas::blendWithMask8(const Renderer<Mask8>& source) { (*mask8RLE) |= source; }

void MaskMakerCanvas::defineBounds(const IntRect& newBounds) {
	(void)newBounds;
	Interpreter::throwRunTimeError("Bounds cannot be declared for mask");
}

IntRect MaskMakerCanvas::getBounds() const { return mask8RLE->calcBounds(); }

RLERaster<Mask8>* MaskMakerCanvas::finish(bool invert) {
	if (invert) (*mask8RLE) = ~(*mask8RLE);
	return mask8RLE.release();
}

/* --- Options --- */

void Options::setGamma(double newGamma) {
	assert(0.0 < gamma);
	if (gamma != newGamma) {
		gamma = newGamma;
		gammaTable = ((fabs(gamma - 1.0) < 0.0001) ? 0 : new GammaTable(gamma));
	}
}

/* --- CombinedMask --- */

class CombinedMask {
	public:		CombinedMask(const Renderer<Mask8>& region, const Renderer<Mask8>* mask, const GammaTable* gammaTable)
						: output(&region) {
					if (gammaTable != 0) {
						lookup.reset(new Lookup<Mask8, LookupTable<Mask8> > (*output, *gammaTable));
						output = lookup.get();
					}
					if (mask != 0) {
						multiplier.reset(new Multiplier<Mask8, Mask8>(*output, *mask));
						output = multiplier.get();
					}
				}
	public:		operator const Renderer<Mask8>&() const { return *output; }
	protected:	const Renderer<Mask8>* output;
	protected:	std::unique_ptr< Lookup<Mask8, LookupTable<Mask8> > > lookup;
	protected:	std::unique_ptr< Multiplier<Mask8, Mask8> > multiplier;
};

/* --- Colors --- */

/* Built with QuickHashGen */
static int findStandardColorName(size_t n /* string length */, const char* s /* zero-terminated string */) {
	static const char* STRINGS[17] = {
		"none", "aqua", "black", "blue", "fuchsia", "gray", "green", "lime", "maroon",
		"navy", "olive", "purple", "red", "silver", "teal", "white", "yellow"
	};
	static const int QUICK_HASH_TABLE[32] = {
		-1, 12, 8, 7, 3, -1, -1, 1, -1, -1, 5, 10, 9, -1, 2, -1,
		14, -1, 6, 13, -1, 15, -1, 11, -1, -1, 0, -1, -1, 16, 4, -1
	};
	if (n < 3 || n > 7) return -1;
	int stringIndex = QUICK_HASH_TABLE[((s[1] + s[3] ^ s[2])) & 31];
	return (stringIndex >= 0 && strcmp(s, STRINGS[stringIndex]) == 0) ? stringIndex : -1;
}

template<class PIXEL_TYPE> typename PIXEL_TYPE::Pixel parseColor(Interpreter& impd, const StringRange& r);

template<> Mask8::Pixel parseColor<Mask8>(Interpreter& impd, const StringRange& r) { return parseOpacity(impd, r); }

static bool parseNumericColor(Interpreter& impd, const StringRange& r, ARGB32::Pixel& argb) {
	StringIt p = r.b;
	
	if (r.e - p >= 5 && r.e[-1] == ')') {
		p += 4;
		String s(Interpreter::toLower(StringRange(r.b, p)));
		bool isRGB = (s == "rgb(");
		if (isRGB || s == "hsv(") {
			double n[4];
			int count = parseNumberList(impd, StringRange(p, r.e - 1), n, 3, 4);
			for (int i = 0; i < count; ++i) {
				if (n[i] < 0.0 || n[i] > 1.0) {
					impd.throwRunTimeError(String("hsv value number ") + impd.toString(i + 1)
							+ " out of range [0..1]: " + impd.toString(n[i]));
				}
			}
			assert(count == 3 || count == 4);
			if (isRGB) {
				unsigned char c[4];
				for (int i = 0; i < count; ++i) c[i] = min(static_cast<int>(n[i] * 256), 255);
				unsigned int i = 0xFF000000 | (c[0] << 16) | (c[1] << 8) | (c[2] << 0);
				argb = (count == 4 ? ARGB32::multiply(i, c[3]) : i);
			} else {
				argb = ARGB32::fromFloatHSV(n[0], n[1], n[2], (count == 4 ? n[3] : 1.0));
			}
			return true;
		}
	}
	return false;
}

template<> ARGB32::Pixel parseColor<ARGB32>(Interpreter& impd, const StringRange& r) {
	ARGB32::Pixel argb;
	if (r.e != r.b && *r.b == '#') {
		unsigned int i;
		StringIt p = Interpreter::parseHex(r.b + 1, r.e, i);
		switch (p - (r.b + 1)) {
			default: impd.throwBadSyntax(String("Invalid color: ") + String(r.b + 1, r.e));
			case 6: return 0xFF000000 | i;
			case 8: {
				if (!ARGB32::isValid(i)) {
					impd.throwBadSyntax(String("Invalid pre-multiplied alpha color: ") + String(r.b + 1, r.e));
				}
				return i;
			}
		}
	} else if (parseNumericColor(impd, r, argb)) {
		return argb;
	} else {
		static const ARGB32::Pixel STANDARD_COLORS[17] = {
			0, 0xFF00FFFF, 0xFF000000, 0xFF0000FF, 0xFFFF00FF
			, 0xFF808080, 0xFF008000, 0xFF00FF00, 0xFF800000
			, 0xFF000080, 0xFF808000, 0xFF800080, 0xFFFF0000
			, 0xFFC0C0C0, 0xFF008080, 0xFFFFFFFF, 0xFFFFFF00
		};
		int i = findStandardColorName(IMPD::lossless_cast<int>(r.e - r.b), impd.toLower(r).c_str());
		assert(i < 17);
		if (i < 0) {
			impd.throwBadSyntax(String("Invalid color name: ") + String(r.b, r.e));
		}
		return STANDARD_COLORS[i];
	}
}

ARGB32::Pixel parseColor(const String& color) {
	class DummyExecutor : public Executor {
		public:		virtual bool format(Interpreter&, const FormatInfo&) { return false; }
		public:		virtual bool execute(Interpreter&, const String&, const String&) { return true; }
		public:		virtual bool progress(Interpreter&, int) { return true; }
		public:		virtual bool load(Interpreter&, const WideString&, String&) { return false; }
		public:		virtual void trace(Interpreter&, const WideString&) { }
		public:		virtual bool meta(Interpreter&, const String&, const String&) { return false; }
	};
	DummyExecutor executor;
	STLMapVariables vars;
	FormatInfo formatInfo;	// Standalone helper runs in its own format scope.
	Interpreter impd(executor, vars, formatInfo);
	return parseColor<ARGB32>(impd, color);
}

/* --- Transforms --- */

enum TransformType {
	MATRIX_TRANSFORM,
	SCALE_TRANSFORM,
	ROTATE_TRANSFORM,
	OFFSET_TRANSFORM,
	SHEAR_TRANSFORM
};

/* Built with QuickHashGen */
static int findTransformType(size_t n /* string length */, const char* s /* zero-terminated string */) {
	static const char* STRINGS[5] = {
		"matrix", "scale", "rotate", "offset", "shear"
	};
	static const int QUICK_HASH_TABLE[8] = {
		4, 0, -1, 1, -1, -1, 3, 2
	};
	if (n < 5 || n > 6) return -1;
	int stringIndex = QUICK_HASH_TABLE[(s[1]) & 7];
	return (stringIndex >= 0 && strcmp(s, STRINGS[stringIndex]) == 0) ? stringIndex : -1;
}

static AffineTransformation parseSingleTransformation(Interpreter& impd, TransformType transformType, ArgumentsContainer& arguments) {
	double numbers[6];
	double anchor[2];
	const String* anchorArg = (transformType != OFFSET_TRANSFORM ? arguments.fetchOptional("anchor") : 0);
	AffineTransformation xf = AffineTransformation();
	if (anchorArg != 0) {
		parseNumberList(impd, *anchorArg, anchor, 2, 2);
		xf = xf.translate(-anchor[0], -anchor[1]);
	}
	const String& firstArg = arguments.fetchRequired(0);
	switch (transformType) {
		case MATRIX_TRANSFORM: { // matrix
			parseNumberList(impd, firstArg, numbers, 6, 6);
			xf = xf.transform(AffineTransformation(numbers[0], numbers[2], numbers[4], numbers[1], numbers[3], numbers[5]));
			break;
		}
		
		case SCALE_TRANSFORM: { // scale
			int count = parseNumberList(impd, firstArg, numbers, 1, 2);
			xf = (count == 1 ? xf.scale(numbers[0]) : xf.scale(numbers[0], numbers[1]));
			break;
		}

		case ROTATE_TRANSFORM: { // rotate
			parseNumberList(impd, firstArg, numbers, 1, 1);
			xf = xf.rotate(numbers[0] * DEGREES);
			break;
		}

		case OFFSET_TRANSFORM: { // offset
			parseNumberList(impd, firstArg, numbers, 2, 2);
			xf = xf.translate(numbers[0], numbers[1]);
			break;
		}

		case SHEAR_TRANSFORM: { // shear
			parseNumberList(impd, firstArg, numbers, 2, 2);
			xf = xf.shear(numbers[0], numbers[1]);
			break;
		}
	}
	if (anchorArg != 0) {
		xf = xf.translate(anchor[0], anchor[1]);
	}
	return xf;
}

class TransformationExecutor : public Executor {
	public:		TransformationExecutor(Executor& parentExecutor) : parentExecutor(parentExecutor) { };
	public:		virtual bool format(Interpreter& impd, const FormatInfo& formatInfo) {
					(void)impd;
					(void)formatInfo;
					return false;
				}
	public:		virtual bool execute(Interpreter& impd, const String& instruction, const String& arguments) {
					const int foundTransform = findTransformType(IMPD::lossless_cast<int>(instruction.size()), instruction.c_str());
					if (foundTransform < 0) {
						return false;
					}
					ArgumentsContainer args(ArgumentsContainer::parse(impd, arguments));
					AffineTransformation thisXF = parseSingleTransformation(impd, static_cast<TransformType>(foundTransform), args);
					args.throwIfAnyUnfetched();
					xf = thisXF.transform(xf);
					return true;
				}
	public:		virtual void trace(Interpreter& impd, const WideString& s) { parentExecutor.trace(impd, s); }
	public:		virtual bool progress(Interpreter& impd, int maxStatementsLeft) { return parentExecutor.progress(impd, maxStatementsLeft); }
	public:		virtual bool load(Interpreter& impd, const WideString& filename, String& contents) { return parentExecutor.load(impd, filename, contents); }
	public:		virtual bool meta(Interpreter& interpreter, const String& key, const String& arguments) {
					(void)interpreter;
					(void)key;
					(void)arguments;
					return false;
				}
	public:		const AffineTransformation& getTransform() const { return xf; }
	protected:	Executor& parentExecutor;
	protected:	AffineTransformation xf;
};

static AffineTransformation parseTransformationBlock(Interpreter& impd, const String& source) {
	TransformationExecutor xfExecutor(impd.getExecutor());
	Interpreter newInterpreter(xfExecutor, impd);	// Share the parent document's format declarations.
	newInterpreter.run(source);
	return xfExecutor.getTransform();
}

static double calcCurveQualityForTransform(const AffineTransformation& xf) {
	return min(max(sqrt(max(square(xf.matrix[0][0]) + square(xf.matrix[1][0])
			, square(xf.matrix[0][1]) + square(xf.matrix[1][1]))), MIN_CURVE_QUALITY), MAX_CURVE_QUALITY);
}

/* --- Gradients --- */

struct GradientSpec {
	// FIX : reverseRadialStops is always true?
	GradientSpec(const Interpreter& impd, const String& s, bool reverseRadialStops = false);
	struct StopSpec {
		double position;
		String color;
	};
	bool isRadial;
	double coords[4];
	vector<StopSpec> stops;
};

GradientSpec::GradientSpec(const Interpreter& impd, const String& source, bool reverseRadialStops) {
	ArgumentsContainer gradientArgs(ArgumentsContainer::parse(impd, source));

	isRadial = false;
	const String gradientType = gradientArgs.fetchRequired(0);
	const String gradientTypeLower = impd.toLower(gradientType);
	if (gradientTypeLower == "radial") {
		isRadial = true;
	} else if (gradientTypeLower != "linear") {
		impd.throwBadSyntax(String("Unrecognized gradient type: ") + gradientType);
	}
	reverseRadialStops = reverseRadialStops && isRadial;

	const int count = parseNumberList(impd, gradientArgs.fetchRequired(1), coords, (isRadial ? 3 : 4), 4);
	if (count == 3) {
		coords[3] = coords[2];
	}
	if (isRadial && (coords[2] < 0.0 || coords[3] < 0.0)) {
		impd.throwRunTimeError(String("Negative radial gradient radius: ")
				+ impd.toString(coords[coords[2] < 0.0 ? 2 : 3]));
	}

	const String* s = gradientArgs.fetchOptional("stops");
	if (s != 0) {
		StringVector stopsList;
		int stopsListCount = impd.parseList(*s, stopsList, true, false, 2);
		if ((stopsListCount & 1) != 0) {
			impd.throwBadSyntax(String("Invalid stops for gradient (odd number of elements): ") + *s);
		}

		double lastPosition = 0.0;
		stops.resize(stopsListCount / 2);		
		vector<StopSpec>::iterator outIt = stops.begin();
		for (StringVector::const_iterator it = stopsList.begin(), e = stopsList.end(); it != e; it += 2) {
			double position = impd.toDouble(*it);
			if (position < lastPosition || position > 1.0) {
				impd.throwBadSyntax(String("Invalid stops for gradient (invalid position: ") + impd.toString(position) + ")");
			}
			lastPosition = position;
			outIt->position = (reverseRadialStops ? 1.0 - position : position);
			outIt->color = *(it + 1);
			++outIt;
		}
		assert(outIt == stops.end());
		if (reverseRadialStops) {
			reverse(stops.begin(), stops.end());
		}
	} else {
		stops.resize(2);
		stops[0].position = 0.0;
		stops[1].position = 1.0;
		stops[(reverseRadialStops ? 1 : 0)].color = gradientArgs.fetchRequired("from");
		stops[(reverseRadialStops ? 0 : 1)].color = gradientArgs.fetchRequired("to");
	}

	gradientArgs.throwIfAnyUnfetched();
}

/* --- Patterns --- */

PatternBase::PatternBase(int scale) : scale(scale) { }

void PatternBase::makePattern(Interpreter& impd, IVGExecutor& executor, Context& parentContext, const String& source) {
	Context patternContext(*this, parentContext);
	patternContext.initState.transformation = AffineTransformation().scale(scale);
	patternContext.initState.mask = 0;
	patternContext.state.transformation = patternContext.initState.transformation;
	patternContext.state.mask = 0;
	executor.runInNewContext(impd, patternContext, source);
}

template<> void PatternPainter<Mask8>::blendWithARGB32(const Renderer<ARGB32>& source) {
	if (image.get() == 0) Interpreter::throwRunTimeError("Undeclared bounds");
	(*image) |= Converter<ARGB32, Mask8>(source);
}

template<> void PatternPainter<ARGB32>::blendWithMask8(const Renderer<Mask8>& source) {
	(void)source;
	assert(0);
}

/* --- Masks --- */

FadedMask::FadedMask(const Renderer<Mask8>& mask, Mask8::Pixel opacity)
		: solid(opacity), multiplier(mask, solid), output(opacity != 255 ? multiplier : mask) { }
FadedMask::operator const Renderer<Mask8>&() const { return output; }

/* --- Canvas --- */

template<> void Canvas::blend<ARGB32>(const Renderer<ARGB32>& source) { blendWithARGB32(source); }
template<> void Canvas::blend<Mask8>(const Renderer<Mask8>& source) { blendWithMask8(source); }

template<class PIXEL_TYPE> void Canvas::parsePaintOfType(Interpreter& impd, IVGExecutor& executor, Context& context, ArgumentsContainer& args, Paint& paint) const {
	const String* s;

	// Important to parse pattern first so that we don't apply other attributes (e.g. "relative") before we draw the texture.
	if ((s = args.fetchOptional("pattern", false)) != 0) {
		std::unique_ptr< PatternPainter<PIXEL_TYPE> > patternPainter(new PatternPainter<PIXEL_TYPE>(context.calcPatternScale()));
		patternPainter->makePattern(impd, executor, context, *s);
		paint.painter = patternPainter.release();
	} else if ((s = args.fetchOptional("gradient")) != 0) {
		GradientSpec spec(impd, *s, true);
		vector< typename Gradient<PIXEL_TYPE>::Stop > stops(spec.stops.size());
		typename vector< typename Gradient<PIXEL_TYPE>::Stop >::iterator outIt = stops.begin();
		vector< GradientSpec::StopSpec >::const_iterator e = spec.stops.end();
		for (vector< GradientSpec::StopSpec >::const_iterator it = spec.stops.begin(); it != e; ++it) {
			outIt->position = it->position;
			outIt->color = parseColor<PIXEL_TYPE>(impd, it->color);
			++outIt;
		}
		assert(outIt == stops.end());
		
		if (!spec.isRadial) {
			paint.painter = new LinearGradientPainter<PIXEL_TYPE>(
					spec.coords[0], spec.coords[1], spec.coords[2], spec.coords[3]
					, IMPD::lossless_cast<int>(stops.size()), (stops.empty() ? 0 : &stops[0]));
		} else {	// NuXPixels radial ascend goes to max in the center
			paint.painter = new RadialGradientPainter<PIXEL_TYPE>(
					spec.coords[0], spec.coords[1], spec.coords[2], spec.coords[3]
					, IMPD::lossless_cast<int>(stops.size()), (stops.empty() ? 0 : &stops[0]));
		}
	} else if ((s = args.fetchOptional(0)) != 0) {
		paint.painter = new ColorPainter<PIXEL_TYPE>(parseColor<PIXEL_TYPE>(impd, *s));
	}
	if ((s = args.fetchOptional("opacity")) != 0) paint.opacity = parseOpacity(impd, *s);
	if ((s = args.fetchOptional("relative")) != 0) paint.relative = impd.toBool(*s);
	if ((s = args.fetchOptional("transform", false)) != 0) paint.transformation = parseTransformationBlock(impd, *s);
}

/* --- Context --- */

Context::Context(Canvas& canvas, const AffineTransformation& initialTransform) : canvas(canvas) {
	initState.transformation = initialTransform;
	state.transformation = initState.transformation;
	initState.textStyle.fill.painter = new ColorPainter<ARGB32>(0xFF000000);
	state.textStyle.fill.painter = initState.textStyle.fill.painter;
}

Context::Context(Canvas& canvas, Context& parentContext)
		: canvas(canvas), initState(parentContext.state), state(parentContext.state) { }

void Context::stroke(const Path& path, Stroke& stroke, const Rect<double>& paintSourceBounds, double widthMultiplier) {
	if (stroke.paint.isVisible() && stroke.width > EPSILON) {
		Path strokePath(path);
		if (stroke.gap > EPSILON) {
			double l = stroke.dash + stroke.gap;
			double dashOffset = fmod(fmod(stroke.dashOffset, l) + l, l);  // floor modulo trick
			strokePath.dash(stroke.dash, stroke.gap, dashOffset);
		}
		strokePath.stroke(stroke.width * widthMultiplier, stroke.caps, stroke.joints, stroke.miterLimit
				, calcCurveQuality());
		strokePath.transform(state.transformation);
		PolygonMask polygonMask(strokePath, canvas.getBounds());
		if (!polygonMask.isValid()) {
			Interpreter::throwRunTimeError("Vertices outside valid coordinate range");
		}
		stroke.paint.doPaint(*this, paintSourceBounds, CombinedMask(polygonMask, state.mask, state.options.gammaTable));
	}
}

void Context::fill(const Path& path, Paint& fill, bool evenOddFillRule, const Rect<double>& paintSourceBounds) {
	if (fill.isVisible()) {
		const FillRule* fillRule = evenOddFillRule
				? static_cast<const FillRule*>(&PolygonMask::evenOddFillRule)
				: static_cast<const FillRule*>(&PolygonMask::nonZeroFillRule);
		Path fillPath(path);
		fillPath.closeAll();
		fillPath.transform(state.transformation);
		PolygonMask polygonMask(fillPath, canvas.getBounds(), *fillRule);
		if (!polygonMask.isValid()) {
			Interpreter::throwRunTimeError("Vertices outside valid coordinate range");
		}
		fill.doPaint(*this, paintSourceBounds, CombinedMask(polygonMask, state.mask, state.options.gammaTable));
	}
}

void Context::draw(const Path& path) {
	const Rect<double> pathBounds(path.calcFloatBounds());
	fill(path, state.fill, state.evenOddFillRule, pathBounds);
	stroke(path, state.pen, pathBounds, 1.0);
}

void Context::resetState() {
	state = initState;
}

double Context::calcCurveQuality() const {
	return calcCurveQualityForTransform(state.transformation) * state.options.curveQuality;
}

int Context::calcPatternScale() const {
	const AffineTransformation& xf = state.transformation;
	const double scale = sqrt(max(square(xf.matrix[0][0]) + square(xf.matrix[1][0])
			, square(xf.matrix[0][1]) + square(xf.matrix[1][1])));
	return static_cast<int>(max(ceil(scale * state.options.patternResolution - 0.0001), 1.0));
}

/* Built with QuickHashGen */
static int findIVGInstruction(size_t n /* string length */, const char* s /* zero-terminated string */) {
	static const char* STRINGS[21] = {
		"rect", "pen", "fill", "path", "matrix", "scale", "rotate", "offset", "shear", 
		"context", "wipe", "options", "reset", "ellipse", "star", "mask", "bounds", 
		"define", "font", "text", "image"
	};
	static const int HASH_TABLE[64] = {
		-1, 16, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
		-1, -1, -1, 8, -1, -1, -1, -1, -1, -1, 5, -1, -1, -1, 10, 20, 
		14, -1, -1, 6, -1, 0, 12, -1, -1, -1, 7, 17, 3, -1, 11, -1, 
		13, 15, 2, -1, -1, -1, -1, -1, 19, 4, -1, -1, 18, -1, 1, 9
	};
	assert(s[n] == '\0');
	if (n < 3 || n > 7) return -1;
	int stringIndex = HASH_TABLE[(s[3] - s[0] + s[2]) & 63];
	return (stringIndex >= 0 && strcmp(s, STRINGS[stringIndex]) == 0) ? stringIndex : -1;
}

enum IVGInstruction {
	RECT_INSTRUCTION, PEN_INSTRUCTION, FILL_INSTRUCTION, PATH_INSTRUCTION, MATRIX_INSTRUCTION, SCALE_INSTRUCTION
	, ROTATE_INSTRUCTION, OFFSET_INSTRUCTION, SHEAR_INSTRUCTION, CONTEXT_INSTRUCTION, WIPE_INSTRUCTION
	, OPTIONS_INSTRUCTION, RESET_INSTRUCTION, ELLIPSE_INSTRUCTION, STAR_INSTRUCTION, MASK_INSTRUCTION
	, BOUNDS_INSTRUCTION, DEFINE_INSTRUCTION, FONT_INSTRUCTION, TEXT_INSTRUCTION, IMAGE_INSTRUCTION
};

/* --- IVGExecutor --- */

IVGExecutor::IVGExecutor(Canvas& canvas, const NuXPixels::AffineTransformation& initialTransform)
		: rootContext(canvas, initialTransform), currentContext(&rootContext) { }

void IVGExecutor::parseStroke(Interpreter& impd, ArgumentsContainer& args, Stroke& stroke) {
	const String* s;
	if ((s = args.fetchOptional("width")) != 0) {
		double d = impd.toDouble(*s);
		if (d < 0.0) impd.throwRunTimeError(String("Negative stroke width: ") + impd.toString(d));
		stroke.width = d;
	}
	if ((s = args.fetchOptional("caps")) != 0) {
		String capsString = impd.toLower(*s);
		if (capsString == "butt") stroke.caps = Path::BUTT;
		else if (capsString == "round") stroke.caps = Path::ROUND;
		else if (capsString == "square") stroke.caps = Path::SQUARE;
		else impd.throwBadSyntax(String("Unrecognized stroke caps: ") + *s);
	}
	if ((s = args.fetchOptional("joints")) != 0) {
		String jointsString = impd.toLower(*s);
		if (jointsString == "bevel") stroke.joints = Path::BEVEL;
		else if (jointsString == "curve") stroke.joints = Path::CURVE;
		else if (jointsString == "miter") stroke.joints = Path::MITER;
		else impd.throwBadSyntax(String("Unrecognized stroke joints: ") + *s);
	}
	if ((s = args.fetchOptional("miter-limit")) != 0) {
		double d = impd.toDouble(*s);
		if (d < 1.0) impd.throwRunTimeError(String("miter-limit out of range [1..inf): ") + impd.toString(d));
		stroke.miterLimit = d;
	}
	if ((s = args.fetchOptional("dash")) != 0) {
		if (impd.toLower(*s) == "none") {
			stroke.dash = 0.0;
			stroke.gap = 0.0;
		} else {
			double numbers[2];
			int count = parseNumberList(impd, *s, numbers, 1, 2);
			double dash = numbers[0];
			if (dash < 0.0) {
				impd.throwRunTimeError(String("Negative dash value: ") + impd.toString(dash));
			}
			double gap = (count == 1 ? numbers[0] : numbers[1]);
			if (gap < 0.0) {
				impd.throwRunTimeError(String("Negative gap value: ") + impd.toString(gap));
			}
			stroke.dash = dash;
			stroke.gap = gap;
		}
	}
	if ((s = args.fetchOptional("dash-offset")) != 0) stroke.dashOffset = impd.toDouble(*s);
	currentContext->accessCanvas().parsePaint(impd, *this, *currentContext, args, stroke.paint);
	args.throwIfNoneFetched();
	args.throwIfAnyUnfetched();
}

void IVGExecutor::runInNewContext(Interpreter& interpreter, Context& context, const String& source) {
	Context* const lastContext = currentContext;
	try {
		currentContext = &context;
		interpreter.run(source);
		currentContext = lastContext;
	}
	catch (...) {
		currentContext = lastContext;
		throw;
	}
}

bool IVGExecutor::format(Interpreter& impd, const FormatInfo& formatInfo) {
	(void)impd;
	return (formatInfo.formatId == "ivg-1" || formatInfo.formatId == "ivg-2") && formatInfo.requires.empty();
}

bool IVGExecutor::meta(Interpreter& impd, const String& key, const String& arguments) {
	(void)impd;
	(void)key;
	(void)arguments;
	return false;
}

void IVGExecutor::trace(Interpreter& impd, const WideString& s) {
	(void)impd;
	wcout << s << endl;
}

bool IVGExecutor::progress(Interpreter& impd, int maxStatementsLeft) {
	(void)impd; (void)maxStatementsLeft;
	return true;
}

bool IVGExecutor::load(Interpreter& impd, const WideString& filename, String& contents) {
	(void)impd; (void)filename; (void)contents;
	return false;
}

Image IVGExecutor::loadImage(Interpreter& impd, const WideString& imageSource, const IntRect* sourceRectangle
		, bool forStretching, double forXSize, bool xSizeIsRelative, double forYSize, bool ySizeIsRelative) {
	(void)impd; (void)imageSource; (void)sourceRectangle; (void)forStretching;
	(void)forXSize; (void)xSizeIsRelative;
	(void)forYSize; (void)ySizeIsRelative;
	return Image();
}

std::vector<const Font*> IVGExecutor::lookupFonts(Interpreter& impd, const WideString& fontName, const UniString& forString) {
	(void)impd; (void)fontName; (void)forString;
	return std::vector<const Font*>();
}

std::vector<const Font*> IVGExecutor::lookupExternalOrInternalFonts(Interpreter& impd, const WideString& name
		, const UniString& forString) {
	if (lastFontName != name) {
		lastFontName = name;
		const FontMap::const_iterator it = embeddedFonts.find(name);
		lastFontPointers = (it != embeddedFonts.end()
				? std::vector<const Font*>(1, &it->second) : lookupFonts(impd, name, forString));
	}
	return lastFontPointers;
}

void IVGExecutor::executeDefine(Interpreter& impd, ArgumentsContainer& args) {
	const String& type = args.fetchRequired(0, true);
	const String& typeLower = impd.toLower(type);
	if (typeLower == "font") {
		const WideString name = impd.unescapeToWide(args.fetchRequired(1, true));
		const String& definition = args.fetchRequired(2, false);
		args.throwIfAnyUnfetched();

		if (embeddedFonts.find(name) != embeddedFonts.end()) {
			Interpreter::throwRunTimeError(String("Duplicate font definition: ") + String(name.begin(), name.end()));
		}
		IVG::FontParser fontParser(this);
		FormatInfo fontFormatInfo;	// Fresh format scope for embedded font documents.
		Interpreter newInterpreter(fontParser, impd, fontFormatInfo);
		newInterpreter.run(definition);
		embeddedFonts[name] = fontParser.finalizeFont();
		lastFontName = WideString();	// must reset cache cause there is no guarantee that STL preserves pointers after insertion
		lastFontPointers.clear();
	} else if (typeLower == "image") {
		const WideString name = impd.unescapeToWide(args.fetchRequired(1, true));
		const String& definition = args.fetchRequired(2, false);
		const String* s = args.fetchOptional("resolution");
		const double resolution = (s != 0 ? impd.toDouble(*s) : 1.0);
		if (resolution < 0.0001) {
			impd.throwRunTimeError(String("resolution out of range [0.0001..inf): ") + impd.toString(resolution));
		}
		args.throwIfAnyUnfetched();

		if (definedImages.find(name) != definedImages.end()) {
			Interpreter::throwRunTimeError(String("Duplicate image definition: ") + String(name.begin(), name.end()));
		}

		SelfContainedARGB32Canvas offscreenCanvas(resolution);
		Context imageContext(offscreenCanvas, AffineTransformation().scale(resolution));
		runInNewContext(impd, imageContext, definition);
		Image& image = definedImages[name];
		image.xResolution = resolution;
		image.yResolution = resolution;
		image.raster = offscreenCanvas.relinquishRaster();
	} else {
		Interpreter::throwBadSyntax(String("Invalid define instruction type: ") + type);
	}
}

/* Built with QuickHashGen */
static int findAlignmentKeyword(size_t n /* string length */, const char* s /* string (zero terminated) */) {
	static const char* STRINGS[6] = {
		"left", "center", "right", "top", "middle", "bottom"
	};
	static const int HASH_TABLE[32] = {
		-1, -1, -1, -1, 4, -1, 0, 2, -1, -1, -1, -1, -1, -1, 1, -1, 
		3, -1, -1, -1, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
	};
	assert(s[n] == '\0');
	if (n < 3 || n > 6) return -1;
	int stringIndex = HASH_TABLE[(s[2]) & 31];
	return (stringIndex >= 0 && strcmp(s, STRINGS[stringIndex]) == 0) ? stringIndex : -1;
}

static IntRect expandToIntRect(const Rect<double>& floatRect) {
	IntRect r;
	r.left = static_cast<int>(floor(floatRect.left));
	r.top = static_cast<int>(floor(floatRect.top));
	r.width = static_cast<int>(ceil(floatRect.calcRight())) - r.left;
	r.height = static_cast<int>(ceil(floatRect.calcBottom())) - r.top;
	return r;
}

void IVGExecutor::executeImage(Interpreter& impd, ArgumentsContainer& args) {
	double numbers[4];
	parseNumberList(impd, args.fetchRequired(0), numbers, 2, 2);
	if (fabs(numbers[0]) > COORDINATE_LIMIT || fabs(numbers[1]) > COORDINATE_LIMIT) {
		Interpreter::throwRunTimeError("Image coordinates out of range");
	}
	const Vertex atPosition = Vertex(numbers[0], numbers[1]);
	const WideString imageName = impd.unescapeToWide(args.fetchRequired(1));
	const String* s;
	Mask8::Pixel opacity = 255;
	AffineTransformation imageXF;
	enum HorizontalAlignment {
		NO_HORIZONTAL_ALIGN, LEFT_ALIGN, CENTER_ALIGN, RIGHT_ALIGN
	} horizontalAlignment = NO_HORIZONTAL_ALIGN;
	enum VerticalAlignment {
		NO_VERTICAL_ALIGN, TOP_ALIGN, MIDDLE_ALIGN, BOTTOM_ALIGN
	} verticalAlignment = NO_VERTICAL_ALIGN;
	bool doFitWidth = false;
	double fitWidth;
	bool doFitHeight = false;
	double fitHeight;
	bool doStretch = true;
	bool gotSourceRectangle = false;
	IntRect sourceRectangle;
	if ((s = args.fetchOptional("align")) != 0) {
		bool gotHorizontalAlignment = false;
		bool gotVerticalAlignment = false;
		StringVector elems;
		int count = impd.parseList(*s, elems, true, false, 0, 2);
		for (int i = 0; i < count; ++i) {
			const String alignment = elems[i];
			const String alignmentLower = impd.toLower(alignment);
			int foundAlignment = findAlignmentKeyword(alignmentLower.size(), alignmentLower.c_str());
			if (foundAlignment < 0) {
				impd.throwBadSyntax(String("Unrecognized alignment: ") + alignment);
			} else if (foundAlignment < 3) {
				if (gotHorizontalAlignment) {
					impd.throwBadSyntax(String("Duplicate horizontal alignment: " + *s));
				}
				gotHorizontalAlignment = true;
				horizontalAlignment = static_cast<HorizontalAlignment>(foundAlignment + 1);
			} else {
				assert(foundAlignment < 6);
				if (gotVerticalAlignment) {
					impd.throwBadSyntax(String("Duplicate vertical alignment: " + *s));
				}
				gotVerticalAlignment = true;
				verticalAlignment = static_cast<VerticalAlignment>(foundAlignment - 3 + 1);
			}
		}
	}
	if ((s = args.fetchOptional("width")) != 0) {
		doFitWidth = true;
		fitWidth = impd.toDouble(*s);
		if (fitWidth < 0.0 || fitWidth > COORDINATE_LIMIT) {
			impd.throwRunTimeError(String("Invalid image width: ") + impd.toString(fitWidth));
		}
	}
	if ((s = args.fetchOptional("height")) != 0) {
		doFitHeight = true;
		fitHeight = impd.toDouble(*s);
		if (fitHeight < 0.0 || fitHeight > COORDINATE_LIMIT) {
			impd.throwRunTimeError(String("Invalid image height: ") + impd.toString(fitHeight));
		}
	}
	if (doFitWidth || doFitHeight) {
		const String* s = args.fetchOptional("stretch");
		if (s != 0) {
			doStretch = impd.toBool(*s);
		}
	}
	if ((s = args.fetchOptional("opacity")) != 0) {
		opacity = parseOpacity(impd, *s);
	}
	if ((s = args.fetchOptional("transform")) != 0) {
		imageXF = parseTransformationBlock(impd, *s);
	}
	if ((s = args.fetchOptional("clip")) != 0) {
		parseNumberList(impd, *s, numbers, 4, 4);
		if (numbers[2] < 0.0) {
			impd.throwRunTimeError(String("Negative clip width: ") + impd.toString(numbers[2]));
		}
		if (numbers[3] < 0.0) {
			impd.throwRunTimeError(String("Negative clip height: ") + impd.toString(numbers[2]));
		}
		sourceRectangle = IntRect(static_cast<int>(floor(numbers[0]))
				, static_cast<int>(floor(numbers[1]))
				, static_cast<int>(ceil(numbers[2]))
				, static_cast<int>(ceil(numbers[3])));
		gotSourceRectangle = true;
	}
	args.throwIfAnyUnfetched();

	State& state = currentContext->accessState();

	const ImageMap::const_iterator it = definedImages.find(imageName);
	Image image;
	if (it != definedImages.end()) {
		image = it->second;
	} else {
		const AffineTransformation xf = imageXF.transform(state.transformation);
		const double xfXScale = sqrt(square(xf.matrix[0][0]) + square(xf.matrix[1][0]));
		const double xfYScale = sqrt(square(xf.matrix[0][1]) + square(xf.matrix[1][1]));
		const double forXSize = (doFitWidth ? fitWidth : xfXScale);
		const double forYSize = (doFitHeight ? fitHeight : xfYScale);
		image = loadImage(impd, imageName, gotSourceRectangle ? &sourceRectangle : 0
				, doStretch, forXSize, !doFitWidth, forYSize, !doFitHeight);
		if (image.raster == 0) {
			Interpreter::throwRunTimeError(String("Missing image: ") + String(imageName.begin(), imageName.end()));
		}
	}
	assert(image.xResolution > 0);
	assert(image.yResolution > 0);

	assert(image.raster != 0);
	const IntRect rasterBounds = image.raster->calcBounds();
	const Rect<double> subRasterBounds = (gotSourceRectangle ? Rect<double>(sourceRectangle.left * image.xResolution
			, sourceRectangle.top * image.yResolution, sourceRectangle.width * image.xResolution
			, sourceRectangle.height * image.yResolution) : Rect<double>(rasterBounds.left, rasterBounds.top
			, rasterBounds.width, rasterBounds.height));
	if (subRasterBounds.width <= 0 || subRasterBounds.height <= 0) {
		return;
	}
	Canvas& canvas = currentContext->accessCanvas();
	double alignX = 0.0;
	double alignY = 0.0;
	switch (horizontalAlignment) {
		case NO_HORIZONTAL_ALIGN: break;
		case LEFT_ALIGN: alignX = -subRasterBounds.left; break;
		case CENTER_ALIGN: alignX = -subRasterBounds.left - subRasterBounds.width * 0.5; break;
		case RIGHT_ALIGN: alignX = -subRasterBounds.left - subRasterBounds.width; break;
	}
	switch (verticalAlignment) {
		case NO_VERTICAL_ALIGN: break;
		case TOP_ALIGN: alignY = -subRasterBounds.top; break;
		case MIDDLE_ALIGN: alignY = -subRasterBounds.top - subRasterBounds.height * 0.5; break;
		case BOTTOM_ALIGN: alignY = -subRasterBounds.top - subRasterBounds.height; break;
	}
	double scaleX;
	double scaleY;
	if (doStretch) {
		scaleX = (doFitWidth ? fitWidth / subRasterBounds.width : 1.0 / image.xResolution);
		scaleY = (doFitHeight ? fitHeight / subRasterBounds.height : 1.0 / image.yResolution);
	} else {
		if (doFitWidth && doFitHeight) {
			scaleX = min(fitWidth / subRasterBounds.width, fitHeight / subRasterBounds.height);
			scaleY = scaleX;
		} else if (doFitWidth) {
			scaleX = fitWidth / subRasterBounds.width;
			scaleY = scaleX;
		} else {
			assert(doFitHeight);
			scaleY = fitHeight / subRasterBounds.height;
			scaleX = scaleY;
		}
	}
	
	const AffineTransformation textureTransform = AffineTransformation().translate(alignX, alignY)
			.scale(scaleX, scaleY).transform(imageXF).translate(atPosition.x, atPosition.y)
			.transform(state.transformation);
	const double totalXScale = sqrt(square(textureTransform.matrix[0][0]) + square(textureTransform.matrix[1][0]));
	const double totalYScale = sqrt(square(textureTransform.matrix[0][1]) + square(textureTransform.matrix[1][1]));
	if (!isfinite(totalXScale) || !isfinite(totalYScale)
			|| totalXScale * subRasterBounds.width > COORDINATE_LIMIT
			|| totalYScale * subRasterBounds.height > COORDINATE_LIMIT) {
		impd.throwRunTimeError("Image scale out of range");
	}
	
	// FIX : sub in nuxpixels for making a sub-raster?
	const Raster<ARGB32>* raster = image.raster;
	Raster<ARGB32> subRaster(raster->getPixelPointer(), raster->getStride()
			, expandToIntRect(subRasterBounds).calcIntersection(rasterBounds), raster->isOpaque());
	if (gotSourceRectangle) {
		raster = &subRaster;
	}
	
	Texture<ARGB32> texture(*raster, false, textureTransform);
	Renderer<ARGB32>* renderer = &texture;
	Solid<Mask8> opacitySolid(opacity);
	Multiplier<ARGB32, Mask8> opacityMultiplier(*renderer, opacitySolid);
	if (opacity != 255) {
		renderer = &opacityMultiplier;
	}
	// dummy argument if no mask
	Multiplier<ARGB32, Mask8> maskMultiplier(*renderer
			, (state.mask != 0 ? static_cast< const Renderer<Mask8>& >(*state.mask) : opacitySolid));
	if (state.mask != 0) {
		renderer = &maskMultiplier;
	}
	canvas.blend(*renderer);
}

bool IVGExecutor::execute(Interpreter& impd, const String& instruction, const String& arguments) {
	int foundInstruction = findIVGInstruction(instruction.size(), instruction.c_str());
	if (foundInstruction < 0) {
		return false;
	}
	
	ArgumentsContainer args(ArgumentsContainer::parse(impd, arguments));
	double numbers[6];

	IVGInstruction ivgInstruction = static_cast<IVGInstruction>(foundInstruction);
	switch (ivgInstruction) {
		case RECT_INSTRUCTION: { // RECT
			parseNumberList(impd, args.fetchRequired(0), numbers, 4, 4);
			const String* s = args.fetchOptional("rounded");
			args.throwIfAnyUnfetched();
			if (numbers[2] < 0.0) {
				impd.throwRunTimeError(String("Negative rectangle width: ") + impd.toString(numbers[2]));
			}
			if (numbers[3] < 0.0) {
				impd.throwRunTimeError(String("Negative rectangle height: ") + impd.toString(numbers[2]));
			}

			Path p;
			if (s == 0) {
				p.addRect(numbers[0], numbers[1], numbers[2], numbers[3]);
			} else {
				double rounded[2];
				const int count = parseNumberList(impd, *s, rounded, 1, 2);
				if (count == 1) {
					rounded[1] = rounded[0];
				}
				if (rounded[0] < 0.0 || rounded[1] < 0.0) {
					impd.throwRunTimeError(String("Negative rounded corner radius: ") + impd.toString(numbers[numbers[0] < 0.0 ? 0 : 1]));
				}
				p.addRoundedRect(numbers[0], numbers[1], numbers[2], numbers[3], min(rounded[0], numbers[2] * 0.5)
						, min(rounded[1], numbers[3] * 0.5), currentContext->calcCurveQuality());
			}
			currentContext->draw(p);
			break;
		}

		case PEN_INSTRUCTION: { // pen
			parseStroke(impd, args, currentContext->accessState().pen);
			break;
		}

		case FILL_INSTRUCTION: { // fill
			State& state = currentContext->accessState();
			Canvas& canvas = currentContext->accessCanvas();
			canvas.parsePaint(impd, *this, *currentContext, args, state.fill);
			const String* s = args.fetchOptional("rule");
			if (s != 0) {
				String ruleString = impd.toLower(*s);
				if (ruleString == "non-zero") state.evenOddFillRule = false;
				else if (ruleString == "even-odd") state.evenOddFillRule = true;
				else impd.throwBadSyntax(String("Unrecognized fill rule: ") + *s);
			}
			args.throwIfNoneFetched();
			args.throwIfAnyUnfetched();
			break;
		}

		case PATH_INSTRUCTION: { // PATH
			const String* s = args.fetchOptional("svg");
			if (s != 0) {
				Path p;
				const char* errorString;
				if (!buildPathFromSVG(*s, currentContext->calcCurveQuality(), p, errorString)) {
					impd.throwBadSyntax(errorString);
				}
				args.throwIfAnyUnfetched();
				currentContext->draw(p);
			} else {
				args.throwIfAnyUnfetched();
				impd.throwBadSyntax("Invalid PATH arguments (missing svg argument)");
			}
			break;
		}
			
		case MATRIX_INSTRUCTION:
		case SCALE_INSTRUCTION:
		case ROTATE_INSTRUCTION:
		case OFFSET_INSTRUCTION:
		case SHEAR_INSTRUCTION: { // transforms
			State& state = currentContext->accessState();
			AffineTransformation thisXF = parseSingleTransformation(impd, static_cast<TransformType>(ivgInstruction - MATRIX_INSTRUCTION), args);
			args.throwIfAnyUnfetched();
			// FIX : should reverse concat order as standard in new AffineTransform class?
			state.transformation = thisXF.transform(state.transformation);
			break;
		}

		case CONTEXT_INSTRUCTION: { // context
			const String& block = args.fetchRequired(0, false);
			args.throwIfAnyUnfetched();
			Context newContext(currentContext->accessCanvas(), *currentContext);
			runInNewContext(impd, newContext, block);
			break;
		}
		
		case WIPE_INSTRUCTION: { // wipe
			Paint wipePaint;
			currentContext->accessCanvas().parsePaint(impd, *this, *currentContext, args, wipePaint);
			args.throwIfNoneFetched();
			args.throwIfAnyUnfetched();
			if (wipePaint.relative) {
				impd.throwRunTimeError("Relative paint is not allowed with wipe");
			}
			if (wipePaint.isVisible()) {
				State& state = currentContext->accessState();
				if (state.mask != 0) wipePaint.doPaint(*currentContext, Rect<double>(), *state.mask);
				else wipePaint.doPaint(*currentContext, Rect<double>(), Solid<Mask8>(0xFF));
			}
			break;
		}
		
		case OPTIONS_INSTRUCTION: { // options
			const String* s;
			State& state = currentContext->accessState();
			if ((s = args.fetchOptional("aa-gamma")) != 0) {
				double d = impd.toDouble(*s);
				if (d < 0.0001 || d > 99.9999) {
					impd.throwRunTimeError(String("aa-gamma out of range (0..100): ") + impd.toString(d));
				}
				state.options.setGamma(d);
			}
			if ((s = args.fetchOptional("curve-quality")) != 0) {
				double d = impd.toDouble(*s);
				if (d < 0.0001 || d > 99.9999) {
					impd.throwRunTimeError(String("curve-quality out of range (0..100): ") + impd.toString(d));
				}
				state.options.curveQuality = d;
			}
			if ((s = args.fetchOptional("pattern-resolution")) != 0) {
				double d = impd.toDouble(*s);
				if (d < 0.0001 || d > 99.9999) {
					impd.throwRunTimeError(String("pattern-resolution out of range (0..100): ") + impd.toString(d));
				}
				state.options.patternResolution = d;
			}
			args.throwIfNoneFetched();
			args.throwIfAnyUnfetched();
			break;
		}
		
		case RESET_INSTRUCTION: { // reset
			args.throwIfAnyUnfetched();
			currentContext->resetState();
			break;
		}
		
		case ELLIPSE_INSTRUCTION: { // ELLIPSE
			int count = parseNumberList(impd, args.fetchRequired(0), numbers, 3, 4);
			args.throwIfAnyUnfetched();			
			Path p;
			const double rx = numbers[2];
			const double ry = (count == 4 ? numbers[3] : rx);
			if (rx < 0.0 || ry < 0.0) {
				impd.throwRunTimeError(String("Negative ellipse radius: ") + impd.toString(rx < 0.0 ? rx : ry));
			}
			if (rx == ry) {
				p.addCircle(numbers[0], numbers[1], rx, currentContext->calcCurveQuality());
			} else {
				p.addEllipse(numbers[0], numbers[1], rx, ry, currentContext->calcCurveQuality());
			}
			currentContext->draw(p);
			break;
		}

		case STAR_INSTRUCTION: { // STAR
			StringVector elems;
			int count = impd.parseList(args.fetchRequired(0), elems, true, false, 4, 5);
			const String* s = args.fetchOptional("rotation");
			args.throwIfAnyUnfetched();			

			const double rotation = (s != 0 ? impd.toDouble(*s) * DEGREES : 0.0);
			const double cx = impd.toDouble(elems[0]);
			const double cy = impd.toDouble(elems[1]);
			const int points = impd.toInt(elems[2]);
			if (points <= 0 || points > 10000) {
				impd.throwRunTimeError(String("star points out of range [1..10000]: ") + impd.toString(points));
			}
			const double r1 = impd.toDouble(elems[3]);
			const double r2 = (count == 5 ? impd.toDouble(elems[4]) : r1);
			if (r1 < 0.0 || r2 < 0.0) {
				impd.throwRunTimeError(String("Negative star radius: ") + impd.toString(r1 < 0.0 ? r1 : r2));
			}

			Path p;
			p.addStar(cx, cy, points, r1, r2, rotation);
			currentContext->draw(p);
			break;
		}

		case MASK_INSTRUCTION: { // mask
			const String& block = args.fetchRequired(0, false);
			bool inverted = false;
			const String* s = args.fetchOptional("inverted");
			if (s != 0) inverted = impd.toBool(*s);

			args.throwIfAnyUnfetched();
			MaskMakerCanvas maskMaker(currentContext->accessCanvas().getBounds());
			Context maskContext(maskMaker, *currentContext);
			State& maskState = maskContext.accessState();
			maskState.pen = Stroke();
			maskState.fill = Paint();
			maskState.fill.painter = new ColorPainter<Mask8>(0xFF);
			maskState.textStyle.fill = Paint();
			maskState.textStyle.fill.painter = new ColorPainter<Mask8>(0xFF);
			maskState.textStyle.outline = Stroke();
			maskState.evenOddFillRule = false;
			runInNewContext(impd, maskContext, block);
			currentContext->accessState().mask = maskMaker.finish(inverted);
			break;
		}
		
		case BOUNDS_INSTRUCTION: { // bounds
			StringVector elems;
			impd.parseList(args.fetchRequired(0), elems, true, false, 4, 4);
			args.throwIfAnyUnfetched();
			currentContext->accessCanvas().defineBounds(IntRect(impd.toInt(elems[0]), impd.toInt(elems[1])
					, impd.toInt(elems[2]), impd.toInt(elems[3])));
			break;
		}
		
		case DEFINE_INSTRUCTION: executeDefine(impd, args); break;
		
		case FONT_INSTRUCTION: {
			const String* s;
			State& state = currentContext->accessState();
			if ((s = args.fetchOptional(0, true)) != 0) {
				const WideString newFontName = impd.unescapeToWide(*s);
				if (newFontName.empty()) {
					Interpreter::throwRunTimeError("Invalid font name");
				}
				if (lookupExternalOrInternalFonts(impd, newFontName, UniString()).empty()) {
					Interpreter::throwRunTimeError(String("Missing font: ")
							+ String(newFontName.begin(), newFontName.end()));
				}
				state.textStyle.fontName = newFontName;
			}
			if ((s = args.fetchOptional("color", true)) != 0) {
				ArgumentsContainer colorArgs(ArgumentsContainer::parse(impd, *s));
				currentContext->accessCanvas().parsePaint(impd, *this, *currentContext, colorArgs, state.textStyle.fill);
			}
			if ((s = args.fetchOptional("outline", true)) != 0) {
				ArgumentsContainer outlineArgs(ArgumentsContainer::parse(impd, *s));
				parseStroke(impd, outlineArgs, state.textStyle.outline);
			}

			if ((s = args.fetchOptional("transform", false)) != 0) {
				state.textStyle.glyphTransform = parseTransformationBlock(impd, *s);
			}
			if ((s = args.fetchOptional("size", true)) != 0) {
				double d = impd.toDouble(*s);
				if (d <= 0.0) {
					impd.throwRunTimeError(String("font size out of range (0..inf): ") + impd.toString(d));
				}
				state.textStyle.size = d;
			}
			if ((s = args.fetchOptional("tracking", true)) != 0) {
				state.textStyle.letterSpacing = impd.toDouble(*s);
			}
			args.throwIfNoneFetched();
			args.throwIfAnyUnfetched();
			break;
		}

		case TEXT_INSTRUCTION: {
			const String* s;
			enum { LEFT_ANCHOR, CENTER_ANCHOR, RIGHT_ANCHOR } anchor = LEFT_ANCHOR;
			State& state = currentContext->accessState();
			if ((s = args.fetchOptional("at", true)) != 0) {
				parseNumberList(impd, *s, numbers, 2, 2);
				state.textCaret = Vertex(numbers[0], numbers[1]);
			}
			if ((s = args.fetchOptional("anchor", true)) != 0) {
				const String anchorString = impd.toLower(*s);
				if (anchorString == "left") {
					anchor = LEFT_ANCHOR;
				} else if (anchorString == "center") {
					anchor = CENTER_ANCHOR;
				} else if (anchorString == "right") {
					anchor = RIGHT_ANCHOR;
				} else {
					impd.throwBadSyntax(String("Unrecognized anchor: ") + *s);
				}
			}
			const UniString text = impd.unescapeToUni(args.fetchRequired(0, true));
			const String* caretVariable = args.fetchOptional("caret", true);
			args.throwIfAnyUnfetched();
			
			if (state.textStyle.fontName.empty()) {
				Interpreter::throwRunTimeError("Need to set font before writing");
			}
			std::vector<const Font*> fonts = lookupExternalOrInternalFonts(impd, state.textStyle.fontName, text);
			if (fonts.empty()) {
				Interpreter::throwRunTimeError(String("Missing font: ")
						+ String(state.textStyle.fontName.begin(), state.textStyle.fontName.end()));
			}
			
			double advance;
			const char* errorString;
			Path textPath;
			bool success = buildPathForString(text, fonts, state.textStyle.size, state.textStyle.glyphTransform
					, state.textStyle.letterSpacing, currentContext->calcCurveQuality(), textPath, advance
					, errorString);
			if (!success) {
				trace(impd, WideString(errorString, errorString + strlen(errorString)));
			}
			switch (anchor) {
				case LEFT_ANCHOR: break;
				case CENTER_ANCHOR: state.textCaret.x -= advance * 0.5; break;
				case RIGHT_ANCHOR: state.textCaret.x -= advance; break;
			}
			textPath.transform(AffineTransformation().translate(state.textCaret.x, state.textCaret.y));
			const Rect<double> pathBounds(textPath.calcFloatBounds());
			currentContext->stroke(textPath, state.textStyle.outline, pathBounds, 2.0);
			currentContext->fill(textPath, state.textStyle.fill, false, pathBounds);
			switch (anchor) {
				case LEFT_ANCHOR: // Fall through
				case CENTER_ANCHOR: state.textCaret.x += advance; break;
				case RIGHT_ANCHOR: break;
			}
			if (caretVariable != 0) {
				impd.set(*caretVariable, impd.toString(state.textCaret.x));
			}
			break;
		}

		case IMAGE_INSTRUCTION: executeImage(impd, args); break;
	}
	
	return true;
}

IVGExecutor::~IVGExecutor() {
	for (ImageMap::iterator it = definedImages.begin(); it != definedImages.end(); ++it) {
		delete it->second.raster;
		it->second.raster = 0;
	}
}

/* --- ARGB32Canvas --- */

ARGB32Canvas::ARGB32Canvas(Raster<ARGB32>& output) : argb32Raster(output) { }

void ARGB32Canvas::parsePaint(Interpreter& impd, IVGExecutor& executor, Context& context, ArgumentsContainer& args, Paint& paint) const {
	parsePaintOfType<ARGB32>(impd, executor, context, args, paint);
}

void ARGB32Canvas::blendWithARGB32(const Renderer<ARGB32>& source) { argb32Raster |= source; }
void ARGB32Canvas::defineBounds(const IntRect& newBounds) { (void)newBounds; }
IntRect ARGB32Canvas::getBounds() const { return argb32Raster.calcBounds(); }

/* --- SelfContainedARGB32Canvas --- */

SelfContainedARGB32Canvas::SelfContainedARGB32Canvas(double rescaleBounds) : rescaleBounds(rescaleBounds) { }

void SelfContainedARGB32Canvas::parsePaint(Interpreter& impd, IVGExecutor& executor, Context& context, ArgumentsContainer& args, Paint& paint) const {
	parsePaintOfType<ARGB32>(impd, executor, context, args, paint);
}

void SelfContainedARGB32Canvas::checkBoundsDeclared() const {
	if (raster.get() == 0) Interpreter::throwRunTimeError("Undeclared bounds");
}

void SelfContainedARGB32Canvas::defineBounds(const IntRect& newBounds) {
	IntRect scaledBounds = newBounds;
	if (rescaleBounds != 1.0) {
		scaledBounds = expandToIntRect(Rect<double>(newBounds.left * rescaleBounds
				, newBounds.top * rescaleBounds, newBounds.width * rescaleBounds, newBounds.height * rescaleBounds));
	}
	if (raster.get() != 0) Interpreter::throwRunTimeError("Multiple bounds declarations");
	checkBounds(scaledBounds);
	raster.reset(new SelfContainedRaster<ARGB32>(scaledBounds));
	(*raster) = Solid<ARGB32>(ARGB32::transparent());
}

void SelfContainedARGB32Canvas::blendWithARGB32(const Renderer<ARGB32>& source) { checkBoundsDeclared(); (*raster) |= source; }
IntRect SelfContainedARGB32Canvas::getBounds() const { checkBoundsDeclared(); return raster->calcBounds(); }
SelfContainedRaster<ARGB32>* SelfContainedARGB32Canvas::accessRaster() { checkBoundsDeclared(); return raster.get(); }
SelfContainedRaster<ARGB32>* SelfContainedARGB32Canvas::relinquishRaster() { checkBoundsDeclared(); return raster.release(); }

/* --- Font --- */

Font::Metrics::Metrics() : upm(0.0), ascent(0.0), descent(0.0), linegap(0.0) { }

Font::Font() { }

Font::Font(const Metrics& metrics, const std::vector<Font::Glyph>& glyphs
		, const std::vector<KerningPair>& kerningPairs) : metrics(metrics), glyphs(glyphs), kerningPairs(kerningPairs) {
	if (!glyphs.empty()) {
		for (std::vector<Font::Glyph>::const_iterator it = glyphs.begin() + 1; it != glyphs.end(); ++it) {
			assert(it->character > (it - 1)->character);
		}
	}
	if (!kerningPairs.empty()) {
		for (std::vector<Font::KerningPair>::const_iterator it = kerningPairs.begin() + 1; it != kerningPairs.end(); ++it) {
			assert(it->characters > (it - 1)->characters);
		}
	}
}

static bool compareGlyphWithCharacter(const Font::Glyph& glyph, UniChar character) {
	return glyph.character < character;
}

const Font::Glyph* Font::findGlyph(UniChar forCharacter) const {
	const std::vector<Glyph>::const_iterator it = std::lower_bound(glyphs.begin(), glyphs.end(), forCharacter
			, compareGlyphWithCharacter);
	return (it != glyphs.end() && it->character == forCharacter ? &(*it) : 0);
}

static bool compareKerningPairWithCharPair(const Font::KerningPair& kerningPair, const std::pair<UniChar, UniChar>& charPair) {
	return kerningPair.characters < charPair;
}

const double Font::findKerningAdjust(UniChar characterA, UniChar characterB) const {
	const std::pair<UniChar, UniChar> charPair(characterA, characterB);
	const std::vector<KerningPair>::const_iterator it = std::lower_bound(kerningPairs.begin(), kerningPairs.end()
			, charPair, compareKerningPairWithCharPair);
	return (it != kerningPairs.end() && it->characters == charPair ? it->adjust : 0.0);
}

const Font::Metrics& Font::getMetrics() const {
	return metrics;
}

struct BuildPathFontInfo {
	double mpu;
	AffineTransformation scaledXF;
	double effectiveQuality;
};

bool buildPathForString(const UniString& string, const std::vector<const Font*>& fonts, double size
		, const AffineTransformation& glyphTransform, double letterSpacing, double curveQuality, Path& path
		, double& advance, const char*& errorString, UniChar lastCharacter) {
	assert(!fonts.empty());
	std::vector<BuildPathFontInfo> fontInfos;
	fontInfos.reserve(fonts.size());
	for (std::vector<const Font*>::const_iterator fontIt = fonts.begin(); fontIt != fonts.end(); ++fontIt) {
		BuildPathFontInfo fontInfo;
		assert(*fontIt != 0);
		fontInfo.mpu = 1.0 / (*fontIt)->getMetrics().upm;
		fontInfo.scaledXF = AffineTransformation().scale(fontInfo.mpu).transform(glyphTransform).scale(size);
		fontInfo.effectiveQuality = calcCurveQualityForTransform(fontInfo.scaledXF) * curveQuality;
		fontInfos.push_back(fontInfo);
	}
	bool success = true;
	advance = 0.0;
	const Font* lastFont = 0;
	for (UniString::const_iterator it = string.begin(); it != string.end(); ++it) {
		UniChar thisCharacter = *it;
		Path glyphPath;
		const IVG::Font::Glyph* glyph = 0;
		std::vector<BuildPathFontInfo>::const_iterator fontInfoIt = fontInfos.begin();
		std::vector<const Font*>::const_iterator fontIt = fonts.begin();
		while (fontIt != fonts.end() && (glyph = (*fontIt)->findGlyph(thisCharacter)) == 0) {
			++fontIt;
			++fontInfoIt;
		}
		if (glyph == 0) {
			fontIt = fonts.begin();
			fontInfoIt = fontInfos.begin();
			thisCharacter = 0;
			glyph = fonts[0]->findGlyph(thisCharacter);
		}
		const char* thisError = 0;
		if (glyph != 0 && IVG::buildPathFromSVG(glyph->svgPath, fontInfoIt->effectiveQuality, glyphPath, thisError)) {
			advance += (*fontIt == lastFont
					? (*fontIt)->findKerningAdjust(lastCharacter, thisCharacter) * fontInfoIt->mpu * size : 0.0);
			lastFont = *fontIt;
			lastCharacter = thisCharacter;
			glyphPath.transform(fontInfoIt->scaledXF.translate(advance, 0.0));
			advance += (glyph->advance * fontInfoIt->mpu + letterSpacing) * size;
			path.append(glyphPath);
		} else {
			if (success) {
				errorString = (glyph == 0 ? "Missing glyph" : (thisError == 0 ? "Unknown error" : thisError));
			}
			success = false;
		}
	}
	return success;
}

/* --- FontParser --- */

FontParser::FontParser(Executor* parentExecutor) : parentExecutor(parentExecutor) { }

bool FontParser::format(Interpreter& impd, const FormatInfo& formatInfo) {
	(void)impd;
	return (formatInfo.formatId == "ivgfont-1" && formatInfo.requires.empty());
}

bool FontParser::meta(Interpreter& impd, const String& key, const String& arguments) {
	(void)impd;
	(void)key;
	(void)arguments;
	return false;
}

/* Built with QuickHashGen */
static int findIVGFontInstruction(size_t n /* string length */, const char* s /* string (zero terminated) */) {
	static const char* STRINGS[3] = {
		"metrics", "glyph", "kern"
	};
	static const int HASH_TABLE[4] = {
		0, 1, 2, -1
	};
	assert(s[n] == '\0');
	if (n < 4 || n > 7) return -1;
	int stringIndex = HASH_TABLE[(s[2]) & 3];
	return (stringIndex >= 0 && strcmp(s, STRINGS[stringIndex]) == 0) ? stringIndex : -1;
}

enum IVGFontInstruction {
	FONT_METRICS_INSTRUCTION, FONT_GLYPH_INSTRUCTION, FONT_KERN_INSTRUCTION
};

bool FontParser::execute(Interpreter& impd, const String& instruction, const String& arguments) {
	const int foundInstruction = findIVGFontInstruction(instruction.size(), instruction.c_str());
	if (foundInstruction < 0) {
		return false;
	}
	
	ArgumentsContainer args(ArgumentsContainer::parse(impd, arguments));

	const IVGFontInstruction fontInstruction = static_cast<IVGFontInstruction>(foundInstruction);
	switch (fontInstruction) {
		case FONT_METRICS_INSTRUCTION: {
			if (metrics.upm != 0.0) {
				impd.throwBadSyntax("Duplicate metrics instruction in font definition");
			}

			metrics.upm = impd.toDouble(args.fetchRequired("upm"));
			metrics.ascent = impd.toDouble(args.fetchRequired("ascent"));
			metrics.descent = impd.toDouble(args.fetchRequired("descent"));
			const String* linegapString = args.fetchOptional("linegap");
			if (linegapString != 0) {
				metrics.linegap = impd.toDouble(*linegapString);
			}
			args.throwIfAnyUnfetched();

			if (metrics.upm <= 0.0 || metrics.ascent < 0 || metrics.descent > 0) {
				impd.throwBadSyntax("Invalid metrics instruction in font definition");
			}
			return true;
		}
		
		case FONT_GLYPH_INSTRUCTION: {
			Font::Glyph glyph;
			const UniString ws = impd.unescapeToUni(args.fetchRequired(0));
			if (ws.size() != 1) {
				impd.throwBadSyntax(String("Invalid glyph character (length is not 1): ") + String(ws.begin(), ws.end()));
			}
			glyph.character = static_cast<UniChar>(ws[0]);
			glyph.advance = impd.toDouble(args.fetchRequired(1));
			glyph.svgPath = args.fetchRequired(2);
			args.throwIfAnyUnfetched();

			if (metrics.upm == 0.0) {
				impd.throwBadSyntax("Missing metrics before glyph instruction in font definition");
			}
			if (glyph.advance < 0.0) {
				impd.throwBadSyntax(String("Negative glyph advance in font definition: ")
						+ impd.toString(glyph.advance));
			}
			if (!glyphs.insert(std::make_pair(glyph.character, glyph)).second) {
				impd.throwBadSyntax(String("Duplicate glyph definition in font definition (unicode: ")
						+ impd.toString(static_cast<int>(glyph.character)) + ")");
			}
			return true;
		}
		
		case FONT_KERN_INSTRUCTION: {
			const double adjust = impd.toDouble(args.fetchRequired(0));
			int i = 1;
			const String* s;
			while ((s = args.fetchOptional(i)) != 0) {
				const UniString a = impd.unescapeToUni(*s);
				const UniString b = impd.unescapeToUni(args.fetchRequired(i + 1));
				for (UniString::const_iterator itA = a.begin(); itA != a.end(); ++itA) {
					for (UniString::const_iterator itB = b.begin(); itB != b.end(); ++itB) {
						if (!kerningPairs.insert(std::make_pair(std::make_pair(*itA, *itB), adjust)).second) {
							impd.throwBadSyntax(String("Duplicate kerning pair in font definition: ")
									+ impd.toString(static_cast<int>(*itA)) + "," + impd.toString(static_cast<int>(*itB)));
						}
					}
				}
				i += 2;
			}
			args.throwIfAnyUnfetched();
			return true;
		}
	}
	
	return false;
}

Font FontParser::finalizeFont() const {
	std::vector<Font::Glyph> glyphsVector;
	glyphsVector.reserve(glyphs.size());
	for (GlyphsMap::const_iterator it = glyphs.begin(); it != glyphs.end(); ++it) {
		glyphsVector.push_back(it->second);
	}

	std::vector<Font::KerningPair> kerningPairsVector;
	kerningPairsVector.reserve(kerningPairs.size());
	for (KerningPairsMap::const_iterator it = kerningPairs.begin(); it != kerningPairs.end(); ++it) {
		Font::KerningPair kerningPair;
		kerningPair.characters = it->first;
		kerningPair.adjust = it->second;
		kerningPairsVector.push_back(kerningPair);
	}

	return Font(metrics, glyphsVector, kerningPairsVector);
}

void FontParser::trace(Interpreter& interpreter, const WideString& s) {
	if (parentExecutor != 0) {
		parentExecutor->trace(interpreter, s);
	}
}

bool FontParser::progress(Interpreter& interpreter, int maxStatementsLeft) {
	return (parentExecutor != 0 ? parentExecutor->progress(interpreter, maxStatementsLeft) : true);
}

bool FontParser::load(Interpreter& interpreter, const WideString& filename, String& contents) {
	return (parentExecutor != 0 ? parentExecutor->load(interpreter, filename, contents) : false);
}

} // namespace IVG
