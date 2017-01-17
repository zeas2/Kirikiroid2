#include "FontImpl.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include "StorageIntf.h"
#include "DebugIntf.h"
#include "MsgIntf.h"
#include <map>
#include <math.h>
#include "Application.h"
#include "Platform.h"
#include "ConfigManager/IndividualConfigManager.h"
#ifndef M_PI
#define M_PI       3.14159265358979323846
#endif

#ifdef _MSC_VER
#pragma comment(lib,"freetype250.lib")
#endif
#include "platform/CCFileUtils.h"
#include <sys/stat.h>
#include "StorageImpl.h"
#include "BinaryStream.h"

tTJSHashTable<ttstr, TVPFontNamePathInfo, tTVPttstrHash>
    TVPFontNames;
static ttstr TVPDefaultFontName;
const ttstr &TVPGetDefaultFontName() {
	return TVPDefaultFontName;
}
void TVPGetAllFontList(std::vector<ttstr>& list) {
	auto itend = TVPFontNames.GetLast();
	for (auto it = TVPFontNames.GetFirst(); it != itend; ++it) {
		list.push_back(it.GetKey());
	}
}
static FT_Library TVPFontLibrary;
FT_Library &TVPGetFontLibrary() {
	if (!TVPFontLibrary) {
		FT_Error error = FT_Init_FreeType(&TVPFontLibrary);
		if (error) TVPThrowExceptionMessage(
			(ttstr(TJS_W("Initialize FreeType failed, error = ")) + TJSIntegerToString((tjs_int)error)).c_str());
		TVPInitFontNames();
	}
	return TVPFontLibrary;
}
void TVPReleaseFontLibrary() {
	if (TVPFontLibrary) {
		FT_Done_FreeType(TVPFontLibrary);
	}
}
//---------------------------------------------------------------------------
int TVPEnumFontsProc(const ttstr &FontPath)
{
    unsigned int faceCount = 0;
    if(!TVPIsExistentStorageNoSearch(FontPath)) {
        return faceCount;
    }

    tTJSBinaryStream * Stream = TVPCreateStream(FontPath, TJS_BS_READ);
    if(!Stream) {
        return faceCount;
    }
    int bufflen = Stream->GetSize();
    FT_Byte *pBuf = new FT_Byte[bufflen]; 
    if(!pBuf) {
        delete Stream;
		return 0;
    }
    Stream->ReadBuffer(pBuf, bufflen);
    delete Stream;
    FT_Face fontface;
    FT_Error error = FT_New_Memory_Face(
		TVPGetFontLibrary(),
        pBuf,
        bufflen,
        0,
        &fontface);
    if(error) {
        TVPAddLog(ttstr(TJS_W("Load Font \"") + FontPath + "\" failed (" + TJSIntegerToString((int)error) + ")"));
        delete []pBuf;
        return faceCount;
    }
    int nFaceNum = fontface->num_faces;
    for(int i = 0; i <nFaceNum; ++i) {
        if( i > 0) {
            if(FT_New_Memory_Face(
				TVPGetFontLibrary(),
                pBuf,
                bufflen,
                i,
                &fontface)) {
                continue;
            }
        }
        if(FT_IS_SCALABLE(fontface)) {
            ttstr fontname((tjs_nchar*)fontface->family_name);
            TVPFontNamePathInfo info;
            info.Path = FontPath;
            info.Index = i;
            TVPFontNames.Add(fontname, info);
            ++faceCount;
        }

        FT_Done_Face(fontface);
    }
    delete []pBuf;
    return faceCount;
}

tTJSBinaryStream* TVPCreateFontStream(const ttstr &fontname)
{
	TVPFontNamePathInfo *info = TVPFontNames.Find(fontname);
	if (!info) {
		info = TVPFontNames.Find(TVPDefaultFontName);
		if (!info) return nullptr;
	}
	return TVPCreateBinaryStreamForRead(info->Path, TJS_W(""));
}

