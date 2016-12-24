#pragma once
#include <utility>
#include <vector>
#include "AE.h"

NS_KRMOVIE_BEGIN
class CSetting;
class CAEStreamInfo;

class CAEFactory
{
public:
  static bool SupportsRaw(AEAudioFormat &format);
  static IAEStream *MakeStream(AEAudioFormat &audioFormat, unsigned int options = 0, IAEClockCallback *clock = NULL);
  static bool FreeStream(IAEStream *stream);
private:
  static IAE *AE;

  static void SettingOptionsAudioDevicesFillerGeneral(const CSetting *setting, std::vector< std::pair<std::string, std::string> > &list, std::string &current, bool passthrough);
};

NS_KRMOVIE_END