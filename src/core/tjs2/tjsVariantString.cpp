//---------------------------------------------------------------------------
/*
	TJS2 Script Engine
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// string heap management used by tTJSVariant and tTJSString
//---------------------------------------------------------------------------
#include "tjsCommHead.h"

#include "tjsVariantString.h"
#include "tjsError.h"
#include "tjsUtils.h"
#include "tjsLex.h"


namespace TJS
{
//---------------------------------------------------------------------------
// throwing error functions
//---------------------------------------------------------------------------
void TJSThrowStringDeallocError()
{
	TJS_eTJSVariantError(TJSStringDeallocError);
}
//---------------------------------------------------------------------------
void TJSThrowStringAllocError()
{
	TJS_eTJSVariantError(TJSStringAllocError);
}
//---------------------------------------------------------------------------
void TJSThrowNarrowToWideConversionError()
{
	TJS_eTJSVariantError(TJSNarrowToWideConversionError);
}
//---------------------------------------------------------------------------
tjs_int TJSGetShorterStrLen(const tjs_char *str, tjs_int max)
{
	// select shorter length over strlen(str) and max
	if(!str) return 0;
	const tjs_char *p = str;
	max++;
	while(*p && --max) p++;
	return (tjs_int)(p - str);
}
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// base memory allocation functions for long string
//---------------------------------------------------------------------------
#define TJSVS_ALLOC_INC_SIZE_S 16
	// additional space for long string heap under TJSVS_ALLOC_DOUBLE_LIMIT
#define TJSVS_ALLOC_INC_SIZE_L 4000000
	// additional space for long string heap over TJSVS_ALLOC_DOUBLE_LIMIT
#define TJSVS_ALLOC_DOUBLE_LIMIT 4000000
	// switching value of double-sizing or incremental-sizing
//---------------------------------------------------------------------------
/*static inline*/ tjs_char *TJSVS_malloc(tjs_uint len)
{
	char *ret = (char*)malloc(
		(len = (len + TJSVS_ALLOC_INC_SIZE_S))*sizeof(tjs_char) + sizeof(size_t));
	if(!ret) TJSThrowStringAllocError();
	*(size_t *)ret = len; // embed size
	return (tjs_char*)(ret + sizeof(size_t));
}
//---------------------------------------------------------------------------
/*static inline*/ tjs_char *TJSVS_realloc(tjs_char *buf, tjs_uint len)
{
	if(!buf) return TJSVS_malloc(len);

	// compare embeded size
	size_t * ptr = (size_t *)((char*)buf - sizeof(size_t));
	if(*ptr >= len) return buf; // still adequate

//	char *ret = (char*)realloc(ptr,
//		(len = (len + TJSVS_ALLOC_INC_SIZE))*sizeof(tjs_char) + sizeof(size_t));
	if(len < TJSVS_ALLOC_DOUBLE_LIMIT)
		len = len * 2;
	else
		len = len + TJSVS_ALLOC_INC_SIZE_L;

	char *ret = (char*)realloc(ptr,
		len*sizeof(tjs_char) + sizeof(size_t));
	if(!ret) TJSThrowStringAllocError();
	*(size_t *)ret = len; // embed size
	return (tjs_char*)(ret + sizeof(size_t));
}
//---------------------------------------------------------------------------
/*static inline*/ void TJSVS_free(tjs_char *buf)
{
	// free buffer
	free((char*)buf - sizeof(size_t));
}
//---------------------------------------------------------------------------





//---------------------------------------------------------------------------
// StringHeap
//---------------------------------------------------------------------------
//#define TJS_VS_USE_SYSTEM_NEW

#define HEAP_FLAG_USING 0x01
#define HEAP_FLAG_DELETE 0x02
#define HEAP_CAPACITY_INC 4096
static tTJSSpinLock TJSStringHeapCS;
static std::vector<tTJSVariantString*> *TJSStringHeapList = NULL;
static tTJSVariantString ** TJSStringHeapFreeCellList = NULL;
//static tjs_uint TJSStringHeapFreeCellListCapacity = 0;
static tjs_uint TJSStringHeapFreeCellListPointer = 0;
static tjs_uint TJSStringHeapAllocCount = 0;

