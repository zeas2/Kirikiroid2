/*
  Photoshop-like layer blender for KIRIKIRI (C-version)
  (c)2004-2005 Kengo Takagi (Kenjo) <kenjo@ceres.dti.ne.jp>

  Include file for function macro expansion (see also tvpps.c)
*/

/* ------------------------------ */
#define OPERATION1 { TVPPS_REG tjs_uint32 s = *buf++, d = *dest, a = s>>24; TVPPS_OPERATION; *dest++ = s; }
void __cdecl TVPPS_FUNC_NORM(tjs_uint32 *dest, const tjs_uint32 *buf, tjs_int len)
{
	TVPPS_MAINLOOP
}
#undef OPERATION1
/* ------------------------------ */
#define OPERATION1 { TVPPS_REG tjs_uint32 s = *buf++, d = *dest, a = ((s>>24)*opa)>>8; TVPPS_OPERATION; *dest++ = s; }
void __cdecl TVPPS_FUNC_O(tjs_uint32 *dest, const tjs_uint32 *buf, tjs_int len, tjs_int opa)
{
	TVPPS_MAINLOOP
}
#undef OPERATION1
/* ------------------------------ */
#define OPERATION1 { TVPPS_REG tjs_uint32 s = *buf++, d = *dest, a = s>>24; TVPPS_OPERATION; *dest++ = s|(d&0xff000000); }
void __cdecl TVPPS_FUNC_HDA(tjs_uint32 *dest, const tjs_uint32 *buf, tjs_int len)
{
	TVPPS_MAINLOOP
}
#undef OPERATION1
/* ------------------------------ */
#define OPERATION1 { TVPPS_REG tjs_uint32 s = *buf++, d = *dest, a = ((s>>24)*opa)>>8; TVPPS_OPERATION; *dest++ = s|(d&0xff000000); }
void __cdecl TVPPS_FUNC_HDA_O(tjs_uint32 *dest, const tjs_uint32 *buf, tjs_int len, tjs_int opa)
{
	TVPPS_MAINLOOP
}
#undef OPERATION1

#undef TVPPS_FUNC_NORM
#undef TVPPS_FUNC_O
#undef TVPPS_FUNC_HDA
#undef TVPPS_FUNC_HDA_O
#undef TVPPS_OPERATION
