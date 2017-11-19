//---------------------------------------------------------------------------
/*
	TJS2 Script Engine
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Intermediate Code Execution
//---------------------------------------------------------------------------

#include "tjsCommHead.h"

#include "tjsInterCodeExec.h"
#include "tjsInterCodeGen.h"
#include "tjsScriptBlock.h"
#include "tjsError.h"
#include "tjs.h"
#include "tjsUtils.h"
#include "tjsNative.h"
#include "tjsArray.h"
#include "tjsDebug.h"
#include "tjsOctPack.h"
#include "tjsGlobalStringMap.h"
#include <set>
#include <mutex>

#ifdef ENABLE_DEBUGGER
#include "debugger.h"
static bool isDebuggerPresent() {
	return true;
}
#define IsDebuggerPresent isDebuggerPresent
#endif // ENABLE_DEBUGGER
#include <thread>

namespace TJS
{
//---------------------------------------------------------------------------
// utility functions
//---------------------------------------------------------------------------
static void ThrowFrom_tjs_error_num(tjs_error hr, tjs_int num)
{
	tjs_char buf[34];
	TJS_int_to_str(num, buf);
	TJSThrowFrom_tjs_error(hr, buf);
}
//---------------------------------------------------------------------------
static void ThrowInvalidVMCode()
{
	TJS_eTJSError(TJSInvalidOpecode);
}
//---------------------------------------------------------------------------
static void GetStringProperty(tTJSVariant *result, const tTJSVariant *str,
	const tTJSVariant &member)
{
	// processes properties toward strings.
	if(member.Type() != tvtInteger && member.Type() != tvtReal)
	{
		const tjs_char *name = member.GetString();
		if(!name) TJSThrowFrom_tjs_error(TJS_E_MEMBERNOTFOUND, TJS_W(""));

		if(!TJS_strcmp(name, TJS_W("length")))
		{
			// get string length
			const tTJSVariantString * s = str->AsStringNoAddRef();
#ifdef __CODEGUARD__
			if(!s)
				*result = tTVInteger(0); // tTJSVariantString::GetLength can return zero if 'this' is NULL
			else
#endif
			*result = tTVInteger(s->GetLength());
			return;
		}
		else if(name[0] >= TJS_W('0') && name[0] <= TJS_W('9'))
		{
			const tTJSVariantString * valstr = str->AsStringNoAddRef();
			const tjs_char *s = str->GetString();
			tjs_int n = TJS_atoi(name);
			tjs_int len = valstr->GetLength();
			if(n == len) { *result = tTJSVariant(TJS_W("")); return; }
			if(n<0 || n>len)
				TJS_eTJSError(TJSRangeError);
			tjs_char bf[2];
			bf[1] = 0;
			bf[0] = s[n];
			*result = tTJSVariant(bf);
			return;
		}

		TJSThrowFrom_tjs_error(TJS_E_MEMBERNOTFOUND, name);
	}
	else // member.Type() == tvtInteger || member.Type() == tvtReal
	{
		const tTJSVariantString * valstr = str->AsStringNoAddRef();
		const tjs_char *s = str->GetString();
		tjs_int n = (tjs_int)member.AsInteger();
		tjs_int len = valstr->GetLength();
		if(n == len) { *result = tTJSVariant(TJS_W("")); return; }
		if(n<0 || n>len)
			TJS_eTJSError(TJSRangeError);
		tjs_char bf[2];
		bf[1] = 0;
		bf[0] = s[n];
		*result = tTJSVariant(bf);
		return;
	}
}
//---------------------------------------------------------------------------
static void SetStringProperty(tTJSVariant *param, const tTJSVariant *str,
	const tTJSVariant &member)
{
	// processes properties toward strings.
	if(member.Type() != tvtInteger && member.Type() != tvtReal)
	{
		const tjs_char *name = member.GetString();
		if(!name) TJSThrowFrom_tjs_error(TJS_E_MEMBERNOTFOUND, TJS_W(""));

		if(!TJS_strcmp(name, TJS_W("length")))
		{
			TJSThrowFrom_tjs_error(TJS_E_ACCESSDENYED, TJS_W(""));
		}
		else if(name[0] >= TJS_W('0') && name[0] <= TJS_W('9'))
		{
			TJSThrowFrom_tjs_error(TJS_E_ACCESSDENYED, TJS_W(""));
		}

		TJSThrowFrom_tjs_error(TJS_E_MEMBERNOTFOUND, name);
	}
	else // member.Type() == tvtInteger || member.Type() == tvtReal
	{
		TJSThrowFrom_tjs_error(TJS_E_ACCESSDENYED, TJS_W(""));
	}
}
//---------------------------------------------------------------------------
static void GetOctetProperty(tTJSVariant *result, const tTJSVariant *octet,
	const tTJSVariant &member)
{
	// processes properties toward octets.
	if(member.Type() != tvtInteger && member.Type() != tvtReal)
	{
		const tjs_char *name = member.GetString();
		if(!name) TJSThrowFrom_tjs_error(TJS_E_MEMBERNOTFOUND, TJS_W(""));

		if(!TJS_strcmp(name, TJS_W("length")))
		{
			// get string length
			tTJSVariantOctet *o = octet->AsOctetNoAddRef();
			if(o)
				*result = tTVInteger(o->GetLength());
			else
				*result = tTVInteger(0);
			return;
		}
		else if(name[0] >= TJS_W('0') && name[0] <= TJS_W('9'))
		{
			tTJSVariantOctet *o = octet->AsOctetNoAddRef();
			tjs_int n = TJS_atoi(name);
			tjs_int len = o?o->GetLength():0;
			if(n<0 || n>=len)
				TJS_eTJSError(TJSRangeError);
			*result = tTVInteger(o->GetData()[n]);
			return;
		}

		TJSThrowFrom_tjs_error(TJS_E_MEMBERNOTFOUND, name);
	}
	else // member.Type() == tvtInteger || member.Type() == tvtReal
	{
		tTJSVariantOctet *o = octet->AsOctetNoAddRef();
		tjs_int n = (tjs_int)member.AsInteger();
		tjs_int len = o?o->GetLength():0;
		if(n<0 || n>=len)
			TJS_eTJSError(TJSRangeError);
		*result = tTVInteger(o->GetData()[n]);
		return;
	}
}
//---------------------------------------------------------------------------
static void SetOctetProperty(tTJSVariant *param, const tTJSVariant *octet,
	const tTJSVariant &member)
{
	// processes properties toward octets.
	if(member.Type() != tvtInteger && member.Type() != tvtReal)
	{
		const tjs_char *name = member.GetString();
		if(!name) TJSThrowFrom_tjs_error(TJS_E_MEMBERNOTFOUND, TJS_W(""));

		if(!TJS_strcmp(name, TJS_W("length")))
		{
			TJSThrowFrom_tjs_error(TJS_E_ACCESSDENYED, TJS_W(""));
		}
		else if(name[0] >= TJS_W('0') && name[0] <= TJS_W('9'))
		{
			TJSThrowFrom_tjs_error(TJS_E_ACCESSDENYED, TJS_W(""));
		}

		TJSThrowFrom_tjs_error(TJS_E_MEMBERNOTFOUND, name);
	}
	else // member.Type() == tvtInteger || member.Type() == tvtReal
	{
		TJSThrowFrom_tjs_error(TJS_E_ACCESSDENYED, TJS_W(""));
	}
}

//---------------------------------------------------------------------------
// tTJSObjectProxy
//---------------------------------------------------------------------------
class tTJSObjectProxy : public iTJSDispatch2
{
/*
	a class that do:
	1. first access to the Dispatch1
	2. if failed, then access to the Dispatch2
*/
//	tjs_uint RefCount;

public:
	tTJSObjectProxy()
	{
//		RefCount = 1;
//		Dispatch1 = NULL;
//		Dispatch2 = NULL;
		// Dispatch1 and Dispatch2 are to be set by subsequent call of SetObjects
	};

	virtual ~tTJSObjectProxy()
	{
		if(Dispatch1) Dispatch1->Release();
		if(Dispatch2) Dispatch2->Release();
	};

	void SetObjects(iTJSDispatch2 *dsp1, iTJSDispatch2 *dsp2)
	{
		Dispatch1 = dsp1;
		Dispatch2 = dsp2;
		if(dsp1) dsp1->AddRef();
		if(dsp2) dsp2->AddRef();
	}

private:
	iTJSDispatch2 *Dispatch1;
	iTJSDispatch2 *Dispatch2;

public:

	tjs_uint TJS_INTF_METHOD  AddRef(void)
	{
		return 1;
//		return ++RefCount;
	}

	tjs_uint TJS_INTF_METHOD  Release(void)
	{
		return 1;
/*
		if(RefCount == 1)
		{
			delete this;
			return 0;
		}
		else
		{
			RefCount--;
		}
		return RefCount;
*/
	}