static tjs_uint TJSStringHeapLastCheckedFreeBlock = 0;
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
static void TJSAddStringHeapBlock(void)
{
	// allocate StringHeapBlock
	tTJSVariantString *h;
	h = new tTJSVariantString[HEAP_CAPACITY_INC];
	memset(h, 0, sizeof(tTJSVariantString) * HEAP_CAPACITY_INC);
	TJSStringHeapList->push_back(h);

	// re-allocate TJSFreeCellList
	if(TJSStringHeapFreeCellList) delete [] TJSStringHeapFreeCellList;
	TJSStringHeapFreeCellList =
		new tTJSVariantString *[TJSStringHeapList->size() * HEAP_CAPACITY_INC];

	// prepare free list
	for(tjs_int i = HEAP_CAPACITY_INC - 1; i >= 0; i--)
		TJSStringHeapFreeCellList[i] = h + i;
	TJSStringHeapFreeCellListPointer = HEAP_CAPACITY_INC;
}
//---------------------------------------------------------------------------
static void TJSInitStringHeap()
{
	TJSStringHeapList = new std::vector<tTJSVariantString*>();
	TJSAddStringHeapBlock(); // initial block
}
//---------------------------------------------------------------------------
static void TJSUninitStringHeap(void)
{
	if(!TJSStringHeapList) return;
	if(!TJSStringHeapList->empty())
	{
#if 0
		std::vector<tTJSVariantString*>::iterator c;
		for(c = TJSStringHeapList->end()-1; c >= TJSStringHeapList->begin(); c--)
		{
			tTJSVariantString *h = *c;
			for(tjs_int i = 0; i < HEAP_CAPACITY_INC; i++)
			{
				if(h[i].HeapFlag & HEAP_FLAG_USING)
				{
					// using cell
					if(h[i].LongString) TJSVS_free(h[i].LongString);
				}
			}
			delete [] h;
		}
#else
		std::vector<tTJSVariantString*>::reverse_iterator c;
		for( c = TJSStringHeapList->rbegin(); c != TJSStringHeapList->rend(); c++ ) {
			tTJSVariantString *h = *c;
			for(tjs_int i = 0; i < HEAP_CAPACITY_INC; i++) {
				if(h[i].HeapFlag & HEAP_FLAG_USING) {
					// using cell
					if(h[i].LongString) TJSVS_free(h[i].LongString);
				}
			}
			delete [] h;
		}
#endif
	}

	delete TJSStringHeapList;
	TJSStringHeapList = NULL;

	if(TJSStringHeapFreeCellList)
	{
		delete [] TJSStringHeapFreeCellList;
		TJSStringHeapFreeCellList = NULL;
	}
}
//---------------------------------------------------------------------------
#ifdef TJS_DEBUG_UNRELEASED_STRING
static void TJSUninitStringHeap(void)
{
	if(!TJSStringHeapList) return;
	if(!TJSStringHeapList->empty())
	{
		std::vector<tTJSVariantString*>::iterator c;
		for(c = TJSStringHeapList->end()-1; c >= TJSStringHeapList->begin(); c--)
		{
			tTJSVariantString *h = *c;
			for(tjs_int i = 0; i < HEAP_CAPACITY_INC; i++)
			{
				if(h[i].HeapFlag & HEAP_FLAG_USING)
				{
					// using cell
					char buf[1024];
					TJS_nsprintf(buf, "%p:%ls", h + i,
						h[i].LongString?h[i].LongString:h[i].ShortString);
					OutputDebugString(buf);
				}
			}
		}
	}
}
#pragma exit TJSReportUnreleasedStringHeap 1

#endif

