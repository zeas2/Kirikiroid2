//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Wave Player interface
//---------------------------------------------------------------------------
#include "tjsCommHead.h"

#include <algorithm>
#include "WaveIntf.h"
#include "EventIntf.h"
#include "StorageIntf.h"
#include "MsgIntf.h"
#include "UtilStreams.h"
#include "WaveLoopManager.h"
#include "tjsDictionary.h"
#include "VorbisWaveDecoder.h"
#include "FFWaveDecoder.h"


//---------------------------------------------------------------------------
// PCM related constants / definitions
//---------------------------------------------------------------------------

#ifndef WAVE_FORMAT_PCM
	#define WAVE_FORMAT_PCM 0x0001
#endif
#ifndef WAVE_FORMAT_IEEE_FLOAT
	#define WAVE_FORMAT_IEEE_FLOAT 0x0003
#endif
#ifndef WAVE_FORMAT_EXTENSIBLE
	#define WAVE_FORMAT_EXTENSIBLE 0xFFFE
#endif

#ifndef SPEAKER_FRONT_LEFT
// from Windows ksmedia.h
// speaker config

#define     SPEAKER_FRONT_LEFT              0x1
#define     SPEAKER_FRONT_RIGHT             0x2
#define     SPEAKER_FRONT_CENTER            0x4
#define     SPEAKER_LOW_FREQUENCY           0x8
#define     SPEAKER_BACK_LEFT               0x10
#define     SPEAKER_BACK_RIGHT              0x20
#define     SPEAKER_FRONT_LEFT_OF_CENTER    0x40
#define     SPEAKER_FRONT_RIGHT_OF_CENTER   0x80
#define     SPEAKER_BACK_CENTER             0x100
#define     SPEAKER_SIDE_LEFT               0x200
#define     SPEAKER_SIDE_RIGHT              0x400
#define     SPEAKER_TOP_CENTER              0x800
#define     SPEAKER_TOP_FRONT_LEFT          0x1000
#define     SPEAKER_TOP_FRONT_CENTER        0x2000
#define     SPEAKER_TOP_FRONT_RIGHT         0x4000
#define     SPEAKER_TOP_BACK_LEFT           0x8000
#define     SPEAKER_TOP_BACK_CENTER         0x10000
#define     SPEAKER_TOP_BACK_RIGHT          0x20000

#endif
//---------------------------------------------------------------------------






//---------------------------------------------------------------------------
// GUIDs used in WAVRFORMATEXTENSIBLE
//---------------------------------------------------------------------------
tjs_uint8 TVP_GUID_KSDATAFORMAT_SUBTYPE_PCM[16] =
{ 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00,
  0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 };
tjs_uint8 TVP_GUID_KSDATAFORMAT_SUBTYPE_IEEE_FLOAT[16] =
{ 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00,
  0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 };
//---------------------------------------------------------------------------


#include "DetectCPU.h"
//#include "tvpgl_ia32_intf.h"

//---------------------------------------------------------------------------
// CPU specific optimized routine prototypes
//---------------------------------------------------------------------------
extern void (*PCMConvertLoopInt16ToFloat32)(void * dest, const void * src, size_t numsamples);
extern void (*PCMConvertLoopFloat32ToInt16)(void * dest, const void * src, size_t numsamples);
#if 0
extern void PCMConvertLoopInt16ToFloat32_sse(void * __restrict dest, const void * __restrict src, size_t numsamples);
extern void PCMConvertLoopFloat32ToInt16_sse(void * __restrict dest, const void * __restrict src, size_t numsamples);
#endif


