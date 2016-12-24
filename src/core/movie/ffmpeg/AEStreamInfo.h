#pragma once
#include "KRMovieDef.h"
#include "AEChannelInfo.h"
#include <stdint.h>

/* ffmpeg re-defines this, so undef it to squash the warning */
#undef restrict

extern "C" {
#include "libavutil/crc.h"
}

NS_KRMOVIE_BEGIN
#define MAX_IEC61937_PACKET  61440
class CAEStreamInfo
{
public:
  CAEStreamInfo();
  double GetDuration();
  bool operator==(const CAEStreamInfo& info) const;

  enum DataType
  {
    STREAM_TYPE_NULL,
    STREAM_TYPE_AC3,
    STREAM_TYPE_DTS_512,
    STREAM_TYPE_DTS_1024,
    STREAM_TYPE_DTS_2048,
    STREAM_TYPE_DTSHD,
    STREAM_TYPE_DTSHD_CORE,
    STREAM_TYPE_EAC3,
    STREAM_TYPE_MLP,
    STREAM_TYPE_TRUEHD
  };
  DataType m_type;
  unsigned int m_sampleRate;
  unsigned int m_channels;
  bool m_dataIsLE;
  unsigned int m_dtsPeriod;
  unsigned int m_repeat;
  unsigned int m_ac3FrameSize;
};

class CAEStreamParser
{
public:

  CAEStreamParser();
  ~CAEStreamParser();

  int AddData(uint8_t *data, unsigned int size, uint8_t **buffer = NULL, unsigned int *bufferSize = 0);

  void SetCoreOnly(bool value) { m_coreOnly = value; }
  unsigned int IsValid() { return m_hasSync; }
  unsigned int GetSampleRate() { return m_info.m_sampleRate; }
  unsigned int GetChannels() { return m_info.m_channels; }
  unsigned int GetFrameSize() { return m_fsize; }
  // unsigned int GetDTSBlocks() { return m_dtsBlocks; }
  unsigned int GetDTSPeriod() { return m_info.m_dtsPeriod; }
  unsigned int GetEAC3BlocksDiv() { return m_info.m_repeat; }
  enum CAEStreamInfo::DataType GetDataType() { return m_info.m_type; }
  bool IsLittleEndian() { return m_info.m_dataIsLE; }
  unsigned int GetBufferSize() { return m_bufferSize; }
  CAEStreamInfo& GetStreamInfo() { return m_info; }
  void Reset();

private:
  uint8_t m_buffer[MAX_IEC61937_PACKET];
  unsigned int m_bufferSize;
  unsigned int m_skipBytes;

  typedef unsigned int (CAEStreamParser::*ParseFunc)(uint8_t *data, unsigned int size);

  CAEStreamInfo m_info;
  bool m_coreOnly;
  unsigned int m_needBytes;
  ParseFunc m_syncFunc;
  bool m_hasSync;

  unsigned int m_coreSize;         /* core size for dtsHD */
  unsigned int m_dtsBlocks;
  unsigned int m_fsize;
  unsigned int m_fsizeMain;        /* used for EAC3 substreams */
  int m_substreams;       /* used for TrueHD  */
  AVCRC m_crcTrueHD[1024];  /* TrueHD crc table */

  void GetPacket(uint8_t **buffer, unsigned int *bufferSize);
  unsigned int DetectType(uint8_t *data, unsigned int size);
  unsigned int SyncAC3(uint8_t *data, unsigned int size);
  unsigned int SyncDTS(uint8_t *data, unsigned int size);
  unsigned int SyncTrueHD(uint8_t *data, unsigned int size);

  static unsigned int GetTrueHDChannels(const uint16_t chanmap);
};

NS_KRMOVIE_END