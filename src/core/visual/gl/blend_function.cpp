/*

	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000-2009 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"


*/
/******************************************************************************/
/**
 * テンプレートベースのGL
 * @note		従来の TVP GL を置き換える
 *****************************************************************************/

#define TVPPS_USE_OVERLAY_TABLE

#include <math.h>
#include <string.h>

#include "tjsTypes.h"
#include "tvpgl.h"

#include "tjsTypes.h"
#include "tvpgl.h"
//#include "tvpgl_ia32_intf.h"
#include "tvpgl_mathutil.h"

extern "C" {
extern unsigned char TVPDivTable[256*256];
extern unsigned char TVPOpacityOnOpacityTable[256*256];
extern unsigned char TVPNegativeMulTable[256*256];
extern unsigned char TVPOpacityOnOpacityTable65[65*256];
extern unsigned char TVPNegativeMulTable65[65*256];
extern unsigned char TVPDitherTable_5_6[8][4][2][256];
extern unsigned char TVPDitherTable_676[3][4][4][256];
extern unsigned char TVP252DitherPalette[3][256];
extern tjs_uint32 TVPRecipTable256[256];
extern tjs_uint16 TVPRecipTable256_16[256];
};

#include "blend_functor_c.h"


unsigned char ps_soft_light_table::TABLE[256][256];
unsigned char ps_color_dodge_table::TABLE[256][256];
unsigned char ps_color_burn_table::TABLE[256][256];
#ifdef TVPPS_USE_OVERLAY_TABLE
unsigned char ps_overlay_table::TABLE[256][256];
#endif
// 変換する
template<typename functor>
static inline void convert_func_c( tjs_uint32 *dest, tjs_int len ) {
	functor func;
	for( int i = 0; i < len; i++ ) {
		dest[i] = func( dest[i] );
	}
}
template<typename functor>
static inline void convert_func_c( tjs_uint32 *dest, tjs_int len, const functor& func ) {
	for( int i = 0; i < len; i++ ) {
		dest[i] = func( dest[i] );
	}
}

// ブレンドしつつコピー
template<typename functor>
static inline void copy_func_c( tjs_uint32 * __restrict dest, const tjs_uint32 * __restrict src, tjs_int len ) {
	functor func;
	for( int i = 0; i < len; i++ ) {
		dest[i] = func( dest[i], src[i] );
	}
}
template<typename functor>
static inline void blend_func_c( tjs_uint32 * __restrict dest, const tjs_uint32 * __restrict src, tjs_int len, const functor& func ) {
	for( int i = 0; i < len; i++ ) {
		dest[i] = func( dest[i], src[i] );
	}
}
// 8bit版
template<typename functor>
static inline void blend_func_c( tjs_uint8 * __restrict dest, const tjs_uint8 * __restrict src, tjs_int len, const functor& func ) {
	for( int i = 0; i < len; i++ ) {
		dest[i] = func( dest[i], src[i] );
	}
}
// src と dest が重複している可能性のあるもの
template<typename functor>
static inline void overlap_blend_func_c( tjs_uint32 * dest, const tjs_uint32 * src, tjs_int len, const functor& func ) {
	const tjs_uint32 *src_end = src + len;
	if( dest > src && dest < src_end ) {
		// backward オーバーラップするので後ろからコピー
		len--;
		while( len >= 0 ) {
			dest[len] = func( dest[len], src[len] );
			len--;
		}
	} else {
		// forward
		blend_func_c<functor>( dest, src, len, func );
	}
}
template<typename functor>
static void overlap_copy_func_c( tjs_uint32 * dest, const tjs_uint32 * src, tjs_int len ) {
	functor func;
	overlap_blend_func_c<functor>( dest, src, len, func );
}
// dest = src1 * src2 となっているもの
template<typename functor>
static inline void sd_blend_func_c( tjs_uint32 *__restrict dest, const tjs_uint32 * __restrict src1, const tjs_uint32 * __restrict src2, tjs_int len, const functor& func ) {
	for( int i = 0; i < len; i++ ) {
		dest[i] = func( src1[i], src2[i] );
	}
}
// 主に文字描画時にアルファ値と色を使って描画を行う
template<typename functor>
static inline void apply_color_map_o_func_c( tjs_uint32 * __restrict dest, const tjs_uint8 * __restrict src, tjs_int len, tjs_uint32 color, tjs_int opa ) {
	functor func(color,opa);
	for( int i = 0; i < len; i++ ) {
		dest[i] = func( dest[i], src[i] );
	}
}
template<typename functor>
static inline void apply_color_map_func_c( tjs_uint32 * __restrict dest, const tjs_uint8 * __restrict src, tjs_int len, tjs_uint32 color ) {
	functor func(color);
	for( int i = 0; i < len; i++ ) {
		dest[i] = func( dest[i], src[i] );
	}
}

