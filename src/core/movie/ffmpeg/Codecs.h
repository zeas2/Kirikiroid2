#pragma once
#include "KRMovieDef.h"
#if (defined HAVE_CONFIG_H) && (!defined TARGET_WINDOWS)
  #include "config.h"
#endif

#include "RenderFormats.h"
#include <string>

NS_KRMOVIE_BEGIN
// 0x100000 is the video starting range

// 0x200000 is the audio starting range

// special options that can be passed to a codec
class CDVDCodecOption
{
public:
  CDVDCodecOption(const std::string& name, const std::string& value) : m_name(name), m_value(value) {}
  std::string m_name;
  std::string m_value;
};

class CDVDCodecOptions
{
public:
  std::vector<CDVDCodecOption> m_keys;
  std::vector<ERenderFormat> m_formats;
  const void *m_opaque_pointer;
};
NS_KRMOVIE_END