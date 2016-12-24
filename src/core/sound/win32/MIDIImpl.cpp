//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000-2007 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// MIDI sequencer implementation
//---------------------------------------------------------------------------
#include "tjsCommHead.h"

#include "MIDIImpl.h"

#include <algorithm>
#include "TickCount.h"
#include "StorageIntf.h"
#include "MsgIntf.h"
#include "SysInitIntf.h"
#include "ThreadIntf.h"
#include "UtilStreams.h"


#ifdef TVP_ENABLE_MIDI
//---------------------------------------------------------------------------
// MIDI output routines
//---------------------------------------------------------------------------
static HMIDIOUT TVPMIDIOutHandle = NULL;
static tTJSStaticCriticalSection TVPMIDIOutCS;
static std::vector<tTJSNI_MIDISoundBuffer*> TVPMIDIBuffers;
static bool TVPMIDIDelayAlive = false;
static uint32_t TVPMIDIDelayTick = 0;
//---------------------------------------------------------------------------
static void TVPInitMIDIOut(void)
{
    if(!TVPMIDIOutHandle)
		midiOutOpen(&TVPMIDIOutHandle, (UINT)MIDI_MAPPER, NULL, 0, CALLBACK_NULL);
}
//---------------------------------------------------------------------------
static void TVPUninitMIDIOut(void)
{
    if(TVPMIDIOutHandle) midiOutClose(TVPMIDIOutHandle), TVPMIDIOutHandle=NULL;
}
//---------------------------------------------------------------------------
static tTVPAtExit
	TVPUninitMIDIOutAtExit(TVP_ATEXIT_PRI_RELEASE, TVPUninitMIDIOut);
