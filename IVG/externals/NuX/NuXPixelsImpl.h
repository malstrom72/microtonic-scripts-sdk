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
	\file NuXPixelsImpl.h
*/

#ifndef NuXPixelsImpl_h
#define NuXPixelsImpl_h

#include <math.h>
#include "NuXPixels.h"

namespace NuXPixels {

/* --- Point --- */

template<typename T> Point<T>::Point() : x(0), y(0) { }
template<typename T> Point<T>::Point(T x, T y) : x(x), y(y) { }
template<typename T> bool Point<T>::operator==(const Point<T>& other) const { return (x == other.x && y == other.y); }
template<typename T> bool Point<T>::operator!=(const Point<T>& other) const { return (x != other.x || y != other.y); }
template<typename T> void Point<T>::swap(Point<T>& other) { std::swap(x, other.x); std::swap(y, other.y); }

/* --- Rect --- */

template<typename T> Rect<T>::Rect() : left(0), top(0), width(0), height(0) { }
template<typename T> Rect<T>::Rect(T left, T top, T width, T height) : left(left), top(top), width(width), height(height) { }
template<typename T> T Rect<T>::calcRight() const { return left + width; }
template<typename T> T Rect<T>::calcBottom() const { return top + height; }
template<typename T> bool Rect<T>::isEmpty() const { return (width == 0 && height == 0); }
template<typename T> bool Rect<T>::operator!=(const Rect<T>& other) const { return !(*this == other); }
template<typename T> Rect<T> Rect<T>::offset(T x, T y) const { return Rect<T>(left + x, top + y, width, height); }

template<typename T> Rect<T> Rect<T>::calcUnion(const Rect<T>& other) const
{
	if (isEmpty()) {
		return other;
	} else if (other.isEmpty()) {
		return (*this);
	} else {
		const T unionLeft = minValue(left, other.left);
		const T unionTop = minValue(top, other.top);
		return Rect<T>(unionLeft, unionTop, maxValue(calcRight(), other.calcRight()) - unionLeft, maxValue(calcBottom(), other.calcBottom()) - unionTop); 
	}
}

template<typename T> Rect<T> Rect<T>::calcIntersection(const Rect<T>& other) const
{
	const T intersectionLeft = maxValue(left, other.left);
	const T intersectionTop = maxValue(top, other.top);
	const T intersectionWidth = minValue(calcRight(), other.calcRight()) - intersectionLeft;
	const T intersectionHeight = minValue(calcBottom(), other.calcBottom()) - intersectionTop;
	if (intersectionWidth <= 0 || intersectionHeight <= 0) {
		return Rect<T>();
	} else {
		return Rect<T>(intersectionLeft, intersectionTop, intersectionWidth, intersectionHeight);
	}
}

template<typename T> bool Rect<T>::operator==(const Rect<T>& other) const
{
	return (left == other.left && top == other.top && width == other.width && height == other.height);
}

template<typename T> void Rect<T>::swap(Rect<T>& other)
{
	std::swap(left, other.left);
	std::swap(top, other.top);
	std::swap(width, other.width);
	std::swap(height, other.height);
}

/* --- Low-level support --- */

template<class T> void standardFillPixels(int count, typename T::Pixel* target, typename T::Pixel color) {
	int j = 0;
	while (j + 4 <= count) {
		target[j + 0] = color;
		target[j + 1] = color;
		target[j + 2] = color;
		target[j + 3] = color;
		j += 4;
	}
	while (j < count) {
		target[j] = color;
		++j;
	}
}

template<class T> void fillPixels(int count, typename T::Pixel* target, typename T::Pixel color) {
	standardFillPixels<T>(count, target, color);
}

template<class T> void standardCopyPixels(int count, typename T::Pixel* target, const typename T::Pixel* source) {
	if (target != source) {
		int j = 0;
		while (j + 4 <= count) {
			target[j + 0] = source[j + 0];
			target[j + 1] = source[j + 1];
			target[j + 2] = source[j + 2];
			target[j + 3] = source[j + 3];
			j += 4;
		}
		while (j < count) {
			target[j] = source[j];
			++j;
		}
	}
}

template<class T> void copyPixels(int count, typename T::Pixel* target, const typename T::Pixel* source) {
	standardCopyPixels<T>(count, target, source);
}

template<class T> void standardBlendSolidToPixels(int count, typename T::Pixel* target, typename T::Pixel foreground
		, const typename T::Pixel* background) {
	int i = 0;
	while (i + 4 <= count) {
		const typename T::Pixel a0 = background[i + 0];
		const typename T::Pixel a1 = background[i + 1];
		const typename T::Pixel a2 = background[i + 2];
		const typename T::Pixel a3 = background[i + 3];
		target[i + 0] = T::blend(a0, foreground);
		target[i + 1] = T::blend(a1, foreground);
		target[i + 2] = T::blend(a2, foreground);
		target[i + 3] = T::blend(a3, foreground);
		i += 4;
	}
	while (i < count) {
		target[i] = T::blend(background[i], foreground);
		++i;
	}
}

template<class T> void blendSolidToPixels(int count, typename T::Pixel* target, typename T::Pixel foreground
		, const typename T::Pixel* background) {
	standardBlendSolidToPixels<T>(count, target, foreground, background);
}

template<class T> void standardBlendPixelsToPixels(int count, typename T::Pixel* target
		, const typename T::Pixel* foreground, const typename T::Pixel* background) {
	int i = 0;
	while (i + 4 <= count) {
		const typename T::Pixel a0 = background[i + 0];
		const typename T::Pixel a1 = background[i + 1];
		const typename T::Pixel a2 = background[i + 2];
		const typename T::Pixel a3 = background[i + 3];
		const typename T::Pixel b0 = foreground[i + 0];
		const typename T::Pixel b1 = foreground[i + 1];
		const typename T::Pixel b2 = foreground[i + 2];
		const typename T::Pixel b3 = foreground[i + 3];
		target[i + 0] = T::blend(a0, b0);
		target[i + 1] = T::blend(a1, b1);
		target[i + 2] = T::blend(a2, b2);
		target[i + 3] = T::blend(a3, b3);
		i += 4;
	}
	while (i < count) {
		target[i] = T::blend(background[i], foreground[i]);
		++i;
	}
}

template<class T> void blendPixelsToPixels(int count, typename T::Pixel* target, const typename T::Pixel* foreground
		, const typename T::Pixel* background) {
	standardBlendPixelsToPixels<T>(count, target, foreground, background);
}

template<class T> inline void standardInterpolatePixels(int count, typename T::Pixel*& target
		, const typename T::Pixel*& source, int imageStride, Fixed32_32& sx, Fixed32_32& sy, const Fixed32_32 dxx
		, const Fixed32_32 dxy, int hop) {
	for (int i = 0; i < count; ++i) {
		*target = T::interpolate(source[0], source[1], source[imageStride], source[imageStride + 1]
				, low32(sx) >> 24, low32(sy) >> 24);
		source += hop + addCarry(sx, dxx) + (-addCarry(sy, dxy) & imageStride);
		++target;
	}
}

template<class T> void interpolatePixels(int count, typename T::Pixel* target, const typename T::Pixel* source
		, int imageStride, Fixed32_32& sx, Fixed32_32& sy, const Fixed32_32 dxx, const Fixed32_32 dxy, int hop) {
	standardInterpolatePixels<T>(count, target, source, imageStride, sx, sy, dxx, dxy, hop);
}

template<class T> inline void standardInterpolatePixelsXOnly(int count, typename T::Pixel*& target
		, const typename T::Pixel*& source, Fixed32_32& sx, const Fixed32_32 dxx, int hop) {
	for (int i = 0; i < count; ++i) {
		*target = T::interpolate(source[0], source[1], low32(sx) >> 24);
		source += hop + addCarry(sx, dxx);
		++target;
	}
}

template<class T> void interpolatePixelsXOnly(int count, typename T::Pixel* target, const typename T::Pixel* source
		, Fixed32_32& sx, const Fixed32_32 dxx, int hop) {
	standardInterpolatePixelsXOnly<T>(count, target, source, sx, dxx, hop);
}

template<class T> inline void standardInterpolatePixelsYOnly(int count, typename T::Pixel*& target
		, const typename T::Pixel*& source, int imageStride, Fixed32_32& sy, const Fixed32_32 dxy, int hop) {
	for (int i = 0; i < count; ++i) {
		*target = T::interpolate(source[0], source[imageStride], low32(sy) >> 24);
		source += hop + (-addCarry(sy, dxy) & imageStride);
		++target;
	}
}

template<class T> void interpolatePixelsYOnly(int count, typename T::Pixel* target, const typename T::Pixel* source
		, int imageStride, Fixed32_32& sy, const Fixed32_32 dxy, int hop) {
	standardInterpolatePixelsYOnly<T>(count, target, source, imageStride, sy, dxy, hop);
}

#if (NUXPIXELS_SIMD)
// Optimized specializations
template<> void fillPixels<ARGB32>(int count, ARGB32::Pixel* target, ARGB32::Pixel color);
template<> void copyPixels<ARGB32>(int count, ARGB32::Pixel* target, const ARGB32::Pixel* source);
template<> void blendSolidToPixels<ARGB32>(int count, ARGB32::Pixel* target, ARGB32::Pixel foreground
		, const ARGB32::Pixel* background);
template<> void blendPixelsToPixels<ARGB32>(int count, ARGB32::Pixel* target, const ARGB32::Pixel* foreground
		, const ARGB32::Pixel* background);
template<> void interpolatePixels<ARGB32>(int count, typename ARGB32::Pixel* target
		, const typename ARGB32::Pixel* source, int imageStride, Fixed32_32& sx, Fixed32_32& sy, const Fixed32_32 dxx
		, const Fixed32_32 dxy, int hop);
template<> void interpolatePixelsXOnly<ARGB32>(int count, typename ARGB32::Pixel* target
		, const typename ARGB32::Pixel* source, Fixed32_32& sx, const Fixed32_32 dxx, int hop);
template<> void interpolatePixelsYOnly<ARGB32>(int count, typename ARGB32::Pixel* target
		, const typename ARGB32::Pixel* source, int imageStride, Fixed32_32& sy, const Fixed32_32 dxy, int hop);
#endif


/* --- Span -- */

template<class T> Span<T>::Span(int length, bool solid, bool opaque, const typename T::Pixel* pixels)
	: lengthAndFlags(length | (solid << 31) | (opaque << 30))
	, pixels(pixels)
{
	assert(length >= 0);
}

template<class T> Span<T>::Span() { }
template<class T> bool Span<T>::isSolid() const { return ((lengthAndFlags & 0x80000000) != 0); }
template<class T> bool Span<T>::isOpaque() const { return ((lengthAndFlags & 0x40000000) != 0); }
template<class T> bool Span<T>::isTransparent() const { return (isSolid() && T::isTransparent(pixels[0])); }
template<class T> bool Span<T>::isMaximum() const { return (isSolid() && T::isMaximum(pixels[0])); }
template<class T> int Span<T>::getLength() const { return (lengthAndFlags & 0x3FFFFFFF); }
template<class T> typename T::Pixel Span<T>::getSolidPixel() const { assert(isSolid()); return pixels[0]; }
template<class T> const typename T::Pixel* Span<T>::getVariablePixels() const { assert(!isSolid()); return pixels; }
template<class T> const typename T::Pixel* Span<T>::getPixelPointer() const { return pixels; }

/* --- SpanBuffer --- */

/**
        Iterator walks spans in a SpanBuffer sequentially. Incrementing advances
        by the length of the current span, while decrementing relies on the copy
        stored at the end of the previous span to step backwards.
**/
template<class T> class SpanBuffer<T>::iterator {
	public:		iterator(Span<T>* p) : p(p) { }
	public:		Span<T>& operator*() { return *p; }
	public:		Span<T>* operator->() { return p; }
	public:		iterator& operator++() { p += p->getLength(); return (*this); }
	public:		iterator& operator--() { p -= (p - 1)->getLength(); return (*this); }
	public:		iterator operator++(int) { iterator o(*this); ++(*this); return o; }
	public:		iterator operator--(int) { iterator o(*this); --(*this); return o; }
	public:		iterator operator+(int i) { iterator it(*this); if (i < 0) { while (++i <= 0) { --it; } } else { while (--i >= 0) { ++it; } }; return it; }
	public:		iterator operator-(int i) { return (*this) + -i; }
	public:		bool operator==(iterator o) { return (p == o.p); } 
	public:		bool operator!=(iterator o) { return (p != o.p); } 
	protected:	Span<T>* p;
};

#if (NUXPIXELS_ICC_HACK)
	template<class T> SpanBuffer<T>::SpanBuffer(char* spans, typename T::Pixel* pixels)
		: spans(reinterpret_cast<Span<T>*>(spans))
		, pixels(pixels)
		, endSpan(reinterpret_cast<Span<T>*>(spans))
		, endPixel(pixels)
	{
	}
#elif (!NUXPIXELS_ICC_HACK)
	template<class T> SpanBuffer<T>::SpanBuffer(Span<T>* spans, typename T::Pixel* pixels)
		: spans(spans)
		, pixels(pixels)
		, endSpan(spans)
		, endPixel(pixels)
	{
	}
#endif

template<class T> typename SpanBuffer<T>::iterator SpanBuffer<T>::begin() const
{
	return spans;
}

template<class T> typename SpanBuffer<T>::iterator SpanBuffer<T>::end() const
{
	return endSpan;
}

template<class T> void SpanBuffer<T>::add(int length, const Span<T>& span)
{
	assert(length > 0 && length == span.getLength());
	*endSpan = span;
	*(endSpan + length - 1) = span; // For backwards iteration.
	endSpan += length;
	endPixel += length;
}

template<class T> void SpanBuffer<T>::addSpan(const Span<T>& span) { add(span.getLength(), span); }
template<class T> void SpanBuffer<T>::addTransparent(int length) { addSolid(length, 0); }

template<class T> typename T::Pixel* SpanBuffer<T>::preallocatePixels() const { return endPixel; }

template<class T> void SpanBuffer<T>::addSolid(int length, const typename T::Pixel& pixel)
{
	assert(T::isValid(pixel));
	*endPixel = pixel;
	add(length, Span<T>(length, true, T::isOpaque(pixel), endPixel));
}

template<class T> typename T::Pixel* SpanBuffer<T>::addVariable(int length, bool opaque)
{
	typename T::Pixel* p = endPixel;
	add(length, Span<T>(length, false, opaque, p));
	return p;
}

template<class T> void SpanBuffer<T>::addReference(int length, const typename T::Pixel* pixels, bool opaque)
{
	#if !defined(NDEBUG)
		for (int i = 0; i < length; ++i) {
			assert(T::isValid(pixels[i]));
			assert(!opaque || T::isOpaque(pixels[i]));
		}
	#endif
	add(length, Span<T>(length, false, opaque, pixels));
}

template<class T> void SpanBuffer<T>::split(iterator it, int splitPoint) // This method is a low-level method used for "merging" two SpanBuffers. It only splits for "forward iteration", but if you would iterate backwards the array would still be invariant since the split doesn't modify any pixel contents.
{
	assert(0 < splitPoint && splitPoint < it->getLength());
	const int remaining = it->getLength() - splitPoint;
	const bool solid = it->isSolid();
	const bool opaque = it->isOpaque();
	const typename T::Pixel* const pixelPointer = it->getPixelPointer();
	const typename T::Pixel* newPixelPointer = pixelPointer + splitPoint;
	if (solid) { // Solid pixel must be copied if it resides in this SpanBuffer. Otherwise, it may be overwritten if we overwrite this buffer forwards when merging.
		if (pixels <= pixelPointer && pixelPointer < endPixel) {
			*const_cast<typename T::Pixel*>(newPixelPointer) = *pixelPointer;
		} else {
			newPixelPointer = pixelPointer;
		}
	}
	*it = Span<T>(splitPoint, solid, opaque, pixelPointer);
	*(it + 1) = Span<T>(remaining, solid, opaque, newPixelPointer);
}

/* --- merge --- */

template<class A, class B> void merge(SpanBuffer<A>& spansA, SpanBuffer<B>& spansB, typename SpanBuffer<A>::iterator itA, typename SpanBuffer<B>::iterator itB)
{
	const int lengthA = itA->getLength();
	const int lengthB = itB->getLength();
	if (lengthA < lengthB) {
		spansB.split(itB, lengthA);
	} else if (lengthB < lengthA) {
		spansA.split(itA, lengthB);
	}
}

/* --- Renderer --- */

template<class T> Renderer<T>::~Renderer() { }

/* --- Solid -- */

template<class T> Solid<T>::Solid(const typename T::Pixel& pixel) : pixel(pixel) { assert(T::isValid(pixel)); };
template<class T> IntRect Solid<T>::calcBounds() const { return FULL_RECT; }
template<class T> void Solid<T>::render(int /*x*/, int /*y*/, int length, SpanBuffer<T>& output) const {
	assert(0 < length && length <= MAX_RENDER_LENGTH);
	output.addSolid(length, pixel);
}

/* --- SolidRect --- */

template<class T> SolidRect<T>::SolidRect(const typename T::Pixel& pixel, const IntRect& rect)
	: pixel(pixel)
	, rect(rect)
{
	assert(T::isValid(pixel));
}

template<class T> IntRect SolidRect<T>::calcBounds() const
{
	return rect;
}

template<class T> void SolidRect<T>::render(int x, int y, int length, SpanBuffer<T>& output) const
{
// FIX : sub, or should we have a clipped-renderer class?
	assert(0 < length && length <= MAX_RENDER_LENGTH);
	
	if (y >= rect.top && y < rect.calcBottom()) {
		if (x < rect.left) {
			const int c = minValue(rect.left - x, length);
			output.addTransparent(c);
			x += c;
			length -= c;
		}
		assert(length >= 0);
		if (length > 0 && x < rect.calcRight()) {
			const int c = minValue(rect.calcRight() - x, length);
			output.addSolid(c, pixel);
			x += c;
			length -= c;
		}
	}
	if (length > 0) {
		output.addTransparent(length);
	}
}

/* --- Clipper --- */

template<class T> Clipper<T>::Clipper(const Renderer<T>& source, const IntRect& rect)
	: source(source)
	, rect(rect)
{
}

template<class T> IntRect Clipper<T>::calcBounds() const
{
	return rect.calcIntersection(source.calcBounds());
}

template<class T> void Clipper<T>::render(int x, int y, int length, SpanBuffer<T>& output) const
{
// FIX : sub, or should we have a clipped-renderer class?
	assert(0 < length && length <= MAX_RENDER_LENGTH);
	
	if (y >= rect.top && y < rect.calcBottom()) {
		if (x < rect.left) {
			const int c = minValue(rect.left - x, length);
			output.addTransparent(c);
			x += c;
			length -= c;
		}
		assert(length >= 0);
		if (length > 0 && x < rect.calcRight()) {
			const int c = minValue(rect.calcRight() - x, length);
			source.render(x, y, c, output);
			x += c;
			length -= c;
		}
	}
	if (length > 0) {
		output.addTransparent(length);
	}	
}

/* --- Offsetter --- */

template<class T> Offsetter<T>::Offsetter(const Renderer<T>& source, int offsetX, int offsetY)
	: source(source)
	, offsetX(offsetX)
	, offsetY(offsetY)
{
}

template<class T> IntRect Offsetter<T>::calcBounds() const
{
	return source.calcBounds().offset(offsetX, offsetY);
}

template<class T> void Offsetter<T>::render(int x, int y, int length, SpanBuffer<T>& output) const
{
	assert(0 < length && length <= MAX_RENDER_LENGTH);
	return source.render(x - offsetX, y - offsetY, length, output);
}

/* --- Raster --- */

template<class T> Raster<T>::Raster() { }

template<class T> Raster<T>::Raster(typename T::Pixel* pixels, int stride, const IntRect& bounds, bool opaque)
	: pixels(pixels)
	, stride(stride)
	, bounds(bounds)
	, opaque(opaque)
{
}

template<class T> IntRect Raster<T>::calcBounds() const
{
	return bounds;
}

template<class T> void Raster<T>::render(int x, int y, int length, SpanBuffer<T>& output) const
{
// FIX : sub, or should we have a clipped-renderer class?
	assert(0 < length && length <= MAX_RENDER_LENGTH);
	
	if (y >= bounds.top && y < bounds.calcBottom()) {
		if (x < bounds.left) {
			const int c = minValue(bounds.left - x, length);
			output.addTransparent(c);
			x += c;
			length -= c;
		}
		assert(length >= 0);
		if (length > 0 && x < bounds.calcRight()) {
			const int c = minValue(bounds.calcRight() - x, length);
			output.addReference(c, pixels + stride * y + x, opaque);
			x += c;
			length -= c;
		}
	}
	if (length > 0) {
		output.addTransparent(length);
	}
}

template<class T> void Raster<T>::fill(const Renderer<T>& source, const IntRect& area)
{
	assert(area.isEmpty() || bounds.calcUnion(area) == bounds);		// area must be within target raster bounds
	const int right = area.calcRight();
	const int bottom = area.calcBottom();
	for (int y = area.top; y < bottom; ++y) {
		for (int x = area.left; x < right; x += MAX_RENDER_LENGTH) {
			const int length = minValue(right - x, MAX_RENDER_LENGTH);
			NUXPIXELS_SPAN_ARRAY(T, spanArray);
			SpanBuffer<T> output(spanArray, pixels + stride * y + x);
			source.render(x, y, length, output);
			typename T::Pixel* target = pixels + stride * y + x;
			typename SpanBuffer<T>::iterator it = output.begin();
			while (it != output.end()) {
				const int count = it->getLength();
				if (it->isSolid()) {
					fillPixels<T>(count, target, it->getSolidPixel());
				} else {
					copyPixels<T>(count, target, it->getVariablePixels());
				}
				target += it->getLength();
				++it;
			}
		}
	}
}

template<class T> Raster<T>& Raster<T>::operator=(const Renderer<T>& source) { fill(source, bounds); return (*this); }
template<class T> Raster<T>& Raster<T>::operator|=(const Renderer<T>& source) { fill((*this) | source, bounds.calcIntersection(source.calcBounds())); return (*this); }
template<class T> Raster<T>& Raster<T>::operator+=(const Renderer<T>& source) { fill((*this) + source, bounds.calcIntersection(source.calcBounds())); return (*this); }

/* --- SelfContainedRaster --- */

template<class T> SelfContainedRaster<T>::SelfContainedRaster() : allocated(0) {
	this->bounds = IntRect(0, 0, 0, 0);
	this->stride = 0;
	this->opaque = false;
	this->allocated = T::allocate(0);
	this->pixels = this->allocated;
}

template<class T> SelfContainedRaster<T>::SelfContainedRaster(const IntRect& bounds, bool opaque) : allocated(0) {
	this->bounds = bounds;
	this->stride = bounds.width;
	this->opaque = opaque;
	this->allocated = T::allocate(bounds.width * bounds.height);
	this->pixels = this->allocated - (bounds.top * this->stride + bounds.left);
}

template<class T> SelfContainedRaster<T>::SelfContainedRaster(const SelfContainedRaster& that) : allocated(0) {
	this->bounds = that.bounds;
	this->stride = that.bounds.width;
	this->opaque = that.opaque;
	this->allocated = T::allocate(this->bounds.width * this->bounds.height);
	this->pixels = allocated - (this->bounds.top * this->stride + this->bounds.left);
	(*this) = static_cast< const Renderer<T>& >(that);
}

template<class T> SelfContainedRaster<T>& SelfContainedRaster<T>::operator=(const SelfContainedRaster& that) {
	if (&that != this) {
		if (that.bounds != this->bounds) {
			SelfContainedRaster<T> newRaster(that);
			std::swap(this->bounds, newRaster.bounds);
			std::swap(this->stride, newRaster.stride);
			std::swap(this->opaque, newRaster.opaque);
			std::swap(this->allocated, newRaster.allocated);
			std::swap(this->pixels, newRaster.pixels);
		} else {
			(*this) = static_cast< const Renderer<T>& >(that);
		}
	}
	return (*this);
}

template<class T> SelfContainedRaster<T>::~SelfContainedRaster() {
	T::free(allocated);
	allocated = 0;
}

/* --- RLERaster --- */

template<class T> RLERaster<T>::RLERaster(const IntRect& bounds, const Renderer<T>& source) : bounds(bounds), opaque(false) { fill(source); }
template<class T> RLERaster<T>::RLERaster() : opaque(false) { rewind(); }
template<class T> IntRect RLERaster<T>::calcBounds() const { return bounds; }

template<class T> void RLERaster<T>::render(int x, int y, int length, SpanBuffer<T>& output) const
{
// FIX : sub, or should we have a clipped-renderer class?
	assert(0 < length && length <= MAX_RENDER_LENGTH);
	
	if (y >= bounds.top && y < bounds.calcBottom()) {
		if (x < bounds.left) {
			const int c = minValue(bounds.left - x, length);
			output.addTransparent(c);
			x += c;
			length -= c;
		}
		assert(length >= 0);
		size_t spanIndex;
		size_t pixelIndex;
		int sx;
		if (y != lastY || x < lastX) {
			spanIndex = rows[y - bounds.top].first;
			pixelIndex = rows[y - bounds.top].second;
			sx = bounds.left;
		} else {
			spanIndex = lastSpanIndex;
			pixelIndex = lastPixelIndex;
			sx = lastX;
		}
		while (length > 0 && x < bounds.calcRight()) {
			int c = minValue(bounds.calcRight() - x, length);
										
			int l = spans[spanIndex] & 0x3FFF;
			while (x >= sx + l) {
				sx += l;
				pixelIndex += (((spans[spanIndex] & 0x8000) != 0) ? 1 : l);
				++spanIndex;
				assert(pixelIndex < pixels.size());
				assert(spanIndex < spans.size());
				l = spans[spanIndex] & 0x3FFF;
			}
			c = minValue(c, sx + l - x);
			if ((spans[spanIndex] & 0x8000) != 0) {
				output.addSolid(c, pixels[pixelIndex]);
			}  else {
				output.addReference(c, &pixels[pixelIndex + x - sx], ((spans[spanIndex] & 0x4000) != 0));
			}
			x += c;
			length -= c;
		}
		lastX = sx;
		lastY = y;
		lastSpanIndex = spanIndex;
		lastPixelIndex = pixelIndex;
	}
	if (length > 0) {
		output.addTransparent(length);
	}
}

template<class T> void RLERaster<T>::rewind()
{
	lastX = bounds.left;
	lastY = bounds.top;
	lastSpanIndex = 0;
	lastPixelIndex = 0;
}

template<class T> void RLERaster<T>::swap(RLERaster<T>& other)
{
	bounds.swap(other.bounds);
	spans.swap(other.spans);
	pixels.swap(other.pixels);
	rows.swap(other.rows);
	std::swap(opaque, other.opaque);
	rewind();
}

template<class T> void RLERaster<T>::fill(const Renderer<T>& source)
{
	RLERaster<T> newRLE;
	newRLE.bounds = bounds;
	newRLE.opaque = true;

	const int right = bounds.calcRight();
	const int bottom = bounds.calcBottom();
	for (int y = bounds.top; y < bottom; ++y) {
		newRLE.rows.push_back(std::pair<size_t, size_t>(newRLE.spans.size(), newRLE.pixels.size()));
		bool first = true;
		for (int x = bounds.left; x < right; x += MAX_RENDER_LENGTH) {
			const int length = minValue(right - x, MAX_RENDER_LENGTH);
			NUXPIXELS_SPAN_ARRAY(T, spanArray);
			typename T::Pixel rlePixels[MAX_RENDER_LENGTH];
			SpanBuffer<T> output(spanArray, rlePixels);
			source.render(x, y, length, output);
			typename SpanBuffer<T>::iterator it = output.begin();
			while (it != output.end()) {
				assert(it->getLength() < 0x4000);
				const bool opaqueSpan = it->isOpaque();
				const bool solidSpan = it->isSolid();
				const UInt16 span = it->getLength() | (solidSpan ? 0x8000 : 0) | (opaqueSpan ? 0x4000 : 0);
				if (!first
						&& (span & 0xC000) == (newRLE.spans.back() & 0xC000)
						&& (!solidSpan || it->getSolidPixel() == newRLE.pixels.back())
						&& ((newRLE.spans.back() & 0x3FFF) + it->getLength()) < 0x4000) {
					newRLE.spans.back() += it->getLength();
					if (!it->isSolid()) {
						newRLE.pixels.insert(newRLE.pixels.end(), it->getVariablePixels(), it->getVariablePixels() + it->getLength());
					}									
				} else {
					newRLE.spans.push_back(span);
					if (it->isSolid()) {
						newRLE.pixels.push_back(it->getSolidPixel());
					} else {
						newRLE.pixels.insert(newRLE.pixels.end(), it->getVariablePixels(), it->getVariablePixels() + it->getLength());
					}
				}
				if (!opaqueSpan) {
					newRLE.opaque = false;
				}
				first = false;
				++it;
			}
		}
	}

	swap(newRLE);
}

template<class T> bool RLERaster<T>::isOpaque() const { return opaque; }

template<class T> RLERaster<T>& RLERaster<T>::operator=(const Renderer<T>& source) { fill(source); return (*this); }
template<class T> RLERaster<T>& RLERaster<T>::operator|=(const Renderer<T>& source) { (*this) = (*this) | source; return (*this); }
template<class T> RLERaster<T>& RLERaster<T>::operator+=(const Renderer<T>& source) { (*this) = (*this) + source; return (*this); }
			
/* --- UnaryOperator --- */

template<class S, class T> UnaryOperator<S, T>::UnaryOperator(const Renderer<S>& source)
	: source(source)
{
}

template<class S, class T> void UnaryOperator<S, T>::render(int x, int y, int length, SpanBuffer<S>& inputBuffer, SpanBuffer<T>& output) const
{
	typename SpanBuffer<S>::iterator sourceIt = inputBuffer.end();
	source.render(x, y, length, inputBuffer);
	while (sourceIt != inputBuffer.end()) {
		const Span<S> span = *sourceIt++;
		const int spanLength = span.getLength();
		bool opaque = span.isOpaque();
		if (span.isSolid()) {
			const typename S::Pixel sourcePixel = span.getSolidPixel();
			typename T::Pixel targetPixel;
			process(1, &sourcePixel, &targetPixel, opaque);
			output.addSolid(spanLength, targetPixel);
		} else {
			const typename S::Pixel* const sourcePixels = span.getVariablePixels();
			typename T::Pixel* const targetPixels = output.preallocatePixels();
			process(spanLength, sourcePixels, targetPixels, opaque);
			output.addVariable(spanLength, opaque);
		}
	}
}

template<class S, class T> template<class U> void UnaryOperator<S, T>::render(int x, int y, int length, const Renderer<U>&, SpanBuffer<T>& output) const
{
	NUXPIXELS_SPAN_ARRAY(S, inputSpans);
	typename S::Pixel inputPixels[MAX_RENDER_LENGTH];
	SpanBuffer<S> inputBuffer(inputSpans, inputPixels);
	render(x, y, length, inputBuffer, output);
}

template<class S, class T> void UnaryOperator<S, T>::render(int x, int y, int length, const Renderer<T>&, SpanBuffer<T>& output) const
{
	SpanBuffer<S> inputBuffer(output);
	render(x, y, length, inputBuffer, output);
}

template<class S, class T> void UnaryOperator<S, T>::render(int x, int y, int length, SpanBuffer<T>& output) const
{
	assert(0 < length && length <= MAX_RENDER_LENGTH);
	render(x, y, length, source, output);
}
				
/* --- LookupTable --- */

template<class T> LookupTable<T>::LookupTable()
	: opaque(false)
{
}

template<class T> LookupTable<T>::LookupTable(const typename T::Pixel* table, bool opaque)
	: opaque(opaque)
{
	std::copy(table, table + 256, this->table);
}

/* --- Lookup --- */

template<class T, class L> Lookup<T, L>::Lookup(const Renderer<Mask8>& source, const L& table)
	: UnaryOperator<Mask8, T>(source)
	, table(table)
{
}

template<class T, class L> IntRect Lookup<T, L>::calcBounds() const
{
	return (T::isTransparent(table[0])) ? super::source.calcBounds() : FULL_RECT;
}

template<class T, class L> void Lookup<T, L>::process(int count, const Mask8::Pixel* source, typename T::Pixel* target, bool& opaque) const
{
	for (int i = 0; i < count; ++i) target[i] = table[source[i]];
	opaque = table.isOpaque();
}

/* --- Inverter --- */

template<class T> Inverter<T>::Inverter(const Renderer<T>& source)
	: UnaryOperator<T, T>(source)
{
}

template<class T> IntRect Inverter<T>::calcBounds() const { return FULL_RECT; }

template<class T> void Inverter<T>::process(int count, const typename T::Pixel* source, typename T::Pixel* target, bool& opaque) const
{
	for (int i = 0; i < count; ++i) target[i] = T::invert(source[i]);
	opaque = false;
}

/* --- Converter --- */

template<class S, class T> Converter<S, T>::Converter(const Renderer<S>& source)
	: UnaryOperator<S, T>(source)
{
}

template<class S, class T> IntRect Converter<S, T>::calcBounds() const
{
	return T::isTransparent(convert(S(), T(), S::transparent())) ? super::source.calcBounds() : FULL_RECT;
}

template<class S, class T> void Converter<S, T>::process(int count, const typename S::Pixel* source, typename T::Pixel* target, bool& /*opaque*/) const
{
	for (int i = 0; i < count; ++i) target[i] = convert(S(), T(), source[i]);
}

/* --- BinaryOperator --- */

template<class A, class B> BinaryOperator<A, B>::BinaryOperator(const Renderer<A>& rendererA, const Renderer<B>& rendererB)
	: rendererA(rendererA)
	, rendererB(rendererB)
{
}

/* --- Blender --- */

template<class T> Blender<T>::Blender(const Renderer<T>& rendererA, const Renderer<T>& rendererB) ///< rendererA is background, rendererB is overlay
	: super(rendererA, rendererB)
	, boundsA(rendererA.calcBounds())
	, boundsB(rendererB.calcBounds())
{
}

template<class T> IntRect Blender<T>::calcBounds() const
{
	// FIX : check that transparent * transparent == transparent before unioning
	return boundsA.calcUnion(boundsB);
}

template<class T> void Blender<T>::render(int x, int y, int length, SpanBuffer<T>& output) const
{
	// FIX : Example of unoptimized code in comment here...
	
	assert(0 < length && length <= MAX_RENDER_LENGTH);

	bool intersectsA = y >= boundsA.top && y <= boundsA.top + boundsA.height && x + length > boundsA.left && x < boundsA.left + boundsA.width;
	bool intersectsB = y >= boundsB.top && y <= boundsB.top + boundsB.height && x + length > boundsB.left && x < boundsB.left + boundsB.width;
	if (!intersectsA && !intersectsB) {
		output.addTransparent(length);
		return;
	} else if (!intersectsB) {
		super::rendererA.render(x, y, length, output);
		return;
	} else if (!intersectsA) {
		super::rendererB.render(x, y, length, output);
		return;
	}

	NUXPIXELS_SPAN_ARRAY(T, spanArrayB);
	SpanBuffer<T> spansB(spanArrayB, super::pixelsB);
	super::rendererB.render(x, y, length, spansB);
	typename SpanBuffer<T>::iterator beginB = spansB.begin();
	typename SpanBuffer<T>::iterator endB = spansB.end();
	if (endB == (beginB + 1) && beginB->isTransparent()) {
		super::rendererA.render(x, y, length, output);
		return;
	}
	int right = x + length;
	while (beginB != endB && beginB->isOpaque()) {
		x += beginB->getLength();
		output.addSpan(*beginB++);
	}
	typename SpanBuffer<T>::iterator trimmedEnd = endB;
	while (trimmedEnd != beginB && (trimmedEnd - 1)->isOpaque()) {
		--trimmedEnd;
		right -= trimmedEnd->getLength();
	}

	SpanBuffer<T> spansA(output);
	typename SpanBuffer<T>::iterator beginA = spansA.end();
	if (x < right) {
		super::rendererA.render(x, y, right - x, spansA);
	}

	typename SpanBuffer<T>::iterator itA = beginA;
	typename SpanBuffer<T>::iterator itB = beginB;
	while (itA != spansA.end()) {
		assert(itB != endB);
		merge(spansA, spansB, itA, itB);
		const Span<T> spanA = *itA++;
		const Span<T> spanB = *itB++;
		int spanLength = spanA.getLength();
		if (spanB.isTransparent()) {
			output.add(spanLength, spanA);
		} else if (spanB.isOpaque()) {
			output.add(spanLength, spanB);
		} else if (spanA.isSolid() && spanB.isSolid()) {
			output.addSolid(spanLength, T::blend(spanA.getSolidPixel(), spanB.getSolidPixel()));
		} else {
			typename T::Pixel* const pixels = output.addVariable(spanLength, spanA.isOpaque());
			if (spanA.isSolid()) {
				const typename T::Pixel pixelA = spanA.getSolidPixel();
				const typename T::Pixel* pixelsB = spanB.getVariablePixels();
				int i = 0;
				while (i + 4 <= spanLength) {
					const typename T::Pixel b0 = pixelsB[i + 0];
					const typename T::Pixel b1 = pixelsB[i + 1];
					const typename T::Pixel b2 = pixelsB[i + 2];
					const typename T::Pixel b3 = pixelsB[i + 3];
					pixels[i + 0] = T::blend(pixelA, b0);
					pixels[i + 1] = T::blend(pixelA, b1);
					pixels[i + 2] = T::blend(pixelA, b2);
					pixels[i + 3] = T::blend(pixelA, b3);
					i += 4;
				}
				while (i < spanLength) {
					pixels[i] = T::blend(pixelA, pixelsB[i]);
					++i;
				}
			} else if (spanB.isSolid()) {
				blendSolidToPixels<T>(spanLength, pixels, spanB.getSolidPixel(), spanA.getVariablePixels());
			} else {
				blendPixelsToPixels<T>(spanLength, pixels, spanB.getVariablePixels(), spanA.getVariablePixels());
			}
		}
	}
	while (trimmedEnd != endB) {
		output.addSpan(*trimmedEnd++);
	}
}

/* --- Adder --- */

template<class T> Adder<T>::Adder(const Renderer<T>& rendererA, const Renderer<T>& rendererB)
	: super(rendererA, rendererB)
{
}

template<class T> IntRect Adder<T>::calcBounds() const
{
	return super::rendererA.calcBounds().calcUnion(super::rendererB.calcBounds());
}

template<class T> void Adder<T>::render(int x, int y, int length, SpanBuffer<T>& output) const
{
	assert(0 < length && length <= MAX_RENDER_LENGTH);
	
	NUXPIXELS_SPAN_ARRAY(T, spanArrayB);
	SpanBuffer<T> spansB(spanArrayB, super::pixelsB);
	super::rendererB.render(x, y, length, spansB);
	typename SpanBuffer<T>::iterator beginB = spansB.begin();
	typename SpanBuffer<T>::iterator endB = spansB.end();
	if (endB == (beginB + 1) && beginB->isTransparent()) {
		super::rendererA.render(x, y, length, output);
		return;
	}

	int leftEdge = x;
	int rightEdge = x + length;
	while (beginB != endB && beginB->isMaximum()) {
		leftEdge += beginB->getLength();
		++beginB;
	}
	while (endB != beginB && (endB - 1)->isMaximum()) {
		--endB;
		rightEdge -= endB->getLength();
	}
	if (leftEdge - x > 0) {
		output.addSolid(leftEdge - x, T::maximum());
	}
	SpanBuffer<T> spansA(output);
	typename SpanBuffer<T>::iterator beginA = spansA.end();
	if (leftEdge < rightEdge) {
		super::rendererA.render(leftEdge, y, rightEdge - leftEdge, spansA);
	}
	
	typename SpanBuffer<T>::iterator itA = beginA;
	typename SpanBuffer<T>::iterator itB = beginB;
	while (itA != spansA.end()) {
		assert(itB != spansB.end());
		merge(spansA, spansB, itA, itB);
		const Span<T> spanA = *itA++;
		const Span<T> spanB = *itB++;
		int spanLength = spanA.getLength();
		if (spanA.isMaximum() || spanB.isTransparent()) {
			output.add(spanLength, spanA);
		} else if (spanB.isMaximum() || spanA.isTransparent()) {
			output.add(spanLength, spanB);
		} else if (spanA.isSolid() && spanB.isSolid()) {
			output.addSolid(spanLength, T::add(spanA.getSolidPixel(), spanB.getSolidPixel()));
		} else {
			typename T::Pixel* const pixels = output.addVariable(spanLength, spanA.isOpaque() || spanB.isOpaque());
			if (spanA.isSolid()) {
				typename T::Pixel pixelA = spanA.getSolidPixel();
				const typename T::Pixel* pixelsB = spanB.getVariablePixels();
				for (int i = 0; i < spanLength; ++i) pixels[i] = T::add(pixelA, pixelsB[i]);
			} else if (spanB.isSolid()) {
				const typename T::Pixel* pixelsA = spanA.getVariablePixels();
				typename T::Pixel pixelB = spanB.getSolidPixel();
				for (int i = 0; i < spanLength; ++i) pixels[i] = T::add(pixelsA[i], pixelB);
			} else {
				const typename T::Pixel* pixelsA = spanA.getVariablePixels();
				const typename T::Pixel* pixelsB = spanB.getVariablePixels();
				for (int i = 0; i < spanLength; ++i) pixels[i] = T::add(pixelsA[i], pixelsB[i]);
			}
		}
	}
	if (x + length - rightEdge > 0) {
		output.addSolid(x + length - rightEdge, T::maximum());
	}
}

/* --- Multiplier --- */

template<class A, class B> Multiplier<A, B>::Multiplier(const Renderer<A>& rendererA, const Renderer<B>& rendererB)
	: super(rendererA, rendererB)
{
}

template<class A, class B> IntRect Multiplier<A, B>::calcBounds() const
{
	return super::rendererA.calcBounds().calcIntersection(super::rendererB.calcBounds());
}

template<class A, class B> void Multiplier<A, B>::render(int x, int y, int length, SpanBuffer<A>& output) const
{
	assert(0 < length && length <= MAX_RENDER_LENGTH);
	
	NUXPIXELS_SPAN_ARRAY(B, spanArrayB);
	SpanBuffer<B> spansB(spanArrayB, super::pixelsB);
	super::rendererB.render(x, y, length, spansB);
	typename SpanBuffer<B>::iterator beginB = spansB.begin();
	typename SpanBuffer<B>::iterator endB = spansB.end();
	if (endB == (beginB + 1) && beginB->isMaximum()) {
		super::rendererA.render(x, y, length, output);
		return;
	}

	int leftEdge = x;
	int rightEdge = x + length;
	while (beginB != endB && beginB->isTransparent()) {
		leftEdge += beginB->getLength();
		++beginB;
	}
	while (endB != beginB && (endB - 1)->isTransparent()) {
		--endB;
		rightEdge -= endB->getLength();
	}
	if (leftEdge - x > 0) {
		output.addTransparent(leftEdge - x);
	}
	SpanBuffer<A> spansA(output);
	typename SpanBuffer<A>::iterator beginA = spansA.end();
	if (rightEdge - leftEdge > 0) {
		super::rendererA.render(leftEdge, y, rightEdge - leftEdge, spansA);
	}
	
	typename SpanBuffer<A>::iterator itA = beginA;
	typename SpanBuffer<B>::iterator itB = beginB;
	while (itA != spansA.end()) {
		assert(itB != spansB.end());
		merge(spansA, spansB, itA, itB);
		const Span<A> spanA = *itA++;
		const Span<B> spanB = *itB++;
		int spanLength = spanA.getLength();
		if (spanA.isTransparent() || spanB.isMaximum()) {
			output.add(spanLength, spanA);
		} else if (spanB.isTransparent()) {
			output.addTransparent(spanLength);
		} else if (spanA.isSolid() && spanB.isSolid()) {
			output.addSolid(spanLength, A::multiply(spanA.getSolidPixel(), spanB.getSolidPixel()));
		} else {
			typename A::Pixel* pixels = output.addVariable(spanLength, spanA.isOpaque() && spanB.isOpaque());
			if (spanA.isSolid()) {
				typename A::Pixel pixelA = spanA.getSolidPixel();
				const typename B::Pixel* pixelsB = spanB.getVariablePixels();
				for (int i = 0; i < spanLength; ++i) pixels[i] = A::multiply(pixelA, pixelsB[i]);
			} else if (spanB.isSolid()) {
				const typename A::Pixel* pixelsA = spanA.getVariablePixels();
				typename B::Pixel pixelB = spanB.getSolidPixel();
				for (int i = 0; i < spanLength; ++i) pixels[i] = A::multiply(pixelsA[i], pixelB);
			} else {
				const typename A::Pixel* pixelsA = spanA.getVariablePixels();
				const typename B::Pixel* pixelsB = spanB.getVariablePixels();
				for (int i = 0; i < spanLength; ++i) pixels[i] = A::multiply(pixelsA[i], pixelsB[i]);
			}
		}
	}

	if (x + length - rightEdge > 0) {
		output.addTransparent(x + length - rightEdge);
	}
}

/* --- Optimizer --- */

template<class T> Optimizer<T>::Optimizer(const Renderer<T>& source)
	: source(source)
{
}

template<class T> IntRect Optimizer<T>::calcBounds() const
{
	return source.calcBounds();
}

template<class T> const typename T::Pixel* Optimizer<T>::outputVariable(const typename T::Pixel* b, const typename T::Pixel* e, bool opaque, SpanBuffer<T>& output)
{
	if (e - b != 0) {
		output.addReference(static_cast<int>(e - b), b, opaque);
	}
	return e;
}
		
template<class T> const typename T::Pixel* Optimizer<T>::analyzeSolid(const typename T::Pixel* b, const typename T::Pixel* e, SpanBuffer<T>& output)
{
	assert(b + 4 <= e && b[1] == b[0] && b[2] == b[0] && b[3] == b[0]);
	const typename T::Pixel* p = b + 4;
	while (p != e && *p == *b) {
		++p;
	}
	output.addSolid(static_cast<int>(p - b), *b);
	return p;
}

template<class T> const typename T::Pixel* Optimizer<T>::analyzeOpaque(const typename T::Pixel* b, const typename T::Pixel* e, SpanBuffer<T>& output)
{
	const typename T::Pixel* p = b;
	while (p != e && T::isOpaque(*p)) {
		if (p + 4 <= e && p[1] == p[0] && p[2] == p[0] && p[3] == p[0]) {
			p = b = analyzeSolid(outputVariable(b, p, true, output), e, output);
		} else {
			++p;
		}
	}
	return outputVariable(b, p, true, output);
}

template<class T> const typename T::Pixel* Optimizer<T>::analyzeNonOpaque(const typename T::Pixel* b, const typename T::Pixel* e, SpanBuffer<T>& output)
{
	const typename T::Pixel* p = b;
	while (p != e) {
		if (p + 4 <= e && p[1] == p[0] && p[2] == p[0] && p[3] == p[0]) {
			p = b = analyzeSolid(outputVariable(b, p, false, output), e, output);
		} else if (p + 4 <= e && T::isOpaque(p[0]) && T::isOpaque(p[1]) && T::isOpaque(p[2]) && T::isOpaque(p[3])) {
			p = b = analyzeOpaque(outputVariable(b, p, false, output), e, output);
		} else {
			++p;
		}
	}
	return outputVariable(b, p, false, output);
}

template<class T> void Optimizer<T>::render(int x, int y, int length, SpanBuffer<T>& output) const
{
	assert(0 < length && length <= MAX_RENDER_LENGTH);
	
	SpanBuffer<T> inputBuffer(output);
	typename SpanBuffer<T>::iterator it = inputBuffer.end();
	source.render(x, y, length, inputBuffer);
	while (it != inputBuffer.end()) {
		if (it->isSolid()) {
			output.addSpan(*it++);
		} else {
			const typename T::Pixel* b = it->getVariablePixels();
			const typename T::Pixel* e = b + it->getLength();
			if ((it++)->isOpaque()) {
				b = analyzeOpaque(b, e, output);
			} else {
				b = analyzeNonOpaque(b, e, output);
			}
			assert(b == e); // If not we might have run into an unopaque pixel in an opaque span.
		}
	}
}

/* --- Gradient --- */

template<class T> void Gradient<T>::init(int count, const Stop* points)
{
	assert(count > 0);

	std::vector<int> positions(count + 2);
	std::vector<typename T::Pixel> colors(count + 2);
	positions[0] = 0;
	for (int i = 0; i < count; ++i) {
		assert(T::isValid(points[i].color));
		positions[i + 1] = roundToInt(points[i].position * 255);
		assert(0 <= positions[i + 1] && positions[i + 1] <= 255);
		assert(positions[i + 1] >= positions[i]);
		colors[i + 1] = points[i].color;
	}
	colors[0] = colors[1];
	colors[count + 1] = colors[count];
	positions[count + 1] = 256;

	super::opaque = true;
	int pointIndex = 0;
	Int32 fractionScale = 0;
	for (int tableIndex = 0; tableIndex < 256; ++tableIndex) {
		while (tableIndex >= positions[pointIndex + 1]) {
			++pointIndex;
			if (positions[pointIndex + 1] > positions[pointIndex]) {
				fractionScale = (1 << 30) / (positions[pointIndex + 1] - positions[pointIndex]);
			}
		}
		UInt32 fraction = static_cast<UInt32>((tableIndex - positions[pointIndex]) * fractionScale >> 22);
		assert(0 <= fraction && fraction <= 256);
		super::table[tableIndex] = T::interpolate(colors[pointIndex], colors[pointIndex + 1], fraction);
		assert(T::isValid(super::table[tableIndex]));
		super::opaque = (super::opaque && T::isOpaque(super::table[tableIndex]));
	}
}

template<class T> Gradient<T>::Gradient(int count, const Stop* points)
{
	init(count, points);
}

template<class T> Gradient<T>::Gradient(typename T::Pixel start, typename T::Pixel end)
{
	Stop points[2] = { { 0.0, start }, { 1.0, end } };
	init(2, points);
}

/* --- Texture --- */

/**
	Impl contains the sampling logic for the Texture renderer.
**/
template<class T> class Texture<T>::Impl {
	friend class Texture<T>;
	public:		Impl(const Raster<T>& image, bool wrap, const AffineTransformation& transformation, const IntRect& sourceRect);
	protected:	int findImage(int span, Fixed32_32& sx, Fixed32_32& sy, SpanBuffer<T>& output) const;
	protected:	int interpolateEdge(int span, Fixed32_32& sx, Fixed32_32& sy, SpanBuffer<T>& output) const;
	protected:	int interpolateInside(int span, Fixed32_32& sx, Fixed32_32& sy, SpanBuffer<T>& output) const;
	protected:	void calcStartPosition(int x, int y, Fixed32_32& sx, Fixed32_32& sy) const;
	protected:	enum TransformType {
					INVALID			///< Can't inverse transformation (i.e. extreme scaling etc).
					, IDENTITY		///< Exact copy, fastest algo.
					, INTEGER		///< 45 degrees rotation etc.
					, UPSCALE		///< Horizontal upscaling, interpolate vertically every n pixels.
					, FRACTIONAL_X	///< Fractional horizontal translation, interpolate horizontally only.
					, FRACTIONAL_Y	///< Fractional vertical translation, interpolate vertically only.
					, ARBITRARY		///< Arbitrary, interpolate every pixel, slowest algo.
				};
	protected:	IntRect imageBounds;
	protected:	int imageStride;
	protected:	typename T::Pixel* imagePixels;
	protected:	bool opaque;
	protected:	bool wrap;
	protected:	IntRect outputBounds;
	protected:	TransformType transformType;
	protected:	Fixed32_32 dxx;
	protected:	Fixed32_32 dxy;
	protected:	Fixed32_32 dyx;
	protected:	Fixed32_32 dyy;
	protected:	Fixed32_32 ox;
	protected:	Fixed32_32 oy;
	protected:	int hop;
};

template<class T> Texture<T>::Impl::Impl(const Raster<T>& image, bool wrap, const AffineTransformation& transformation, const IntRect& sourceRect)
	: imageBounds(image.calcBounds().calcIntersection(sourceRect))
	, imageStride(image.getStride())
	, imagePixels(image.getPixelPointer() + imageBounds.top * imageStride + imageBounds.left) // Offset image so that 0, 0 is always top-left coordinate.
	, opaque(image.isOpaque())
	, wrap(wrap)
	, outputBounds(FULL_RECT) // Set to enclose everything (if wrapping).
	, transformType(INVALID)
{
	AffineTransformation inverseXForm = transformation;
	
	// If not inversible, this probably means an extremely small scale, so no need to draw anything.
	
	if (inverseXForm.invert()) {
		// If not wrapping, find approximative boundaries, to speed up further rendering.
		
		if (!wrap) {
			// FIX : optimize, is there an efficient formula for this?
			Path p;
			p.addRect(imageBounds.left - 1, imageBounds.top - 1, imageBounds.width + 1, imageBounds.height + 1);
			p.transform(transformation);
			outputBounds = p.calcIntBounds();
			outputBounds.left -= 1;
			outputBounds.top -= 1;
			outputBounds.width += 3;
			outputBounds.height += 3;
		}
		
		dxx = toFixed32_32(inverseXForm.matrix[0][0]);
		dxy = toFixed32_32(inverseXForm.matrix[1][0]);
		dyx = toFixed32_32(inverseXForm.matrix[0][1]);
		dyy = toFixed32_32(inverseXForm.matrix[1][1]);

		// We offset with the image top-left in the "integer world" so that any source clipping will be exact.

		ox = add(toFixed32_32(inverseXForm.matrix[0][2]), toFixed32_32(-imageBounds.left, 0));
		oy = add(toFixed32_32(inverseXForm.matrix[1][2]), toFixed32_32(-imageBounds.top, 0));
		hop = high32(dxy) * imageStride + high32(dxx);
		
		// Find optimal transformation algorithm.
		
		bool horizontalInterpolation = (low32(dxx) != 0 || low32(dyx) != 0 || (low32(ox) >> 24) != 0);
		bool verticalInterpolation = (low32(dxy) != 0 || low32(dyy) != 0 || (low32(oy) >> 24) != 0);
		bool noInterpolation = (!horizontalInterpolation && !verticalInterpolation);
		
		if (high32(dxx) == 1 && high32(dxy) == 0 && high32(dyx) == 0 && high32(dyy) == 1 && noInterpolation) {
			transformType = IDENTITY;
		} else if (noInterpolation) {
			transformType = INTEGER;
		} else if (high32(dxx) >= -1 && high32(dxx) <= 0 && high32(dxy) == 0 && low32(dxy) == 0) {
			transformType = UPSCALE;
		} else if (!verticalInterpolation) {
			transformType = FRACTIONAL_X;
		} else if (!horizontalInterpolation) {
			transformType = FRACTIONAL_Y;
		} else {
			transformType = ARBITRARY;
		}
	}
}

template<class T> int Texture<T>::Impl::findImage(int length, Fixed32_32& sx, Fixed32_32& sy, SpanBuffer<T>& output) const
{
	assert(length > 0);
	
	if (wrap) {
		sx = toFixed32_32(NuXPixels::wrap(high32(sx), imageBounds.width), low32(sx));
		sy = toFixed32_32(NuXPixels::wrap(high32(sy), imageBounds.height), low32(sy));
		return 0;
	} else {
		// Binary search for the optimal run-length until image begins (this is similar to a standard division algorithm).
		
int col = high32(sx);
int row = high32(sy);

		if (transformType == IDENTITY) {
			int spanLength;
			if (row < 0 || row >= imageBounds.height || col >= imageBounds.width) {
				spanLength = length;
			} else {
				spanLength = minValue(-col, length);
			}
			output.addTransparent(spanLength);
			assert(low32(sx) == 0);
			sx = toFixed32_32(col + spanLength);
			return spanLength;
		}
		
		int spanLength = 0;
		for (int shift = MAX_SPAN_BITS; shift >= 0; --shift) {
			const Fixed32_32 nx = add(sx, shiftLeft(dxx, shift));
			const Fixed32_32 ny = add(sy, shiftLeft(dxy, shift));
			if ((col < -1 && high32(nx) < -1) || (col >= imageBounds.width && high32(nx) >= imageBounds.width)
					|| (row < -1 && high32(ny) < -1) || (row >= imageBounds.height && high32(ny) >= imageBounds.height)) {
				spanLength += (1 << shift);
				if (spanLength >= length) {
					output.addTransparent(length);
					return length; // FIX : ugly return
				}
				sx = nx;
				sy = ny;
			}
		}
		
		// Go one step into the image.
		
		++spanLength;
		sx = add(sx, dxx);
		sy = add(sy, dxy);

		output.addTransparent(spanLength);
		return spanLength;
	}
}

template<class T> int Texture<T>::Impl::interpolateEdge(int length, Fixed32_32& sx, Fixed32_32& sy, SpanBuffer<T>& output) const
{
	assert(length > 0);
	
	int col = high32(sx);
	int row = high32(sy);
	const typename T::Pixel* s = &imagePixels[row * imageStride + col];
	typename T::Pixel* const pixels = output.preallocatePixels();
	typename T::Pixel* d = pixels;
	typename T::Pixel* e = d + length;
	do {
		typename T::Pixel c00, c10, c01, c11;
		
		if (wrap) {
			int x0 = ((col >= 0) ? 0 : imageBounds.width);
			int x1 = ((col + 1 < imageBounds.width) ? 1 : (1 - imageBounds.width));
			int y0 = ((row >= 0) ? 0 : imageBounds.height * imageStride);
			int y1 = ((row + 1 < imageBounds.height) ? imageStride : imageStride - imageBounds.height * imageStride);
			c00 = s[x0 + y0];
			c10 = s[x1 + y0];
			c01 = s[x0 + y1];
			c11 = s[x1 + y1];
		} else {
			c00 = (col >= 0 && row >= 0) ? s[0] : 0;
			c10 = (col + 1 < imageBounds.width && row >= 0) ? s[1] : 0;
			c01 = (col >= 0 && row + 1 < imageBounds.height) ? s[imageStride] : 0;
			c11 = (col + 1 < imageBounds.width && row + 1 < imageBounds.height) ? s[imageStride + 1] : 0;
		}
		
		int delta;
		do {
			const UInt32 colFraction = low32(sx) >> 24;
			const UInt32 rowFraction = low32(sy) >> 24;
			const typename T::Pixel argb0 = T::interpolate(c00, c10, colFraction);
			const typename T::Pixel argb1 = T::interpolate(c01, c11, colFraction);
			*d++ = T::interpolate(argb0, argb1, rowFraction);
			delta = hop + addCarry(sx, dxx) + (-addCarry(sy, dxy) & imageStride);
		} while (high32(sx) == col && high32(sy) == row && d < e); // Note: can't just check delta, cause we can actually land at the same sample offset even if we change row and column (e.g. in a 1*1 image).
		
		s += delta;
		col = high32(sx);
		row = high32(sy);
	
	} while (d < e
			&& col >= -1 && col < imageBounds.width && row >= -1 && row < imageBounds.height
			&& (col == -1 || col == imageBounds.width - 1 || row == -1 || row == imageBounds.height - 1));
	
	typename T::Pixel* checkPixels = output.addVariable(static_cast<int>(d - pixels), wrap && opaque);
	(void)checkPixels;
	assert(checkPixels == pixels);

	return static_cast<int>(d - pixels);
}

template<class T> int Texture<T>::Impl::interpolateInside(int length, Fixed32_32& sx, Fixed32_32& sy, SpanBuffer<T>& output) const
{
	assert(length > 0);
	
	int spanLength = 0;
	
	if (transformType == IDENTITY) {
		spanLength = minValue(imageBounds.width - high32(sx), length);
	} else {
		// Binary search for the optimal run-length until image wraps / ends (this is similar to a standard division algorithm).

		Fixed32_32 ex = sx;
		Fixed32_32 ey = sy;
		for (int shift = MAX_SPAN_BITS; shift >= 0; --shift) {
			const Fixed32_32 nx = add(ex, shiftLeft(dxx, shift));
			const Fixed32_32 ny = add(ey, shiftLeft(dxy, shift));
			if (high32(nx) >= 0 && high32(nx) + 1 < imageBounds.width && high32(ny) >= 0 && high32(ny) + 1 < imageBounds.height) {
				spanLength = spanLength + (1 << shift);
				if (spanLength >= length) {
					spanLength = length - 1;
					break;
				}
				ex = nx;
				ey = ny;
			}
		}

		// Go one step outside the image.

		++spanLength;
	}

	const typename T::Pixel* s = &imagePixels[high32(sy) * imageStride + high32(sx)];
	switch (transformType) {
		case IDENTITY: {
			output.addReference(spanLength, s, opaque);
			sx = add(sx, toFixed32_32(spanLength, 0));
		}
		break;
		
		case INTEGER: {
			typename T::Pixel* const pixels = output.addVariable(spanLength, opaque);
			for (int i = 0; i < spanLength; ++i) {
				pixels[i] = *s;
				s += hop;
			}
			sx = add(sx, toFixed32_32(spanLength * high32(dxx), 0));
			sy = add(sy, toFixed32_32(spanLength * high32(dxy), 0));
		}
		break;
		
		case UPSCALE: {
			typename T::Pixel* const pixels = output.addVariable(spanLength, opaque);
			int i = 0;
			while (i < spanLength) {
				const UInt32 rowFraction = static_cast<UInt32>(low32(sy) >> 24);
				const typename T::Pixel argb0 = T::interpolate(s[0], s[imageStride], rowFraction);
				const typename T::Pixel argb1 = T::interpolate(s[1], s[imageStride + 1], rowFraction);
				int delta;
				do {
					pixels[i] = T::interpolate(argb0, argb1, low32(sx) >> 24);
					delta = hop + addCarry(sx, dxx);
					++i;
				} while (i < spanLength && delta == 0);
				s += delta;
			}
		}
		break;
		
		case FRACTIONAL_X: {
			typename T::Pixel* const pixels = output.addVariable(spanLength, opaque);
			interpolatePixelsXOnly<T>(spanLength, pixels, s, sx, dxx, hop);
			sy = add(sy, toFixed32_32(spanLength * high32(dxy), 0));
		}
		break;

		case FRACTIONAL_Y: {
			typename T::Pixel* const pixels = output.addVariable(spanLength, opaque);
			interpolatePixelsYOnly<T>(spanLength, pixels, s, imageStride, sy, dxy, hop);
			sx = add(sx, toFixed32_32(spanLength * high32(dxx), 0));
		}
		break;
		
		case ARBITRARY: {
			typename T::Pixel* const pixels = output.addVariable(spanLength, opaque);
			assert(high32(sx) >= 0 && high32(sx) + 1 < imageBounds.width && high32(sy) >= 0 && high32(sy) + 1 < imageBounds.height);
			interpolatePixels<T>(spanLength, pixels, s, imageStride, sx, sy, dxx, dxy, hop);
		}
		break;
		
		default: assert(0);
	}
	
	return spanLength;
}

template<class T> void Texture<T>::Impl::calcStartPosition(int x, int y, Fixed32_32& sx, Fixed32_32& sy) const
{
	switch (transformType) {
		case IDENTITY: {
			sx = toFixed32_32(high32(ox) + x, 0);
			sy = toFixed32_32(high32(oy) + y, 0);
		}
		break;
		
		case INTEGER: {
			sx = toFixed32_32(high32(ox) + x * high32(dxx) + y * high32(dyx), 0);
			sy = toFixed32_32(high32(oy) + x * high32(dxy) + y * high32(dyy), 0);
		}
		break;
		
		case UPSCALE:
		case FRACTIONAL_X:
		case FRACTIONAL_Y:
		case ARBITRARY: {
			// FIX : MUST SUPPORT SIGNED MULT!
			sx = add(add(ox, multiply(static_cast<UInt32>(x), dxx)), multiply(static_cast<UInt32>(y), dyx));
			sy = add(add(oy, multiply(static_cast<UInt32>(x), dxy)), multiply(static_cast<UInt32>(y), dyy));
		}
		break;
		
		default: assert(0);
	}
}

template<class T> Texture<T>::Texture(const Raster<T>& image, bool wrap, const AffineTransformation& transformation, const IntRect& sourceRect)
	: impl(new Impl(image, wrap, transformation, sourceRect))
{
}

template<class T> IntRect Texture<T>::calcBounds() const
{
	return impl->outputBounds;
}

template<class T> void Texture<T>::render(int x, int y, int length, SpanBuffer<T>& output) const
{
	assert(0 < length && length <= MAX_RENDER_LENGTH);
	
	// Invalid transform (probably too small) or outside outputBounds (if not wrapping)
	
	if (impl->transformType == Impl::INVALID
			|| y < impl->outputBounds.top || y >= impl->outputBounds.calcBottom()
			|| x + length <= impl->outputBounds.left || x >= impl->outputBounds.calcRight()) {
		output.addTransparent(length);
		return;
	}
	
	// FIX : drop, didn't improve performance
	/*
	int trimLeft = outputBounds.left - x;
	if (trimLeft > 0) {
		*spans++ = Span<T>(trimLeft, buffer, true, false);
		*buffer++ = 0;
		x += trimLeft;
		length -= trimLeft;
	}
	int trimRight = maxValue(x + length - outputBounds.calcRight(), 0);
	length -= trimRight;
	*/
	
	Fixed32_32 sx;
	Fixed32_32 sy;
	impl->calcStartPosition(x, y, sx, sy);
	
	int colMargin = 1;
	int rowMargin = 1;
	switch (impl->transformType) {
		case Impl::IDENTITY:
		case Impl::INTEGER: {
			colMargin = 0;
			rowMargin = 0;
		}
		break;
		case Impl::FRACTIONAL_X: rowMargin = 0; break;
		case Impl::FRACTIONAL_Y: colMargin = 0; break;
		default: break;
	}
	
	int offset = 0;
	while (offset < length) {
		const int col = high32(sx);
		const int row = high32(sy);
		int spanLength;
		if (col < -colMargin || col >= impl->imageBounds.width || row < -rowMargin || row >= impl->imageBounds.height) {
			spanLength = impl->findImage(length - offset, sx, sy, output);
		} else if (col < 0 || col + colMargin >= impl->imageBounds.width || row < 0 || row + rowMargin >= impl->imageBounds.height) {
			spanLength = impl->interpolateEdge(length - offset, sx, sy, output);
		} else {
			spanLength = impl->interpolateInside(length - offset, sx, sy, output);
		}
		offset += spanLength;
	}

	// FIX : drop, didn't improve performance
/*	if (trimRight > 0) {
		*spans++ = Span<T>(trimRight, buffer, true, false);
		*buffer++ = 0;
	}*/
}

template<class T> Texture<T>::~Texture()
{
	delete impl;
}

} // namespace NuXPixels
					
#endif
