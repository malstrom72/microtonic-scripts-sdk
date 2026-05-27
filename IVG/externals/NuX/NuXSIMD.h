/**
	NuXSIMD is released under the BSD 2-Clause License.

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
#ifndef NuXSIMD_h
#define NuXSIMD_h

#include "assert.h"
#include <algorithm>
#include <float.h>

#ifndef NUXSIMD_SIMULATE
	#define NUXSIMD_SIMULATE 0
#endif

#if defined(_MSC_VER) // Visual C++ version

	#define NUXSIMD_WIN 1
	#define NUXSIMD_UNIX 0 // macOS/Linux
	#define NUXSIMD_BIG_ENDIAN 0
	#define NUXSIMD_LITTLE_ENDIAN 1
	#define NUXSIMD_INLINE __forceinline
	#define NUXSIMD_ALTIVEC 0
	#define NUXSIMD_NEON 0
	#define NUXSIMD_SSE 1
	
#elif defined(__APPLE__) || defined(__linux__) // Unix-like (macOS/Linux)

	#define NUXSIMD_WIN 0
	#define NUXSIMD_UNIX 1 // macOS/Linux
	#if (__ARM_NEON__)
		#define NUXSIMD_BIG_ENDIAN 0
		#define NUXSIMD_LITTLE_ENDIAN 1
		#define NUXSIMD_ALTIVEC 0
		#define NUXSIMD_NEON 1
		#define NUXSIMD_SSE 0
	#elif (__LITTLE_ENDIAN__) || (defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__))
		#define NUXSIMD_BIG_ENDIAN 0
		#define NUXSIMD_LITTLE_ENDIAN 1
		#define NUXSIMD_ALTIVEC 0
		#define NUXSIMD_NEON 0
		#define NUXSIMD_SSE 1
	#elif (__BIG_ENDIAN__) || (defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__))
		#define NUXSIMD_BIG_ENDIAN 1
		#define NUXSIMD_LITTLE_ENDIAN 0
		#define NUXSIMD_ALTIVEC 1
		#define NUXSIMD_NEON 0
		#define NUXSIMD_SSE 0
	#endif
	#define NUXSIMD_INLINE inline __attribute__((always_inline))
	
#else

	#error Unknown platform!

#endif

#if NUXSIMD_SSE
	#ifndef NUXSIMD_SSE2
		#define NUXSIMD_SSE2 1
	#endif
#endif

#if (!NUXSIMD_SIMULATE && NUXSIMD_SSE)
	#include <emmintrin.h>
#endif

#if (!NUXSIMD_SIMULATE && NUXSIMD_NEON)
	#include <arm_neon.h>
#endif

namespace NuXSIMD {

#if (NUXSIMD_WIN)
	#define SIMD_ALIGN(x) __declspec(align(16)) x
#elif (NUXSIMD_UNIX)
	#define SIMD_ALIGN(x) x __attribute__ ((aligned (16)))
#endif // (NUXSIMD_UNIX)

NUXSIMD_INLINE bool isAligned(const void* p) { return ((reinterpret_cast<intptr_t>(p) & 0xF) == 0); }
template<typename T> NUXSIMD_INLINE T* allocateAligned(size_t size) {
	const size_t bytes = ((size * sizeof (T) + 15) & ~15U);
#if (NUXSIMD_NEON || NUXSIMD_UNIX)
	T* alloced = reinterpret_cast<T*>(aligned_alloc(16, bytes));
#else
	T* alloced = reinterpret_cast<T*>(_mm_malloc(bytes, 16));
#endif
	if (alloced == 0) {
		throw std::bad_alloc();
	}
	assert(isAligned(alloced));
	return alloced;
}
NUXSIMD_INLINE void freeAligned(void* p) {
	assert(p != 0);
#if (NUXSIMD_NEON || NUXSIMD_UNIX)
	free(p);
#else
	_mm_free(p);
#endif
}
struct Aligned {
	void* operator new(size_t size) { return allocateAligned<unsigned char>(size); }
	void* operator new[](size_t size) { return allocateAligned<unsigned char>(size); }
	void operator delete(void* p) { freeAligned(p); }
	void operator delete[](void* p) { freeAligned(p); }
};

template<typename T, int N> struct AlignedArray : public Aligned {
	operator T*() { return a; }
	operator const T*() const { return a; }
	T& operator[](ptrdiff_t i) { return a[i]; }
	const T& operator [](ptrdiff_t i) const { return a[i]; }
#if (NUXSIMD_WIN)
	_MM_ALIGN16 T a[N];
#else
	T a[N] __attribute__ ((aligned (16)));
#endif
};

#if (NUXSIMD_SIMULATE)

	typedef union { float f; int i; } FloatIntUnion;
	typedef struct { FloatIntUnion v[4]; } QFloat;

	inline QFloat makeQFloat(FloatIntUnion y) { QFloat q; q.v[0] = q.v[1] = q.v[2] = q.v[3] = y; return q; }
	inline QFloat makeQFloat(float y) { QFloat q; q.v[0].f = q.v[1].f = q.v[2].f = q.v[3].f = y; return q; }
	inline QFloat makeQFloat(int y) { QFloat q; q.v[0].i = q.v[1].i = q.v[2].i = q.v[3].i = y; return q; }
	inline QFloat makeQFloat(FloatIntUnion y3, FloatIntUnion y2, FloatIntUnion y1, FloatIntUnion y0) { QFloat q; q.v[0] = y0; q.v[1] = y1; q.v[2] = y2; q.v[3] = y3; return q; }
	inline QFloat makeQFloat(float y3, float y2, float y1, float y0) { QFloat q; q.v[0].f = y0; q.v[1].f = y1; q.v[2].f = y2; q.v[3].f = y3; return q; }
	inline QFloat makeQFloat(int y3, int y2, int y1, int y0) { QFloat q; q.v[0].i = y0; q.v[1].i = y1; q.v[2].i = y2; q.v[3].i = y3; return q; }

	NUXSIMD_INLINE QFloat loadUnaligned(const float y[4]) { return makeQFloat(y[3], y[2], y[1], y[0]); }
	NUXSIMD_INLINE QFloat loadAligned(const float y[4]) { assert(isAligned(y)); return loadUnaligned(y); }
	NUXSIMD_INLINE QFloat load1(float y) { return makeQFloat(0.0f, 0.0f, 0.0f, y); }
	NUXSIMD_INLINE QFloat load1(QFloat y) { return load1(y.v[0].f); }
	NUXSIMD_INLINE QFloat load1(QFloat x, QFloat y) { return makeQFloat(x.v[3].f, x.v[2].f, x.v[1].f, y.v[0].f); }
	NUXSIMD_INLINE QFloat load4(float y0, float y1, float y2, float y3) { return makeQFloat(y3, y2, y1, y0); }
	NUXSIMD_INLINE QFloat load4(float y) { return makeQFloat(y); }

	// FIX :	NUXSIMD_INLINE void transpose(QFloat& a, QFloat& b, QFloat& c, QFloat& d) { _MM_TRANSPOSE4_PS(a, b, c, d); }

	// FIX :	NUXSIMD_INLINE void interleave(QFloat& x, QFloat& y) {
	//		QFloat nx(_mm_unpacklo_ps(x, y));
	//		y = _mm_unpackhi_ps(x, y);
	//		x = nx;
	//	}

	// FIX :	NUXSIMD_INLINE void deinterleave(QFloat& x, QFloat& y) {
	//		QFloat nx(_mm_shuffle_ps(x, y, _MM_SHUFFLE(2, 0, 2, 0)));
	//		y = _mm_shuffle_ps(x, y, _MM_SHUFFLE(3, 1, 3, 1));
	//		x = nx;
	//	}

	template<int x0, int x1, int x2, int x3> NUXSIMD_INLINE QFloat shuffle(QFloat x) {
		assert(0 <= x0 && x0 <= 3 && 0 <= x1 && x1 <= 3 && 0 <= x2 && x2 <= 3 && 0 <= x3 && x3 <= 3);
		return makeQFloat(x.v[x0], x.v[x1], x.v[x2], x.v[x3]);
	}

	/* FIX :
	template<int x0, int x1, int y2, int y3> NUXSIMD_INLINE QFloat shuffle2(QFloat x, QFloat y) {
		assert(0 <= x0 && x0 <= 3 && 0 <= x1 && x1 <= 3 && 0 <= y2 && y2 <= 3 && 0 <= y3 && y3 <= 3);
		return _mm_shuffle_ps(x, y, _MM_SHUFFLE(y3, y2, x1, x0));
	}
	*/

	const unsigned int SIGN_BIT_INT = 0x80000000UL;
	const QFloat SIGN_BITS = load4(*reinterpret_cast<const float*>(&SIGN_BIT_INT));

	NUXSIMD_INLINE QFloat add(QFloat x, QFloat y) { return makeQFloat(x.v[3].f + y.v[3].f, x.v[2].f + y.v[2].f, x.v[1].f + y.v[1].f, x.v[0].f + y.v[0].f); }
	NUXSIMD_INLINE QFloat sub(QFloat x, QFloat y) { return makeQFloat(x.v[3].f - y.v[3].f, x.v[2].f - y.v[2].f, x.v[1].f - y.v[1].f, x.v[0].f - y.v[0].f); }
	NUXSIMD_INLINE QFloat mul(QFloat x, QFloat y) { return makeQFloat(x.v[3].f * y.v[3].f, x.v[2].f * y.v[2].f, x.v[1].f * y.v[1].f, x.v[0].f * y.v[0].f); }
	NUXSIMD_INLINE QFloat div(QFloat x, QFloat y) { return makeQFloat(x.v[3].f / y.v[3].f, x.v[2].f / y.v[2].f, x.v[1].f / y.v[1].f, x.v[0].f / y.v[0].f); }
	NUXSIMD_INLINE QFloat mulAdd(QFloat x, QFloat y, QFloat z) { return makeQFloat(x.v[3].f * y.v[3].f + z.v[3].f, x.v[2].f * y.v[2].f + z.v[2].f, x.v[1].f * y.v[1].f + z.v[1].f, x.v[0].f * y.v[0].f + z.v[0].f); }
	NUXSIMD_INLINE QFloat minimum(QFloat x, QFloat y) { return makeQFloat(std::min(x.v[3].f, y.v[3].f), std::min(x.v[2].f, y.v[2].f), std::min(x.v[1].f, y.v[1].f), std::min(x.v[0].f, y.v[0].f)); }
	NUXSIMD_INLINE QFloat maximum(QFloat x, QFloat y) { return makeQFloat(std::max(x.v[3].f, y.v[3].f), std::max(x.v[2].f, y.v[2].f), std::max(x.v[1].f, y.v[1].f), std::max(x.v[0].f, y.v[0].f)); }
	NUXSIMD_INLINE QFloat abs(QFloat x) { return makeQFloat(fabsf(x.v[3].f), fabsf(x.v[2].f), fabsf(x.v[1].f), fabsf(x.v[0].f)); }
	// FIX : 	NUXSIMD_INLINE QFloat rcpApprox(QFloat x) { return _mm_rcp_ps(x); }
	// FIX : NUXSIMD_INLINE QFloat rcpImproved(QFloat x) { QFloat y = rcpApprox(x); return sub(add(y, y), mul(mul(x, y), y)); }
	NUXSIMD_INLINE QFloat bitAnd(QFloat x, QFloat y) { return makeQFloat(x.v[3].i & y.v[3].i, x.v[2].i & y.v[2].i, x.v[1].i & y.v[1].i, x.v[0].i & y.v[0].i); }
	NUXSIMD_INLINE QFloat bitAndNot(QFloat x, QFloat y) { return makeQFloat(x.v[3].i & ~y.v[3].i, x.v[2].i & ~y.v[2].i, x.v[1].i & ~y.v[1].i, x.v[0].i & ~y.v[0].i); }
	NUXSIMD_INLINE QFloat bitOr(QFloat x, QFloat y) { return makeQFloat(x.v[3].i | y.v[3].i, x.v[2].i | y.v[2].i, x.v[1].i | y.v[1].i, x.v[0].i | y.v[0].i); }
	NUXSIMD_INLINE QFloat bitXOr(QFloat x, QFloat y) { return makeQFloat(x.v[3].i ^ y.v[3].i, x.v[2].i ^ y.v[2].i, x.v[1].i ^ y.v[1].i, x.v[0].i ^ y.v[0].i); }

	NUXSIMD_INLINE void storeUnaligned(QFloat x, float y[4]) { y[0] = x.v[0].f; y[1] = x.v[1].f; y[2] = x.v[2].f; y[3] = x.v[3].f; }
	NUXSIMD_INLINE void storeAligned(QFloat x, float y[4]) { storeUnaligned(x, y); }
	NUXSIMD_INLINE void streamAligned(QFloat x, float y[4]) { storeUnaligned(x, y); }
	NUXSIMD_INLINE float get1(QFloat x) { return x.v[0].f; }

	/* FIX :
	#if (!NUXSIMD_SSE2)
	template<int exp> NUXSIMD_INLINE QFloat loadAlignedInts(const int y[4])
	{
		assert(isAligned(y));
		assert(0 <= exp && exp < 32);
		AlignedArray<float, 4> yf;
		yf[0] = static_cast<float>(y[0]);
		yf[1] = static_cast<float>(y[1]);
		yf[2] = static_cast<float>(y[2]);
		yf[3] = static_cast<float>(y[3]);
		return (exp == 0) ? loadAligned(yf) : mul(loadAligned(yf), CONST_QFLOAT(static_cast<float>(1.0 / (1U << exp))));
	}
	template<int exp> NUXSIMD_INLINE void storeAlignedInts(QFloat x, int y[4])
	{
		assert(isAligned(y));
		assert(0 <= exp && exp < 32);
		AlignedArray<float, 4> yf;
		storeAligned((exp == 0) ? x : mul(x, CONST_QFLOAT(1U << exp)), yf);
		y[0] = static_cast<int>(yf[0]);
		y[1] = static_cast<int>(yf[1]);
		y[2] = static_cast<int>(yf[2]);
		y[3] = static_cast<int>(yf[3]);
	}
	#else
	typedef __m128i QInt;
	#define CONST_QINT(y) (_mm_set1_epi32(y))
	#define CONST_QINT4(y0, y1, y2, y3) (_mm_set_epi32(y3, y2, y1, y0))

	template<int exp> NUXSIMD_INLINE void storeAlignedInts(QFloat x, int y[4]) { assert(isAligned(y)); assert(0 <= exp && exp < 32); if (exp == 0) { _mm_store_si128(reinterpret_cast<__m128i*>(y), _mm_cvttps_epi32(x)); } else { const __m128 k = _mm_set_ps1(static_cast<float>(1U << exp)); _mm_store_si128(reinterpret_cast<__m128i*>(y), _mm_cvttps_epi32(_mm_mul_ps(x, k))); } }
	template<int exp> NUXSIMD_INLINE QFloat loadAlignedInts(const int y[4]) { assert(isAligned(y)); assert(0 <= exp && exp < 32); if (exp == 0) { return _mm_cvtepi32_ps(_mm_load_si128(reinterpret_cast<const __m128i*>(y))); } else { const __m128 k = _mm_set_ps1(static_cast<float>(1.0 / (1U << exp))); return _mm_mul_ps(_mm_cvtepi32_ps(_mm_load_si128(reinterpret_cast<const __m128i*>(y))), k); } }
	NUXSIMD_INLINE QInt load4(int y0, int y1, int y2, int y3) { return _mm_set_epi32(y3, y2, y1, y0); }
	NUXSIMD_INLINE QInt load4(int y) { return _mm_set1_epi32(y); }
	NUXSIMD_INLINE QInt loadAligned(const int y[]) { return _mm_load_si128(reinterpret_cast<const __m128i*>(y)); }
	NUXSIMD_INLINE void storeAligned(QInt x, int y[]) { return _mm_store_si128(reinterpret_cast<__m128i*>(y), x); }
	template<int exp> NUXSIMD_INLINE QInt toInt(QFloat x) { assert(0 <= exp && exp < 32); if (exp == 0) { return _mm_cvttps_epi32(x); } else { const __m128 k = _mm_set_ps1(static_cast<float>(1U << exp)); return _mm_cvttps_epi32(_mm_mul_ps(x, k)); } }
	template<int exp> NUXSIMD_INLINE QFloat toFloat(QInt x) { assert(0 <= exp && exp < 32); if (exp == 0) { return _mm_cvtepi32_ps(x); } else { const __m128 k = _mm_set_ps1(static_cast<float>(1.0 / (1U << exp))); return _mm_mul_ps(_mm_cvtepi32_ps(x), k); } }
	NUXSIMD_INLINE QInt add(QInt x, QInt y) { return _mm_add_epi32(x, y); }
	NUXSIMD_INLINE QInt sub(QInt x, QInt y) { return _mm_sub_epi32(x, y); }
	NUXSIMD_INLINE QInt bitAnd(QInt x, QInt y) { return _mm_and_si128(x, y); }
	NUXSIMD_INLINE QInt bitAndNot(QInt x, QInt y) { return _mm_andnot_si128(y, x); }
	NUXSIMD_INLINE QInt bitOr(QInt x, QInt y) { return _mm_or_si128(x, y); }
	NUXSIMD_INLINE QInt bitXOr(QInt x, QInt y) { return _mm_xor_si128(x, y); }
	NUXSIMD_INLINE QInt shiftLeft(QInt x, int y) { return _mm_slli_epi32(x, y); }
	NUXSIMD_INLINE QInt zeroShiftRight(QInt x, int y) { return _mm_srli_epi32(x, y); }
	NUXSIMD_INLINE QInt shiftRight(QInt x, int y) { return _mm_srai_epi32(x, y); }
	#endif
	*/