template<typename functor>
static inline void alpha_convert_func_c( tjs_uint32 * __restrict dest, const tjs_uint8 * __restrict src, tjs_int len ) {
	functor func;
	for( int i = 0; i < len; i++ ) {
		dest[i] = func( dest[i], src[i] );
	}
}
template<typename functor>
static inline void alpha_convert_func_c( tjs_uint32 * __restrict dest, const tjs_uint8 * __restrict src, tjs_int len, const functor& func ) {
	for( int i = 0; i < len; i++ ) {
		dest[i] = func( dest[i], src[i] );
	}
}


#define DEFINE_CONVERT_FUNCTION( FUNC ) \
static void TVP_##FUNC( tjs_uint32 *dest, tjs_int len ) {	\
	convert_func_c<FUNC##_functor>( dest, len );			\
}
#define DEFINE_CONVERT_BLEND_FUNCTION( FUNC ) \
static void TVP_##FUNC( tjs_uint32 *dest, tjs_int len, tjs_int opa ) {	\
	FUNC##_functor func(opa);												\
	convert_func_c<FUNC##_functor>( dest, len, func );						\
}
#define DEFINE_ALPHA_COPY_FUNCTION( FUNC ) \
static void TVP_##FUNC( tjs_uint32 *dest, const tjs_uint8 *src, tjs_int len ) {	\
	alpha_convert_func_c<FUNC##_functor>( dest, src, len );							\
}
#define DEFINE_ALPHA_BLEND_FUNCTION( FUNC ) \
static void TVP_##FUNC( tjs_uint32 *dest, const tjs_uint8 *src, tjs_int len, tjs_int opa ) {	\
	FUNC##_functor func(opa);																	\
	alpha_convert_func_c( dest, src, len, func );												\
}

#define DEFINE_BLEND_FUNCTION_VARIATION( FUNC ) \
static void TVP_##FUNC( tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len ) {						\
	copy_func_c<FUNC##_functor>( dest, src, len );														\
}																										\
static void TVP_##FUNC##_HDA( tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len ) {					\
	copy_func_c<FUNC##_HDA_functor>( dest, src, len );													\
}																										\
static void TVP_##FUNC##_o( tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa ) {		\
	FUNC##_o_functor func(opa);																			\
	blend_func_c( dest, src, len, func );																\
}																										\
static void TVP_##FUNC##_HDA_o( tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa ) {	\
	FUNC##_HDA_o_functor func(opa);																		\
	blend_func_c( dest, src, len, func );																\
}																										\
static void TVP_##FUNC##_d( tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len ) {					\
	copy_func_c<FUNC##_d_functor>( dest, src, len );													\
}																										\
static void TVP_##FUNC##_a( tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len ) {					\
	copy_func_c<FUNC##_a_functor>( dest, src, len );													\
}																										\
static void TVP_##FUNC##_do( tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa ) {		\
	FUNC##_do_functor func(opa);																		\
	blend_func_c( dest, src, len, func );																\
}																										\
static void TVP_##FUNC##_ao( tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa ) {		\
	FUNC##_ao_functor func(opa);																		\
	blend_func_c( dest, src, len, func );																\
}

#define DEFINE_BLEND_FUNCTION_MIN_VARIATION( FUNC ) \
static void TVP_##FUNC( tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len ) {						\
	copy_func_c<FUNC##_functor>( dest, src, len );														\
}																										\
static void TVP_##FUNC##_HDA( tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len ) {					\
	copy_func_c<FUNC##_HDA_functor>( dest, src, len );													\
}																										\
static void TVP_##FUNC##_o( tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa ) {		\
	FUNC##_o_functor func(opa);																			\
	blend_func_c( dest, src, len, func );																\
}																										\
static void TVP_##FUNC##_HDA_o( tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa ) {	\
	FUNC##_HDA_o_functor func(opa);																		\
	blend_func_c( dest, src, len, func );																\
}																										\