//---------------------------------------------------------------------------
// Wave format convertion routines
//---------------------------------------------------------------------------
static void TVPConvertFloatPCMTo16bits(tjs_int16 *output, const float *input,
	tjs_int channels, tjs_int count, bool downmix)
{
	// convert 32bit float to 16bit integer

	// float PCM is in range of +1.0 ... 0 ... -1.0
	// clip sample which is out of the range.

	if(!downmix)
	{
		tjs_int total = channels * count;
#if 0
		bool use_sse =
				(TVPCPUType & TVP_CPU_HAS_MMX) &&
				(TVPCPUType & TVP_CPU_HAS_SSE) &&
				(TVPCPUType & TVP_CPU_HAS_CMOV);

		if(use_sse)
			PCMConvertLoopFloat32ToInt16_sse(output, input, total);
		else
			PCMConvertLoopFloat32ToInt16(output, input, total);
#else
		PCMConvertLoopFloat32ToInt16(output, input, total);
#endif
	}
	else
	{
		float nc = 32768.0f / (float)channels;
		while(count--)
		{
			tjs_int n = channels;
			float t = 0;
			while(n--) t += *(input++) * nc;
			if(t > 0)
			{
				int i = (int)(t + 0.5);
				if(i > 32767) i = 32767;
				*(output++) = (tjs_int16)i;
			}
			else
			{
				int i = (int)(t - 0.5);
				if(i < -32768) i = -32768;
				*(output++) = (tjs_int16)i;
			}
		}
	}
}
//---------------------------------------------------------------------------
static void TVPConvertIntegerPCMTo16bits(tjs_int16 *output, const void *input,
	tjs_int bytespersample,
	tjs_int validbits, tjs_int channels, tjs_int count, bool downmix)
{
	// convert integer PCMs to 16bit integer PCM
#define PROCESS_BY_CHANNELS \
	switch(channels) \
	{ \
	case 2: PROCESS(2); break; \
	case 4: PROCESS(4); break; \
	case 8: PROCESS(8); break; \
	default: PROCESS(channels); break; \
	}


#if TJS_HOST_IS_BIG_ENDIAN
	#define GET_24BIT(p) (p[2] + (p[1] << 8) + (p[0] << 16))
#else
	#define GET_24BIT(p) (p[0] + (p[1] << 8) + (p[2] << 16))
#endif

	if(bytespersample == 1)
	{
		// here assumes that the input 8bit PCM has always 8bit valid data
		const tjs_int8 *p = (tjs_int8 *)input;
		if(!downmix || channels == 1)
		{
			tjs_int total = channels * count;
			while(total--)
				*(output++) = (tjs_int16)( ((tjs_int)*(p++)-0x80) * 0x100);
		}
		else
		{
		#define PROCESS(channels) \
			while(count--) \
			{ \
				tjs_int v = 0; \
				tjs_int n = channels; \
				while(n--) \
					v += (tjs_int16)( ((tjs_int)*(p++)-0x80) * 0x100); \
				v = v / channels; \
				*(output++) = (tjs_int16)v; \
			}
		PROCESS_BY_CHANNELS
		#undef PROCESS
		}
	}
	else if(bytespersample == 2)
	{
		tjs_uint16 mask =  ~( (1 << (16 - validbits)) - 1);
		const tjs_int16 *p = (const tjs_int16 *)input;
		if(!downmix || channels == 1)
		{
			tjs_int total = channels * count;
			while(total--) *(output++) = (tjs_int16)(*(p++) & mask);
		}
		else
		{
		#define PROCESS(channels) \
			while(count--) \
			{ \
				tjs_int v = 0; \
				tjs_int n = channels; \
				while(n--) \
					v += (tjs_int16)(*(p++) & mask); \
				v = v / channels; \
				*(output++) = (tjs_int16)v; \
			}
		PROCESS_BY_CHANNELS
		#undef PROCESS
		}
	}
	else if(bytespersample == 3)
	{
		tjs_uint32 mask = ~( (1 << (24 - validbits)) - 1);
		const tjs_uint8 *p = (const tjs_uint8 *)input;

		if(!downmix || channels == 1)
		{
			tjs_int total = channels * count;
			while(total--)
			{
				tjs_int32 t = GET_24BIT(p);
				p += 3;
				t |= -(t&0x800000); // extend sign
				t &= mask; // apply mask
				t >>= 8;
				*(output++) = (tjs_int16)t;
			}
		}
		else
		{
		#define PROCESS(channels) \
			while(count--) \
			{ \
				tjs_int v = 0; \
				tjs_int n = channels; \
				while(n--) \
				{ \
					tjs_int32 t = GET_24BIT(p); \
					p += 3; \
					t |= -(t&0x800000); \
					t &= mask; \
					t >>= 8; \
					v += t; \
				} \
				v = v / channels; \
				*(output++) = (tjs_int16)v; \
			}
		PROCESS_BY_CHANNELS
		#undef PROCESS
		}
	}
	else if(bytespersample == 4)
	{
		tjs_int32 mask = ~( (1 << (32 - validbits)) - 1);
		const tjs_int32 *p = (const tjs_int32 *)input;
		if(!downmix || channels == 1)
		{
			tjs_int total = channels * count;
			while(total--)
				*(output++) = (tjs_int16)((*(p++) & mask) >> 16);
		}
		else
		{
		#define PROCESS(channels) \
			while(count--) \
			{ \
				tjs_int v = 0; \
				tjs_int n = channels; \
				while(n--) \
					v += (tjs_int16)((*(p++) & mask) >> 16); \
				v = v / channels; \
				*(output++) = (tjs_int16)v; \
			}
		PROCESS_BY_CHANNELS
		#undef PROCESS
		}
	}

#undef PROCESS_BY_CHANNELS
#undef GET_24BIT
}
//---------------------------------------------------------------------------
void TVPConvertPCMTo16bits(tjs_int16 *output, const void *input,
	tjs_int channels, tjs_int bytespersample, tjs_int bitspersample, bool isfloat,
	tjs_int count, bool downmix)
{
	// cconvert specified format to 16bit PCM

	if(isfloat)
		TVPConvertFloatPCMTo16bits(output, (const float *)input, channels, count, downmix);
	else
		TVPConvertIntegerPCMTo16bits(output, input,
			bytespersample, bitspersample, channels, count, downmix);
}
//---------------------------------------------------------------------------
void TVPConvertPCMTo16bits(tjs_int16 *output, const void *input,
	const tTVPWaveFormat &format, tjs_int count, bool downmix)
{
	// cconvert specified format to 16bit PCM
	TVPConvertPCMTo16bits(output, input, format.Channels, format.BytesPerSample,
		format.BitsPerSample, format.IsFloat, count, downmix);
}
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
static void TVPConvertFloatPCMToFloat(float *output, const float *input,
	tjs_int channels, tjs_int count)
{
	// convert 32bit float to float

	// yes, acctually, this does nothing.
	memcpy(output, input, sizeof(float)*channels * count);
}
//---------------------------------------------------------------------------
static void TVPConvertIntegerPCMToFloat(float *output, const void *input,
	tjs_int bytespersample,
	tjs_int validbits, tjs_int channels, tjs_int count)
{
	// convert integer PCMs to float PCM

#ifdef TJS_HOST_IS_BIG_ENDIAN
	#undef TJS_HOST_IS_BIG_ENDIAN
#endif
#if TJS_HOST_IS_BIG_ENDIAN
	#define GET_24BIT(p) (p[2] + (p[1] << 8) + (p[0] << 16))
#else
	#define GET_24BIT(p) (p[0] + (p[1] << 8) + (p[2] << 16))
#endif

	if(bytespersample == 1)
	{
		// here assumes that the input 8bit PCM has always 8bit valid data
		const tjs_int8 *p = (tjs_int8 *)input;
		tjs_int total = channels * count;
		while(total--)
			*(output++) = (float)( ((tjs_int)*(p++)- 0x80) * (1.0 / 128) );
	}
	else if(bytespersample == 2)
	{
		const tjs_int16 *p = (const tjs_int16 *)input;
		tjs_int total = channels * count;

		if(validbits == 16)
		{
#if 0
			// most popular
			bool use_sse =
					(TVPCPUType & TVP_CPU_HAS_MMX) &&
					(TVPCPUType & TVP_CPU_HAS_SSE) &&
					(TVPCPUType & TVP_CPU_HAS_CMOV);

			if(use_sse)
				PCMConvertLoopInt16ToFloat32_sse(output, p, total);
			else
				PCMConvertLoopInt16ToFloat32(output, p, total);
#else
			PCMConvertLoopInt16ToFloat32(output, p, total);
#endif
		}
		else
		{
			// generic
			tjs_uint16 mask =  ~( (1 << (16 - validbits)) - 1);

			while(total--) *(output++) = (float)((*(p++) & mask) * (1.0 / 32768));
		}
	}
	else if(bytespersample == 3)
	{
		tjs_uint32 mask = ~( (1 << (24 - validbits)) - 1);
		const tjs_uint8 *p = (const tjs_uint8 *)input;

		tjs_int total = channels * count;
		while(total--)
		{
			tjs_int32 t = GET_24BIT(p);
			p += 3;
			t |= -(t&0x800000); // extend sign
			t &= mask; // apply mask
			*(output++) = (float)(t * (1.0 / (1<<23)));
		}
	}
	else if(bytespersample == 4)
	{
		tjs_int32 mask = ~( (1 << (32 - validbits)) - 1);
		const tjs_int32 *p = (const tjs_int32 *)input;
		tjs_int total = channels * count;
		while(total--)
			*(output++) = (float)(((*(p++) & mask) >> 0) * (1.0 / (1<<31)));
	}
}
//---------------------------------------------------------------------------
void TVPConvertPCMToFloat(float *output, const void *input,
	tjs_int channels, tjs_int bytespersample, tjs_int bitspersample, bool isfloat,
	tjs_int count)
{
	// cconvert specified format to 16bit PCM

	if(isfloat)
		TVPConvertFloatPCMToFloat(output, (const float *)input, channels, count);
	else
		TVPConvertIntegerPCMToFloat(output, input,
			bytespersample, bitspersample, channels, count);
}
//---------------------------------------------------------------------------
void TVPConvertPCMToFloat(float *output, const void *input,
	const tTVPWaveFormat &format, tjs_int count)
{
	// cconvert specified format to 16bit PCM
	TVPConvertPCMToFloat(output, input, format.Channels, format.BytesPerSample,
		format.BitsPerSample, format.IsFloat, count);
}
//---------------------------------------------------------------------------





