//---------------------------------------------------------------------------
/*
	TJS2 Script Engine
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// TJS2 "Object" class implementation
//---------------------------------------------------------------------------
#ifndef tjsObjectH
#define tjsObjectH

#include <vector>
#include "tjsInterface.h"
#include "tjsVariant.h"
#include "tjsUtils.h"
#include "tjsError.h"

namespace TJS
{
//---------------------------------------------------------------------------
// utility functions
//---------------------------------------------------------------------------
extern tjs_error
	TJSDefaultFuncCall(tjs_uint32 flag, tTJSVariant &targ, tTJSVariant *result,
		tjs_int numparams, tTJSVariant **param, iTJSDispatch2 *objthis);

extern tjs_error TJSDefaultPropGet(tjs_uint32 flag, tTJSVariant &targ, tTJSVariant *result,
	iTJSDispatch2 *objthis);

extern tjs_error TJSDefaultPropSet(tjs_uint32 flag, tTJSVariant &targ, const tTJSVariant *param,
	iTJSDispatch2 *objthis);

extern tjs_error
	TJSDefaultInvalidate(tjs_uint32 flag, tTJSVariant &targ, iTJSDispatch2 *objthis);

extern tjs_error
	TJSDefaultIsValid(tjs_uint32 flag, tTJSVariant &targ, iTJSDispatch2 * objthis);

extern tjs_error
	TJSDefaultCreateNew(tjs_uint32 flag, tTJSVariant &targ,
		iTJSDispatch2 **result, tjs_int numparams, tTJSVariant **param,
		iTJSDispatch2 *objthis);

extern tjs_error
	TJSDefaultIsInstanceOf(tjs_uint32 flag, tTJSVariant &targ, const tjs_char *name,
		iTJSDispatch2 *objthis);

extern tjs_error
	TJSDefaultOperation(tjs_uint32 flag, tTJSVariant &targ,
		tTJSVariant *result, const tTJSVariant *param, iTJSDispatch2 *objthis);

TJS_EXP_FUNC_DEF(void, TJSDoVariantOperation, (tjs_int op, tTJSVariant &target, const tTJSVariant *param));
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// hash rebuilding
//---------------------------------------------------------------------------
TJS_EXP_FUNC_DEF(void, TJSDoRehash, ());
//---------------------------------------------------------------------------



/*[*/
//---------------------------------------------------------------------------
// tTJSDispatch
//---------------------------------------------------------------------------
/*
	tTJSDispatch is a base class that implements iTJSDispatch2, and every methods.
	most methods are maked as simply returning "TJS_E_NOTIMPL";
	methods, those access the object by index, call another methods that access
	the object by string.
*/
/*
#define TJS_SELECT_OBJTHIS(__closure__, __override__) \
	((__closure__).ObjThis?((__override__)?(__override__):(__closure__).ObjThis):(__override__))
*/
#define TJS_SELECT_OBJTHIS(__closure__, __override__) \
	((__closure__).ObjThis?(__closure__).ObjThis:(__override__))

class tTJSDispatch : public iTJSDispatch2
{
	virtual void BeforeDestruction(void) {;}
	bool BeforeDestructionCalled;
		// BeforeDestruction will be certainly called before object destruction
private:
	tjs_uint RefCount;
public:
	tTJSDispatch();
	virtual ~tTJSDispatch();

//	bool DestructionTrace;

public:
	tjs_uint TJS_INTF_METHOD  AddRef(void);
	tjs_uint TJS_INTF_METHOD  Release(void);

protected:
	tjs_uint GetRefCount() { return RefCount; }

public:
	tjs_error TJS_INTF_METHOD
	FuncCall(
		tjs_uint32 flag,
		const tjs_char * membername,
		tjs_uint32 *hint,
		tTJSVariant *result,
		tjs_int numparams,
		tTJSVariant **param,
		iTJSDispatch2 *objthis
		)
	{
		return membername?TJS_E_MEMBERNOTFOUND:TJS_E_NOTIMPL;
	}