#endif // (NUXSIMD_SIMULATE)

#if (!NUXSIMD_SIMULATE && NUXSIMD_SSE)

	// WARNING! Only use CONST_QFLOAT with immediate constants, never do 'const float f = 3.0; CONST_QFLOAT(f)' for instance. See http://developer.apple.com/hardwaredrivers/ve/model_details.html for more info.
	#define CONST_QFLOAT(y) (_mm_set_ps1(y))
	#define CONST_QFLOAT4(y0, y1, y2, y3) (_mm_set_ps(y3, y2, y1, y0))

	#if defined(_MSC_VER) && !defined(__INTEL_COMPILER)
		NUXSIMD_INLINE float _cdecl copysignf(float a, float b) { return static_cast<float>(_copysign(a, b)); }
	#endif

	typedef __m128 QFloat;

	NUXSIMD_INLINE QFloat loadAligned(const float y[4]) { assert(isAligned(y)); return _mm_load_ps(y); }
	NUXSIMD_INLINE QFloat loadAligned(const QFloat* y) { assert(isAligned(y)); return _mm_load_ps(reinterpret_cast<const float*>(y)); }
	NUXSIMD_INLINE QFloat loadUnaligned(const float y[4]) { return _mm_loadu_ps(y); }
	NUXSIMD_INLINE QFloat loadUnaligned(const QFloat* y) { return _mm_loadu_ps(reinterpret_cast<const float*>(y)); }
	NUXSIMD_INLINE QFloat load1(float y) { return _mm_set_ss(y); }
	NUXSIMD_INLINE QFloat load1(QFloat y) { return _mm_move_ss(_mm_setzero_ps(), y); }
	NUXSIMD_INLINE QFloat load1(QFloat x, QFloat y) { return _mm_move_ss(x, y); } // moves low order word of y to low order word of x
	NUXSIMD_INLINE QFloat load4(float y0, float y1, float y2, float y3) { return _mm_set_ps(y3, y2, y1, y0); }
	NUXSIMD_INLINE QFloat load4(float y) { return _mm_set_ps1(y); }
	NUXSIMD_INLINE QFloat loadFirst2(QFloat x, const float y[2]) { return _mm_loadl_pi(x, reinterpret_cast<const __m64*>(y)); }
	NUXSIMD_INLINE QFloat loadLast2(QFloat x, const float y[2]) { return _mm_loadh_pi(x, reinterpret_cast<const __m64*>(y)); }

	NUXSIMD_INLINE void transpose(QFloat& a, QFloat& b, QFloat& c, QFloat& d) { _MM_TRANSPOSE4_PS(a, b, c, d); }

	NUXSIMD_INLINE void interleave(QFloat& x, QFloat& y) {
		QFloat nx(_mm_unpacklo_ps(x, y));
		y = _mm_unpackhi_ps(x, y);
		x = nx;
	}

	NUXSIMD_INLINE void deinterleave(QFloat& x, QFloat& y) {
		QFloat nx(_mm_shuffle_ps(x, y, _MM_SHUFFLE(2, 0, 2, 0)));
		y = _mm_shuffle_ps(x, y, _MM_SHUFFLE(3, 1, 3, 1));
		x = nx;
	}

	template<int x0, int x1, int x2, int x3> NUXSIMD_INLINE QFloat shuffle(QFloat x) {
		assert(0 <= x0 && x0 <= 3 && 0 <= x1 && x1 <= 3 && 0 <= x2 && x2 <= 3 && 0 <= x3 && x3 <= 3);
		return _mm_shuffle_ps(x, x, _MM_SHUFFLE(x3, x2, x1, x0));
	}

	template<int x0, int x1, int y2, int y3> NUXSIMD_INLINE QFloat shuffle2(QFloat x, QFloat y) {
		assert(0 <= x0 && x0 <= 3 && 0 <= x1 && x1 <= 3 && 0 <= y2 && y2 <= 3 && 0 <= y3 && y3 <= 3);
		return _mm_shuffle_ps(x, y, _MM_SHUFFLE(y3, y2, x1, x0));
	}

	const unsigned int SIGN_BIT_INT = 0x80000000UL;
	const QFloat SIGN_BITS = load4(*reinterpret_cast<const float*>(&SIGN_BIT_INT));

	NUXSIMD_INLINE QFloat add(QFloat x, QFloat y) { return _mm_add_ps(x, y); }
	NUXSIMD_INLINE QFloat sub(QFloat x, QFloat y) { return _mm_sub_ps(x, y); }
	NUXSIMD_INLINE QFloat mul(QFloat x, QFloat y) { return _mm_mul_ps(x, y); }
	NUXSIMD_INLINE QFloat div(QFloat x, QFloat y) { return _mm_div_ps(x, y); }
	NUXSIMD_INLINE QFloat mulAdd(QFloat x, QFloat y, QFloat z) { return _mm_add_ps(_mm_mul_ps(x, y), z); }
	NUXSIMD_INLINE QFloat minimum(QFloat x, QFloat y) { return _mm_min_ps(x, y); }
	NUXSIMD_INLINE QFloat maximum(QFloat x, QFloat y) { return _mm_max_ps(x, y); }
	NUXSIMD_INLINE QFloat abs(QFloat x) { return _mm_andnot_ps(SIGN_BITS, x); }
	NUXSIMD_INLINE QFloat rcpApprox(QFloat x) { return _mm_rcp_ps(x); }
	NUXSIMD_INLINE QFloat rcpImproved(QFloat x) { QFloat y = rcpApprox(x); return sub(add(y, y), mul(mul(x, y), y)); }
	NUXSIMD_INLINE QFloat bitAnd(QFloat x, QFloat y) { return _mm_and_ps(x, y); }
	NUXSIMD_INLINE QFloat bitAndNot(QFloat x, QFloat y) { return _mm_andnot_ps(y, x); }
	NUXSIMD_INLINE QFloat bitOr(QFloat x, QFloat y) { return _mm_or_ps(x, y); }
	NUXSIMD_INLINE QFloat bitXOr(QFloat x, QFloat y) { return _mm_xor_ps(x, y); }
	NUXSIMD_INLINE QFloat less(QFloat x, QFloat y) { return _mm_cmplt_ps(x, y); }
	NUXSIMD_INLINE QFloat lessOrEqual(QFloat x, QFloat y) { return _mm_cmple_ps(x, y); }
	NUXSIMD_INLINE QFloat equal(QFloat x, QFloat y) { return _mm_cmpeq_ps(x, y); }
	NUXSIMD_INLINE QFloat notEqual(QFloat x, QFloat y) { return _mm_cmpneq_ps(x, y); }
	NUXSIMD_INLINE QFloat greaterOrEqual(QFloat x, QFloat y) { return _mm_cmpge_ps(x, y); }
	NUXSIMD_INLINE QFloat greater(QFloat x, QFloat y) { return _mm_cmpgt_ps(x, y); }
	NUXSIMD_INLINE QFloat sqrt(QFloat x) { return _mm_sqrt_ps(x); } // Notice: not available on Altivec

	NUXSIMD_INLINE void storeUnaligned(QFloat x, float y[4]) { _mm_storeu_ps(y, x); }
	NUXSIMD_INLINE void storeUnaligned(QFloat x, QFloat* y) { _mm_storeu_ps(reinterpret_cast<float*>(y), x); }
	NUXSIMD_INLINE void storeAligned(QFloat x, float y[4]) { assert(isAligned(y)); _mm_store_ps(y, x); }
	NUXSIMD_INLINE void storeAligned(QFloat x, QFloat* y) { assert(isAligned(y)); _mm_store_ps(reinterpret_cast<float*>(y), x); }
	NUXSIMD_INLINE void streamAligned(QFloat x, float y[4]) { assert(isAligned(y)); _mm_stream_ps(y, x); }
	NUXSIMD_INLINE void streamAligned(QFloat x, QFloat* y) { assert(isAligned(y)); _mm_stream_ps(reinterpret_cast<float*>(y), x); }
	NUXSIMD_INLINE float get1(QFloat x) { float y; _mm_store_ss(&y, x); return y; }
			
	NUXSIMD_INLINE int getSigns(QFloat x) { return _mm_movemask_ps(x); }

	#if (!NUXSIMD_SSE2)
	template<int exp> NUXSIMD_INLINE QFloat loadAlignedInts(const int y[4])
	{
		assert(isAligned(y));
		assert(0 <= exp && exp < 32);
		AlignedArray<float, 4> yf;
		yf[0] = static_cast<float>(y[0]);
		yf[1] = static_cast<float>(y[1]);
		yf[2] = static_cast<float>(y[2]);
		yf[3] = static_cast<float>(y[3]);
		return (exp == 0) ? loadAligned(yf) : mul(loadAligned(yf), _mm_set_ps1(static_cast<float>(1.0 / (1U << exp))));
	}
	template<int exp> NUXSIMD_INLINE void storeAlignedInts(QFloat x, int y[4])
	{
		assert(isAligned(y));
		assert(0 <= exp && exp < 32);
		AlignedArray<float, 4> yf;
		storeAligned((exp == 0) ? x : mul(x, _mm_set_ps1(1U << exp)), yf);
		y[0] = static_cast<int>(yf[0]);
		y[1] = static_cast<int>(yf[1]);
		y[2] = static_cast<int>(yf[2]);
		y[3] = static_cast<int>(yf[3]);
	}
	#elif (NUXSIMD_SSE2)
	typedef __m128i QInt;
	#define CONST_QINT(y) (_mm_set1_epi32(y))
	#define CONST_QINT4(y0, y1, y2, y3) (_mm_set_epi32(y3, y2, y1, y0))

	template<int exp> NUXSIMD_INLINE void storeAlignedInts(QFloat x, int y[4]) { assert(isAligned(y)); assert(0 <= exp && exp < 32); if (exp == 0) { _mm_store_si128(reinterpret_cast<__m128i*>(y), _mm_cvttps_epi32(x)); } else { const __m128 k = _mm_set_ps1(static_cast<float>(1U << exp)); _mm_store_si128(reinterpret_cast<__m128i*>(y), _mm_cvttps_epi32(_mm_mul_ps(x, k))); } }
	template<int exp> NUXSIMD_INLINE QFloat loadAlignedInts(const int y[4]) { assert(isAligned(y)); assert(0 <= exp && exp < 32); if (exp == 0) { return _mm_cvtepi32_ps(_mm_load_si128(reinterpret_cast<const __m128i*>(y))); } else { const __m128 k = _mm_set_ps1(static_cast<float>(1.0 / (1U << exp))); return _mm_mul_ps(_mm_cvtepi32_ps(_mm_load_si128(reinterpret_cast<const __m128i*>(y))), k); } }
	NUXSIMD_INLINE QInt load4(int y0, int y1, int y2, int y3) { return _mm_set_epi32(y3, y2, y1, y0); }
	NUXSIMD_INLINE QInt load4(int y) { return _mm_set1_epi32(y); }
	NUXSIMD_INLINE QInt load4(unsigned int y) { return _mm_set1_epi32(y); }
	NUXSIMD_INLINE QInt loadAligned(const int y[]) { assert(isAligned(y)); return _mm_load_si128(reinterpret_cast<const __m128i*>(y)); }
	NUXSIMD_INLINE QInt loadAligned(const unsigned int y[]) { assert(isAligned(y)); return _mm_load_si128(reinterpret_cast<const __m128i*>(y)); }
	NUXSIMD_INLINE QInt loadAligned(const QInt* y) { assert(isAligned(y)); return _mm_load_si128(y); }
	NUXSIMD_INLINE QInt loadUnaligned(const int y[]) { return _mm_loadu_si128(reinterpret_cast<const __m128i*>(y)); }
	NUXSIMD_INLINE QInt loadUnaligned(const unsigned int y[]) { return _mm_loadu_si128(reinterpret_cast<const __m128i*>(y)); }
	NUXSIMD_INLINE QInt loadUnaligned(const QInt* y) { return _mm_loadu_si128(y); }
	NUXSIMD_INLINE void storeAligned(QInt x, int y[]) { assert(isAligned(y)); return _mm_store_si128(reinterpret_cast<__m128i*>(y), x); }
	NUXSIMD_INLINE void storeAligned(QInt x, QInt* y) { assert(isAligned(y)); return _mm_store_si128(y, x); }
	NUXSIMD_INLINE void storeAligned(QInt x, unsigned int y[]) { assert(isAligned(y)); return _mm_store_si128(reinterpret_cast<__m128i*>(y), x); }
	NUXSIMD_INLINE void storeUnaligned(QInt x, int y[]) { return _mm_storeu_si128(reinterpret_cast<__m128i*>(y), x); }
	NUXSIMD_INLINE void storeUnaligned(QInt x, QInt* y) { return _mm_storeu_si128(y, x); }
	NUXSIMD_INLINE void storeUnaligned(QInt x, unsigned int y[]) { return _mm_storeu_si128(reinterpret_cast<__m128i*>(y), x); }
	NUXSIMD_INLINE void streamAligned(QInt x, int y[]) { return _mm_stream_si128(reinterpret_cast<__m128i*>(y), x); }
	NUXSIMD_INLINE void streamAligned(QInt x, unsigned int y[]) { return _mm_stream_si128(reinterpret_cast<__m128i*>(y), x); }
	NUXSIMD_INLINE void streamAligned(QInt x, QInt* y) { return _mm_stream_si128(y, x); }
	template<int exp> NUXSIMD_INLINE QInt toInt(QFloat x) { assert(0 <= exp && exp < 32); if (exp == 0) { return _mm_cvttps_epi32(x); } else { const __m128 k = _mm_set_ps1(static_cast<float>(1U << exp)); return _mm_cvttps_epi32(_mm_mul_ps(x, k)); } }
	template<int exp> NUXSIMD_INLINE QFloat toFloat(QInt x) { assert(0 <= exp && exp < 32); if (exp == 0) { return _mm_cvtepi32_ps(x); } else { const __m128 k = _mm_set_ps1(static_cast<float>(1.0 / (1U << exp))); return _mm_mul_ps(_mm_cvtepi32_ps(x), k); } }
	NUXSIMD_INLINE QInt add(QInt x, QInt y) { return _mm_add_epi32(x, y); }
	NUXSIMD_INLINE QInt sub(QInt x, QInt y) { return _mm_sub_epi32(x, y); }
	NUXSIMD_INLINE QInt bitAnd(QInt x, QInt y) { return _mm_and_si128(x, y); }
	NUXSIMD_INLINE QInt bitAndNot(QInt x, QInt y) { return _mm_andnot_si128(y, x); }
	NUXSIMD_INLINE QInt bitOr(QInt x, QInt y) { return _mm_or_si128(x, y); }
	NUXSIMD_INLINE QInt bitXOr(QInt x, QInt y) { return _mm_xor_si128(x, y); }
	template<int y> NUXSIMD_INLINE QInt shiftLeft(QInt x) { static_assert(0 < y && y < 32, "0 < y && y < 32"); return _mm_slli_epi32(x, y); }
	template<int y> NUXSIMD_INLINE QInt zeroShiftRight(QInt x) { static_assert(0 < y && y < 32, "0 < y && y < 32"); return _mm_srli_epi32(x, y); }
	template<int y> NUXSIMD_INLINE QInt shiftRight(QInt x) { static_assert(0 < y && y < 32, "0 < y && y < 32"); return _mm_srai_epi32(x, y); }
	NUXSIMD_INLINE QInt less(QInt x, QInt y) { return _mm_cmplt_epi32(x, y); }
	NUXSIMD_INLINE QInt equal(QInt x, QInt y) { return _mm_cmpeq_epi32(x, y); }
	NUXSIMD_INLINE QInt greater(QInt x, QInt y) { return _mm_cmpgt_epi32(x, y); }
	NUXSIMD_INLINE QInt less16(QInt x, QInt y) { return _mm_cmplt_epi16(x, y); }
	NUXSIMD_INLINE QInt equal16(QInt x, QInt y) { return _mm_cmpeq_epi16(x, y); }
	NUXSIMD_INLINE QInt greater16(QInt x, QInt y) { return _mm_cmpgt_epi16(x, y); }
	NUXSIMD_INLINE QInt less8(QInt x, QInt y) { return _mm_cmplt_epi8(x, y); }
	NUXSIMD_INLINE QInt equal8(QInt x, QInt y) { return _mm_cmpeq_epi8(x, y); }
	NUXSIMD_INLINE QInt greater8(QInt x, QInt y) { return _mm_cmpgt_epi8(x, y); }
	
	NUXSIMD_INLINE QInt add8(QInt x, QInt y) { return _mm_add_epi8(x, y); }
	NUXSIMD_INLINE QInt sub8(QInt x, QInt y) { return _mm_sub_epi8(x, y); }
	NUXSIMD_INLINE QInt add16(QInt x, QInt y) { return _mm_add_epi16(x, y); }
	NUXSIMD_INLINE QInt sub16(QInt x, QInt y) { return _mm_sub_epi16(x, y); }
	NUXSIMD_INLINE QInt mulHigh16(QInt x, QInt y) { return _mm_mulhi_epi16(x, y); }
	NUXSIMD_INLINE QInt mulHighUnsigned16(QInt x, QInt y) { return _mm_mulhi_epu16(x, y); }
	NUXSIMD_INLINE QInt mulLow16(QInt x, QInt y) { return _mm_mullo_epi16(x, y); }
	template<int y> NUXSIMD_INLINE QInt shiftLeft16(QInt x) { static_assert(0 < y && y < 16, "0 < y && y < 16"); return _mm_slli_epi16(x, y); }
	template<int y> NUXSIMD_INLINE QInt zeroShiftRight16(QInt x) { static_assert(0 < y && y < 16, "0 < y && y < 16"); return _mm_srli_epi16(x, y); }
	template<int y> NUXSIMD_INLINE QInt shiftRight16(QInt x) { static_assert(0 < y && y < 16, "0 < y && y < 16"); return _mm_srai_epi16(x, y); }

	NUXSIMD_INLINE int getSigns8(QInt x) { return _mm_movemask_epi8(x); }

	#endif // (NUXSIMD_SSE2)