//---------------------------------------------------------------------------
// tTVPWD_RIFFWave
//---------------------------------------------------------------------------
class tTVPWD_RIFFWave : public tTVPWaveDecoder
{
	tTJSBinaryStream *Stream;
	tTVPWaveFormat Format;
	tjs_uint64 DataStart;
	tjs_uint64 CurrentPos;
	tjs_uint SampleSize;

public:
	tTVPWD_RIFFWave(tTJSBinaryStream *stream, tjs_uint64 datastart,
		const tTVPWaveFormat & format);
	~tTVPWD_RIFFWave();
	void GetFormat(tTVPWaveFormat & format);
	bool Render(void *buf, tjs_uint bufsamplelen, tjs_uint& rendered);
	bool SetPosition(tjs_uint64 samplepos);
};
//---------------------------------------------------------------------------
tTVPWD_RIFFWave::tTVPWD_RIFFWave(tTJSBinaryStream *stream, tjs_uint64 datastart,
	const tTVPWaveFormat & format)
{
	Stream = stream;
	DataStart = datastart;
	Format = format;
	SampleSize = format.BytesPerSample * format.Channels;
	CurrentPos = 0;
	Stream->SetPosition(DataStart);
}
//---------------------------------------------------------------------------
tTVPWD_RIFFWave::~tTVPWD_RIFFWave()
{
	delete Stream;
}
//---------------------------------------------------------------------------
void tTVPWD_RIFFWave::GetFormat(tTVPWaveFormat & format)
{
	format = Format;
}
//---------------------------------------------------------------------------
bool tTVPWD_RIFFWave::Render(void *buf, tjs_uint bufsamplelen, tjs_uint& rendered)
{
	tjs_uint64 remain = Format.TotalSamples - CurrentPos;
	tjs_uint writesamples = bufsamplelen < remain ? bufsamplelen : (tjs_uint)remain;
	if(writesamples == 0)
	{
		// already finished stream or bufsamplelen is zero
		rendered = 0;
		return false;
	}

	tjs_uint readsize = writesamples * SampleSize;
	tjs_uint read = Stream->Read(buf, readsize);

#if TJS_HOST_IS_BIG_ENDIAN
	// endian-ness conversion
	if(Format.BytesPerSample == 2)
	{
		tjs_uint16 *p = (tjs_uint16 *)buf;
		tjs_uint16 *plim = (tjs_uint16 *)( (tjs_uint8*)buf + read);
		while(p < plim)
		{
			*p = (*p>>8) + (*p<<8);
			p++;
		}
	}
	else if(Format.BytesPerSample == 3)
	{
		tjs_uint8 *p = (tjs_uint8 *)buf;
		tjs_uint8 *plim = (tjs_uint8 *)( (tjs_uint8*)buf + read);
		while(p < plim)
		{
			tjs_uint8 tmp = p[0];
			p[0] = p[2];
			p[2] = tmp;
			p += 3;
		}
	}
	else if(Format.BytesPerSample == 4)
	{
		tjs_uint32 *p = (tjs_uint32 *)buf;
		tjs_uint32 *plim = (tjs_uint32 *)( (tjs_uint8*)buf + read);
		while(p < plim)
		{
			*p =
				(*p &0xff000000) >> 24 +
				(*p &0x00ff0000) >> 8 +
				(*p &0x0000ff00) << 8 +
				(*p &0x000000ff) << 24;
			p ++;
		}
	}
#endif


	rendered = read / SampleSize;
	CurrentPos += rendered;

	if(read < readsize || writesamples < bufsamplelen)
		return false; // read error or end of stream

	return true;
}
//---------------------------------------------------------------------------
bool tTVPWD_RIFFWave::SetPosition(tjs_uint64 samplepos)
{
	if(Format.TotalSamples <= samplepos) return false;

	tjs_uint64 streampos = DataStart + samplepos * SampleSize;
	tjs_uint64 possave = Stream->GetPosition();

	if(streampos != Stream->Seek(streampos, TJS_BS_SEEK_SET))
	{
		// seek failed
		Stream->Seek(possave, TJS_BS_SEEK_SET);
		return false;
	}

	CurrentPos = samplepos;
	return true;
}
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// default RIFF Wave Decoder creator
//---------------------------------------------------------------------------
class tTVPWDC_RIFFWave : public tTVPWaveDecoderCreator
{
public:
	tTVPWaveDecoder * Create(const ttstr & storagename, const ttstr & extension);
};
//---------------------------------------------------------------------------
static bool TVPFindRIFFChunk(tTVPStreamHolder & stream, const tjs_uint8 *chunk)
{
	tjs_uint8 buf[4];
	while(true)
	{
		if(4 != stream->Read(buf, 4)) return false;
		if(memcmp(buf, chunk, 4))
		{
			// skip to next chunk
			tjs_uint32 chunksize = stream->ReadI32LE();
			tjs_int64 next = stream->GetPosition() + chunksize;
			if(next != stream->Seek(next, TJS_BS_SEEK_SET)) return false;
		}
		else
		{
			return true;
		}
	}
}
//---------------------------------------------------------------------------
tTVPWaveDecoder * tTVPWDC_RIFFWave::Create(const ttstr & storagename,
	const ttstr &extension)
{
	if(extension != TJS_W(".wav")) return NULL;

	tTVPStreamHolder stream(storagename);

	static const tjs_uint8 riff_mark[] =
		{ /*R*/0x52, /*I*/0x49, /*F*/0x46, /*F*/0x46 };
	static const tjs_uint8 wave_mark[] =
		{ /*W*/0x57, /*A*/0x41, /*V*/0x56, /*E*/0x45 };
	static const tjs_uint8 fmt_mark[] =
		{ /*f*/0x66, /*m*/0x6d, /*t*/0x74, /* */0x20 };
	static const tjs_uint8 data_mark[] =
		{ /*d*/0x64, /*a*/0x61, /*t*/0x74, /*a*/0x61 };

	try
	{
		tjs_uint32 size;
		tjs_int64 next;

		// check RIFF mark
		tjs_uint8 buf[4];
		if(4 != stream->Read(buf, 4)) return NULL;
		if(memcmp(buf, riff_mark, 4)) return NULL;

		if(4 != stream->Read(buf, 4)) return NULL; // RIFF chunk size; discard

		// check WAVE subid
		if(4 != stream->Read(buf, 4)) return NULL;
		if(memcmp(buf, wave_mark, 4)) return NULL;

		// find fmt chunk
		if(!TVPFindRIFFChunk(stream, fmt_mark)) return NULL;

		size = stream->ReadI32LE();
		next = stream->GetPosition() + size;

		// read format
		tTVPWaveFormat format;
//		bool do_reorder_5poiont1ch = false;
			// whether to reorder 5.1ch channel samples
			// (AC3 order to WAVEFORMATEXTENSIBLE order)

		tjs_uint16 format_tag = stream->ReadI16LE(); // wFormatTag
		if(format_tag != WAVE_FORMAT_PCM &&
			format_tag != WAVE_FORMAT_IEEE_FLOAT &&
			format_tag != WAVE_FORMAT_EXTENSIBLE) return NULL;


		format.Channels = stream->ReadI16LE(); // nChannels
		format.SamplesPerSec = stream->ReadI32LE(); // nSamplesPerSec

		if(4 != stream->Read(buf, 4)) return NULL; // nAvgBytesPerSec; discard

		tjs_uint16 block_align = stream->ReadI16LE(); // nBlockAlign

		format.BitsPerSample = stream->ReadI16LE(); // wBitsPerSample

		tjs_uint16 ext_size = stream->ReadI16LE(); // cbSize
		if(format_tag == WAVE_FORMAT_EXTENSIBLE)
		{
			if(ext_size != 22) return NULL; // invalid extension length
			if(format.BitsPerSample & 0x07) return NULL;  // not integer multiply by 8
			format.BytesPerSample = format.BitsPerSample / 8;
			format.BitsPerSample = stream->ReadI16LE(); // wValidBitsPerSample
			format.SpeakerConfig = stream->ReadI32LE(); // dwChannelMask

			tjs_uint8 guid[16];
			if(16 != stream->Read(guid, 16)) return NULL;
			if(!memcmp(guid, TVP_GUID_KSDATAFORMAT_SUBTYPE_PCM, 16))
				format.IsFloat = false;
			else if(!memcmp(guid, TVP_GUID_KSDATAFORMAT_SUBTYPE_IEEE_FLOAT, 16))
				format.IsFloat = true;
			else
				return NULL;
		}
		else
		{
			if(format.BitsPerSample & 0x07) return NULL; // not integer multiplyed by 8
			format.BytesPerSample = format.BitsPerSample / 8;

			if(format.Channels == 4)
			{
				format.SpeakerConfig = 0;
//				format.SpeakerConfig =
//					SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT |
//					SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT;

			}
			else if(format.Channels == 6)
			{
				format.SpeakerConfig = 0;
//				do_reorder_5poiont1ch = true;
//				format.SpeakerConfig =
//					SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT |
			}
			else
			{
				format.SpeakerConfig = 0;
			}

			format.IsFloat = format_tag == WAVE_FORMAT_IEEE_FLOAT;
		}


		if(format.BitsPerSample > 32) return NULL; // too large bits
		if(format.BitsPerSample < 8) return NULL; // too less bits
		if(format.BitsPerSample > format.BytesPerSample * 8)
			return NULL; // bits per sample is larger than bytes per sample
		if(format.IsFloat)
		{
			if(format.BitsPerSample != 32) return NULL; // not a 32-bit IEEE float
			if(format.BytesPerSample != 4) return NULL;
		}

		if((tjs_int) block_align != (tjs_int)(format.BytesPerSample * format.Channels))
			return NULL; // invalid align

		if(next != stream->Seek(next, TJS_BS_SEEK_SET)) return NULL;

		// find data chunk
		if(!TVPFindRIFFChunk(stream, data_mark)) return NULL;

		size = stream->ReadI32LE();

		tjs_int64 datastart;

		tjs_int64 remain_size = stream->GetSize() -
			(datastart = stream->GetPosition());
		if(size > remain_size) return NULL;
			// data ends before "size" described in the header

		// compute total sample count and total length in time
		format.TotalSamples = size / (format.Channels * format.BitsPerSample / 8);
		format.TotalTime = format.TotalSamples * 1000 / format.SamplesPerSec;

		// create tTVPWD_RIFFWave instance
		format.Seekable = true;
		tTVPWD_RIFFWave * decoder = new tTVPWD_RIFFWave(stream.Get(), datastart,
			format);
		stream.Disown();

		return decoder;
	}
	catch(...)
	{
		// this function must be a silent one unless file open error...
		return NULL;
	}
}
//---------------------------------------------------------------------------




