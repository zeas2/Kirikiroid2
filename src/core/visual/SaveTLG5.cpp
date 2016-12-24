#include "tjsCommHead.h"

#include "GraphicsLoaderIntf.h"
#include "MsgIntf.h"
#include "tjsUtils.h"
#include "tvpgl.h"
#include "LayerBitmapIntf.h"
#include "StorageIntf.h"
#include "BinaryStream.h"

#include <stdlib.h>
#include "SaveTLG.h"

//---------------------------------------------------------------------------
SlideCompressor::SlideCompressor()
{
	S = 0;
	for(int i = 0; i < SLIDE_N + SLIDE_M; i++) Text[i] = 0;
	for(int i = 0; i < 256*256; i++)
		Map[i] = -1;
	for(int i = 0; i < SLIDE_N; i++)
		Chains[i].Prev = Chains[i].Next = -1;
	for(int i = SLIDE_N - 1; i >= 0; i--)
		AddMap(i);
}
//---------------------------------------------------------------------------
SlideCompressor::~SlideCompressor()
{
}
//---------------------------------------------------------------------------
int SlideCompressor::GetMatch(const unsigned char*cur, int curlen, int &pos, int s)
{
	// get match length
	if(curlen < 3) return 0;

	int place = cur[0] + ((int)cur[1] << 8);

	int maxlen = 0;
	if((place = Map[place]) != -1)
	{
		int place_org;
		curlen -= 1;
		do
		{
			place_org = place;
			if(s == place || s == ((place + 1) & (SLIDE_N -1))) continue;
			place += 2;
			int lim = (SLIDE_M < curlen ? SLIDE_M : curlen) + place_org;
			const unsigned char *c = cur + 2;
			if(lim >= SLIDE_N)
			{
				if(place_org <= s && s < SLIDE_N)
					lim = s;
				else if(s < (lim&(SLIDE_N-1)))
					lim = s + SLIDE_N;
			}
			else
			{
				if(place_org <= s && s < lim)
					lim = s;
			}
			while(Text[place] == *(c++) && place < lim) place++;
			int matchlen = place - place_org;
			if(matchlen > maxlen) pos = place_org, maxlen = matchlen;
			if(matchlen == SLIDE_M) return maxlen;

		} while((place = Chains[place_org].Next) != -1);
	}
	return maxlen;
}
//---------------------------------------------------------------------------
void SlideCompressor::AddMap(int p)
{
	int place = Text[p] + ((int)Text[(p + 1) & (SLIDE_N - 1)] << 8);

	if(Map[place] == -1)
	{
		// first insertion
		Map[place] = p;
	}
	else
	{
		// not first insertion
		int old = Map[place];
		Map[place] = p;
		Chains[old].Prev = p;
		Chains[p].Next = old;
		Chains[p].Prev = -1;
	}
}
//---------------------------------------------------------------------------
void SlideCompressor::DeleteMap(int p)
{
	int n;
	if((n = Chains[p].Next) != -1)
		Chains[n].Prev = Chains[p].Prev;

	if((n = Chains[p].Prev) != -1)
	{
		Chains[n].Next = Chains[p].Next;
	}
	else if(Chains[p].Next != -1)
	{
		int place = Text[p] + ((int)Text[(p + 1) & (SLIDE_N - 1)] << 8);
		Map[place] = Chains[p].Next;
	}
	else
	{
		int place = Text[p] + ((int)Text[(p + 1) & (SLIDE_N - 1)] << 8);
		Map[place] = -1;
	}

	Chains[p].Prev = -1;
	Chains[p].Next = -1;
}
//---------------------------------------------------------------------------
void SlideCompressor::Encode(const unsigned char *in, long inlen,
		unsigned char *out, long & outlen)
{
	unsigned char code[40], codeptr, mask;

	if(inlen == 0) return;

	outlen = 0;
	code[0] = 0;
	codeptr = mask = 1;

	int s = S;
	while(inlen > 0)
	{
		int pos = 0;
		int len = GetMatch(in, inlen, pos, s);
		if(len >= 3)
		{
			code[0] |= mask;
			if(len >= 18)
			{
				code[codeptr++] = pos & 0xff;
				code[codeptr++] = ((pos &0xf00)>> 8) | 0xf0;
				code[codeptr++] = len - 18;
			}
			else
			{
				code[codeptr++] = pos & 0xff;
				code[codeptr++] = ((pos&0xf00)>> 8) | ((len-3)<<4);
			}
			while(len--)
			{
				unsigned char c = 0[in++];
				DeleteMap((s - 1) & (SLIDE_N - 1));
				DeleteMap(s);
				if(s < SLIDE_M - 1) Text[s + SLIDE_N] = c;
				Text[s] = c;
				AddMap((s - 1) & (SLIDE_N - 1));
				AddMap(s);
				s++;
				inlen--;
				s &= (SLIDE_N - 1);
			}
		}
		else
		{
			unsigned char c = 0[in++];
			DeleteMap((s - 1) & (SLIDE_N - 1));
			DeleteMap(s);
			if(s < SLIDE_M - 1) Text[s + SLIDE_N] = c;
			Text[s] = c;
	 		AddMap((s - 1) & (SLIDE_N - 1));
			AddMap(s);
			s++;
			inlen--;
			s &= (SLIDE_N - 1);
			code[codeptr++] = c;
		}
		mask <<= 1;

		if(mask == 0)
		{
			for(int i = 0; i < codeptr; i++)
				out[outlen++] = code[i];
			mask = codeptr = 1;
			code[0] = 0;
		}
	}

	if(mask != 1)
	{
		for(int i = 0; i < codeptr; i++)
			out[outlen++] = code[i];
	}

	S = s;
}
//---------------------------------------------------------------------------
void SlideCompressor::Store()
{
	S2 = S;
	int i;
	for(i = 0; i < SLIDE_N + SLIDE_M - 1; i++)
		Text2[i] = Text[i];

	for(i = 0; i < 256*256; i++)
		Map2[i] = Map[i];

	for(i = 0; i < SLIDE_N; i++)
		Chains2[i] = Chains[i];
}
//---------------------------------------------------------------------------
void SlideCompressor::Restore()
{
	S = S2;
	int i;
	for(i = 0; i < SLIDE_N + SLIDE_M - 1; i++)
		Text[i] = Text2[i];

	for(i = 0; i < 256*256; i++)
		Map[i] = Map2[i];

	for(i = 0; i < SLIDE_N; i++)
		Chains[i] = Chains2[i];
}
//---------------------------------------------------------------------------

