/*

	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000-2009 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"


*/


#ifndef __TVP_MATH_UTIL_H__
#define __TVP_MATH_UTIL_H__

/* fast_int_hypot from http://demo.and.or.jp/makedemo/effect/math/hypot/fast_hypot.c */
static inline tjs_uint fast_int_hypot(tjs_int lx, tjs_int ly) {
	tjs_uint len1, len2,t,length;
	if(lx<0) lx = -lx;
	if(ly<0) ly = -ly;
	if (lx >= ly) { len1 = lx ; len2 = ly; }
	else { len1 = ly ; len2 = lx; }
	t = len2 + (len2 >> 1) ;
	length = len1 - (len1 >> 5) - (len1 >> 7) + (t >> 2) + (t >> 6) ;
	return length;
}

#endif //__TVP_MATH_UTIL_H__
