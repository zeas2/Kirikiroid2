//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Phase Vocoder Filter
//---------------------------------------------------------------------------

#include "tjsCommHead.h"
#include "PhaseVocoderDSP.h"
#include "PhaseVocoderFilter.h"
#include "WaveIntf.h"
#include "MsgIntf.h"


//---------------------------------------------------------------------------
// tTJSNC_PhaseVocoder : PhaseVocoder TJS native class
//---------------------------------------------------------------------------
tjs_uint32 tTJSNC_PhaseVocoder::ClassID = (tjs_uint32)-1;
tTJSNC_PhaseVocoder::tTJSNC_PhaseVocoder() :
	tTJSNativeClass(TJS_W("PhaseVocoder"))
{
	// register native methods/properties

	TJS_BEGIN_NATIVE_MEMBERS(PhaseVocoder)
	TJS_DECL_EMPTY_FINALIZE_METHOD
//----------------------------------------------------------------------
// constructor/methods
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_CONSTRUCTOR_DECL(/*var.name*/_this, /*var.type*/tTJSNI_PhaseVocoder,
	/*TJS class name*/PhaseVocoder)
{
	return TJS_S_OK;
}
TJS_END_NATIVE_CONSTRUCTOR_DECL(/*TJS class name*/PhaseVocoder)
//----------------------------------------------------------------------

//---------------------------------------------------------------------------




//----------------------------------------------------------------------
// properties
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(interface)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_PhaseVocoder);
		*result = reinterpret_cast<tjs_int64>((iTVPBasicWaveFilter*)_this);
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(interface)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(window)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_PhaseVocoder);
		*result = (tjs_int64)(_this->GetWindow());
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_PhaseVocoder);
		_this->SetWindow(*param);
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(window)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(overlap)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_PhaseVocoder);
		*result = (tjs_int64)(_this->GetOverlap());
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_PhaseVocoder);
		_this->SetOverlap(*param);
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(overlap)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(pitch)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_PhaseVocoder);
		*result = (double)_this->GetPitch();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_PhaseVocoder);
		_this->SetPitch((float)(double)*param);
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(pitch)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(time)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_PhaseVocoder);
		*result = (double)_this->GetTime();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_PhaseVocoder);
		_this->SetTime((float)(double)*param);
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(time)
//---------------------------------------------------------------------------

//----------------------------------------------------------------------
	TJS_END_NATIVE_MEMBERS
}
//---------------------------------------------------------------------------
iTJSNativeInstance *tTJSNC_PhaseVocoder::CreateNativeInstance()
{
	return new tTJSNI_PhaseVocoder();
}
//---------------------------------------------------------------------------