#endif // (!NUXSIMD_SIMULATE && NUXSIMD_SSE)

#if (!NUXSIMD_SIMULATE && NUXSIMD_ALTIVEC)

	// WARNING! Only use CONST_QFLOAT with immediate constants, never do 'const float f = 3.0; CONST_QFLOAT(f)' for instance. See http://developer.apple.com/hardwaredrivers/ve/model_details.html for more info.
	#define CONST_QFLOAT(y) (vector float)(y)
	#define CONST_QFLOAT4(y0, y1, y2, y3) (vector float)(y0, y1, y2, y3)

	typedef vector float QFloat;

	NUXSIMD_INLINE QFloat loadAligned(const float y[4]) { assert(isAligned(y)); return vec_ld(0, y); }
	NUXSIMD_INLINE QFloat loadUnaligned(const float y[4]) { return vec_perm(vec_ld(0, y), vec_ld(15, y), vec_lvsl(0, y)); }
	NUXSIMD_INLINE QFloat load4(float y) { AlignedArray<float, 4> a; a[0] = y; return vec_splat(vec_ld(0, &a[0]), 0); }
	NUXSIMD_INLINE QFloat load4(float y0, float y1, float y2, float y3) { return (vector float)(y0, y1, y2, y3); }
	template<int exp> NUXSIMD_INLINE QFloat loadAlignedInts(const int y[4]) { assert(isAligned(y)); assert(0 <= exp && exp < 32); return vec_ctf(vec_ld(0, y), exp); }
	NUXSIMD_INLINE float get1(QFloat x) { AlignedArray<float, 4> a; vec_ste(x, 0, &a[0]); return a[0]; }
	NUXSIMD_INLINE void storeAligned(QFloat x, float y[4]) { assert(isAligned(y)); vec_st(x, 0, y); }
	NUXSIMD_INLINE void streamAligned(QFloat x, float y[4]) { assert(isAligned(y)); vec_st(x, 0, y); }

	NUXSIMD_INLINE void storeUnaligned(QFloat x, float y[4])
	{
		QFloat msq = vec_ld(0, y); // most significant quadword
		QFloat lsq = vec_ld(15, y); // least significant quadword
		vector unsigned char edgeAlign = vec_lvsl(0, y); // permute map to extract edges
		vector unsigned char align = vec_lvsr(0, y); // permute map to misalign data
		QFloat edges = vec_perm(lsq, msq, edgeAlign); // extract the edges
		msq = vec_perm(edges, x, align); // misalign the data (msq)
		lsq = vec_perm(x, edges, align); // misalign the data (LSQ)
		vec_st(lsq, 15, y); // Store the LSQ part first
		vec_st(msq, 0, y); // Store the MSQ part
	}

	template<int exp> NUXSIMD_INLINE void storeAlignedInts(QFloat x, int y[4]) {
		assert(isAligned(y));
		assert(0 <= exp && exp < 32);
		vec_st(vec_cts(x, exp), 0, y);
	}

	NUXSIMD_INLINE void transpose(QFloat& a, QFloat& b, QFloat& c, QFloat& d)
	{
		QFloat transpose1 = vec_mergeh(a, c);
		QFloat transpose2 = vec_mergeh(b, d);
		QFloat transpose3 = vec_mergel(a, c);
		QFloat transpose4 = vec_mergel(b, d);
		a = vec_mergeh(transpose1, transpose2);
		b = vec_mergel(transpose1, transpose2);
		c = vec_mergeh(transpose3, transpose4);
		d = vec_mergel(transpose3, transpose4);
	}

	NUXSIMD_INLINE void interleave(QFloat& x, QFloat& y) {
		QFloat nx = vec_mergeh(x, y);
		y = vec_mergel(x, y);
		x = nx;
	}

	NUXSIMD_INLINE void deinterleave(QFloat& x, QFloat& y) {
		QFloat nx = vec_perm(x, y, (vector unsigned char)(0, 1, 2, 3, 8, 9, 10, 11, 16, 17, 18, 19, 24, 25, 26, 27));
		y = vec_perm(x, y, (vector unsigned char)(4, 5, 6, 7, 12, 13, 14, 15, 20, 21, 22, 23, 28, 29, 30, 31));
		x = nx;
	}

	template<int x0, int x1, int x2, int x3> NUXSIMD_INLINE QFloat shuffle(QFloat x) {
		assert(0 <= x0 && x0 <= 3 && 0 <= x1 && x1 <= 3 && 0 <= x2 && x2 <= 3 && 0 <= x3 && x3 <= 3);
		return vec_perm(x, x, (vector unsigned char)(x0 * 4 + 0, x0 * 4 + 1, x0 * 4 + 2, x0 * 4 + 3, x1 * 4 + 0, x1 * 4 + 1, x1 * 4 + 2, x1 * 4 + 3, x2 * 4 + 0, x2 * 4 + 1, x2 * 4 + 2, x2 * 4 + 3, x3 * 4 + 0, x3 * 4 + 1, x3 * 4 + 2, x3 * 4 + 3));
	}

	template<int x0, int x1, int y2, int y3> NUXSIMD_INLINE QFloat shuffle2(QFloat x, QFloat y) {
		assert(0 <= x0 && x0 <= 3 && 0 <= x1 && x1 <= 3 && 0 <= y2 && y2 <= 3 && 0 <= y3 && y3 <= 3);
		return vec_perm(x, y, (vector unsigned char)(x0 * 4 + 0, x0 * 4 + 1, x0 * 4 + 2, x0 * 4 + 3, x1 * 4 + 0, x1 * 4 + 1, x1 * 4 + 2, x1 * 4 + 3, y2 * 4 + 16, y2 * 4 + 17, y2 * 4 + 18, y2 * 4 + 19, y3 * 4 + 16, y3 * 4 + 17, y3 * 4 + 18, y3 * 4 + 19));
	}

	NUXSIMD_INLINE QFloat add(QFloat x, QFloat y) { return vec_add(x, y); }
	NUXSIMD_INLINE QFloat sub(QFloat x, QFloat y) { return vec_sub(x, y); }
	NUXSIMD_INLINE QFloat mul(QFloat x, QFloat y) { return vec_madd(x, y, (vector float)(0.0f)); }
	NUXSIMD_INLINE QFloat mulAdd(QFloat x, QFloat y, QFloat z) { return vec_madd(x, y, z); }
	NUXSIMD_INLINE QFloat bitAnd(QFloat x, QFloat y) { return (vector float)(vec_and((vector int)(x), (vector int)(y))); }
	NUXSIMD_INLINE QFloat bitAndNot(QFloat x, QFloat y) { return (vector float)(vec_andc((vector int)(x), (vector int)(y))); }
	NUXSIMD_INLINE QFloat bitOr(QFloat x, QFloat y) { return (vector float)(vec_or((vector int)(x), (vector int)(y))); }
	NUXSIMD_INLINE QFloat bitXOr(QFloat x, QFloat y) { return (vector float)(vec_xor((vector int)(x), (vector int)(y))); }
	NUXSIMD_INLINE QFloat minimum(QFloat x, QFloat y) { return vec_min(x, y); }
	NUXSIMD_INLINE QFloat maximum(QFloat x, QFloat y) { return vec_max(x, y); }
	NUXSIMD_INLINE QFloat abs(QFloat x) { return vec_abs(x); }
	NUXSIMD_INLINE QFloat rcpApprox(QFloat x) { return vec_re(x); }
	/* NewtonRaphson Reciprocal [2 * rcpps(x) - (x * rcpps(x) * rcpps(x))] */
	NUXSIMD_INLINE QFloat rcpImproved(QFloat x) { QFloat y = rcpApprox(x); return sub(add(y, y), mul(mul(x, y), y)); }