	tjs_error TJS_INTF_METHOD
	FuncCallByNum(
		tjs_uint32 flag,
		tjs_int num,
		tTJSVariant *result,
		tjs_int numparams,
		tTJSVariant **param,
		iTJSDispatch2 *objthis
		);

	tjs_error TJS_INTF_METHOD
	PropGet(
		tjs_uint32 flag,
		const tjs_char * membername,
		tjs_uint32 *hint,
		tTJSVariant *result,
		iTJSDispatch2 *objthis
		)
	{
		return membername?TJS_E_MEMBERNOTFOUND:TJS_E_NOTIMPL;
	}

	tjs_error TJS_INTF_METHOD
	PropGetByNum(
		tjs_uint32 flag,
		tjs_int num,
		tTJSVariant *result,
		iTJSDispatch2 *objthis
		);

	tjs_error TJS_INTF_METHOD
	PropSet(
		tjs_uint32 flag,
		const tjs_char *membername,
		tjs_uint32 *hint,
		const tTJSVariant *param,
		iTJSDispatch2 *objthis
		)
	{
		return membername?TJS_E_MEMBERNOTFOUND:TJS_E_NOTIMPL;
	}

	tjs_error TJS_INTF_METHOD
	PropSetByNum(
		tjs_uint32 flag,
		tjs_int num,
		const tTJSVariant *param,
		iTJSDispatch2 *objthis
		);
	
	tjs_error TJS_INTF_METHOD
	GetCount(
		tjs_int *result,
		const tjs_char *membername,
		tjs_uint32 *hint,
		iTJSDispatch2 *objthis
		)
	{
		return TJS_E_NOTIMPL;
	}

	tjs_error TJS_INTF_METHOD
	GetCountByNum(
		tjs_int *result,
		tjs_int num,
		iTJSDispatch2 *objthis
		);


	tjs_error TJS_INTF_METHOD
	PropSetByVS(tjs_uint32 flag, tTJSVariantString *membername,
		const tTJSVariant *param, iTJSDispatch2 *objthis)
	{
		return TJS_E_NOTIMPL;
	}

	tjs_error TJS_INTF_METHOD
	EnumMembers(tjs_uint32 flag, tTJSVariantClosure *callback, iTJSDispatch2 *objthis)
	{
		return TJS_E_NOTIMPL;
	}

	tjs_error TJS_INTF_METHOD
	DeleteMember(
		tjs_uint32 flag,
		const tjs_char *membername,
		tjs_uint32 *hint,
		iTJSDispatch2 *objthis
		)
	{
		return membername?TJS_E_MEMBERNOTFOUND:TJS_E_NOTIMPL;
	}

	tjs_error TJS_INTF_METHOD
	DeleteMemberByNum(
		tjs_uint32 flag,
		tjs_int num,
		iTJSDispatch2 *objthis
		);

	tjs_error TJS_INTF_METHOD
	Invalidate(
		tjs_uint32 flag,
		const tjs_char *membername,
		tjs_uint32 *hint,
		iTJSDispatch2 *objthis
		)
	{
		return membername?TJS_E_MEMBERNOTFOUND:TJS_E_NOTIMPL;
	}

	tjs_error TJS_INTF_METHOD
	InvalidateByNum(
		tjs_uint32 flag,
		tjs_int num,
		iTJSDispatch2 *objthis
		);

	tjs_error TJS_INTF_METHOD
	IsValid(
		tjs_uint32 flag,
		const tjs_char *membername,
		tjs_uint32 *hint,
		iTJSDispatch2 *objthis
		)
	{
		return membername?TJS_E_MEMBERNOTFOUND:TJS_E_NOTIMPL;
	}

	tjs_error TJS_INTF_METHOD
	IsValidByNum(
		tjs_uint32 flag,
		tjs_int num,
		iTJSDispatch2 *objthis
		);

	tjs_error TJS_INTF_METHOD
	CreateNew(
		tjs_uint32 flag,
		const tjs_char * membername,
		tjs_uint32 *hint,
		iTJSDispatch2 **result,
		tjs_int numparams,
		tTJSVariant **param,
		iTJSDispatch2 *objthis
		)
	{
		return membername?TJS_E_MEMBERNOTFOUND:TJS_E_NOTIMPL;
	}

