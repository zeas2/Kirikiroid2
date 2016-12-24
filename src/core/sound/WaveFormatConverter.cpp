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


//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
/**
 * int16→float32変換
 */
static void PCMConvertLoopInt16ToFloat32_c(void * dest, const void * src, size_t numsamples)
{
	float * d = static_cast<float*>(dest);
	const tjs_int16 * s = static_cast<const tjs_int16*>(src);
	const tjs_int16 * s_lim = s + numsamples;

	while(s < s_lim)
	{
		*d = *s * (1.0f/32768.0f);
		d += 1; s += 1;
	}
}
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
/**
 * float32→int16変換
 */
static void PCMConvertLoopFloat32ToInt16_c(void * dest, const void * src, size_t numsamples)
{
	tjs_uint16 * d = static_cast<tjs_uint16*>(dest);
	const float * s = static_cast<const float*>(src);
	const float * s_lim = s + numsamples;

	while(s < s_lim)
	{
		float v = *s * 32767.0f;
		*d = 
			 v > (float) 32767 ?  32767 :
			 v < (float)-32768 ? -32768 :
			 	v < 0 ? (tjs_int16)(v - 0.5) : (tjs_int16)(v + 0.5);
		d += 1; s += 1;
	}
}
//---------------------------------------------------------------------------

void(*PCMConvertLoopInt16ToFloat32)(void * dest, const void * src, size_t numsamples) = PCMConvertLoopInt16ToFloat32_c;
void(*PCMConvertLoopFloat32ToInt16)(void * dest, const void * src, size_t numsamples) = PCMConvertLoopFloat32ToInt16_c;

//---------------------------------------------------------------------------