#endif	// (!NUXSIMD_SIMULATE && NUXSIMD_ALTIVEC)

#if (!NUXSIMD_SIMULATE && NUXSIMD_NEON)
	typedef float32x4_t QFloat;

	#define CONST_QFLOAT(y) vdupq_n_f32(y)
	#define CONST_QFLOAT4(y0, y1, y2, y3) load4(y0, y1, y2, y3)

	NUXSIMD_INLINE QFloat loadAligned(const float y[4]) { assert(isAligned(y)); return vld1q_f32(y); }
	NUXSIMD_INLINE QFloat loadAligned(const QFloat* y) { assert(isAligned(y)); return vld1q_f32(reinterpret_cast<const float*>(y)); }
	NUXSIMD_INLINE QFloat loadUnaligned(const float y[4]) { return vld1q_f32(y); }
	NUXSIMD_INLINE QFloat loadUnaligned(const QFloat* y) { return vld1q_f32(reinterpret_cast<const float*>(y)); }
	NUXSIMD_INLINE QFloat load1(float y) { SIMD_ALIGN(float x[4]) = { y, 0.0f, 0.0f, 0.0f }; return vld1q_f32(x); }
	NUXSIMD_INLINE QFloat load1(QFloat x, QFloat y) { return vsetq_lane_f32(vgetq_lane_f32(y, 0), x, 0); } // moves low order word of y to low order word of x
	NUXSIMD_INLINE QFloat load4(float y0, float y1, float y2, float y3) { SIMD_ALIGN(float x[4]) = { y0, y1, y2, y3 }; return vld1q_f32(x); }
	NUXSIMD_INLINE QFloat load4(float y) { return vdupq_n_f32(y); }

	NUXSIMD_INLINE void storeAligned(QFloat x, float y[4]) { assert(isAligned(y)); vst1q_f32(y, x); }
	NUXSIMD_INLINE void storeUnaligned(QFloat x, float y[4]) { vst1q_f32(y, x); }
	NUXSIMD_INLINE void streamAligned(QFloat x, float y[4]) { assert(isAligned(y)); vst1q_f32(y, x); }
	NUXSIMD_INLINE float get1(QFloat x) { float y; vst1q_lane_f32(&y, x, 0); return y; }
	// FIX : what is most correct?  = { 0, 1, 2, 3 } or vld1q_s32 from array?
	NUXSIMD_INLINE int getSigns(QFloat x) { static const int32x4_t shifts = { 0, 1, 2, 3 }; return vaddvq_u32(vshlq_u32(vshrq_n_u32(x, 31), shifts)); }

	NUXSIMD_INLINE QFloat add(QFloat x, QFloat y) { return vaddq_f32(x, y); }
	NUXSIMD_INLINE QFloat sub(QFloat x, QFloat y) { return vsubq_f32(x, y); }
	NUXSIMD_INLINE QFloat mul(QFloat x, QFloat y) { return vmulq_f32(x, y); }
	NUXSIMD_INLINE QFloat div(QFloat x, QFloat y) { return vdivq_f32(x, y); }
	NUXSIMD_INLINE QFloat minimum(QFloat x, QFloat y) { return vminq_f32(x, y); }
	NUXSIMD_INLINE QFloat maximum(QFloat x, QFloat y) { return vmaxq_f32(x, y); }
	NUXSIMD_INLINE QFloat abs(QFloat x) { return vabsq_f32(x); }
	NUXSIMD_INLINE QFloat mulAdd(QFloat x, QFloat y, QFloat z) { return vfmaq_f32(z, x, y); }
	NUXSIMD_INLINE QFloat bitAnd(QFloat x, QFloat y) { return vandq_s32(x, y); }
	NUXSIMD_INLINE QFloat bitAndNot(QFloat x, QFloat y) { return vbicq_s32(x, y); }
