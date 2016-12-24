#ifndef __TVP_SYS_FONT_H__
#define __TVP_SYS_FONT_H__

class tTVPSysFont {
// 	HFONT hFont_;
// 	HFONT hOldFont_;
// 	HDC hMemDC_;
// 	HBITMAP hBmp_;
// 	HBITMAP hOldBmp_;

private:
	void InitializeMemDC();

public:
	tTVPSysFont();
	tTVPSysFont( const tTVPFont &font );
	~tTVPSysFont();

	int GetAscentHeight();
	void Assign( const tTVPSysFont* font );
	void Assign( const tTVPFont &font );
// 	void ApplyFont( const LOGFONT* info );
// 	void GetFont( LOGFONT* font ) const;
// 	HDC GetDC() { return hMemDC_; }
};

extern void TVPGetAllFontList(std::vector<ttstr>& list);
extern void TVPGetFontList(std::vector<ttstr> & list, tjs_uint32 flags, const tTVPFont & font);
extern tjs_uint8 TVPGetCharSetFromFaceName( const tjs_char* face );
#endif // __TVP_SYS_FONT_H__