#define DEFINE_SD_BLEND_FUNCTION( FUNC )																					\
static void TVP_##FUNC##_SD(tjs_uint32 *dest, const tjs_uint32 *src1, const tjs_uint32 *src2, tjs_int len, tjs_int opa){	\
	FUNC##_functor func(opa);																								\
	sd_blend_func_c( dest, src1, src2, len, func );																			\
}
#define DEFINE_OVERLAP_COPY( FUNC ) \
static void TVP_##FUNC( tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len ) {	\
	overlap_copy_func_c<FUNC##_functor>( dest, src, len );							\
}
#define DEFINE_COPY_FUNCTION( FUNC ) \
static void TVP_##FUNC( tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len ) {	\
	copy_func_c<FUNC##_functor>( dest, src, len );									\
}
#define DEFINE_BLEND_FUNCTION( FUNC ) \
static void TVP_##FUNC( tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa ) {	\
	FUNC##_functor func(opa);																	\
	blend_func_c( dest, src, len, func );														\
}
#define DEFINE_BLEND8_FUNCTION( FUNC ) \
static void TVP_##FUNC( tjs_uint8 *dest, const tjs_uint8 *src, tjs_int len, tjs_int opa ) {	\
	FUNC##_functor func(opa);																	\
	blend_func_c( dest, src, len, func );														\
}
// 以下2つはダミー
struct premulalpha_blend_d_functor {
	inline tjs_uint32 operator()( tjs_uint32 d, tjs_uint32 s ) const { return 0; }
};
struct premulalpha_blend_do_functor {
	inline premulalpha_blend_do_functor( tjs_int opa ){}
	inline tjs_uint32 operator()( tjs_uint32 d, tjs_uint32 s ) const { return 0; }
};
// 以下の2つは実装されない
// TVPAdditiveAlphaBlend_d_c
// TVPAdditiveAlphaBlend_do_c

#define DEFINE_COLOR_COPY( FUNC ) \
static void TVP_##FUNC( tjs_uint32 *dest, tjs_int len, tjs_uint32 value ) {	\
	FUNC##_functor func( value );												\
	const_color_copy( dest, len, func );										\
}

#define DEFINE_COLOR_BLEND( FUNC ) \
static void TVP_##FUNC( tjs_uint32 *dest, tjs_int len, tjs_uint32 value, tjs_int opa ) {	\
	FUNC##_functor func( opa, value );														\
	const_color_copy( dest, len, func );													\
}

#define DEFINE_APPLY_FUNCTION_VARIATION( FUNC ) \
static void TVP_##FUNC( tjs_uint32 *dest, const tjs_uint8 *src, tjs_int len, tjs_uint32 color ) {						\
	apply_color_map_func_c<FUNC##_functor>( dest, src, len, color );													\
}																														\
static void TVP_##FUNC##_HDA( tjs_uint32 *dest, const tjs_uint8 *src, tjs_int len, tjs_uint32 color ) {					\
	apply_color_map_func_c<FUNC##_HDA_functor>( dest, src, len, color );												\
}																														\
static void TVP_##FUNC##_o( tjs_uint32 *dest, const tjs_uint8 *src, tjs_int len, tjs_uint32 color, tjs_int opa ) {		\
	apply_color_map_o_func_c<FUNC##_o_functor>( dest, src, len, color, opa );											\
}																														\
static void TVP_##FUNC##_HDA_o( tjs_uint32 *dest, const tjs_uint8 *src, tjs_int len, tjs_uint32 color, tjs_int opa ) {	\
	apply_color_map_o_func_c<FUNC##_HDA_o_functor>( dest, src, len, color, opa );										\
}																														\
static void TVP_##FUNC##_d( tjs_uint32 *dest, const tjs_uint8 *src, tjs_int len, tjs_uint32 color ) {					\
	apply_color_map_func_c<FUNC##_d_functor>( dest, src, len, color );													\
}																														\
static void TVP_##FUNC##_a( tjs_uint32 *dest, const tjs_uint8 *src, tjs_int len, tjs_uint32 color ) {					\
	apply_color_map_func_c<FUNC##_a_functor>( dest, src, len, color );													\
}																														\
static void TVP_##FUNC##_do( tjs_uint32 *dest, const tjs_uint8 *src, tjs_int len, tjs_uint32 color, tjs_int opa ) {		\
	apply_color_map_o_func_c<FUNC##_do_functor>( dest, src, len, color, opa );											\
}																														\
static void TVP_##FUNC##_ao( tjs_uint32 *dest, const tjs_uint8 *src, tjs_int len, tjs_uint32 color, tjs_int opa ) {		\
	apply_color_map_o_func_c<FUNC##_ao_functor>( dest, src, len, color, opa );											\
}