//---------------------------------------------------------------------------
static void TVPCheckMIDIDelay()
{
	if(!TVPMIDIDelayAlive) return;

	int delay = TVPMIDIDelayTick - TVPGetTickTime();
	if(delay > 0) Sleep(delay);

	TVPMIDIDelayAlive = false;
}
//---------------------------------------------------------------------------
static void TVPMIDIOut(tjs_uint8 *data,int len)
{
    if(!TVPMIDIOutHandle) return;
	TVPCheckMIDIDelay();
	MIDIHDR hdr;
	hdr.lpData = (char*)data;
	hdr.dwFlags = 0;
	hdr.dwBufferLength = len;
	midiOutPrepareHeader(TVPMIDIOutHandle, &hdr, sizeof(MIDIHDR));
	midiOutLongMsg(TVPMIDIOutHandle, &hdr, sizeof(MIDIHDR));
	midiOutUnprepareHeader(TVPMIDIOutHandle, &hdr, sizeof(MIDIHDR));
}
//---------------------------------------------------------------------------
static void inline TVPMIDIOut(tjs_uint8 d1, tjs_uint8 d2, tjs_uint8 d3)
{
    if(!TVPMIDIOutHandle) return;

	TVPCheckMIDIDelay();
	midiOutShortMsg(TVPMIDIOutHandle, d1 + (d2<<8) + (d3<<16));
}
//---------------------------------------------------------------------------
static void TVPSetMIDIDelay(int ms)
{
	TVPMIDIDelayAlive = true;
	TVPMIDIDelayTick = TVPGetTickTime() + ms;
}
#endif
//---------------------------------------------------------------------------
void TVPMIDIOutData(const tjs_uint8 *data, int len)
{
#ifdef TVP_ENABLE_MIDI
    // output MIDI messages ( can be multiple messages more than one )
	TVPInitMIDIOut();
	if(!TVPMIDIOutHandle) return;
	if(len == 0 || !data) return;

	tTJSCriticalSectionHolder holder(TVPMIDIOutCS);

	tjs_uint8 prevevent = 0;

	for(int i = 0; i < len; )
	{
		tjs_uint8 ev;
		if(!(data[i] & 0x80))
		{
			// running status
			if(!prevevent)
				TVPThrowExceptionMessage(TVPMalformedMIDIMessage);
			ev = prevevent;
		}
		else
		{
			ev = data[i++];
		}

		if(ev < 0xf0)
		{
			tjs_uint8 b1, b2=0;
			switch(ev >> 4)
			{
			case 0x9:
			case 0x8:
			case 0xa:
			case 0xe:
			case 0xb:
				if(i >= len)
					TVPThrowExceptionMessage(TVPMalformedMIDIMessage);
				b1 = data[i++];
				if(i >= len)
					TVPThrowExceptionMessage(TVPMalformedMIDIMessage);
				b2 = data[i++];
				break;

			case 0xc:
			case 0xd:
				if(i >= len)
					TVPThrowExceptionMessage(TVPMalformedMIDIMessage);
				b1 = data[i++];
				break;
			}

			TVPMIDIOut(ev, b1, b2);
		}
		else
		{
			tjs_uint8 b1, b2;
			int cmd = ev&0xf;
			switch(cmd)
			{
			case 0x1: // f1
			case 0x3: // f3
				if(i >= len)
					TVPThrowExceptionMessage(TVPMalformedMIDIMessage);
				b1 = data[i++];
				TVPMIDIOut(ev, b1, 0);
				break;

			case 0x2: // f2
				if(i >= len)
					TVPThrowExceptionMessage(TVPMalformedMIDIMessage);
				b1 = data[i++];
				if(i >= len)
					TVPThrowExceptionMessage(TVPMalformedMIDIMessage);
				b2 = data[i++];
				TVPMIDIOut(ev, b1, b2);
				break;

			case 0xf: // ff
				if(i >= len)
					TVPThrowExceptionMessage(TVPMalformedMIDIMessage);
				b1 = data[i++];
				if(b1 == 0x00)
				{
					// 50ms wait
					TVPSetMIDIDelay(50);
				}
				break;

			case 0x7: // f7
				break;

			case 0x0: // f0
			  {
				// sysex
				int count;
				count = 1;
				int j;
				for(j = i; j < len; j++)
				{
					count++;
					if(data[j] == 0xf7) break;
				}
				if(j == len)
					TVPThrowExceptionMessage(TVPMalformedMIDIMessage); // EOX not found
				tjs_uint8 * sex = new tjs_uint8[count];
				memcpy(sex, data + i - 1, count);
				TVPMIDIOut(sex, count);
				delete [] sex;
				i += count -1;
				break;
			  }

			default:
				TVPMIDIOut(ev, 0, 0);
				break;

			}
		}

		prevevent = ev;
	}
#endif
}
//---------------------------------------------------------------------------




#ifdef TVP_ENABLE_MIDI
//---------------------------------------------------------------------------
// MIDI sequencer thread and buffer management
//---------------------------------------------------------------------------
class tTVPMIDIThread : public tTVPThread
{
public:
	tTVPMIDIThread() : tTVPThread(true)
	{
		SetPriority(ttpTimeCritical);
	};

	~tTVPMIDIThread()
	{
		Terminate();
		Resume();
		WaitFor();
	}

	void Execute(void) // "Execute" override
	{
		while(!GetTerminated())
		{
			// fire OnTimer to process MIDI events.

			tjs_int sleepcount = 0;
			tjs_int buffercount;

			{ // critical section protected area
				tTJSCriticalSectionHolder holder(TVPMIDIOutCS);

				buffercount = TVPMIDIBuffers.size();

				std::vector<tTJSNI_MIDISoundBuffer*>::iterator i;

				for(i = TVPMIDIBuffers.begin(); i != TVPMIDIBuffers.end(); i++)
				{
					if(!(*i)->OnTimer()) sleepcount++;
				}
			}

			if(sleepcount == buffercount) Suspend(); // all buffers are sleeping

			Sleep(5); // but 10ms is sufficient to play MIDIs I think ...
		}

	}

} static *TVPMIDIThread = NULL;
//---------------------------------------------------------------------------
static void TVPDeleteMIDIThread()
{
	if(TVPMIDIThread) delete TVPMIDIThread, TVPMIDIThread = NULL;
}
//---------------------------------------------------------------------------
static tTVPAtExit TVPDeleteMIDIThreadAtExit(TVP_ATEXIT_PRI_SHUTDOWN,
	TVPDeleteMIDIThread);
