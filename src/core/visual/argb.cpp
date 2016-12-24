#include "argb.h"

template <>
void tTVPARGB<tjs_uint8>::Zero()
{
	*(tjs_uint32 *)this = 0;
}

template <>
void tTVPARGB<tjs_uint8>::operator = (tjs_uint32 v)
{
	*(tjs_uint32 *)this = v;
}

template <>
tTVPARGB<tjs_uint8>::operator tjs_uint32() const
{
	return *(const tjs_uint32 *)this;
}

template <>
void tTVPARGB<tjs_uint8>::average(tjs_int n)
{
	tjs_int half_n = n >> 1;

	tjs_int recip = (1L << 23) / n;

	b = (b + half_n)*recip >> 23;
	g = (g + half_n)*recip >> 23;
	r = (r + half_n)*recip >> 23;
	a = (a + half_n)*recip >> 23;
}

template <>
void tTVPARGB<tjs_uint16>::average(tjs_int n)
{
	tjs_int half_n = n >> 1;

	tjs_int recip = (1L << 16) / n;

	b = (b + half_n)*recip >> 16;
	g = (g + half_n)*recip >> 16;
	r = (r + half_n)*recip >> 16;
	a = (a + half_n)*recip >> 16;
}

template <>
void tTVPARGB<tjs_uint32>::average(tjs_int n)
{
	tjs_int half_n = n >> 1;

	b = (b + half_n) / n;
	g = (g + half_n) / n;
	r = (r + half_n) / n;
	a = (a + half_n) / n;
}

// avoid bug for VC compiler
void __argb_hack_for_vc() {
	tTVPARGB<tjs_uint8> a;
	a = 0;
	a.Zero();
	a.operator tjs_uint32();
	a.average(0);
	tTVPARGB<tjs_uint16> b;
	b.average(0);
	tTVPARGB<tjs_uint32> c;
	c.average(0);
}
