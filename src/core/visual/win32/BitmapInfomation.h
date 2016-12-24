

#ifndef __BITMAP_INFOMATION_H__
#define __BITMAP_INFOMATION_H__

struct TVPRGBQUAD {
	uint8_t    rgbBlue;
	uint8_t    rgbGreen;
	uint8_t    rgbRed;
	uint8_t    rgbReserved;
};

struct TVPBITMAPINFOHEADER{
	uint32_t biSize;
	int32_t            biWidth;
	int32_t            biHeight;
	uint16_t  biPlanes;
	uint16_t  biBitCount;
	uint32_t   biCompression;
	uint32_t   biSizeImage;
	int32_t            biXPelsPerMeter;
	int32_t            biYPelsPerMeter;
	uint32_t   biClrUsed;
	uint32_t   biClrImportant;
};

struct TVPBITMAPINFO {
	TVPBITMAPINFOHEADER    bmiHeader;
	TVPRGBQUAD             bmiColors[1];
};

class BitmapInfomation {
	TVPBITMAPINFO* BitmapInfo;
	tjs_int BitmapInfoSize;
public:
	BitmapInfomation( tjs_uint width, tjs_uint height, int bpp );
	~BitmapInfomation();

	inline unsigned int GetBPP() const { return BitmapInfo->bmiHeader.biBitCount; }
	inline bool Is32bit() const { return GetBPP() == 32; }
	inline bool Is8bit() const { return GetBPP() == 8; }
	inline int GetWidth() const { return BitmapInfo->bmiHeader.biWidth; }
	inline int GetHeight() const { return BitmapInfo->bmiHeader.biHeight; }
	inline tjs_uint GetImageSize() const { return BitmapInfo->bmiHeader.biSizeImage; }
	inline int GetPitchBytes() const { return GetImageSize()/GetHeight(); }
	BitmapInfomation& operator=(BitmapInfomation& r) {
		memcpy(BitmapInfo, r.BitmapInfo, BitmapInfoSize);
		return *this;
	}

	// 以下、Win32 のみのメソッド
	TVPBITMAPINFO* GetBITMAPINFO() { return BitmapInfo; }
	const TVPBITMAPINFO* GetBITMAPINFO() const { return BitmapInfo; }
};

#endif // __BITMAP_INFOMATION_H__

