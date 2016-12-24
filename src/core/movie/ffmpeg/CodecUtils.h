#pragma once
#include "VideoCodec.h"
#include "RenderFormats.h"

extern "C" {
#include "libavutil/pixfmt.h"
}

NS_KRMOVIE_BEGIN
struct YV12Image;

class CDVDCodecUtils
{
public:
  static DVDVideoPicture* AllocatePicture(int iWidth, int iHeight);
  static void FreePicture(DVDVideoPicture* pPicture);
  static bool CopyPicture(DVDVideoPicture* pDst, DVDVideoPicture* pSrc);
  static bool CopyPicture(YV12Image* pDst, DVDVideoPicture *pSrc);
  
  static DVDVideoPicture* ConvertToNV12Picture(DVDVideoPicture *pSrc);
  static DVDVideoPicture* ConvertToYUV422PackedPicture(DVDVideoPicture *pSrc, ERenderFormat format);
  static bool CopyNV12Picture(YV12Image* pImage, DVDVideoPicture *pSrc);
  static bool CopyYUV422PackedPicture(YV12Image* pImage, DVDVideoPicture *pSrc);

  static bool IsVP3CompatibleWidth(int width);

  static double NormalizeFrameduration(double frameduration, bool *match = NULL);

  static ERenderFormat EFormatFromPixfmt(int fmt);
  static AVPixelFormat PixfmtFromEFormat(ERenderFormat format);
};

NS_KRMOVIE_END