DEFINE_CONVERT_BLEND_FUNCTION( remove_const_opacity );
DEFINE_ALPHA_COPY_FUNCTION( remove_opacity );
DEFINE_ALPHA_COPY_FUNCTION( remove_opacity65 );
DEFINE_ALPHA_BLEND_FUNCTION( remove_opacity_o );
DEFINE_ALPHA_BLEND_FUNCTION( remove_opacity65_o );

DEFINE_OVERLAP_COPY( color_copy );
DEFINE_OVERLAP_COPY( alpha_copy );
DEFINE_COPY_FUNCTION( color_opaque );

DEFINE_BLEND_FUNCTION( const_alpha_blend );
DEFINE_BLEND_FUNCTION( const_alpha_blend_hda );
DEFINE_BLEND_FUNCTION( const_alpha_blend_d );
DEFINE_BLEND_FUNCTION( const_alpha_blend_a );

DEFINE_SD_BLEND_FUNCTION( const_alpha_blend );
DEFINE_SD_BLEND_FUNCTION( sd_const_alpha_blend_a );
DEFINE_SD_BLEND_FUNCTION( sd_const_alpha_blend_d );

DEFINE_APPLY_FUNCTION_VARIATION( apply_color_map65 )
DEFINE_APPLY_FUNCTION_VARIATION( apply_color_map )

DEFINE_BLEND8_FUNCTION( ch_blur_mul_copy65 );
DEFINE_BLEND8_FUNCTION( ch_blur_mul_copy );
DEFINE_BLEND8_FUNCTION( ch_blur_add_mul_copy65 );
DEFINE_BLEND8_FUNCTION( ch_blur_add_mul_copy );

DEFINE_COLOR_COPY( fill_argb );
DEFINE_COLOR_COPY( const_color_copy );
DEFINE_COLOR_COPY( const_alpha_copy );
DEFINE_COLOR_BLEND( const_alpha_fill_blend );
DEFINE_COLOR_BLEND( const_alpha_fill_blend_d );
DEFINE_COLOR_BLEND( const_alpha_fill_blend_a );

DEFINE_BLEND_FUNCTION_VARIATION( alpha_blend );
DEFINE_BLEND_FUNCTION_VARIATION( premulalpha_blend )

DEFINE_BLEND_FUNCTION_MIN_VARIATION( add_blend );
DEFINE_BLEND_FUNCTION_MIN_VARIATION( sub_blend );
DEFINE_BLEND_FUNCTION_MIN_VARIATION( mul_blend );
DEFINE_BLEND_FUNCTION_MIN_VARIATION( color_dodge_blend );
DEFINE_BLEND_FUNCTION_MIN_VARIATION( darken_blend );
DEFINE_BLEND_FUNCTION_MIN_VARIATION( lighten_blend );
DEFINE_BLEND_FUNCTION_MIN_VARIATION( screen_blend );

