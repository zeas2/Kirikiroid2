#pragma once
#include "tjs.h"
#include "tjsHashSearch.h"

bool TVPFontExists(const ttstr &name);
void TVPInitFontNames();
int TVPEnumFontsProc(const ttstr &FontPath);
tTJSBinaryStream* TVPCreateFontStream(const ttstr &fontname);
struct TVPFontNamePathInfo {
    ttstr Path;
    int Index;
};

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