#define BLOCK_HEIGHT 4
//---------------------------------------------------------------------------
static void WriteInt32(long num, tTJSBinaryStream *out)
{
	char buf[4];
	buf[0] = num & 0xff;
	buf[1] = (num >> 8) & 0xff;
	buf[2] = (num >> 16) & 0xff;
	buf[3] = (num >> 24) & 0xff;
	out->WriteBuffer(buf, 4);
}
//---------------------------------------------------------------------------
static void Compress( const iTVPBaseBitmap *bmp, tTJSBinaryStream * out, bool is24 = false )
{
	// compress 'bmp' to 'out'.
	// bmp will be modified (destroyed).
	int colors;

	if( bmp->Is32BPP() ) {
		if( is24 ) colors = 3;
		else colors = 4;
	} else {
		colors = 1;
	}

	// header
	{
		out->WriteBuffer("TLG5.0\x00raw\x1a\x00", 11);
		out->WriteBuffer(&colors, 1);
		int width = bmp->GetWidth();
		int height = bmp->GetHeight();
		WriteInt32(width, out);
		WriteInt32(height, out);
		int blockheight = BLOCK_HEIGHT;
		WriteInt32(blockheight, out);
	}

	int blockcount = (int)((bmp->GetHeight() - 1) / BLOCK_HEIGHT) + 1;


	// buffers/compressors
	SlideCompressor * compressor = NULL;
	unsigned char *cmpinbuf[4];
	unsigned char *cmpoutbuf[4];
	for(int i = 0; i < colors; i++)
		cmpinbuf[i] = cmpoutbuf[i] = NULL;
	long written[4];
	int *blocksizes;

	// allocate buffers/compressors
	try
	{
		compressor = new SlideCompressor();
		for(int i = 0; i < colors; i++)
		{
			cmpinbuf[i] = new unsigned char [bmp->GetWidth() * BLOCK_HEIGHT];
			cmpoutbuf[i] = new unsigned char [bmp->GetWidth() * BLOCK_HEIGHT * 9 / 4];
			written[i] = 0;
		}
		blocksizes = new int[blockcount];

		tjs_uint64 blocksizepos = out->GetPosition();
		// write block size header
		// (later fill this)
		for(int i = 0; i < blockcount; i++)
		{
			out->WriteBuffer("    ", 4);
		}

		//
		int block = 0;
		for(int blk_y = 0; blk_y < (int)bmp->GetHeight(); blk_y += BLOCK_HEIGHT, block++)
		{
			int ylim = blk_y + BLOCK_HEIGHT;
			if(ylim > (int)bmp->GetHeight()) ylim = bmp->GetHeight();

			int inp = 0;


			for(int y = blk_y; y < ylim; y++)
			{
				// retrieve scan lines
				const unsigned char * upper;
				if(y != 0)
					upper = (const unsigned char *)bmp->GetScanLine(y-1);
				else
					upper = NULL;
				const unsigned char * current;
				current = (const unsigned char *)bmp->GetScanLine(y);

				// prepare buffer
				int prevcl[4];
				int val[4];

				for(int c = 0; c < colors; c++) prevcl[c] = 0;

				for(int x = 0; x < (int)bmp->GetWidth(); x++)
				{
					for(int c = 0; c < colors; c++)
					{
						int cl;
						if(upper)
							cl = 0[current++] - 0[upper++];
						else
							cl = 0[current++];
						val[c] = cl - prevcl[c];
						prevcl[c] = cl;
					}
					// composite colors
					switch(colors)
					{
					case 1:
						cmpinbuf[0][inp] = val[0];
						break;
					case 3:
						cmpinbuf[0][inp] = val[0] - val[1];
						cmpinbuf[1][inp] = val[1];
						cmpinbuf[2][inp] = val[2] - val[1];
						// skip alpha
						current++;
						if( upper ) upper++;
						break;
					case 4:
						cmpinbuf[0][inp] = val[0] - val[1];
						cmpinbuf[1][inp] = val[1];
						cmpinbuf[2][inp] = val[2] - val[1];
						cmpinbuf[3][inp] = val[3];
						break;
					}

					inp++;
				}

			}


			// compress buffer and write to the file

			// LZSS
			int blocksize = 0;
			for(int c = 0; c < colors; c++)
			{
				long wrote = 0;
				compressor->Store();
				compressor->Encode(cmpinbuf[c], inp,
					cmpoutbuf[c], wrote);
				if(wrote < inp)
				{
					out->WriteBuffer("\x00", 1);
					WriteInt32(wrote, out);
					out->WriteBuffer(cmpoutbuf[c], wrote);
					blocksize += wrote + 4 + 1;
				}
				else
				{
					compressor->Restore();
					out->WriteBuffer("\x01", 1);
					WriteInt32(inp, out);
					out->WriteBuffer(cmpinbuf[c], inp);
					blocksize += inp + 4 + 1;
				}
				written[c] += wrote;
			}

			blocksizes[block] = blocksize;
		}


		// write block sizes
		tjs_uint64 pos_save = out->GetPosition();
		out->SetPosition( blocksizepos );
		for(int i = 0; i < blockcount; i++)
		{
			WriteInt32(blocksizes[i], out);
		}
		out->SetPosition( pos_save );



		// deallocate buffers/compressors
	}
	catch(...)
	{
		for(int i = 0; i < colors; i++)
		{
			if(cmpinbuf[i]) delete [] cmpinbuf[i];
			if(cmpoutbuf[i]) delete [] cmpoutbuf[i];
		}
		if(compressor) delete compressor;
		if(blocksizes) delete [] blocksizes;
		throw;
	}
	for(int i = 0; i < colors; i++)
	{
		if(cmpinbuf[i]) delete [] cmpinbuf[i];
		if(cmpoutbuf[i]) delete [] cmpoutbuf[i];
	}
	if(compressor) delete compressor;
	if(blocksizes) delete [] blocksizes;
}
//---------------------------------------------------------------------------
void SaveTLG5( tTJSBinaryStream* stream, const iTVPBaseBitmap* image, bool is24 )
{
	Compress(image, stream, is24);
}
//---------------------------------------------------------------------------


