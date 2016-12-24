//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Wave Loop Manager
//---------------------------------------------------------------------------
/*
	This module will be shared between TVP2 and
	Loop Tuner 2 (a GUI loop-point editor)
*/

#include "tjsCommHead.h"

#include <algorithm>
#include <string.h>
#include "tjsTypes.h"
#include "WaveLoopManager.h"
#include "CharacterSet.h"

#ifdef TVP_IN_LOOP_TUNER
	#include "WaveReader.h"
#else
	#include "WaveIntf.h"
#endif


#ifdef __BORLANDC__
	#define strcasecmp strcmpi
	#define strncasecmp strncmpi
#endif

#ifdef _MSC_VER
	#define strcasecmp _stricmp
	#define strncasecmp _strnicmp
#endif

//---------------------------------------------------------------------------
const int TVPWaveLoopLinkGiveUpCount = 10;
	// This is for preventing infinite loop caused by recursive links.
	// If the decoding point does not change when the loop manager follows the
	// links, after 'TVPWaveLoopLinkGiveUpCount' times the loop manager
	// will give up the decoding.
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// Wave Sample Types
//---------------------------------------------------------------------------
#ifndef TJS_HOST_IS_BIG_ENDIAN
	#define TJS_HOST_IS_BIG_ENDIAN 0
#endif

#ifdef __WIN32__
	// for assembler compatibility
	#pragma pack(push,1)
#endif
struct tTVPPCM8
{
	tjs_uint8 value;
	tTVPPCM8(tjs_int v) { value = (tjs_uint8)(v + 0x80); }
	void operator = (tjs_int v) { value = (tjs_uint8)(v + 0x80); }
	operator tjs_int () const { return (tjs_int)value - 0x80; }
};
struct tTVPPCM24
{
	tjs_uint8 value[3];
	tTVPPCM24(tjs_int v)
	{
		operator = (v);
	}
	void operator =(tjs_int v)
	{
#if TJS_HOST_IS_BIG_ENDIAN
		value[0] = (v & 0xff0000) >> 16;
		value[1] = (v & 0x00ff00) >> 8;
		value[2] = (v & 0x0000ff);
#else
		value[0] = (v & 0x0000ff);
		value[1] = (v & 0x00ff00) >> 8;
		value[2] = (v & 0xff0000) >> 16;
#endif
	}
	operator tjs_int () const
	{
		tjs_int t;
#if TJS_HOST_IS_BIG_ENDIAN
		t = ((tjs_int)value[0] << 16) + ((tjs_int)value[1] << 8) + ((tjs_int)value[2]);
#else
		t = ((tjs_int)value[2] << 16) + ((tjs_int)value[1] << 8) + ((tjs_int)value[0]);
#endif
		t |= -(t&0x800000); // extend sign
		return t;
	}
};
#ifdef __WIN32__
	#pragma pack(pop)
#endif
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// Crossfade Template function
//---------------------------------------------------------------------------
template <typename T>
static void TVPCrossFadeIntegerBlend(void *dest, void *src1, void *src2,
	tjs_int ratiostart, tjs_int ratioend,
	tjs_int samples, tjs_int channels)
{
	tjs_uint blend_step = (tjs_int)(
		(
			(ratioend - ratiostart) * ((tjs_int64)1<<32) / 100
		) / samples);
	const T *s1 = (const T *)src1;
	const T *s2 = (const T *)src2;
	T *out = (T *)dest;
	tjs_uint ratio = (tjs_int)(ratiostart * ((tjs_int64)1<<32) / 100);
	for(tjs_int i = 0; i < samples; i++)
	{
		for(tjs_int j = channels - 1; j >= 0; j--)
		{
			tjs_int si1 = (tjs_int)*s1;
			tjs_int si2 = (tjs_int)*s2;
			tjs_int o = (tjs_int) (
						(((tjs_int64)si2 * (tjs_uint64)ratio) >> 32) +
						(((tjs_int64)si1 * (0x100000000ull - (tjs_uint64)ratio) ) >> 32) );
			*out = o;
			s1 ++;
			s2 ++;
			out ++;
		}
		ratio += blend_step;
	}
}
//---------------------------------------------------------------------------