//	NUXSIMD_INLINE QFloat bitAndNot(QFloat x, QFloat y) { return _mm_andnot_ps(y, x); }
//	NUXSIMD_INLINE QFloat bitOr(QFloat x, QFloat y) { return _mm_or_ps(x, y); }
//	NUXSIMD_INLINE QFloat bitXOr(QFloat x, QFloat y) { return _mm_xor_ps(x, y); }

	NUXSIMD_INLINE QFloat less(QFloat x, QFloat y) { return vcltq_f32(x, y); }
	NUXSIMD_INLINE QFloat lessOrEqual(QFloat x, QFloat y) { return vcleq_f32(x, y); }
	NUXSIMD_INLINE QFloat equal(QFloat x, QFloat y) { return vceqq_f32(x, y); }
	NUXSIMD_INLINE QFloat notEqual(QFloat x, QFloat y) { return vmvnq_u32(vceqq_f32(x, y)); }
	NUXSIMD_INLINE QFloat greaterOrEqual(QFloat x, QFloat y) { return vcgeq_f32(x, y); }
	NUXSIMD_INLINE QFloat greater(QFloat x, QFloat y) { return vcgtq_f32(x, y); }

	// FIX : from https://github.com/DLTcollab/sse2neon/blob/master/sse2neon.h, must test this to match SIMD
	NUXSIMD_INLINE QFloat rcpApprox(QFloat x) {
		const float32x4_t recip = vrecpeq_f32(x);
		return vmulq_f32(recip, vrecpsq_f32(recip, x));
	}
	NUXSIMD_INLINE QFloat rcpImproved(QFloat x) { QFloat y = rcpApprox(x); return sub(add(y, y), mul(mul(x, y), y)); }

	NUXSIMD_INLINE QFloat sqrt(QFloat x) { return vsqrtq_f32(x); }

	// FIX : what becomes of this?
	template<int x0, int x1, int x2, int x3> NUXSIMD_INLINE QFloat shuffle(QFloat x) {
		assert(0 <= x0 && x0 <= 3 && 0 <= x1 && x1 <= 3 && 0 <= x2 && x2 <= 3 && 0 <= x3 && x3 <= 3);
		return __builtin_shufflevector(x, x, x0, x1, x2, x3);
	}

	// FIX : from https://github.com/DLTcollab/sse2neon/blob/master/sse2neon.h . is it the best?
	NUXSIMD_INLINE void transpose(QFloat& a, QFloat& b, QFloat& c, QFloat& d) {
		float32x4x2_t row01 = vtrnq_f32(a, b);
		float32x4x2_t row23 = vtrnq_f32(c, d);
		a = vcombine_f32(vget_low_f32(row01.val[0]), vget_low_f32(row23.val[0]));
		b = vcombine_f32(vget_low_f32(row01.val[1]), vget_low_f32(row23.val[1]));
		c = vcombine_f32(vget_high_f32(row01.val[0]), vget_high_f32(row23.val[0]));
		d = vcombine_f32(vget_high_f32(row01.val[1]), vget_high_f32(row23.val[1]));
	}

	typedef int32x4_t QInt;
	#define CONST_QINT(y) vdupq_n_s32(y)
	#define CONST_QINT4(y0, y1, y2, y3) load4(y0, y1, y2, y3)

	NUXSIMD_INLINE QInt load4(int y0, int y1, int y2, int y3) { SIMD_ALIGN(int x[4]) = { y0, y1, y2, y3 }; return vld1q_s32(x); }
	NUXSIMD_INLINE QInt load4(int y) { return vdupq_n_s32(y); }
	NUXSIMD_INLINE QInt load4(unsigned int y) { return vdupq_n_u32(y); }

	NUXSIMD_INLINE QInt loadAligned(const int y[4]) { assert(isAligned(y)); return vld1q_s32(y); }
	NUXSIMD_INLINE QInt loadAligned(const unsigned int y[4]) { assert(isAligned(y)); return vld1q_u32(y); }
	NUXSIMD_INLINE QInt loadAligned(const QInt* y) { assert(isAligned(y)); return vld1q_s32(reinterpret_cast<const int*>(y)); }
	NUXSIMD_INLINE QInt loadUnaligned(const int y[4]) { return vld1q_s32(y); }
	NUXSIMD_INLINE QInt loadUnaligned(const unsigned int y[4]) { return vld1q_u32(y); }
	NUXSIMD_INLINE QInt loadUnaligned(const QInt* y) { return vld1q_s32(reinterpret_cast<const int*>(y)); }
	NUXSIMD_INLINE void storeAligned(QInt x, int y[4]) { assert(isAligned(y)); vst1q_s32(y, x); }
	NUXSIMD_INLINE void storeAligned(QInt x, unsigned int y[4]) { assert(isAligned(y)); vst1q_u32(y, x); }
	NUXSIMD_INLINE void storeAligned(QInt x, QInt* y) { assert(isAligned(y)); vst1q_s32(reinterpret_cast<int*>(y), x); }
	NUXSIMD_INLINE void storeUnaligned(QInt x, int y[4]) { vst1q_s32(y, x); }
	NUXSIMD_INLINE void storeUnaligned(QInt x, unsigned int y[4]) { vst1q_u32(y, x); }
	NUXSIMD_INLINE void storeUnaligned(QInt x, QInt* y) { vst1q_s32(reinterpret_cast<int*>(y), x); }
	NUXSIMD_INLINE void streamAligned(QInt x, int y[4]) { assert(isAligned(y)); vst1q_s32(y, x); }
	NUXSIMD_INLINE void streamAligned(QInt x, unsigned int y[4]) { assert(isAligned(y)); vst1q_u32(y, x); }
	NUXSIMD_INLINE void streamAligned(QInt x, QInt* y) { assert(isAligned(y)); vst1q_s32(reinterpret_cast<int*>(y), x); }

	NUXSIMD_INLINE QInt bitAnd(QInt x, QInt y) { return vandq_s32(x, y); }
	NUXSIMD_INLINE QInt bitAndNot(QInt x, QInt y) { return vbicq_s64(x, y); }
	NUXSIMD_INLINE QInt bitOr(QInt x, QInt y) { return vorrq_s32(x, y); }
	NUXSIMD_INLINE QInt bitXOr(QInt x, QInt y) { return veorq_s32(x, y); }
	template<int y> NUXSIMD_INLINE QInt shiftLeft(QInt x) { static_assert(0 < y && y < 32, "0 < y && y < 32"); return vshlq_n_s32(x, y); }
	template<int y> NUXSIMD_INLINE QInt shiftRight(QInt x) { static_assert(0 < y && y < 32, "0 < y && y < 32"); return vshrq_n_s32(x, y); }
	template<int y> NUXSIMD_INLINE QInt zeroShiftRight(QInt x) { static_assert(0 < y && y < 32, "0 < y && y < 32"); return vshrq_n_u32(x, y); }

	NUXSIMD_INLINE QInt add(QInt x, QInt y) { return vaddq_s32(x, y); }
	NUXSIMD_INLINE QInt sub(QInt x, QInt y) { return vsubq_s32(x, y); }
	NUXSIMD_INLINE QInt add8(QInt x, QInt y) { return vaddq_s8(x, y); }
	NUXSIMD_INLINE QInt sub8(QInt x, QInt y) { return vsubq_s8(x, y); }
	NUXSIMD_INLINE QInt add16(QInt x, QInt y) { return vaddq_s16(x, y); }
	NUXSIMD_INLINE QInt sub16(QInt x, QInt y) { return vsubq_s16(x, y); }
