#pragma once
#include "KRMovieDef.h"
NS_KRMOVIE_BEGIN
class IAudioCallback
{
public:
  IAudioCallback() {};
  virtual ~IAudioCallback() {};
  virtual void OnInitialize(int iChannels, int iSamplesPerSec, int iBitsPerSample) = 0;
  virtual void OnAudioData(const float* pAudioData, int iAudioDataLength) = 0;
};

NS_KRMOVIE_END