//---------------------------------------------------------------------------
// tTVPWaveLoopManager
//---------------------------------------------------------------------------
tTVPWaveLoopManager::tTVPWaveLoopManager()
{
	Position = 0;
	IsLinksSorted = false;
	IsLabelsSorted = false;
	CrossFadeSamples = NULL;
	CrossFadeLen = 0;
	CrossFadePosition = 0;
	Decoder = NULL;
	IgnoreLinks = false;
	Looping = false;

	Format = new tTVPWaveFormat;
	memset(Format, 0, sizeof(*Format));

	ClearFlags();
	FlagsModifiedByLabelExpression = false;
}
//---------------------------------------------------------------------------
tTVPWaveLoopManager::~tTVPWaveLoopManager()
{
	ClearCrossFadeInformation();
	delete Format;
}
//---------------------------------------------------------------------------
void tTVPWaveLoopManager::SetDecoder(tTVPWaveDecoder * decoder)
{
	// set decoder and compute ShortCrossFadeHalfSamples
	Decoder = decoder;
	if(decoder)
		decoder->GetFormat(*Format);
	else
		memset(Format, 0, sizeof(*Format));
	ShortCrossFadeHalfSamples =
		Format->SamplesPerSec * TVP_WL_SMOOTH_TIME_HALF / 1000;
}
//---------------------------------------------------------------------------
int tTVPWaveLoopManager::GetFlag(tjs_int index)
{
	volatile tTJSCriticalSectionHolder CS(FlagsCS);
	return Flags[index];
}
//---------------------------------------------------------------------------
void tTVPWaveLoopManager::CopyFlags(tjs_int *dest)
{
	volatile tTJSCriticalSectionHolder CS(FlagsCS);
	// copy flags into dest, and clear FlagsModifiedByLabelExpression
	memcpy(dest, Flags, sizeof(Flags));
	FlagsModifiedByLabelExpression = false;
}
//---------------------------------------------------------------------------
bool tTVPWaveLoopManager::GetFlagsModifiedByLabelExpression()
{
	volatile tTJSCriticalSectionHolder CS(FlagsCS);
	return FlagsModifiedByLabelExpression;
}
//---------------------------------------------------------------------------
void tTVPWaveLoopManager::SetFlag(tjs_int index, tjs_int f)
{
	volatile tTJSCriticalSectionHolder CS(FlagsCS);
	if(f < 0) f = 0;
	if(f > TVP_WL_MAX_FLAG_VALUE) f = TVP_WL_MAX_FLAG_VALUE;
	Flags[index] = f;
}
//---------------------------------------------------------------------------
void tTVPWaveLoopManager::ClearFlags()
{
	volatile tTJSCriticalSectionHolder CS(FlagsCS);
	for(tjs_int i = 0; i < TVP_WL_MAX_FLAGS; i++) Flags[i] = 0;
}
//---------------------------------------------------------------------------
void tTVPWaveLoopManager::ClearLinksAndLabels()
{
	// clear links and labels
	volatile tTJSCriticalSectionHolder CS(FlagsCS);
	Labels.clear();
	Links.clear();
	IsLinksSorted = false;
	IsLabelsSorted = false;
}
//---------------------------------------------------------------------------
const std::vector<tTVPWaveLoopLink> & tTVPWaveLoopManager::GetLinks() const
{
	volatile tTJSCriticalSectionHolder
		CS(const_cast<tTVPWaveLoopManager*>(this)->FlagsCS);
	return Links;
}
//---------------------------------------------------------------------------
const std::vector<tTVPWaveLabel> & tTVPWaveLoopManager::GetLabels() const
{
	volatile tTJSCriticalSectionHolder
		CS(const_cast<tTVPWaveLoopManager*>(this)->FlagsCS);
	return Labels;
}
//---------------------------------------------------------------------------
void tTVPWaveLoopManager::SetLinks(const std::vector<tTVPWaveLoopLink> & links)
{
	volatile tTJSCriticalSectionHolder CS(FlagsCS);
	Links = links;
	IsLinksSorted = false;
}
//---------------------------------------------------------------------------
void tTVPWaveLoopManager::SetLabels(const std::vector<tTVPWaveLabel> & labels)
{
	volatile tTJSCriticalSectionHolder CS(FlagsCS);
	Labels = labels;
	IsLabelsSorted = false;
}
//---------------------------------------------------------------------------
bool tTVPWaveLoopManager::GetIgnoreLinks() const
{
	volatile tTJSCriticalSectionHolder
		CS(const_cast<tTVPWaveLoopManager*>(this)->DataCS);
	return IgnoreLinks;
}
//---------------------------------------------------------------------------
void tTVPWaveLoopManager::SetIgnoreLinks(bool b)
{
	volatile tTJSCriticalSectionHolder CS(DataCS);
	IgnoreLinks = b;
}
//---------------------------------------------------------------------------
tjs_int64 tTVPWaveLoopManager::GetPosition() const
{
	// we cannot assume that the 64bit data access is truely atomic on 32bit machines.
	volatile tTJSCriticalSectionHolder
		CS(const_cast<tTVPWaveLoopManager*>(this)->FlagsCS);
	return Position;
}
//---------------------------------------------------------------------------
void tTVPWaveLoopManager::SetPosition(tjs_int64 pos)
{
	volatile tTJSCriticalSectionHolder CS(DataCS);
	Position = pos;
	ClearCrossFadeInformation();
	Decoder->SetPosition(pos);
}
//---------------------------------------------------------------------------
void tTVPWaveLoopManager::Decode(void *dest, tjs_uint samples, tjs_uint &written,
		tTVPWaveSegmentQueue & segments)
{
	// decode from current position
	// note that segements will not be cleared
	volatile tTJSCriticalSectionHolder CS(DataCS);

//	segments.clear();  ///!!! NOT CLEARED !!!!
	written = 0;
	tjs_uint8 *d = (tjs_uint8*)dest;

	tjs_int give_up_count = 0;

	std::deque<tTVPWaveLabel> labels;

	while(written != samples/* && Position < Format->TotalSamples*/)
	{
		// clear labels
		labels.clear();

		// decide next operation
		tjs_int64 next_event_pos;
		bool next_not_found = false;
		tjs_int before_count;

		// check nearest link
		tTVPWaveLoopLink link;
		if(!IgnoreLinks && GetNearestEvent(Position, link, false))
		{
			// nearest event found ...
			if(link.From == Position)
			{
				// do jump
				give_up_count ++;
				if(give_up_count >= TVPWaveLoopLinkGiveUpCount)
					break; // give up decoding

				Position = link.To;
				if(!CrossFadeSamples)
					Decoder->SetPosition(Position);
				continue;
			}

			if(link.Smooth)
			{
				// the nearest event is a smooth link
				// bofore_count is sample count before 50%-50% blend
				// after_count  is sample count after  50%-50% blend
				before_count = ShortCrossFadeHalfSamples;
				// adjust before count
				if(link.From - before_count < 0)
					before_count = (tjs_int)link.From;
				if(link.To - before_count < 0)
					before_count = (tjs_int)link.To;
				if(link.From - before_count > Position)
				{
					// Starting crossfade is the nearest next event,
					// but some samples must be decoded before the event comes.
					next_event_pos = link.From - before_count;
				}
				else if(!CrossFadeSamples)
				{
					// just position to start crossfade
					// or crossfade must already start
					next_event_pos = link.From;
					// adjust before_count
					before_count = static_cast<tjs_int>(link.From - Position);
					// adjust after count
					tjs_int after_count = ShortCrossFadeHalfSamples;
					if(Format->TotalSamples - link.From < after_count)
						after_count =
							(tjs_int)(Format->TotalSamples - link.From);
					if(Format->TotalSamples - link.To < after_count)
						after_count =
							(tjs_int)(Format->TotalSamples - link.To);
					tTVPWaveLoopLink over_to_link;
					if(GetNearestEvent(link.To, over_to_link, true))
					{
						if(over_to_link.From - link.To < after_count)
							after_count =
								(tjs_int)(over_to_link.From - link.To);
					}
					// prepare crossfade
					// allocate memory
					tjs_uint8 *src1 = NULL;
					tjs_uint8 *src2 = NULL;
					try
					{
						tjs_int alloc_size =
							(before_count + after_count) * 
								Format->BytesPerSample * Format->Channels;
						CrossFadeSamples = new tjs_uint8[alloc_size];
						src1 = new tjs_uint8[alloc_size];
						src2 = new tjs_uint8[alloc_size];
					}
					catch(...)
					{
						// memory allocation failed. perform normal link.
						if(CrossFadeSamples)
							delete [] CrossFadeSamples,
								CrossFadeSamples = NULL;
						if(src1) delete [] src1;
						if(src2) delete [] src2;
						next_event_pos = link.From;
					}
					if(CrossFadeSamples)
					{
						// decode samples
						tjs_uint decoded1 = 0, decoded2 = 0;

						Decoder->Render((void*)src1,
							before_count + after_count, decoded1);

						Decoder->SetPosition(
							link.To - before_count);

						Decoder->Render((void*)src2,
							before_count + after_count, decoded2);

						// perform crossfade
						tjs_int after_offset =
							before_count * Format->BytesPerSample * Format->Channels;
						DoCrossFade(CrossFadeSamples,
							src1, src2, before_count, 0, 50);
						DoCrossFade(CrossFadeSamples + after_offset,
							src1 + after_offset, src2 + after_offset,
								after_count, 50, 100);
						delete [] src1;
						delete [] src2;
						// reset CrossFadePosition and CrossFadeLen
						CrossFadePosition = 0;
						CrossFadeLen = before_count + after_count;
					}
				}
				else
				{
					next_event_pos = link.From;
				}
			}
			else
			{
				// normal jump
				next_event_pos = link.From;
			}
		}
		else
		{
			// event not found
			next_not_found = true;
		}

		tjs_int one_unit;

		if(next_not_found || next_event_pos - Position > (samples - written))
			one_unit = samples - written;
		else
			one_unit = (tjs_int) (next_event_pos - Position);

		if(CrossFadeSamples)
		{
			if(one_unit > CrossFadeLen - CrossFadePosition)
				one_unit = CrossFadeLen - CrossFadePosition;
		}

		if(one_unit > 0) give_up_count = 0; // reset give up count

		// evaluate each label
		GetLabelAt(Position, Position + one_unit, labels);
		for(std::deque<tTVPWaveLabel>::iterator i = labels.begin();
			i != labels.end(); i++)
		{
			if(i->Name.c_str()[0] == ':')
			{
				// for each label
				EvalLabelExpression(i->Name);
			}
		}

		// calculate each label offset
		for(std::deque<tTVPWaveLabel>::iterator i = labels.begin();
			i != labels.end(); i++)
			i->Offset = (tjs_int)(i->Position - Position) + written;

		// enqueue labels
		segments.Enqueue(labels);

		// enqueue segment
		segments.Enqueue(tTVPWaveSegment(Position, one_unit));

		// decode or copy
		if(!CrossFadeSamples)
		{
			// not crossfade
			// decode direct into destination buffer
			tjs_uint decoded;
			Decoder->Render((void *)d, one_unit, decoded);
			Position += decoded;
			written += decoded;
			if(decoded != (tjs_uint)one_unit)
			{
				// must be the end of the decode
				if(!Looping) break; // end decoding
				// rewind and continue
				if(Position == 0)
				{
					// already rewinded; must be an error
					break;
				}
				Position = 0;
				Decoder->SetPosition(0);
			}
			d += decoded * Format->BytesPerSample * Format->Channels;
		}
		else
		{
			// in cross fade
			// copy prepared samples
			memcpy((void *)d,
				CrossFadeSamples +
					CrossFadePosition * Format->BytesPerSample * Format->Channels,
				one_unit * Format->BytesPerSample * Format->Channels);
			CrossFadePosition += one_unit;
			Position += one_unit;
			written += one_unit;
			d += one_unit * Format->BytesPerSample * Format->Channels;
			if(CrossFadePosition == CrossFadeLen)
			{
				// crossfade has finished
				ClearCrossFadeInformation();
			}
		}
	}	// while 
}
//---------------------------------------------------------------------------
bool tTVPWaveLoopManager::GetNearestEvent(tjs_int64 current,
		tTVPWaveLoopLink & link, bool ignore_conditions)
{
	// search nearest event in future, from current.
	// this checks conditions unless ignore_conditions is true.
	volatile tTJSCriticalSectionHolder CS(FlagsCS);

	if(Links.size() == 0) return false; // there are no event

	if(!IsLinksSorted)
	{
		std::sort(Links.begin(), Links.end());
		IsLinksSorted = true;
	}

	// search nearest next event using binary search
	tjs_int s = 0, e = (tjs_int)Links.size();
	while(e - s > 1)
	{
		tjs_int m = (s+e)/2;
		if(Links[m].From <= current)
			s = m;
		else
			e = m;
	}

	if(s < (int)Links.size()-1 && Links[s].From < current) s++;

	if((tjs_uint)s >= Links.size() || Links[s].From < current)
	{
		// no links available
		return false;
	}

	// rewind while the link 'from' is the same
	tjs_int64 from = Links[s].From;
	while(true)
	{
		if(s >= 1 && Links[s-1].From == from)
			s--;
		else
			break;
	}

	// check conditions
	if(!ignore_conditions)
	{
		do
		{
			// check condition
			if(Links[s].CondVar != -1)
			{
				bool match = false;
				switch(Links[s].Condition)
				{
				case llcNone:
					match = true; break;
				case llcEqual:
					if(Links[s].RefValue == Flags[Links[s].CondVar]) match = true;
					break;
				case llcNotEqual:
					if(Links[s].RefValue != Flags[Links[s].CondVar]) match = true;
					break;
				case llcGreater:
					if(Links[s].RefValue <  Flags[Links[s].CondVar]) match = true;
					break;
				case llcGreaterOrEqual:
					if(Links[s].RefValue <= Flags[Links[s].CondVar]) match = true;
					break;
				case llcLesser:
					if(Links[s].RefValue >  Flags[Links[s].CondVar]) match = true;
					break;
				case llcLesserOrEqual:
					if(Links[s].RefValue >= Flags[Links[s].CondVar]) match = true;
					break;
				default:
					match = false;
				}
				if(match) break; // condition matched
			}
			else
			{
				break;
			}
			s++;
		} while((tjs_uint)s < Links.size());

		if((tjs_uint)s >= Links.size() || Links[s].From < current)
		{
			// no links available
			return false;
		}
	}

	link = Links[s];

	return true;
}
//---------------------------------------------------------------------------
void tTVPWaveLoopManager::GetLabelAt(tjs_int64 from, tjs_int64 to,
		std::deque<tTVPWaveLabel> & labels)
{
	volatile tTJSCriticalSectionHolder CS(FlagsCS);

	if(Labels.size() == 0) return; // no labels found
	if(!IsLabelsSorted)
	{
		std::sort(Labels.begin(), Labels.end());
		IsLabelsSorted = true;
	}

	// search nearest label using binary search
	tjs_int s = 0, e = (tjs_int)Labels.size();
	while(e - s > 1)
	{
		tjs_int m = (s+e)/2;
		if(Labels[m].Position <= from)
			s = m;
		else
			e = m;
	}

	if(s < (int)Labels.size()-1 && Labels[s].Position < from) s++;

	if((tjs_uint)s >= Labels.size() || Labels[s].Position < from)
	{
		// no labels available
		return;
	}

	// rewind while the label position is the same
	tjs_int64 pos = Labels[s].Position;
	while(true)
	{
		if(s >= 1 && Labels[s-1].Position == pos)
			s--;
		else
			break;
	}

	// search labels
	for(; s < (int)Labels.size(); s++)
	{
		if(Labels[s].Position >= from && Labels[s].Position < to)
			labels.emplace_back(Labels[s]);
		else
			break;
	}
}
//---------------------------------------------------------------------------
void tTVPWaveLoopManager::DoCrossFade(void *dest, void *src1,
	void *src2, tjs_int samples, tjs_int ratiostart, tjs_int ratioend)
{
	// do on-memory wave crossfade
	// using src1 (fading out) and src2 (fading in).
	if(samples == 0) return; // nothing to do

	if(Format->IsFloat)
	{
		float blend_step =
			(float)((ratioend - ratiostart) / 100.0 / samples);
		const float *s1 = (const float *)src1;
		const float *s2 = (const float *)src2;
		float *out = (float *)dest;
		float ratio = static_cast<float>(ratiostart / 100.0);
		for(tjs_int i = 0; i < samples; i++)
		{
			for(tjs_int j = Format->Channels - 1; j >= 0; j--)
			{
				*out = *s1 + (*s2 - *s1) * ratio;
				s1 ++;
				s2 ++;
				out ++;
			}
			ratio += blend_step;
		}
	}
	else
	{
		if(Format->BytesPerSample == 1)
		{
			TVPCrossFadeIntegerBlend<tTVPPCM8>(dest, src1, src2,
				ratiostart, ratioend, samples, Format->Channels);
		}
		else if(Format->BytesPerSample == 2)
		{
			TVPCrossFadeIntegerBlend<tjs_int16>(dest, src1, src2,
				ratiostart, ratioend, samples, Format->Channels);
		}
		else if(Format->BytesPerSample == 3)
		{
			TVPCrossFadeIntegerBlend<tTVPPCM24>(dest, src1, src2,
				ratiostart, ratioend, samples, Format->Channels);
		}
		else if(Format->BytesPerSample == 4)
		{
			TVPCrossFadeIntegerBlend<tjs_int32>(dest, src1, src2,
				ratiostart, ratioend, samples, Format->Channels);
		}
	}
}
//---------------------------------------------------------------------------
void tTVPWaveLoopManager::ClearCrossFadeInformation()
{
	if(CrossFadeSamples) delete [] CrossFadeSamples, CrossFadeSamples = NULL;
}
//---------------------------------------------------------------------------
bool tTVPWaveLoopManager::GetLabelExpression(const tTVPLabelStringType &label,
	tTVPWaveLoopManager::tExpressionToken * ope,
	tjs_int *lv,
	tjs_int *rv, bool *is_rv_indirect)
{
	const tTVPLabelCharType * p = label.c_str();
	tExpressionToken token;
	tExpressionToken operation;
	tjs_int value  = 0;
	tjs_int lvalue = 0;
	tjs_int rvalue = 0;
	bool rv_indirect = false;

	if(*p != ':') return false; // not expression
	p++;

	token = GetExpressionToken(p, &value);
	if(token != etLBracket) return false; // lvalue must form of '[' integer ']'
	token = GetExpressionToken(p, &value);
	if(token != etInteger) return false; // lvalue must form of '[' integer ']'
	lvalue = value;
	if(lvalue < 0 || lvalue >= TVP_WL_MAX_FLAGS) return false; // out of the range
	token = GetExpressionToken(p, &value);
	if(token != etRBracket) return false; // lvalue must form of '[' integer ']'

	token = GetExpressionToken(p, &value);
	switch(token)
	{
	case etEqual:
	case etPlusEqual:  case etIncrement:
	case etMinusEqual: case etDecrement:
		break;
	default:
		return false; // unknown operation
	}
	operation = token;

	token = GetExpressionToken(p, &value);
	if(token == etLBracket)
	{
		// indirect value
		token = GetExpressionToken(p, &value);
		if(token != etInteger) return false; // rvalue does not have form of '[' integer ']'
		rvalue = value;
		if(rvalue < 0 || rvalue >= TVP_WL_MAX_FLAGS) return false; // out of the range
		token = GetExpressionToken(p, &value);
		if(token != etRBracket) return false; // rvalue does not have form of '[' integer ']'
		rv_indirect = true;
	}
	else if(token == etInteger)
	{
		// direct value
		rv_indirect = false;
		rvalue = value;
	}
	else if(token == etEOE)
	{
		if(!(operation == etIncrement || operation == etDecrement))
			return false; // increment or decrement cannot have operand
	}
	else
	{
		return false; // syntax error
	}

	token = GetExpressionToken(p, &value);
	if(token != etEOE) return false; // excess characters

	if(ope) *ope = operation;
	if(lv)  *lv = lvalue;
	if(rv)  *rv = rvalue;
	if(is_rv_indirect) * is_rv_indirect = rv_indirect;

	return true;
}
//---------------------------------------------------------------------------
bool tTVPWaveLoopManager::EvalLabelExpression(const tTVPLabelStringType &label)
{
	// eval expression specified by 'label'
	// commit the result when 'commit' is true.
	// returns whether the label syntax is correct.
	volatile tTJSCriticalSectionHolder CS(FlagsCS);

	tExpressionToken operation;
	tjs_int lvalue;
	tjs_int rvalue;
	bool is_rv_indirect;

	if(!GetLabelExpression(label, &operation, &lvalue, &rvalue, &is_rv_indirect)) return false;

	if(is_rv_indirect) rvalue = Flags[rvalue];

	switch(operation)
	{
	case etEqual:
		FlagsModifiedByLabelExpression = true;
		Flags[lvalue] = rvalue;
		break;
	case etPlusEqual:
		FlagsModifiedByLabelExpression = true;
		Flags[lvalue] += rvalue;
		break;
	case etMinusEqual:
		FlagsModifiedByLabelExpression = true;
		Flags[lvalue] -= rvalue;
		break;
	case etIncrement:
		FlagsModifiedByLabelExpression = true;
		Flags[lvalue] ++;
		break;
	case etDecrement:
		FlagsModifiedByLabelExpression = true;
		Flags[lvalue] --;
		break;
	default:
		break;
	}

	if(Flags[lvalue] < 0) Flags[lvalue] = 0;
	if(Flags[lvalue] > TVP_WL_MAX_FLAG_VALUE) Flags[lvalue] = TVP_WL_MAX_FLAG_VALUE;

	return true;
}
//---------------------------------------------------------------------------
tTVPWaveLoopManager::tExpressionToken
	tTVPWaveLoopManager::GetExpressionToken(const tTVPLabelCharType *  & p, tjs_int * value)
{
	// get token at pointer 'p'

	while(*p && *p <= 0x20) p++; // skip spaces
	if(!*p) return etEOE;

	switch(*p)
	{
	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
		// numbers
		if(value) GetLabelCharInt(p, *value);
		while(*p && *p >= '0' && *p <= '9') p++;
		return etInteger;

	case '[':
		p++;
		return etLBracket;
	case ']':
		p++;
		return etRBracket;

	case '=':
		p++;
		return etEqual;

	case '+':
		p++;
		if(*p == '=') { p++; return etPlusEqual; }
		if(*p == '+') { p++; return etIncrement; }
		return etPlus;
	case '-':
		p++;
		if(*p == '=') { p++; return etMinusEqual; }
		if(*p == '-') { p++; return etDecrement; }
		return etPlus;

	default:
		;
	}

	p++;
	return etUnknown;
}
//---------------------------------------------------------------------------
bool tTVPWaveLoopManager::GetLabelCharInt(const tTVPLabelCharType *s, tjs_int &v)
{
	// convert string to integer
	tjs_int r = 0;
	bool sign = false;
	while(*s && *s <= 0x20) s++; // skip spaces
	if(!*s) return false;
	if(*s == '-')
	{
		sign = true;
		s++;
		while(*s && *s <= 0x20) s++; // skip spaces
		if(!*s) return false;
	}

	while(*s >= '0' && *s <= '9')
	{
		r *= 10;
		r += *s - '0';
		s++;
	}
	if(sign) r = -r;
	v = r;
	return true;
}
//---------------------------------------------------------------------------
bool tTVPWaveLoopManager::GetInt(char *s, tjs_int &v)
{
	// convert string to integer
	tjs_int r = 0;
	bool sign = false;
	while(*s && *s <= 0x20) s++; // skip spaces
	if(!*s) return false;
	if(*s == '-')
	{
		sign = true;
		s++;
		while(*s && *s <= 0x20) s++; // skip spaces
		if(!*s) return false;
	}

	while(*s >= '0' && *s <= '9')
	{
		r *= 10;
		r += *s - '0';
		s++;
	}
	if(sign) r = -r;
	v = r;
	return true;
}
//---------------------------------------------------------------------------
bool tTVPWaveLoopManager::GetInt64(char *s, tjs_int64 &v)
{
	// convert string to integer
	tjs_int64 r = 0;
	bool sign = false;
	while(*s && *s <= 0x20) s++; // skip spaces
	if(!*s) return false;
	if(*s == '-')
	{
		sign = true;
		s++;
		while(*s && *s <= 0x20) s++; // skip spaces
		if(!*s) return false;
	}

	while(*s >= '0' && *s <= '9')
	{
		r *= 10;
		r += *s - '0';
		s++;
	}
	if(sign) r = -r;
	v = r;
	return true;
}
//---------------------------------------------------------------------------
bool tTVPWaveLoopManager::GetBool(char *s, bool &v)
{
	// convert string to boolean
	if(!strcasecmp(s, "True"))		{	v = true;	return true;	}
	if(!strcasecmp(s, "False"))		{	v = false;	return true;	}
	if(!strcasecmp(s, "Yes"))		{	v = true;	return true;	}
	if(!strcasecmp(s, "No"))		{	v = false;	return true;	}
	return false;
}
//---------------------------------------------------------------------------
bool tTVPWaveLoopManager::GetCondition(char *s, tTVPWaveLoopLinkCondition &v)
{
	// get condition value
	if(!strcasecmp(s, "no"))		{ v = llcNone;				return true;	}
	if(!strcasecmp(s, "eq"))		{ v = llcEqual;				return true;	}
	if(!strcasecmp(s, "ne"))		{ v = llcNotEqual;			return true;	}
	if(!strcasecmp(s, "gt"))		{ v = llcGreater;			return true;	}
	if(!strcasecmp(s, "ge"))		{ v = llcGreaterOrEqual;	return true;	}
	if(!strcasecmp(s, "lt"))		{ v = llcLesser;			return true;	}
	if(!strcasecmp(s, "le"))		{ v = llcLesserOrEqual;		return true;	}
	return false;
}
//---------------------------------------------------------------------------
bool tTVPWaveLoopManager::GetString(char *s, tTVPLabelStringType &v)
{
	// convert utf-8 string s to v

	// compute output (unicode) size
	tjs_int size = TVPUtf8ToWideCharString(s, NULL);
	if(size == -1) return false; // not able to convert the string

	// allocate output buffer
	tjs_char *us = new tjs_char[size + 1];
	try
	{
		TVPUtf8ToWideCharString(s, us);
		us[size] = TJS_W('\0');

#ifdef TVP_IN_LOOP_TUNER
		// convert us (an array of wchar_t) to AnsiString
		v = AnsiString(us);
#else
		// convert us (an array of wchar_t) to ttstr
		v = ttstr(us);
#endif

	}
	catch(...)
	{
		delete [] us;
		throw;
	}
	delete [] us;
	return true;
}
//---------------------------------------------------------------------------
bool tTVPWaveLoopManager::GetEntityToken(char * & p, char **name, char **value)
{
	// get name=value string at 'p'.
	// returns whether the token can be got or not.
	// on success, *id will point start of the name, and value will point
	// start of the value. the buffer given by 'start' will be destroied.

	char * namelast;
	char * valuelast;
	char delimiter = '\0';

	// skip preceeding white spaces
	while(isspace(*p)) p++;
	if(!*p) return false;

	// p will now be a name
	*name = p;

	// find white space or '='
	while(!isspace(*p) && *p != '=' && *p) p++;
	if(!*p) return false;

	namelast = p;

	// skip white space
	while(isspace(*p)) p++;
	if(!*p) return false;

	// is current pointer pointing '='  ?
	if(*p != '=') return false;

	// step pointer
	p ++;
	if(!*p) return false;

	// skip white space
	while(isspace(*p)) p++;
	if(!*p) return false;

	// find delimiter
	if(*p == '\'') delimiter = *p, p++;
	if(!*p) return false;

	// now p will be start of value
	*value = p;

	// find delimiter or white space or ';'
	if(delimiter == '\0')
	{
		while((!isspace(*p) && *p != ';') && *p) p++;
	}
	else
	{
		while((*p != delimiter) && *p) p++;
	}

	// remember value last point
	valuelast = p;

	// skip last delimiter
	if(*p == delimiter) p++;

	// put null terminator
	*namelast = '\0';
	*valuelast = '\0';

	// finish
	return true;
}
//---------------------------------------------------------------------------
bool tTVPWaveLoopManager::ReadLinkInformation(char * & p, tTVPWaveLoopLink &link)
{
	// read link information from 'p'.
	// p must point '{' , which indicates start of the block.
	if(*p != '{') return false;

	p++;
	if(!*p) return false;

	do
	{
		// get one token from 'p'
		char * name;
		char * value;
		if(!GetEntityToken(p, &name, &value))
			return false;

		if(!strcasecmp(name, "From"))
		{	if(!GetInt64(value, link.From))					return false;}
		else if(!strcasecmp(name, "To"))
		{	if(!GetInt64(value, link.To))					return false;}
		else if(!strcasecmp(name, "Smooth"))
		{	if(!GetBool(value, link.Smooth))				return false;}
		else if(!strcasecmp(name, "Condition"))
		{	if(!GetCondition(value, link.Condition))		return false;}
		else if(!strcasecmp(name, "RefValue"))
		{	if(!GetInt(value, link.RefValue))				return false;}
		else if(!strcasecmp(name, "CondVar"))
		{	if(!GetInt(value, link.CondVar))				return false;}
		else
		{													return false;}

		// skip space
		while(isspace(*p)) p++;

		// check ';'. note that this will also be a null, if no delimiters are used
		if(*p != ';' && *p != '\0') return false;
		p++;
		if(!*p) return false;

		// skip space
		while(isspace(*p)) p++;
		if(!*p) return false;

		// check '}'
		if(*p == '}') break;
	} while(true);

	p++;

	return true;
}
//---------------------------------------------------------------------------
bool tTVPWaveLoopManager::ReadLabelInformation(char * & p, tTVPWaveLabel &label)
{
	// read label information from 'p'.
	// p must point '{' , which indicates start of the block.
	if(*p != '{') return false;

	p++;
	if(!*p) return false;

	do
	{
		// get one token from 'p'
		char * name;
		char * value;
		if(!GetEntityToken(p, &name, &value))
			return false;

		if(!strcasecmp(name, "Position"))
		{	if(!GetInt64(value, label.Position))			return false;}
		else if(!strcasecmp(name, "Name"))
		{	if(!GetString(value, label.Name))				return false;}
		else
		{													return false;}

		// skip space
		while(isspace(*p)) p++;

		// check ';'. note that this will also be a null, if no delimiters are used
		if(*p != ';' && *p != '\0') return false;
		p++;
		if(!*p) return false;

		// skip space
		while(isspace(*p)) p++;
		if(!*p) return false;

		// check '}'
		if(*p == '}') break;
	} while(true);

	p++;

	return true;
}
//---------------------------------------------------------------------------
bool tTVPWaveLoopManager::ReadInformation(char * p)
{
	// read information from 'p'
	volatile tTJSCriticalSectionHolder CS(FlagsCS);

	char *p_org = p;
	Links.clear();
	Labels.clear();

	// check version
	if(*p != '#')
	{
		// old sli format
		char *p_length = strstr(p, "LoopLength=");
		char *p_start  = strstr(p, "LoopStart=");
		if(!p_length || !p_start) return false; // read error
		tTVPWaveLoopLink link;
		link.Smooth = false;
		link.Condition = llcNone;
		link.RefValue = 0;
		link.CondVar = 0;
		tjs_int64 start;
		tjs_int64 length;
		if(!GetInt64(p_length + 11, length)) return false;
		if(!GetInt64(p_start  + 10, start )) return false;
		link.From = start + length;
		link.To = start;
		Links.emplace_back(link);
	}
	else
	{
		// sli v2.0+
		if(strncmp(p, "#2.00", 5) > 0)
			return false; // version mismatch 

		while(true)
		{
			if((p == p_org || p[-1] == '\n') && *p == '#')
			{
				// line starts with '#' is a comment
				// skip the comment
				while(*p != '\n' && *p) p++;
				if(!*p) break;

				p ++;
				continue;
			}

			// skip white space
			while(isspace(*p)) p++;
			if(!*p) break;

			// read id (Link or Label)
			if(!strncasecmp(p, "Link", 4) && !isalpha(p[4]))
			{
				p += 4;
				while(isspace(*p)) p++;
				if(!*p) return false;
				tTVPWaveLoopLink link;
				if(!ReadLinkInformation(p, link)) return false;
				Links.emplace_back(link);
			}
			else if(!strncasecmp(p, "Label", 5) && !isalpha(p[5]))
			{
				p += 5;
				while(isspace(*p)) p++;
				if(!*p) return false;
				tTVPWaveLabel label;
				if(!ReadLabelInformation(p, label)) return false;
				Labels.emplace_back(label);
			}
			else
			{
				return false; // read error
			}

			// skip white space
			while(isspace(*p)) p++;
			if(!*p) break;
		}
	}

	return true; // done
}