//--
#define OBJ1 ((objthis)?(objthis):(Dispatch1))
#define OBJ2 ((objthis)?(objthis):(Dispatch2))

	tjs_error TJS_INTF_METHOD
	FuncCall(tjs_uint32 flag, const tjs_char * membername, tjs_uint32 *hint,
	tTJSVariant *result,
		tjs_int numparams, tTJSVariant **param, iTJSDispatch2 *objthis)
	{
		tjs_error hr =
			Dispatch1->FuncCall(flag, membername, hint, result, numparams, param, OBJ1);
		if(hr == TJS_E_MEMBERNOTFOUND && Dispatch1 != Dispatch2)
			return Dispatch2->FuncCall(flag, membername, hint, result, numparams, param, OBJ2);
		return hr;
	}

	tjs_error TJS_INTF_METHOD
	FuncCallByNum(tjs_uint32 flag, tjs_int num, tTJSVariant *result,
		tjs_int numparams, tTJSVariant **param, iTJSDispatch2 *objthis)
	{
		tjs_error hr =
			Dispatch1->FuncCallByNum(flag, num, result, numparams, param, OBJ1);
		if(hr == TJS_E_MEMBERNOTFOUND && Dispatch1 != Dispatch2)
			return Dispatch2->FuncCallByNum(flag, num, result, numparams, param, OBJ2);
		return hr;
	}

	tjs_error TJS_INTF_METHOD
	PropGet(tjs_uint32 flag, const tjs_char * membername, tjs_uint32 *hint,
	tTJSVariant *result,
		iTJSDispatch2 *objthis)
	{
		tjs_error hr =
			Dispatch1->PropGet(flag, membername, hint, result, OBJ1);
		if(hr == TJS_E_MEMBERNOTFOUND && Dispatch1 != Dispatch2)
			return Dispatch2->PropGet(flag, membername, hint, result, OBJ2);
		return hr;
	}

	tjs_error TJS_INTF_METHOD
	PropGetByNum(tjs_uint32 flag, tjs_int num, tTJSVariant *result,
		iTJSDispatch2 *objthis)
	{
		tjs_error hr =
			Dispatch1->PropGetByNum(flag, num, result, OBJ1);
		if(hr == TJS_E_MEMBERNOTFOUND && Dispatch1 != Dispatch2)
			return Dispatch2->PropGetByNum(flag, num, result, OBJ2);
		return hr;
	}

	tjs_error TJS_INTF_METHOD
	PropSet(tjs_uint32 flag, const tjs_char *membername, tjs_uint32 *hint,
	const tTJSVariant *param,
		iTJSDispatch2 *objthis)
	{
		tjs_error hr =
			Dispatch1->PropSet(flag, membername, hint, param, OBJ1);
		if(hr == TJS_E_MEMBERNOTFOUND && Dispatch1 != Dispatch2)
			return Dispatch2->PropSet(flag, membername, hint, param, OBJ2);
		return hr;
	}

	tjs_error TJS_INTF_METHOD
	PropSetByNum(tjs_uint32 flag, tjs_int num, const tTJSVariant *param,
		iTJSDispatch2 *objthis)
	{
		tjs_error hr =
			Dispatch1->PropSetByNum(flag, num, param, OBJ1);
		if(hr == TJS_E_MEMBERNOTFOUND && Dispatch1 != Dispatch2)
			return Dispatch2->PropSetByNum(flag, num, param, OBJ2);
		return hr;
	}

	tjs_error TJS_INTF_METHOD
	GetCount(tjs_int *result, const tjs_char *membername, tjs_uint32 *hint,
	iTJSDispatch2 *objthis)
	{
		tjs_error hr =
			Dispatch1->GetCount(result, membername, hint, OBJ1);
		if(hr == TJS_E_MEMBERNOTFOUND && Dispatch1 != Dispatch2)
			return Dispatch2->GetCount(result, membername, hint, OBJ2);
		return hr;
	}

	tjs_error TJS_INTF_METHOD
	GetCountByNum(tjs_int *result, tjs_int num, iTJSDispatch2 *objthis)
	{
		tjs_error hr =
			Dispatch1->GetCountByNum(result, num, OBJ1);
		if(hr == TJS_E_MEMBERNOTFOUND && Dispatch1 != Dispatch2)
			return Dispatch2->GetCountByNum(result, num, OBJ2);
		return hr;
	}

	tjs_error TJS_INTF_METHOD
	PropSetByVS(tjs_uint32 flag, tTJSVariantString *membername,
		const tTJSVariant *param, iTJSDispatch2 *objthis)
	{
		tjs_error hr =
			Dispatch1->PropSetByVS(flag, membername, param, OBJ1);
		if(hr == TJS_E_MEMBERNOTFOUND && Dispatch1 != Dispatch2)
			return Dispatch2->PropSetByVS(flag, membername, param, OBJ2);
		return hr;
	}

	tjs_error TJS_INTF_METHOD
	EnumMembers(tjs_uint32 flag, tTJSVariantClosure *callback, iTJSDispatch2 *objthis)
	{
		return TJS_E_NOTIMPL;
	}

	tjs_error TJS_INTF_METHOD
	DeleteMember(tjs_uint32 flag, const tjs_char *membername, tjs_uint32 *hint,
	iTJSDispatch2 *objthis)
	{
		tjs_error hr =
			Dispatch1->DeleteMember(flag, membername, hint, OBJ1);
		if(hr == TJS_E_MEMBERNOTFOUND && Dispatch1 != Dispatch2)
			return Dispatch2->DeleteMember(flag, membername, hint, OBJ2);
		return hr;
	}

	tjs_error TJS_INTF_METHOD
	DeleteMemberByNum(tjs_uint32 flag, tjs_int num, iTJSDispatch2 *objthis)
	{
		tjs_error hr =
			Dispatch1->DeleteMemberByNum(flag, num, OBJ1);
		if(hr == TJS_E_MEMBERNOTFOUND && Dispatch1 != Dispatch2)
			return Dispatch2->DeleteMemberByNum(flag, num, OBJ2);
		return hr;
	}

	tjs_error TJS_INTF_METHOD
	Invalidate(tjs_uint32 flag, const tjs_char *membername, tjs_uint32 *hint,
		iTJSDispatch2 *objthis)
	{
		tjs_error hr =
			Dispatch1->Invalidate(flag, membername, hint, OBJ1);
		if(hr == TJS_E_MEMBERNOTFOUND && Dispatch1 != Dispatch2)
			return Dispatch2->Invalidate(flag, membername, hint, OBJ2);
		return hr;
	}

	tjs_error TJS_INTF_METHOD
	InvalidateByNum(tjs_uint32 flag, tjs_int num, iTJSDispatch2 *objthis)
	{
		tjs_error hr =
			Dispatch1->InvalidateByNum(flag, num, OBJ1);
		if(hr == TJS_E_MEMBERNOTFOUND && Dispatch1 != Dispatch2)
			return Dispatch2->InvalidateByNum(flag, num, OBJ2);
		return hr;
	}

	tjs_error TJS_INTF_METHOD
	IsValid(tjs_uint32 flag, const tjs_char *membername, tjs_uint32 *hint,
	iTJSDispatch2 *objthis)
	{
		tjs_error hr =
			Dispatch1->IsValid(flag, membername, hint, OBJ1);
		if(hr == TJS_E_MEMBERNOTFOUND && Dispatch1 != Dispatch2)
			return Dispatch2->IsValid(flag, membername, hint, OBJ2);
		return hr;
	}

	tjs_error TJS_INTF_METHOD
	IsValidByNum(tjs_uint32 flag, tjs_int num, iTJSDispatch2 *objthis)
	{
		tjs_error hr =
			Dispatch1->IsValidByNum(flag, num, OBJ1);
		if(hr == TJS_E_MEMBERNOTFOUND && Dispatch1 != Dispatch2)
			return Dispatch2->IsValidByNum(flag, num, OBJ2);
		return hr;
	}

	tjs_error TJS_INTF_METHOD
	CreateNew(tjs_uint32 flag, const tjs_char * membername, tjs_uint32 *hint,
	iTJSDispatch2 **result,
		tjs_int numparams, tTJSVariant **param,	iTJSDispatch2 *objthis)
	{
		tjs_error hr =
			Dispatch1->CreateNew(flag, membername, hint, result, numparams, param, OBJ1);
		if(hr == TJS_E_MEMBERNOTFOUND && Dispatch1 != Dispatch2)
			return Dispatch2->CreateNew(flag, membername, hint, result, numparams, param, OBJ2);
		return hr;
	}

	tjs_error TJS_INTF_METHOD
	CreateNewByNum(tjs_uint32 flag, tjs_int num, iTJSDispatch2 **result,
		tjs_int numparams, tTJSVariant **param,	iTJSDispatch2 *objthis)
	{
		tjs_error hr =
			Dispatch1->CreateNewByNum(flag, num, result, numparams, param, OBJ1);
		if(hr == TJS_E_MEMBERNOTFOUND && Dispatch1 != Dispatch2)
			return Dispatch2->CreateNewByNum(flag, num, result, numparams, param, OBJ2);
		return hr;
	}

	tjs_error TJS_INTF_METHOD
	Reserved1()
	{
		return TJS_E_NOTIMPL;
	}

	tjs_error TJS_INTF_METHOD
	IsInstanceOf(tjs_uint32 flag, const tjs_char *membername, tjs_uint32 *hint,
	const tjs_char *classname,
		iTJSDispatch2 *objthis)
	{
		tjs_error hr =
			Dispatch1->IsInstanceOf(flag, membername, hint, classname, OBJ1);
		if(hr == TJS_E_MEMBERNOTFOUND && Dispatch1 != Dispatch2)
			return Dispatch2->IsInstanceOf(flag, membername, hint, classname, OBJ2);
		return hr;
	}

	tjs_error TJS_INTF_METHOD
	IsInstanceOfByNum(tjs_uint32 flag, tjs_int num, const tjs_char *classname,
		iTJSDispatch2 *objthis)
	{
		tjs_error hr =
			Dispatch1->IsInstanceOfByNum(flag, num, classname, OBJ1);
		if(hr == TJS_E_MEMBERNOTFOUND && Dispatch1 != Dispatch2)
			return Dispatch2->IsInstanceOfByNum(flag, num, classname, OBJ2);
		return hr;
	}

	tjs_error TJS_INTF_METHOD
	Operation(tjs_uint32 flag, const tjs_char *membername, tjs_uint32 *hint,
	tTJSVariant *result,
		const tTJSVariant *param,	iTJSDispatch2 *objthis)
	{
		tjs_error hr =
			Dispatch1->Operation(flag, membername, hint, result, param, OBJ1);
		if(hr == TJS_E_MEMBERNOTFOUND && Dispatch1 != Dispatch2)
			return Dispatch2->Operation(flag, membername, hint, result, param, OBJ2);
		return hr;
	}

	tjs_error TJS_INTF_METHOD
	OperationByNum(tjs_uint32 flag, tjs_int num, tTJSVariant *result,
		const tTJSVariant *param,	iTJSDispatch2 *objthis)
	{
		tjs_error hr =
			Dispatch1->OperationByNum(flag, num, result, param, OBJ1);
		if(hr == TJS_E_MEMBERNOTFOUND && Dispatch1 != Dispatch2)
			return Dispatch2->OperationByNum(flag, num, result, param, OBJ2);
		return hr;
	}

	tjs_error TJS_INTF_METHOD
	NativeInstanceSupport(tjs_uint32 flag, tjs_int32 classid,
		iTJSNativeInstance **pointer)  { return TJS_E_NOTIMPL; }

	tjs_error TJS_INTF_METHOD
	ClassInstanceInfo(tjs_uint32 flag, tjs_uint num, tTJSVariant *value)
		{ return TJS_E_NOTIMPL;	}

	tjs_error TJS_INTF_METHOD
	Reserved2()
	{
		return TJS_E_NOTIMPL;
	}


	tjs_error TJS_INTF_METHOD
	Reserved3()
	{
		return TJS_E_NOTIMPL;
	}

};
#undef OBJ1
#undef OBJ2

