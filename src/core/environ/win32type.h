#pragma once
#if 0
typedef long HRESULT;
typedef void* HANDLE;
typedef HANDLE HWND;

typedef struct tagRGBQUAD {
	uint8_t    rgbBlue;
	uint8_t    rgbGreen;
	uint8_t    rgbRed;
	uint8_t    rgbReserved;
} RGBQUAD;

typedef struct BITMAPINFOHEADER{
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
} *LPBITMAPINFOHEADER, *PBITMAPINFOHEADER;

typedef struct BITMAPINFO {
	BITMAPINFOHEADER    bmiHeader;
	RGBQUAD             bmiColors[1];
} *LPBITMAPINFO, *PBITMAPINFO;

typedef struct {
	int	left;
	int	top;
	int	right;
	int	bottom;
} RECT, RECTL;
#endif