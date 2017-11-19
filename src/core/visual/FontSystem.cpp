

#include "tjsCommHead.h"

#include "FontSystem.h"
#include "StringUtil.h"
#include "MsgIntf.h"
#include <vector>
#include "ConfigManager/IndividualConfigManager.h"

extern void TVPGetAllFontList(std::vector<ttstr>& list);
extern const ttstr &TVPGetDefaultFontName();

void FontSystem::InitFontNames() {
	// enumlate all fonts
	if(FontNamesInit) return;

	std::vector<ttstr> list;
	TVPGetAllFontList( list );
	size_t count = list.size();
	for( size_t i = 0; i < count; i++ ) {
		AddFont( list[i] );
	}

	FontNamesInit = true;
}
//---------------------------------------------------------------------------
void FontSystem::AddFont(const ttstr& name) {
	TVPFontNames.Add( name, 1 );
}
//---------------------------------------------------------------------------
bool FontSystem::FontExists(const ttstr &name) {
	// check existence of font
	InitFontNames();

	int * t = TVPFontNames.Find(name);
	return t != NULL;
}

FontSystem::FontSystem() : FontNamesInit(false), DefaultLOGFONTCreated(false) {
	ConstructDefaultFont();
}

void FontSystem::ConstructDefaultFont() {
	if( !DefaultLOGFONTCreated ) {
		DefaultLOGFONTCreated = true;
		DefaultFont.Height = -12;
		DefaultFont.Flags = 0;
		DefaultFont.Angle = 0;
		DefaultFont.Face = ttstr(TVPGetDefaultFontName());
	}
}

ttstr FontSystem::GetBeingFont(ttstr fonts) {
	// retrieve being font in the system.
	// font candidates are given by "fonts", separated by comma.

	bool vfont;

	if(fonts.c_str()[0] == TJS_W('@')) {     // for vertical writing
		fonts = fonts.c_str() + 1;
		vfont = true;
	} else {
		vfont = false;
	}

	static bool force_default_font = IndividualConfigManager::GetInstance()->GetValue<bool>("force_default_font", false);
	if (!force_default_font) {
		bool prev_empty_name = false;
		while (fonts != TJS_W("")) {
			ttstr fontname;
			int pos = fonts.IndexOf(TJS_W(","));
			if (pos != -1) {
				fontname = Trim(fonts.SubString(0, pos));
				fonts = fonts.SubString(pos + 1, -1);
			} else {
				fontname = Trim(fonts);
				fonts = TJS_W("");
			}

			// no existing check if previously specified font candidate is empty
			// eg. ",Fontname"

			if (fontname != TJS_W("") && (prev_empty_name || FontExists(fontname))) {
				if (vfont && fontname.c_str()[0] != TJS_W('@')) {
					return  TJS_W("@") + fontname;
				} else {
					return fontname;
				}
			}

			prev_empty_name = (fontname == TJS_W(""));
		}
	}

	if(vfont) {
		return ttstr(TJS_W("@")) + TVPGetDefaultFontName();
	} else {
		return TVPGetDefaultFontName();
	}
}