/* FIX : drop, not tested
	NUXSIMD_INLINE QInt mulHighUnsigned16(QInt x, QInt y) {
		const uint16x4_t x3210 = vget_low_u16(x);
		const uint16x4_t y3210 = vget_low_u16(y);
		const uint32x4_t xy3210 = vmull_u16(x3210, y3210);
		const uint32x4_t xy7654 = vmull_high_u16(x, y);
		const uint16x8_t r = vuzp2q_u16(xy3210, xy7654);
		return r;
	}
*/
	NUXSIMD_INLINE QInt mulLow16(QInt x, QInt y) { return vmulq_s16(x, y); }
	template<int y> NUXSIMD_INLINE QInt shiftLeft16(QInt x) { static_assert(0 < y && y < 16, "0 < y && y < 16"); return vshlq_n_s16(x, y); }
	template<int y> NUXSIMD_INLINE QInt shiftRight16(QInt x) { static_assert(0 < y && y < 16, "0 < y && y < 16"); return vshrq_n_s16(x, y); }
	template<int y> NUXSIMD_INLINE QInt zeroShiftRight16(QInt x) { static_assert(0 < y && y < 16, "0 < y && y < 16"); return vshrq_n_u16(x, y); }

	// FIX : can we block the functions that don't exist so we don't implicitly use QFloat?
	
	NUXSIMD_INLINE QInt less(QInt x, QInt y) { return vcltq_s32(x, y); }
	NUXSIMD_INLINE QInt equal(QInt x, QInt y) { return vceqq_s32(x, y); }
	NUXSIMD_INLINE QInt greater(QInt x, QInt y) { return vcgtq_s32(x, y); }
	NUXSIMD_INLINE QInt less16(QInt x, QInt y) { return vcltq_s16(x, y); }
	NUXSIMD_INLINE QInt equal16(QInt x, QInt y) { return vceqq_s16(x, y); }
	NUXSIMD_INLINE QInt greater16(QInt x, QInt y) { return vcgtq_s16(x, y); }
	NUXSIMD_INLINE QInt less8(QInt x, QInt y) { return vcltq_s8(x, y); }
	NUXSIMD_INLINE QInt equal8(QInt x, QInt y) { return vceqq_s8(x, y); }
	NUXSIMD_INLINE QInt greater8(QInt x, QInt y) { return vcgtq_s8(x, y); }

