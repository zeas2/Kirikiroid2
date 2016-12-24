//---------------------------------------------------------------------------
/*
	Risa [りさ]      alias 吉里吉里3 [kirikiri-3]
	 stands for "Risa Is a Stagecraft Architecture"
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
//! @file
//! @brief Waveフォーマットコンバータのコア関数
//---------------------------------------------------------------------------

#include "tjsCommHead.h"
#include "MathAlgorithms.h"
#if defined(_M_IX86)||defined(_M_X64)
//---------------------------------------------------------------------------

_ALIGN16(const float) TJS_V_VEC_REDUCE[4] =
	{ 1.0f/32767.0f, 1.0f/32767.0f, 1.0f/32767.0f, 1.0f/32767.0f };
_ALIGN16(const float) TJS_V_VEC_MAGNIFY[4] =
	{ 32767.0f, 32767.0f, 32767.0f, 32767.0f };

static inline void Int16ToFloat32_sse2( float * d, const tjs_int16 * s ) {
	__m128i src = _mm_loadu_si128((__m128i const*)s);
	__m128i ext_val = _mm_cmpgt_epi16(_mm_setzero_si128(), src);
	__m128 mlo = _mm_cvtepi32_ps(_mm_unpacklo_epi16(src, ext_val) );
	__m128 mhi = _mm_cvtepi32_ps(_mm_unpackhi_epi16(src, ext_val) );
	_mm_store_ps(d+0, _mm_mul_ps(mlo,PM128(TJS_V_VEC_REDUCE)) );
	_mm_store_ps(d+4, _mm_mul_ps(mhi,PM128(TJS_V_VEC_REDUCE)) );
}
static inline void Int16ToFloat32_sse41( float * d, const tjs_int16 * s ) {
	__m128 mlo = _mm_cvtepi32_ps(_mm_cvtepi16_epi32( _mm_loadl_epi64( (__m128i const*)(s+0) ) ) );
	__m128 mhi = _mm_cvtepi32_ps(_mm_cvtepi16_epi32( _mm_loadl_epi64( (__m128i const*)(s+4) ) ));
	_mm_store_ps(d+0, _mm_mul_ps(mlo,PM128(TJS_V_VEC_REDUCE)) );
	_mm_store_ps(d+4, _mm_mul_ps(mhi,PM128(TJS_V_VEC_REDUCE)) );
}
static inline void Float32ToInt16_sse2( tjs_uint16 * d, const float * s ) {
	__m128i mlo = _mm_cvtps_epi32( _mm_mul_ps( *(__m128*)(s + 0), PM128(TJS_V_VEC_MAGNIFY) ) );
	__m128i mhi = _mm_cvtps_epi32( _mm_mul_ps( *(__m128*)(s + 4), PM128(TJS_V_VEC_MAGNIFY) ) );
	_mm_store_si128( (__m128i *)d, _mm_packs_epi32(mlo, mlo) );
}
#ifdef TJS_64BIT_OS
// 64bit の時は MMX を使わず、SSE2/SSE で処理
#define Int16ToFloat32_sse Int16ToFloat32_sse2
#define Float32ToInt16_sse Float32ToInt16_sse2
#else
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4799)	// ignore _mm_empty request.
#endif
static inline __m128 _mm64_cvtpi16_ps(__m64 a) {
  __m128 tmp;
  __m64  ext_val = _mm_cmpgt_pi16(_mm_setzero_si64(), a);
  tmp = _mm_cvtpi32_ps(_mm_setzero_ps(), _mm_unpackhi_pi16(a, ext_val));
  return(_mm_cvtpi32_ps(_mm_movelh_ps(tmp, tmp), 
                        _mm_unpacklo_pi16(a, ext_val)));
}
static inline void Int16ToFloat32_sse( float * d, const tjs_int16 * s ) {
	*(__m128*)(d + 0) = _mm_mul_ps(_mm64_cvtpi16_ps(*(__m64*)(s+0)), PM128(TJS_V_VEC_REDUCE));
	*(__m128*)(d + 4) = _mm_mul_ps(_mm64_cvtpi16_ps(*(__m64*)(s+4)), PM128(TJS_V_VEC_REDUCE));
}
static inline void Float32ToInt16_sse( tjs_uint16 * d, const float * s ) {
	*(__m64*)(d + 0) = _mm_cvtps_pi16( _mm_mul_ps(*(__m128*)(s + 0), PM128(TJS_V_VEC_MAGNIFY)) );
	*(__m64*)(d + 4) = _mm_cvtps_pi16( _mm_mul_ps(*(__m128*)(s + 4), PM128(TJS_V_VEC_MAGNIFY)) );
}
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#endif

//---------------------------------------------------------------------------
/**
 * int16→float32変換
 */
void PCMConvertLoopInt16ToFloat32_sse(void * __restrict dest, const void * __restrict src, size_t numsamples)
{
	float * d = static_cast<float*>(dest);
	const tjs_int16 * s = static_cast<const tjs_int16*>(src);
	size_t n;

	// d がアラインメントされるまで一つずつ処理をする
	for(n = 0  ; n < numsamples && !IsAlignedTo128bits(d+n); n ++)
	{
		d[n] = s[n] * (1.0f/32767.0f);
	}

	// メインの部分
	if(numsamples >= 8)
	{
		for(       ; n < numsamples - 7; n += 8)
		{
			Int16ToFloat32_sse( d+n, s+n );
		}
#ifndef _M_X64
		_mm_empty();
#endif
	}

	// のこり
	for(       ; n < numsamples; n ++)
	{
		d[n] = s[n] * (1.0f/32767.0f);
	}
}

//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
/**
 * float32→int16変換
 */
void PCMConvertLoopFloat32ToInt16_sse(void * __restrict dest, const void * __restrict src, size_t numsamples)
{
	tjs_uint16 * d = reinterpret_cast<tjs_uint16*>(dest);
	const float * s = reinterpret_cast<const float*>(src);
	size_t n;

	// s がアラインメントされるまで一つずつ処理をする
	for(n = 0; n < numsamples && !IsAlignedTo128bits(s+n); n ++)
	{
		float v = (float)(s[n] * 32767.0);
		d[n] = 
			 v > (float) 32767 ?  32767 :
			 v < (float)-32768 ? -32768 :
			 	v < 0 ? (tjs_int16)(v - 0.5) : (tjs_int16)(v + 0.5);
	}

	// メインの部分
	if(numsamples >= 8)
	{
		SetRoundingModeToNearest_SSE();
		for(     ; n < numsamples - 7; n += 8)
		{
			Float32ToInt16_sse( d+n, s+n );
		}
#ifndef _M_X64
		_mm_empty();
#endif
	}

	// のこり
	for(     ; n < numsamples; n ++)
	{
		float v = (float)(s[n] * 32767.0);
		d[n] = 
			 v > (float) 32767 ?  32767 :
			 v < (float)-32768 ? -32768 :
			 	v < 0 ? (tjs_int16)(v - 0.5) : (tjs_int16)(v + 0.5);
	}
}
//---------------------------------------------------------------------------
#endif