//---------------------------------------------------------------------------
#ifdef TJS_DEBUG_DUMP_STRING
void TJSDumpStringHeap(void)
{
	if(!TJSStringHeapList) return;
	if(!TJSStringHeapList->empty())
	{
		FILE *f = fopen("string.txt", "wt");
		if(!f) return;
		std::vector<tTJSVariantString*>::iterator c;
		for(c = TJSStringHeapList->end()-1; c >= TJSStringHeapList->begin(); c--)
		{
			tTJSVariantString *h = *c;
			for(tjs_int i = 0; i < HEAP_CAPACITY_INC; i++)
			{
				if(h[i].HeapFlag & HEAP_FLAG_USING)
				{
					// using cell
					tTJSNarrowStringHolder narrow(h[i].LongString?h[i].LongString:h[i].ShortString);

					fprintf(f, "%s\n", (const char*)narrow);
				}
			}
		}
		fclose(f);
	}
}
#endif
//---------------------------------------------------------------------------
static int TJS_USERENTRY TJSStringHeapSortFunction(const void *a, const void *b)
{
	return (int)(*(const tTJSVariantString **)b - *(const tTJSVariantString **)a);
}
//---------------------------------------------------------------------------
void TJSCompactStringHeap()
{
#ifdef TJS_DEBUG_CHECK_STRING_HEAP_INTEGRITY

	if(!TJSStringHeapList) return;
	if(!TJSStringHeapList->empty())
	{
		std::vector<tTJSVariantString*>::iterator c;
		for(c = TJSStringHeapList->end()-1; c >= TJSStringHeapList->begin(); c--)
		{
			tTJSVariantString *h = *c;
			for(tjs_int i = 0; i < HEAP_CAPACITY_INC; i++)
			{
				if(h[i].HeapFlag & HEAP_FLAG_USING)
				{
					// using cell
					const tjs_char * ptr = h[i].operator const tjs_char *();
					if(ptr[0] == 0)
					{
						OutputDebugString("empty string cell found");
					}
					if(TJS_strlen(ptr) != h[i].GetLength())
					{
						OutputDebugString("invalid string length cell found");
					}
				}
			}
		}
	}
#endif

#define TJS_SORT_STR_CELL_MAX 1000
#define TJS_CHECK_FREE_BLOCK_MAX 50

	// sort free cell list by address
	if(!TJSStringHeapFreeCellList) return;
	if(!TJSStringHeapList) return;

	{	// thread-protected
		tTJSSpinLockHolder csh(TJSStringHeapCS);

#ifndef __CODEGUARD__
	// may be very slow when used with codeguard
		// sort TJSStringHeapFreeCellList
		tTJSVariantString ** start;
		tjs_int count;
		if(TJSStringHeapFreeCellListPointer > TJS_SORT_STR_CELL_MAX)
		{
			// too large; sort last TJS_SORT_STR_CELL_MAX items
			start = TJSStringHeapFreeCellList +
				TJSStringHeapFreeCellListPointer - TJS_SORT_STR_CELL_MAX;
			count = TJS_SORT_STR_CELL_MAX;
		}
		else
		{
			start = TJSStringHeapFreeCellList;
			count = TJSStringHeapFreeCellListPointer;
		}

		qsort(start,
			count,
			sizeof(tTJSVariantString *),
			TJSStringHeapSortFunction);
#endif

		// delete all-freed heap block

		// - mark all-freed heap block
		tjs_int free_block_count = 0;

		if(TJSStringHeapList)
		{
			if(TJSStringHeapLastCheckedFreeBlock >= TJSStringHeapList->size())
				TJSStringHeapLastCheckedFreeBlock = 0;
			tjs_uint block_ind = TJSStringHeapLastCheckedFreeBlock;
			tjs_int count = 0;

			do
			{
				tjs_int freecount = 0;
				tTJSVariantString * block = (*TJSStringHeapList)[block_ind];
				for(tjs_int i = 0; i < HEAP_CAPACITY_INC; i++)
				{
					if(!(block[i].HeapFlag & HEAP_FLAG_USING))
						freecount ++;
				}

				if(freecount == HEAP_CAPACITY_INC)
				{
					// all freed
					free_block_count ++;
					for(tjs_int i = 0; i < HEAP_CAPACITY_INC; i++)
					{
						block[i].HeapFlag = HEAP_FLAG_DELETE;
					}
				}

				block_ind ++;
				if(block_ind >= TJSStringHeapList->size()) block_ind = 0;
				count++;
				
			} while(count < TJS_CHECK_FREE_BLOCK_MAX &&
				block_ind != TJSStringHeapLastCheckedFreeBlock);

			TJSStringHeapLastCheckedFreeBlock = block_ind;
		}

		// - delete all marked cell from TJSStringHeapFreeCellList
		if(free_block_count)
		{
			tTJSVariantString ** newlist =
				new tTJSVariantString *
					[(TJSStringHeapList->size() - free_block_count) *
					  HEAP_CAPACITY_INC];

			tjs_uint wp = 0;
			tjs_uint rp;
			for(rp = 0; rp < TJSStringHeapFreeCellListPointer; rp++)
			{
				if(TJSStringHeapFreeCellList[rp]->HeapFlag != HEAP_FLAG_DELETE)
					newlist[wp++] =
						TJSStringHeapFreeCellList[rp];
			}

			TJSStringHeapFreeCellListPointer = wp;

			delete [] TJSStringHeapFreeCellList;
			TJSStringHeapFreeCellList = newlist;
		}

		// - delete all marked block
		if(free_block_count)
		{
			std::vector<tTJSVariantString*>::iterator i;
			for(i = TJSStringHeapList->begin(); i != TJSStringHeapList->end();)
			{
				if((*i)[0].HeapFlag == HEAP_FLAG_DELETE)
				{
					// to be deleted
					delete [] (*i);
					i = TJSStringHeapList->erase(i);
				}
				else
				{
					i++;
				}
			}
		}
	}
}
//---------------------------------------------------------------------------
tTJSVariantString * TJSAllocStringHeap(void)
{
	if(TJSStringHeapAllocCount == 0)
	{
		// first string to alloc
		// here must be called in main thread...
		TJSInitStringHeap(); // initial block
	}

	{	// thread-protected
		tTJSSpinLockHolder csh(TJSStringHeapCS);

#ifdef TJS_VS_USE_SYSTEM_NEW
		tTJSVariantString *ret = new tTJSVariantString();
#else
		if(TJSStringHeapFreeCellListPointer == 0)
			TJSAddStringHeapBlock();

		tTJSVariantString *ret =
			TJSStringHeapFreeCellList[--TJSStringHeapFreeCellListPointer];
#endif

		ret->RefCount = 0;
		ret->Length = 0;
		ret->LongString = NULL;
		ret->HeapFlag = HEAP_FLAG_USING;
		ret->Hint = 0;

		TJSStringHeapAllocCount ++;

		return ret;

	}	// end-of-thread-protected
}
//---------------------------------------------------------------------------
void TJSDeallocStringHeap(tTJSVariantString * vs)
{
	// free vs

	{	// thread-pretected
		tTJSSpinLockHolder csh(TJSStringHeapCS);

#ifdef TJS_DEBUG_CHECK_STRING_HEAP_INTEGRITY
		{
			const tjs_char * ptr = vs->operator const tjs_char *();
			if(ptr[0] == 0)
			{
				OutputDebugString("empty string cell found");
			}
			if(TJS_strlen(ptr) != vs->GetLength())
			{
				OutputDebugString("invalid string length cell found");
			}
		}
#endif


		if(vs->LongString) TJSVS_free(vs->LongString);

		vs->HeapFlag = 0;

#ifdef TJS_VS_USE_SYSTEM_NEW
		delete vs;
#else
		TJSStringHeapFreeCellList[TJSStringHeapFreeCellListPointer++] = vs;
#endif

		TJSStringHeapAllocCount--;
	}	// end-of-thread-protected

	if(TJSStringHeapAllocCount == 0)
	{
		// last string was freed
		// here must be called in main thread...
		TJSUninitStringHeap();
	}
}
//---------------------------------------------------------------------------





