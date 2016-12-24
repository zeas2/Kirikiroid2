#pragma once
#include <vector>
#include "ProcessInfo.h"
#include "RenderFormats.h"

NS_KRMOVIE_BEGIN

class CDVDVideoCodec;
class CDVDAudioCodec;
class CDVDOverlayCodec;

class CDemuxStreamVideo;
struct CDVDStreamInfo;
class CDVDCodecOption;
class CDVDCodecOptions;


class CDVDFactoryCodec
{
public:
  static CDVDVideoCodec* CreateVideoCodec(CDVDStreamInfo &hint,
                                          CProcessInfo &processInfo,
                                          const CRenderInfo &info = CRenderInfo());
  static CDVDAudioCodec* CreateAudioCodec(CDVDStreamInfo &hint, CProcessInfo &processInfo,
                                          bool allowpassthrough = true, bool allowdtshddecode = true);
//  static CDVDOverlayCodec* CreateOverlayCodec(CDVDStreamInfo &hint );

  static CDVDAudioCodec* OpenCodec(CDVDAudioCodec* pCodec, CDVDStreamInfo &hint, CDVDCodecOptions &options );
  static CDVDVideoCodec* OpenCodec(CDVDVideoCodec* pCodec, CDVDStreamInfo &hint, CDVDCodecOptions &options );
//  static CDVDOverlayCodec* OpenCodec(CDVDOverlayCodec* pCodec, CDVDStreamInfo &hint, CDVDCodecOptions &options );
};

NS_KRMOVIE_END