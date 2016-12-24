#ifndef TVP_IWAVEUNPACKER_DEFINED
#define TVP_IWAVEUNPACKER_DEFINED
class IWaveUnpacker
{
public:
// IUnknown 派生クラスではないので注意
	virtual ULONG STDMETHODCALLTYPE AddRef(void) = 0;
	virtual ULONG STDMETHODCALLTYPE Release(void) = 0;

// IWaveUnpacker
	virtual HRESULT STDMETHODCALLTYPE GetTypeName(char *buf,long buflen)=0;
		/*
			buf に、この Wave 形式を表す文字列を *buf に設定してください。
			buflen は、buf に確保された文字列で、null terminater も含むので
			注意。
		*/

	virtual HRESULT STDMETHODCALLTYPE GetWaveFormat(long *samplepersec,
		long *channels,long *bitspersample)=0;
		/*
			出力する Wave の形式を *samplepersec, *channels, *bitspersample に
			返してください。
		*/

	virtual HRESULT STDMETHODCALLTYPE Render(void *buffer,long bufsize,
		long *numwrite) =0;
		/*
			デコードしてください。
			bufsize には buffer のサイズがバイト単位で指定されます。
			numwrite には、バッファに書かれたデータの数をバイト単位で返します。
			ただし、WaveUnpacker は、numwrite<bufsize の場合は、残りを
			0 で埋めてください。
		*/
	
	virtual HRESULT STDMETHODCALLTYPE GetLength(long *length)=0;
		/*
			データ長を ms 単位で *length に返してください。
			対応できない場合は E_NOTIMPL を返してください。その場合は
			WaveSoundBuffer の totalTime プロパティは 0 を表すようになります。
		*/

	virtual HRESULT STDMETHODCALLTYPE GetCurrentPosition(long *pos)=0;
		/*
			現在のデコード位置を *pos に返してください。
			対応できない場合は E_NOTIMPL を返してください。その場合は
			WaveSoundBuffer の position プロパティは意味のない数値を
			示すようになります。
		*/

	virtual HRESULT STDMETHODCALLTYPE SetCurrentPosition(long pos)=0;
		/*
			現在のデコード位置を設定してください。pos は ms 単位での
			位置です。
			最低でも pos=0 として呼ばれたときに、先頭への巻き戻しが
			出来ようにしてください。

			そのほかの場合、対応できない場合は E_NOTIMPL を返してください。
			その場合はWaveSoundBuffer の position プロパティへの代入は無視されます。
		*/

	virtual HRESULT STDMETHODCALLTYPE Invoke(); // reserved
};

#endif
//---------------------------------------------------------------------------