#endif

#if (!NUXSIMD_SIMULATE && !NUXSIMD_NEON && !NUXSIMD_SSE && !NUXSIMD_ALTIVEC)
	#error No SIMD?
#endif

template<bool aligned> QFloat loadAny(const float y[4]) { return aligned ? loadAligned(y) : loadUnaligned(y); }
template<bool aligned> void storeAny(QFloat x, float y[4]) { if (aligned) { storeAligned(x, y); } else { storeUnaligned(x, y); } }

NUXSIMD_INLINE QFloat square(QFloat x) { return mul(x, x); }

NUXSIMD_INLINE float* resetAlignedFloatArray(size_t size, float* const array) throw() {
	assert(size >= 0);
	assert(isAligned(array));
	float* a = array;
	float* e = a + size;
	QFloat z = CONST_QFLOAT(0.0f);
	while (a + 16 <= e) {
		storeAligned(z, a + 0);
		storeAligned(z, a + 4);
		storeAligned(z, a + 8);
		storeAligned(z, a + 12);
		a += 16;
	}
	if ((size & 15) != 0) {
		while (a + 4 <= e) {
			storeAligned(z, a);
			a += 4;
		}
	}
	if ((size & 3) != 0) {
		while (a + 1 <= e) {
			*a = 0;
			++a;
		}
	}
	return array;
}