//---------------------------------------------------------------------------
// tTVPWaveDecoder interface management
//---------------------------------------------------------------------------
bool TVPWaveDecoderManagerAvail = false;
struct tTVPWaveDecoderManager
{
	std::vector<tTVPWaveDecoderCreator *> Creators;
	tTVPWDC_RIFFWave RIFFWaveDecoderCreator;
    VorbisWaveDecoderCreator vorbisWaveDecoderCreator;
    FFWaveDecoderCreator ffWaveDecoderCreator;
    OpusWaveDecoderCreator opusWaveDecoderCreator;

	tTVPWaveDecoderManager()
	{
		TVPWaveDecoderManagerAvail = true;
        TVPRegisterWaveDecoderCreator(&ffWaveDecoderCreator);
        TVPRegisterWaveDecoderCreator(&opusWaveDecoderCreator);
		TVPRegisterWaveDecoderCreator(&RIFFWaveDecoderCreator);
        TVPRegisterWaveDecoderCreator(&vorbisWaveDecoderCreator);
	}

	~tTVPWaveDecoderManager()
	{
		TVPWaveDecoderManagerAvail = false;
	}

} static TVPWaveDecoderManager;
//---------------------------------------------------------------------------
void TVPRegisterWaveDecoderCreator(tTVPWaveDecoderCreator *d)
{
	if(TVPWaveDecoderManagerAvail)
		TVPWaveDecoderManager.Creators.push_back(d);
}
//---------------------------------------------------------------------------
void TVPUnregisterWaveDecoderCreator(tTVPWaveDecoderCreator *d)
{
	if(TVPWaveDecoderManagerAvail)
	{
		std::vector<tTVPWaveDecoderCreator *>::iterator i;
		i = std::find(TVPWaveDecoderManager.Creators.begin(),
			TVPWaveDecoderManager.Creators.end(), d);
		if(i != TVPWaveDecoderManager.Creators.end())
		{
			TVPWaveDecoderManager.Creators.erase(i);
		}
	}
}
//---------------------------------------------------------------------------
tTVPWaveDecoder *  TVPCreateWaveDecoder(const ttstr & storagename)
{
	// find a decoder and create its instance.
	// throws an exception when the decodable decoder is not found.
	if(!TVPWaveDecoderManagerAvail) return NULL;

	ttstr ext(TVPExtractStorageExt(storagename));
	ext.ToLowerCase();

	tjs_int i = (tjs_int)(TVPWaveDecoderManager.Creators.size()-1);
	for(; i>=0; i--)
	{
		tTVPWaveDecoder * decoder;
		decoder = TVPWaveDecoderManager.Creators[i]->Create(storagename, ext);
		if(decoder) return decoder;
	}

	TVPThrowExceptionMessage(TVPUnknownWaveFormat, storagename);
	return NULL;
}
//---------------------------------------------------------------------------




