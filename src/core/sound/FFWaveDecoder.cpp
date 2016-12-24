#include "FFWaveDecoder.h"
#include "WaveIntf.h"
#include "StorageIntf.h"
#include "DebugIntf.h"
#include "SysInitIntf.h"
#include "BinaryStream.h"
extern "C" {
#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#ifndef UINT64_C
#define UINT64_C(x)  (x ## ULL)
#endif
#include "libavutil/avutil.h"
#include "libavutil/opt.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
};

class FFWaveDecoder : public tTVPWaveDecoder // decoder interface
{
    bool IsPlanar;

    int StreamIdx;
    int audio_buf_index;
    int audio_buf_samples;
    int64_t audio_frame_next_pts;
    int64_t stream_start_time;
    tTVPWaveFormat Format; // output PCM format
    AVSampleFormat AVFmt;
    AVPacket pkt_temp;
    AVStream *AudioStream;

    // release in Clear()
    AVPacket Packet;
    tTJSBinaryStream *InputStream; // input stream
    AVFormatContext *FormatCtx;
    AVFrame *frame;

    int audio_decode_frame();
    void Clear() {
        if (Packet.data)
            av_free_packet(&Packet);
        if(frame) av_frame_free(&frame), frame = nullptr;
		if (FormatCtx) {
			av_free(FormatCtx->pb->buffer);
			av_free(FormatCtx->pb);
			avformat_free_context(FormatCtx), FormatCtx = nullptr;
		}
        if(InputStream) delete InputStream, InputStream = nullptr;
    }
    bool ReadPacket();

public:
    FFWaveDecoder()
        : InputStream(nullptr)
        , FormatCtx(nullptr)
        , frame(nullptr)
    {
        memset(&Packet, 0, sizeof(Packet));
    }
    ~FFWaveDecoder() {
        Clear();
    }

public:
    // ITSSWaveDecoder
    virtual void GetFormat(tTVPWaveFormat & format) { format = Format; }
    virtual bool Render(void *buf, tjs_uint bufsamplelen, tjs_uint& rendered);
    virtual bool SetPosition(tjs_uint64 samplepos);

    // others
    bool SetStream(const ttstr & storagename);
};

static int AVReadFunc(void *opaque, uint8_t *buf, int buf_size)
{
    TJS::tTJSBinaryStream *stream = (TJS::tTJSBinaryStream *)opaque;
    return stream->Read(buf, buf_size);
}

static int64_t AVSeekFunc(void *opaque, int64_t offset, int whence)
{
    TJS::tTJSBinaryStream *stream = (TJS::tTJSBinaryStream *)opaque;
    switch (whence)
    {
    case AVSEEK_SIZE:
        return stream->GetSize();
    default:
        return stream->Seek(offset, whence & 0xFF);
    }
}
void TVPInitLibAVCodec();
tTVPWaveDecoder * FFWaveDecoderCreator::Create(const ttstr & storagename, const ttstr & extension) {
	TVPInitLibAVCodec();

    FFWaveDecoder * decoder = new FFWaveDecoder();
    if(!decoder->SetStream(storagename)) {
        delete decoder;
        decoder = nullptr;
    }
    return decoder;
}

template <typename T>
static unsigned char* _CopySmaples(unsigned char *dst, AVFrame *frame, int samples, int buf_index)
{
    int buf_pos = buf_index * sizeof(T);
    T* pDst = (T*)dst;
    for(int i = 0; i < samples; ++i, buf_pos += sizeof(T)) {
        for(int j = 0; j < frame->channels; ++j) {
            *pDst++ = *(T*)(frame->data[j] + buf_pos);
        }
    }
    return (unsigned char*)pDst;
}

// optimized for stereo
template <typename T>
static unsigned char* _CopySmaples2(unsigned char *dst, AVFrame *frame, int samples, int buf_index)
{
    int buf_pos = buf_index * sizeof(T);
    T* pDst = (T*)dst;
    for(int i = 0; i < samples; ++i, buf_pos += sizeof(T)) {
        *pDst++ = *(T*)(frame->data[0] + buf_pos);
        *pDst++ = *(T*)(frame->data[1] + buf_pos);
    }
    return (unsigned char*)pDst;
}

static unsigned char* CopySmaples(unsigned char *dst, AVFrame *frame, int samples, int buf_index)
{
    switch(frame->format) {
    case AV_SAMPLE_FMT_FLTP:
    case AV_SAMPLE_FMT_S32P:
        if(frame->channels == 2)
            return _CopySmaples2<tjs_uint32>(dst, frame, samples, buf_index);
        else
			return _CopySmaples<tjs_uint32>(dst, frame, samples, buf_index);
        break;
    case AV_SAMPLE_FMT_S16P:
        if(frame->channels == 2)
			return _CopySmaples2<tjs_uint16>(dst, frame, samples, buf_index);
        else
			return _CopySmaples<tjs_uint16>(dst, frame, samples, buf_index);
        break;
    }
    return nullptr;
}