//---------------------------------------------------------------------------
tTJSNI_PhaseVocoder::tTJSNI_PhaseVocoder()
{
	Window = 4096;
	Overlap = 0;
	Pitch = 1.0;
	Time = 1.0;

	Source = NULL;
	PhaseVocoder = NULL;
	FormatConvertBuffer = NULL;
	FormatConvertBufferSize = 0;

}
//---------------------------------------------------------------------------
tTJSNI_PhaseVocoder::~tTJSNI_PhaseVocoder()
{
	if(PhaseVocoder) delete PhaseVocoder, PhaseVocoder = NULL;
	if(FormatConvertBuffer) delete [] FormatConvertBuffer, FormatConvertBuffer = NULL, FormatConvertBufferSize = 0;
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD
	tTJSNI_PhaseVocoder::Construct(tjs_int numparams, tTJSVariant **param,
		iTJSDispatch2 *tjs_obj)
{
	return TJS_S_OK;
}
//---------------------------------------------------------------------------
void TJS_INTF_METHOD tTJSNI_PhaseVocoder::Invalidate()
{
}
//---------------------------------------------------------------------------
void tTJSNI_PhaseVocoder::SetWindow(int window)
{
	// 値をチェック
	switch(window)
	{
	case 64: case 128: case 256: case 512: case 1024: case 2048: case 4096: case 8192:
	case 16384: case 32768:
		break;
	default:
		TVPThrowExceptionMessage(TVPInvalidWindowSizeMustBeIn64to32768);
	}
	Window = window;
}
//---------------------------------------------------------------------------
void tTJSNI_PhaseVocoder::SetOverlap(int overlap)
{
	// 値をチェック
	switch(overlap)
	{
	case 0:
	case 2: case 4: case 8: case 16: case 32:
		break;
	default:
		TVPThrowExceptionMessage(TVPInvalidOverlapCountMustBeIn2to32);
	}
	Overlap = overlap;
}
//---------------------------------------------------------------------------
tTVPSampleAndLabelSource * tTJSNI_PhaseVocoder::Recreate(tTVPSampleAndLabelSource * source)
{
	if(Source)
		TVPThrowExceptionMessage(TVPCannotConnectMultipleWaveSoundBufferAtOnce);

	if(PhaseVocoder) delete PhaseVocoder, PhaseVocoder = NULL;

	Source = source;
	InputFormat = Source->GetFormat();
	OutputFormat = InputFormat;
	OutputFormat.IsFloat = true; // 出力は float
	OutputFormat.BitsPerSample = 32; // ビットは 32 ビット
	OutputFormat.BytesPerSample = 4;

	return this;
}
//---------------------------------------------------------------------------
void tTJSNI_PhaseVocoder::Reset(void)
{
	InputSegments.Clear();
	OutputSegments.Clear();
	if(PhaseVocoder) delete PhaseVocoder, PhaseVocoder = NULL;
}
//---------------------------------------------------------------------------
void tTJSNI_PhaseVocoder::Clear(void)
{
	Source = NULL;
	InputSegments.Clear();
	OutputSegments.Clear();
	if(PhaseVocoder) delete PhaseVocoder, PhaseVocoder = NULL;
	if(FormatConvertBuffer) delete [] FormatConvertBuffer, FormatConvertBuffer = NULL, FormatConvertBufferSize = 0;
}
//---------------------------------------------------------------------------
void tTJSNI_PhaseVocoder::Update(void)
{
	// Update filter internal state.
	// Note that this method may be called by non-main thread (decoding thread)
	// and setting Pitch and Time may be called from main thread, and these may
	// be simultaneous.
	// We do not care about that because typically writing size of float is atomic
	// on most platform. (I don't know any platform which does not guarantee that).
	if(PhaseVocoder)
	{
		PhaseVocoder->SetFrequencyScale(Pitch);
		PhaseVocoder->SetTimeScale(Time);
		PhaseVocoder->SetOverSampling(Overlap);
	}
}
//---------------------------------------------------------------------------
void tTJSNI_PhaseVocoder::Fill(float * dest, tjs_uint samples, tjs_uint &written,
		tTVPWaveSegmentQueue & segments)
{
	if(InputFormat.IsFloat && InputFormat.BitsPerSample == 32 && InputFormat.BytesPerSample == 4)
	{
		// 入力も32bitフロートなので変換の必要はない
		Source->Decode(dest, samples, written, segments);
	}
	else
	{
		// 入力が32bitフロートではないので変換の必要がある
		// いったん変換バッファにためる
		tjs_uint buf_size = samples * InputFormat.BytesPerSample * InputFormat.Channels;
		if(FormatConvertBufferSize < buf_size)
		{
			// バッファを再確保
			if(FormatConvertBuffer) delete [] FormatConvertBuffer, FormatConvertBuffer = NULL;
			FormatConvertBuffer = new char[buf_size];
			FormatConvertBufferSize = buf_size;
		}
		// バッファにデコードを行う
		Source->Decode(FormatConvertBuffer, samples, written, segments);
		// 変換を行う
		TVPConvertPCMToFloat(dest, FormatConvertBuffer, InputFormat, written);
	}
	if(written < samples)
	{
		// デコードされたサンプル数が要求されたサンプル数に満たない場合
		// 残りを 0 で埋める
		memset(dest + written * InputFormat.Channels, 0,
			(samples - written) * sizeof(float) * InputFormat.Channels);
	}
}
//---------------------------------------------------------------------------
void tTJSNI_PhaseVocoder::Decode(void *dest, tjs_uint samples, tjs_uint &written,
	tTVPWaveSegmentQueue & segments)
{
	if(!PhaseVocoder)
	{
		// PhaseVocoder を作成
		tRisaPhaseVocoderDSP * pv = new tRisaPhaseVocoderDSP(Window,
					InputFormat.SamplesPerSec, InputFormat.Channels);
		pv->SetFrequencyScale(Pitch);
		pv->SetTimeScale(Time);
		pv->SetOverSampling(Overlap);
		PhaseVocoder = pv; // now visible from other function
	}

	size_t inputhopsize = PhaseVocoder->GetInputHopSize();
	size_t outputhopsize = PhaseVocoder->GetOutputHopSize();
	tTVPWaveSegmentQueue queue;

	float * dest_buf = (float*) dest;
	written = 0;
	while(samples > 0)
	{
		tRisaPhaseVocoderDSP::tStatus status;
		do
		{
			size_t inputfree = PhaseVocoder->GetInputFreeSize();
			if(inputfree >= inputhopsize)
			{
				// 入力にデータを流し込む
				float *p1, *p2;
				size_t p1len, p2len;
				PhaseVocoder->GetInputBuffer(inputhopsize, p1, p1len, p2, p2len);
				tjs_uint filled = 0;
				tjs_uint total = 0;
				Fill       (p1, (tjs_uint)p1len, filled, InputSegments), total += filled;
				if(p2) Fill(p2, (tjs_uint)p2len, filled, InputSegments), total += filled;
				if(total == 0) { break ; } // もうデータがない
			}

			// PhaseVocoderの処理を行う
			// 一回の処理では、入力をinputhopsize分消費し、出力を
			// outputhopsize分出力する。
			status = PhaseVocoder->Process();
			if(status == tRisaPhaseVocoderDSP::psNoError)
			{
				// 処理に成功。inputhopsize 分のキューを InputSegments から読み出し、
				// outputhopsize 分にスケールし直した後、OutputSegments に書き込む。
				InputSegments.Dequeue(queue, inputhopsize);
				queue.Scale(outputhopsize);
				OutputSegments.Enqueue(queue);
			}
		} while(status == tRisaPhaseVocoderDSP::psInputNotEnough);

		// 入力にデータを流し込んでおいて出力が無いことはないが
		// 要求したサイズよりも小さい場合はある
		size_t output_ready = PhaseVocoder->GetOutputReadySize();
		if(output_ready >= outputhopsize)
		{
			// PhaseVocoder の出力から dest にコピーする
			size_t copy_size = outputhopsize > samples ? samples : outputhopsize;
			const float *p1, *p2;
			size_t p1len, p2len;
			PhaseVocoder->GetOutputBuffer(copy_size, p1, p1len, p2, p2len);
			memcpy(dest_buf, p1, p1len * sizeof(float)*OutputFormat.Channels);
			if(p2) memcpy(dest_buf + p1len * OutputFormat.Channels, p2,
							p2len * sizeof(float)*OutputFormat.Channels);

			samples  -= (tjs_uint)copy_size;
			written  += (tjs_uint)copy_size;
			dest_buf += copy_size * OutputFormat.Channels;

			// segment queue の書き出し
			OutputSegments.Dequeue(queue, copy_size);
			segments.Enqueue(queue);
		}
		else
		{
			return; // もう入力データも無ければ出力データもない
		}
	}
}
//---------------------------------------------------------------------------