DEFINE_BLEND_FUNCTION_MIN_VARIATION( ps_alpha_blend );
DEFINE_BLEND_FUNCTION_MIN_VARIATION( ps_add_blend );
DEFINE_BLEND_FUNCTION_MIN_VARIATION( ps_sub_blend );
DEFINE_BLEND_FUNCTION_MIN_VARIATION( ps_mul_blend );
DEFINE_BLEND_FUNCTION_MIN_VARIATION( ps_screen_blend );
DEFINE_BLEND_FUNCTION_MIN_VARIATION( ps_soft_light_blend );
DEFINE_BLEND_FUNCTION_MIN_VARIATION( ps_color_dodge_blend );
DEFINE_BLEND_FUNCTION_MIN_VARIATION( ps_color_burn_blend );
DEFINE_BLEND_FUNCTION_MIN_VARIATION( ps_overlay_blend );
DEFINE_BLEND_FUNCTION_MIN_VARIATION( ps_hard_light_blend );
DEFINE_BLEND_FUNCTION_MIN_VARIATION( ps_color_dodge5_blend );
DEFINE_BLEND_FUNCTION_MIN_VARIATION( ps_lighten_blend );
DEFINE_BLEND_FUNCTION_MIN_VARIATION( ps_darken_blend );
DEFINE_BLEND_FUNCTION_MIN_VARIATION( ps_diff_blend );
DEFINE_BLEND_FUNCTION_MIN_VARIATION( ps_diff5_blend );
DEFINE_BLEND_FUNCTION_MIN_VARIATION( ps_exclusion_blend );

static void TVP_make_alpha_from_key(tjs_uint32 *dest, tjs_int len, tjs_uint32 key) {
	make_alpha_from_key_functor func(key);
	convert_func_c( dest, len, func );
}
static void TVP_alpha_color_mat(tjs_uint32 *dest, tjs_uint32 color, tjs_int len ) {
	alpha_color_mat_functor func(color);
	convert_func_c( dest, len, func );
}
DEFINE_CONVERT_FUNCTION( convet_premulalpha_to_alpha );
DEFINE_CONVERT_FUNCTION( convet_alpha_to_premulalpha );
DEFINE_ALPHA_COPY_FUNCTION( bind_mask_to_main );
DEFINE_CONVERT_FUNCTION( do_gray_scale );

