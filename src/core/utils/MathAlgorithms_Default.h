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
#ifndef MATHALGORITHMSH
#define MATHALGORITHMSH

#include <stdlib.h>


//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
/**
 * 窓関数を適用しながらのインターリーブ解除
 * @param dest		格納先(複数)
 * @param src		ソース
 * @param win		窓関数
 * @param numch		チャンネル数
 * @param destofs	destの処理開始位置
 * @param len		処理するサンプル数
 *					(各チャンネルごとの数; 実際に処理されるサンプル
 *					数の総計はlen*numchになる)
 */
void DeinterleaveApplyingWindow(float *  dest[], const float *  src,
					float *  win, int numch, size_t destofs, size_t len);
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
/**
 * 窓関数を適用しながらのインターリーブ+オーバーラッピング
 * @param dest		格納先
 * @param src		ソース(複数)
 * @param win		窓関数
 * @param numch		チャンネル数
 * @param srcofs	srcの処理開始位置
 * @param len		処理するサンプル数
 *					(各チャンネルごとの数; 実際に処理されるサンプル
 *					数の総計はlen*numchになる)
 */
void  InterleaveOverlappingWindow(float *  dest,
	const float *  const *  src,
	float *  win, int numch, size_t srcofs, size_t len);
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------



#endif