//---------------------------------------------------------------------------
// sound format convert filter
//---------------------------------------------------------------------------
class iSoundBufferConvertToPCM16 : public tTJSDispatch, // could be hold by tFilterObjectAndInterface
	public iTVPBasicWaveFilter, public tTVPSampleAndLabelSource
{
protected:
	tTVPSampleAndLabelSource * Source; // source filter
	tTVPWaveFormat OutputFormat;

public:
	virtual tTVPSampleAndLabelSource * Recreate(tTVPSampleAndLabelSource * source) {
		Source = source;
		OutputFormat = source->GetFormat();
		OutputFormat.IsFloat = false;
		OutputFormat.BitsPerSample = 16;
		OutputFormat.BytesPerSample = 2;
		return this;
	}
	virtual ~iSoundBufferConvertToPCM16() {}
	virtual void Clear(void) { Source = nullptr; }
	virtual void Update(void) {};
	virtual void Reset(void) {};

	virtual const tTVPWaveFormat & GetFormat() const { return OutputFormat; }
};
//---------------------------------------------------------------------------
class SoundBufferConvertFloatToPCM16 : public iSoundBufferConvertToPCM16 {
	std::vector<float> buffer;
	virtual void Decode(void *dest, tjs_uint samples, tjs_uint &written,
		tTVPWaveSegmentQueue &segments) {
		tjs_uint numsamples = samples * OutputFormat.Channels;
		if (buffer.size() < numsamples) buffer.resize(numsamples);
		float *p = &buffer[0];
		Source->Decode(p, samples, written, segments);
		numsamples = written * OutputFormat.Channels;
		PCMConvertLoopFloat32ToInt16(dest, p, numsamples);
	}
};
//---------------------------------------------------------------------------
class SoundBufferConvertPCM24ToPCM16 : public iSoundBufferConvertToPCM16 {
	std::vector<char> buffer;
	virtual void Decode(void *dest, tjs_uint samples, tjs_uint &written,
		tTVPWaveSegmentQueue &segments) {
		tjs_uint numsamples = samples * OutputFormat.Channels;
		if (buffer.size() < numsamples * 3) buffer.resize(numsamples * 3);
		char *p = &buffer[0]; int16_t *d = (int16_t *)dest;
		Source->Decode(p, samples, written, segments);
		numsamples = written * OutputFormat.Channels;
		for (int i = 0; i < numsamples; ++i) {
			d[i] = *(int16_t*)(p + 1);
			p += 3;
		}
	}
};
//---------------------------------------------------------------------------
class SoundBufferConvertPCM32ToPCM16 : public iSoundBufferConvertToPCM16 {
	std::vector<int32_t> buffer;
	virtual void Decode(void *dest, tjs_uint samples, tjs_uint &written,
		tTVPWaveSegmentQueue &segments) {
		tjs_uint numsamples = samples * OutputFormat.Channels;
		if (buffer.size() < numsamples) buffer.resize(numsamples);
		char *p = (char *)&buffer[0]; int16_t *d = (int16_t *)dest;
		Source->Decode(p, samples, written, segments);
		numsamples = written * OutputFormat.Channels;
		for (int i = 0; i < numsamples; ++i) {
			d[i] = *(int16_t*)(p + 2);
			p += 4;
		}
	}
};
//---------------------------------------------------------------------------