/* simple blur for character data */
/* shuld be more optimized */
template<typename functor>
static inline void ch_blur_copy_func( tjs_uint8 *dest, tjs_int destpitch, tjs_int destwidth, tjs_int destheight, const tjs_uint8 * src, tjs_int srcpitch, tjs_int srcwidth, tjs_int srcheight, tjs_int blurwidth, tjs_int blurlevel ) {
	/* clear destination */
	memset(dest, 0, destpitch*destheight);

	/* compute filter level */
	tjs_int lvsum = 0;
	for(tjs_int y = -blurwidth; y <= blurwidth; y++) {
		for(tjs_int x = -blurwidth; x <= blurwidth; x++) {
			tjs_int len = fast_int_hypot(x, y);
			if(len <= blurwidth)
				lvsum += (blurwidth - len +1);
		}
	}

	if(lvsum) lvsum = (1<<18)/lvsum; else lvsum=(1<<18);

	/* apply */
	for(tjs_int y = -blurwidth; y <= blurwidth; y++) {
		for(tjs_int x = -blurwidth; x <= blurwidth; x++) {
			tjs_int len = fast_int_hypot(x, y);
			if(len <= blurwidth) {
				len = blurwidth - len +1;
				len *= lvsum;
				len *= blurlevel;
				len >>= 8;
				functor func(len);
				for(tjs_int sy = 0; sy < srcheight; sy++) {
					blend_func_c( dest + (y + sy + blurwidth)*destpitch + x + blurwidth, src + sy * srcpitch, srcwidth, func );
				}
			}
		}
	}
}
void TVP_ch_blur_copy65( tjs_uint8 *dest, tjs_int destpitch, tjs_int destwidth, tjs_int destheight, const tjs_uint8 * src, tjs_int srcpitch, tjs_int srcwidth, tjs_int srcheight, tjs_int blurwidth, tjs_int blurlevel ) {
	ch_blur_copy_func<ch_blur_add_mul_copy65_functor>( dest, destpitch, destwidth, destheight, src, srcpitch, srcwidth, srcheight, blurwidth, blurlevel );
}
void TVP_ch_blur_copy( tjs_uint8 *dest, tjs_int destpitch, tjs_int destwidth, tjs_int destheight, const tjs_uint8 * src, tjs_int srcpitch, tjs_int srcwidth, tjs_int srcheight, tjs_int blurwidth, tjs_int blurlevel ) {
	ch_blur_copy_func<ch_blur_add_mul_copy_functor>( dest, destpitch, destwidth, destheight, src, srcpitch, srcwidth, srcheight, blurwidth, blurlevel );
}
void TVP_reverse32(tjs_uint32 *pixels, tjs_int len) {
	tjs_uint32 *pixels2 = pixels + len -1;
	len/=2;
	tjs_uint32 *limit = pixels+len;
	while( pixels < limit ) {
		tjs_uint32 tmp = *pixels;
		*pixels = *pixels2;
		*pixels2 = tmp;
		pixels2 --;
		pixels++;
	}
}
void TVP_reverse8( tjs_uint8 *pixels, tjs_int len ) {
	tjs_uint8 *pixels2 = pixels + len -1;
	len/=2;
	tjs_uint8 *limit = pixels+len;
	while( pixels < limit ) {
		tjs_uint8 tmp = *pixels;
		*pixels = *pixels2;
		*pixels2 = tmp;
		pixels2 --;
		pixels++;
	}
}
void TVP_swap_line32(tjs_uint32 *line1, tjs_uint32 *line2, tjs_int len) {
	for( tjs_int i = 0; i < len; i++ ) {
		tjs_uint32 tmp = line1[i];
		line1[i] = line2[i];
		line2[i] = tmp;
	}
}
void TVP_swap_line8(tjs_uint8 *line1, tjs_uint8 *line2, tjs_int len)
{
	#define swap_tmp_buf_size 256
	tjs_uint8 swap_tmp_buf[swap_tmp_buf_size];
	while(len)
	{
		tjs_int le = len < swap_tmp_buf_size ? len : swap_tmp_buf_size;
		for( tjs_int i = 0; i < le/4; i++ ) {
		}
		memcpy(swap_tmp_buf, line1, le);
		memcpy(line1, line2, le);
		memcpy(line2, swap_tmp_buf, le);
		line1 += le;
		line2 += le;
		len -= le;
	}
	#undef swap_tmp_buf_size
}
/* --------------------------------------------------------------------
  Table initialize function
-------------------------------------------------------------------- */
static void TVPPsInitTable(void)
{
	int s, d;
	for (s=0; s<256; s++) {
		for (d=0; d<256; d++) {
			ps_soft_light_table::TABLE[s][d]  = (s>=128) ?
				( ((unsigned char)(pow(d/255.0, 128.0/s)*255.0)) ) :
				( ((unsigned char)(pow(d/255.0, (1.0-s/255.0)/0.5)*255.0)) );
			ps_color_dodge_table::TABLE[s][d] = ((255-s)<=d) ? (0xff) : ((d*255)/(255-s));
			ps_color_burn_table::TABLE[s][d]  = (s<=(255-d)) ? (0x00) : (255-((255-d)*255)/s);
#ifdef TVPPS_USE_OVERLAY_TABLE
			ps_overlay_table::TABLE[s][d]  = (d<128) ? ((s*d*2)/255) : (((s+d)*2)-((s*d*2)/255)-255);
#endif
		}
	}
}


#define SET_BLEND_FUNCTIONS( DEST_FUNC, FUNC )	\
TVP##DEST_FUNC = TVP_##FUNC;					\
TVP##DEST_FUNC##_HDA = TVP_##FUNC##_HDA;		\
TVP##DEST_FUNC##_o = TVP_##FUNC##_o;			\
TVP##DEST_FUNC##_HDA_o = TVP_##FUNC##_HDA_o;	\
TVP##DEST_FUNC##_d = TVP_##FUNC##_d;			\
TVP##DEST_FUNC##_a = TVP_##FUNC##_a;			\
TVP##DEST_FUNC##_do = TVP_##FUNC##_do;			\
TVP##DEST_FUNC##_ao = TVP_##FUNC##_ao;

#define SET_BLEND_MID_FUNCTIONS( DEST_FUNC, FUNC )	\
TVP##DEST_FUNC = TVP_##FUNC;					\
TVP##DEST_FUNC##_HDA = TVP_##FUNC##_HDA;		\
TVP##DEST_FUNC##_o = TVP_##FUNC##_o;			\
TVP##DEST_FUNC##_HDA_o = TVP_##FUNC##_HDA_o;	\
TVP##DEST_FUNC##_d = TVP_##FUNC##_d;			\
TVP##DEST_FUNC##_do = TVP_##FUNC##_do;

