//---------------------------------------------------------------------------
#include "ncbind/ncbind.hpp"
#include <stdio.h>
#include "WaveIntf.h"
//#include "typedefine.h"
#define BYTE uint8_t
#define LONG int32_t
#define ULONG uint32_t

#define NCB_MODULE_NAME TJS_W("wutcwf.dll")

#pragma pack(push,1)
struct TTCWFHeader
{
	char mark[6];  // = "TCWF0\x1a"
	BYTE channels; // チャネル数
	BYTE reserved;
	LONG frequency; // サンプリング周波数
	LONG numblocks; // ブロック数
	LONG bytesperblock; // ブロックごとのバイト数
	LONG samplesperblock; // ブロックごとのサンプル数
};
struct TTCWUnexpectedPeak
{
	unsigned short int pos;
	short int revise;
};
struct TTCWBlockHeader  // ブロックヘッダ ( ステレオの場合はブロックが右・左の順に２つ続く)
{
	short int ms_sample0;
	short int ms_sample1;
	short int ms_idelta;
	unsigned char ms_bpred;
	unsigned char ima_stepindex;
	TTCWUnexpectedPeak peaks[6];
};
#pragma pack(pop)

static const int AdaptationTable[]= 
{
	230, 230, 230, 230, 307, 409, 512, 614,
	768, 614, 512, 409, 307, 230, 230, 230 
};

static const int AdaptCoeff1 [] = 
{	256, 512, 0, 192, 240, 460, 392 
} ;

static const int AdaptCoeff2 [] = 
{	0, -256, 0, 64, 0, -208, -232
} ;

static const int ima_index_adjust [16] =
{	-1, -1, -1, -1,		// +0 - +3, decrease the step size
	 2,  4,  6,  8,     // +4 - +7, increase the step size
	-1, -1, -1, -1,		// -0 - -3, decrease the step size
	 2,  4,  6,  8,		// -4 - -7, increase the step size
} ;

static const int ima_step_size [89] = 
{	7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45, 
	50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130, 143, 157, 173, 190, 209, 230, 
	253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658, 724, 796, 876, 963, 
	1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327, 
	3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442,
	11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 
	32767
} ;

//---------------------------------------------------------------------------
class TCWFDecoder : public tTVPWaveDecoder
{
    TTCWFHeader Header;
    tTJSBinaryStream *InputStream;
    tjs_int64 StreamPos;
    short int *SamplePos;
    long DataStart;
    long DataSize;
    tjs_int64 Pos;
    long BufferRemain;
    BYTE *BlockBuffer;
    short int *Samples;
    tTVPWaveFormat TSSFormat;

public:
    TCWFDecoder();
    ~TCWFDecoder();

public:
    // ITSSWaveDecoder
    virtual void GetFormat(tTVPWaveFormat & format);
    virtual bool Render(void *buf, tjs_uint bufsamplelen, tjs_uint& rendered);
    virtual bool SetPosition(tjs_uint64 samplepos);

    bool Open(const ttstr & url);
    bool ReadBlock(int , int );
};