//---------------------------------------------------------------------------
// tTJSNI_BaseWaveSoundBuffer
//---------------------------------------------------------------------------
tTJSNI_BaseWaveSoundBuffer::tTJSNI_BaseWaveSoundBuffer()
{
	LoopManager = NULL;
	WaveFlagsObject = NULL;
	WaveLabelsObject = NULL;
	Filters = TJSCreateArrayObject();
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD
	tTJSNI_BaseWaveSoundBuffer::Construct(tjs_int numparams, tTJSVariant **param,
		iTJSDispatch2 *tjs_obj)
{
	tjs_error hr = inherited::Construct(numparams, param, tjs_obj);
	if(TJS_FAILED(hr)) return hr;

	return TJS_S_OK;
}
//---------------------------------------------------------------------------
void TJS_INTF_METHOD tTJSNI_BaseWaveSoundBuffer::Invalidate()
{
	// invalidate wave flags object
	RecreateWaveLabelsObject();

	// release filter arrays
	if(Filters) Filters->Release(), Filters = NULL;

	inherited::Invalidate();
}
//---------------------------------------------------------------------------
void tTJSNI_BaseWaveSoundBuffer::InvokeLabelEvent(const ttstr & name)
{
	// the invoked event is to be delivered asynchronously.
	// the event is to be erased when the SetStatus is called, but it's ok.
	// ( SetStatus calls TVPCancelSourceEvents(Owner); )

	if(Owner && CanDeliverEvents)
	{
		tTJSVariant param(name);
		static ttstr eventname(TJS_W("onLabel"));
		TVPPostEvent(Owner, Owner, eventname, 0, TVP_EPT_POST,
			1, &param);
	}
}
//---------------------------------------------------------------------------
void tTJSNI_BaseWaveSoundBuffer::RecreateWaveLabelsObject()
{
	// indicate recreating WaveLabelsObject
	if(WaveLabelsObject)
	{
		WaveLabelsObject->Invalidate(0, NULL, NULL, WaveLabelsObject);
		WaveLabelsObject->Release();
		WaveLabelsObject = NULL;
	}
}
//---------------------------------------------------------------------------
void tTJSNI_BaseWaveSoundBuffer::RebuildFilterChain()
{
	// rebuild filter array
	FilterInterfaces.clear();

	// get filter count
	tTJSVariant v;
	tjs_int count = 0;
	Filters->PropGet(0, TJS_W("count"), NULL, &v, Filters);
	count = v;

	// reset filter output
	FilterOutput = LoopManager;

	// for each filter ...
	for(int i = 0; i < count; i++)
	{
		Filters->PropGetByNum(0, i, &v, Filters);

		// get iTVPBasicWaveFilter interface
		tTJSVariantClosure clo = v.AsObjectClosureNoAddRef();
		tTJSVariant iface_v;
		if(TJS_FAILED(clo.PropGet(0, TJS_W("interface"), NULL, &iface_v, NULL))) continue;
		iTVPBasicWaveFilter * filter =
			reinterpret_cast<iTVPBasicWaveFilter *>((tjs_intptr_t)(tjs_int64)iface_v);
		// save to the backupped array
		FilterInterfaces.emplace_back(v, filter);
	}

	// reset filter output
	FilterOutput = LoopManager;

	// for each filter ...
	for(std::vector<tFilterObjectAndInterface>::iterator i = FilterInterfaces.begin();
		i != FilterInterfaces.end(); i++)
	{
		// recreate filter
		FilterOutput = i->Interface->Recreate(FilterOutput);
	}

	const tTVPWaveFormat &filteredFormat = FilterOutput->GetFormat();
	if (filteredFormat.IsFloat) {
		SoundBufferConvertFloatToPCM16 *filter = new SoundBufferConvertFloatToPCM16;
		FilterInterfaces.emplace_back(filter, filter);
		FilterOutput = filter->Recreate(FilterOutput);
		filter->Release();
	} else if (filteredFormat.BitsPerSample == 24) {
		SoundBufferConvertPCM24ToPCM16 *filter = new SoundBufferConvertPCM24ToPCM16;
		FilterInterfaces.emplace_back(filter, filter);
		FilterOutput = filter->Recreate(FilterOutput);
		filter->Release();
	} else if (filteredFormat.BitsPerSample == 32) {
		SoundBufferConvertPCM32ToPCM16 *filter = new SoundBufferConvertPCM32ToPCM16;
		FilterInterfaces.emplace_back(filter, filter);
		FilterOutput = filter->Recreate(FilterOutput);
		filter->Release();
	}
}
//---------------------------------------------------------------------------
void tTJSNI_BaseWaveSoundBuffer::ClearFilterChain()
{
	// delete object which is created from this filter array

	// reset filter output
	FilterOutput = NULL;

	for(std::vector<tFilterObjectAndInterface>::iterator i = FilterInterfaces.begin();
		i != FilterInterfaces.end(); i++)
	{
		// recreate filter
		i->Interface->Clear();
	}

	// clear backupped filter array
	FilterInterfaces.clear();
}
//---------------------------------------------------------------------------
void tTJSNI_BaseWaveSoundBuffer::ResetFilterChain()
{
	// Reset filter chain.
	for(std::vector<tFilterObjectAndInterface>::iterator i = FilterInterfaces.begin();
		i != FilterInterfaces.end(); i++)
		i->Interface->Reset();
}
//---------------------------------------------------------------------------
void tTJSNI_BaseWaveSoundBuffer::UpdateFilterChain()
{
	// Update filter chain.
	// This method is called before that the player is about to decode a small 
	// PCM unit (typically piece of 125ms of sound).
	// Note that this method may be called by decoding thread,
	// but it's guaranteed that the call is never overlapped with
	// UpdateFilterChain self and ClearFilterChain and RebuildFilterChain.
	// so we does not need to protect this call by CriticalSection.
	for(std::vector<tFilterObjectAndInterface>::iterator i = FilterInterfaces.begin();
		i != FilterInterfaces.end(); i++)
		i->Interface->Update();
}
//---------------------------------------------------------------------------
iTJSDispatch2 * tTJSNI_BaseWaveSoundBuffer::GetWaveFlagsObjectNoAddRef()
{
	if(WaveFlagsObject) return WaveFlagsObject;

	if(!Owner) TVPThrowInternalError;

	WaveFlagsObject = TVPCreateWaveFlagsObject(Owner);

	return WaveFlagsObject;
}
//---------------------------------------------------------------------------
iTJSDispatch2 * tTJSNI_BaseWaveSoundBuffer::GetWaveLabelsObjectNoAddRef()
{
	if(WaveLabelsObject) return WaveLabelsObject;

	// build label dictionay from WaveLoopManager
	WaveLabelsObject = TJSCreateDictionaryObject();

	if(LoopManager)
	{
		const std::vector<tTVPWaveLabel> & labels = LoopManager->GetLabels();

		int freq = FilterOutput->GetFormat().SamplesPerSec;

		int count = 0;
		for(std::vector<tTVPWaveLabel>::const_iterator i = labels.begin();
			i != labels.end(); i++, count++)
		{
			iTJSDispatch2 * item_dic = TJSCreateDictionaryObject();
			try
			{
				tTJSVariant val;
				val = i->Name.c_str(); // c_str() to avoid race condition for ttstr
				item_dic->PropSet(TJS_MEMBERENSURE, TJS_W("name"), NULL, &val, item_dic);
				val = i->Position;
				item_dic->PropSet(TJS_MEMBERENSURE, TJS_W("samplePosition"), NULL, &val, item_dic);
				val = freq ? i->Position * 1000 / freq : 0;
				item_dic->PropSet(TJS_MEMBERENSURE, TJS_W("position"), NULL, &val, item_dic);

				tTJSVariant item_dic_var(item_dic, item_dic);

				if(!i->Name.IsEmpty())
					WaveLabelsObject->PropSet(TJS_MEMBERENSURE, i->Name.c_str(), NULL,
						&item_dic_var, WaveLabelsObject);
			}
			catch(...)
			{
				item_dic->Release();
				throw;
			}
			item_dic->Release();
		}
	}

	return WaveLabelsObject;
}
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// tTJSNC_WaveSoundBuffer : TJS WaveSoundBuffer class
//---------------------------------------------------------------------------
tjs_uint32 tTJSNC_WaveSoundBuffer::ClassID = -1;
tTJSNC_WaveSoundBuffer::tTJSNC_WaveSoundBuffer()  :
	tTJSNativeClass(TJS_W("WaveSoundBuffer"))
{
	// registration of native members

	TJS_BEGIN_NATIVE_MEMBERS(WaveSoundBuffer) // constructor
	TJS_DECL_EMPTY_FINALIZE_METHOD
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_CONSTRUCTOR_DECL(/*var.name*/_this,
	/*var.type*/tTJSNI_WaveSoundBuffer,
	/*TJS class name*/WaveSoundBuffer)
{
	return TJS_S_OK;
}
TJS_END_NATIVE_CONSTRUCTOR_DECL(/*TJS class name*/WaveSoundBuffer)
//----------------------------------------------------------------------

//-- methods

//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/open)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this,
		/*var. type*/tTJSNI_WaveSoundBuffer);

	if(numparams < 1) return TJS_E_BADPARAMCOUNT;
	_this->Open(*param[0]);

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/open)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/play)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this,
		/*var. type*/tTJSNI_WaveSoundBuffer);

	_this->Play();

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/play)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/stop)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this,
		/*var. type*/tTJSNI_WaveSoundBuffer);

	_this->Stop();

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/stop)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/fade)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this,
		/*var. type*/tTJSNI_WaveSoundBuffer);
	if(numparams < 2) return TJS_E_BADPARAMCOUNT;

	tjs_int to;
	tjs_int time;
	tjs_int delay = 0;
	to = (tjs_int)(*param[0]);
	time = (tjs_int)(*param[1]);
	if(numparams >= 3 && param[2]->Type() != tvtVoid)
		delay = (tjs_int)(*param[2]);

	_this->Fade(to, time, delay);

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/fade)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/stopFade)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this,
		/*var. type*/tTJSNI_WaveSoundBuffer);

	_this->StopFade(false, true);

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/stopFade)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/setPos) // not setPosition
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this,
		/*var. type*/tTJSNI_WaveSoundBuffer);

	if(numparams < 3) return TJS_E_BADPARAMCOUNT;

	tTVReal x, y, z;
	x = (*param[0]);
	y = (*param[1]);
	z = (*param[2]);

	_this->SetPos( static_cast<D3DVALUE>(x), static_cast<D3DVALUE>(y), static_cast<D3DVALUE>(z) );

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/setPos)
//----------------------------------------------------------------------