bool FFWaveDecoder::Render( void *buf, tjs_uint bufsamplelen, tjs_uint& rendered )
{
	// render output PCM
    if(!InputStream) return false; // InputFile is yet not inited
    int remain = bufsamplelen; // remaining PCM (in sample)
    int sample_size = av_samples_get_buffer_size(NULL, Format.Channels, 1, AVFmt, 1);
    unsigned char *stream = (unsigned char*)buf;
    while(remain)
    {
        if (audio_buf_index >= audio_buf_samples) {
            int decoded_samples = audio_decode_frame();
            if (decoded_samples < 0) {
                break;
            }
            audio_buf_samples = decoded_samples;
            audio_buf_index = 0;
        }
        int samples = audio_buf_samples - audio_buf_index;
        if (samples > remain)
            samples = remain;

        if(!IsPlanar || Format.Channels == 1) {
            memcpy(stream, (frame->data[0] + audio_buf_index * sample_size), samples * sample_size);
            stream += samples * sample_size;
        } else {
            stream = CopySmaples(stream, frame, samples, audio_buf_index);
        }
        remain -= samples;
        audio_buf_index += samples;
    }

    rendered = bufsamplelen - remain; // return rendered PCM samples

    return !remain; //if the decoding is ended
}

bool FFWaveDecoder::SetPosition( tjs_uint64 samplepos )
{
    // set PCM position (seek)
    if(!InputStream) return false;
    if(samplepos && !Format.Seekable) return false;

	int64_t seek_target = samplepos / av_q2d(AudioStream->time_base) / Format.SamplesPerSec;
	if (AudioStream->start_time != AV_NOPTS_VALUE) {
		seek_target += AudioStream->start_time;
	}
    if (Packet.duration <= 0) {
        if (Packet.data)
            av_free_packet(&Packet);
        if(!ReadPacket()) {
            int ret = avformat_seek_file(FormatCtx, StreamIdx, 0, 0, 0, AVSEEK_FLAG_BACKWARD);
            if(ret < 0) return false;
            if(!ReadPacket()) return false;
        }
    }
    int64_t seek_temp = seek_target - Packet.duration;
    for(; ; ) {
        if(seek_temp < 0) seek_temp = 0;
        int ret = avformat_seek_file(FormatCtx, StreamIdx, seek_temp, seek_temp, seek_temp, AVSEEK_FLAG_BACKWARD);
        if(ret < 0) return false;
        if (Packet.data)
            av_free_packet(&Packet);
        if(!ReadPacket()) return false;
        if(seek_target < Packet.dts) {
            seek_temp -= Packet.duration;
            continue;
        }
        pkt_temp = Packet;
        do {
            audio_buf_samples = audio_decode_frame();
            if (audio_buf_samples < 0) {
                return false;
            }
		} while (samplepos > audio_frame_next_pts);
		audio_buf_index = (samplepos - frame->pts) /*/ av_q2d(AudioStream->time_base) / Format.SamplesPerSec*/;
        if(audio_buf_index < 0) audio_buf_index = 0;
        return true;
    }
    return false;
}

