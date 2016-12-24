//---------------------------------------------------------------------------
/*
	Risa [りさ]      alias 吉里吉里3 [kirikiri-3]
	 stands for "Risa Is a Stagecraft Architecture"
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
//! @file
//! @brief 数学関数群
//---------------------------------------------------------------------------
#include <stdlib.h>

//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
void DeinterleaveApplyingWindow(float *  dest[], const float *  src,
					float *  win, int numch, size_t destofs, size_t len)
{
	size_t n;
	switch(numch)
	{
	case 1: // mono
		{
			float * dest0 = dest[0] + destofs;
			for(n = 0; n < len; n++)
			{
				dest0[n] = src[n] * win[n];
			}
		}
		break;

	case 2: // stereo
		{
			float * dest0 = dest[0] + destofs;
			float * dest1 = dest[1] + destofs;
			for(n = 0; n < len; n++)
			{
				dest0[n] = src[n*2 + 0] * win[n];
				dest1[n] = src[n*2 + 1] * win[n];
			}
		}
		break;

	default: // generic
		for(n = 0; n < len; n++)
		{
			for(int ch = 0; ch < numch; ch++)
			{
				dest[ch][n + destofs] = *src * win[n];
				src ++;
			}
		}
		break;
	}
}
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
void  InterleaveOverlappingWindow(float *  dest,
	const float *  const *  src,
	float *  win, int numch, size_t srcofs, size_t len)
{
	size_t n;
	switch(numch)
	{
	case 1: // mono
		{
			const float * src0 = src[0] + srcofs;
			for(n = 0; n < len; n++)
			{
				dest[n] += src0[n] * win[n];
			}
		}
		break;

	case 2: // stereo
		{
			const float * src0 = src[0] + srcofs;
			const float * src1 = src[1] + srcofs;
			for(n = 0; n < len; n++)
			{
				dest[n*2 + 0] += src0[n] * win[n];
				dest[n*2 + 1] += src1[n] * win[n];
			}
		}
		break;

	default: // generic
		for(n = 0; n < len; n++)
		{
			for(int ch = 0; ch < numch; ch++)
			{
				*dest += src[ch][n + srcofs] * win[n];
				dest ++;
			}
		}
		break;
	}
}
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------