//---------------------------------------------------------------------------
void TVPGetListAt(const ttstr &name, iTVPStorageLister * lister);
#ifdef __ANDROID__
extern std::vector<ttstr> Android_GetExternalStoragePath();
extern ttstr Android_GetInternalStoragePath();
extern ttstr Android_GetApkStoragePath();
#endif
void TVPInitFontNames()
{
    static bool TVPFontNamesInit = false;
    // enumlate all fonts
    if(TVPFontNamesInit) return;
#ifdef __ANDROID__
	std::vector<ttstr> pathlist = Android_GetExternalStoragePath();
#endif
	do {
		ttstr userFont = IndividualConfigManager::GetInstance()->GetValue<std::string>("default_font", "");
		if (!userFont.IsEmpty() && TVPEnumFontsProc(userFont)) break;

		if (TVPEnumFontsProc(TVPGetAppPath() + "default.ttf")) break;
		if (TVPEnumFontsProc(TVPGetAppPath() + "default.ttc")) break;
		if (TVPEnumFontsProc(TVPGetAppPath() + "default.otf")) break;
		if (TVPEnumFontsProc(TVPGetAppPath() + "default.otc")) break;
#if defined(__ANDROID__)
		int fontCount = 0;
		for (const ttstr &path : pathlist) {
			fontCount += TVPEnumFontsProc(path + "/default.ttf");
			if (fontCount) break;
		}
		if (fontCount) break;
		
		if (TVPEnumFontsProc(Android_GetInternalStoragePath() + "/default.ttf")) break;
		if (TVPEnumFontsProc(Android_GetApkStoragePath() + ">assets/DroidSansFallback.ttf")) break;
		if (TVPEnumFontsProc(TJS_W("file://./system/fonts/DroidSansFallback.ttf"))) break;
		if (TVPEnumFontsProc(TJS_W("file://./system/fonts/NotoSansHans-Regular.otf"))) break;
		if (TVPEnumFontsProc(TJS_W("file://./system/fonts/DroidSans.ttf"))) break;
#elif defined(WIN32)
		if (TVPEnumFontsProc(TJS_W("file://./c/windows/fonts/msyh.ttf"))) break;
		if (TVPEnumFontsProc(TJS_W("file://./c/windows/fonts/simhei.ttf"))) break;
#endif
        
        std::string fullPath = cocos2d::FileUtils::getInstance()->fullPathForFilename("DroidSansFallback.ttf");
        if (TVPEnumFontsProc(fullPath)) break;
	} while (false);
    if(TVPFontNames.GetCount() > 0)
    {
        // set default fontface name
        TVPDefaultFontName = TVPFontNames.GetLast().GetKey();
    }
    class tLister : public iTVPStorageLister
    {
    public:
        std::vector<ttstr> list;
        void TJS_INTF_METHOD Add(const ttstr &file)
        {
			list.emplace_back(file);
        }
    } lister;

#ifdef __ANDROID__
	TVPGetListAt(Android_GetInternalStoragePath() + "/fonts", &lister);
	for (const ttstr &path : pathlist) {
		TVPGetListAt(path + "/fonts", &lister);
	}
#endif
    // check exePath + "/fonts/*.ttf"
	{
		TVPGetLocalFileListAt(TVPGetAppPath() + "/fonts", &lister, S_IFREG);
        auto itend = lister.list.end();
        for (auto it = lister.list.begin(); it != itend; ++it) {
            TVPEnumFontsProc(*it);
        }
    }

	if (TVPDefaultFontName.IsEmpty()) {
		TVPShowSimpleMessageBox(("Could not found any font.\nPlease ensure that at least \"default.ttf\" exists"), "Exception Occured");
    }

    TVPFontNamesInit = true;
}
//---------------------------------------------------------------------------
bool TVPFontExists(const ttstr &name)
{
    // check existence of font
    TVPInitFontNames();

    TVPFontNamePathInfo * t = TVPFontNames.Find(name);

    return t != NULL;
}

tjs_uint32 tTVPttstrHash::Make( const ttstr &val )
{
    const tjs_char * ptr = val.c_str();
    if(*ptr == 0) return 0;
    tjs_uint32 v = 0;
    while(*ptr)
    {
        v += *ptr;
        v += (v << 10);
        v ^= (v >> 6);
        ptr++;
    }
    v += (v << 3);
    v ^= (v >> 11);
    v += (v << 15);
    if(!v) v = (tjs_uint32)-1;
    return v;
}
