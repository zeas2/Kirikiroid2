/*

	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000-2009 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"


*/
/******************************************************************************/
/**
 * ブレンド処理を C のみで処理する場合のユーティリティクラス
 *****************************************************************************/


#ifndef __BLEND_UTIL_FUNC_H__
#define __BLEND_UTIL_FUNC_H__

/** 飽和加算演算(8bit) */
struct saturated_u8_add_func {
	inline tjs_uint32 operator()( tjs_uint32 a, tjs_uint32 b ) const {
		/* Add each byte of packed 8bit values in two 32bit uint32, with saturation. */
		tjs_uint32 tmp = (  ( a & b ) + ( ((a ^ b)>>1) & 0x7f7f7f7f)  ) & 0x80808080;
		tmp = (tmp<<1) - (tmp>>7);
		return (a + b - tmp) | tmp;
	}
};
/**
 * add_aldpha_blend_dest_src[_o]
 * dest/src    :    a(additive-alpha)  d(alpha)  n(none alpha)
 * _o          :    with opacity
 */
struct premulalpha_blend_n_a_func {	// TVPAddAlphaBlend_n_a
	saturated_u8_add_func sat_add_;
	inline tjs_uint32 operator()( tjs_uint32 d, tjs_uint32 s ) const {
		tjs_uint32 sopa = (~s) >> 24;
		return sat_add_( (((d & 0xff00ff)*sopa >> 8) & 0xff00ff) + 
			(((d & 0xff00)*sopa >> 8) & 0xff00), s);
	}
};
/* struct add_alpha_blend_hda_n_a_func {	// TVPAddAlphaBlend_HDA_n_a
	add_aldpha_blend_n_a_func add_alpha_;
	inline tjs_uint32 operator()( tjs_uint32 d, tjs_uint32 s ) const {
		return (d & 0xff000000) + (add_alpha_(d, s) & 0xffffff);
	}
};*/
/* struct add_aldpha_blend_n_a_o_func {	// TVPAddAlphaBlend_n_a_o
	add_aldpha_blend_n_a_func add_alpha_;
	inline tjs_uint32 operator()( tjs_uint32 d, tjs_uint32 s, tjs_int opa ) const {
		s = (((s & 0xff00ff)*opa >> 8) & 0xff00ff) + (((s >> 8) & 0xff00ff)*opa & 0xff00ff00);
		return add_alpha_(d, s);
	}
};*/
/* struct add_alpha_blend_hda_n_a_o_func {	// TVPAddAlphaBlend_HDA_n_a_o
	add_aldpha_blend_n_a_o_func add_alpha_;
	inline tjs_uint32 operator()( tjs_uint32 d, tjs_uint32 s, tjs_int opa ) const {
		return (d & 0xff000000) + (add_alpha_(d, s, opa) & 0xffffff);
	}
};*/

/** アルファ乗算済みカラー同士のブレンド
 *
 * Di = sat(Si, (1-Sa)*Di)
 * Da = Sa + Da - SaDa
 */
struct premulalpha_blend_a_a_func {	// TVPAddAlphaBlend_a_a
	saturated_u8_add_func sat_add_;
	inline tjs_uint32 operator()( tjs_uint32 d, tjs_uint32 s ) const {
		tjs_uint32 da = d >> 24;
		tjs_uint32 sa = s >> 24;
		da = da + sa - (da*sa >> 8);
		da -= (da >> 8); /* adjust alpha */
		sa ^= 0xff;
		s &= 0xffffff;
		return (da << 24) + sat_add_((((d & 0xff00ff)*sa >> 8) & 0xff00ff) + (((d & 0xff00)*sa >> 8) & 0xff00), s);
	}
};
struct premulalpha_blend_a_a_o_func {	// TVPAddAlphaBlend_a_a_o
	premulalpha_blend_a_a_func func_;
	inline tjs_uint32 operator()( tjs_uint32 d, tjs_uint32 s, tjs_int opa ) const {
		s = (((s & 0xff00ff)*opa >> 8) & 0xff00ff) + (((s >> 8) & 0xff00ff)*opa & 0xff00ff00);
		return func_(d, s);
	}
};

