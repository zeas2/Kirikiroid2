#pragma once
#include "AudioCodec.h"
#include "AEAudioFormat.h"
#include "AEStreamInfo.h"
#include <memory>
NS_KRMOVIE_BEGIN
class CProcessInfo;

class CDVDAudioCodecPassthrough : public CDVDAudioCodec
{
public:
  CDVDAudioCodecPassthrough(CProcessInfo &processInfo);
  virtual ~CDVDAudioCodecPassthrough();

  virtual bool Open(CDVDStreamInfo &hints, CDVDCodecOptions &options);
  virtual void Dispose();
  virtual int Decode(uint8_t* pData, int iSize, double dts, double pts);
  virtual void GetData(DVDAudioFrame &frame);
  virtual int GetData(uint8_t** dst);
  virtual void Reset();
  virtual AEAudioFormat GetFormat() { return m_format; }
  virtual bool NeedPassthrough() { return true; }
  virtual const char* GetName() { return "passthrough"; }
  virtual int GetBufferSize();
private:
  CAEStreamParser m_parser;
  uint8_t* m_buffer;
  unsigned int m_bufferSize;
  unsigned int m_dataSize;
  AEAudioFormat m_format;
  uint8_t m_backlogBuffer[61440];
  unsigned int m_backlogSize;
  double m_currentPts;
  double m_nextPts;

  // TrueHD specifics
  std::unique_ptr<uint8_t[]> m_trueHDBuffer;
  unsigned int m_trueHDoffset;
};

NS_KRMOVIE_END