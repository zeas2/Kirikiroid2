#pragma once
#include "KRMovieDef.h"
#include "AudioCodec.h"

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/avutil.h"
#include "libswresample/swresample.h"
}

NS_KRMOVIE_BEGIN
class CProcessInfo;

class CDVDAudioCodecFFmpeg : public CDVDAudioCodec
{
public:
  CDVDAudioCodecFFmpeg(CProcessInfo &processInfo);
  virtual ~CDVDAudioCodecFFmpeg();
  virtual bool Open(CDVDStreamInfo &hints, CDVDCodecOptions &options);
  virtual void Dispose();
  virtual int Decode(uint8_t* pData, int iSize, double dts, double pts);
  virtual void GetData(DVDAudioFrame &frame);
  virtual int GetData(uint8_t** dst);
  virtual void Reset();
  virtual AEAudioFormat GetFormat() { return m_format; }
  virtual const char* GetName() { return "FFmpeg"; }
  virtual enum AVMatrixEncoding GetMatrixEncoding();
  virtual enum AVAudioServiceType GetAudioServiceType();
  virtual int GetProfile();

protected:
  enum AEDataFormat GetDataFormat();
  int GetSampleRate();
  int GetChannels();
  CAEChannelInfo GetChannelMap();
  int GetBitRate();

  AEAudioFormat m_format;
  AVCodecContext* m_pCodecContext;
  enum AVSampleFormat m_iSampleFormat;
  CAEChannelInfo m_channelLayout;
  enum AVMatrixEncoding m_matrixEncoding;

  AVFrame* m_pFrame1;
  int m_gotFrame;

  int m_channels;
  uint64_t m_layout;

  void BuildChannelMap();
  void ConvertToFloat();
};

NS_KRMOVIE_END