#pragma once
#include "KRMovieDef.h"
extern "C" {
#include "libavcodec/avcodec.h"
}
#include <vector>
#include <string>

NS_KRMOVIE_BEGIN
enum StreamType
{
	STREAM_NONE = 0,// if unknown
	STREAM_AUDIO,   // audio stream
	STREAM_VIDEO,   // video stream
	STREAM_DATA,    // data stream
// 	STREAM_SUBTITLE,// subtitle stream
// 	STREAM_TELETEXT, // Teletext data stream
// 	STREAM_RADIO_RDS // Radio RDS data stream
};

enum StreamSource {
	STREAM_SOURCE_NONE = 0x000,
	STREAM_SOURCE_DEMUX = 0x100,
// 	STREAM_SOURCE_NAV = 0x200,
// 	STREAM_SOURCE_DEMUX_SUB = 0x300,
	STREAM_SOURCE_TEXT = 0x400,
	STREAM_SOURCE_VIDEOMUX = 0x500
};

#define STREAM_SOURCE_MASK(a) ((a) & 0xf00)

class CDemuxStream
{
public:
	CDemuxStream()
	{
		uniqueId = 0;
		dvdNavId = 0;
		demuxerId = -1;
		codec = (AVCodecID)0; // AV_CODEC_ID_NONE
		codec_fourcc = 0;
		profile = FF_PROFILE_UNKNOWN;
		level = FF_LEVEL_UNKNOWN;
		type = STREAM_NONE;
		source = STREAM_SOURCE_NONE;
		iDuration = 0;
		pPrivate = NULL;
		ExtraData = NULL;
		ExtraSize = 0;
		memset(language, 0, sizeof(language));
		disabled = false;
		changes = 0;
		flags = FLAG_NONE;
		realtime = false;
		bandwidth = 0;
	}

	virtual ~CDemuxStream()
	{
		delete[] ExtraData;
	}

	virtual std::string GetStreamName() { return ""; }

	int uniqueId;          // unique stream id
	int dvdNavId;
	int64_t demuxerId; // id of the associated demuxer
	AVCodecID codec;
	unsigned int codec_fourcc; // if available
	int profile; // encoder profile of the stream reported by the decoder. used to qualify hw decoders.
	int level;   // encoder level of the stream reported by the decoder. used to qualify hw decoders.
	StreamType type;
	int source;
	bool realtime;
	unsigned int bandwidth;

	int iDuration; // in mseconds
	void* pPrivate; // private pointer for the demuxer
	uint8_t*     ExtraData; // extra data for codec to use
	unsigned int ExtraSize; // size of extra data

	char language[4]; // ISO 639 3-letter language code (empty string if undefined)
	bool disabled; // set when stream is disabled. (when no decoder exists)

	std::string codecName;

	int  changes; // increment on change which player may need to know about

	enum EFlags
	{
		FLAG_NONE = 0x0000
		, FLAG_DEFAULT = 0x0001
		, FLAG_DUB = 0x0002
		, FLAG_ORIGINAL = 0x0004
		, FLAG_COMMENT = 0x0008
		, FLAG_LYRICS = 0x0010
		, FLAG_KARAOKE = 0x0020
		, FLAG_FORCED = 0x0040
		, FLAG_HEARING_IMPAIRED = 0x0080
		, FLAG_VISUAL_IMPAIRED = 0x0100
	} flags;
};

class CDemuxStreamVideo : public CDemuxStream
{
public:
	CDemuxStreamVideo() : CDemuxStream()
	{
		iFpsScale = 0;
		iFpsRate = 0;
		iHeight = 0;
		iWidth = 0;
		fAspect = 0.0;
		bVFR = false;
		bPTSInvalid = false;
		bForcedAspect = false;
		type = STREAM_VIDEO;
		iOrientation = 0;
		iBitsPerPixel = 0;
	}

	virtual ~CDemuxStreamVideo() {}
	int iFpsScale; // scale of 1000 and a rate of 29970 will result in 29.97 fps
	int iFpsRate;
	int iHeight; // height of the stream reported by the demuxer
	int iWidth; // width of the stream reported by the demuxer
	float fAspect; // display aspect of stream
	bool bVFR;  // variable framerate
	bool bPTSInvalid; // pts cannot be trusted (avi's).
	bool bForcedAspect; // aspect is forced from container
	int iOrientation; // orientation of the video in degress counter clockwise
	int iBitsPerPixel;
	std::string stereo_mode; // expected stereo mode
};

class CDemuxStreamAudio : public CDemuxStream
{
public:
	CDemuxStreamAudio() : CDemuxStream()
	{
		iChannels = 0;
		iSampleRate = 0;
		iBlockAlign = 0;
		iBitRate = 0;
		iBitsPerSample = 0;
		iChannelLayout = 0;
		type = STREAM_AUDIO;
	}

	virtual ~CDemuxStreamAudio() {}

	std::string GetStreamType();

	int iChannels;
	int iSampleRate;
	int iBlockAlign;
	int iBitRate;
	int iBitsPerSample;
	uint64_t iChannelLayout;
};

struct DemuxPacket;
class IDemux {
public:
	IDemux() : m_demuxerId(NewGuid()) {}
	virtual ~IDemux() {}
	virtual void Reset() = 0;
	/*
	* Aborts any internal reading that might be stalling main thread
	* NOTICE - this can be called from another thread
	*/
	virtual void Abort() = 0;
	virtual void Flush() = 0;
	virtual DemuxPacket* Read() = 0;
	virtual bool SeekTime(int time, bool backwords = false, double* startpts = NULL) = 0;
	virtual void SetSpeed(int iSpeed) = 0;
	virtual int GetStreamLength() = 0;
	virtual CDemuxStream* GetStream(int64_t demuxerId, int iStreamId) const { return GetStream(iStreamId); };
	virtual std::vector<CDemuxStream*> GetStreams() const = 0;
	virtual int GetNrOfStreams() const = 0;
	virtual std::string GetStreamCodecName(int64_t demuxerId, int iStreamId) { return GetStreamCodecName(iStreamId); };
	virtual void EnableStream(int64_t demuxerId, int id, bool enable) { EnableStream(id, enable); };
	virtual void SetVideoResolution(int width, int height) {};
	int64_t GetDemuxerId() { return m_demuxerId; };
protected:
	virtual void EnableStream(int id, bool enable) {};
	virtual CDemuxStream* GetStream(int iStreamId) const = 0;
	virtual std::string GetStreamCodecName(int iStreamId) { return ""; };
	int GetNrOfStreams(StreamType streamType);
	int64_t m_demuxerId;
private:
	int64_t NewGuid();
};
NS_KRMOVIE_END
