
#define _USE_MATH_DEFINES
#include "GDIFontRasterizer.h"

#include "LayerBitmapIntf.h"
#include <math.h>

#include "FontSystem.h"
#include "TVPSysFont.h"
#include "SysInitImpl.h"

extern FontSystem* TVPFontSystem;


//---------------------------------------------------------------------------
void GDIFontRasterizer::InitChAntialiasMethod() {
	if(ChAntialiasMethodInit) return;

	ChAntialiasMethod = camAPI; // default

	tTJSVariant val;
	if( TVPGetCommandLine(TJS_W("-aamethod"), &val) ) {
		ttstr str(val);
#if 0 // まったく意味のないコード？
		if(str == TJS_W("auto"))
			; // nothing to do
#endif
		if(str == TJS_W("res8"))
			ChAntialiasMethod = camResample8;
		else if(str == TJS_W("res4"))
			ChAntialiasMethod = camResample4;
		else if(str == TJS_W("api"))
			ChAntialiasMethod = camAPI;
		else if(str == TJS_W("rgb"))
			ChAntialiasMethod = camSubpixelRGB;
		else if(str == TJS_W("bgr"))
			ChAntialiasMethod = camSubpixelBGR;
	}

	ChAntialiasMethodInit = true;
}
//---------------------------------------------------------------------------


GDIFontRasterizer::GDIFontRasterizer()
: RefCount(0), FontDC(NULL), NonBoldFontDC(NULL), LastBitmap(NULL), ChAntialiasMethodInit(false)
, ChUseResampling(false), ChAntialiasMethod(camResample8)
{
	AddRef();
}