#define SET_BLEND_MIN_FUNCTIONS( DEST_FUNC, FUNC )	\
TVP##DEST_FUNC = TVP_##FUNC;					\
TVP##DEST_FUNC##_HDA = TVP_##FUNC##_HDA;		\
TVP##DEST_FUNC##_o = TVP_##FUNC##_o;			\
TVP##DEST_FUNC##_HDA_o = TVP_##FUNC##_HDA_o;

// dummy
static void (*TVPAdditiveAlphaBlend_d)(tjs_uint32*,const tjs_uint32*,tjs_int);
static void (*TVPAdditiveAlphaBlend_do)(tjs_uint32*,const tjs_uint32*,tjs_int,tjs_int);

extern void TVP_ch_blur_add_mul_copy65_sse2_c( tjs_uint8 *dest, const tjs_uint8 *src, tjs_int len, tjs_int opa );
extern void TVP_ch_blur_add_mul_copy_sse2_c( tjs_uint8 *dest, const tjs_uint8 *src, tjs_int len, tjs_int opa );
extern void TVP_ch_blur_mul_copy65_sse2_c( tjs_uint8 *dest, const tjs_uint8 *src, tjs_int len, tjs_int opa );
extern void TVP_ch_blur_mul_copy_sse2_c( tjs_uint8 *dest, const tjs_uint8 *src, tjs_int len, tjs_int opa );
/**
 * GL初期化。関数ポインタを設定する
 */