//-- events

//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/onStatusChanged)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this,
		/*var. type*/tTJSNI_WaveSoundBuffer);

	tTJSVariantClosure obj = _this->GetActionOwnerNoAddRef();
	if(obj.Object)
	{
		TVP_ACTION_INVOKE_BEGIN(1, "onStatusChanged", objthis);
		TVP_ACTION_INVOKE_MEMBER("status");
		TVP_ACTION_INVOKE_END(obj);
	}

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/onStatusChanged)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/onFadeCompleted)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this,
		/*var. type*/tTJSNI_WaveSoundBuffer);

	tTJSVariantClosure obj = _this->GetActionOwnerNoAddRef();
	if(obj.Object)
	{
		TVP_ACTION_INVOKE_BEGIN(0, "onFadeCompleted", objthis);
		TVP_ACTION_INVOKE_END(obj);
	}

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/onFadeCompleted)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/onLabel)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this,
		/*var. type*/tTJSNI_WaveSoundBuffer);

	tTJSVariantClosure obj = _this->GetActionOwnerNoAddRef();
	if(obj.Object)
	{
		TVP_ACTION_INVOKE_BEGIN(1, "onLabel", objthis);
		TVP_ACTION_INVOKE_MEMBER("name");
		TVP_ACTION_INVOKE_END(obj);
	}

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/onLabel)
//----------------------------------------------------------------------

//-- properties

//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(position)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_WaveSoundBuffer);

		*result = (tjs_int64)_this->GetPosition();

		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_WaveSoundBuffer);

		_this->SetPosition((tjs_uint64)(tjs_int64)*param);

		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(position)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(samplePosition)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_WaveSoundBuffer);

		*result = (tjs_int64)_this->GetSamplePosition();

		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_WaveSoundBuffer);

		_this->SetSamplePosition((tjs_uint64)(tjs_int64)*param);

		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(samplePosition)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(paused)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_WaveSoundBuffer);

		*result = _this->GetPaused();

		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_WaveSoundBuffer);

		_this->SetPaused(*param);

		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(paused)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(totalTime)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_WaveSoundBuffer);

		*result = (tjs_int64)_this->GetTotalTime();

		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_WaveSoundBuffer);

		// not yet implemented

		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(totalTime)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(looping)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_WaveSoundBuffer);

		*result = _this->GetLooping();

		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_WaveSoundBuffer);

		_this->SetLooping(*param);

		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(looping)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(volume)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_WaveSoundBuffer);

		*result = _this->GetVolume();

		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_WaveSoundBuffer);

		_this->SetVolume(*param);

		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(volume)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(volume2)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_WaveSoundBuffer);

		*result = _this->GetVolume2();

		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_WaveSoundBuffer);

		_this->SetVolume2(*param);

		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(volume2)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(pan)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_WaveSoundBuffer);

		*result = _this->GetPan();

		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_WaveSoundBuffer);

		_this->SetPan(*param);

		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(pan)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(posX)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_WaveSoundBuffer);

		*result = (tTVReal)_this->GetPosX();

		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_WaveSoundBuffer);

		_this->SetPosX( static_cast<D3DVALUE>( (tTVReal)*param ));

		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(posX)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(posY)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_WaveSoundBuffer);

		*result = (tTVReal)_this->GetPosY();

		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_WaveSoundBuffer);

		_this->SetPosY( static_cast<D3DVALUE>( (tTVReal)*param ) );

		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(posY)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(posZ)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_WaveSoundBuffer);

		*result = (tTVReal)_this->GetPosZ();

		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_WaveSoundBuffer);

		_this->SetPosZ( static_cast<D3DVALUE>((tTVReal)*param) );

		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(posZ)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(status)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_WaveSoundBuffer);

		*result = _this->GetStatusString();

		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(status)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(frequency)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_WaveSoundBuffer);

		*result = _this->GetFrequency();

		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_WaveSoundBuffer);

		_this->SetFrequency((tjs_int)*param);

		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(frequency)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(bits) // not bitsPerSample
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_WaveSoundBuffer);

		*result = _this->GetBitsPerSample();

		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(bits)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(channels)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_WaveSoundBuffer);

		*result = _this->GetChannels();

		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(channels)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(flags)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_WaveSoundBuffer);

		iTJSDispatch2 * dsp = _this->GetWaveFlagsObjectNoAddRef();
		*result = tTJSVariant(dsp, dsp);
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(flags)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(labels)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_WaveSoundBuffer);

		iTJSDispatch2 * dsp = _this->GetWaveLabelsObjectNoAddRef();
		*result = tTJSVariant(dsp, dsp);
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(labels)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(filters)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_WaveSoundBuffer);

		iTJSDispatch2 * dsp = _this->GetFiltersNoAddRef();
		*result = tTJSVariant(dsp, dsp);
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(filters)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(globalVolume)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		*result = tTJSNI_WaveSoundBuffer::GetGlobalVolume();

		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		tTJSNI_WaveSoundBuffer::SetGlobalVolume(*param);

		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_STATIC_PROP_DECL(globalVolume)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(globalFocusMode)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		*result = (tjs_int)tTJSNI_WaveSoundBuffer::GetGlobalFocusMode();

		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		tTJSNI_WaveSoundBuffer::SetGlobalFocusMode(
			(tTVPSoundGlobalFocusMode)(tjs_int)*param);

		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_STATIC_PROP_DECL(globalFocusMode)
