#include "FontImpl.h"
#include <ft2build.h>
#include FT_TRUETYPE_IDS_H
#include FT_SFNT_NAMES_H
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
#pragma comment(lib,"freetype.lib")
#endif
#include "platform/CCFileUtils.h"
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
static int TVPInternalEnumFonts(FT_Byte* pBuf, int buflen, const ttstr &FontPath, const std::function<tTJSBinaryStream*(TVPFontNamePathInfo*)>& getter) {
	unsigned int faceCount = 0;
	FT_Face fontface;
	FT_Error error = FT_New_Memory_Face(
		TVPGetFontLibrary(),
		pBuf,
		buflen,
		0,
		&fontface);
	if (error) {
		TVPAddLog(ttstr(TJS_W("Load Font \"") + FontPath + "\" failed (" + TJSIntegerToString((int)error) + ")"));
		return faceCount;
	}
	int nFaceNum = fontface->num_faces;
	for (int i = 0; i < nFaceNum; ++i) {
		if (i > 0) {
			if (FT_New_Memory_Face(
				TVPGetFontLibrary(),
				pBuf,
				buflen,
				i,
				&fontface)) {
				continue;
			}
		}
		if (FT_IS_SCALABLE(fontface)) {
			FT_UInt namecount = FT_Get_Sfnt_Name_Count(fontface);
			int addCount = 0;
			for (FT_UInt i = 0; i < namecount; ++i) {
				FT_SfntName name;
				if (FT_Get_Sfnt_Name(fontface, i, &name)) {
					continue;
				}
				if (name.name_id != TT_NAME_ID_FONT_FAMILY) {
					continue;
				}
				if (name.platform_id != TT_PLATFORM_MICROSOFT) {
					continue;
				}
				switch (name.language_id) { // for CJK names
				case TT_MS_LANGID_JAPANESE_JAPAN:
				case TT_MS_LANGID_CHINESE_GENERAL:
				case TT_MS_LANGID_CHINESE_TAIWAN:
				case TT_MS_LANGID_CHINESE_PRC:
				case TT_MS_LANGID_CHINESE_HONG_KONG:
				case TT_MS_LANGID_CHINESE_SINGAPORE:
				case TT_MS_LANGID_KOREAN_EXTENDED_WANSUNG_KOREA:
				case TT_MS_LANGID_KOREAN_JOHAB_KOREA:
					break;
				default:
					continue;
				}
				ttstr fontname;
				if (name.encoding_id == TT_MS_ID_UNICODE_CS) {
					std::vector<tjs_char> tmp;
					int namelen = name.string_len / 2;
					tmp.resize(namelen + 1);
					for (int j = 0; j < namelen; ++j) {
						tmp[j] = (name.string[j * 2] << 8) | (name.string[j * 2 + 1]);
					}
					fontname = &tmp.front();
				} else {
					continue;
				}
				TVPFontNamePathInfo info;
				info.Path = FontPath;
				info.Index = i;
				info.Getter = getter;
				TVPFontNames.Add(fontname, info);
				addCount = 1;
			}
			/*if (!addCount)*/ {
				ttstr fontname((tjs_nchar*)fontface->family_name);
				TVPFontNamePathInfo info;
				info.Path = FontPath;
				info.Index = i;
				info.Getter = getter;
				TVPFontNames.Add(fontname, info);
			}
			++faceCount;
		}

		FT_Done_Face(fontface);
	}
	return faceCount;
}

int TVPEnumFontsProc(const ttstr &FontPath)
{
    if(!TVPIsExistentStorageNoSearch(FontPath)) {
        return 0;
    }

    tTJSBinaryStream * Stream = TVPCreateStream(FontPath, TJS_BS_READ);
    if(!Stream) {
        return 0;
    }
    int bufflen = Stream->GetSize();
	std::vector<FT_Byte> buf; buf.resize(bufflen);
    Stream->ReadBuffer(&buf.front(), bufflen);
    delete Stream;
	return TVPInternalEnumFonts(&buf.front(), bufflen, FontPath, nullptr);
}

tTJSBinaryStream* TVPCreateFontStream(const ttstr &fontname)
{
	TVPFontNamePathInfo *info = TVPFindFont(fontname);
	if (!info) {
		info = TVPFontNames.Find(TVPDefaultFontName);
		if (!info) return nullptr;
	}
	if (info->Getter) {
		return info->Getter(info);
	}
	return TVPCreateBinaryStreamForRead(info->Path, TJS_W(""));
}

//---------------------------------------------------------------------------
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
	TVPFontNamesInit = true;
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

		{	// from internal storage
			auto data = cocos2d::FileUtils::getInstance()->getDataFromFile("DroidSansFallback.ttf");
			if (TVPInternalEnumFonts(data.getBytes(), data.getSize(), "DroidSansFallback.ttf", [](TVPFontNamePathInfo* info)->tTJSBinaryStream* {
				auto data = cocos2d::FileUtils::getInstance()->getDataFromFile(info->Path.AsStdString());
				tTVPMemoryStream *ret = new tTVPMemoryStream();
				ret->WriteBuffer(data.getBytes(), data.getSize());
				ret->SetPosition(0);
				return ret;
			})) break;
		}
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

    // check exePath + "/fonts/*.ttf"
	{
		std::vector<ttstr> list;
		auto lister = [&](const ttstr &name, tTVPLocalFileInfo* s) {
			if (s->Mode & (S_IFREG | S_IFDIR)) {
				list.emplace_back(name);
			}
		};
#ifdef __ANDROID__
		TVPGetLocalFileListAt(Android_GetInternalStoragePath() + "/fonts", lister);
		for (const ttstr &path : pathlist) {
			TVPGetLocalFileListAt(path + "/fonts", lister);
		}
#endif
		TVPGetLocalFileListAt(TVPGetAppPath() + "/fonts", lister);
        auto itend = list.end();
        for (auto it = list.begin(); it != itend; ++it) {
            TVPEnumFontsProc(*it);
        }
    }

	if (TVPDefaultFontName.IsEmpty()) {
		TVPShowSimpleMessageBox(("Could not found any font.\nPlease ensure that at least \"default.ttf\" exists"), "Exception Occured");
    }
}
//---------------------------------------------------------------------------
TVPFontNamePathInfo* TVPFindFont(const ttstr &fontname)
{
    // check existence of font
    TVPInitFontNames();

	TVPFontNamePathInfo *info = nullptr;
	if (!fontname.IsEmpty() && fontname[0] == TJS_W('@')) { // vertical version
		info = TVPFontNames.Find(fontname.c_str() + 1);
	}
	if (!info) {
		info = TVPFontNames.Find(fontname);
	}
    return info;
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