//---------------------------------------------------------------------------
static void TVPAddMIDIBuffer(tTJSNI_MIDISoundBuffer *buf)
{
	tTJSCriticalSectionHolder holder(TVPMIDIOutCS);

	TVPMIDIBuffers.push_back(buf);

	if(TVPMIDIBuffers.size() == 1)
	{
		// first buffer
		//TVPStartTickCount(); // in TickCount.cpp
		TVPInitMIDIOut();
		if(!TVPMIDIThread) TVPMIDIThread = new tTVPMIDIThread();
	}
}
//---------------------------------------------------------------------------
static void TVPRemoveMIDIBuffer(tTJSNI_MIDISoundBuffer *buf)
{
	tTJSCriticalSectionHolder holder(TVPMIDIOutCS);

	std::vector<tTJSNI_MIDISoundBuffer *>::iterator i;
	i = std::find(TVPMIDIBuffers.begin(), TVPMIDIBuffers.end(), buf);
	if(i != TVPMIDIBuffers.end()) TVPMIDIBuffers.erase(i);

	if(TVPMIDIBuffers.size() == 0)
	{
		// all buffers are removed
		TVPDeleteMIDIThread();
		TVPUninitMIDIOut();
	}
}
//---------------------------------------------------------------------------





//---------------------------------------------------------------------------
// SMF access utility functions
//---------------------------------------------------------------------------
static tjs_uint16 TVPGetSMFInt16(const tjs_uint8 *&p)
{
	tjs_uint16 r;
	*(((tjs_uint8*)&r)+1)= p[0];
	*(((tjs_uint8*)&r)+0)= p[1];
	p += 2;
	return r;
}
//---------------------------------------------------------------------------
static tjs_uint32 TVPGetSMFInt32(const tjs_uint8 *&p)
{
	tjs_uint32 r;
	*(((tjs_uint8*)&r)+3)= p[0];
	*(((tjs_uint8*)&r)+2)= p[1];
	*(((tjs_uint8*)&r)+1)= p[2];
	*(((tjs_uint8*)&r)+0)= p[3];
	p += 4;
	return r;
}
//---------------------------------------------------------------------------
static tjs_int64 TVPGetSMFValue(const tjs_uint8 *&p)
{
	tjs_int64 r;
	tjs_uint8 d;
	r=0;

	do
	{
		d = *p;
		p++;
		r = (r<<7) + (d&0x7f);

	} while(d & 0x80);

	return r;
}
//---------------------------------------------------------------------------




//---------------------------------------------------------------------------
// tTVPSMFTrack : SMF Track Class
//---------------------------------------------------------------------------
class tTVPSMFTrack
{
	tjs_uint8 *Data; // track data
	const tjs_uint8 *Position; // current position
	tjs_int TrackSize; // track data size
	tjs_int64 Count; // current counter time
	tjs_uint8 PrevEvent; // previous event
	bool Ended; // is ended?

	tTJSNI_MIDISoundBuffer *Owner;

public:
	tTVPSMFTrack(tTJSNI_MIDISoundBuffer *owner, tTJSBinaryStream *instream, tjs_int n);

	~tTVPSMFTrack();

	void Rewind(void);
	void ProcessEvent();