//----------------------------------------------------------------------

	TJS_END_NATIVE_MEMBERS

//----------------------------------------------------------------------


}
//---------------------------------------------------------------------------








//---------------------------------------------------------------------------
// tTJSNI_WaveFlags : Wave Flags object
//---------------------------------------------------------------------------
tTJSNI_WaveFlags::tTJSNI_WaveFlags()
{
	Buffer = NULL;
}
//---------------------------------------------------------------------------
tTJSNI_WaveFlags::~tTJSNI_WaveFlags()
{
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD tTJSNI_WaveFlags::Construct(tjs_int numparams,
	tTJSVariant **param, iTJSDispatch2 *tjs_obj)
{
	if(numparams < 1) return TJS_E_BADPARAMCOUNT;

	iTJSDispatch2 *dsp = param[0]->AsObjectNoAddRef();

	tTJSNI_WaveSoundBuffer *buffer = NULL;
	if(TJS_FAILED(dsp->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
		tTJSNC_WaveSoundBuffer::ClassID, (iTJSNativeInstance**)&buffer)))
		TVPThrowInternalError;

	Buffer = buffer;

	return TJS_S_OK;
}
//---------------------------------------------------------------------------
void TJS_INTF_METHOD tTJSNI_WaveFlags::Invalidate()
{
	Buffer = NULL;

	inherited::Invalidate();
}
//---------------------------------------------------------------------------





//---------------------------------------------------------------------------
// tTJSNC_WaveFlags : Wave Flags class
//---------------------------------------------------------------------------
tjs_uint32 tTJSNC_WaveFlags::ClassID = -1;
tTJSNC_WaveFlags::tTJSNC_WaveFlags() : tTJSNativeClass(TJS_W("WaveFlags"))
{
	TJS_BEGIN_NATIVE_MEMBERS(WaveFlags) // constructor
	TJS_DECL_EMPTY_FINALIZE_METHOD
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_CONSTRUCTOR_DECL(/*var.name*/_this, /*var.type*/tTJSNI_WaveFlags,
	/*TJS class name*/WaveFlags)
{
	return TJS_S_OK;
}
TJS_END_NATIVE_CONSTRUCTOR_DECL(/*TJS class name*/WaveFlags)
//----------------------------------------------------------------------

//-- methods

//---------------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/reset)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_WaveFlags);

	tTVPWaveLoopManager * manager =
		_this->GetBuffer()->GetWaveLoopManager(); /* note that the manager can be null */

	if(manager) manager->ClearFlags();

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/reset)
//----------------------------------------------------------------------

//-- properties

//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(count)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_WaveFlags);
		*result = TVP_WL_MAX_FLAGS; // always this value
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(count)
//----------------------------------------------------------------------
#define TVP_DEFINE_FLAG_PROP(N) \
	TJS_BEGIN_NATIVE_PROP_DECL(N)                                                                    \
	{                                                                                                \
		TJS_BEGIN_NATIVE_PROP_GETTER                                                                 \
		{                                                                                            \
			TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_WaveFlags);              \
			tTVPWaveLoopManager * manager =                                                          \
				_this->GetBuffer()->GetWaveLoopManager(); /* note that the manager can be null */    \
			if(manager) *result = manager->GetFlag(N); else *result = (tjs_int)0;                    \
			return TJS_S_OK;                                                                         \
		}                                                                                            \
		TJS_END_NATIVE_PROP_GETTER                                                                   \
                                                                                                     \
		TJS_BEGIN_NATIVE_PROP_SETTER                                                                 \
		{                                                                                            \
			TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_WaveFlags);              \
			tTVPWaveLoopManager * manager =                                                          \
				_this->GetBuffer()->GetWaveLoopManager(); /* note that the manager can be null */    \
			if(manager) manager->SetFlag(N, (tjs_int)*param);                                        \
			return TJS_S_OK;                                                                         \
		}                                                                                            \
		TJS_END_NATIVE_PROP_SETTER                                                                   \
	}                                                                                                \
	TJS_END_NATIVE_PROP_DECL(N)                                                                      

TVP_DEFINE_FLAG_PROP(0)
TVP_DEFINE_FLAG_PROP(1)
TVP_DEFINE_FLAG_PROP(2)
TVP_DEFINE_FLAG_PROP(3)
TVP_DEFINE_FLAG_PROP(4)
TVP_DEFINE_FLAG_PROP(5)
TVP_DEFINE_FLAG_PROP(6)
TVP_DEFINE_FLAG_PROP(7)
TVP_DEFINE_FLAG_PROP(8)
TVP_DEFINE_FLAG_PROP(9)
TVP_DEFINE_FLAG_PROP(10)
TVP_DEFINE_FLAG_PROP(11)
TVP_DEFINE_FLAG_PROP(12)
TVP_DEFINE_FLAG_PROP(13)
TVP_DEFINE_FLAG_PROP(14)
TVP_DEFINE_FLAG_PROP(15)

//----------------------------------------------------------------------
	TJS_END_NATIVE_MEMBERS
}
//---------------------------------------------------------------------------
iTJSDispatch2 * TVPCreateWaveFlagsObject(iTJSDispatch2 * buffer)
{
	struct tHolder
	{
		iTJSDispatch2 * Obj;
		tHolder() { Obj = new tTJSNC_WaveFlags(); }
		~tHolder() { Obj->Release(); }
	} static waveflagsclass;

	iTJSDispatch2 *out;
	tTJSVariant param(buffer);
	tTJSVariant *pparam = &param;
	if(TJS_FAILED(waveflagsclass.Obj->CreateNew(0, NULL, NULL, &out, 1, &pparam,
		waveflagsclass.Obj)))
		TVPThrowInternalError;

	return out;
}
//---------------------------------------------------------------------------


