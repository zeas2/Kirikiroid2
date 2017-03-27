#pragma once
#include "tjs.h"
#include "tjsHashSearch.h"

void TVPInitFontNames();
int TVPEnumFontsProc(const ttstr &FontPath);
tTJSBinaryStream* TVPCreateFontStream(const ttstr &fontname);
struct TVPFontNamePathInfo {
    ttstr Path;
    int Index;
};
TVPFontNamePathInfo* TVPFindFont(const ttstr &name);

//---------------------------------------------------------------------------
// font enumeration and existence check
//---------------------------------------------------------------------------
class tTVPttstrHash
{
public:
    static tjs_uint32 Make(const ttstr &val);
};
extern tTJSHashTable<ttstr, TVPFontNamePathInfo, tTVPttstrHash>
    TVPFontNames;