	bool GetEnded() const { return Ended; }
	const tjs_int64 & GetCount() const { return Count; }
};
//---------------------------------------------------------------------------
tTVPSMFTrack::tTVPSMFTrack(tTJSNI_MIDISoundBuffer *owner, tTJSBinaryStream *instream,
	tjs_int n)
{
	TrackSize = n;
	Data = new tjs_uint8[n];
	try
	{
		instream->ReadBuffer(Data, n);
	}
	catch(...)
	{
		delete [] Data;
		throw;
	}
	Owner = owner;

	Rewind();
}
//---------------------------------------------------------------------------
tTVPSMFTrack::~tTVPSMFTrack()
{
	delete [] Data;
}
//---------------------------------------------------------------------------
void tTVPSMFTrack::Rewind(void)
{
	Position = Data;
	Ended = false;
	PrevEvent = 0;
	Count= TVPGetSMFValue(Position);
}
//---------------------------------------------------------------------------
void tTVPSMFTrack::ProcessEvent()
{
	// process one track event

	// some reset message data
	static tjs_uint8 sysex_gsreset[]=
		{0xf0,0x41,0x10,0x42, 0x12,0x40,0x00,0x7f, 0x00,0x41,0xf7};
	static tjs_uint8 sysex_modeset[]=
		{0xf0,0x41,0x10,0x42, 0x12,0x00,0x00,0x7f}; // followed by initialize data
	static tjs_uint8 sysex_gmon[]=
		{0xf0,0x7e,0x7f,0x09, 0x01,0xf7};
	static tjs_uint8 sysex_xgon[]=
		{0xf0,0x43,0x10,0x4c, 0x00,0x00,0x7e,0x00, 0xf7};


	// check playing status
	if(Ended) return;

	tjs_uint8 ev;
	ev = *Position;
	if(ev & 0x80)
	{
		// not a running status
		Position++;
	}
	else
	{
		// a running status
		ev = PrevEvent;
	}

	if(ev < 0xf0)
	{
		tjs_uint8 b1, b2=0;
		bool svflag = false;
		bool send = true;
		switch(ev >> 4)
		{
		case 0x9:
			if(!Owner->UsingChannel[ev&0x0f])
			{
				// first chance to set volume
				TVPMIDIOut((tjs_uint8)(0xb0+(ev&0x0f)), 0x07,
					(tjs_uint8)(Owner->Volumes[ev&0x0f] * Owner->BufferVolume/127));
			}
			Owner->UsingChannel[ev&0x0f] = true; // note on
			if(*Position < 128)
			{
				// check "on" notes
				if(Position[1]!=0)
					Owner->UsingNote[ev&0x0f][*Position/32] |=
						(tjs_uint32)1<<(*Position%32);
				else
					Owner->UsingNote[ev&0x0f][*Position/32] &=
						~((tjs_uint32)1<<(*Position%32));
			}
			b1 = *(Position++);
			b2 = *(Position++);
			break;

		case 0x8:
			if(*Position < 128)
				Owner->UsingNote[ev&0x0f][*Position/32] &=
					~((tjs_uint32)1<<(*Position%32));
			// no "break" here
		case 0xa:
		case 0xe:
			b1 = *(Position++);
			b2 = *(Position++);
			break;

		case 0xc:
		case 0xd:
			b1 = *(Position++);
			break;

		case 0xb:  // control change
			b1 = *(Position++);
			b2 = *(Position++);
			if(b1 == 0x07) // channel volume
			{
				Owner->UsingChannel[ev&0x0f] = true;
				Owner->Volumes[ev&0x0f] = b2;
				b2=(tjs_uint8)(b2 * Owner->BufferVolume/127);
					// new volume
			}
			else if(b1 == 0x79) // reset all controllers
			{
				Owner->UsingChannel[ev&0x0f] = true;
				Owner->Volumes[ev&0x0f] = 100;
				svflag = true;
			}
			break;

		default:
			send = false;
		}

		if(send) TVPMIDIOut(ev,b1,b2);

		if(svflag)
		{
			// re-send volume
			TVPMIDIOut((tjs_uint8)(0xb0+(ev&0x0f)), 0x07,
				(tjs_uint8)(Owner->Volumes[ev&0x0f] * Owner->BufferVolume/127));
		}
	}
	else
	{
		int cmd = ev&0xf;
		if(cmd == 0x00)
		{
			tjs_uint32 dlen = TVPGetSMFValue(Position);
			tjs_uint8 *data = new tjs_uint8[dlen+1];
			data[0] = 0xf0;
			memcpy(data+1, Position, dlen);
			Position += dlen;

			TVPMIDIOut(data, dlen+1);

			if((dlen>=10 && !memcmp(data, sysex_gsreset, 11)) ||
			   (dlen>=7  && !memcmp(data, sysex_modeset, 8)) ||
			   (dlen>=5  && !memcmp(data, sysex_gmon, 6)) ||
			   (dlen>=8  && !memcmp(data, sysex_xgon, 9)) )
			{
				// these are MIDI module reset messages;
				// clear volume information

				// sleep while the MIDI module is resetting...
				TVPSetMIDIDelay(50);

				// re-send track volumes
				tjs_int i;
				for(i=0; i<16; i++)
				{
					Owner->Volumes[i] = 100;

					Owner->UsingChannel[i] = true;

					TVPMIDIOut((tjs_uint8)(0xb0+i),
						0x07,
						(tjs_uint8)(Owner->Volumes[i]*
						Owner->BufferVolume/127));
				}
			}

			delete [] data;
		}
		else if(cmd == 0x07)
		{
			tjs_uint32 dlen = TVPGetSMFValue(Position);
			tjs_uint8 *data = new tjs_uint8[dlen];
			memcpy(data, Position, dlen);
			Position += dlen;
			TVPMIDIOut(data, dlen);
			delete [] data;
		}
		else if(cmd == 0x0f)
		{
			// meta events
			tjs_uint8 meta=*(Position++);
			tjs_uint32 dlen=TVPGetSMFValue(Position);

			switch(meta)
			{
			case 0x2f: // track end
				Ended=true;
				return;

			case 0x51: // tempo
			  {
				tjs_uint32 tempo=0;
				while(dlen--)
				{
					tempo=(tempo<<8)+ *(Position++);
				}

				Owner->SetTempo(tempo);
					// note: this does not perform accurate tempo
					// changing; just a simple implementation
					// but enough.
				break;
			  }
				  
			default:
				Position += dlen; // others; discard
			}
		}
		else if(cmd == 0x01 || cmd == 0x03)
		{
			TVPMIDIOut(ev, *(Position++), 0);
		}
		else if(cmd == 2)
		{
			tjs_uint8 b1, b2;
			b1 = *(Position++);
			b2 = *(Position++);
			TVPMIDIOut(ev, b1, b2);
		}
		else
		{
			TVPMIDIOut(ev, 0, 0);
		}

	}

	PrevEvent = ev;

	if(Position-Data >= TrackSize) { Ended = true; return; }
	Count += TVPGetSMFValue(Position);
	if(Position-Data >= TrackSize) { Ended = true; return; }

	return;
}
//---------------------------------------------------------------------------
#endif