//---------------------------------------------------------------------------
// tTJSVariantString
//---------------------------------------------------------------------------
void tTJSVariantString::Release()
{
/*
	if(!this) return;
		// this is not a REAL remedy, but enough to buster careless misses...
		// (NULL->Release() problem)
*/

	if(RefCount==0)
	{
		TJSDeallocStringHeap(this);
		return;
	}
	RefCount--;
}
//---------------------------------------------------------------------------
tTVInteger tTJSVariantString::ToInteger() const
{
	if(!this) return 0;

	tTJSVariant val;
	const tjs_char *ptr = this->operator const tjs_char *();
	if(TJSParseNumber(val, &ptr)) 	return val.AsInteger();
	return 0;
}
//---------------------------------------------------------------------------
tTVReal tTJSVariantString::ToReal() const
{
	if(!this) return 0;

	tTJSVariant val;
	const tjs_char *ptr = this->operator const tjs_char *();
	if(TJSParseNumber(val, &ptr)) return val.AsReal();
	return 0;
}
//---------------------------------------------------------------------------
void tTJSVariantString::ToNumber(tTJSVariant &dest) const
{
	if(!this) { dest = 0; return; }

	const tjs_char *ptr = this->operator const tjs_char *();
	if(TJSParseNumber(dest, &ptr)) return;

	dest = 0;
}
//---------------------------------------------------------------------------
tTJSVariantString::operator const tjs_char *() const
{
	return (!this) ? (NULL) : (LongString ? LongString : ShortString);
}
//---------------------------------------------------------------------------
tjs_int tTJSVariantString::GetLength() const
{
	if (!this) return 0;
	return Length;
}
//---------------------------------------------------------------------------
tTJSVariantString * tTJSVariantString::FixLength()
{
	if(!this) return NULL;

	if(RefCount != 0) TJSThrowStringDeallocError();
	Length = (tjs_int)TJS_strlen(this->operator const tjs_char*());
	if(!Length)
	{
		TJSDeallocStringHeap(this);
		return NULL;
	}
	return this;
}
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// other allocation functions
//---------------------------------------------------------------------------
tTJSVariantString * TJSAllocVariantString(const tjs_char *ref1, const tjs_char *ref2)
{
	if(!ref1 && !ref2) return NULL;

	if(ref1)
	{
		if(ref1[0]==0)
		{
			if(ref2)
			{
				if(ref2[0]==0) return NULL;
			}
		}
	}

	tjs_intptr_t len1 = ref1 ? TJS_strlen(ref1) : 0;
	tjs_intptr_t len2 = ref2 ? TJS_strlen(ref2) : 0;

	tTJSVariantString *ret = TJSAllocStringHeap();

	if(len1+len2>TJS_VS_SHORT_LEN)
	{
		ret->LongString = TJSVS_malloc((tjs_uint)(len1+len2+1));
		if(ref1) TJS_strcpy(ret->LongString , ref1);
		if(ref2) TJS_strcpy(ret->LongString + len1, ref2);
	}
	else
	{
		if(ref1) TJS_strcpy(ret->ShortString, ref1);
		if(ref2) TJS_strcpy(ret->ShortString + len1, ref2);
	}
	ret->Length = (tjs_int)(len1+len2);
	return ret;
}
//---------------------------------------------------------------------------
tTJSVariantString * TJSAllocVariantString(const tjs_char *ref)
{
	if(!ref) return NULL;
	if(ref[0]==0) return NULL;
	tTJSVariantString *ret = TJSAllocStringHeap();
	ret->SetString(ref);
	return ret;
}
//---------------------------------------------------------------------------
tTJSVariantString * TJSAllocVariantString(const tjs_char *ref, tjs_int n)
{
	if(n==0) return NULL;
	if(!ref) return NULL;
	if(ref[0]==0) return NULL;
	tTJSVariantString *ret = TJSAllocStringHeap();
	ret->SetString(ref, n);
	return ret;
}
//---------------------------------------------------------------------------
tTJSVariantString * TJSAllocVariantString(const tjs_nchar *ref)
{
	if(!ref) return NULL;
	if(ref[0]==0) return NULL;
	tTJSVariantString *ret = TJSAllocStringHeap();
	ret->SetString(ref);
	return ret;
}
//---------------------------------------------------------------------------
tTJSVariantString * TJSAllocVariantString(const tjs_uint8 **src)
{
	tjs_uint size = *(const tjs_uint *)(*src);
	*src += sizeof(tjs_uint);
	if(!size) return 0;
	*src += sizeof(tjs_uint);
	tTJSVariantString *ret = TJSAllocStringHeap();
	ret->SetString((const tjs_char *)src, size);
	*src += sizeof(tjs_char) * size;
	return ret;
}
//---------------------------------------------------------------------------
tTJSVariantString * TJSAllocVariantStringBuffer(tjs_uint len)
{
	/* note that you must call FixLength if you allocate larger than the
	actual string size */


	tTJSVariantString *ret = TJSAllocStringHeap();
	ret->AllocBuffer(len);
	return ret;
}

