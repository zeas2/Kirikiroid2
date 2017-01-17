//---------------------------------------------------------------------------
/*
	TJS2 Script Engine
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// TJS2 "Object" class implementation
//---------------------------------------------------------------------------
#include "tjsCommHead.h"

#include "tjsObject.h"
#include "tjsUtils.h"
#include "tjsNative.h"
#include "tjsHashSearch.h"
#include "tjsGlobalStringMap.h"
#include "tjsDebug.h"


namespace TJS
{

//---------------------------------------------------------------------------
// utility functions
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
void TJSDoVariantOperation(tjs_int op, tTJSVariant &target, const tTJSVariant *param)
{
	switch(op)
	{
	case TJS_OP_BAND:
		target.operator &= (*param);
		return;
	case TJS_OP_BOR:
		target.operator |= (*param);
		return;
	case TJS_OP_BXOR:
		target.operator ^= (*param);
		return;
	case TJS_OP_SUB:
		target.operator -= (*param);
		return;
	case TJS_OP_ADD:
		target.operator += (*param);
		return;
	case TJS_OP_MOD:
		target.operator %= (*param);
		return;
	case TJS_OP_DIV:
		target.operator /= (*param);
		return;
	case TJS_OP_IDIV:
		target.idivequal(*param);
		return;
	case TJS_OP_MUL:
		target.operator *= (*param);
		return;
	case TJS_OP_LOR:
		target.logicalorequal(*param);
		return;
	case TJS_OP_LAND:
		target.logicalandequal(*param);
		return;
	case TJS_OP_SAR:
		target.operator >>= (*param);
		return;
	case TJS_OP_SAL:
		target.operator <<= (*param);
		return;
	case TJS_OP_SR:
		target.rbitshiftequal(*param);
		return;
	case TJS_OP_INC:
		target.increment();
		return;
	case TJS_OP_DEC:
		target.decrement();
		return;
	}

}
//---------------------------------------------------------------------------




/*[C*/
//---------------------------------------------------------------------------
// tTJSDispatch
//---------------------------------------------------------------------------
tTJSDispatch::tTJSDispatch()
{
	BeforeDestructionCalled = false;
	RefCount = 1;
#ifdef TVP_IN_PLUGIN_STUB // TVP plug-in support
	TVPPluginGlobalRefCount++;
#endif
}
//---------------------------------------------------------------------------
tTJSDispatch::~tTJSDispatch()
{
	if(!BeforeDestructionCalled)
	{
		BeforeDestructionCalled = true;
		BeforeDestruction();
	}
}
//---------------------------------------------------------------------------
tjs_uint TJS_INTF_METHOD  tTJSDispatch::AddRef(void)
{
#ifdef TVP_IN_PLUGIN_STUB // TVP plug-in support
	TVPPluginGlobalRefCount++;
#endif
	return ++RefCount;
}
//---------------------------------------------------------------------------
tjs_uint TJS_INTF_METHOD  tTJSDispatch::Release(void)
{
#ifdef TVP_IN_PLUGIN_STUB // TVP plug-in support
	TVPPluginGlobalRefCount--;
#endif
	if(RefCount == 1) // avoid to call "BeforeDestruction" with RefCount == 0
	{
		// object destruction
		if(!BeforeDestructionCalled)
		{
			BeforeDestructionCalled = true;
			BeforeDestruction();
		}

		if(RefCount == 1) // really ready to destruct ?
		{
			delete this;
			return 0;
		}
	}
	return --RefCount;
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD
	tTJSDispatch::FuncCallByNum(
		tjs_uint32 flag,
		tjs_int num,
		tTJSVariant *result,
		tjs_int numparams,
		tTJSVariant **param,
		iTJSDispatch2 *objthis
		)
{
	tjs_char buf[34];
	TJS_int_to_str(num, buf);
	return FuncCall(flag, buf, NULL, result, numparams, param, objthis);
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD
	tTJSDispatch::PropGetByNum(
		tjs_uint32 flag,
		tjs_int num,
		tTJSVariant *result,
		iTJSDispatch2 *objthis
		)
{
	tjs_char buf[34];
	TJS_int_to_str(num, buf);
	return PropGet(flag, buf, NULL, result, objthis);
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD
	tTJSDispatch::PropSetByNum(
		tjs_uint32 flag,
		tjs_int num,
		const tTJSVariant *param,
		iTJSDispatch2 *objthis
		)
{
	tjs_char buf[34];
	TJS_int_to_str(num, buf);
	return PropSet(flag, buf, NULL, param, objthis);
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD
	tTJSDispatch::GetCountByNum(
		tjs_int *result,
		tjs_int num,
		iTJSDispatch2 *objthis
		)
{
	tjs_char buf[34];
	TJS_int_to_str(num, buf);
	return GetCount(result, buf, NULL, objthis);
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD
	tTJSDispatch::DeleteMemberByNum(
		tjs_uint32 flag,
		tjs_int num,
		iTJSDispatch2 *objthis
		)
{
	tjs_char buf[34];
	TJS_int_to_str(num, buf);
	return DeleteMember(flag, buf, NULL, objthis);
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD
	tTJSDispatch::InvalidateByNum(
		tjs_uint32 flag,
		tjs_int num,
		iTJSDispatch2 *objthis
		)
{
	tjs_char buf[34];
	TJS_int_to_str(num, buf);
	return Invalidate(flag, buf, NULL, objthis);
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD
	tTJSDispatch::IsValidByNum(
		tjs_uint32 flag,
		tjs_int num,
		iTJSDispatch2 *objthis
		)
{
	tjs_char buf[34];
	TJS_int_to_str(num, buf);
	return IsValid(flag, buf, NULL, objthis);
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD
	tTJSDispatch::CreateNewByNum(
		tjs_uint32 flag,
		tjs_int num,
		iTJSDispatch2 **result,
		tjs_int numparams,
		tTJSVariant **param,
		iTJSDispatch2 *objthis
		)
{
	tjs_char buf[34];
	TJS_int_to_str(num, buf);
	return CreateNew(flag, buf, NULL, result, numparams, param, objthis);
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD
	tTJSDispatch::IsInstanceOfByNum(
		tjs_uint32 flag,
		tjs_int num,
		const tjs_char *classname,
		iTJSDispatch2 *objthis
		)
{
	tjs_char buf[34];
	TJS_int_to_str(num, buf);
	return IsInstanceOf(flag, buf, NULL, classname, objthis);
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD
	tTJSDispatch::OperationByNum(
		tjs_uint32 flag,
		tjs_int num,
		tTJSVariant *result,
		const tTJSVariant *param,
		iTJSDispatch2 *objthis
		)
{
	tjs_char buf[34];
	TJS_int_to_str(num, buf);
	return Operation(flag, buf, NULL, result, param, objthis);
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD
	tTJSDispatch::Operation(
		tjs_uint32 flag,
		const tjs_char *membername,
		tjs_uint32 *hint,
		tTJSVariant *result,
		const tTJSVariant *param,
		iTJSDispatch2 *objthis
	)
{
	tjs_uint32 op = flag & TJS_OP_MASK;

	if(op!=TJS_OP_INC && op!=TJS_OP_DEC && param == NULL)
		return TJS_E_INVALIDPARAM;

	if(op<TJS_OP_MIN || op>TJS_OP_MAX)
		return TJS_E_INVALIDPARAM;

	tTJSVariant tmp;
	tjs_error hr;
	hr = PropGet(0, membername, hint, &tmp, objthis);
	if(TJS_FAILED(hr)) return hr;

	TJSDoVariantOperation(op, tmp, param);

	hr = PropSet(0, membername, hint, &tmp, objthis);
	if(TJS_FAILED(hr)) return hr;

	if(result) result->CopyRef(tmp);

	return TJS_S_OK;
}
//---------------------------------------------------------------------------
/*C]*/





//---------------------------------------------------------------------------
// property object to get/set missing member
//---------------------------------------------------------------------------
class tTJSSimpleGetSetProperty : public tTJSDispatch
{
private:
	tTJSVariant &Value;

public:
	tTJSSimpleGetSetProperty(tTJSVariant &value) : tTJSDispatch(), Value(value)
	{
	};

	tjs_error TJS_INTF_METHOD
	PropGet(tjs_uint32 flag, const tjs_char * membername, tjs_uint32 *hint,
	 tTJSVariant *result,
		iTJSDispatch2 *objthis)
	{
		if(membername) return TJS_E_MEMBERNOTFOUND;
		if(result) *result = Value;
		return TJS_S_OK;
	}


	tjs_error TJS_INTF_METHOD
	PropSet(tjs_uint32 flag, const tjs_char *membername, tjs_uint32 *hint,
	 const tTJSVariant *param,
		iTJSDispatch2 *objthis)
	{
		if(membername) return TJS_E_MEMBERNOTFOUND;
		Value = *param;
		return TJS_S_OK;
	}

	tjs_error TJS_INTF_METHOD
	PropSetByVS(tjs_uint32 flag, tTJSVariantString *membername,
		const tTJSVariant *param, iTJSDispatch2 *objthis)
	{
		if(membername) return TJS_E_MEMBERNOTFOUND;
		Value = *param;
		return TJS_S_OK;
	}

};
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
// magic number for rebuilding hash
//---------------------------------------------------------------------------
static tjs_uint TJSGlobalRebuildHashMagic = 0;
void TJSDoRehash() { TJSGlobalRebuildHashMagic ++; }
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// tTJSCustomObject
//---------------------------------------------------------------------------
tjs_int TJSObjectHashBitsLimit = 32;
static ttstr FinalizeName;
static ttstr MissingName;
//---------------------------------------------------------------------------
void tTJSCustomObject::tTJSSymbolData::ReShare()
{
	// search shared string map using TJSMapGlobalStringMap,
	// and share the name string (if it can)
	if(Name)
	{
		ttstr name(Name);
		Name->Release(), Name = NULL;
		name = TJSMapGlobalStringMap(name);
		Name = name.AsVariantStringNoAddRef();
		Name->AddRef();
	}
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
tTJSCustomObject::tTJSCustomObject(tjs_int hashbits)
{
	if(TJSObjectHashMapEnabled()) TJSAddObjectHashRecord(this);
	Count = 0;
	RebuildHashMagic = TJSGlobalRebuildHashMagic;
	if(hashbits > TJSObjectHashBitsLimit) hashbits = TJSObjectHashBitsLimit;
	HashSize = (1 << hashbits);
	HashMask = HashSize - 1;
	Symbols = new tTJSSymbolData[HashSize];
	memset(Symbols, 0, sizeof(tTJSSymbolData) * HashSize);
	IsInvalidated = false;
	IsInvalidating = false;
	CallFinalize = true;
	CallMissing = false;
	ProsessingMissing = false;
	if(FinalizeName.IsEmpty())
	{
		// first time; initialize 'finalize' name and 'missing' name
		static ttstr _finalize = TJSMapGlobalStringMap(TJS_W("finalize"));
		static ttstr _missing = TJSMapGlobalStringMap(TJS_W("missing"));
		FinalizeName = _finalize;
		MissingName = _missing;
	}
	finalize_name = FinalizeName;
	missing_name = MissingName;
	for(tjs_int i=0; i<TJS_MAX_NATIVE_CLASS; i++)
		ClassIDs[i] = (tjs_int32)-1;
}
//---------------------------------------------------------------------------
tTJSCustomObject::~tTJSCustomObject()
{
	for(tjs_int i=TJS_MAX_NATIVE_CLASS-1; i>=0; i--)
	{
		if(ClassIDs[i]!=-1)
		{
			if(ClassInstances[i]) ClassInstances[i]->Destruct();
		}
	}
	delete [] Symbols;
	if(TJSObjectHashMapEnabled()) TJSRemoveObjectHashRecord(this);
}
//---------------------------------------------------------------------------
void tTJSCustomObject::_Finalize(void)
{
	if(IsInvalidating) return; // to avoid re-entrance
	IsInvalidating = true;
	try
	{
		if(!IsInvalidated)
		{
			Finalize();
			IsInvalidated = true;
		}
	}
	catch(...)
	{
		IsInvalidating = false;
		throw;
	}
	IsInvalidating = false;
}

//---------------------------------------------------------------------------
void tTJSCustomObject::Finalize(void)
{
	// call this object's "finalize"
	if(CallFinalize)
	{
		FuncCall(0, finalize_name.c_str(), finalize_name.GetHint(), NULL, 0,
			NULL, this);
	}

	for(tjs_int i=TJS_MAX_NATIVE_CLASS-1; i>=0; i--)
	{
		if(ClassIDs[i]!=-1)
		{
			if(ClassInstances[i]) ClassInstances[i]->Invalidate();
		}
	}
	DeleteAllMembers();
}
//---------------------------------------------------------------------------
void tTJSCustomObject::BeforeDestruction(void)
{
	if(TJSObjectHashMapEnabled())
		TJSSetObjectHashFlag(this, TJS_OHMF_DELETING, TJS_OHMF_SET);
	_Finalize();
}
//---------------------------------------------------------------------------
bool tTJSCustomObject::CallGetMissing(const tjs_char *name, tTJSVariant &result)
{
	// call 'missing' method for PopGet
	if(ProsessingMissing) return false;
	ProsessingMissing = true;
	bool res = false;
	try
	{
		tTJSVariant val;
		tTJSSimpleGetSetProperty * prop = new tTJSSimpleGetSetProperty(val);
		try
		{
			tTJSVariant args[3];
			args[0] = (tjs_int) false; // false: get
			args[1] = name;        // member name
			args[2] = prop;
			tTJSVariant *pargs[3] = {args +0, args +1, args +2};
			tTJSVariant funcresult;
			tjs_error er = 
				FuncCall(0, missing_name.c_str(), missing_name.GetHint(), &funcresult,
					3, pargs, this);
			if(TJS_FAILED(er))
			{
				res = false;
			}
			else
			{
				res = 0!=(tjs_int)funcresult;
				result = val;
			}
		}
		catch(...)
		{
			prop->Release();
			throw;
		}
		prop->Release();
	}
	catch(...)
	{
		ProsessingMissing = false;
		throw;
	}
	ProsessingMissing = false;
	return res;
}
//---------------------------------------------------------------------------
bool tTJSCustomObject::CallSetMissing(const tjs_char *name, const tTJSVariant &value)
{
	// call 'missing' method for PopSet
	if(ProsessingMissing) return false;
	ProsessingMissing = true;
	bool res = false;
	try
	{
		tTJSVariant val(value);
		tTJSSimpleGetSetProperty * prop = new tTJSSimpleGetSetProperty(val);
		try
		{
			tTJSVariant args[3];
			args[0] = (tjs_int) true; // true: set
			args[1] = name;        // member name
			args[2] = prop;
			tTJSVariant *pargs[3] = {args +0, args +1, args +2};
			tTJSVariant funcresult;
			tjs_error er = 
				FuncCall(0, missing_name.c_str(), missing_name.GetHint(), &funcresult,
					3, pargs, this);
			if(TJS_FAILED(er))
			{
				res = false;
			}
			else
			{
				res = 0!=(tjs_int)funcresult;
			}
		}
		catch(...)
		{
			prop->Release();
			throw;
		}
		prop->Release();
	}
	catch(...)
	{
		ProsessingMissing = false;
		throw;
	}
	ProsessingMissing = false;
	return res;
}
//---------------------------------------------------------------------------
tTJSCustomObject::tTJSSymbolData * tTJSCustomObject::Add(const tjs_char * name,
	tjs_uint32 *hint)
{
	// add a data element named "name".
	// return existing element if the element named "name" is already alive.

	if(name == NULL)
	{
		return NULL;
	}

	tTJSSymbolData *data;
	data = Find(name, hint);
	if(data)
	{
		// the element is already alive
		return data;
	}

	tjs_uint32 hash;
	if(hint && *hint)
		hash = *hint;  // hint must be hash because of previous calling of "Find"
	else
		hash = tTJSHashFunc<tjs_char *>::Make(name);

	tTJSSymbolData *lv1 = Symbols + (hash & HashMask);

	if((lv1->SymFlags & TJS_SYMBOL_USING))
	{
		// lv1 is using
		// make a chain and insert it after lv1

		data = new tTJSSymbolData;

		data->SelfClear();

		data->Next = lv1->Next;
		lv1->Next = data;

		data->SetName(name, hash);
		data->SymFlags |= TJS_SYMBOL_USING;
	}
	else
	{
		// lv1 is unused
		if(!(lv1->SymFlags & TJS_SYMBOL_INIT))
		{
			lv1->SelfClear();
		}

		lv1->SetName(name, hash);
		lv1->SymFlags |= TJS_SYMBOL_USING;
		data = lv1;
	}

	Count++;


	return data;

}
//---------------------------------------------------------------------------
tTJSCustomObject::tTJSSymbolData * tTJSCustomObject::Add(tTJSVariantString * name)
{
	// tTJSVariantString version of above

	if(name == NULL)
	{
		return NULL;
	}

	tTJSSymbolData *data;
	data = Find((const tjs_char *)(*name), name->GetHint());
	if(data)
	{
		// the element is already alive
		return data;
	}

	tjs_uint32 hash;
	if(*(name->GetHint()))
		hash = *(name->GetHint());  // hint must be hash because of previous calling of "Find"
	else
		hash = tTJSHashFunc<tjs_char *>::Make((const tjs_char *)(*name));

	tTJSSymbolData *lv1 = Symbols + (hash & HashMask);

	if((lv1->SymFlags & TJS_SYMBOL_USING))
	{
		// lv1 is using
		// make a chain and insert it after lv1

		data = new tTJSSymbolData;

		data->SelfClear();

		data->Next = lv1->Next;
		lv1->Next = data;

		data->SetName(name, hash);
		data->SymFlags |= TJS_SYMBOL_USING;
	}
	else
	{
		// lv1 is unused
		if(!(lv1->SymFlags & TJS_SYMBOL_INIT))
		{
			lv1->SelfClear();
		}

		lv1->SetName(name, hash);
		lv1->SymFlags |= TJS_SYMBOL_USING;
		data = lv1;
	}

	Count++;


	return data;
}
//---------------------------------------------------------------------------
tTJSCustomObject::tTJSSymbolData * tTJSCustomObject::AddTo(tTJSVariantString *name,
		tTJSSymbolData *newdata, tjs_int newhashmask)
{
	// similar to Add, except for adding member to new hash space.
	if(name == NULL)
	{
		return NULL;
	}

	// at this point, the member must not exist in destination hash space

	tjs_uint32 hash;
	hash = tTJSHashFunc<tjs_char *>::Make((const tjs_char *)(*name));

	tTJSSymbolData *lv1 = newdata + (hash & newhashmask);
	tTJSSymbolData *data;

	if((lv1->SymFlags & TJS_SYMBOL_USING))
	{
		// lv1 is using
		// make a chain and insert it after lv1

		data = new tTJSSymbolData;

		data->SelfClear();

		data->Next = lv1->Next;
		lv1->Next = data;

		data->SetName(name, hash);
		data->SymFlags |= TJS_SYMBOL_USING;
	}
	else
	{
		// lv1 is unused
		if(!(lv1->SymFlags & TJS_SYMBOL_INIT))
		{
			lv1->SelfClear();
		}

		lv1->SetName(name, hash);
		lv1->SymFlags |= TJS_SYMBOL_USING;
		data = lv1;
	}

	// count is not incremented

	return data;
}
//---------------------------------------------------------------------------
#define GetValue(x) (*((tTJSVariant *)(&(x->Value))))
//---------------------------------------------------------------------------
void tTJSCustomObject::RebuildHash()
{
	RebuildHash( Count );
}
//---------------------------------------------------------------------------
void tTJSCustomObject::RebuildHash( tjs_int requestcount )
{
	// rebuild hash table
	RebuildHashMagic = TJSGlobalRebuildHashMagic;

	// decide new hash table size

	tjs_int r, v = requestcount;
	if(v & 0xffff0000) r = 16, v >>= 16; else r = 0;
	if(v & 0xff00) r += 8, v >>= 8;
	if(v & 0xf0) r += 4, v >>= 4;
	v<<=1;
	tjs_int newhashbits = r + ((0xffffaa50 >> v) &0x03) + 2;
	if(newhashbits > TJSObjectHashBitsLimit) newhashbits = TJSObjectHashBitsLimit;
	tjs_int newhashsize = (1 << newhashbits);


	if(newhashsize == HashSize) return;

	tjs_int newhashmask = newhashsize - 1;
	tjs_int orgcount = Count;

	// allocate new hash space
	tTJSSymbolData *newsymbols = new tTJSSymbolData[newhashsize];


	// enumerate current symbol and push to new hash space

	try
	{
		memset(newsymbols, 0, sizeof(tTJSSymbolData) * newhashsize);
		//tjs_int i;
		tTJSSymbolData * lv1 = Symbols;
		tTJSSymbolData * lv1lim = lv1 + HashSize;
		for(; lv1 < lv1lim; lv1++)
		{
			tTJSSymbolData * d = lv1->Next;
			while(d)
			{
				tTJSSymbolData * nextd = d->Next;
				if(d->SymFlags & TJS_SYMBOL_USING)
				{
//					d->ReShare();
					tTJSSymbolData *data = AddTo(d->Name, newsymbols, newhashmask);
					if(data)
					{
						GetValue(data).CopyRef(*(tTJSVariant*)(&(d->Value)));
						data->SymFlags &= ~ (TJS_SYMBOL_HIDDEN | TJS_SYMBOL_STATIC);
						data->SymFlags |= d->SymFlags & (TJS_SYMBOL_HIDDEN | TJS_SYMBOL_STATIC);
						CheckObjectClosureAdd(GetValue(data));
					}
				}
				d = nextd;
			}

			if(lv1->SymFlags & TJS_SYMBOL_USING)
			{
//				lv1->ReShare();
				tTJSSymbolData *data = AddTo(lv1->Name, newsymbols, newhashmask);
				if(data)
				{
					GetValue(data).CopyRef(*(tTJSVariant*)(&(lv1->Value)));
					data->SymFlags &= ~ (TJS_SYMBOL_HIDDEN | TJS_SYMBOL_STATIC);
					data->SymFlags |= lv1->SymFlags & (TJS_SYMBOL_HIDDEN | TJS_SYMBOL_STATIC);
					CheckObjectClosureAdd(GetValue(data));
				}
			}
		}
	}
	catch(...)
	{
		// recover
		tjs_int _HashMask = HashMask;
		tjs_int _HashSize = HashSize;
		tTJSSymbolData * _Symbols = Symbols;

		Symbols = newsymbols;
		HashSize = newhashsize;
		HashMask = newhashmask;

		DeleteAllMembers();
		delete [] Symbols;

		HashMask = _HashMask;
		HashSize = _HashSize;
		Symbols = _Symbols;
		Count = orgcount;

		throw;
	}

	// delete all current members
	DeleteAllMembers();
	delete [] Symbols;

	// assign new members
	Symbols = newsymbols;
	HashSize = newhashsize;
	HashMask = newhashmask;
	Count = orgcount;
}
//---------------------------------------------------------------------------
bool tTJSCustomObject::DeleteByName(const tjs_char * name, tjs_uint32 *hint)
{
	// TODO: utilize hint
	// find an element named "name" and deletes it
	tjs_uint32 hash = tTJSHashFunc<tjs_char *>::Make(name);
	tTJSSymbolData * lv1 = Symbols + (hash & HashMask);

	if(!(lv1->SymFlags & TJS_SYMBOL_USING) && lv1->Next== NULL)
		return false; // not found

	if((lv1->SymFlags & TJS_SYMBOL_USING) && lv1->NameMatch(name))
	{
		// mark the element place as "unused"
		CheckObjectClosureRemove(*(tTJSVariant*)(&(lv1->Value)));
		lv1->PostClear();
		Count--;
		return true;
	}

	// chain processing
	tTJSSymbolData * d = lv1->Next;
	tTJSSymbolData * prevd = lv1;
	while(d)
	{
		if((d->SymFlags & TJS_SYMBOL_USING) && d->Hash == hash)
		{
			if(d->NameMatch(name))
			{
				// sever from the chain
				prevd->Next = d->Next;
				CheckObjectClosureRemove(*(tTJSVariant*)(&(d->Value)));
				d->Destory();

				delete d;

				Count--;
				return true;
			}
		}
		prevd = d;
		d = d->Next;
	}

	return false;
}
//---------------------------------------------------------------------------
void tTJSCustomObject::DeleteAllMembers(void)
{
	// delete all members
	if(Count <= 10) return _DeleteAllMembers();

	std::vector<iTJSDispatch2*> vector;
	try
	{
		tTJSSymbolData * lv1, *lv1lim;

		// list all members up that hold object
		lv1 = Symbols;
		lv1lim = lv1 + HashSize;
		for(; lv1 < lv1lim; lv1++)
		{
			tTJSSymbolData * d = lv1->Next;
			while(d)
			{
				tTJSSymbolData * nextd = d->Next;
				if(d->SymFlags & TJS_SYMBOL_USING)
				{
					if(((tTJSVariant*)(&(d->Value)))->Type() == tvtObject)
					{
						CheckObjectClosureRemove(*(tTJSVariant*)(&(d->Value)));
						tTJSVariantClosure clo =
							((tTJSVariant*)(&(d->Value)))->AsObjectClosureNoAddRef();
						clo.AddRef();
						if(clo.Object) vector.push_back(clo.Object);
						if(clo.ObjThis) vector.push_back(clo.ObjThis);
						((tTJSVariant*)(&(d->Value)))->Clear();
					}
				}
				d = nextd;
			}

			if(lv1->SymFlags & TJS_SYMBOL_USING)
			{
				if(((tTJSVariant*)(&(lv1->Value)))->Type() == tvtObject)
				{
					CheckObjectClosureRemove(*(tTJSVariant*)(&(lv1->Value)));
					tTJSVariantClosure clo =
						((tTJSVariant*)(&(lv1->Value)))->AsObjectClosureNoAddRef();
					clo.AddRef();
					if(clo.Object) vector.push_back(clo.Object);
					if(clo.ObjThis) vector.push_back(clo.ObjThis);
					((tTJSVariant*)(&(lv1->Value)))->Clear();
				}
			}
		}

		// delete all members
		lv1 = Symbols;
		lv1lim = lv1 + HashSize;
		for(; lv1 < lv1lim; lv1++)
		{
			tTJSSymbolData * d = lv1->Next;
			while(d)
			{
				tTJSSymbolData * nextd = d->Next;
				if(d->SymFlags & TJS_SYMBOL_USING)
				{
					d->Destory();
				}

				delete d;

				d = nextd;
			}

			if(lv1->SymFlags & TJS_SYMBOL_USING)
			{
				lv1->PostClear();
			}

			lv1->Next = NULL;
		}

		Count = 0;
	}
	catch(...)
	{
		std::vector<iTJSDispatch2*>::iterator i;
		for(i = vector.begin(); i != vector.end(); i++)
		{
			(*i)->Release();
		}

		throw;
	}

	// release all objects
	std::vector<iTJSDispatch2*>::iterator i;
	for(i = vector.begin(); i != vector.end(); i++)
	{
		(*i)->Release();
	}
}
//---------------------------------------------------------------------------
void tTJSCustomObject::_DeleteAllMembers(void)
{
	iTJSDispatch2 * dsps[20];
	tjs_int num_dsps = 0;

	try
	{
		tTJSSymbolData * lv1, *lv1lim;

		// list all members up that hold object
		lv1 = Symbols;
		lv1lim = lv1 + HashSize;
		for(; lv1 < lv1lim; lv1++)
		{
			tTJSSymbolData * d = lv1->Next;
			while(d)
			{
				tTJSSymbolData * nextd = d->Next;
				if(d->SymFlags & TJS_SYMBOL_USING)
				{
					if(((tTJSVariant*)(&(d->Value)))->Type() == tvtObject)
					{
						CheckObjectClosureRemove(*(tTJSVariant*)(&(d->Value)));
						tTJSVariantClosure clo =
							((tTJSVariant*)(&(d->Value)))->AsObjectClosureNoAddRef();
						clo.AddRef();
						if(clo.Object) dsps[num_dsps++] = clo.Object;
						if(clo.ObjThis) dsps[num_dsps++] = clo.ObjThis;
						((tTJSVariant*)(&(d->Value)))->Clear();
					}
				}
				d = nextd;
			}

			if(lv1->SymFlags & TJS_SYMBOL_USING)
			{
				if(((tTJSVariant*)(&(lv1->Value)))->Type() == tvtObject)
				{
					CheckObjectClosureRemove(*(tTJSVariant*)(&(lv1->Value)));
					tTJSVariantClosure clo =
						((tTJSVariant*)(&(lv1->Value)))->AsObjectClosureNoAddRef();
					clo.AddRef();
					if(clo.Object) dsps[num_dsps++] = clo.Object;
					if(clo.ObjThis) dsps[num_dsps++] = clo.ObjThis;
					((tTJSVariant*)(&(lv1->Value)))->Clear();
				}
			}
		}

		// delete all members
		lv1 = Symbols;
		lv1lim = lv1 + HashSize;
		for(; lv1 < lv1lim; lv1++)
		{
			tTJSSymbolData * d = lv1->Next;
			while(d)
			{
				tTJSSymbolData * nextd = d->Next;
				if(d->SymFlags & TJS_SYMBOL_USING)
				{
					d->Destory();
				}

				delete d;

				d = nextd;
			}

			if(lv1->SymFlags & TJS_SYMBOL_USING)
			{
				lv1->PostClear();
			}

			lv1->Next = NULL;
		}

		Count = 0;
	}
	catch(...)
	{
		for(int i = 0; i<num_dsps; i++)
		{
			dsps[i]->Release();
		}
		throw;
	}

	// release all objects
	for(int i = 0; i<num_dsps; i++)
	{
		dsps[i]->Release();
	}

}
//---------------------------------------------------------------------------
tTJSCustomObject::tTJSSymbolData * tTJSCustomObject::Find(const tjs_char * name,
	tjs_uint32 *hint)
{
	// searche an element named "name" and return its "SymbolData".
	// return NULL if the element is not found.

	if(!name) return NULL;

	if(hint && *hint)
	{
		// try finding via hint
		// search over the chain
		tjs_uint32 hash = *hint;
		tjs_int cnt = 0;
		tTJSSymbolData * lv1 = Symbols + (hash & HashMask);
		tTJSSymbolData * prevd = lv1;
		tTJSSymbolData * d = lv1->Next;
		for(; d; prevd = d, d=d->Next, cnt++)
		{
			if(d->Hash == hash && (d->SymFlags & TJS_SYMBOL_USING))
			{
				if(d->NameMatch(name))
				{
					if(cnt>2)
					{
						// move to first
						prevd->Next = d->Next;
						d->Next = lv1->Next;
						lv1->Next = d;
					}
					return d;
				}
			}
		}

		if(lv1->Hash == hash && (lv1->SymFlags & TJS_SYMBOL_USING))
		{
			if(lv1->NameMatch(name))
			{
				return lv1;
			}
		}
	}

	tjs_uint32 hash = tTJSHashFunc<tjs_char *>::Make(name);
	if(hint && *hint)
	{
		if(*hint == hash) return NULL;
			// given hint was not differ from the hash;
			// we already know that the member was not found.
	}

	if(hint) *hint = hash;

	tTJSSymbolData * lv1 = Symbols + (hash & HashMask);

	if(!(lv1->SymFlags & TJS_SYMBOL_USING) && lv1->Next == NULL)
		return NULL; // lv1 is unused and does not have any chains

	// search over the chain
	tjs_int cnt = 0;
	tTJSSymbolData * prevd = lv1;
	tTJSSymbolData * d = lv1->Next;
	for(; d; prevd = d, d=d->Next, cnt++)
	{
		if(d->Hash == hash && (d->SymFlags & TJS_SYMBOL_USING))
		{
			if(d->NameMatch(name))
			{
				if(cnt>2)
				{
					// move to first
					prevd->Next = d->Next;
					d->Next = lv1->Next;
					lv1->Next = d;
				}
				return d;
			}
		}
	}

	if(lv1->Hash == hash && (lv1->SymFlags & TJS_SYMBOL_USING))
	{
		if(lv1->NameMatch(name))
		{
			return lv1;
		}
	}

	return NULL;

}
//---------------------------------------------------------------------------
bool tTJSCustomObject::CallEnumCallbackForData(
	tjs_uint32 flags, tTJSVariant ** params,
	tTJSVariantClosure & callback, iTJSDispatch2 * objthis,
	const tTJSCustomObject::tTJSSymbolData * data)
{
	tjs_uint32 newflags = 0;
	if(data->SymFlags & TJS_SYMBOL_HIDDEN) newflags |= TJS_HIDDENMEMBER;
	if(data->SymFlags & TJS_SYMBOL_STATIC) newflags |= TJS_STATICMEMBER;

	*params[0] = data->Name;
	*params[1] = (tjs_int)newflags;

	if(!(flags & TJS_ENUM_NO_VALUE))
	{
		// get value
		if(TJS_FAILED(TJSDefaultPropGet(flags, *(tTJSVariant*)(&(data->Value)),
			params[2], objthis))) return false;
	}

	tTJSVariant res;
	if(TJS_FAILED(callback.FuncCall(NULL, NULL, NULL, &res,
		(flags & TJS_ENUM_NO_VALUE) ? 2 : 3, params, NULL))) return false;
	return 0!=(tjs_int)(res);
}
//---------------------------------------------------------------------------
void tTJSCustomObject::InternalEnumMembers(tjs_uint32 flags,
	tTJSVariantClosure *callback, iTJSDispatch2 *objthis)
{
	// enumlate members by calling callback.
	// note that member changes(delete or insert) through this function is not guaranteed.
	if(!callback) return;

	tTJSVariant name;
	tTJSVariant newflags;
	tTJSVariant value;
	tTJSVariant * params[3] = { &name, &newflags, &value };

	const tTJSSymbolData * lv1 = Symbols;
	const tTJSSymbolData * lv1lim = lv1 + HashSize;
	for(; lv1 < lv1lim; lv1++)
	{
		const tTJSSymbolData * d = lv1->Next;
		while(d)
		{
			const tTJSSymbolData * nextd = d->Next;

			if(d->SymFlags & TJS_SYMBOL_USING)
			{
				if(!CallEnumCallbackForData(flags, params, *callback, objthis, d)) return ;
			}
			d = nextd;
		}

		if(lv1->SymFlags & TJS_SYMBOL_USING)
		{
			if(!CallEnumCallbackForData(flags, params, *callback, objthis, lv1)) return ;
		}
	}
}
//---------------------------------------------------------------------------
tjs_int tTJSCustomObject::GetValueInteger(const tjs_char * name, tjs_uint32 *hint)
{
	tTJSSymbolData *data = Find(name, hint);
	if(!data) return -1;
	return (tjs_int)data->Value.Integer;
}
//---------------------------------------------------------------------------
tjs_error TJSTryFuncCallViaPropGet(tTJSVariantClosure tvclosure,
	tjs_uint32 flag, tTJSVariant *result,
	tjs_int numparams, tTJSVariant **param, iTJSDispatch2 *objthis)
{
	// retry using PropGet
	tTJSVariant tmp;
	tvclosure.AddRef();
	tjs_error er;
	try
	{
		er = tvclosure.Object->PropGet(0, NULL, NULL, &tmp,
			TJS_SELECT_OBJTHIS(tvclosure, objthis));
	}
	catch(...)
	{
		tvclosure.Release();
		throw;
	}
	tvclosure.Release();

	if(TJS_SUCCEEDED(er))
	{
		tvclosure = tmp.AsObjectClosure();
		er =
			tvclosure.Object->FuncCall(flag, NULL, NULL, result,
				numparams, param, TJS_SELECT_OBJTHIS(tvclosure, objthis));
		tvclosure.Release();
	}
	return er;
}
//---------------------------------------------------------------------------
tjs_error TJSDefaultFuncCall(tjs_uint32 flag, tTJSVariant &targ, tTJSVariant *result,
	tjs_int numparams, tTJSVariant **param, iTJSDispatch2 *objthis)
{
	if(targ.Type() == tvtObject)
	{
		tjs_error er = TJS_E_INVALIDTYPE;
		tTJSVariantClosure tvclosure = targ.AsObjectClosure();
		try
		{
			if(tvclosure.Object)
			{
				// bypass
				er =
					tvclosure.Object->FuncCall(flag, NULL, NULL, result,
						numparams, param, TJS_SELECT_OBJTHIS(tvclosure, objthis));
				if(er == TJS_E_INVALIDTYPE)
				{
					// retry using PropGet
					er = TJSTryFuncCallViaPropGet(tvclosure, flag, result,
						numparams, param, objthis);
				}
			}
		}
		catch(...)
		{
			tvclosure.Release();
			throw;
		}
		tvclosure.Release();
		return er;
	}

	return TJS_E_INVALIDTYPE;
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD
tTJSCustomObject::FuncCall(tjs_uint32 flag, const tjs_char * membername, tjs_uint32 *hint,
	tTJSVariant *result, tjs_int numparams, tTJSVariant **param, iTJSDispatch2 *objthis)
{
	if(!GetValidity())
		return TJS_E_INVALIDOBJECT;

	if(membername == NULL)
	{
		// this function is called as to call a default method,
		// but this object is not a function.
		return TJS_E_INVALIDTYPE; // so returns TJS_E_INVALIDTYPE
	}

	tTJSSymbolData *data =  Find(membername, hint);

	if(!data)
	{
		if(CallMissing)
		{
			// call 'missing' method
			tTJSVariant value_func;
			if(CallGetMissing(membername, value_func))
				return TJSDefaultFuncCall(flag, value_func, result, numparams, param, objthis);
		}

		return TJS_E_MEMBERNOTFOUND; // member not found
	}

	return TJSDefaultFuncCall(flag, GetValue(data), result, numparams, param, objthis);
}
//---------------------------------------------------------------------------
tjs_error TJSDefaultPropGet(tjs_uint32 flag, tTJSVariant &targ, tTJSVariant *result,
	iTJSDispatch2 *objthis)
{
	if(!(flag & TJS_IGNOREPROP))
	{
		// if TJS_IGNOREPROP is not specified

		// if member's type is tvtObject, call the object's PropGet with "member=NULL"
		//  ( default member invocation ). if it is succeeded, return its return value.
		// if the PropGet's return value is TJS_E_ACCESSDENYED,
		// return as an error, otherwise return the member itself.
		if(targ.Type() == tvtObject)
		{
			tTJSVariantClosure tvclosure = targ.AsObjectClosure();
			tjs_error hr = TJS_E_NOTIMPL;
			try
			{
				if(tvclosure.Object)
				{
					hr = tvclosure.Object->PropGet(0, NULL, NULL, result,
						TJS_SELECT_OBJTHIS(tvclosure, objthis));
				}
			}
			catch(...)
			{
				tvclosure.Release();
				throw;
			}
			tvclosure.Release();
			if(TJS_SUCCEEDED(hr)) return hr;
			if(hr != TJS_E_NOTIMPL && hr != TJS_E_INVALIDTYPE &&
				hr != TJS_E_INVALIDOBJECT)
				return hr;
		}
	}

	// return the member itself
	if(!result) return TJS_E_INVALIDPARAM;

	result->CopyRef(targ);

	return TJS_S_OK;
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD
tTJSCustomObject::PropGet(tjs_uint32 flag, const tjs_char * membername, tjs_uint32 *hint,
	tTJSVariant *result,
	iTJSDispatch2 *objthis)
{
	if(RebuildHashMagic != TJSGlobalRebuildHashMagic)
	{
		RebuildHash();
	}


	if(!GetValidity())
		return TJS_E_INVALIDOBJECT;

	if(membername == NULL)
	{
		// this object itself has no information on PropGet with membername == NULL
		return TJS_E_INVALIDTYPE;
	}


	tTJSSymbolData * data = Find(membername, hint);
	if(!data)
	{
		if(CallMissing)
		{
			// call 'missing' method
			tTJSVariant value;
			if(CallGetMissing(membername, value))
				return TJSDefaultPropGet(flag, value, result, objthis);
		}
	}

	if(!data && flag & TJS_MEMBERENSURE)
	{
		// create a member when TJS_MEMBERENSURE is specified
		data = Add(membername, hint);
	}

	if(!data) return TJS_E_MEMBERNOTFOUND; // not found

	return TJSDefaultPropGet(flag, GetValue(data), result, objthis);
}
//---------------------------------------------------------------------------
tjs_error TJSDefaultPropSet(tjs_uint32 flag, tTJSVariant &targ, const tTJSVariant *param,
	iTJSDispatch2 *objthis)
{
	if(!(flag & TJS_IGNOREPROP))
	{
		if(targ.Type() == tvtObject)
		{
			// roughly the same as TJSDefaultPropGet
			tTJSVariantClosure tvclosure = targ.AsObjectClosure();
			tjs_error hr = TJS_E_NOTIMPL;
			try
			{
				if(tvclosure.Object)
				{
					hr = tvclosure.Object->PropSet(0, NULL, NULL, param,
						TJS_SELECT_OBJTHIS(tvclosure, objthis));
				}
			}
			catch(...)
			{
				tvclosure.Release();
				throw;
			}
			tvclosure.Release();
			if(TJS_SUCCEEDED(hr)) return hr;
			if(hr != TJS_E_NOTIMPL && hr != TJS_E_INVALIDTYPE &&
				hr != TJS_E_INVALIDOBJECT)
				return hr;
		}
	}

	// normal substitution
	if(!param) return TJS_E_INVALIDPARAM;

	targ.CopyRef(*param);

	return TJS_S_OK;

}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD
tTJSCustomObject::PropSet(tjs_uint32 flag, const tjs_char *membername, tjs_uint32 *hint,
	const tTJSVariant *param, iTJSDispatch2 *objthis)
{
	if(!GetValidity())
		return TJS_E_INVALIDOBJECT;

	if(membername == NULL)
	{
		// no action is defined with the default member
		return TJS_E_INVALIDTYPE;
	}

	tTJSSymbolData * data;
	if(CallMissing)
	{
		data = Find(membername, hint);
		if(!data)
		{
			// call 'missing' method
			if(CallSetMissing(membername, *param))
				return TJS_S_OK;
		}
	}

	if(flag & TJS_MEMBERENSURE)
		data = Add(membername, hint); // create a member when TJS_MEMBERENSURE is specified
	else
		data = Find(membername, hint);

	if(!data) return TJS_E_MEMBERNOTFOUND; // not found

	if(flag & TJS_HIDDENMEMBER)
		data->SymFlags |= TJS_SYMBOL_HIDDEN;
	else
		data->SymFlags &= ~TJS_SYMBOL_HIDDEN;

	if(flag & TJS_STATICMEMBER)
		data->SymFlags |= TJS_SYMBOL_STATIC;
	else
		data->SymFlags &= ~TJS_SYMBOL_STATIC;

	//-- below is mainly the same as TJSDefaultPropSet

	if(!(flag & TJS_IGNOREPROP))
	{
		if(GetValue(data).Type() == tvtObject)
		{
			tTJSVariantClosure tvclosure =
				GetValue(data).AsObjectClosureNoAddRef();
			if(tvclosure.Object)
			{
				tjs_error hr = tvclosure.Object->PropSet(0, NULL, NULL, param,
					TJS_SELECT_OBJTHIS(tvclosure, objthis));
				if(TJS_SUCCEEDED(hr)) return hr;
				if(hr != TJS_E_NOTIMPL && hr != TJS_E_INVALIDTYPE &&
					hr != TJS_E_INVALIDOBJECT)
					return hr;
			}
			data = Find(membername, hint);
		}
	}


	if(!param) return TJS_E_INVALIDPARAM;

	CheckObjectClosureRemove(GetValue(data));
	try
	{
		GetValue(data).CopyRef(*param);
	}
	catch(...)
	{
		CheckObjectClosureAdd(GetValue(data));
		throw;
	}
	CheckObjectClosureAdd(GetValue(data));

	return TJS_S_OK;
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD
tTJSCustomObject::GetCount(tjs_int *result, const tjs_char *membername, tjs_uint32 *hint,
	iTJSDispatch2 *objthis)
{
	if(!GetValidity())
		return TJS_E_INVALIDOBJECT;

	if(!result) return TJS_E_INVALIDPARAM;

	*result = Count;

	return TJS_S_OK;
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD
tTJSCustomObject::PropSetByVS(tjs_uint32 flag, tTJSVariantString *membername,
	const tTJSVariant *param, iTJSDispatch2 *objthis)
{
	if(!GetValidity())
		return TJS_E_INVALIDOBJECT;

	if(membername == NULL)
	{
		// no action is defined with the default member
		return TJS_E_INVALIDTYPE;
	}

	tTJSSymbolData * data;
	if(CallMissing)
	{
		data = Find((const tjs_char *)(*membername), membername->GetHint());
		if(!data)
		{
			// call 'missing' method
			if(CallSetMissing((const tjs_char *)(*membername), *param))
				return TJS_S_OK;
		}
	}

	if(flag & TJS_MEMBERENSURE)
		data = Add(membername); // create a member when TJS_MEMBERENSURE is specified
	else
		data = Find((const tjs_char *)(*membername), membername->GetHint());

	if(!data) return TJS_E_MEMBERNOTFOUND; // not found

	if(flag & TJS_HIDDENMEMBER)
		data->SymFlags |= TJS_SYMBOL_HIDDEN;
	else
		data->SymFlags &= ~TJS_SYMBOL_HIDDEN;

	if(flag & TJS_STATICMEMBER)
		data->SymFlags |= TJS_SYMBOL_STATIC;
	else
		data->SymFlags &= ~TJS_SYMBOL_STATIC;

	//-- below is mainly the same as TJSDefaultPropSet

	if(!(flag & TJS_IGNOREPROP))
	{
		if(GetValue(data).Type() == tvtObject)
		{
			tTJSVariantClosure tvclosure =
				GetValue(data).AsObjectClosureNoAddRef();
			if(tvclosure.Object)
			{
				tjs_error hr = tvclosure.Object->PropSet(0, NULL, NULL, param,
					TJS_SELECT_OBJTHIS(tvclosure, objthis));
				if(TJS_SUCCEEDED(hr)) return hr;
				if(hr != TJS_E_NOTIMPL && hr != TJS_E_INVALIDTYPE &&
					hr != TJS_E_INVALIDOBJECT)
					return hr;
			}
			data = Find((const tjs_char *)(*membername), membername->GetHint());
		}
	}


	if(!param) return TJS_E_INVALIDPARAM;

	CheckObjectClosureRemove(GetValue(data));
	try
	{
		GetValue(data).CopyRef(*param);
	}
	catch(...)
	{
		CheckObjectClosureAdd(GetValue(data));
		throw;
	}
	CheckObjectClosureAdd(GetValue(data));

	return TJS_S_OK;
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD
tTJSCustomObject::EnumMembers(tjs_uint32 flag, tTJSVariantClosure *callback, iTJSDispatch2 *objthis)
{
	if(!GetValidity()) return TJS_E_INVALIDOBJECT;

	InternalEnumMembers(flag, callback, objthis);

	return TJS_S_OK;
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD
tTJSCustomObject::DeleteMember(tjs_uint32 flag, const tjs_char *membername, tjs_uint32 *hint,
	iTJSDispatch2 *objthis)
{
	if(!GetValidity())
		return TJS_E_INVALIDOBJECT;

	if(membername == NULL)
	{
		return TJS_E_MEMBERNOTFOUND;
	}

	if(!DeleteByName(membername, hint)) return TJS_E_MEMBERNOTFOUND;

	return TJS_S_OK;
}
//---------------------------------------------------------------------------
tjs_error TJSDefaultInvalidate(tjs_uint32 flag, tTJSVariant &targ, iTJSDispatch2 *objthis)
{

	if(targ.Type() == tvtObject)
	{
		tTJSVariantClosure tvclosure = targ.AsObjectClosureNoAddRef();
		if(tvclosure.Object)
		{
			// bypass
			return
				tvclosure.Object->Invalidate(flag, NULL, NULL,
					TJS_SELECT_OBJTHIS(tvclosure, objthis));
		}
	}

	return TJS_S_FALSE;
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD
tTJSCustomObject::Invalidate(tjs_uint32 flag, const tjs_char *membername, tjs_uint32 *hint,
	iTJSDispatch2 *objthis)
{
	if(!GetValidity())
		return TJS_E_INVALIDOBJECT;

	if(membername == NULL)
	{
		if(IsInvalidated) return TJS_S_FALSE;
		_Finalize();
		return TJS_S_TRUE;
	}

	tTJSSymbolData * data = Find(membername, hint);

	if(!data)
	{
		if(CallMissing)
		{
			// call 'missing' method
			tTJSVariant value;
			if(CallGetMissing(membername, value))
				return TJSDefaultInvalidate(flag, value, objthis);
		}
	}

	if(!data) return TJS_E_MEMBERNOTFOUND; // not found

	return TJSDefaultInvalidate(flag, GetValue(data), objthis);
}
//---------------------------------------------------------------------------
tjs_error TJSDefaultIsValid(tjs_uint32 flag, tTJSVariant &targ, iTJSDispatch2 * objthis)
{
	if(targ.Type() == tvtObject)
	{
		tTJSVariantClosure tvclosure =targ.AsObjectClosureNoAddRef();
		if(tvclosure.Object)
		{
			// bypass
			return
				tvclosure.Object->IsValid(flag, NULL, NULL,
					TJS_SELECT_OBJTHIS(tvclosure, objthis));
		}
	}

	// the target type is not tvtObject
	return TJS_S_TRUE;
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD
tTJSCustomObject::IsValid(tjs_uint32 flag, const tjs_char *membername, tjs_uint32 *hint,
	iTJSDispatch2 *objthis)
{
	if(membername == NULL)
	{
		if(IsInvalidated) return TJS_S_FALSE;
		return TJS_S_TRUE;
	}

	tTJSSymbolData * data = Find(membername, hint);

	if(!data)
	{
		if(CallMissing)
		{
			// call 'missing' method
			tTJSVariant value;
			if(CallGetMissing(membername, value))
				return TJSDefaultIsValid(flag, value, objthis);
		}
	}

	if(!data) return TJS_E_MEMBERNOTFOUND; // not found

	return TJSDefaultIsValid(flag, GetValue(data), objthis);
}
//---------------------------------------------------------------------------
tjs_error TJSDefaultCreateNew(tjs_uint32 flag, tTJSVariant &targ,
	iTJSDispatch2 **result, tjs_int numparams, tTJSVariant **param,
	iTJSDispatch2 *objthis)
{
	if(targ.Type() == tvtObject)
	{
		tTJSVariantClosure tvclosure = targ.AsObjectClosureNoAddRef();
		if(tvclosure.Object)
		{
			// bypass
			return
				tvclosure.Object->CreateNew(flag, NULL, NULL, result, numparams,
					param, TJS_SELECT_OBJTHIS(tvclosure, objthis));
		}
	}

	return TJS_E_INVALIDTYPE;
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD
tTJSCustomObject::CreateNew(tjs_uint32 flag, const tjs_char * membername, tjs_uint32 *hint,
	iTJSDispatch2 **result, tjs_int numparams, tTJSVariant **param,
	iTJSDispatch2 *objthis)
{
	if(!GetValidity())
		return TJS_E_INVALIDOBJECT;

	if(membername == NULL)
	{
		// as an action of the default member, this object cannot create an object
		// because this object is not a class
		return TJS_E_INVALIDTYPE;
	}

	tTJSSymbolData * data = Find(membername, hint);

	if(!data)
	{
		if(CallMissing)
		{
			// call 'missing' method
			tTJSVariant value;
			if(CallGetMissing(membername, value))
				return TJSDefaultCreateNew(flag, value, result, numparams, param, objthis);
		}
	}

	if(!data) return TJS_E_MEMBERNOTFOUND; // not found

	return TJSDefaultCreateNew(flag, GetValue(data), result, numparams, param, objthis);
}
//---------------------------------------------------------------------------
/*
tjs_error TJS_INTF_METHOD
tTJSCustomObject::GetSuperClass(tjs_uint32 flag, iTJSDispatch2 **result,
		iTJSDispatch2 *objthis)
{
	// TODO: GetSuperClass's reason for being
	if(!GetValidity())
		return TJS_E_INVALIDOBJECT;

	return TJS_E_NOTIMPL;
}
*/
//---------------------------------------------------------------------------
tjs_error TJSDefaultIsInstanceOf(tjs_uint32 flag, tTJSVariant &targ, const tjs_char *name,
	iTJSDispatch2 *objthis)
{
	tTJSVariantType vt;
	vt = targ.Type();
	if(vt == tvtVoid)
	{
		return TJS_S_FALSE;
	}

	if(!TJS_strcmp(name, TJS_W("Object"))) return TJS_S_TRUE;


	switch(vt)
	{
	case tvtVoid:
		return TJS_S_FALSE; // returns always false about tvtVoid
	case tvtInteger:
	case tvtReal:
		if(!TJS_strcmp(name, TJS_W("Number"))) return TJS_S_TRUE;
		return TJS_S_FALSE;
	case tvtString:
		if(!TJS_strcmp(name, TJS_W("String"))) return TJS_S_TRUE;
		return TJS_S_FALSE;
	case tvtOctet:
		if(!TJS_strcmp(name, TJS_W("Octet"))) return TJS_S_TRUE;
		return TJS_S_FALSE;
	case tvtObject:
		if(vt == tvtObject)
		{
			tTJSVariantClosure tvclosure =targ.AsObjectClosureNoAddRef();
			if(tvclosure.Object)
			{
				// bypass
				return
					tvclosure.Object->IsInstanceOf(flag, NULL, NULL, name,
						TJS_SELECT_OBJTHIS(tvclosure, objthis));
			}
			return TJS_S_FALSE;
		}
	}

	return TJS_S_FALSE;
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD
tTJSCustomObject::IsInstanceOf(tjs_uint32 flag, const tjs_char *membername, tjs_uint32 *hint,
	const tjs_char *classname, iTJSDispatch2 *objthis)
{
	if(!GetValidity())
		return TJS_E_INVALIDOBJECT;

	if(membername == NULL)
	{
		// always returns true if "Object" is specified
		if(!TJS_strcmp(classname, TJS_W("Object")))
		{
			return TJS_S_TRUE;
		}

		// look for the class instance information
		for(tjs_uint i=0; i<ClassNames.size(); i++)
		{
			if(!TJS_strcmp(ClassNames[i].c_str(), classname))
			{
				return TJS_S_TRUE;
			}
		}

		return TJS_S_FALSE;
	}

	tTJSSymbolData * data = Find(membername, hint);

	if(!data)
	{
		if(CallMissing)
		{
			// call 'missing' method
			tTJSVariant value;
			if(CallGetMissing(membername, value))
				return TJSDefaultIsInstanceOf(flag, value, classname, objthis);
		}
	}

	if(!data) return TJS_E_MEMBERNOTFOUND; // not found

	return TJSDefaultIsInstanceOf(flag, GetValue(data), classname, objthis);
}
//---------------------------------------------------------------------------
tjs_error TJSDefaultOperation(tjs_uint32 flag, tTJSVariant &targ,
	tTJSVariant *result, const tTJSVariant *param, iTJSDispatch2 *objthis)
{
	tjs_uint32 op = flag & TJS_OP_MASK;

	if(op!=TJS_OP_INC && op!=TJS_OP_DEC && param == NULL)
		return TJS_E_INVALIDPARAM;

	if(op<TJS_OP_MIN || op>TJS_OP_MAX)
		return TJS_E_INVALIDPARAM;

	if(targ.Type() == tvtObject)
	{
		// the member may be a property handler if the member's type is "tvtObject"
		// so here try to access the object.
		tjs_error hr;

		tTJSVariantClosure tvclosure;
		tvclosure = targ.AsObjectClosureNoAddRef();
		if(tvclosure.Object)
		{
			iTJSDispatch2 * ot = TJS_SELECT_OBJTHIS(tvclosure, objthis);

			tTJSVariant tmp;
			hr = tvclosure.Object->PropGet(0, NULL, NULL, &tmp, ot);
			if(TJS_SUCCEEDED(hr))
			{
				TJSDoVariantOperation(op, tmp, param);

				hr = tvclosure.Object->PropSet(0, NULL, NULL, &tmp, ot);
				if(TJS_FAILED(hr)) return hr;

				if(result) result->CopyRef(tmp);

				return TJS_S_OK;
			}
			else if(hr != TJS_E_NOTIMPL && hr != TJS_E_INVALIDTYPE &&
				hr != TJS_E_INVALIDOBJECT)
			{
				return hr;
			}

			// normal operation is proceeded if "PropGet" is failed.
		}
	}

	TJSDoVariantOperation(op, targ, param);

	if(result) result->CopyRef(targ);

	return TJS_S_OK;
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD
tTJSCustomObject::Operation(tjs_uint32 flag, const tjs_char *membername, tjs_uint32 *hint,
	tTJSVariant *result, const tTJSVariant *param, iTJSDispatch2 *objthis)
{
	if(!GetValidity())
		return TJS_E_INVALIDOBJECT;

	// operation about the member
	// processing line is the same as above function

	if(membername == NULL)
	{
		return TJS_E_INVALIDTYPE;
	}

	tjs_uint32 op = flag & TJS_OP_MASK;

	if(op!=TJS_OP_INC && op!=TJS_OP_DEC && param == NULL)
		return TJS_E_INVALIDPARAM;

	if(op<TJS_OP_MIN || op>TJS_OP_MAX)
		return TJS_E_INVALIDPARAM;

	tTJSSymbolData * data = Find(membername, hint);

	if(!data)
	{
		if(CallMissing)
		{
			// call default operation
			return inherited::Operation(flag, membername, hint, result, param, objthis);
		}
	}

	if(!data) return TJS_E_MEMBERNOTFOUND; // not found

	if(GetValue(data).Type() == tvtObject)
	{
		tjs_error hr;

		tTJSVariantClosure tvclosure;
		tvclosure = GetValue(data).AsObjectClosureNoAddRef();
		if(tvclosure.Object)
		{
			iTJSDispatch2 * ot = TJS_SELECT_OBJTHIS(tvclosure, objthis);

			tTJSVariant tmp;
			hr = tvclosure.Object->PropGet(0, NULL, NULL, &tmp, ot);
			if(TJS_SUCCEEDED(hr))
			{
				TJSDoVariantOperation(op, tmp, param);

				hr = tvclosure.Object->PropSet(0, NULL, NULL, &tmp, ot);
				if(TJS_FAILED(hr)) return hr;

				if(result) result->CopyRef(tmp);

				return TJS_S_OK;
			}
			else if(hr != TJS_E_NOTIMPL && hr != TJS_E_INVALIDTYPE &&
				hr != TJS_E_INVALIDOBJECT)
			{
				return hr;
			}
		}
	}


	CheckObjectClosureRemove(GetValue(data));

	tTJSVariant &tmp = GetValue(data);
	try
	{
		TJSDoVariantOperation(op, tmp, param);
	}
	catch(...)
	{
		CheckObjectClosureAdd(GetValue(data));
		throw;
	}
	CheckObjectClosureAdd(GetValue(data));

	if(result) result->CopyRef(tmp);

	return TJS_S_OK;

}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD
tTJSCustomObject::NativeInstanceSupport(tjs_uint32 flag, tjs_int32 classid,
		iTJSNativeInstance **pointer)
{
	if(flag == TJS_NIS_GETINSTANCE)
	{
		// search "classid"
		for(tjs_int i=0; i<TJS_MAX_NATIVE_CLASS; i++)
		{
			if(ClassIDs[i] == classid)
			{
				*pointer = ClassInstances[i];
				return TJS_S_OK;
			}
		}
		return TJS_E_FAIL;
	}

	if(flag == TJS_NIS_REGISTER)
	{
		// search for the empty place
		for(tjs_int i=0; i<TJS_MAX_NATIVE_CLASS; i++)
		{
			if(ClassIDs[i] == -1)
			{
				// found... writes there
				ClassIDs[i] = classid;
				ClassInstances[i] = *pointer;
				return TJS_S_OK;
			}
		}
		return TJS_E_FAIL;
	}

	return TJS_E_NOTIMPL;
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD 
tTJSCustomObject::ClassInstanceInfo(tjs_uint32 flag, tjs_uint num, tTJSVariant *value)
{
	switch(flag)
	{
	case TJS_CII_ADD:
	  {
		// add value
		ttstr name = value->AsStringNoAddRef();
		if(TJSObjectHashMapEnabled() && ClassNames.size() == 0)
			TJSObjectHashSetType(this, TJS_W("instance of class ") + name);
				// First class name is used for the object classname
				// because the order of the class name
				// registration is from descendant to ancestor.
		ClassNames.push_back(name);
		return TJS_S_OK;
	  }

	case TJS_CII_GET:
	  {
		// get value
		if(num>=ClassNames.size()) return TJS_E_FAIL;
		*value = ClassNames[num];
		return TJS_S_OK;
	  }

	case TJS_CII_SET_FINALIZE:
	  {
		// set 'finalize' method name
		finalize_name = *value;
		CallFinalize = !finalize_name.IsEmpty();
		return TJS_S_OK;
	  }

	case TJS_CII_SET_MISSING:
	  {
		// set 'missing' method name
		missing_name = *value;
		CallMissing = !missing_name.IsEmpty();
		return TJS_S_OK;
	  }


	}

	return TJS_E_NOTIMPL;
}
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
// TJSCreateCustomObject
//---------------------------------------------------------------------------
iTJSDispatch2 * TJSCreateCustomObject()
{
	// utility function; returns newly created empty tTJSCustomObject
	return new tTJSCustomObject();
}
//---------------------------------------------------------------------------


}