NUXSIMD_INLINE float* resetFloatArray(size_t size, float* const array) throw()
{
	if (size < 8) {
		for (size_t i = 0; i < size; ++i) {
			array[i] = 0.0f;
		}
		return array;
	} else {
		float* a = array;
		const float* e = reinterpret_cast<float*>((reinterpret_cast<intptr_t>(array) + 15) & ~15);
		size -= (e - a);
		while (a + 1 <= e) {
			*a = 0;
			++a;
		}
		return resetAlignedFloatArray(size, a);
	}
}

template<bool sourceAligned> float* copyFloatArrayWithAlignment(size_t size, float* const destination, const float* const source) throw()
{
	assert(size >= 0);
	assert(isAligned(destination));
	assert(!sourceAligned || isAligned(source));
	
	float* d = destination;
	float* e = d + size;
	const float* s = source;
	while (d + 16 <= e) {
		storeAligned(loadAny<sourceAligned>(s + 0), d + 0);
		storeAligned(loadAny<sourceAligned>(s + 4), d + 4);
		storeAligned(loadAny<sourceAligned>(s + 8), d + 8);
		storeAligned(loadAny<sourceAligned>(s + 12), d + 12);
		d += 16;
		s += 16;
	}
	if ((size & 15) != 0) {
		while (d + 4 <= e) {
			storeAligned(loadAny<sourceAligned>(s), d);
			d += 4;
			s += 4;
		}
	}
	if ((size & 3) != 0) {
		while (d + 1 <= e) {
			*d = *s;
			++d;
			++s;
		}
	}
	return destination;
}

NUXSIMD_INLINE float* copyAlignedFloatArray(size_t size, float* const destination, const float* const source) throw()
{
	return copyFloatArrayWithAlignment<true>(size, destination, source);
}

NUXSIMD_INLINE float* copyFloatArray(size_t size, float* const destination, const float* const source) throw()
{
	if (size < 8) {
		for (size_t i = 0; i < size; ++i) {
			destination[i] = source[i];
		}
		return destination;
	} else {
		float* d = destination;
		const float* s = source;
		const float* e = reinterpret_cast<float*>((reinterpret_cast<intptr_t>(destination) + 15) & ~15);
		size -= (e - d);
		while (d + 1 <= e) {
			*d = *s;
			++d;
			++s;
		}
		if (isAligned(s)) {
			return copyFloatArrayWithAlignment<true>(size, d, s);
		} else {
			return copyFloatArrayWithAlignment<false>(size, d, s);
		}
	}
}

template<bool leftRightAligned> float* addFloatArraysWithAlignment(size_t size, float* const destination, const float* const left, const float* const right) throw()
{
	assert(size >= 0);
	assert(isAligned(destination));
	assert(!leftRightAligned || isAligned(left));
	assert(!leftRightAligned || isAligned(right));
	float* d = destination;
	float* e = d + size;
	const float* l = left;
	const float* r = right;
	while (d + 16 <= e) {
		storeAligned(add(loadAny<leftRightAligned>(l + 0), loadAny<leftRightAligned>(r + 0)), d + 0);
		storeAligned(add(loadAny<leftRightAligned>(l + 4), loadAny<leftRightAligned>(r + 4)), d + 4);
		storeAligned(add(loadAny<leftRightAligned>(l + 8), loadAny<leftRightAligned>(r + 8)), d + 8);
		storeAligned(add(loadAny<leftRightAligned>(l + 12), loadAny<leftRightAligned>(r + 12)), d + 12);
		d += 16;
		l += 16;
		r += 16;
	}
	if ((size & 15) != 0) {
		while (d + 4 <= e) {
			storeAligned(add(loadAny<leftRightAligned>(l), loadAny<leftRightAligned>(r)), d);
			d += 4;
			l += 4;
			r += 4;
		}
	}
	if ((size & 3) != 0) {
		while (d + 1 <= e) {
			*d = *l + *r;
			++d;
			++l;
			++r;
		}
	}
	return destination;
}

NUXSIMD_INLINE float* addAlignedFloatArrays(size_t size, float* const destination, const float* const left, const float* const right) throw()
{
	return addFloatArraysWithAlignment<true>(size, destination, left, right);
}

NUXSIMD_INLINE float* addFloatArrays(size_t size, float* const destination, const float* const left, const float* const right) throw()
{
	if (size < 8) {
		for (size_t i = 0; i < size; ++i) {
			destination[i] = left[i] + right[i];
		}
		return destination;
	} else {
		float* d = destination;
		const float* l = left;
		const float* r = right;
		const float* e = reinterpret_cast<float*>((reinterpret_cast<intptr_t>(destination) + 15) & ~15);
		size -= (e - d);
		while (d + 1 <= e) {
			*d = *l + *r;
			++d;
			++l;
			++r;
		}
		if (isAligned(l) && isAligned(r)) {
			return addFloatArraysWithAlignment<true>(size, d, l, r);
		} else {
			return addFloatArraysWithAlignment<false>(size, d, l, r);
		}
	}
}

NUXSIMD_INLINE float* multiplyFloatArray(size_t count, float* const destination, const float* const left, float right)
{
	for (size_t i = 0; i < count; ++i) {
		destination[i] = left[i] * right;
	}
	return destination;
}

/*
	Like a std::vector that will never reallocate after construction. operator= only works if both arrays are exact
	same size.
*/
class SIMDFloatArray {
	public:		SIMDFloatArray(size_t count) : count(count), elements(allocateAligned<float>(count)) { }
	public:		SIMDFloatArray(const SIMDFloatArray& copy) : count(copy.count)
						, elements(allocateAligned<float>(count)) {
					if (count > 0) {
						std::copy(copy.elements + 0, copy.elements + count, elements);
					}
				}
	public:		SIMDFloatArray(SIMDFloatArray&& move) : count(move.count), elements(move.elements) {
					move.count = 0;
					move.elements = nullptr;
				}
	public:		size_t size() const { return count; }
	public:		float* data() { return elements; }
	public:		const float* data() const { return elements; }
	public:		const float operator[](size_t index) const { assert(index < count); return elements[index]; }
	public:		float& operator[](size_t index) { assert(index < count); return elements[index]; }
	public:		const float operator[](int index) const { assert(0 <= index && index < static_cast<int>(count)); return elements[index]; }
	public:		float& operator[](int index) { assert(0 <= index && index < static_cast<int>(count)); return elements[index]; }
	public:		operator float*() { return elements; }
	public:		operator const float*() const { return elements; }
	public:		SIMDFloatArray& operator=(const SIMDFloatArray& other) {
					if (this != &other) {
						assert(other.count == count);
						if (count > 0) {
							std::copy(other.elements + 0, other.elements + count, elements);
						}
					}
					return (*this);
				}
	public:		SIMDFloatArray& operator=(SIMDFloatArray&& other) {
					assert(other.count == count);
					std::swap(elements, other.elements);
					return (*this);
				}
	public:		~SIMDFloatArray() { freeAligned(elements); }
	protected:	size_t count;
	protected:	float* elements;
};

} // namespace NuXSIMD

#endif // NuXSIMD_h