class TCWFWaveDecoderCreator : public tTVPWaveDecoderCreator
{
public:
    tTVPWaveDecoder * Create(const ttstr & storagename, const ttstr & extension) {
        TCWFDecoder * decoder = new TCWFDecoder();
        if(!decoder->Open(storagename))
        {
            delete decoder;
            return nullptr;
        }
        return decoder;
    }
};
//---------------------------------------------------------------------------
// TCWFDecoder インプリメンテーション #######################################
//---------------------------------------------------------------------------
TCWFDecoder::TCWFDecoder()
{
	InputStream = NULL;
	BlockBuffer=NULL;
	Samples=NULL;
}
//---------------------------------------------------------------------------
TCWFDecoder::~TCWFDecoder()
{
	if(InputStream)
	{
		delete InputStream;
		InputStream = NULL;
	}
	if(BlockBuffer) delete [] BlockBuffer;
	if(Samples) delete [] Samples;
}
//---------------------------------------------------------------------------
void  TCWFDecoder::GetFormat(tTVPWaveFormat & format)
{
	format = TSSFormat;
}
//---------------------------------------------------------------------------
bool  TCWFDecoder::Render(void *buf, tjs_uint bufsamplelen, tjs_uint& rendered)
{
	// 展開
	unsigned long n;
	short int *pbuf=(short int*)buf;
	for(n=0;n<bufsamplelen;n++)
	{
		if(BufferRemain<=0)
		{
			int i;
			for(i=0; i<Header.channels; i++)
			{
				if(!ReadBlock(Header.channels, i))
				{
					rendered = n;
					Pos+=n;
					return false;
				}
			}
			SamplePos = Samples;
			BufferRemain = Header.samplesperblock;
		}
		int i = Header.channels;
		while(i--) *(pbuf++)=*(SamplePos++);
		BufferRemain--;
	}

	rendered=n;
	Pos+=n;
	return true;
}
//---------------------------------------------------------------------------
bool  TCWFDecoder::SetPosition(tjs_uint64 samplepos)
{
	// pos (ms単位) に移動する
	// 現在位置を保存
	tjs_uint64 bytepossave=InputStream->GetPosition();
	tjs_uint64 samplepossave=Pos;

	// 新しい位置を特定
	long newbytepos = samplepos / (Header.samplesperblock);
	long remnant = samplepos - newbytepos * (Header.samplesperblock);
	Pos = samplepos;
	newbytepos *= Header.bytesperblock * Header.channels;

	// シーク
    tjs_uint64 newpos = DataStart+newbytepos;
    if(InputStream->Seek(DataStart+newbytepos, TJS_BS_SEEK_SET) != newpos)
    {
		// シーク失敗
		newpos=bytepossave;
		InputStream->SetPosition(newpos);
		Pos=samplepossave;
		return false;
	}
	
	StreamPos=DataStart+newbytepos;

	int i;
	for(i=0; i<Header.channels; i++)
	{
		ReadBlock(Header.channels, i);
	}

	SamplePos = Samples + remnant * Header.channels;
	BufferRemain = Header.samplesperblock - remnant;

	return true;
}
//---------------------------------------------------------------------------
bool TCWFDecoder::Open(const ttstr & url)
{
	// url で指定された URL を開きます
	InputStream = TVPCreateStream(url, TJS_BS_READ);
	if(!InputStream)
	{
		return false;
	}
	InputStream->SetPosition(0);
	// TCWF0 チェック
	tjs_uint read = InputStream->Read(&Header, sizeof(Header));
	if(read!=sizeof(Header)) return false;
	if (memcmp(Header.mark, "TCWF0\x1a", 6)) return false; // マーク

	// 現在位置を取得
	StreamPos=InputStream->GetPosition();
	DataStart=StreamPos;

	// その他、初期化
	BufferRemain=0;
	Pos=0;

	memset(&TSSFormat, 0, sizeof(TSSFormat));
	TSSFormat.SamplesPerSec = Header.frequency;
	TSSFormat.Channels = Header.channels;
    TSSFormat.BitsPerSample = 16;
    TSSFormat.BytesPerSample = 2;
	TSSFormat.Seekable = 2;
	TSSFormat.TotalSamples = 0;
	TSSFormat.TotalTime = 0;
    TSSFormat.SpeakerConfig = 0;
    TSSFormat.IsFloat = false;
// 	TSSFormat.BigEndian = false;
// 	TSSFormat.Signed = true;

	return true;
}
//---------------------------------------------------------------------------
bool TCWFDecoder::ReadBlock(int numchans, int chan)
{

	// メモリ確保
	if(!BlockBuffer)
		BlockBuffer=new BYTE[Header.bytesperblock];
	if(!this->Samples)
		this->Samples=new short int[Header.samplesperblock * Header.channels];

	short int * Samples = this->Samples + chan;


	// シーク
	if(InputStream->GetPosition()!=StreamPos)
	{
		if(InputStream->Seek(StreamPos, TJS_BS_SEEK_SET) != StreamPos) return false;
	}


	// ブロックヘッダ読み込み
	TTCWBlockHeader bheader;
	ULONG read;
	read = InputStream->Read(&bheader,sizeof(bheader));
	StreamPos+=read;
	if(sizeof(bheader)!=read) return false;
	read = InputStream->Read(BlockBuffer, Header.bytesperblock - sizeof(bheader));
	StreamPos+=read;
	if((ULONG)Header.bytesperblock- sizeof(bheader)!=read) return false;

	// デコード
	Samples[0*numchans] = bheader.ms_sample0;
	Samples[1*numchans] = bheader.ms_sample1;
	int idelta = bheader.ms_idelta;
	int bpred = bheader.ms_bpred;
	if(bpred>=7) return false; // おそらく同期がとれていない

	int k;
	int p;

	//MS ADPCM デコード
	int predict;
	int bytecode;
	for (k = 2, p = 0 ; k < Header.samplesperblock ; k ++, p++)
	{
		bytecode = BlockBuffer[p] & 0xF ;

	    int idelta_save=idelta;
		idelta = (AdaptationTable [bytecode] * idelta) >> 8 ;
	    if (idelta < 16) idelta = 16;
	    if (bytecode & 0x8) bytecode -= 0x10 ;
	
    	predict = ((Samples [(k - 1)*numchans] * AdaptCoeff1 [bpred]) 
					+ (Samples [(k - 2)*numchans] * AdaptCoeff2 [bpred])) >> 8 ;
 
		int current = (bytecode * idelta_save) + predict;
    
	    if (current > 32767) 
			current = 32767 ;
	    else if (current < -32768) 
			current = -32768 ;
    
		Samples [k*numchans] = (short int) current ;
	};

	//IMA ADPCM デコード
	int step;
	int stepindex = bheader.ima_stepindex;
	int prev = 0;
	int diff;
	for (k = 2, p = 0 ; k < Header.samplesperblock ; k ++, p++)
	{
		bytecode= (BlockBuffer[p]>>4) & 0xF;
		
		step = ima_step_size [stepindex] ;
		int current = prev;
  

		diff = step >> 3 ;
		if (bytecode & 1) 
			diff += step >> 2 ;
		if (bytecode & 2) 
			diff += step >> 1 ;
		if (bytecode & 4) 
			diff += step ;
		if (bytecode & 8) 
			diff = -diff ;

		current += diff ;

		if (current > 32767) current = 32767;
		else if (current < -32768) current = -32768 ;

		stepindex+= ima_index_adjust [bytecode] ;
	
		if (stepindex< 0)  stepindex = 0 ;
		else if (stepindex > 88)	stepindex = 88 ;

		prev = current ;

		int n = Samples[k*numchans];
		n+=current;
		if (n > 32767) n = 32767;
		else if (n < -32768) n = -32768 ;
		Samples[k*numchans] =n;
	};

	// unexpected peak の修正
	int i;
	for(i=0; i<6; i++)
	{
		if(bheader.peaks[i].revise)
		{
			int pos = bheader.peaks[i].pos;
			int n = Samples[pos*numchans];
			n -= bheader.peaks[i].revise;
			if (n > 32767) n = 32767;
			else if (n < -32768) n = -32768 ;
			Samples[pos*numchans] = n;
		}
	}

	return true;
}
//---------------------------------------------------------------------------

static TCWFWaveDecoderCreator creator;
static void _init()
{
    TVPRegisterWaveDecoderCreator(&creator);
}

NCB_PRE_REGIST_CALLBACK(_init);
