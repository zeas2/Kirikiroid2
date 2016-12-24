/******************************************************************************/
/**
 * 拡大縮小内部使用ヘッダ
 * ----------------------------------------------------------------------------
 * 	Copyright (C) T.Imoto <http://www.kaede-software.com>
 * ----------------------------------------------------------------------------
 * @author		T.Imoto
 * @date		2014/04/04
 * @note
 *****************************************************************************/


#ifndef __RESAMPLE_IMAGE_INTERNAL_H__
#define __RESAMPLE_IMAGE_INTERNAL_H__

struct tTVPImageCopyFuncBase {
	virtual ~tTVPImageCopyFuncBase() {}
	virtual void operator() ( tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len ) const = 0;
};
struct tTVPBlendImageFunc : public tTVPImageCopyFuncBase {
	tjs_int opa_;
	void (*blend_func_)(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa);

	tTVPBlendImageFunc( tjs_int opa, void (*blend_func)(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa) )
	: opa_(opa), blend_func_(blend_func) {}

	virtual void operator() ( tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len ) const {
		blend_func_( dest, src, len, opa_ );
	}
};
struct tTVPCopyImageFunc : public tTVPImageCopyFuncBase {
	void (*copy_func_)(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len);

	tTVPCopyImageFunc( void (*copy_func)(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len) )
	: copy_func_(copy_func) {}

	virtual void operator() ( tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len ) const {
		copy_func_( dest, src, len );
	}
};
/**
 * ブレンドパラメータ
 * ブレンド関数ポインタは拡大縮小等考慮しないもの
 */
struct tTVPBlendParameter {
	/** ブレンド方式 */
	tTVPBBBltMethod method_;
	/** 不透明度 */
	tjs_int opa_;
	/** 転送先アルファ保持有無 */
	bool hda_;
	/** 不透明度を考慮してブレンドする */
	void (*blend_func)(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa);
	/** コピーする(ソースやデスティネーションのアルファが考慮される場合もあり) */
	void (*copy_func)(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len);

	tTVPBlendParameter() : method_(bmCopy), opa_(255), hda_(false), blend_func(NULL), copy_func(NULL) {}
	tTVPBlendParameter( tTVPBBBltMethod method, tjs_int opa, bool hda ) : method_(method), opa_(opa), hda_(hda), blend_func(NULL), copy_func(NULL) {}

	/**
	 * パラメータを元に関数ポインタを決定する
	 */
	void setFunctionFromParam();
};
/**
 * クリッピングパラメータ
 */
struct tTVPResampleClipping {
	tjs_int offsetx_;	// クリッピングされる左端量
	tjs_int offsety_;	// クリッピングされる上端量
	tjs_int width_;		// コピー幅、offsetx_も含んでいる
	tjs_int height_;	// コピー高さ、offsety_も含んでいる
	tjs_int dst_left_;	// 実際のコピー先左端
	tjs_int dst_top_;	// 実際のコピー先上端

	void setClipping( const tTVPRect &cliprect, const tTVPRect &destrect );

	/** 実際にコピーされる幅 */
	inline tjs_int getDestWidth() const { return width_ - offsetx_; }
	/** 実際にコピーされる高さ */
	inline tjs_int getDestHeight() const { return height_ - offsety_; }
};


/**
 * 面積平均パラメータ用
 */
template<typename TVector>
void TVPCalculateAxisAreaAvg( int srcstart, int srcend, int srclength, int dstlength, std::vector<int>& start, std::vector<int>& length, TVector& weight ) {
	start.clear();
	start.reserve( dstlength );
	length.clear();
	length.reserve( dstlength );
	weight.clear();
	int wlength = srclength + dstlength;
	weight.reserve( wlength );
	int delta = 0;
	int srctarget = srclength;
	int len = 0;
	start.push_back( 0 );
	for( int x = 0; x < srclength; x++ ) {
		if( (delta + dstlength) <= srctarget ) {	// 境界に達していない
			weight.push_back( 1.0f );
			len++;
		} else { // 境界をまたいだ
			int d = (delta + dstlength) - srctarget;
			weight.push_back( (float)(dstlength - d) / (float)dstlength ); // 前の領域
			length.push_back( len+1 );

			start.push_back( x );
			len = 1;
			weight.push_back( (float)d / (float)dstlength );
			srctarget += srclength;
		}
		delta += dstlength;
	}
	length.push_back( len );
}

/**
 * 面積平均パラメータ正規化用
 */
template<typename TVector>
void TVPNormalizeAxisAreaAvg( std::vector<int>& length, TVector& weight ) {
	// 正規化
	const int count = (const int)length.size();
	float* wstart = &weight[0];
	for( int i = 0; i < count; i++ ) {
		int len = length[i];
		// 合計値を求める
		float* w = wstart;
		float sum = 0.0f;
		for( int j = 0; j < len; j++ ) {
			sum += *w;
			w++;
		}

		// EPSILON より小さい場合は 0 を設定
		float rcp;
		if( sum < FLT_EPSILON ) {
			rcp = 0.0f;
		} else {
			rcp = 1.0f / sum;
		}
		// 正規化
		w = wstart;
		for( int i = 0; i < len; i++ ) {
			*w *= rcp;
			w++;
		}
		wstart = w;
	}
}

#endif // __RESAMPLE_IMAGE_INTERNAL_H__