bool tTVPWaveLoopManager::DesiredFormat(const tTVPWaveFormat & format) {
	if (!Decoder->DesiredFormat(format)) return false;
	SetDecoder(Decoder);
	return true;
}
//---------------------------------------------------------------------------
#ifdef TVP_IN_LOOP_TUNER
//---------------------------------------------------------------------------
void tTVPWaveLoopManager::PutInt(AnsiString &s, tjs_int v)
{
	s += AnsiString((int)v);
}
//---------------------------------------------------------------------------
void tTVPWaveLoopManager::PutInt64(AnsiString &s, tjs_int64 v)
{
	s += AnsiString((__int64)v);
}
//---------------------------------------------------------------------------
void tTVPWaveLoopManager::PutBool(AnsiString &s, bool v)
{
	s += v ? "True" : "False";
}
//---------------------------------------------------------------------------
void tTVPWaveLoopManager::PutCondition(AnsiString &s, tTVPWaveLoopLinkCondition v)
{
	switch(v)
	{
		case llcNone:             s += "no" ; break;
		case llcEqual:            s += "eq" ; break;
		case llcNotEqual:         s += "ne" ; break;
		case llcGreater:          s += "gt" ; break;
		case llcGreaterOrEqual:   s += "ge" ; break;
		case llcLesser:           s += "lt" ; break;
		case llcLesserOrEqual:    s += "le" ; break;
	}
}
//---------------------------------------------------------------------------
void tTVPWaveLoopManager::PutString(AnsiString &s, tTVPLabelStringType v)
{
	// convert v to a utf-8 string
	const tjs_char *pi;

#ifdef TVP_IN_LOOP_TUNER
	WideString wstr = v;
	pi = wstr.c_bstr();
#else
	pi = v.c_str();
#endif

	// count output bytes
	int size = TVPWideCharToUtf8String(pi, NULL);

	char * out = new char [size + 1];
	try
	{
		// convert the string
		TVPWideCharToUtf8String(pi, out);
		out[size] = '\0';

		// append the string with quotation
		s += "\'" + AnsiString(out) + "\'";
	}
	catch(...)
	{
		delete [] out;
		throw;
	}
	delete [] out;
}
//---------------------------------------------------------------------------
void tTVPWaveLoopManager::DoSpacing(AnsiString &l, int col)
{
	// fill space until the string becomes specified length
	static const char * spaces16 = "                ";
	int length = l.Length();
	if(length < col)
	{
		int remain = col - length;
		while(remain)
		{
			int one_size = remain > 16 ? 16 : remain;
			l += ((16 - one_size) + spaces16);
			remain -= one_size;
		}
	}
}
//---------------------------------------------------------------------------
void tTVPWaveLoopManager::WriteInformation(AnsiString &s)
{
	// write current link/label information into s
	volatile tTJSCriticalSectionHolder CS(FlagsCS);

	// write banner
	s = "#2.00\n# Sound Loop Information (utf-8)\n"
		"# Generated by WaveLoopManager.cpp\n";

	// write links
/*
Link { From=0000000000000000; To=0000000000000000; Smooth=False; Condition=ne; RefValue=444444444; CondVar=99; }
*/
	for(std::vector<tTVPWaveLoopLink>::iterator i = Links.begin();
		i != Links.end(); i++)
	{
		AnsiString l;
		l = "Link { ";

		l += "From=";
		PutInt64(l, i->From);
		l += ";";
		DoSpacing(l, 30);

		l += "To=";
		PutInt64(l, i->To);
		l += ";";
		DoSpacing(l, 51);

		l += "Smooth=";
		PutBool(l, i->Smooth);
		l += ";";
		DoSpacing(l, 65);

		l += "Condition=";
		PutCondition(l, i->Condition);
		l += ";";
		DoSpacing(l, 79);

		l += "RefValue=";
		PutInt(l, i->RefValue);
		l += ";";
		DoSpacing(l, 100);

		l += "CondVar=";
		PutInt(l, i->CondVar);
		l += ";";
		DoSpacing(l, 112);

		l += "}\n";
		s += l;
	}


	// write labels
/*
Label { Position=0000000000000000; name="                                         "; }
*/
	for(std::vector<tTVPWaveLabel>::iterator i = Labels.begin();
		i != Labels.end(); i++)
	{
		AnsiString l;
		l = "Label { ";

		l += "Position=";
		PutInt64(l, i->Position);
		l += ";";
		DoSpacing(l, 35);

		l += "Name=";
		PutString(l, i->Name);
		l += "; ";
		DoSpacing(l, 85);

		l += "}\n";
		s += l;
	}

}
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------





