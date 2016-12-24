#pragma once
#include "KRMovieDef.h"

#if (defined HAVE_CONFIG_H) && (!defined TARGET_WINDOWS)
  #include "config.h"
#endif
#include <vector>

extern "C" {
#include "libavcodec/avcodec.h"
}
#include "AEAudioFormat.h"
#include "ProcessInfo.h"

NS_KRMOVIE_BEGIN
struct AVStream;

struct CDVDStreamInfo;
class CDVDCodecOption;
class CDVDCodecOptions;

typedef struct stDVDAudioFrame
{
  uint8_t* data[16];
  double pts;
  bool hasTimestamp;
  double duration;
  unsigned int nb_frames;
  unsigned int framesize;
  unsigned int planes;

  AEAudioFormat format;
  int bits_per_sample;
  bool passthrough;
  AEAudioFormat audioFormat;
  enum AVAudioServiceType audio_service_type;
  enum AVMatrixEncoding matrix_encoding;
  int               profile;
} DVDAudioFrame;

class CDVDAudioCodec
{
public:

  CDVDAudioCodec(CProcessInfo &processInfo) : m_processInfo(processInfo) {}
  virtual ~CDVDAudioCodec() {}

  /*
   * Open the decoder, returns true on success
   */
  virtual bool Open(CDVDStreamInfo &hints, CDVDCodecOptions &options) = 0;

  /*
   * Dispose, Free all resources
   */
  virtual void Dispose() = 0;

  /*
   * returns bytes used or -1 on error
   *
   */
  virtual int Decode(uint8_t* pData, int iSize, double dts, double pts) = 0;

  /*
   * returns nr of bytes in decode buffer
   * the data is valid until the next Decode call
   */
  virtual int GetData(uint8_t** dst) = 0;

  /*
   * the data is valid until the next Decode call
   */
  virtual void GetData(DVDAudioFrame &frame) = 0;

  /*
   * resets the decoder
   */
  virtual void Reset() = 0;

  /*
   * returns the format for the audio stream
   */
  virtual AEAudioFormat GetFormat() = 0;

  /*
   * should return the average input bit rate
   */
  virtual int GetBitRate() { return 0; }

  /*
   * returns if the codec requests to use passtrough
   */
  virtual bool NeedPassthrough() { return false; }

  /*
   * should return codecs name
   */
  virtual const char* GetName() = 0;

  /*
   * should return amount of data decoded has buffered in preparation for next audio frame
   */
  virtual int GetBufferSize() { return 0; }

  /*
   * should return the ffmpeg matrix encoding type
   */
  virtual enum AVMatrixEncoding GetMatrixEncoding() { return AV_MATRIX_ENCODING_NONE; }

  /*
   * should return the ffmpeg audio service type
   */
  virtual enum AVAudioServiceType GetAudioServiceType() { return AV_AUDIO_SERVICE_TYPE_MAIN; }

  /*
   * should return the ffmpeg profile value
   */
  virtual int GetProfile() { return 0; }

protected:
  CProcessInfo &m_processInfo;
};
NS_KRMOVIE_END