	tjs_error TJS_INTF_METHOD
	CreateNewByNum(
		tjs_uint32 flag,
		tjs_int num,
		iTJSDispatch2 **result,
		tjs_int numparams,
		tTJSVariant **param,
		iTJSDispatch2 *objthis
		);

	tjs_error TJS_INTF_METHOD
	Reserved1(
		)
	{
		return TJS_E_NOTIMPL;
	}


	tjs_error TJS_INTF_METHOD
	IsInstanceOf(
		tjs_uint32 flag,
		const tjs_char *membername,
		tjs_uint32 *hint,
		const tjs_char *classname,
		iTJSDispatch2 *objthis
		)
	{
		return membername?TJS_E_MEMBERNOTFOUND:TJS_E_NOTIMPL;
	}

	tjs_error TJS_INTF_METHOD
	IsInstanceOfByNum(
		tjs_uint32 flag,
		tjs_int num,
		const tjs_char *classname,
		iTJSDispatch2 *objthis
		);

	tjs_error TJS_INTF_METHOD
	Operation(
		tjs_uint32 flag,
		const tjs_char *membername,
		tjs_uint32 *hint,
		tTJSVariant *result,
		const tTJSVariant *param,
		iTJSDispatch2 *objthis
		);

	tjs_error TJS_INTF_METHOD
	OperationByNum(
		tjs_uint32 flag,
		tjs_int num,
		tTJSVariant *result,
		const tTJSVariant *param,
		iTJSDispatch2 *objthis
		);


	tjs_error TJS_INTF_METHOD
	NativeInstanceSupport(
		tjs_uint32 flag,
		tjs_int32 classid,
		iTJSNativeInstance **pointer
		)
	{
		return TJS_E_NOTIMPL;
	}

	tjs_error TJS_INTF_METHOD
	ClassInstanceInfo(
		tjs_uint32 flag,
		tjs_uint num,
		tTJSVariant *value
		)
	{
		return TJS_E_NOTIMPL;
	}

	tjs_error TJS_INTF_METHOD
	Reserved2(
		)
	{
		return TJS_E_NOTIMPL;
	}

	tjs_error TJS_INTF_METHOD
	Reserved3(
		)
	{
		return TJS_E_NOTIMPL;
	}


};
//---------------------------------------------------------------------------
/*]*/




//---------------------------------------------------------------------------
// tTJSCustomObject
//---------------------------------------------------------------------------

#define TJS_NAMESPACE_DEFAULT_HASH_BITS 3

extern tjs_int TJSObjectHashBitsLimit;
	// this limits hash table size


#define TJS_SYMBOL_USING	0x1
#define TJS_SYMBOL_INIT     0x2
#define TJS_SYMBOL_HIDDEN   0x8
#define TJS_SYMBOL_STATIC	0x10

#define TJS_MAX_NATIVE_CLASS 4
/*
	Number of "Native Class Instance" that can be used per a TJS Object is
	limited as the number above.
*/


class tTJSCustomObject : public tTJSDispatch
{
	typedef tTJSDispatch inherited;

	// tTJSSymbolData -----------------------------------------------------
public:
	struct tTJSSymbolData
	{
		tTJSVariantString *Name; // name
		tjs_uint32 Hash; // hash code of the name
		tjs_uint32 SymFlags; // management flags
		tjs_uint32 Flags;  // flags

		tTJSVariant_S Value; // the value
			/*
				TTJSVariant_S must work with construction that fills
					all member to zero.
			*/

		tTJSSymbolData * Next; // next chain

		void SelfClear(void)
		{
			memset(this, 0, sizeof(*this));
			SymFlags = TJS_SYMBOL_INIT;
		}

		void _SetName(const tjs_char * name)
		{
			if(Name) Name->Release(), Name = NULL;
			if(!name) TJS_eTJSError(TJSIDExpected);
			if(!name[0]) TJS_eTJSError(TJSIDExpected);
			Name = TJSAllocVariantString(name);
		}

