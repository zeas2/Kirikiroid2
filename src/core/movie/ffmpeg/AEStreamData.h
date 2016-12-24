#pragma once
#include "KRMovieDef.h"
NS_KRMOVIE_BEGIN
/**
 * Bit options to pass to IAE::MakeStream
 */
enum AEStreamOptions
{
  AESTREAM_FORCE_RESAMPLE = 1 << 0,   /* force resample even if rates match */
  AESTREAM_PAUSED         = 1 << 1,   /* create the stream paused */
  AESTREAM_AUTOSTART      = 1 << 2,   /* autostart the stream when enough data is buffered */
  AESTREAM_BYPASS_ADSP    = 1 << 3    /* if this option is set the ADSP-System is bypassed and the raw stream will be passed through IAESink */
};
NS_KRMOVIE_END