void TVPGL_C_Init() {

	// 各種ブレンドを関数オブジェクト化することでアフィン変換や拡縮に展開しやすくする
	SET_BLEND_FUNCTIONS( AlphaBlend, alpha_blend );
	SET_BLEND_FUNCTIONS( AdditiveAlphaBlend, premulalpha_blend );
	SET_BLEND_MIN_FUNCTIONS( AddBlend, add_blend );
	SET_BLEND_MIN_FUNCTIONS( SubBlend, sub_blend );
	SET_BLEND_MIN_FUNCTIONS( MulBlend, mul_blend );
	SET_BLEND_MIN_FUNCTIONS( ColorDodgeBlend, color_dodge_blend );
	SET_BLEND_MIN_FUNCTIONS( DarkenBlend, darken_blend );
	SET_BLEND_MIN_FUNCTIONS( LightenBlend, lighten_blend );
	SET_BLEND_MIN_FUNCTIONS( ScreenBlend, screen_blend );

	SET_BLEND_MIN_FUNCTIONS( PsAlphaBlend, ps_alpha_blend );
	SET_BLEND_MIN_FUNCTIONS( PsAddBlend, ps_add_blend );
	SET_BLEND_MIN_FUNCTIONS( PsSubBlend, ps_sub_blend );
	SET_BLEND_MIN_FUNCTIONS( PsMulBlend, ps_mul_blend );
	SET_BLEND_MIN_FUNCTIONS( PsScreenBlend, ps_screen_blend );
	SET_BLEND_MIN_FUNCTIONS( PsSoftLightBlend, ps_soft_light_blend );
	SET_BLEND_MIN_FUNCTIONS( PsColorDodgeBlend, ps_color_dodge_blend );
	SET_BLEND_MIN_FUNCTIONS( PsColorBurnBlend, ps_color_burn_blend );
	SET_BLEND_MIN_FUNCTIONS( PsOverlayBlend, ps_overlay_blend );
	SET_BLEND_MIN_FUNCTIONS( PsHardLightBlend, ps_hard_light_blend );
	SET_BLEND_MIN_FUNCTIONS( PsColorDodge5Blend, ps_color_dodge5_blend );
	SET_BLEND_MIN_FUNCTIONS( PsLightenBlend, ps_lighten_blend );
	SET_BLEND_MIN_FUNCTIONS( PsDarkenBlend, ps_darken_blend );
	SET_BLEND_MIN_FUNCTIONS( PsDiffBlend, ps_diff_blend );
	SET_BLEND_MIN_FUNCTIONS( PsDiff5Blend, ps_diff5_blend );
	SET_BLEND_MIN_FUNCTIONS( PsExclusionBlend, ps_exclusion_blend );

	TVPConstAlphaBlend = TVP_const_alpha_blend;
	TVPConstAlphaBlend_HDA = TVP_const_alpha_blend_hda;
	TVPConstAlphaBlend_d = TVP_const_alpha_blend_d;
	TVPConstAlphaBlend_a = TVP_const_alpha_blend_a;

	TVPConstAlphaBlend_SD =  TVP_const_alpha_blend_SD;
	TVPConstAlphaBlend_SD_a = TVP_sd_const_alpha_blend_a_SD;
	TVPConstAlphaBlend_SD_d = TVP_sd_const_alpha_blend_d_SD;

	TVPCopyColor = TVP_color_copy;
	TVPCopyMask = TVP_alpha_copy;
	TVPCopyOpaqueImage = TVP_color_opaque;

	TVPFillARGB = TVP_fill_argb;
	TVPFillARGB_NC = TVP_fill_argb;
	TVPFillColor = TVP_const_color_copy;
	TVPFillMask = TVP_const_alpha_copy;

	TVPConstColorAlphaBlend = TVP_const_alpha_fill_blend;
	TVPConstColorAlphaBlend_d = TVP_const_alpha_fill_blend_d;
	TVPConstColorAlphaBlend_a = TVP_const_alpha_fill_blend_a;

	TVPApplyColorMap65 = TVP_apply_color_map65;
	TVPApplyColorMap65_HDA = TVP_apply_color_map65_HDA;
	TVPApplyColorMap65_o = TVP_apply_color_map65_o;
	TVPApplyColorMap65_HDA_o = TVP_apply_color_map65_HDA_o;
	TVPApplyColorMap65_d = TVP_apply_color_map65_d;
	TVPApplyColorMap65_a = TVP_apply_color_map65_a;
	TVPApplyColorMap65_do = TVP_apply_color_map65_do;
	TVPApplyColorMap65_ao = TVP_apply_color_map65_ao;

	TVPApplyColorMap = TVP_apply_color_map;
	TVPApplyColorMap_HDA = TVP_apply_color_map_HDA;
	TVPApplyColorMap_o = TVP_apply_color_map_o;
	TVPApplyColorMap_HDA_o = TVP_apply_color_map_HDA_o;
	TVPApplyColorMap_d = TVP_apply_color_map_d;
	TVPApplyColorMap_a = TVP_apply_color_map_a;
	TVPApplyColorMap_do = TVP_apply_color_map_do;
	TVPApplyColorMap_ao = TVP_apply_color_map_ao;

	TVPRemoveConstOpacity = TVP_remove_const_opacity;
	TVPRemoveOpacity = TVP_remove_opacity;
	TVPRemoveOpacity_o = TVP_remove_opacity_o;
	TVPRemoveOpacity65 = TVP_remove_opacity65;
	TVPRemoveOpacity65_o = TVP_remove_opacity65_o;

	TVPMakeAlphaFromKey = TVP_make_alpha_from_key;
	TVPAlphaColorMat = TVP_alpha_color_mat;
	TVPConvertAdditiveAlphaToAlpha = TVP_convet_premulalpha_to_alpha;
	TVPConvertAlphaToAdditiveAlpha = TVP_convet_alpha_to_premulalpha;
	TVPBindMaskToMain = TVP_bind_mask_to_main;

	TVPSwapLine8 = TVP_swap_line8;
	TVPSwapLine32 = TVP_swap_line32;
	TVPReverse8 = TVP_reverse8;
	TVPReverse32 = TVP_reverse32;
	TVPDoGrayScale = TVP_do_gray_scale;
	TVPChBlurMulCopy65 = TVP_ch_blur_mul_copy65;
	TVPChBlurAddMulCopy65 = TVP_ch_blur_add_mul_copy65;
	TVPChBlurCopy65 = TVP_ch_blur_copy65;
	TVPChBlurMulCopy = TVP_ch_blur_mul_copy;
	TVPChBlurAddMulCopy = TVP_ch_blur_add_mul_copy;
	TVPChBlurCopy = TVP_ch_blur_copy;

	TVPPsInitTable();
}