bool FFWaveDecoder::SetStream( const ttstr & url )
{
    Clear();
    InputStream = TVPCreateBinaryStreamForRead(url, TJS_W(""));
    if(!InputStream) return false;
    int bufSize = 32 * 1024;
    AVIOContext *pIOCtx = avio_alloc_context(
        (unsigned char *)av_malloc(bufSize + AVPROBE_PADDING_SIZE),
        bufSize,  // internal Buffer and its size
        0,                  // bWriteable (1=true,0=false) 
        InputStream,        // user data ; will be passed to our callback functions
        AVReadFunc, 
        0,                  // Write callback function (not used in this example) 
        AVSeekFunc);

    AVInputFormat *fmt = NULL;
    tTJSNarrowStringHolder holder(url.c_str());
    av_probe_input_buffer2(pIOCtx, &fmt, holder, NULL, 0, 0);
    AVFormatContext *ic = FormatCtx = avformat_alloc_context();
    ic->pb = pIOCtx;
	if (avformat_open_input(&ic, "", fmt, nullptr) < 0) {
        FormatCtx = nullptr;
        return false;
    }
    if (avformat_find_stream_info(ic, nullptr) < 0) {
        return false;
    }

    if (ic->pb)
        ic->pb->eof_reached = 0; // FIXME hack, ffplay maybe should not use url_feof() to test for the end

    float max_frame_duration = (ic->iformat->flags & AVFMT_TS_DISCONT) ? 10.0 : 3600.0;

    StreamIdx = av_find_best_stream(ic, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);

    if(StreamIdx < 0) {
        return false;
    }
    
    AVCodecContext *avctx = ic->streams[StreamIdx]->codec;
    if(avctx->codec_type != AVMEDIA_TYPE_AUDIO) {
        return false;
    }

    AVCodec *codec = avcodec_find_decoder(avctx->codec_id);
    if (!codec) {
        return false;
    }

    avctx->codec_id = codec->id;
    avctx->workaround_bugs = /*workaround_bugs*/1;
    avctx->error_concealment = 3;
    if (codec->capabilities & CODEC_CAP_DR1)
        avctx->flags |= CODEC_FLAG_EMU_EDGE;

    if (avcodec_open2(avctx, codec, nullptr) < 0)
    {
        return false;
    }

    Format.SamplesPerSec = avctx->sample_rate;
    Format.Channels = avctx->channels;
    Format.Seekable = 
		(FormatCtx->iformat->flags & (AVFMT_NOBINSEARCH | AVFMT_NOGENSEARCH | AVFMT_NO_BYTE_SEEK))
		!= (AVFMT_NOBINSEARCH | AVFMT_NOGENSEARCH | AVFMT_NO_BYTE_SEEK);
	Format.IsFloat = false;
// 	Format.BigEndian = false;
// 	Format.Signed = true;
	switch (AVFmt = avctx->sample_fmt) {
    case AV_SAMPLE_FMT_S16P:
    case AV_SAMPLE_FMT_S16:
        Format.BitsPerSample = 16;
        break;
    case AV_SAMPLE_FMT_FLTP:
    case AV_SAMPLE_FMT_FLT:
        Format.BitsPerSample = 32;
		Format.IsFloat = true;
        break;
    case AV_SAMPLE_FMT_S32P:
    case AV_SAMPLE_FMT_S32:
        Format.BitsPerSample = 32;
        break;
    default:
        return false;
    }
    IsPlanar = false;
    if( AVFmt == AV_SAMPLE_FMT_S16P ||
        AVFmt == AV_SAMPLE_FMT_FLTP ||
        AVFmt == AV_SAMPLE_FMT_S32P )
        IsPlanar = true;
    Format.BytesPerSample = Format.BitsPerSample / 8;
    Format.IsFloat = AVFmt == AV_SAMPLE_FMT_FLTP;
    Format.SpeakerConfig = 0;
    AudioStream = FormatCtx->streams[StreamIdx];
    Format.TotalTime = av_q2d(AudioStream->time_base) * AudioStream->duration * 1000;
    Format.TotalSamples = av_q2d(AudioStream->time_base) * AudioStream->duration * Format.SamplesPerSec;

    audio_buf_index = 0;
    audio_buf_samples = 0;
    audio_frame_next_pts = 0;
    pkt_temp.stream_index = -1;

    return true;
}

int FFWaveDecoder::audio_decode_frame() {
    AVStream *audio_st = AudioStream;
    AVCodecContext *dec = audio_st->codec;
    for (;;) {
        /* NOTE: the audio packet can contain several frames */
        while (pkt_temp.stream_index != -1) {
            if(!frame) {
                frame = av_frame_alloc();
            } else {
                av_frame_unref(frame);
            }

            int got_frame;
            int len1 = avcodec_decode_audio4(dec, frame, &got_frame, &pkt_temp);
            if(len1 < 0) {
                /* if error, we skip the frame */
                pkt_temp.size = 0;
                break;
            }
            pkt_temp.dts = pkt_temp.pts = AV_NOPTS_VALUE;
            pkt_temp.data += len1;
            pkt_temp.size -= len1;
            if ((pkt_temp.data && pkt_temp.size <= 0) || (!pkt_temp.data && !got_frame))
                pkt_temp.stream_index = -1;
            if (!pkt_temp.data && !got_frame)
                ; //is->audio_finished = is->audio_pkt_temp_serial;

            if (!got_frame)
                continue;

            AVRational tb = {1, frame->sample_rate};

            if (frame->pts != AV_NOPTS_VALUE)
                frame->pts = av_rescale_q(frame->pts, dec->time_base, tb);
            else if (frame->pkt_pts != AV_NOPTS_VALUE)
                frame->pts = av_rescale_q(frame->pkt_pts, audio_st->time_base, tb);
            else if (audio_frame_next_pts != AV_NOPTS_VALUE) {
                AVRational a = { 1, (int)Format.SamplesPerSec };
                frame->pts = av_rescale_q(audio_frame_next_pts, a, tb);
            }

            if (frame->pts != AV_NOPTS_VALUE)
                audio_frame_next_pts = frame->pts + frame->nb_samples;

//             int data_size = av_samples_get_buffer_size(NULL, av_frame_get_channels(frame),
//                 frame->nb_samples, (AVSampleFormat)frame->format, 1);

            int wanted_nb_samples = frame->nb_samples;

            return frame->nb_samples;
        }

        /* free the current packet */
        if (Packet.data)
            av_free_packet(&Packet);

        pkt_temp.stream_index = -1;

        /* read next packet */
        if(!ReadPacket()) return -1;
        //packet_queue_get(&is->audioq, Packet, 1, &is->audio_pkt_temp_serial);
        
        pkt_temp = Packet;
    }
    return -1;
}

bool FFWaveDecoder::ReadPacket() {
    for(;;) {
        int ret = av_read_frame(FormatCtx, &Packet);
        if (ret < 0) {
            return false;
        }
        if(Packet.stream_index == StreamIdx) {
            stream_start_time = AudioStream->start_time;
            return true;
        }
        av_free_packet(&Packet);
    }
    return false;
}