		void SetName(const tjs_char * name, tjs_uint32 hash)
		{
			_SetName(name);
			Hash = hash;
		}

		void _SetName(tTJSVariantString *name)
		{
			if(name == Name) return;
			if(Name) Name->Release();
			Name = name;
			if(Name) Name->AddRef();
		}

		void SetName(tTJSVariantString *name, tjs_uint32 hash)
		{
			_SetName(name);
			Hash = hash;
		}

		const tjs_char * GetName() const
		{
			return (const tjs_char *)(*Name);
		}

		void PostClear()
		{
			if(Name) Name->Release(), Name = NULL;
			((tTJSVariant*)(&Value))->~tTJSVariant();
			memset(&Value, 0, sizeof(Value));
			SymFlags &= ~TJS_SYMBOL_USING;
		}

		void Destory()
		{
			if(Name) Name->Release();
			((tTJSVariant*)(&Value))->~tTJSVariant();
		}

		bool NameMatch(const tjs_char * name)
		{
			const tjs_char * this_name = GetName();
			if(this_name == name) return true;
			return !TJS_strcmp(name, this_name);
		}

		void ReShare();
	};



	//---------------------------------------------------------------------
	tjs_int Count;
	tjs_int HashMask;
	tjs_int HashSize;
	tTJSSymbolData * Symbols;
	tjs_uint RebuildHashMagic;
	bool IsInvalidated;
	bool IsInvalidating;
	iTJSNativeInstance* ClassInstances[TJS_MAX_NATIVE_CLASS];
	tjs_int32 ClassIDs[TJS_MAX_NATIVE_CLASS];


	void _Finalize(void);

	//---------------------------------------------------------------------
protected:
	bool GetValidity() const { return !IsInvalidated; }
	bool CallFinalize; // set false if this object does not need to call "finalize"
	ttstr finalize_name; // name of the 'finalize' method
	bool CallMissing; // set true if this object should call 'missing' method
	bool ProsessingMissing; // true if 'missing' method is being called
	ttstr missing_name; // name of the 'missing' method
	virtual void Finalize(void);
	std::vector<ttstr > ClassNames;

	//---------------------------------------------------------------------
public:

	tTJSCustomObject(tjs_int hashbits = TJS_NAMESPACE_DEFAULT_HASH_BITS);
	virtual ~tTJSCustomObject();

private:
	void BeforeDestruction(void);

	void CheckObjectClosureAdd(const tTJSVariant &val)
	{
		// adjust the reference counter when the object closure's "objthis" is
		// referring to the object itself.
		if(val.Type() == tvtObject)
		{
			iTJSDispatch2 *dsp = val.AsObjectClosureNoAddRef().ObjThis;
			if(dsp == (iTJSDispatch2*)this) this->Release();
		}
	}

	void CheckObjectClosureRemove(const tTJSVariant &val)
	{
		if(val.Type() == tvtObject)
		{
			iTJSDispatch2 *dsp = val.AsObjectClosureNoAddRef().ObjThis;
			if(dsp == (iTJSDispatch2*)this) this->AddRef();
		}
	}

	bool CallGetMissing(const tjs_char *name, tTJSVariant &result);
	bool CallSetMissing(const tjs_char *name, const tTJSVariant &value);

	tTJSSymbolData * Add(const tjs_char * name, tjs_uint32 *hint);
		// Adds the symbol, returns the newly created data;
		// if already exists, returns the data.

	tTJSSymbolData * Add(tTJSVariantString * name);
		// tTJSVariantString version of above.

	tTJSSymbolData * AddTo(tTJSVariantString *name,
		tTJSSymbolData *newdata, tjs_int newhashmask);
		// Adds member to the new hash space, used in RebuildHash

	void RebuildHash(); // rebuild hash table

	bool DeleteByName(const tjs_char * name, tjs_uint32 *hint);
		// Deletes Name

	void DeleteAllMembers(void);
		// Deletes all members
	void _DeleteAllMembers(void);
		// Deletes all members ( not to use std::vector )