//---------------------------------------------------------------------------
// tTJSVariantArrayStack
//---------------------------------------------------------------------------
// TODO: adjust TJS_VA_ONE_ALLOC_MIN
#define TJS_VA_ONE_ALLOC_MAX 1024
#define TJS_COMPACT_FREQ 10000
static tjs_int TJSCompactVariantArrayMagic = 0;
static std::mutex TJSVariantArrayStackMutex;
static std::set<tTJSVariantArrayStack*> TJSVariantArrayStacks;
//---------------------------------------------------------------------------
tTJSVariantArrayStack::tTJSVariantArrayStack()
{
	NumArraysAllocated = NumArraysUsing = 0;
	Arrays = NULL;
	Current = NULL;
	OperationDisabledCount = 0;
	CompactVariantArrayMagic = TJSCompactVariantArrayMagic;
	std::lock_guard<std::mutex> lk(TJSVariantArrayStackMutex);
	TJSVariantArrayStacks.insert(this);
}
//---------------------------------------------------------------------------
tTJSVariantArrayStack::~tTJSVariantArrayStack()
{
	OperationDisabledCount++;
	tjs_int i;
	for(i = 0; i<NumArraysAllocated; i++)
	{
		delete [] Arrays[i].Array;
	}
	TJS_free(Arrays), Arrays = NULL;
	std::lock_guard<std::mutex> lk(TJSVariantArrayStackMutex);
	TJSVariantArrayStacks.erase(this);
}
//---------------------------------------------------------------------------
void tTJSVariantArrayStack::IncreaseVariantArray(tjs_int num)
{
	// increase array block
	NumArraysUsing++;
	if(NumArraysUsing > NumArraysAllocated)
	{
		Arrays = (tVariantArray*)
			TJS_realloc(Arrays, sizeof(tVariantArray)*(NumArraysUsing));
		NumArraysAllocated = NumArraysUsing;
		Current = Arrays + NumArraysUsing -1;
		Current->Array = new tTJSVariant[num];
	}
	else
	{
		Current = Arrays + NumArraysUsing -1;
	}

	Current->Allocated = num;
	Current->Using = 0;
}
//---------------------------------------------------------------------------
void tTJSVariantArrayStack::DecreaseVariantArray(void)
{
	// decrease array block
	NumArraysUsing--;
	if(NumArraysUsing == 0)
		Current = NULL;
	else
		Current = Arrays + NumArraysUsing-1;
}
//---------------------------------------------------------------------------
void tTJSVariantArrayStack::InternalCompact(void)
{
	// minimize variant array block
	OperationDisabledCount++;
	try
	{
		while(NumArraysAllocated > NumArraysUsing)
		{
			NumArraysAllocated --;
			delete [] Arrays[NumArraysAllocated].Array;
		}

		if(Current)
		{
			for(tjs_int i = Current->Using; i < Current->Allocated; i++)
				Current->Array[i].Clear();
		}

		if(NumArraysUsing == 0)
		{
			if(Arrays) TJS_free(Arrays), Arrays = NULL;
			Current = NULL;
		}
		else
		{
			bool availableoffset = false;
			size_t offset = 0;
			if( Current != NULL && Arrays != NULL ) {
				offset = (size_t)Current - (size_t)Arrays;
				availableoffset = true;
			}

			tVariantArray* arraytmp = (tVariantArray*)
				TJS_realloc(Arrays, sizeof(tVariantArray)*(NumArraysUsing));
			if( arraytmp != NULL ) {
				Arrays = arraytmp;
			} else if( NumArraysUsing > 0 ) {
				TJS_eTJSError(TJSInternalError);
			}

			if( availableoffset && Arrays != NULL ) {
				Current = Arrays + offset;
			} else {
				Current = NULL;
			}
		}
	}
	catch(...)
	{
		OperationDisabledCount--;
		throw;
	}
	OperationDisabledCount--;
}
//---------------------------------------------------------------------------
inline tTJSVariant * tTJSVariantArrayStack::Allocate(tjs_int num)
{
//		tTJSCSH csh(CS);

	if(!OperationDisabledCount && num < TJS_VA_ONE_ALLOC_MAX)
	{
		if(!Current || Current->Using + num > Current->Allocated)
		{
			IncreaseVariantArray( TJS_VA_ONE_ALLOC_MAX );
		}
		tTJSVariant *ret = Current->Array + Current->Using;
		Current->Using += num;
		return ret;
	}
	else
	{
		return new tTJSVariant[num];
	}
}
//---------------------------------------------------------------------------
inline void tTJSVariantArrayStack::Deallocate(tjs_int num, tTJSVariant *ptr)
{
//		tTJSCSH csh(CS);

	if(!OperationDisabledCount && num < TJS_VA_ONE_ALLOC_MAX)
	{
		Current->Using -= num;
		if(Current->Using == 0)
		{
			DecreaseVariantArray();
		}
	}
	else
	{
		delete [] ptr;
	}

	if(!OperationDisabledCount)
	{
		if(CompactVariantArrayMagic != TJSCompactVariantArrayMagic)
		{
			Compact();
			CompactVariantArrayMagic = TJSCompactVariantArrayMagic;
		}
	}
}
//---------------------------------------------------------------------------
//static tjs_int TJSVariantArrayStackRefCount = 0;
//---------------------------------------------------------------------------
void tTJSInterCodeContext::TJSVariantArrayStackAddRef()
{
}
//---------------------------------------------------------------------------
void tTJSInterCodeContext::TJSVariantArrayStackRelease()
{
}
//---------------------------------------------------------------------------
void TJSVariantArrayStackCompact()
{
	TJSCompactVariantArrayMagic++;
}
//---------------------------------------------------------------------------
void TJSVariantArrayStackCompactNow()
{
#if 0 // due to multi-thread
	for (tTJSVariantArrayStack* stk : TJSVariantArrayStacks) {
		stk->Compact();
	}
#endif
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// tTJSInterCodeContext ( class definitions are in tjsInterCodeGen.h )
//---------------------------------------------------------------------------
void tTJSInterCodeContext::ExecuteAsFunction(iTJSDispatch2 *objthis,
	tTJSVariant **args, tjs_int numargs, tTJSVariant *result, tjs_int start_ip)
{
	tjs_int num_alloc = MaxVariableCount + VariableReserveCount + 1 + MaxFrameCount;
	TJSVariantArrayStackAddRef();
//	AddRef();
//	if(objthis) objthis->AddRef();
	try
	{
		tTJSVariant *regs =
			TJSVariantArrayStack->Allocate(num_alloc);
		tTJSVariant *ra = regs + MaxVariableCount + VariableReserveCount; // register area

		// objthis-proxy

		tTJSObjectProxy proxy;
		if(objthis)
		{
			proxy.SetObjects(objthis, Block->GetTJS()->GetGlobalNoAddRef());
			// TODO: caching of objthis-proxy

			ra[-2] = &proxy;
		}
		else
		{
			proxy.SetObjects(NULL, NULL);

			iTJSDispatch2 *global = Block->GetTJS()->GetGlobalNoAddRef();

			ra[-2].SetObject(global, global);
		}

/*
		if(objthis)
		{
			// TODO: caching of objthis-proxy
			tTJSObjectProxy *proxy = new tTJSObjectProxy();
			proxy->SetObjects(objthis, Block->GetTJS()->GetGlobalNoAddRef());

			ra[-2] = proxy;

			proxy->Release();
		}
		else
		{
			iTJSDispatch2 *global = Block->GetTJS()->GetGlobalNoAddRef();

			ra[-2].SetObject(global, global);
		}
*/
		if(TJSStackTracerEnabled()) TJSStackTracerPush(this, false);

		// check whether the objthis is deleting
		if(TJSWarnOnExecutionOnDeletingObject && TJSObjectFlagEnabled() &&
			Block->GetTJS()->GetConsoleOutput())
			TJSWarnIfObjectIsDeleting(Block->GetTJS()->GetConsoleOutput(), objthis);

#ifdef ENABLE_DEBUGGER
		ScopeKey oldkey;
		tTJSVariant* oldra = NULL;
#endif	// ENABLE_DEBUGGER
		try
		{
			ra[-1].SetObject(objthis, objthis);
			ra[0].Clear();

			// transfer arguments
			if(numargs >= FuncDeclArgCount)
			{
				// given arguments are greater than or equal to desired arguments
				if(FuncDeclArgCount)
				{
					tTJSVariant *r = ra - 3;
					tTJSVariant **a = args;
					tjs_int n = FuncDeclArgCount;
					while(true)
					{
						*r = **(a++);
						n--;
						if(!n) break;
						r--;
					}
				}
			}
			else
			{
				// given arguments are less than desired arguments
				tTJSVariant *r = ra - 3;
				tTJSVariant **a = args;
				tjs_int i;
				for(i = 0; i<numargs; i++) *(r--) = **(a++);
				for(; i<FuncDeclArgCount; i++) (r--)->Clear();
			}

			// collapse into array when FuncDeclCollapseBase >= 0
			if(FuncDeclCollapseBase >= 0)
			{
				tTJSVariant *r = ra - 3 - FuncDeclCollapseBase; // target variant
				iTJSDispatch2 * dsp = TJSCreateArrayObject();
				*r = tTJSVariant(dsp, dsp);
				dsp->Release();

				if(numargs > FuncDeclCollapseBase)
				{
					// there are arguments to store
					for(tjs_int c = 0, i = FuncDeclCollapseBase; i < numargs; i++, c++)
						dsp->PropSetByNum(0, c, args[i], dsp);
				}
			}

#ifdef ENABLE_DEBUGGER
			if( TJSEnableDebugMode && ::IsDebuggerPresent() ) {
				tjs_int ine_no = Block->SrcPosToLine( CodePosToSrcPos(start_ip) );
				TJSDebuggerHook( DBGHOOK_PREV_CALL, Block->GetName(), ine_no );
			}
			oldkey = DebuggerScopeKey;
			oldra = DebuggerRegisterArea;
			TJSDebuggerGetScopeKey( DebuggerScopeKey, GetClassName().c_str(), GetName(), Block->GetName(), start_ip );
			DebuggerRegisterArea = ra;
#endif	// ENABLE_DEBUGGER
			// execute
			ExecuteCode(ra, start_ip, args, numargs, result);
		}
		catch(...)
		{
#ifdef ENABLE_DEBUGGER
			// Œ³‚É–ß‚·
			DebuggerScopeKey = oldkey;
			DebuggerRegisterArea = oldra;
#endif	// ENABLE_DEBUGGER
#if 0
			for(tjs_int i=0; i<num_alloc; i++) regs[i].Clear();
#endif
			ra[-2].Clear(); // at least we must clear the object placed at local stack
			TJSVariantArrayStack->Deallocate(num_alloc, regs);
			if(TJSStackTracerEnabled()) TJSStackTracerPop();
			throw;
		}

#ifdef ENABLE_DEBUGGER
		// Œ³‚É–ß‚·
		DebuggerScopeKey = oldkey;
		DebuggerRegisterArea = oldra;
#endif	// ENABLE_DEBUGGER
#if 0
		for(tjs_int i=0; i<MaxVariableCount + VariableReserveCount; i++)
			regs[i].Clear();
#endif
		ra[-2].Clear(); // at least we must clear the object placed at local stack

		TJSVariantArrayStack->Deallocate(num_alloc, regs);

		if(TJSStackTracerEnabled()) TJSStackTracerPop();
	}
	catch(...)
	{
//		if(objthis) objthis->Release();
//		Release();
		TJSVariantArrayStackRelease();
		throw;
	}
//	if(objthis) objthis->Release();
//	Release();
	TJSVariantArrayStackRelease();
}
//---------------------------------------------------------------------------
void tTJSInterCodeContext::DisplayExceptionGeneratedCode(tjs_int codepos,
	const tTJSVariant *ra)
{
	tTJS *tjs = Block->GetTJS();
	ttstr info(
		TJS_W("==== An exception occured at ") +
		GetPositionDescriptionString(codepos) +
		TJS_W(", VM ip = ") + ttstr(codepos) + TJS_W(" ===="));
	tjs_int info_len = info.GetLen();

	tjs->OutputToConsole(info.c_str());
	tjs->OutputToConsole(TJS_W("-- Disassembled VM code --"));
	DisassembleSrcLine(codepos);

	tjs->OutputToConsole(TJS_W("-- Register dump --"));

	const tTJSVariant *ra_start = ra - (MaxVariableCount + VariableReserveCount);
	tjs_int ra_count = MaxVariableCount + VariableReserveCount + 1 + MaxFrameCount;
	ttstr line;
	for(tjs_int i = 0; i < ra_count; i ++)
	{
		ttstr reg_info = TJS_W("%") + ttstr(i - (MaxVariableCount + VariableReserveCount))
			+ TJS_W("=") + TJSVariantToReadableString(ra_start[i]);
		if(line.GetLen() + reg_info.GetLen() + 2 > info_len)
		{
			tjs->OutputToConsole(line.c_str());
			line = reg_info;
		}
		else
		{
			if(!line.IsEmpty()) line += TJS_W("  ");
			line += reg_info;
		}
	}

	if(!line.IsEmpty())
	{
		tjs->OutputToConsole(line.c_str());
	}

	tjs->OutputToConsoleSeparator(TJS_W("-"), info_len);
}
//---------------------------------------------------------------------------
void tTJSInterCodeContext::ThrowScriptException(tTJSVariant &val,
	tTJSScriptBlock *block, tjs_int srcpos)
{
	tTJSString msg;
	if(val.Type() == tvtObject)
	{
		try
		{
			tTJSVariantClosure clo = val.AsObjectClosureNoAddRef();
			if(clo.Object != NULL)
			{
				tTJSVariant v2;
				static tTJSString message_name(TJS_W("message"));
				tjs_error hr = clo.PropGet(0, message_name.c_str(),
					message_name.GetHint(), &v2, NULL);
				if(TJS_SUCCEEDED(hr))
				{
					msg = ttstr(TJS_W("script exception : ")) + ttstr(v2);
				}
			}
		}
		catch(...)
		{
		}
	}

	if(msg.IsEmpty())
	{
		msg = TJS_W("script exception");
	}

	TJS_eTJSScriptException(msg, this, srcpos, val);
}
//---------------------------------------------------------------------------
tjs_int tTJSInterCodeContext::ExecuteCode(tTJSVariant *ra_org, tjs_int startip,
	tTJSVariant **args, tjs_int numargs, tTJSVariant *result)
{
	// execute VM codes
	tjs_int32 *codesave;
	try
	{
		tjs_int32 *code = codesave = CodeArea + startip;

		if(TJSStackTracerEnabled()) TJSStackTracerSetCodePointer(CodeArea, &codesave);
#ifdef ENABLE_DEBUGGER
		bool is_enable_debugger = false;
		if( TJSEnableDebugMode && ::IsDebuggerPresent() ) {
			is_enable_debugger = true;
		}
#endif	// ENABLE_DEBUGGER

		tTJSVariant *ra = ra_org;
		tTJSVariant *da = DataArea;

		bool flag = false;

#ifdef ENABLE_DEBUGGER
		tjs_int cur_line_no = -1;
#endif	// ENABLE_DEBUGGER
		while(true)
		{
#ifdef ENABLE_DEBUGGER
			if( is_enable_debugger ) {
				tjs_int next_line_no = Block->SrcPosToLine( CodePosToSrcPos(code-CodeArea) );
				if( cur_line_no != next_line_no ) {
					cur_line_no = next_line_no;
					TJSDebuggerHook( DBGHOOK_PREV_EXE_LINE, Block->GetName(), cur_line_no, this );
				}
			}
#endif	// ENABLE_DEBUGGER
			codesave = code;
			switch(*code)
			{
			case VM_NOP:
				code ++;
				break;

			case VM_CONST:
				TJS_GET_VM_REG(ra, code[1]).CopyRef(TJS_GET_VM_REG(da, code[2]));
				code += 3;
				break;

			case VM_CP:
				TJS_GET_VM_REG(ra, code[1]).CopyRef(TJS_GET_VM_REG(ra, code[2]));
				code += 3;
				break;

			case VM_CL:
				TJS_GET_VM_REG(ra, code[1]).Clear();
				code += 2;
				break;

			case VM_CCL:
				ContinuousClear(ra, code);
				code += 3;
				break;

			case VM_TT:
				flag = TJS_GET_VM_REG(ra, code[1]).operator bool();
				code += 2;
				break;

			case VM_TF:
				flag = !(TJS_GET_VM_REG(ra, code[1]).operator bool());
				code += 2;
				break;

			case VM_CEQ:
				flag = TJS_GET_VM_REG(ra, code[1]).NormalCompare(
					TJS_GET_VM_REG(ra, code[2]));
				code += 3;
				break;

			case VM_CDEQ:
				flag = TJS_GET_VM_REG(ra, code[1]).DiscernCompare(
					TJS_GET_VM_REG(ra, code[2]));
				code += 3;
				break;

			case VM_CLT:
				flag = TJS_GET_VM_REG(ra, code[1]).GreaterThan(
					TJS_GET_VM_REG(ra, code[2]));
				code += 3;
				break;

			case VM_CGT:
				flag = TJS_GET_VM_REG(ra, code[1]).LittlerThan(
					TJS_GET_VM_REG(ra, code[2]));
				code += 3;
				break;

			case VM_SETF:
				TJS_GET_VM_REG(ra, code[1]) = flag;
				code += 2;
				break;

			case VM_SETNF:
				TJS_GET_VM_REG(ra, code[1]) = !flag;
				code += 2;
				break;

			case VM_LNOT:
				TJS_GET_VM_REG(ra, code[1]).logicalnot();
				code += 2;
				break;

			case VM_NF:
				flag = !flag;
				code ++;
				break;

			case VM_JF:
				if(flag)
					TJS_ADD_VM_CODE_ADDR(code, code[1]);
				else
					code += 2;
				break;

			case VM_JNF:
				if(!flag)
					TJS_ADD_VM_CODE_ADDR(code, code[1]);
				else
					code += 2;
				break;

			case VM_JMP:
				TJS_ADD_VM_CODE_ADDR(code, code[1]);
				break;

			case VM_INC:
				TJS_GET_VM_REG(ra, code[1]).increment();
				code += 2;
				break;

			case VM_INCPD:
				OperatePropertyDirect0(ra, code, TJS_OP_INC);
				code += 4;
				break;

			case VM_INCPI:
				OperatePropertyIndirect0(ra, code, TJS_OP_INC);
				code += 4;
				break;

			case VM_INCP:
				OperateProperty0(ra, code, TJS_OP_INC);
				code += 3;
				break;

			case VM_DEC:
				TJS_GET_VM_REG(ra, code[1]).decrement();
				code += 2;
				break;

			case VM_DECPD:
				OperatePropertyDirect0(ra, code, TJS_OP_DEC);
				code += 4;
				break;

			case VM_DECPI:
				OperatePropertyIndirect0(ra, code, TJS_OP_DEC);
				code += 4;
				break;

			case VM_DECP:
				OperateProperty0(ra, code, TJS_OP_DEC);
				code += 3;
				break;

#define TJS_DEF_VM_P(vmcode, rope) \
			case VM_##vmcode: \
				TJS_GET_VM_REG(ra, code[1]).rope(TJS_GET_VM_REG(ra, code[2])); \
				code += 3; \
				break; \
			case VM_##vmcode##PD: \
				OperatePropertyDirect(ra, code, TJS_OP_##vmcode); \
				code += 5; \
				break; \
			case VM_##vmcode##PI: \
				OperatePropertyIndirect(ra, code, TJS_OP_##vmcode); \
				code += 5; \
				break; \
			case VM_##vmcode##P: \
				OperateProperty(ra, code, TJS_OP_##vmcode); \
				code += 4; \
				break

				TJS_DEF_VM_P(LOR, logicalorequal);
				TJS_DEF_VM_P(LAND, logicalandequal);
				TJS_DEF_VM_P(BOR, operator |=);
				TJS_DEF_VM_P(BXOR, operator ^=);
				TJS_DEF_VM_P(BAND, operator &=);
				TJS_DEF_VM_P(SAR, operator >>=);
				TJS_DEF_VM_P(SAL, operator <<=);
				TJS_DEF_VM_P(SR, rbitshiftequal);
				TJS_DEF_VM_P(ADD, operator +=);
				TJS_DEF_VM_P(SUB, operator -=);
				TJS_DEF_VM_P(MOD, operator %=);
				TJS_DEF_VM_P(DIV, operator /=);
				TJS_DEF_VM_P(IDIV, idivequal);
				TJS_DEF_VM_P(MUL, operator *=);

#undef TJS_DEF_VM_P

			case VM_BNOT:
				TJS_GET_VM_REG(ra, code[1]).bitnot();
				code += 2;
				break;

			case VM_ASC:
				CharacterCodeOf(TJS_GET_VM_REG(ra, code[1]));
				code += 2;
				break;

			case VM_CHR:
				CharacterCodeFrom(TJS_GET_VM_REG(ra, code[1]));
				code += 2;
				break;

			case VM_NUM:
				TJS_GET_VM_REG(ra, code[1]).tonumber();
				code += 2;
				break;

			case VM_CHS:
				TJS_GET_VM_REG(ra, code[1]).changesign();
				code += 2;
				break;

			case VM_INV:
				TJS_GET_VM_REG(ra, code[1]) =
					TJS_GET_VM_REG(ra, code[1]).Type() != tvtObject ? false :
					(TJS_GET_VM_REG(ra, code[1]).AsObjectClosureNoAddRef().Invalidate(0,
					NULL, NULL, ra[-1].AsObjectNoAddRef()) == TJS_S_TRUE);
				code += 2;
				break;

			case VM_CHKINV:
				TJS_GET_VM_REG(ra, code[1]) =
					TJS_GET_VM_REG(ra, code[1]).Type() != tvtObject ? true :
					TJSIsObjectValid(TJS_GET_VM_REG(ra, code[1]).AsObjectClosureNoAddRef().IsValid(0,
					NULL, NULL, ra[-1].AsObjectNoAddRef()));
				code += 2;
				break;

			case VM_INT:
				TJS_GET_VM_REG(ra, code[1]).ToInteger();
				code += 2;
				break;

			case VM_REAL:
				TJS_GET_VM_REG(ra, code[1]).ToReal();
				code += 2;
				break;

			case VM_STR:
				TJS_GET_VM_REG(ra, code[1]).ToString();
				code += 2;
				break;

			case VM_OCTET:
				TJS_GET_VM_REG(ra, code[1]).ToOctet();
				code += 2;
				break;

			case VM_TYPEOF:
				TypeOf(TJS_GET_VM_REG(ra, code[1]));
				code += 2;
				break;

			case VM_TYPEOFD:
				TypeOfMemberDirect(ra, code, TJS_MEMBERMUSTEXIST);
				code += 4;
				break;

			case VM_TYPEOFI:
				TypeOfMemberIndirect(ra, code, TJS_MEMBERMUSTEXIST);
				code += 4;
				break;

			case VM_EVAL:
				Eval(TJS_GET_VM_REG(ra, code[1]),
					TJSEvalOperatorIsOnGlobal ? NULL : ra[-1].AsObjectNoAddRef(),
					true);
				code += 2;
				break;

			case VM_EEXP:
				Eval(TJS_GET_VM_REG(ra, code[1]),
					TJSEvalOperatorIsOnGlobal ? NULL : ra[-1].AsObjectNoAddRef(),
					false);
				code += 2;
				break;

			case VM_CHKINS:
				InstanceOf(TJS_GET_VM_REG(ra, code[2]),
					TJS_GET_VM_REG(ra, code[1]));
				code += 3;
				break;

			case VM_CALL:
			case VM_NEW:
				code += CallFunction(ra, code, args, numargs);
				break;

			case VM_CALLD:
				code += CallFunctionDirect(ra, code, args, numargs);
				break;

			case VM_CALLI:
				code += CallFunctionIndirect(ra, code, args, numargs);
				break;

			case VM_GPD:
				GetPropertyDirect(ra, code, 0);
				code += 4;
				break;

			case VM_GPDS:
				GetPropertyDirect(ra, code, TJS_IGNOREPROP);
				code += 4;
				break;

			case VM_SPD:
				SetPropertyDirect(ra, code, 0);
				code += 4;
				break;

			case VM_SPDE:
				SetPropertyDirect(ra, code, TJS_MEMBERENSURE);
				code += 4;
				break;

			case VM_SPDEH:
				SetPropertyDirect(ra, code, TJS_MEMBERENSURE|TJS_HIDDENMEMBER);
				code += 4;
				break;

			case VM_SPDS:
				SetPropertyDirect(ra, code, TJS_MEMBERENSURE|TJS_IGNOREPROP);
				code += 4;
				break;

			case VM_GPI:
				GetPropertyIndirect(ra, code, 0);
				code += 4;
				break;

			case VM_GPIS:
				GetPropertyIndirect(ra, code, TJS_IGNOREPROP);
				code += 4;
				break;

			case VM_SPI:
				SetPropertyIndirect(ra, code, 0);
				code += 4;
				break;

			case VM_SPIE:
				SetPropertyIndirect(ra, code, TJS_MEMBERENSURE);
				code += 4;
				break;

			case VM_SPIS:
				SetPropertyIndirect(ra, code, TJS_MEMBERENSURE|TJS_IGNOREPROP);
				code += 4;
				break;

			case VM_GETP:
				GetProperty(ra, code);
				code += 3;
				break;

			case VM_SETP:
				SetProperty(ra, code);
				code += 3;
				break;

			case VM_DELD:
				DeleteMemberDirect(ra, code);
				code += 4;
				break;

			case VM_DELI:
				DeleteMemberIndirect(ra, code);
				code += 4;
				break;

			case VM_SRV:
				if(result) result->CopyRef(TJS_GET_VM_REG(ra, code[1]));
				code += 2;
				break;

			case VM_RET:
#ifdef ENABLE_DEBUGGER
				if( is_enable_debugger ) {
					TJSDebuggerHook( DBGHOOK_PREV_RETURN, Block->GetName(), cur_line_no, this );
					cur_line_no = -1;
				}
#endif	// ENABLE_DEBUGGER
				return (tjs_int)(code+1-CodeArea);

			case VM_ENTRY:
				code = CodeArea + ExecuteCodeInTryBlock(ra, (tjs_int)(code-CodeArea + 3), args,
					numargs, result, (tjs_int)(TJS_FROM_VM_CODE_ADDR(code[1])+code-CodeArea),
					TJS_FROM_VM_REG_ADDR(code[2]));
				break;

			case VM_EXTRY:
				return (tjs_int)(code+1-CodeArea);  // same as ret

			case VM_THROW:
#ifdef ENABLE_DEBUGGER
				if( is_enable_debugger ) {
					TJSDebuggerHook( DBGHOOK_PREV_EXCEPT, Block->GetName(), cur_line_no, this );
					cur_line_no = -1;
				}
#endif	// ENABLE_DEBUGGER
				ThrowScriptException(TJS_GET_VM_REG(ra, code[1]),
					Block, CodePosToSrcPos((tjs_int)(code-CodeArea)));
				code += 2; // actually here not proceed...
				break;

			case VM_CHGTHIS:
				TJS_GET_VM_REG(ra, code[1]).ChangeClosureObjThis(
					TJS_GET_VM_REG(ra, code[2]).AsObjectNoAddRef());
				code += 3;
				break;

			case VM_GLOBAL:
				TJS_GET_VM_REG(ra, code[1]) = Block->GetTJS()->GetGlobalNoAddRef();
				code += 2;
				break;

			case VM_ADDCI:
				AddClassInstanceInfo(ra, code);
				code+=3;
				break;

			case VM_REGMEMBER:
				RegisterObjectMember(ra[-1].AsObjectNoAddRef());
				code ++;
				break;

			case VM_DEBUGGER:
#ifdef ENABLE_DEBUGGER
				if( is_enable_debugger ) {
					TJSDebuggerHook( DBGHOOK_PREV_BREAK, Block->GetName(), cur_line_no, this );
					cur_line_no = -1;
				}
#else	// ENABLE_DEBUGGER
				TJSNativeDebuggerBreak();
#endif	// ENABLE_DEBUGGER
				code ++;
				break;

			default:
				ThrowInvalidVMCode();
			}
		}
	}
	catch(eTJSSilent &e)
	{
		throw e;
	}
#ifdef ENABLE_DEBUGGER
#define DEBUGGER_EXCEPTION_HOOK	if( TJSEnableDebugMode && ::IsDebuggerPresent() ) TJSDebuggerHook( DBGHOOK_PREV_EXCEPT, NULL, -1, this );
#else	// ENABLE_DEBUGGER
#define DEBUGGER_EXCEPTION_HOOK
#endif	// ENABLE_DEBUGGER
	catch(eTJSScriptException &e)
	{
		DEBUGGER_EXCEPTION_HOOK;
		e.AddTrace(this, (tjs_int)(codesave-CodeArea));
		throw e;
	}
	catch(eTJSScriptError &e)
	{
		DEBUGGER_EXCEPTION_HOOK;
		e.AddTrace(this, (tjs_int)(codesave-CodeArea));
		throw e;
	}
	catch(eTJS &e)
	{
		DEBUGGER_EXCEPTION_HOOK;
		DisplayExceptionGeneratedCode((tjs_int)(codesave - CodeArea), ra_org);
		TJS_eTJSScriptError(e.GetMessage(), this, (tjs_int)(codesave-CodeArea));
	}
	catch(exception &e)
	{
		DEBUGGER_EXCEPTION_HOOK;
		DisplayExceptionGeneratedCode((tjs_int)(codesave - CodeArea), ra_org);
		TJS_eTJSScriptError(e.what(), this, (tjs_int)(codesave-CodeArea));
	}
#if 0
	catch(const wchar_t *text)
	{
		DEBUGGER_EXCEPTION_HOOK;
		DisplayExceptionGeneratedCode((tjs_int)(codesave - CodeArea), ra_org);
		TJS_eTJSScriptError(text, this, (tjs_int)(codesave-CodeArea));
	}
#endif
	catch(const char *text)
	{
		DEBUGGER_EXCEPTION_HOOK;
		DisplayExceptionGeneratedCode((tjs_int)(codesave - CodeArea), ra_org);
		TJS_eTJSScriptError(text, this, (tjs_int)(codesave-CodeArea));
	}
#ifdef TJS_SUPPORT_VCL
	catch(const EAccessViolation &e)
	{
		DEBUGGER_EXCEPTION_HOOK;
		DisplayExceptionGeneratedCode(codesave - CodeArea, ra_org);
		TJS_eTJSScriptError(e.Message.c_str(), this, codesave-CodeArea);
	}
	catch(const Exception &e)
	{
		DEBUGGER_EXCEPTION_HOOK;
		DisplayExceptionGeneratedCode(codesave - CodeArea, ra_org);
		TJS_eTJSScriptError(e.Message.c_str(), this, codesave-CodeArea);
	}
#endif
#undef DEBUGGER_EXCEPTION_HOOK

	return (tjs_int)(codesave-CodeArea);
}
//---------------------------------------------------------------------------
tjs_int tTJSInterCodeContext::ExecuteCodeInTryBlock(tTJSVariant *ra, tjs_int startip,
	tTJSVariant **args, tjs_int numargs, tTJSVariant *result, tjs_int catchip,
	tjs_int exobjreg)
{
	// execute codes in a try-protected block

	try
	{
		if(TJSStackTracerEnabled()) TJSStackTracerPush(this, true);
		tjs_int ret;
		try
		{
			ret = ExecuteCode(ra, startip, args, numargs, result);
		}
		catch(...)
		{
			if(TJSStackTracerEnabled()) TJSStackTracerPop();
			throw;
		}
		if(TJSStackTracerEnabled()) TJSStackTracerPop();
		return ret;
	}
	TJS_CONVERT_TO_TJS_EXCEPTION_OBJECT(
			Block->GetTJS(),
			exobjreg,
			ra + exobjreg,
			{
				;
			},
			{
				return catchip;
			}
		)
}
//---------------------------------------------------------------------------
void tTJSInterCodeContext::ContinuousClear(
	tTJSVariant *ra, const tjs_int32 *code)
{
	tTJSVariant *r = TJS_GET_VM_REG_ADDR(ra, code[1]);
	tTJSVariant *rl = r + code[2]; // code[2] is count ( not reg offset )
	while(r<rl) (r++)->Clear();
}
//---------------------------------------------------------------------------
void tTJSInterCodeContext::GetPropertyDirect(tTJSVariant *ra,
	const tjs_int32 *code, tjs_uint32 flags)
{
	// ra[code[1]] = ra[code[2]][DataArea[ra[code[3]]]];

	tTJSVariant * ra_code2 = TJS_GET_VM_REG_ADDR(ra, code[2]);
	tTJSVariantType type = ra_code2->Type();
	if(type == tvtString)
	{
		GetStringProperty(TJS_GET_VM_REG_ADDR(ra, code[1]), ra_code2,
			TJS_GET_VM_REG(DataArea, code[3]));
		return;
	}
	if(type == tvtOctet)
	{
		GetOctetProperty(TJS_GET_VM_REG_ADDR(ra, code[1]), ra_code2,
			TJS_GET_VM_REG(DataArea, code[3]));
		return;
	}


	tjs_error hr;
	tTJSVariantClosure clo = ra_code2->AsObjectClosureNoAddRef();
	tTJSVariant *name = TJS_GET_VM_REG_ADDR(DataArea, code[3]);
	hr = clo.PropGet(flags,
		name->GetString(), name->GetHint(), TJS_GET_VM_REG_ADDR(ra, code[1]),
			clo.ObjThis?clo.ObjThis:ra[-1].AsObjectNoAddRef());
	if(TJS_FAILED(hr))
		TJSThrowFrom_tjs_error(hr, TJS_GET_VM_REG(DataArea, code[3]).GetString());
}
//---------------------------------------------------------------------------
void tTJSInterCodeContext::SetPropertyDirect(tTJSVariant *ra,
	const tjs_int32 *code, tjs_uint32 flags)
{
	// ra[code[1]][DataArea[ra[code[2]]]] = ra[code[3]]]

	tTJSVariant * ra_code1 = TJS_GET_VM_REG_ADDR(ra, code[1]);
	tTJSVariantType type = ra_code1->Type();
	if(type == tvtString)
	{
		SetStringProperty(TJS_GET_VM_REG_ADDR(ra, code[3]), ra_code1,
			TJS_GET_VM_REG(DataArea, code[2]));
		return;
	}
	if(type == tvtOctet)
	{
		SetOctetProperty(TJS_GET_VM_REG_ADDR(ra, code[3]), ra_code1,
			TJS_GET_VM_REG(DataArea, code[2]));
		return;
	}

	tjs_error hr;
	tTJSVariantClosure clo = ra_code1->AsObjectClosureNoAddRef();
	tTJSVariant *name = TJS_GET_VM_REG_ADDR(DataArea, code[2]);
	hr = clo.PropSetByVS(flags,
		name->AsStringNoAddRef(), TJS_GET_VM_REG_ADDR(ra, code[3]),
			clo.ObjThis?clo.ObjThis:ra[-1].AsObjectNoAddRef());
	if(hr == TJS_E_NOTIMPL)
		hr = clo.PropSet(flags,
			name->GetString(), name->GetHint(), TJS_GET_VM_REG_ADDR(ra, code[3]),
				clo.ObjThis?clo.ObjThis:ra[-1].AsObjectNoAddRef());
	if(TJS_FAILED(hr))
		TJSThrowFrom_tjs_error(hr, TJS_GET_VM_REG(DataArea, code[2]).GetString());
}
//---------------------------------------------------------------------------
void tTJSInterCodeContext::GetProperty(tTJSVariant *ra, const tjs_int32 *code)
{
	// ra[code[1]] = * ra[code[2]]
	tTJSVariantClosure clo =
		TJS_GET_VM_REG_ADDR(ra, code[2])->AsObjectClosureNoAddRef();
	tjs_error hr;
	hr = clo.PropGet(0, NULL, NULL, TJS_GET_VM_REG_ADDR(ra, code[1]),
		clo.ObjThis?clo.ObjThis:ra[-1].AsObjectNoAddRef());
	if(TJS_FAILED(hr))
		TJSThrowFrom_tjs_error(hr, NULL);
}
//---------------------------------------------------------------------------
void tTJSInterCodeContext::SetProperty(tTJSVariant *ra, const tjs_int32 *code)
{
	// * ra[code[1]] = ra[code[2]]
	tTJSVariantClosure clo =
		TJS_GET_VM_REG_ADDR(ra, code[1])->AsObjectClosureNoAddRef();
	tjs_error hr;
	hr = clo.PropSet(0, NULL, NULL, TJS_GET_VM_REG_ADDR(ra, code[2]),
		clo.ObjThis?clo.ObjThis:ra[-1].AsObjectNoAddRef());
	if(TJS_FAILED(hr))
		TJSThrowFrom_tjs_error(hr, NULL);
}
//---------------------------------------------------------------------------
void tTJSInterCodeContext::GetPropertyIndirect(tTJSVariant *ra,
	const tjs_int32 *code, tjs_uint32 flags)
{
	// ra[code[1]] = ra[code[2]][ra[code[3]]];

	tTJSVariant * ra_code2 = TJS_GET_VM_REG_ADDR(ra, code[2]);
	tTJSVariantType type = ra_code2->Type();
	if(type == tvtString)
	{
		GetStringProperty(TJS_GET_VM_REG_ADDR(ra, code[1]), ra_code2,
			TJS_GET_VM_REG(ra, code[3]));
		return;
	}
	if(type == tvtOctet)
	{
		GetOctetProperty(TJS_GET_VM_REG_ADDR(ra, code[1]), ra_code2,
			TJS_GET_VM_REG(ra, code[3]));
		return;
	}

	tjs_error hr;
	tTJSVariantClosure clo = ra_code2->AsObjectClosureNoAddRef();
	tTJSVariant * ra_code3 = TJS_GET_VM_REG_ADDR(ra, code[3]);
	if(ra_code3->Type() != tvtInteger)
	{
		tTJSVariantString *str;
		str = ra_code3->AsString();

		try
		{
			// TODO: verify here needs hint holding
			hr = clo.PropGet(flags, *str, NULL, TJS_GET_VM_REG_ADDR(ra, code[1]),
				clo.ObjThis?clo.ObjThis:ra[-1].AsObjectNoAddRef());
			if(TJS_FAILED(hr)) TJSThrowFrom_tjs_error(hr, *str);
		}
		catch(...)
		{
			if(str) str->Release();
			throw;
		}
		if(str) str->Release();
	}
	else
	{
		hr = clo.PropGetByNum(flags, (tjs_int)ra_code3->AsInteger(),
			TJS_GET_VM_REG_ADDR(ra, code[1]),
			clo.ObjThis?clo.ObjThis:ra[-1].AsObjectNoAddRef());
		if(TJS_FAILED(hr)) ThrowFrom_tjs_error_num(hr, (tjs_int)ra_code3->AsInteger());
	}
}
//---------------------------------------------------------------------------
void tTJSInterCodeContext::SetPropertyIndirect(tTJSVariant *ra,
	const tjs_int32 *code, tjs_uint32 flags)
{
	// ra[code[1]][ra[code[2]]] = ra[code[3]]]

	tTJSVariant *ra_code1 = TJS_GET_VM_REG_ADDR(ra, code[1]);
	tTJSVariantType type = ra_code1->Type();
	if(type == tvtString)
	{
		SetStringProperty(TJS_GET_VM_REG_ADDR(ra, code[3]),
			TJS_GET_VM_REG_ADDR(ra, code[1]), TJS_GET_VM_REG(ra, code[2]));
		return;
	}
	if(type == tvtOctet)
	{
		SetOctetProperty(TJS_GET_VM_REG_ADDR(ra, code[3]),
			TJS_GET_VM_REG_ADDR(ra, code[1]), TJS_GET_VM_REG(ra, code[2]));
		return;
	}

	tTJSVariantClosure clo = ra_code1->AsObjectClosure();
	tTJSVariant *ra_code2 = TJS_GET_VM_REG_ADDR(ra, code[2]);
	if(ra_code2->Type() != tvtInteger)
	{
		tTJSVariantString *str;
		try
		{
			str = ra_code2->AsString();
		}
		catch(...)
		{
			clo.Release();
			throw;
		}
		tjs_error hr;

		try
		{
			hr = clo.PropSetByVS(flags,
				str, TJS_GET_VM_REG_ADDR(ra, code[3]),
					clo.ObjThis?clo.ObjThis:ra[-1].AsObjectNoAddRef());
			if(hr == TJS_E_NOTIMPL)
				hr = clo.PropSet(flags,
					*str, NULL, TJS_GET_VM_REG_ADDR(ra, code[3]),
						clo.ObjThis?clo.ObjThis:ra[-1].AsObjectNoAddRef());
			if(TJS_FAILED(hr)) TJSThrowFrom_tjs_error(hr, *str);
		}
		catch(...)
		{
			if(str) str->Release();
			clo.Release();
			throw;
		}
		if(str) str->Release();
		clo.Release();
	}
	else
	{
		tjs_error hr;

		try
		{
			hr = clo.PropSetByNum(flags,
				(tjs_int)ra_code2->AsInteger(),
					TJS_GET_VM_REG_ADDR(ra, code[3]),
					clo.ObjThis?clo.ObjThis:ra[-1].AsObjectNoAddRef());
			if(TJS_FAILED(hr))
				ThrowFrom_tjs_error_num(hr, (tjs_int)ra_code2->AsInteger());
		}
		catch(...)
		{
			clo.Release();
			throw;
		}
		clo.Release();
	}
}
//---------------------------------------------------------------------------
void tTJSInterCodeContext::OperatePropertyDirect(tTJSVariant *ra,
	const tjs_int32 *code, tjs_uint32 ope)
{
	// ra[code[1]] = ope(ra[code[2]][DataArea[ra[code[3]]]], /*param=*/ra[code[4]]);

	tTJSVariantClosure clo =  TJS_GET_VM_REG(ra, code[2]).AsObjectClosure();
	tjs_error hr;
	try
	{
		tTJSVariant *name = TJS_GET_VM_REG_ADDR(DataArea, code[3]);
		hr = clo.Operation(ope,
			name->GetString(), name->GetHint(),
			code[1]?TJS_GET_VM_REG_ADDR(ra, code[1]):NULL,
			TJS_GET_VM_REG_ADDR(ra, code[4]),
			clo.ObjThis?clo.ObjThis:ra[-1].AsObjectNoAddRef());
	}
	catch(...)
	{
		clo.Release();
		throw;
	}
	clo.Release();
	if(TJS_FAILED(hr))
		TJSThrowFrom_tjs_error(hr, TJS_GET_VM_REG(DataArea, code[3]).GetString());
}
//---------------------------------------------------------------------------
void tTJSInterCodeContext::OperatePropertyIndirect(tTJSVariant *ra,
	const tjs_int32 *code, tjs_uint32 ope)
{
	// ra[code[1]] = ope(ra[code[2]][ra[code[3]]], /*param=*/ra[code[4]]);

	tTJSVariantClosure clo = TJS_GET_VM_REG(ra, code[2]).AsObjectClosure();
	tTJSVariant *ra_code3 = TJS_GET_VM_REG_ADDR(ra, code[3]);
	if(ra_code3->Type() != tvtInteger)
	{
		tTJSVariantString *str;
		try
		{
			str = ra_code3->AsString();
		}
		catch(...)
		{
			clo.Release();
			throw;
		}
		tjs_error hr;
		try
		{
			hr = clo.Operation(ope, *str, NULL,
				code[1]?TJS_GET_VM_REG_ADDR(ra, code[1]):NULL,
				TJS_GET_VM_REG_ADDR(ra, code[4]),
					clo.ObjThis?clo.ObjThis:ra[-1].AsObjectNoAddRef());
			if(TJS_FAILED(hr)) TJSThrowFrom_tjs_error(hr, *str);
		}
		catch(...)
		{
			if(str) str->Release();
			clo.Release();
			throw;
		}
		if(str) str->Release();
		clo.Release();
	}
	else
	{
		tjs_error hr;
		try
		{
			hr = clo.OperationByNum(ope, (tjs_int)ra_code3->AsInteger(),
				code[1]?TJS_GET_VM_REG_ADDR(ra, code[1]):NULL,
				TJS_GET_VM_REG_ADDR(ra, code[4]),
					clo.ObjThis?clo.ObjThis:ra[-1].AsObjectNoAddRef());
			if(TJS_FAILED(hr))
				ThrowFrom_tjs_error_num(hr,
					(tjs_int)TJS_GET_VM_REG(ra, code[3]).AsInteger());
		}
		catch(...)
		{
			clo.Release();
			throw;
		}
		clo.Release();
	}
}
//---------------------------------------------------------------------------
void tTJSInterCodeContext::OperateProperty(tTJSVariant *ra,
	const tjs_int32 *code, tjs_uint32 ope)
{
	// ra[code[1]] = ope(ra[code[2]], /*param=*/ra[code[3]]);
	tTJSVariantClosure clo =  TJS_GET_VM_REG(ra, code[2]).AsObjectClosure();
	tjs_error hr;
	try
	{
		hr = clo.Operation(ope,
			NULL, NULL,
			code[1]?TJS_GET_VM_REG_ADDR(ra, code[1]):NULL,
			TJS_GET_VM_REG_ADDR(ra, code[3]),
			clo.ObjThis?clo.ObjThis:ra[-1].AsObjectNoAddRef());
	}
	catch(...)
	{
		clo.Release();
		throw;
	}
	clo.Release();
	if(TJS_FAILED(hr))
		TJSThrowFrom_tjs_error(hr, NULL);
}
//---------------------------------------------------------------------------
void tTJSInterCodeContext::OperatePropertyDirect0(tTJSVariant *ra,
	const tjs_int32 *code, tjs_uint32 ope)
{
	// ra[code[1]] = ope(ra[code[2]][DataArea[ra[code[3]]]]);

	tTJSVariantClosure clo = TJS_GET_VM_REG(ra, code[2]).AsObjectClosure();
	tjs_error hr;
	try
	{
		tTJSVariant *name = TJS_GET_VM_REG_ADDR(DataArea, code[3]);
		hr = clo.Operation(ope,
			name->GetString(), name->GetHint(),
			code[1]?TJS_GET_VM_REG_ADDR(ra, code[1]):NULL, NULL,
			ra[-1].AsObjectNoAddRef());
	}
	catch(...)
	{
		clo.Release();
		throw;
	}
	clo.Release();
	if(TJS_FAILED(hr))
		TJSThrowFrom_tjs_error(hr, TJS_GET_VM_REG(DataArea, code[3]).GetString());
}
//---------------------------------------------------------------------------
void tTJSInterCodeContext::OperatePropertyIndirect0(tTJSVariant *ra,
	const tjs_int32 *code, tjs_uint32 ope)
{
	// ra[code[1]] = ope(ra[code[2]][ra[code[3]]]);

	tTJSVariantClosure clo = TJS_GET_VM_REG(ra, code[2]).AsObjectClosure();
	tTJSVariant *ra_code3 = TJS_GET_VM_REG_ADDR(ra, code[3]);
	if(ra_code3->Type() != tvtInteger)
	{
		tTJSVariantString *str;
		try
		{
			str = ra_code3->AsString();
		}
		catch(...)
		{
			clo.Release();
			throw;
		}
		tjs_error hr;
		try
		{
			hr = clo.Operation(ope, *str, NULL,
				code[1]?TJS_GET_VM_REG_ADDR(ra, code[1]):NULL, NULL,
					clo.ObjThis?clo.ObjThis:ra[-1].AsObjectNoAddRef());
			if(TJS_FAILED(hr)) TJSThrowFrom_tjs_error(hr, *str);
		}
		catch(...)
		{
			if(str) str->Release();
			clo.Release();
			throw;
		}
		if(str) str->Release();
		clo.Release();
	}
	else
	{
		tjs_error hr;
		try
		{
			hr = clo.OperationByNum(ope, (tjs_int)TJS_GET_VM_REG(ra, code[3]).AsInteger(),
				code[1]?TJS_GET_VM_REG_ADDR(ra, code[1]):NULL, NULL,
					clo.ObjThis?clo.ObjThis:ra[-1].AsObjectNoAddRef());
			if(TJS_FAILED(hr))
				ThrowFrom_tjs_error_num(hr,
					(tjs_int)TJS_GET_VM_REG(ra, code[3]).AsInteger());
		}
		catch(...)
		{
			clo.Release();
			throw;
		}
		clo.Release();
	}
}
//---------------------------------------------------------------------------
void tTJSInterCodeContext::OperateProperty0(tTJSVariant *ra,
	const tjs_int32 *code, tjs_uint32 ope)
{
	// ra[code[1]] = ope(ra[code[2]]);
	tTJSVariantClosure clo =  TJS_GET_VM_REG(ra, code[2]).AsObjectClosure();
	tjs_error hr;
	try
	{
		hr = clo.Operation(ope,
			NULL, NULL,
			code[1]?TJS_GET_VM_REG_ADDR(ra, code[1]):NULL, NULL,
			clo.ObjThis?clo.ObjThis:ra[-1].AsObjectNoAddRef());
	}
	catch(...)
	{
		clo.Release();
		throw;
	}
	clo.Release();
	if(TJS_FAILED(hr))
		TJSThrowFrom_tjs_error(hr, NULL);
}
//---------------------------------------------------------------------------
void tTJSInterCodeContext::DeleteMemberDirect(tTJSVariant *ra,
	const tjs_int32 *code)
{
	// ra[code[1]] = delete ra[code[2]][DataArea[ra[code[3]]]];

	tTJSVariantClosure clo = TJS_GET_VM_REG(ra, code[2]).AsObjectClosure();
	tjs_error hr;
	try
	{
		tTJSVariant *name = TJS_GET_VM_REG_ADDR(DataArea, code[3]);
		hr = clo.DeleteMember(0,
			name->GetString(), name->GetHint(), ra[-1].AsObjectNoAddRef());
	}
	catch(...)
	{
		clo.Release();
		throw;
	}
	clo.Release();
	if(code[1])
	{
		if(TJS_FAILED(hr))
			TJS_GET_VM_REG(ra, code[1]) = false;
		else
			TJS_GET_VM_REG(ra, code[1]) = true;
	}
}
//---------------------------------------------------------------------------
void tTJSInterCodeContext::DeleteMemberIndirect(tTJSVariant *ra,
	const tjs_int32 *code)
{
	// ra[code[1]] = delete ra[code[2]][ra[code[3]]];

	tTJSVariantClosure clo = TJS_GET_VM_REG(ra, code[2]).AsObjectClosure();
	tTJSVariantString *str;
	try
	{
		str = TJS_GET_VM_REG(ra, code[3]).AsString();
	}
	catch(...)
	{
		clo.Release();
		throw;
	}
	tjs_error hr;

	try
	{
		hr = clo.DeleteMember(0, *str, NULL,
			clo.ObjThis?clo.ObjThis:ra[-1].AsObjectNoAddRef());
		if(code[1])
		{
			if(TJS_FAILED(hr))
				TJS_GET_VM_REG(ra, code[1]) = false;
			else
				TJS_GET_VM_REG(ra, code[1]) = true;
		}
	}
	catch(...)
	{
		if(str) str->Release();
		clo.Release();
		throw;
	}
	if(str) str->Release();
	clo.Release();
}
//---------------------------------------------------------------------------
void tTJSInterCodeContext::TypeOfMemberDirect(tTJSVariant *ra,
	const tjs_int32 *code, tjs_uint32 flags)
{
	// ra[code[1]] = typeof ra[code[2]][DataArea[ra[code[3]]]];
	tTJSVariantType type = TJS_GET_VM_REG(ra, code[2]).Type();
	if(type == tvtString)
	{
		GetStringProperty(TJS_GET_VM_REG_ADDR(ra, code[1]),
			TJS_GET_VM_REG_ADDR(ra, code[2]),
			TJS_GET_VM_REG(DataArea,code[3]));
		TypeOf(TJS_GET_VM_REG(ra, code[1]));
		return;
	}
	if(type == tvtOctet)
	{
		GetOctetProperty(TJS_GET_VM_REG_ADDR(ra, code[1]),
			TJS_GET_VM_REG_ADDR(ra, code[2]),
			TJS_GET_VM_REG(DataArea, code[3]));
		TypeOf(TJS_GET_VM_REG(ra, code[1]));
		return;
	}

	tjs_error hr;
	tTJSVariantClosure clo = TJS_GET_VM_REG(ra, code[2]).AsObjectClosure();
	try
	{
		tTJSVariant *name = TJS_GET_VM_REG_ADDR(DataArea, code[3]);
		hr = clo.PropGet(flags,
			name->GetString(), name->GetHint(), TJS_GET_VM_REG_ADDR(ra, code[1]),
				clo.ObjThis?clo.ObjThis:ra[-1].AsObjectNoAddRef());
	}
	catch(...)
	{
		clo.Release();
		throw;
	}
	clo.Release();
	if(hr == TJS_S_OK)
	{
		TypeOf(TJS_GET_VM_REG(ra, code[1]));
	}
	else if(hr == TJS_E_MEMBERNOTFOUND)
	{
		static tTJSString undefined_name(TJSMapGlobalStringMap(TJS_W("undefined")));
		TJS_GET_VM_REG(ra, code[1]) = undefined_name;
	}
	else if(TJS_FAILED(hr))
		TJSThrowFrom_tjs_error(hr, TJS_GET_VM_REG(DataArea, code[3]).GetString());
}
//---------------------------------------------------------------------------
void tTJSInterCodeContext::TypeOfMemberIndirect(tTJSVariant *ra,
	const tjs_int32 *code, tjs_uint32 flags)
{
	// ra[code[1]] = typeof ra[code[2]][ra[code[3]]];

	tTJSVariantType type = TJS_GET_VM_REG(ra, code[2]).Type();
	if(type == tvtString)
	{
		GetStringProperty(TJS_GET_VM_REG_ADDR(ra, code[1]),
			TJS_GET_VM_REG_ADDR(ra, code[2]),
			TJS_GET_VM_REG(ra, code[3]));
		TypeOf(ra[code[1]]);
		return;
	}
	if(type == tvtOctet)
	{
		GetOctetProperty(TJS_GET_VM_REG_ADDR(ra, code[1]),
			TJS_GET_VM_REG_ADDR(ra, code[2]),
			TJS_GET_VM_REG(ra, code[3]));
		TypeOf(TJS_GET_VM_REG(ra, code[1]));
		return;
	}

	tjs_error hr;
	tTJSVariantClosure clo = TJS_GET_VM_REG(ra, code[2]).AsObjectClosure();
	if(TJS_GET_VM_REG(ra, code[3]).Type() != tvtInteger)
	{
		tTJSVariantString *str;
		try
		{
			str = TJS_GET_VM_REG(ra, code[3]).AsString();
		}
		catch(...)
		{
			clo.Release();
			throw;
		}

		try
		{
			// TODO: verify here needs hint holding
			hr = clo.PropGet(flags, *str, NULL, TJS_GET_VM_REG_ADDR(ra, code[1]),
				clo.ObjThis?clo.ObjThis:ra[-1].AsObjectNoAddRef());
			if(hr == TJS_S_OK)
			{
				TypeOf(TJS_GET_VM_REG(ra, code[1]));
			}
			else if(hr == TJS_E_MEMBERNOTFOUND)
			{
				TJS_GET_VM_REG(ra, code[1]) = TJS_W("undefined");
			}
			else if(TJS_FAILED(hr)) TJSThrowFrom_tjs_error(hr, *str);
		}
		catch(...)
		{
			if(str) str->Release();
			clo.Release();
			throw;
		}
		if(str) str->Release();
		clo.Release();
	}
	else
	{
		try
		{
			hr = clo.PropGetByNum(flags,
				(tjs_int)TJS_GET_VM_REG(ra, code[3]).AsInteger(),
				TJS_GET_VM_REG_ADDR(ra, code[1]),
				clo.ObjThis?clo.ObjThis:ra[-1].AsObjectNoAddRef());
			if(hr == TJS_S_OK)
			{
				TypeOf(TJS_GET_VM_REG(ra, code[1]));
			}
			else if(hr == TJS_E_MEMBERNOTFOUND)
			{
				TJS_GET_VM_REG(ra, code[1]) = TJS_W("undefined");
			}
			else if(TJS_FAILED(hr))
				ThrowFrom_tjs_error_num(hr,
					(tjs_int)TJS_GET_VM_REG(ra, code[3]).AsInteger());
		}
		catch(...)
		{
			clo.Release();
			throw;
		}
		clo.Release();
	}
}
//---------------------------------------------------------------------------
// Macros for preparing function argument pointer array.
// code[0] is an argument count;
// -1 for omitting ('...') argument to passing unmodified args from the caller.
// -2 for expanding array to argument
#define TJS_PASS_ARGS_PREPARED_ARRAY_COUNT 20

#define TJS_BEGIN_FUNC_CALL_ARGS(_code)                                   \
	tTJSVariant ** pass_args;                                             \
	tTJSVariant *pass_args_p[TJS_PASS_ARGS_PREPARED_ARRAY_COUNT];         \
	tTJSVariant * pass_args_v = NULL;                                     \
	tjs_int code_size;                                                    \
	bool alloc_args = false;                                              \
	try                                                                   \
	{                                                                     \
		tjs_int pass_args_count = (_code)[0];                             \
		if(pass_args_count == -1)                                         \
		{                                                                 \
			/* omitting args; pass intact aguments from the caller */     \
			pass_args = args;                                             \
			pass_args_count = numargs;                                    \
			code_size = 1;                                                \
		}                                                                 \
		else if(pass_args_count == -2)                                    \
		{                                                                 \
			tjs_int args_v_count = 0;                                     \
			/* count total argument count */                              \
			pass_args_count = 0;                                          \
			tjs_int arg_written_count = (_code)[1];                       \
			code_size = arg_written_count * 2 + 2;                        \
			for(tjs_int i = 0; i < arg_written_count; i++)                \
			{                                                             \
				switch((_code)[i*2+2])                                    \
				{                                                         \
				case fatNormal:                                           \
					pass_args_count ++;                                   \
					break;                                                \
				case fatExpand:                                           \
					args_v_count +=                                       \
						TJSGetArrayElementCount(                          \
							TJS_GET_VM_REG(ra, (_code)[i*2+1+2]).         \
								AsObjectNoAddRef());                      \
					break;                                                \
				case fatUnnamedExpand:                                    \
					pass_args_count +=                                    \
						(numargs > FuncDeclUnnamedArgArrayBase)?          \
							(numargs - FuncDeclUnnamedArgArrayBase) : 0;  \
					break;                                                \
				}                                                         \
			}                                                             \
			pass_args_count += args_v_count;                              \
			/* allocate temporary variant array for Array object */       \
			pass_args_v = new tTJSVariant[args_v_count];                  \
			/* allocate pointer array */                                  \
			if(pass_args_count < TJS_PASS_ARGS_PREPARED_ARRAY_COUNT)      \
				pass_args = pass_args_p;                                  \
			else                                                          \
				pass_args = new tTJSVariant * [pass_args_count],          \
					alloc_args = true;                                    \
			/* create pointer array to pass to callee function */         \
			args_v_count = 0;                                             \
			pass_args_count = 0;                                          \
			for(tjs_int i = 0; i < arg_written_count; i++)                \
			{                                                             \
				switch((_code)[i*2+2])                                    \
				{                                                         \
				case fatNormal:                                           \
					pass_args[pass_args_count++] =                        \
						TJS_GET_VM_REG_ADDR(ra, (_code)[i*2+1+2]);        \
					break;                                                \
				case fatExpand:                                           \
				  {                                                       \
					tjs_int count = TJSCopyArrayElementTo(                \
						TJS_GET_VM_REG(ra, (_code)[i*2+1+2]).             \
								AsObjectNoAddRef(),                       \
								pass_args_v + args_v_count, 0, -1);       \
					for(tjs_int j = 0; j < count; j++)                    \
						pass_args[pass_args_count++] =                    \
							pass_args_v + j + args_v_count;               \
	                                                                      \
					args_v_count += count;                                \
	                                                                      \
					break;                                                \
				  }                                                       \
				case fatUnnamedExpand:                                    \
				  {                                                       \
					tjs_int count =                                       \
						(numargs > FuncDeclUnnamedArgArrayBase)?          \
							(numargs - FuncDeclUnnamedArgArrayBase) : 0;  \
					for(tjs_int j = 0; j < count; j++)                    \
						pass_args[pass_args_count++] =                    \
							args[FuncDeclUnnamedArgArrayBase + j];        \
					break;                                                \
				  }                                                       \
				}                                                         \
			}                                                             \
		}                                                                 \
		else if(pass_args_count <= TJS_PASS_ARGS_PREPARED_ARRAY_COUNT)    \
		{                                                                 \
			code_size = pass_args_count + 1;                              \
			pass_args = pass_args_p;                                      \
			for(tjs_int i = 0; i < pass_args_count; i++)                  \
				pass_args[i] = TJS_GET_VM_REG_ADDR(ra, (_code)[1+i]);     \
		}                                                                 \
		else                                                              \
		{                                                                 \
			code_size = pass_args_count + 1;                              \
			pass_args = new tTJSVariant * [pass_args_count];              \
			alloc_args = true;                                            \
			for(tjs_int i = 0; i < pass_args_count; i++)                  \
				pass_args[i] = TJS_GET_VM_REG_ADDR(ra, (_code)[1+i]);     \
		}


#define TJS_END_FUNC_CALL_ARGS                                            \
	}                                                                     \
	catch(...)                                                            \
	{                                                                     \
		if(alloc_args) delete [] pass_args;                               \
		if(pass_args_v) delete [] pass_args_v;                            \
		throw;                                                            \
	}                                                                     \
	if(alloc_args) delete [] pass_args;                                   \
	if(pass_args_v) delete [] pass_args_v;
//---------------------------------------------------------------------------
tjs_int tTJSInterCodeContext::CallFunction(tTJSVariant *ra,
	const tjs_int32 *code, tTJSVariant **args, tjs_int numargs)
{
	// function calling / create new object
	tjs_error hr;

	TJS_BEGIN_FUNC_CALL_ARGS(code + 3)

		tTJSVariantClosure clo = TJS_GET_VM_REG(ra, code[2]).AsObjectClosure();
		try
		{
			if(code[0] == VM_CALL)
			{
				hr = clo.FuncCall(0, NULL, NULL,
					code[1]?TJS_GET_VM_REG_ADDR(ra, code[1]):NULL, pass_args_count, pass_args,
						clo.ObjThis?clo.ObjThis:ra[-1].AsObjectNoAddRef());
			}
			else
			{
				iTJSDispatch2 *dsp;
				hr = clo.CreateNew(0, NULL, NULL,
					&dsp, pass_args_count, pass_args,
						clo.ObjThis?clo.ObjThis:ra[-1].AsObjectNoAddRef());
				if(TJS_SUCCEEDED(hr))
				{
					if(dsp)
					{
						if(code[1]) TJS_GET_VM_REG(ra, code[1]) = tTJSVariant(dsp, dsp);
						dsp->Release();
					}
				}
			} 
		}
		catch(...)
		{ 
			clo.Release();
			throw;
		}
		clo.Release();
		// TODO: Null Check

	TJS_END_FUNC_CALL_ARGS

	if(TJS_FAILED(hr)) TJSThrowFrom_tjs_error(hr, TJS_W(""));

	return code_size + 3;
}
#undef _code
//---------------------------------------------------------------------------
tjs_int tTJSInterCodeContext::CallFunctionDirect(tTJSVariant *ra,
	const tjs_int32 *code, tTJSVariant **args, tjs_int numargs)
{
	tjs_error hr;

	TJS_BEGIN_FUNC_CALL_ARGS(code + 4)

		tTJSVariantType type = TJS_GET_VM_REG(ra, code[2]).Type();
		tTJSVariant * name = TJS_GET_VM_REG_ADDR(DataArea, code[3]);
		if(type == tvtString)
		{
			ProcessStringFunction(name->GetString(),
				TJS_GET_VM_REG(ra, code[2]),
				pass_args, pass_args_count, code[1]?TJS_GET_VM_REG_ADDR(ra, code[1]):NULL);
			hr = TJS_S_OK;
		}
		else if(type == tvtOctet)
		{
			ProcessOctetFunction(name->GetString(),
				TJS_GET_VM_REG(ra, code[2]).AsOctetNoAddRef(),
				pass_args, pass_args_count, code[1]?TJS_GET_VM_REG_ADDR(ra, code[1]):NULL);
			hr = TJS_S_OK;
		}
		else
		{
			tTJSVariantClosure clo =  TJS_GET_VM_REG(ra, code[2]).AsObjectClosure();
			try
			{
				hr = clo.FuncCall(0,
					name->GetString(), name->GetHint(),
					code[1]?TJS_GET_VM_REG_ADDR(ra, code[1]):NULL,
						pass_args_count, pass_args,
						clo.ObjThis?clo.ObjThis:ra[-1].AsObjectNoAddRef());
			}
			catch(...)
			{
				clo.Release();
				throw;
			}
			clo.Release();
		}

	TJS_END_FUNC_CALL_ARGS

	if(TJS_FAILED(hr))
		TJSThrowFrom_tjs_error(hr,
			ttstr(TJS_GET_VM_REG(DataArea, code[3])).c_str());

	return code_size + 4;
}
//---------------------------------------------------------------------------
tjs_int tTJSInterCodeContext::CallFunctionIndirect(tTJSVariant *ra,
	const tjs_int32 *code, tTJSVariant **args, tjs_int numargs)
{
	tjs_error hr;

	ttstr name = TJS_GET_VM_REG(ra, code[3]);

	TJS_BEGIN_FUNC_CALL_ARGS(code + 4)

		tTJSVariantType type = TJS_GET_VM_REG(ra, code[2]).Type();
		if(type == tvtString)
		{
			ProcessStringFunction(name.c_str(),
				TJS_GET_VM_REG(ra, code[2]),
				pass_args, pass_args_count, code[1]?TJS_GET_VM_REG_ADDR(ra, code[1]):NULL);
			hr = TJS_S_OK;
		}
		else if(type == tvtOctet)
		{
			ProcessOctetFunction(name.c_str(),
				TJS_GET_VM_REG(ra, code[2]).AsOctetNoAddRef(),
				pass_args, pass_args_count, code[1]?TJS_GET_VM_REG_ADDR(ra, code[1]):NULL);
			hr = TJS_S_OK;
		}
		else
		{
			tTJSVariantClosure clo =  TJS_GET_VM_REG(ra, code[2]).AsObjectClosure();
			try
			{
				hr = clo.FuncCall(0,
					name.c_str(), name.GetHint(),
					code[1]?TJS_GET_VM_REG_ADDR(ra, code[1]):NULL,
						pass_args_count, pass_args,
						clo.ObjThis?clo.ObjThis:ra[-1].AsObjectNoAddRef());
			}
			catch(...)
			{
				clo.Release();
				throw;
			}
			clo.Release();
		}

	TJS_END_FUNC_CALL_ARGS

	if(TJS_FAILED(hr))
		TJSThrowFrom_tjs_error(hr, name.c_str());

	return code_size + 4;
}
//---------------------------------------------------------------------------
void tTJSInterCodeContext::AddClassInstanceInfo(tTJSVariant *ra,
	const tjs_int32 *code)
{
	iTJSDispatch2 *dsp;
	dsp = TJS_GET_VM_REG(ra, code[1]).AsObjectNoAddRef();
	if(dsp)
	{
		dsp->ClassInstanceInfo(TJS_CII_ADD, 0, TJS_GET_VM_REG_ADDR(ra, code[2]));
	}
	else
	{
		// ?? must be an error
	}
}
//---------------------------------------------------------------------------
static const tjs_char *StrFuncs[] =
{ 
	TJS_W("charAt"), 
	TJS_W("indexOf"), 
	TJS_W("toUpperCase"),
	TJS_W("toLowerCase"), 
	TJS_W("substring"), 
	TJS_W("substr"), 
	TJS_W("sprintf"),
	TJS_W("replace"), 
	TJS_W("escape"), 
	TJS_W("split"),
	TJS_W("trim"),
	TJS_W("reverse"),
	TJS_W("repeat") 
};

enum tTJSStringMethodNameIndex
{
	TJSStrMethod_charAt = 0,
	TJSStrMethod_indexOf,
	TJSStrMethod_toUpperCase,
	TJSStrMethod_toLowerCase,
	TJSStrMethod_substring, 
	TJSStrMethod_substr, 
	TJSStrMethod_sprintf,
	TJSStrMethod_replace, 
	TJSStrMethod_escape, 
	TJSStrMethod_split,
	TJSStrMethod_trim,
	TJSStrMethod_reverse,
	TJSStrMethod_repeat
};

#define TJS_STRFUNC_MAX (sizeof(StrFuncs) / sizeof(StrFuncs[0]))
static tjs_int32 StrFuncHash[TJS_STRFUNC_MAX];
static bool TJSStrFuncInit = false;
static void InitTJSStrFunc()
{
	TJSStrFuncInit = true;
	for(tjs_int i=0; i<TJS_STRFUNC_MAX; i++)
	{
		const tjs_char *p = StrFuncs[i];
		tjs_int32 hash = 0;
		while(*p) hash += *p, p++;
		StrFuncHash[i] = hash;
	}
}

void tTJSInterCodeContext::ProcessStringFunction(const tjs_char *member,
	const ttstr & target, tTJSVariant **args, tjs_int numargs, tTJSVariant *result)
{
	if(!TJSStrFuncInit) InitTJSStrFunc();

	tjs_int32 hash;

	if(!member) TJSThrowFrom_tjs_error(TJS_E_MEMBERNOTFOUND, TJS_W(""));

	const tjs_char *m = member;
	hash = 0;
	while(*m) hash += *m, m++;

	const tjs_char *s = target.c_str(); // target string
	const tjs_int s_len = target.GetLen();

#define TJS_STR_METHOD_IS(_name) (hash == StrFuncHash[TJSStrMethod_##_name] && !TJS_strcmp(StrFuncs[TJSStrMethod_##_name], member))

	if(TJS_STR_METHOD_IS(charAt))
	{
		if(numargs != 1) TJSThrowFrom_tjs_error(TJS_E_BADPARAMCOUNT);
		if(s_len == 0)
		{
			if(result) *result = TJS_W("");
			return;
		}
		tjs_int i = (tjs_int)*args[0];
		if(i<0 || i>=s_len)
		{
			if(result) *result = TJS_W("");
			return;
		}
		tjs_char bt[2];
		bt[1] = 0;
		bt[0] = s[i];
		if(result) *result = bt;
		return;
	}
	else if(TJS_STR_METHOD_IS(indexOf))
	{
		if(numargs != 1 && numargs != 2) TJSThrowFrom_tjs_error(TJS_E_BADPARAMCOUNT);
		tTJSVariantString *pstr = args[0]->AsString(); // sub string

		if(!s || !pstr)
		{
			if(result) *result = (tjs_int)-1;
			if(pstr) pstr->Release();
			return;
		}
		tjs_int start;
		if(numargs == 1)
		{
			start = 0;
		}
		else
		{
			try // integer convertion may raise an exception
			{
				start = (tjs_int)*args[1];
			}
			catch(...)
			{
				pstr->Release();
				throw;
			}
		}
		if(start >= s_len)
		{
			if(result) *result = (tjs_int)-1;
			if(pstr) pstr->Release();
			return;
		}
		const tjs_char *p;
		p = TJS_strstr(s + start, (const tjs_char*)*pstr);
		if(!p)
		{
			if(result) *result = (tjs_int)-1;
		}
		else
		{
			if(result) *result = (tjs_int)(p-s);
		}
		if(pstr) pstr->Release();
		return;
	}
	else if(TJS_STR_METHOD_IS(toUpperCase))
	{
		if(numargs != 0) TJSThrowFrom_tjs_error(TJS_E_BADPARAMCOUNT);
		if(result)
		{
			*result = s; // here s is copyed to *result ( not reference )
			const tjs_char *pstr = result->GetString();  // buffer in *result
			if(pstr)
			{
				tjs_char *p = (tjs_char*)pstr;    // WARNING!! modification of const
				while(*p)
				{
					if(*p>=TJS_W('a') && *p<=TJS_W('z')) *p += TJS_W('Z')-TJS_W('z');
					p++;
				}
			}
		}
		return;
	}
	else if(TJS_STR_METHOD_IS(toLowerCase))
	{
		if(numargs != 0) TJSThrowFrom_tjs_error(TJS_E_BADPARAMCOUNT);
		if(result)
		{
			*result = s;
			const tjs_char *pstr = result->GetString();
			if(pstr)
			{
				tjs_char *p = (tjs_char*)pstr;    // WARNING!! modification of const
				while(*p)
				{
					if(*p>=TJS_W('A') && *p<=TJS_W('Z')) *p += TJS_W('z')-TJS_W('Z');
					p++;
				}
			}
		}
		return;
	}
	else if(TJS_STR_METHOD_IS(substring) || TJS_STR_METHOD_IS(substr))
	{
		if(numargs != 1 && numargs != 2) TJSThrowFrom_tjs_error(TJS_E_BADPARAMCOUNT);
		tjs_int start = (tjs_int)*args[0];
		if(start < 0 || start >= s_len)
		{
			if(result) *result=TJS_W("");
			return;
		}
		tjs_int count;
		if(numargs == 2)
		{
			count = (tjs_int)*args[1];
			if(count<0)
			{
				if(result) *result = TJS_W("");
				return;
			}
			if(start + count > s_len) count = s_len - start;
			if(result)
				*result = ttstr(s+start, count);
			return;
		}
		else
		{
			if(result) *result = s + start;
		}
		return;
	}
	else if(TJS_STR_METHOD_IS(sprintf))
	{
		if(result)
		{
			tTJSVariantString *res;
			res = TJSFormatString(s, numargs, args);
			*result = res;
			if(res) res->Release();
		}
		return;
	}
	else if(TJS_STR_METHOD_IS(replace))
	{
		// string.replace(pattern, replacement-string)  -->
		// pattern.replace(string, replacement-string)
		if(numargs < 2) TJSThrowFrom_tjs_error(TJS_E_BADPARAMCOUNT);

		tTJSVariantClosure clo = args[0]->AsObjectClosureNoAddRef();
		tTJSVariant str = target;
		tTJSVariant *params[] = { &str, args[1] };
		static tTJSString replace_name(TJS_W("replace"));
		clo.FuncCall(0, replace_name.c_str(), replace_name.GetHint(),
			result, 2, params, NULL);

		return;
	}
	else if(TJS_STR_METHOD_IS(escape))
	{
		if(result)
			*result = target.EscapeC();

		return;
	}
	else if(TJS_STR_METHOD_IS(split))
	{
		// string.split(pattern, reserved, purgeempty) -->
		// Array.split(pattern, string, reserved, purgeempty)
		if(numargs < 1) TJSThrowFrom_tjs_error(TJS_E_BADPARAMCOUNT);

		iTJSDispatch2 * array = TJSCreateArrayObject();
		try
		{
			tTJSVariant str = target;
			tjs_int arg_count = 2;
			tTJSVariant *params[4] = {
				args[0],
				&str };
			if(numargs >= 2)
			{
				arg_count ++;
				params[2] = args[1];
			}
			if(numargs >= 3)
			{
				arg_count ++;
				params[3] = args[2];
			}
			static tTJSString split_name(TJS_W("split"));
			array->FuncCall(0, split_name.c_str(), split_name.GetHint(),
				NULL, arg_count, params, array);

			if(result) *result = tTJSVariant(array, array);
		}
		catch(...)
		{
			array->Release();
			throw;
		}
		array->Release();

		return;
	}
	else if(TJS_STR_METHOD_IS(trim))
	{
		if(numargs != 0) TJSThrowFrom_tjs_error(TJS_E_BADPARAMCOUNT);
		if(!result) return;

		tjs_int w_len = s_len;
		const tjs_char *src = s + s_len - 1;
		/*  s/\s+$//;  */
		while (w_len > 0 && *src > 0x00 && *src <= 0x20)
		{
			w_len--;
			src--;
		}
		src = s;
		/*  s/^\s+//;  */
		while (w_len > 0 && *src > 0x00 && *src <= 0x20)
		{
			w_len--;
			src++;
		}

		*result = tTJSString(src, w_len);
		return;
	}
	else if(TJS_STR_METHOD_IS(reverse))
	{
		if(numargs != 0) TJSThrowFrom_tjs_error(TJS_E_BADPARAMCOUNT);
 		if(!result) return;
		if(result)
		{
			*result = s;
			const tjs_char *pstr = result->GetString();
			if(pstr)
			{
				tjs_int w_len = s_len;
				tjs_char *dest = (tjs_char*)pstr;    // WARNING!! modification of const
				const tjs_char *src = s + s_len - 1;

				while (w_len--)
				{
					*dest++ = *src--;
				}
			}
		}
		return;
	}
	else if(TJS_STR_METHOD_IS(repeat))
	{
		if(numargs != 1) TJSThrowFrom_tjs_error(TJS_E_BADPARAMCOUNT);
		if(!result) return;
		tjs_int count = (tjs_int)*args[0];

		if(count <= 0 || s_len <= 0)
		{
			*result = TJS_W("");
			return;
		}

		const int destLength = s_len * count;
		tTJSString new_str = tTJSString(tTJSStringBufferLength(destLength));
		tjs_char * dest = new_str.Independ();
		while(count--)
		{
			TJS_strcpy(dest, s);
			dest += s_len;
		}
		*result = new_str;

		return;
	}

#undef TJS_STR_METHOD_IS

	TJSThrowFrom_tjs_error(TJS_E_MEMBERNOTFOUND, member);
}
//---------------------------------------------------------------------------
void tTJSInterCodeContext::ProcessOctetFunction(const tjs_char *member, const tTJSVariantOctet * target,
		tTJSVariant **args, tjs_int numargs, tTJSVariant *result)
{
	if(!member) TJSThrowFrom_tjs_error(TJS_E_MEMBERNOTFOUND, TJS_W(""));
	switch( member[0] ) {
	case L'u':
		if( !TJS_strcmp( TJS_W("unpack"), member) ) {
			tjs_error err = TJSOctetUnpack( target, args, numargs, result );
			if( err != TJS_S_OK ) {
				TJSThrowFrom_tjs_error( err );
			}
			return;
		}
		break;
#if 0
	case L'p':
		if( !TJS_strcmp( TJS_W("pack"), member) ) {
			if(numargs < 2) TJSThrowFrom_tjs_error(TJS_E_BADPARAMCOUNT);
			ttstr templ = *args[0];
		}
		break;
#endif
	}

	TJSThrowFrom_tjs_error(TJS_E_MEMBERNOTFOUND, member);
}
//---------------------------------------------------------------------------
void tTJSInterCodeContext::TypeOf(tTJSVariant &val)
{
	// processes TJS2's typeof operator.
	static tTJSString void_name(TJSMapGlobalStringMap(TJS_W("void")));
	static tTJSString Object_name(TJSMapGlobalStringMap(TJS_W("Object")));
	static tTJSString String_name(TJSMapGlobalStringMap(TJS_W("String")));
	static tTJSString Integer_name(TJSMapGlobalStringMap(TJS_W("Integer")));
	static tTJSString Real_name(TJSMapGlobalStringMap(TJS_W("Real")));
	static tTJSString Octet_name(TJSMapGlobalStringMap(TJS_W("Octet")));

	switch(val.Type())
	{
	case tvtVoid:
		val = void_name;   // differs from TJS1
		break;

	case tvtObject:
		val = Object_name;
		break;

	case tvtString:
		val = String_name;
		break;

	case tvtInteger:
		val = Integer_name; // differs from TJS1
		break;

	case tvtReal:
		val = Real_name; // differs from TJS1
		break;

	case tvtOctet:
		val = Octet_name;
		break;
	}
}
//---------------------------------------------------------------------------
void tTJSInterCodeContext::Eval(tTJSVariant &val,
	iTJSDispatch2 * objthis, bool resneed)
{
	if(objthis) objthis->AddRef();
	try
	{
		tTJSVariant res;
		ttstr str(val);
		if(!str.IsEmpty())
		{
			if(resneed)
				Block->GetTJS()->EvalExpression(str, &res, objthis);
			else
				Block->GetTJS()->EvalExpression(str, NULL, objthis);
		}
		if(resneed) val = res;
	}
	catch(...)
	{
		if(objthis) objthis->Release();
		throw;
	}
	if(objthis) objthis->Release();
}
//---------------------------------------------------------------------------
void tTJSInterCodeContext::CharacterCodeOf(tTJSVariant &val)
{
	// puts val's character code on val
	tTJSVariantString * str = val.AsString();
	if(str)
	{
		const tjs_char *ch = (const tjs_char*)*str;
		val = tTVInteger(ch[0]);
		str->Release();
		return;
	}
	val = tTVInteger(0);
}
//---------------------------------------------------------------------------
void tTJSInterCodeContext::CharacterCodeFrom(tTJSVariant &val)
{
	tjs_char ch[2];
	ch[0] = static_cast<tjs_char>(val.AsInteger());
	ch[1] = 0;
	val = ch;
}
//---------------------------------------------------------------------------
void tTJSInterCodeContext::InstanceOf(const tTJSVariant &name, tTJSVariant &targ)
{
	// checks instance inheritance.
	tTJSVariantString *str = name.AsString();
	if(str)
	{
		tjs_error hr;
		try
		{
			hr = TJSDefaultIsInstanceOf(0, targ, (const tjs_char*)*str, NULL);
		}
		catch(...)
		{
			str->Release();
			throw;
		}
		str->Release();
		if(TJS_FAILED(hr)) TJSThrowFrom_tjs_error(hr);

		targ = (hr == TJS_S_TRUE);
		return;
	}
	targ = false;
}
//---------------------------------------------------------------------------
void tTJSInterCodeContext::RegisterObjectMember(iTJSDispatch2 * dest)
{
	// register this object member to 'dest' (destination object).
	// called when new object is to be created.
	// a class to receive member callback from class

	class tCallback : public tTJSDispatch
	{
	public:
		iTJSDispatch2 * Dest; // destination object
		tjs_error TJS_INTF_METHOD FuncCall(
			tjs_uint32 flag, const tjs_char * membername, tjs_uint32 *hint,
			tTJSVariant *result, tjs_int numparams, tTJSVariant **param,
			iTJSDispatch2 *objthis)
		{
			// *param[0] = name   *param[1] = flags   *param[2] = value
			tjs_uint32 flags = (tjs_int)*param[1];
			if(!(flags & TJS_STATICMEMBER))
			{
				tTJSVariant val = *param[2];
				if(val.Type() == tvtObject)
				{
					// change object's objthis if the object's objthis is null
//					if(val.AsObjectThisNoAddRef() == NULL)
						val.ChangeClosureObjThis(Dest);
				}

				if(Dest->PropSetByVS(TJS_MEMBERENSURE|TJS_IGNOREPROP|flags,
					param[0]->AsStringNoAddRef(), &val, Dest) == TJS_E_NOTIMPL)
					Dest->PropSet(TJS_MEMBERENSURE|TJS_IGNOREPROP|flags,
					param[0]->GetString(), NULL, &val, Dest);
			}
			if(result) *result = (tjs_int)(1); // returns true
			return TJS_S_OK;
		}
	};

	tCallback callback;
	callback.Dest = dest;

	// enumerate members
    tTJSVariantClosure clo(&callback, (iTJSDispatch2*)NULL);
	EnumMembers(TJS_IGNOREPROP, &clo, this);
}
//---------------------------------------------------------------------------
#define TJS_DO_SUPERCLASS_PROXY_BEGIN \
		std::vector<tjs_int> &pointer = SuperClassGetter->SuperClassGetterPointer; \
		if(pointer.size() != 0) \
		{ \
			std::vector<tjs_int>::reverse_iterator i; \
			for(i = pointer.rbegin(); \
				i != pointer.rend(); i++) \
			{ \
				tTJSVariant res; \
				SuperClassGetter->ExecuteAsFunction(NULL, NULL, 0, &res, *i); \
				tTJSVariantClosure clo = res.AsObjectClosureNoAddRef();

#define TJS_DO_SUPERCLASS_PROXY_END \
				if(hr != TJS_E_MEMBERNOTFOUND) break; \
			} \
		}

tjs_error TJS_INTF_METHOD  tTJSInterCodeContext::FuncCall(
		tjs_uint32 flag, const tjs_char * membername, tjs_uint32 *hint,
			tTJSVariant *result,
		tjs_int numparams, tTJSVariant **param, iTJSDispatch2 *objthis)
{
	if(!GetValidity())
		return TJS_E_INVALIDOBJECT;

	if(membername == NULL)
	{
		switch(ContextType)
		{
		case ctTopLevel:
			ExecuteAsFunction(
				objthis?objthis:Block->GetTJS()->GetGlobalNoAddRef(),
				NULL, 0, result, 0);
			break;

		case ctFunction:
		case ctExprFunction:
		case ctPropertyGetter:
		case ctPropertySetter:
			ExecuteAsFunction(objthis, param, numparams, result, 0);
			break;

		case ctClass: // on super class' initialization
			ExecuteAsFunction(objthis, param, numparams, result, 0);
			break;

		case ctProperty:
			return TJS_E_INVALIDTYPE;

		case ctSuperClassGetter:
			break;
		}

		return TJS_S_OK;
	}

	tjs_error hr = inherited::FuncCall(flag, membername, hint, result,
		numparams, param, objthis);

	if(membername != NULL && hr == TJS_E_MEMBERNOTFOUND &&
		ContextType == ctClass && SuperClassGetter)
	{
		// look up super class
		TJS_DO_SUPERCLASS_PROXY_BEGIN
			hr = clo.FuncCall(flag, membername, hint, result,
				numparams, param, objthis);
		TJS_DO_SUPERCLASS_PROXY_END
	}
	return hr;
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD  tTJSInterCodeContext::PropGet(tjs_uint32 flag,
	const tjs_char * membername, tjs_uint32 *hint, tTJSVariant *result,
		iTJSDispatch2 *objthis)
{
	if(!GetValidity())
		return TJS_E_INVALIDOBJECT;

	if(membername == NULL)
	{
		if(ContextType == ctProperty)
		{
			// executed as a property getter
			if(PropGetter)
				return PropGetter->FuncCall(0, NULL, NULL, result, 0, NULL,
					objthis);
			else
				return TJS_E_ACCESSDENYED;
		}
	}

	tjs_error hr = inherited::PropGet(flag, membername, hint, result, objthis);

	if(membername != NULL && hr == TJS_E_MEMBERNOTFOUND &&
		ContextType == ctClass && SuperClassGetter)
	{
		// look up super class
		TJS_DO_SUPERCLASS_PROXY_BEGIN
			hr = clo.PropGet(flag, membername, hint, result, objthis);
		TJS_DO_SUPERCLASS_PROXY_END
	}
	return hr;

}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD  tTJSInterCodeContext::PropSet(tjs_uint32 flag,
	const tjs_char *membername, tjs_uint32 *hint,
		const tTJSVariant *param, iTJSDispatch2 *objthis)
{
	if(!GetValidity())
		return TJS_E_INVALIDOBJECT;

	if(membername == NULL)
	{
		if(ContextType == ctProperty)
		{
			// executed as a property setter
			if(PropSetter)
				return PropSetter->FuncCall(0, NULL, NULL, NULL, 1,
					const_cast<tTJSVariant **>(&param), objthis);
			else
				return TJS_E_ACCESSDENYED;

				// WARNING!! const tTJSVariant ** -> tTJSVariant** force casting
		}
	}

	tjs_error hr;
	if(membername != NULL && ContextType == ctClass && SuperClassGetter)
	{
		tjs_uint32 pseudo_flag =
			(flag & TJS_IGNOREPROP) ? flag : (flag &~ TJS_MEMBERENSURE);
			// member ensuring is temporarily disabled unless TJS_IGNOREPROP

		hr = inherited::PropSet(pseudo_flag, membername, hint, param,
			objthis);
		if(hr == TJS_E_MEMBERNOTFOUND)
		{
			TJS_DO_SUPERCLASS_PROXY_BEGIN
				hr = clo.PropSet(pseudo_flag, membername, hint, param,
					objthis);
			TJS_DO_SUPERCLASS_PROXY_END
		}
		
		if(hr == TJS_E_MEMBERNOTFOUND && (flag & TJS_MEMBERENSURE))
		{
			// re-ensure the member for "this" object
			hr = inherited::PropSet(flag, membername, hint, param,
				objthis);
		}
	}
	else
	{
		hr = inherited::PropSet(flag, membername, hint, param,
			objthis);
	}

	return hr;
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD  tTJSInterCodeContext::CreateNew(tjs_uint32 flag,
	const tjs_char * membername, tjs_uint32 *hint,
	iTJSDispatch2 **result, tjs_int numparams,
	tTJSVariant **param, iTJSDispatch2 *objthis)
{
	if(!GetValidity())
		return TJS_E_INVALIDOBJECT;

	if(membername == NULL)
	{
		if(ContextType != ctClass) return TJS_E_INVALIDTYPE;

		iTJSDispatch2 *dsp = new inherited();

		try
		{
			ExecuteAsFunction(dsp, NULL, 0, NULL, 0);
			FuncCall(0, Name, NULL, NULL, numparams, param, dsp);
		}
		catch(...)
		{
			dsp->Release();
			throw;
		}

		*result = dsp;
		return TJS_S_OK;
	}

	tjs_error hr = inherited::CreateNew(flag, membername, hint,
		result, numparams, param,
		objthis);

	if(membername != NULL && hr == TJS_E_MEMBERNOTFOUND &&
		ContextType == ctClass && SuperClassGetter)
	{
		// look up super class
		TJS_DO_SUPERCLASS_PROXY_BEGIN
			hr = clo.CreateNew(flag, membername, hint, result, numparams, param,
				objthis);
		TJS_DO_SUPERCLASS_PROXY_END
	}
	return hr;
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD  tTJSInterCodeContext::IsInstanceOf(tjs_uint32 flag,
	const tjs_char *membername, tjs_uint32 *hint, const tjs_char *classname,
		iTJSDispatch2 *objthis)
{
	if(!GetValidity())
		return TJS_E_INVALIDOBJECT;

	if(membername == NULL)
	{
		switch(ContextType)
		{
		case ctTopLevel:
		case ctPropertySetter:
		case ctPropertyGetter:
		case ctSuperClassGetter:
			break;

		case ctFunction:
		case ctExprFunction:
			if(!TJS_strcmp(classname, TJS_W("Function"))) return TJS_S_TRUE;
			break;

		case ctProperty:
			if(!TJS_strcmp(classname, TJS_W("Property"))) return TJS_S_TRUE;
			break;
			
		case ctClass:
			if(!TJS_strcmp(classname, TJS_W("Class"))) return TJS_S_TRUE;
			break;
		}
	}

	tjs_error hr = inherited::IsInstanceOf(flag, membername, hint,
		classname, objthis);

	if(membername != NULL && hr == TJS_E_MEMBERNOTFOUND &&
		ContextType == ctClass && SuperClassGetter)
	{
		// look up super class
		TJS_DO_SUPERCLASS_PROXY_BEGIN
			hr = clo.IsInstanceOf(flag, membername, hint, classname, objthis);
		TJS_DO_SUPERCLASS_PROXY_END
	}
	return hr;
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD
	tTJSInterCodeContext::GetCount(tjs_int *result, const tjs_char *membername,
		tjs_uint32 *hint, iTJSDispatch2 *objthis)
{
	tjs_error hr = inherited::GetCount(result, membername, hint,
		objthis);

	if(membername != NULL && hr == TJS_E_MEMBERNOTFOUND &&
		ContextType == ctClass && SuperClassGetter)
	{
		// look up super class
		TJS_DO_SUPERCLASS_PROXY_BEGIN
			hr = clo.GetCount(result, membername, hint, objthis);
		TJS_DO_SUPERCLASS_PROXY_END
	}
	return hr;

}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD
	tTJSInterCodeContext::DeleteMember(tjs_uint32 flag, const tjs_char *membername,
		tjs_uint32 *hint,  iTJSDispatch2 *objthis)
{
	tjs_error hr = inherited::DeleteMember(flag, membername, hint,
		objthis);

	if(membername != NULL && hr == TJS_E_MEMBERNOTFOUND &&
		ContextType == ctClass && SuperClassGetter)
	{
		// look up super class
		TJS_DO_SUPERCLASS_PROXY_BEGIN
			hr = clo.DeleteMember(flag, membername, hint, objthis);
		TJS_DO_SUPERCLASS_PROXY_END
	}
	return hr;
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD
	tTJSInterCodeContext::Invalidate(tjs_uint32 flag, const tjs_char *membername,
		tjs_uint32 *hint, iTJSDispatch2 *objthis)
{
	tjs_error hr = inherited::Invalidate(flag, membername, hint,
		objthis);

	if(membername != NULL && hr == TJS_E_MEMBERNOTFOUND &&
		ContextType == ctClass && SuperClassGetter)
	{
		// look up super class
		TJS_DO_SUPERCLASS_PROXY_BEGIN
			hr = clo.Invalidate(flag, membername, hint, objthis);
		TJS_DO_SUPERCLASS_PROXY_END
	}
	return hr;
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD
	tTJSInterCodeContext::IsValid(tjs_uint32 flag, const tjs_char *membername,
		tjs_uint32 *hint, iTJSDispatch2 *objthis)
{
	tjs_error hr = inherited::IsValid(flag, membername, hint,
		objthis);

	if(membername != NULL && hr == TJS_E_MEMBERNOTFOUND &&
		ContextType == ctClass && SuperClassGetter)
	{
		// look up super class
		TJS_DO_SUPERCLASS_PROXY_BEGIN
			hr = clo.IsValid(flag, membername, hint, objthis);
		TJS_DO_SUPERCLASS_PROXY_END
	}
	return hr;
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD
	tTJSInterCodeContext::Operation(tjs_uint32 flag, const tjs_char *membername,
		tjs_uint32 *hint, tTJSVariant *result,
			const tTJSVariant *param,	iTJSDispatch2 *objthis)
{
	if(membername == NULL)
	{
		if(ContextType == ctProperty)
		{
			// operation for property object
			return tTJSDispatch::Operation(flag, membername, hint, result, param,
				objthis);
		}
		else
		{
			return inherited::Operation(flag, membername, hint, result, param,
				objthis);
		}
	}

	//tjs_error hr;

	if(membername != NULL && ContextType == ctClass && SuperClassGetter)
	{
		tjs_uint32 pseudo_flag =
			(flag & TJS_IGNOREPROP) ? flag : (flag &~ TJS_MEMBERENSURE);

		tjs_error hr = inherited::Operation(pseudo_flag, membername, hint,
			result, param, objthis);

		if(hr == TJS_E_MEMBERNOTFOUND)
		{
			// look up super class
			TJS_DO_SUPERCLASS_PROXY_BEGIN
				hr = clo.Operation(pseudo_flag, membername, hint, result, param,
					objthis);
			TJS_DO_SUPERCLASS_PROXY_END
		}

		if(hr == TJS_E_MEMBERNOTFOUND)
			hr = inherited::Operation(flag, membername, hint, result,
				param, objthis);

		return hr;
	}
	else
	{
		return inherited::Operation(flag, membername, hint, result, param,
			objthis);
	}
}
//---------------------------------------------------------------------------

}