GDIFontRasterizer::~GDIFontRasterizer() {
}
void GDIFontRasterizer::AddRef() {
	if( RefCount == 0 ) {
		FontDC = new tTVPSysFont();
		NonBoldFontDC = new tTVPSysFont();
	}
	RefCount ++;
}
void GDIFontRasterizer::Release() {
	RefCount --;
	LastBitmap = NULL;
	if( RefCount == 0 ) {
		delete FontDC;
		FontDC = NULL;
		delete NonBoldFontDC;
		NonBoldFontDC = NULL;

		delete this;
	}
}
void GDIFontRasterizer::ApplyFont( tTVPNativeBaseBitmap *bmp, bool force ) {
	if( bmp != LastBitmap || force ) {
		ApplyFont( bmp->GetFont() );
		LastBitmap = bmp;
	}
}
void GDIFontRasterizer::ApplyFont( const tTVPFont& font ) {
	LOGFONT LogFont={0};
	LogFont.lfHeight = -std::abs(font.Height);
	LogFont.lfItalic = (font.Flags & TVP_TF_ITALIC) ? TRUE:FALSE;
	LogFont.lfWeight = (font.Flags & TVP_TF_BOLD) ? 700 : 400;
	LogFont.lfUnderline = (font.Flags & TVP_TF_UNDERLINE) ? TRUE:FALSE;
	LogFont.lfStrikeOut = (font.Flags & TVP_TF_STRIKEOUT) ? TRUE:FALSE;
	LogFont.lfEscapement = LogFont.lfOrientation = font.Angle;
	LogFont.lfCharSet = DEFAULT_CHARSET;
	LogFont.lfOutPrecision = OUT_DEFAULT_PRECIS;
	LogFont.lfQuality = DEFAULT_QUALITY;
	LogFont.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
	std::wstring face = TVPFontSystem->GetBeingFont(font.Face.AsStdString());
	TJS_strncpy(LogFont.lfFaceName, face.c_str(), LF_FACESIZE -1);
	LogFont.lfFaceName[LF_FACESIZE-1] = 0;

	FontDC->ApplyFont( &LogFont );
	CurentLOGFONT = LogFont;
	int orgweight = CurentLOGFONT.lfWeight;
	CurentLOGFONT.lfWeight = 400;
	NonBoldFontDC->ApplyFont( &CurentLOGFONT );
	CurentLOGFONT.lfWeight = orgweight;
	LastBitmap = NULL;
}
void GDIFontRasterizer::GetTextExtent(tjs_char ch, tjs_int &w, tjs_int &h) {
	SIZE s;
	s.cx = 0;
	s.cy = 0;
	::GetTextExtentPoint32(NonBoldFontDC->GetDC(), &ch, 1, &s);

	w = s.cx;
	h = s.cy;

	if( CurentLOGFONT.lfWeight >= 700 ) {
		w += (int)(h / 50) + 1; // calculate bold font size
	}
}
tjs_int GDIFontRasterizer::GetAscentHeight() {
  return FontDC->GetAscentHeight();
}
tTVPCharacterData* GDIFontRasterizer::GetBitmap( const tTVPFontAndCharacterData & font, tjs_int aofsx, tjs_int aofsy ) {
	InitChAntialiasMethod();

	// setup GLYPHMETRICS and reveive buffer
	GLYPHMETRICS gm;
	ZeroMemory(&gm, sizeof(gm));
	static MAT2 no_transform_matrix = { {0,1}, {0,0}, {0,0}, {0,1} };
		// transformation matrix for intact conversion
		// | 1 0 |
		// | 0 1 |
	static MAT2 scale_4x_matrix = { {0,4}, {0,0}, {0,0}, {0,4} };
		// transformation matrix for 4x
		// | 4 0 |
		// | 0 4 |
	static MAT2 scale_8x_matrix = { {0,8}, {0,0}, {0,0}, {0,8} };
		// transformation matrix for 8x
		// | 8 0 |
		// | 0 8 |


	// determine format and transformation matrix
	tjs_int factor = 0;
	MAT2 *transmat;
	UINT format = font.Antialiased ? GGO_GRAY8_BITMAP : GGO_BITMAP;
	if(font.Antialiased)
	{
		switch(ChAntialiasMethod)
		{
		case camAPI:
			transmat = &no_transform_matrix;
			break;
		case camResample4:
			transmat = &scale_4x_matrix, factor = 2;
			format = GGO_BITMAP;
			break;
		case camSubpixelRGB:
		case camSubpixelBGR:
		case camResample8:
			transmat = &scale_8x_matrix, factor = 3;
			format = GGO_BITMAP;
			break;
		}
	}
	else
	{
		transmat = &no_transform_matrix;
	}


	// retrieve character code
	WORD code;
	// system supports UNICODE
	code = font.Character;

	// get buffer size and output dimensions
	int size = ::GetGlyphOutline( FontDC->GetDC(), code, format, &gm, 0, NULL, transmat);

	// set up structure's variables
	tTVPCharacterData * data = new tTVPCharacterData();

	{
		SIZE s;
		s.cx = 0;
		s.cy = 0;
		GetTextExtentPoint32( NonBoldFontDC->GetDC(), &font.Character, 1, &s);

		if(font.Font.Flags & TVP_TF_BOLD)
		{
			// calculate bold font width;
			// because sucking Win32 API returns different character size
			// between Win9x and WinNT, when we specified bold characters.
			// so we must alternatively calculate bold font size based on
			// non-bold font width.
			s.cx += (int)(s.cx / 50) + 1;
		}

		if(font.Font.Angle == 0)
		{
			data->Metrics.CellIncX = s.cx;
			data->Metrics.CellIncY = 0;
		}
		else if(font.Font.Angle == 2700)
		{
			data->Metrics.CellIncX = 0;
			data->Metrics.CellIncY = s.cx;
		}
		else
		{
			double angle = font.Font.Angle * (M_PI/1800);
			data->Metrics.CellIncX = static_cast<tjs_int>(  cos(angle) * s.cx);
			data->Metrics.CellIncY = static_cast<tjs_int>(- sin(angle) * s.cx);
		}
	}
	data->Gray = 65;
	data->BlackBoxX = gm.gmBlackBoxX;
	data->BlackBoxY = gm.gmBlackBoxY;
	data->OriginX =
		(gm.gmptGlyphOrigin.x) +
		(aofsx<<factor);
	data->OriginY = -gm.gmptGlyphOrigin.y + (aofsy<<factor);

	data->Antialiased = font.Antialiased;

	data->FullColored = false;

	data->Blured = font.Blured;
	data->BlurWidth = font.BlurWidth;
	data->BlurLevel = font.BlurLevel;

	if(size == 0 || gm.gmBlackBoxY == 0)
	{
		data->BlackBoxX = 0;
		data->BlackBoxY = 0;
	}
	else
	{

		if(format == GGO_GRAY8_BITMAP)
		{
			data->Pitch = (size / gm.gmBlackBoxY) & ~0x03;
				// data is aligned to DWORD
			/*
				data->Pitch = (((gm.gmBlackBoxX -1)>>2)+1)<<2 seems to be proper,
				but does not work with some characters.
			*/
		}
		else
		{
			data->Pitch = (((gm.gmBlackBoxX -1)>>5)+1)<<2;
				// data is aligned to DWORD
		}

		data->Alloc(size);

		try
		{
			// draw to buffer
			::GetGlyphOutline( FontDC->GetDC(), code, format, &gm, size, data->GetData(), transmat);

			if( (ChUseResampling ) && (font.Font.Flags & TVP_TF_BOLD) ) {
				// our sucking win9x based OS cannot output bold characters
				// even if BOLD is specified.
				if(format == GGO_BITMAP)
					data->Bold2(font.Font.Height<<factor);
				else
					data->Bold(font.Font.Height<<factor);
					// so here we go a nasty way ...
			}

			if(!font.Antialiased)
			{
				// not antialiased
				data->Expand(); // nasty...
			}
			else
			{
				// antialiased
				switch(ChAntialiasMethod)
				{
				case camAPI:
					break;
				case camResample4:
					data->Resample4();
					break;
				case camResample8:
					data->Resample8();
					break;
				case camSubpixelRGB:
//						data->ResampleRGB();
					break;
				case camSubpixelBGR:
//						data->ResampleBGR();
					break;
				}
			}

			// apply blur
			if(font.Blured) data->Blur(); // nasty ...
		}
		catch(...)
		{
			data->Release();
			throw;
		}
	}
	if( font.Font.Flags & (TVP_TF_UNDERLINE|TVP_TF_STRIKEOUT) ) {
		OUTLINETEXTMETRIC otm = { sizeof(OUTLINETEXTMETRIC) };
		::GetOutlineTextMetrics( FontDC->GetDC(), otm.otmSize, &otm );
		tjs_int y = GetAscentHeight();
		if( font.Font.Flags & TVP_TF_UNDERLINE ) {
			tjs_int liney = y - otm.otmsUnderscorePosition;
			tjs_int height = otm.otmTextMetrics.tmHeight;
			tjs_int thickness = otm.otmsUnderscoreSize;
			if( liney >= height ) liney = height - 1;
			if( liney >= 0 && thickness > 0 ) {
				data->AddHorizontalLine( liney, thickness, 64 );
			}
		}
		if( font.Font.Flags & TVP_TF_STRIKEOUT ) {
			tjs_int liney = y - otm.otmsStrikeoutPosition;
			tjs_int thickness =  otm.otmsStrikeoutSize;
			if( liney >= 0 && thickness > 0 ) {
				data->AddHorizontalLine( liney, thickness, 64 );
			}
		}
	}
	return data;
}
void GDIFontRasterizer::GetGlyphDrawRect( const ttstr & text, tTVPRect& area )
{
	static MAT2 no_transform_matrix = { {0,1}, {0,0}, {0,0}, {0,1} };
	GLYPHMETRICS gm;

	tjs_int ascent = GetAscentHeight();

	LOGFONT LogFont;
	FontDC->GetFont( &LogFont );
	OUTLINETEXTMETRIC otm = { sizeof(OUTLINETEXTMETRIC) };
	if( LogFont.lfUnderline || LogFont.lfStrikeOut ) {
		::GetOutlineTextMetrics( FontDC->GetDC(), otm.otmSize, &otm );
	}

	area.left = area.top = area.right = area.bottom = 0;
	tjs_int offsetx = 0;
	tjs_int offsety = 0;
	tjs_uint len = text.length();
	for( tjs_uint i = 0; i < len; i++ ) {
		tjs_char code = text[i];
		ZeroMemory(&gm, sizeof(gm));
		int err = ::GetGlyphOutline( FontDC->GetDC(), code, GGO_METRICS, &gm, 0, NULL, &no_transform_matrix );
		SIZE s = {0,0};
		if( err != GDI_ERROR ) {
			::GetTextExtentPoint32( NonBoldFontDC->GetDC(), &code, 1, &s );
			tjs_int w = gm.gmBlackBoxX;
			tjs_int h = gm.gmBlackBoxY;
			tjs_int l = gm.gmptGlyphOrigin.x;
			tjs_int t = ascent - gm.gmptGlyphOrigin.y;
			tTVPRect rt( l, t, l+w, t+h );
			if( LogFont.lfUnderline ) {
				tjs_int liney = ascent - otm.otmsUnderscorePosition;
				tjs_int height = otm.otmTextMetrics.tmHeight;
				tjs_int thickness = otm.otmsUnderscoreSize;
				if( liney >= height ) liney = height - 1;
				if( liney >= 0 && thickness > 0 ) {
					if( rt.left > 0 ) rt.left = 0;
					if( rt.right < s.cx ) rt.right = s.cx;
					if( liney < rt.top ) rt.top = liney;
					if( (liney+thickness) >= rt.bottom ) rt.bottom = liney+thickness+1;
				}
			}
			if( LogFont.lfStrikeOut ) {
				tjs_int liney = ascent - otm.otmsStrikeoutPosition;
				tjs_int thickness =  otm.otmsStrikeoutSize;
				if( liney >= 0 && thickness > 0 ) {
					if( rt.left > 0 ) rt.left = 0;
					if( rt.right < s.cx ) rt.right = s.cx;
					if( liney < rt.top ) rt.top = liney;
					if( (liney+thickness) >= rt.bottom ) rt.bottom = liney+thickness+1;
				}
			}
			rt.add_offsets( offsetx, offsety );
			if( i != 0 ) {
				area.do_union( rt );
			} else {
				area = rt;
			}
		}
		offsetx += s.cx;
		offsety = 0;
	}
}