	tTJSSymbolData * Find(const tjs_char * name, tjs_uint32 *hint) ;
		// Finds Name, returns its data; if not found, returns NULL

	static bool CallEnumCallbackForData(tjs_uint32 flags,
		tTJSVariant ** params,
		tTJSVariantClosure & callback, iTJSDispatch2 * objthis,
		const tTJSSymbolData * data);
	void InternalEnumMembers(tjs_uint32 flags, tTJSVariantClosure *callback,
		iTJSDispatch2 *objthis);
	//---------------------------------------------------------------------
public:
	void Clear() { DeleteAllMembers(); }
	/**
	 * rebuild hash table
	 */
	void RebuildHash( tjs_int requestcount );

	//---------------------------------------------------------------------
public:
	tjs_int GetValueInteger(const tjs_char * name, tjs_uint32 *hint);
		// service function for lexical analyzer
	//---------------------------------------------------------------------
public:

	tjs_error TJS_INTF_METHOD
	FuncCall(tjs_uint32 flag, const tjs_char * membername, tjs_uint32 *hint,
	tTJSVariant *result,
		tjs_int numparams, tTJSVariant **param, iTJSDispatch2 *objthis);

	tjs_error TJS_INTF_METHOD
	PropGet(tjs_uint32 flag, const tjs_char * membername, tjs_uint32 *hint,
	 tTJSVariant *result,
		iTJSDispatch2 *objthis);

	tjs_error TJS_INTF_METHOD
	PropSet(tjs_uint32 flag, const tjs_char *membername, tjs_uint32 *hint,
	 const tTJSVariant *param,
		iTJSDispatch2 *objthis);

	tjs_error TJS_INTF_METHOD
	GetCount(tjs_int *result, const tjs_char *membername, tjs_uint32 *hint,
	 iTJSDispatch2 *objthis);

	tjs_error TJS_INTF_METHOD
	PropSetByVS(tjs_uint32 flag, tTJSVariantString *membername,
		const tTJSVariant *param, iTJSDispatch2 *objthis);

	tjs_error TJS_INTF_METHOD
	EnumMembers(tjs_uint32 flag, tTJSVariantClosure *callback, iTJSDispatch2 *objthis);

	tjs_error TJS_INTF_METHOD
	DeleteMember(tjs_uint32 flag, const tjs_char *membername, tjs_uint32 *hint,
	 iTJSDispatch2 *objthis);

	tjs_error TJS_INTF_METHOD
	Invalidate(tjs_uint32 flag, const tjs_char *membername,  tjs_uint32 *hint,
	iTJSDispatch2 *objthis);

	tjs_error TJS_INTF_METHOD
	IsValid(tjs_uint32 flag, const tjs_char *membername, tjs_uint32 *hint,
	 iTJSDispatch2 *objthis);

	tjs_error TJS_INTF_METHOD
	CreateNew(tjs_uint32 flag, const tjs_char * membername, tjs_uint32 *hint,
	 iTJSDispatch2 **result,
		tjs_int numparams, tTJSVariant **param,	iTJSDispatch2 *objthis);

	tjs_error TJS_INTF_METHOD
	IsInstanceOf(tjs_uint32 flag, const tjs_char *membername, tjs_uint32 *hint,
	 const tjs_char *classname,
		iTJSDispatch2 *objthis);

	tjs_error TJS_INTF_METHOD
	Operation(tjs_uint32 flag, const tjs_char *membername, tjs_uint32 *hint,
	 tTJSVariant *result,
		const tTJSVariant *param,	iTJSDispatch2 *objthis);

	tjs_error TJS_INTF_METHOD
	NativeInstanceSupport(tjs_uint32 flag, tjs_int32 classid,
		iTJSNativeInstance **pointer);

	tjs_error TJS_INTF_METHOD
	ClassInstanceInfo(tjs_uint32 flag, tjs_uint num, tTJSVariant *value);
};
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
// TJSCreateCustomObject
//---------------------------------------------------------------------------
TJS_EXP_FUNC_DEF(iTJSDispatch2 *, TJSCreateCustomObject, ());
//---------------------------------------------------------------------------


} // namespace TJS


#endif
