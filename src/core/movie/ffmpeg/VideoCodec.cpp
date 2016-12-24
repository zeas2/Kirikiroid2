#include "VideoCodec.h"
NS_KRMOVIE_BEGIN
bool CDVDVideoCodec::IsSettingVisible(const std::string &condition, const std::string &value, const CSetting *setting, void *data)
{
  if (setting == NULL || value.empty())
    return false;
#if 0
  const std::string &settingId = setting->GetId();

  // check if we are running on nvidia hardware
  std::string gpuvendor = g_Windowing.GetRenderVendor();
  std::transform(gpuvendor.begin(), gpuvendor.end(), gpuvendor.begin(), ::tolower);
  bool isNvidia = (gpuvendor.compare(0, 6, "nvidia") == 0);
  bool isIntel = (gpuvendor.compare(0, 5, "intel") == 0);

  // nvidia does only need mpeg-4 setting
  if (isNvidia)
  {
    if (settingId == CSettings::SETTING_VIDEOPLAYER_USEVDPAUMPEG4)
      return true;

    return false; // will also hide intel settings on nvidia hardware
  }
  else if (isIntel) // intel needs vc1, mpeg-2 and mpeg4 setting
  {
    if (settingId == CSettings::SETTING_VIDEOPLAYER_USEVAAPIMPEG4)
      return true;
    if (settingId == CSettings::SETTING_VIDEOPLAYER_USEVAAPIVC1)
      return true;
    if (settingId == CSettings::SETTING_VIDEOPLAYER_USEVAAPIMPEG2)
      return true;

    return false; // this will also hide nvidia settings on intel hardware
  }
#endif
  // if we don't know the hardware we are running on e.g. amd oss vdpau 
  // or fglrx with xvba-driver we show everything
  return true;
}

bool CDVDVideoCodec::IsCodecDisabled(const std::map<AVCodecID, std::string> &map, AVCodecID id)
{
  auto codec = map.find(id);
  if (codec != map.end())
  {
	  return false;
//     return (!CSettings::GetInstance().GetBool(codec->second) ||
//             !CDVDVideoCodec::IsSettingVisible("unused", "unused",
//                                               CSettings::GetInstance().GetSetting(codec->second),
//                                               NULL));
  }
  return false; // don't disable what we don't have
}
NS_KRMOVIE_END