static inline tjs_uint32 mul_color( tjs_uint32 color, tjs_uint32 fac ) {
	return (((((color & 0x00ff00) * fac) & 0x00ff0000) +
			(((color & 0xff00ff) * fac) & 0xff00ff00) ) >> 8);
}
static inline tjs_uint32 alpha_and_color_to_additive_alpha( tjs_uint32 alpha, tjs_uint32 color ) {
	return mul_color(color, alpha) + (color & 0xff000000);
}
static inline tjs_uint32 alpha_to_additive_alpha( tjs_uint32 a ) {
	return alpha_and_color_to_additive_alpha( a >> 24, a );
}
struct premulalpha_blend_a_d_func {	// TVPAddAlphaBlend_a_d
	premulalpha_blend_a_a_func func_;
	inline tjs_uint32 operator()( tjs_uint32 d, tjs_uint32 s ) const {
		return func_(d, alpha_to_additive_alpha(s));
	}
};
struct premulalpha_blend_a_d_o_func {	// TVPAddAlphaBlend_a_d_o
	premulalpha_blend_a_d_func func_;
	inline tjs_uint32 operator()( tjs_uint32 d, tjs_uint32 s, tjs_int opa ) const {
		s = (s & 0xffffff) + ((((s >> 24) * opa) >> 8) << 24);
		return func_(d, s);
	}
};
/*
	Di = sat(Si, (1-Sa)*Di)
	Da = Sa + Da - SaDa
*/
struct premulalpha_blend_a_ca_func {	// = TVPAddAlphaBlend_a_ca
	saturated_u8_add_func sat_add_;
	inline tjs_uint32 operator()( tjs_uint32 dest, tjs_uint32 sopa, tjs_uint32 sopa_inv, tjs_uint32 src ) const {
		tjs_uint32 dopa = dest >> 24;
		dopa = dopa + sopa - (dopa*sopa >> 8);
		dopa -= (dopa >> 8); /* adjust alpha */
		return (dopa << 24) + sat_add_((((dest & 0xff00ff)*sopa_inv >> 8) & 0xff00ff) + (((dest & 0xff00)*sopa_inv >> 8) & 0xff00), src);
	}
};


//------------------------------------------------------------------------------
// カラー成分にfacをかける
struct mul_color_func {
	inline tjs_uint32 operator()(tjs_uint32 color, tjs_uint32 fac) const {
		return (((((color & 0x00ff00) * fac) & 0x00ff0000) + (((color & 0xff00ff) * fac) & 0xff00ff00) ) >> 8);
	}
};
//------------------------------------------------------------------------------
// カラー成分にアルファをかける、アルファは維持する
struct alpha_and_color_to_premulalpha_func {
	mul_color_func func_;
	inline tjs_uint32 operator()( tjs_uint32 alpha, tjs_uint32 color ) const {
		return func_( color, alpha ) + (color & 0xff000000);
	}
};
//------------------------------------------------------------------------------
// 通常のアルファを持つ色からプレマルチプライド(乗算済み)アルファ形式の色に変換する
struct alpha_to_premulalpha_func {
	alpha_and_color_to_premulalpha_func func_;
	inline tjs_uint32 operator()(tjs_uint32 a) const {
		return func_( a >> 24, a );
	}
};
//------------------------------------------------------------------------------
/* returns a * ratio + b * (1 - ratio) */
struct blend_argb {
	inline tjs_uint32 operator()(tjs_uint32 b, tjs_uint32 a, tjs_int ratio ) const {
		tjs_uint32 b2 = b & 0x00ff00ff;
		tjs_uint32 t = (b2 + (((a & 0x00ff00ff) - b2) * ratio >> 8)) & 0x00ff00ff;
		b2 = (b & 0xff00ff00) >> 8;
		return t +  (((b2 + (( ((a & 0xff00ff00) >> 8) - b2) * ratio >> 8)) << 8)& 0xff00ff00);
	}
};
//------------------------------------------------------------------------------

#endif // __BLEND_UTIL_FUNC_H__