//---------------------------------------------------------------------------
// tTJSNI_MIDISoundBuffer
//---------------------------------------------------------------------------
tTJSNI_MIDISoundBuffer::tTJSNI_MIDISoundBuffer()
{
#ifdef TVP_ENABLE_MIDI
    Playing = false;
	Loaded = false;
	Volume =  100000;
	Volume2 = 100000;
	BufferVolume = 127;
	NextSetVolume = false;
	UtilWindow = NULL;
	for(tjs_int i = 0; i<16; i++)
	{
		UsingChannel[i] = false;
		UsingNote[i][0] = UsingNote[i][1] = UsingNote[i][2] = UsingNote[i][3] =0;
		Volumes[i] = 100;
	}
#endif
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD
tTJSNI_MIDISoundBuffer::Construct(tjs_int numparams, tTJSVariant **param,
		iTJSDispatch2 *tjs_obj)
{
	tjs_error hr = inherited::Construct(numparams, param, tjs_obj);
	if(TJS_FAILED(hr)) return hr;
#ifdef TVP_ENABLE_MIDI

#ifdef __BORLANDC__
	UtilWindow = AllocateHWnd(WndProc);
#else
	UtilWindow = AllocateHWnd( EVENT_FUNC1(tTJSNI_MIDISoundBuffer, WndProc) );
#endif

	TVPAddMIDIBuffer(this);
#endif

	return TJS_S_OK;
}
//---------------------------------------------------------------------------
void TJS_INTF_METHOD tTJSNI_MIDISoundBuffer::Invalidate()
{
	inherited::Invalidate();
#ifdef TVP_ENABLE_MIDI

	StopPlay();

	TVPRemoveMIDIBuffer(this);

	std::vector<tTVPSMFTrack*>::iterator i;
	for(i = Tracks.begin(); i != Tracks.end(); i++)
	{
		delete *i;
	}

	if(UtilWindow) DeallocateHWnd(UtilWindow);
#endif
}
#ifdef TVP_ENABLE_MIDI
//---------------------------------------------------------------------------
void tTJSNI_MIDISoundBuffer::WndProc(Messages::TMessage &Msg)
{
	// called by OS, caused by posting of status message from playing thread.
	if(Msg.Msg == WM_USER+1)
	{
		SetStatusAsync((tTVPSoundStatus)Msg.LParam);
		return;
	}
	Msg.Result =  DefWindowProc(UtilWindow, Msg.Msg, Msg.WParam, Msg.LParam);
}
//---------------------------------------------------------------------------
void tTJSNI_MIDISoundBuffer::AllNoteOff()
{
	tTJSCriticalSectionHolder holder(TVPMIDIOutCS);

	tjs_int i;

	for(i = 0; i<16; i++)
	{
		if(UsingChannel[i])
		{
			TVPMIDIOut((tjs_uint8)(0xb0+i),0x7B,0x00); // All Note Off
			TVPMIDIOut((tjs_uint8)(0xb0+i),0x40,0x00); // Sus. Off
		}
	}

	for(i = 0; i<16; i++)
	{
		if(UsingChannel[i])
		{
			int j;
			for(j=0; j<128; j++)
			{
				if(UsingNote[i][j/32] & (1<<(j%32)))
				{
					TVPMIDIOut((tjs_uint8)(0x90+i), (tjs_uint8)j, (tjs_uint8)0x00);
				}
			}
		}
	}

}
//---------------------------------------------------------------------------
void tTJSNI_MIDISoundBuffer::SetTempo(tjs_uint tempo)
{
	// set tempo count
	if(tempo == 0) tempo = 500000;

	TickCountDelta = ((__int64)(Division * 1000)<<16)/tempo;
}
//---------------------------------------------------------------------------
bool tTJSNI_MIDISoundBuffer::OnTimer()
{
	if(!Playing) return false;

	tjs_int p;
	__int64 curcount = TickCount >> 16;

	while(true)
	{
		p = 0;

		// find a track which has least tick count
		__int64 least_tick = -1;
		bool do_tick = false;
		std::vector<tTVPSMFTrack*>::iterator least;
		for(std::vector<tTVPSMFTrack*>::iterator i = Tracks.begin();
			i != Tracks.end(); i++)
		{
			if(!(*i)->GetEnded())
			{
				p++;
				__int64 tt = (*i)->GetCount();
				if(tt < curcount)
				{
					if(least_tick == -1 || tt < least_tick)
					{
						least_tick = tt;
						least = i;
						do_tick = true;
					}
				}
			}
		}

		if(!p) break;
		if(!do_tick) break;

		// process event
		__int64 curtick = (*least)->GetCount();
		do
		{
			(*least)->ProcessEvent();
		} while(!(*least)->GetEnded() && curtick == (*least)->GetCount());
	}

	if(p==0)
	{
		// all tracks are ended

		if(Looping)
		{
			StopPlay();
			Play();
			return true;
		}
		else
		{
			StopPlay();
			::PostMessage(UtilWindow, WM_USER+1, 0, ssStop);
		}
		return false;
	}

	if(NextSetVolume)
	{
		// set buffer volume

		NextSetVolume = false;

		tjs_int i;

		for(i=0; i<16; i++)
		{
			if(UsingChannel[i])
			{
				TVPMIDIOut((tjs_uint8)(0xb0+i), 0x07,
					(tjs_uint8)(Volumes[i]*BufferVolume/127));
			}
		}

	}

	tjs_uint64 curtick = TVPGetTickCount();
	tjs_uint64 d = curtick - LastTickTime;
	LastTickTime = curtick;

	if(d > 500) d = 0;
		// may be stopped by an unexpected event.
		// this may avoid unpleasant flushing of MIDI messages.

	Position += d;
	TickCount += TickCountDelta * d;

	return true;
}
//---------------------------------------------------------------------------
void tTJSNI_MIDISoundBuffer::Open(const ttstr &name)
{
	// open a stream
	tTVPStreamHolder stream(name);

	Stop();

	try
	{
		// check header
		tjs_uint8 buf[16];
		stream->ReadBuffer(buf, 4);
		if(memcmp(buf, "MThd", 4))
			TVPThrowExceptionMessage(TVPInvalidSMF, TJS_W("MThd not found"));

		tjs_uint32 headerlen;
		tjs_int format;

		stream->ReadBuffer(buf, 10);
		const tjs_uint8 *pb = buf;
		headerlen = TVPGetSMFInt32(pb);

		format = TVPGetSMFInt16(pb);
		if(format != 0 && format != 1)
			TVPThrowExceptionMessage(TVPInvalidSMF, TJS_W("Format must be 0 or 1"));


		// clear tracks
		{
			std::vector<tTVPSMFTrack*>::iterator i;
			for(i = Tracks.begin(); i != Tracks.end(); i++)
			{
				delete *i;
			}
			Tracks.clear();
		}


		// get some information
		tjs_int trackcount;
		trackcount = TVPGetSMFInt16(pb);

		Division = TVPGetSMFInt16(pb);
		if(Division < 0)
			TVPThrowExceptionMessage(TVPInvalidSMF, TJS_W("Invalid division"));

		// read each track
		stream->SetPosition(headerlen + 8);

		for(tjs_int i = 0; i<trackcount; i++)
		{
			stream->ReadBuffer(buf, 8);
			if(memcmp(buf, "MTrk", 4))
				TVPThrowExceptionMessage(TVPInvalidSMF, TJS_W("MTrk not found"));
			pb = buf + 4;
			tjs_uint32 len = TVPGetSMFInt32(pb);
			tTVPSMFTrack *trk = new tTVPSMFTrack(this, stream.Get(), len);
			Tracks.push_back(trk);
		}

		Position = 0;
		TickCount = 0;
		SetTempo(500000);

		for(tjs_int i = 0; i<16; i++)
		{
//			UsingChannel[i] = false;
			UsingNote[i][0] = UsingNote[i][1] = UsingNote[i][2] = UsingNote[i][3] =0;
			Volumes[i] = 100;
		}
	}
	catch(...)
	{
		Loaded = false;
		SetStatus(ssUnload);
		throw;
	}

	Loaded = true;
	SetStatus(ssStop);
}
//---------------------------------------------------------------------------
bool tTJSNI_MIDISoundBuffer::StopPlay()
{
	if(Playing)
	{
		tTJSCriticalSectionHolder holder(TVPMIDIOutCS);

		Playing = false;
		AllNoteOff();
		Position = 0;
		TickCount = 0;
		SetTempo(500000);

		std::vector<tTVPSMFTrack*>::iterator i;
		for(i = Tracks.begin(); i != Tracks.end(); i++)
		{
			(*i)->Rewind();
		}
		return true;
	}
	return false;
}
//---------------------------------------------------------------------------
bool tTJSNI_MIDISoundBuffer::StartPlay()
{
	if(!Loaded) return false;
	if(Playing) return false;

	tTJSCriticalSectionHolder holder(TVPMIDIOutCS);

	if(TVPMIDIThread) TVPMIDIThread->Resume();

	tjs_int i;
	for(i = 0; i<16; i++)
	{
		if(UsingChannel[i])
		{
			TVPMIDIOut((tjs_uint8)(0xb0+i), 0x79, 0x00); // Reset All Controler
			TVPMIDIOut((tjs_uint8)(0xb0+i), 0x40, 0x00); // Sus. Off
		}
	}

	for(i = 0; i<16; i++)
	{
		UsingChannel[i]=false;
		UsingNote[i][0] = UsingNote[i][1] = UsingNote[i][2] = UsingNote[i][3] =0;
		Volumes[i]=100;
	}

	LastTickTime = TVPGetTickCount();
	Playing = true;

	return true;
}
//---------------------------------------------------------------------------
void tTJSNI_MIDISoundBuffer::Play()
{
	if(StartPlay()) SetStatus(ssPlay);
}
//---------------------------------------------------------------------------
void tTJSNI_MIDISoundBuffer::Stop()
{
	if(StopPlay()) SetStatus(ssStop);
}
//---------------------------------------------------------------------------
void tTJSNI_MIDISoundBuffer::SetBufferVolume(tjs_int nv)
{
	if(BufferVolume != nv)
	{
		BufferVolume = nv;
		if(Playing)
		{
			NextSetVolume = true;
		}
		else
		{
			tTJSCriticalSectionHolder holder(TVPMIDIOutCS);
			tjs_int i;

			for(i=0; i<16; i++)
			{
				if(UsingChannel[i])
				{
					TVPMIDIOut((tjs_uint8)(0xb0+i), 0x07,
						(tjs_uint8)(Volumes[i]*BufferVolume/127));
				}
			}
		}
	}
}
//---------------------------------------------------------------------------
void tTJSNI_MIDISoundBuffer::SetVolume(tjs_int v)
{
	// set buffer volume

	if(v<0) v=0;
	if(v>100000) v=100000;
	Volume = v;

	v = (Volume / 10) * (Volume2 / 10) / 1000;

	tjs_int nv = v * 127 / 100000;
	if(nv<0) nv = 0;
	if(nv>127) nv = 127;

	SetBufferVolume(nv);
}
//---------------------------------------------------------------------------
void tTJSNI_MIDISoundBuffer::SetVolume2(tjs_int v)
{
	// set alternative buffer volume

	if(v<0) v=0;
	if(v>100000) v=100000;
	Volume2 = v;

	v = (Volume / 10) * (Volume2 / 10) / 1000;

	tjs_int nv = v * 127 / 100000;
	if(nv<0) nv = 0;
	if(nv>127) nv = 127;

	SetBufferVolume(nv);
}
//---------------------------------------------------------------------------
#endif



//---------------------------------------------------------------------------
// tTJSNC_MIDISoundBuffer
//---------------------------------------------------------------------------
tTJSNativeInstance *tTJSNC_MIDISoundBuffer::CreateNativeInstance()
{
	return new tTJSNI_MIDISoundBuffer();
}
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// TVPCreateNativeClass_MIDISoundBuffer
//---------------------------------------------------------------------------
tTJSNativeClass * TVPCreateNativeClass_MIDISoundBuffer()
{
	tTJSNativeClass *cls = new tTJSNC_MIDISoundBuffer();
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/midiOut)
{
	if(numparams < 1) return TJS_E_BADPARAMCOUNT;

#ifdef TVP_ENABLE_MIDI
	tTJSVariantOctet *octet = param[0]->AsOctetNoAddRef();

	if(octet)
		TVPMIDIOutData(octet->GetData(), octet->GetLength());
#endif

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL_OUTER(/*object to register*/cls,
	/*func. name*/midiOut)
//----------------------------------------------------------------------
	return cls;
}
//---------------------------------------------------------------------------