//---------------------------------------------------------------------------
tTJSVariantString * TJSAppendVariantString(tTJSVariantString *str,
	const tjs_char *app)
{
	if(!app) return str;
	if(!str)
	{
		str = TJSAllocVariantString(app);
		return str;
	}
	str->Append(app);
	return str;
}
//---------------------------------------------------------------------------
tTJSVariantString * TJSAppendVariantString(tTJSVariantString *str,
	const tTJSVariantString *app)
{
	if(!app) return str;
	if(!str)
	{
		str = TJSAllocVariantString(app->operator const tjs_char *());
		return str;
	}
	str->Append(app->operator const tjs_char *(), app->GetLength());
	return str;
}
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
class tTVPVariantStringHolder
{
	// this class keeps one Variant String from program start to program end,
	// to ensure heap system are alive during program's lifetime.
	tTJSVariantString * String;
public:
	tTVPVariantStringHolder()
	{ String = TJSAllocVariantString(TJS_W("This is a dummy.")); }
	~tTVPVariantStringHolder()
	{ String->Release(); }
} static TVPVariantStringHolder;
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
#define TJS_VS_FS_OUT_INC_SIZE 32
tTJSVariantString * TJSFormatString(const tjs_char *format, tjs_uint numparams,
	tTJSVariant **params)
{
	// TODO: more reliable implementation

	// format the string with the format illustrated as "format"
	// this is similar to C sprintf format, but this support only following:
	// % [flags] [width] [.prec] type_char
	// flags : '-'|'+'|' '|'#'
	// width : n|'0'n|*|
	// .prec : '.0'|'.n'|'.*'
	// type_char : [diouxXfegEGcs]
	// [diouxX] as an integer
	// [fegEG] as a real
	// [cs] as a string
	// and, this is safe for output buffer memory or inadequate parameters.

	const tjs_char *f = format;
	if(!f) return NULL;

	TJSSetFPUE();
	tTJSVariantString *ret = TJSAllocVariantStringBuffer(TJS_VS_FS_OUT_INC_SIZE);
	tjs_uint allocsize = TJS_VS_FS_OUT_INC_SIZE;
	tjs_char *o = const_cast<tjs_char*>(ret->operator const tjs_char*());
	tjs_uint s = 0;

	tjs_uint in = 0;

#define check_alloc \
			if(s >= allocsize) \
			{ \
				ret->AppendBuffer(TJS_VS_FS_OUT_INC_SIZE); \
				o = const_cast<tjs_char*>(ret->operator const tjs_char*()); \
				allocsize += TJS_VS_FS_OUT_INC_SIZE; \
			}

	for(;*f;)
	{
		if(*f!=TJS_W('%'))
		{
			check_alloc;
			o[s++] = *(f++);
			continue;
		}

		// following are only format-checking, actual processing is in sprintf.

		tjs_char flag = 0;
		tjs_uint width = 0;
//		bool zeropad = false;
		bool width_ind = false;
		tjs_uint prec = 0;
//		bool precspec = false;
		bool prec_ind = false;
		const tjs_char *fst = f;

		f++;
		if(!*f) goto error;

	// flags
		switch(*f)
		{
		case TJS_W('-'): flag = TJS_W('-'); break;
		case TJS_W('+'): flag = TJS_W('+'); break;
		case TJS_W('#'): flag = TJS_W('#'); break;
		default: goto width;
		}

		f++;
		if(!*f) goto error;

	width:
		switch(*f)
		{
		case TJS_W('0'): /*zeropad = true;*/ break;
		default: goto width_2;
		}

		f++;
		if(!*f) goto error;

	width_2:
		switch(*f)
		{
		case TJS_W('*'): width_ind = true; break;
		default: goto width_3;
		}

		f++;
		if(!*f) goto error;

		goto prec;

	width_3:
		while(true)
		{
			if(*f >= TJS_W('0') && *f <= TJS_W('9'))
				width = width *10 + (*f - TJS_W('0'));
			else
				break;
			f++;
		}

		if(!*f) goto error;

	prec:
		if(*f == TJS_W('.'))
		{
			f++;
			if(!*f) goto error;
			if(*f == TJS_W('*'))
			{
				prec_ind = true;
				f++;
				if(!*f) goto error;
				goto type_char;
			}
			if(*f < TJS_W('0') || *f > TJS_W('9')) goto error;
			/*precspec = true;*/
			do
			{
				prec = prec * 10 + (*f-TJS_W('0'));
				f++;
			} while(*f >= TJS_W('0') && *f <= TJS_W('9'));
		}

	type_char:
		switch(*f)
		{
		case TJS_W('%'):
			// literal '%'
			check_alloc;
			o[s++] = '%';
			f++;
			continue;


		case TJS_W('c'):
		case TJS_W('s'):
		  {
			if(width_ind)
			{
				if(in>=numparams) TJS_eTJSVariantError(TJSBadParamCount);
				width = (tjs_int)(params[in++])->AsInteger();
			}
			if(prec_ind)
			{
				if(in>=numparams) TJS_eTJSVariantError(TJSBadParamCount);
				prec = (tjs_int)(params[in++])->AsInteger();
			}
			if(in>=numparams) TJS_eTJSVariantError(TJSBadParamCount);
			tTJSVariantString *str = (params[in++])->AsString();
			if(str)
			{
				tjs_uint slen = str->GetLength();
				if(!prec) prec = slen;
				if(*f == TJS_W('c') && prec > 1 ) prec = 1;
				if(width < prec) width = prec;
				if(s + width > allocsize)
				{
					try
					{
						tjs_uint inc_size;
						ret->AppendBuffer(inc_size = s+width-allocsize+TJS_VS_FS_OUT_INC_SIZE);
						o = const_cast<tjs_char*>(ret->operator const tjs_char*());
						allocsize += inc_size;
					}
					catch(...)
					{
						str->Release();
						throw;
					}
				}
				tjs_uint pad = width - prec;
				if(pad)
				{
					if(flag == TJS_W('-'))
					{
						// left align
						if(str) TJS_strncpy(o+s, *str, prec);
						tjs_char * p = o + s + prec;
						while(pad--) 0[p++] = TJS_W(' ');
					}
					else
					{
						// right align
						if(str) TJS_strncpy(o+s+pad, *str, prec);
						tjs_char *p = o + s;
						while(pad--) 0[p++] = TJS_W(' ');
					}
					s += width;
				}
				else
				{
					if(str) TJS_strncpy(o+s, *str, prec);
					s += prec;
				}
				str->Release();
			}
			f++;
			continue;
		  }
		case TJS_W('d'): case TJS_W('i'): case TJS_W('o'): case TJS_W('u'):
		case TJS_W('x'): case TJS_W('X'):
		  {
			// integers
			if(width+prec > 900) goto error;// too long
			tjs_char buf[1024];
			//tjs_char *p;
			tjs_char fmt[70];
			tjs_uint fmtlen = (tjs_uint)(f - fst);
			if(fmtlen > 65) goto error;  // too long
			TJS_strncpy(fmt, fst, fmtlen);
// #ifdef WIN32
// 			fmt[fmtlen++] = 'I';
// 			fmt[fmtlen++] = '6';
// 			fmt[fmtlen++] = '4';
// #else
			fmt[fmtlen++] = 'l';
			fmt[fmtlen++] = 'l';
//#endif
			fmt[fmtlen++] = *f;
			fmt[fmtlen++] = 0;
			int ind[2];
			if(!width_ind && !prec_ind)
			{
				if(in>=numparams) TJS_eTJSVariantError(TJSBadParamCount);
				tTVInteger integer = (params[in++])->AsInteger();
				TJS_snprintf(buf, sizeof(buf)/sizeof(tjs_char), fmt, integer);
			}
			else if((!width_ind && prec_ind) || (width_ind && !prec_ind))
			{
				if(in>=numparams) TJS_eTJSVariantError(TJSBadParamCount);
				ind[0] = static_cast<int>( (params[in++])->AsInteger() );
				if(width_ind && ind[0] + prec > 900) goto error;
				if(prec_ind && ind[0] + width > 900) goto error;
				if(in>=numparams) TJS_eTJSVariantError(TJSBadParamCount);
				tTVInteger integer = (params[in++])->AsInteger();
				TJS_snprintf(buf, sizeof(buf)/sizeof(tjs_char), fmt, ind[0], integer);
			}
			else
			{
				if(in>=numparams) TJS_eTJSVariantError(TJSBadParamCount);
				ind[0] = static_cast<int>( (params[in++])->AsInteger() );
				if(in>=numparams) TJS_eTJSVariantError(TJSBadParamCount);
				ind[1] = static_cast<int>( (params[in++])->AsInteger() );
				if(ind[0] + ind[1] > 900) goto error;
				if(in>=numparams) TJS_eTJSVariantError(TJSBadParamCount);
				tTVInteger integer = (params[in++])->AsInteger();
				TJS_snprintf(buf, sizeof(buf)/sizeof(tjs_char), fmt, ind[0], ind[1], integer);
			}

			tjs_uint size = (tjs_uint)TJS_strlen(buf);
			tjs_uint inc_size;
			if(s+size > allocsize)
			{
				ret->AppendBuffer(inc_size = s+size-allocsize+TJS_VS_FS_OUT_INC_SIZE);
				o = const_cast<tjs_char*>(ret->operator const tjs_char*());
				allocsize += inc_size;
			}
			TJS_strcpy(o+s, buf);
			s += size;

			f++;
			continue;
		  }
		case TJS_W('f'): case TJS_W('e'): case TJS_W('g'): case TJS_W('E'):
		case TJS_W('G'):
		  {
			// reals
 			if(width+prec > 900) goto error;// too long
			tjs_char buf[1024];
			tjs_char fmt[70];
			tjs_uint fmtlen = (tjs_uint)(f - fst);
			if(fmtlen > 67) goto error;  // too long
			TJS_strncpy(fmt, fst, fmtlen);
// #ifdef WIN32
// 			fmt[fmtlen++] = 'l';
// #else
			fmt[fmtlen++] = 'l';
//#endif
			fmt[fmtlen++] = *f;
			fmt[fmtlen++] = 0;
			int ind[2];
			if(!width_ind && !prec_ind)
			{
				if(in>=numparams) TJS_eTJSVariantError(TJSBadParamCount);
				tTVReal real = (params[in++])->AsReal();
				TJS_snprintf(buf, sizeof(buf)/sizeof(tjs_char), fmt, real);
			}
			else if((!width_ind && prec_ind) || (width_ind && !prec_ind))
			{
				if(in>=numparams) TJS_eTJSVariantError(TJSBadParamCount);
				ind[0] = static_cast<int>( (params[in++])->AsInteger() );
 				if(width_ind && ind[0] + prec > 900) goto error;
				if(prec_ind && ind[0] + width > 900) goto error;
				if(in>=numparams) TJS_eTJSVariantError(TJSBadParamCount);
				tTVReal real = (params[in++])->AsReal();
				TJS_snprintf(buf, sizeof(buf)/sizeof(tjs_char), fmt, ind[0], real);
			}
			else
			{
				if(in>=numparams) TJS_eTJSVariantError(TJSBadParamCount);
				ind[0] = static_cast<int>( (params[in++])->AsInteger() );
				if(in>=numparams) TJS_eTJSVariantError(TJSBadParamCount);
				ind[1] = static_cast<int>( (params[in++])->AsInteger() );
				if(ind[0] + ind[1] > 900) goto error;
				if(in>=numparams) TJS_eTJSVariantError(TJSBadParamCount);
				tTVReal real = (params[in++])->AsReal();
				TJS_snprintf(buf, sizeof(buf)/sizeof(tjs_char), fmt, ind[0], ind[1], real);
			}

			tjs_uint size = (tjs_uint)TJS_strlen(buf);
			tjs_uint inc_size;
			if(s+size > allocsize)
			{
				ret->AppendBuffer(inc_size = s+size-allocsize+TJS_VS_FS_OUT_INC_SIZE);
				o = const_cast<tjs_char*>(ret->operator const tjs_char*());
				allocsize += inc_size;
			}
			TJS_strcpy(o+s, buf);
			s += size;
			f++;
			continue;
		  }
		}

	}

	o[s] = 0;

	ret = ret->FixLength();

	return ret;

error:
	TJS_eTJSVariantError(TJSInvalidFormatString);
	return NULL; // not reached
}
//---------------------------------------------------------------------------

} // namespcae TJS

