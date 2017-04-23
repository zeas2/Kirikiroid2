//---------------------------------------------------------------------------
/*
	TJS2 Script Engine
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// An Implementation of Hash Searching
//---------------------------------------------------------------------------
#ifndef HashSearchH
#define HashSearchH

#include "tjs.h"

#define TJS_HS_DEFAULT_HASH_SIZE 64
#define TJS_HS_HASH_USING	0x1
#define TJS_HS_HASH_LV1	0x2

// #define TJS_HS_DEBUG_CHAIN  // to debug chain algorithm

namespace TJS
{
//---------------------------------------------------------------------------
// tTJSHashFunc : Hash function object
//---------------------------------------------------------------------------
// the hash function used here is similar to one which used in perl 5.8,
// see also http://burtleburtle.net/bob/hash/doobs.html (One-at-a-Time Hash)
template <typename T>
class tTJSHashFunc
{
public:
	static tjs_uint32 Make(const T &val)
	{
		const char *p = (const char*)&val;
		const char *plim = (const char*)&val + sizeof(T);
		tjs_uint32 ret = 0;
		while(p<plim)
		{
			ret += *p;
			ret += (ret << 10);
			ret ^= (ret >> 6);
			p++;
		}
		ret += (ret << 3);
		ret ^= (ret >> 11);
		ret += (ret << 15);
		if(!ret) ret = (tjs_uint32)-1;
		return ret;
	}
};
//---------------------------------------------------------------------------
template <>
class tTJSHashFunc<ttstr> // a specialized template of tTJSHashFunc for ttstr
{
public:
	static tjs_uint32 Make(const ttstr &val)
	{
		if(val.IsEmpty()) return 0;
		const tjs_char *str = val.c_str();
		tjs_uint32 ret = 0;
		while(*str)
		{
			ret += *str;
			ret += (ret << 10);
			ret ^= (ret >> 6);
			str++;
		}
		ret += (ret << 3);
		ret ^= (ret >> 11);
		ret += (ret << 15);
		if(!ret) ret = (tjs_uint32)-1;
		return ret;
	}
};
//---------------------------------------------------------------------------
template <>
class tTJSHashFunc<tjs_char *> // a specialized template of tTJSHashFunc for tjs_char
{
public:
	static tjs_uint32 Make(const tjs_char *str)
	{
		if(!str) return 0;
		tjs_uint32 ret = 0;
		while(*str)
		{
			ret += *str;
			ret += (ret << 10);
			ret ^= (ret >> 6);
			str++;
		}
		ret += (ret << 3);
		ret ^= (ret >> 11);
		ret += (ret << 15);
		if(!ret) ret = (tjs_uint32)-1;
		return ret;
	}
};
//---------------------------------------------------------------------------
template <>
class tTJSHashFunc<tjs_nchar *> // a specialized template of tTJSHashFunc for tjs_nchar
{
public:
	static tjs_uint32 Make(const tjs_nchar *str)
	{
		if(!str) return 0;
		tjs_uint32 ret = 0;
		while(*str)
		{
			ret += *str;
			ret += (ret << 10);
			ret ^= (ret >> 6);
			str++;
		}
		ret += (ret << 3);
		ret ^= (ret >> 11);
		ret += (ret << 15);
		if(!ret) ret = (tjs_uint32)-1;
		return ret;
	}
};
//---------------------------------------------------------------------------
// tTJSHashTable : a simple implementation of Chain Hash algorithm
//---------------------------------------------------------------------------
/*
	Not that ValueT must implement both "operator =" and a copy constructor.
*/
template <typename KeyT, typename ValueT, typename HashFuncT = tTJSHashFunc<KeyT>,
	tjs_int HashSize = TJS_HS_DEFAULT_HASH_SIZE>
class tTJSHashTable
{
private:
	struct element
	{
		tjs_uint32 Hash;
		tjs_uint32 Flags; // management flag
		char Key[sizeof(KeyT)];
		char Value[sizeof(ValueT)];
		element *Prev; // previous chain item
		element *Next; // next chain item
		element *NPrev; // previous item in the additional order
		element *NNext; // next item in the additional order
	} Elms[HashSize];

	tjs_uint Count;

	element *NFirst; // first item in the additional order
	element *NLast;  // last item in the additional order

public:

	class tIterator // this differs a bit from STL's iterator
	{
		element * elm;
	public:
		tIterator() { elm = NULL; }

//		tIterator(const tIterator &ref)
//		{ elm = ref.elm; }

		tIterator(element *r_elm)
		{ elm = r_elm; }

		tIterator operator ++()
		{ elm = elm->NNext; return elm;}

		tIterator operator --()
		{ elm = elm->NPrev; return elm;}

		tIterator operator ++(int dummy)
		{ element *b_elm = elm; elm = elm->NNext; return b_elm; }

		tIterator operator --(int dummy)
		{ element *b_elm = elm; elm = elm->NPrev; return b_elm; }

		void operator +(tjs_int n)
		{ while(n--) elm = elm->NNext; }

		void operator -(tjs_int n)
		{ while(n--) elm = elm->NPrev; }

		bool operator ==(const tIterator & ref) const
		{ return elm == ref.elm; }

		bool operator !=(const tIterator & ref) const
		{ return elm != ref.elm; }

		KeyT & GetKey()
		{ return *(KeyT*)elm->Key; }

		ValueT & GetValue()
		{ return *(ValueT*)elm->Value; }

		bool IsNull() const { return elm == NULL; }
	};

	static tjs_uint32 MakeHash(const KeyT &key)
		{ return HashFuncT::Make(key); }

	tTJSHashTable()
	{
		InternalInit();
	}

	~tTJSHashTable()
	{
		InternalClear();
	}

	void Clear()
	{
		InternalClear();
	}

	tIterator GetFirst() const
	{
		return tIterator(NFirst);
	}

	tIterator GetLast() const
	{
		return tIterator(NLast);
	}


	void Add(const KeyT &key, const ValueT &value)
	{
		// add Key and Value
		AddWithHash(key, HashFuncT::Make(key), value);
	}

	void AddWithHash(const KeyT &key, tjs_uint32 hash, const ValueT &value)
	{
		// add Key ( hash ) and Value
#ifdef TJS_HS_DEBUG_CHAIN
		hash = 0;
#endif
		element *lv1 = Elms + hash % HashSize;
		element *elm = lv1->Next;
		while(elm)
		{
			if(hash == elm->Hash)
			{
				// same ?
				if(key == *(KeyT*)elm->Key)
				{
					// do copying instead of inserting if these are same
					CheckUpdateElementOrder(elm);
					*(ValueT*)elm->Value = value;
					return;
				}
			}
			elm = elm->Next;
		}

		// lv1 used ?
		if(!(lv1->Flags & TJS_HS_HASH_USING))
		{
			// lv1 is unused
			Construct(*lv1, key, value);
			lv1->Hash = hash;
			lv1->Prev = NULL;
			// not initialize lv1->Next here
			CheckAddingElementOrder(lv1);
			return;
		}

		// lv1 is used
		if(hash == lv1->Hash)
		{
			// same?
			if(key == *(KeyT*)lv1->Key)
			{
				// do copying instead of inserting if these are same
				CheckUpdateElementOrder(lv1);
				*(ValueT*)lv1->Value = value;
				return;
			}
		}

		// insert after lv1
		element *newelm = new element;
		newelm->Flags = 0;
		Construct(*newelm, key, value);
		newelm->Hash = hash;
		if(lv1->Next) lv1->Next->Prev = newelm;
		newelm->Next = lv1->Next;
		newelm->Prev = lv1;
		lv1->Next = newelm;
		CheckAddingElementOrder(newelm);
	}


	ValueT * Find(const KeyT &key) const
	{
		// find key
		// return   NULL  if not found
		const element * elm = InternalFindWithHash(key, HashFuncT::Make(key));
		if(!elm) return NULL;
		return (ValueT*)elm->Value;
	}

	bool Find(const KeyT &key, const KeyT *& keyout, ValueT *& value) const
	{
		// find key
		// return   false  if not found
		return FindWithHash(key, HashFuncT::Make(key), keyout, value);
	}

	ValueT * FindWithHash(const KeyT &key, tjs_uint32 hash) const
	{
		// find key ( hash )
		// return   NULL  if not found
#ifdef TJS_HS_DEBUG_CHAIN
		hash = 0;
#endif
		const element * elm = InternalFindWithHash(key, hash);
		if(!elm) return NULL;
		return (ValueT*)elm->Value;
	}

	bool FindWithHash(const KeyT &key, tjs_uint32 hash, const KeyT *& keyout, ValueT *& value) const
	{
		// find key
		// return   false  if not found
#ifdef TJS_HS_DEBUG_CHAIN
		hash = 0;
#endif
		const element * elm = InternalFindWithHash(key, hash);
		if(elm)
		{
			value = (ValueT*)elm->Value;
			keyout = (const KeyT*)elm->Key;
			return true;
		}
		return false;
	}

	ValueT * FindAndTouch(const KeyT &key)
	{
		// find key and move it first if found
		const element * elm = InternalFindWithHash(key, HashFuncT::Make(key));
		if(!elm) return NULL;
		CheckUpdateElementOrder((element *)elm);
		return (ValueT*)elm->Value;
	}

	bool FindAndTouch(const KeyT &key, const KeyT *& keyout, ValueT *& value)
	{
		// find key ( hash )
		// return   false  if not found
		return FindAndTouchWithHash(key, HashFuncT::Make(key), keyout, value);
	}


	ValueT * FindAndTouchWithHash(const KeyT &key, tjs_uint32 hash)
	{
		// find key ( hash ) and move it first if found
#ifdef TJS_HS_DEBUG_CHAIN
		hash = 0;
#endif
		const element * elm = InternalFindWithHash(key, hash);
		if(!elm) return NULL;
		CheckUpdateElementOrder((element *)elm); // force casting
		return (ValueT*)elm->Value;
	}

	bool FindAndTouchWithHash(const KeyT &key, tjs_uint32 hash, const KeyT *& keyout, ValueT *& value)
	{
		// find key
		// return   false  if not found
		const element * elm = InternalFindWithHash(key, hash);
		if(elm)
		{
			CheckUpdateElementOrder((element *)elm);
			value = (ValueT*)elm->Value;
			keyout = (const KeyT*)elm->Key;
			return true;
		}
		return false;
	}

	bool Delete(const KeyT &key)
	{
		// delete key and return true if successed
		return DeleteWithHash(key, HashFuncT::Make(key));
	}

	bool DeleteWithHash(const KeyT &key, tjs_uint32 hash)
	{
		// delete key ( hash ) and return true if succeeded
#ifdef TJS_HS_DEBUG_CHAIN
		hash = 0;
#endif
		element *lv1 = Elms + hash % HashSize;
		if(lv1->Flags & TJS_HS_HASH_USING && hash == lv1->Hash)
		{
			if(key == *(KeyT*)lv1->Key)
			{
				// delete lv1
				CheckDeletingElementOrder(lv1);
				Destruct(*lv1);
				return true;
			}
		}

		element *prev = lv1;
		element *elm = lv1->Next;
		while(elm)
		{
			if(hash == elm->Hash)
			{
				if(key == *(KeyT*)elm->Key)
				{
					CheckDeletingElementOrder(elm);
					Destruct(*elm);
					prev->Next = elm->Next; // sever from the chain
					if(elm->Next) elm->Next->Prev = prev;
					delete elm;
					return true;
				}
			}
			prev = elm;
			elm = elm->Next;
		}
		return false; // not found
	}

	tjs_int ChopLast(tjs_int count)
	{
		// chop items from last of additional order
		tjs_int ret = 0;
		while(count--)
		{
			if(!NLast) break;
			DeleteByElement(NLast);
			ret++;
		}
		return ret;
	}

	tjs_uint GetCount() { return Count; }

private:
	void InternalClear()
	{
		for(tjs_int i = 0; i < HashSize; i++)
		{
			// delete all items
			element *elm = Elms[i].Next;
			while(elm)
			{
				Destruct(*elm);
				element *elmnext = elm->Next;
				delete elm;
				elm = elmnext;
			}
			if(Elms[i].Flags & TJS_HS_HASH_USING)
			{
				Destruct(Elms[i]);
			}
		}
		InternalInit();
	}

	void InternalInit()
	{
		Count = 0;
		NFirst = NULL;
		NLast = NULL;
		for(tjs_int i = 0; i < HashSize; i++)
		{
			Elms[i].Flags = TJS_HS_HASH_LV1;
			Elms[i].Prev = NULL;
			Elms[i].Next = NULL;
		}
	}


	void DeleteByElement(element *elm)
	{
		CheckDeletingElementOrder(elm);
		Destruct(*elm);
		if(elm->Flags & TJS_HS_HASH_LV1)
		{
			// lv1 element
			// nothing to do
		}
		else
		{
			// other elements
			if(elm->Prev) elm->Prev->Next = elm->Next;
			if(elm->Next) elm->Next->Prev = elm->Prev;
		}
		if(!(elm->Flags & TJS_HS_HASH_LV1)) delete elm;
	}

	const element * InternalFindWithHash(const KeyT &key, tjs_uint32 hash) const
	{
		// find key ( hash )
#ifdef TJS_HS_DEBUG_CHAIN
		hash = 0;
#endif
		const element *lv1 = Elms + hash % HashSize;
		if(hash == lv1->Hash && lv1->Flags & TJS_HS_HASH_USING)
		{
			if(key == *(KeyT*)lv1->Key) return lv1;
		}

		element *elm = lv1->Next;
		while(elm)
		{
			if(hash == elm->Hash)
			{
				if(key == *(KeyT*)elm->Key) return elm;
			}
			elm = elm->Next;
		}
		return NULL; // not found
	}


	void CheckAddingElementOrder(element *elm)
	{
		if(Count==0)
		{
			NLast = elm; // first addition
			elm->NNext = NULL;
		}
		else
		{
			NFirst->NPrev = elm;
			elm->NNext = NFirst;
		}
		NFirst = elm;
		elm->NPrev = NULL;
		Count ++;
	}

	void CheckDeletingElementOrder(element *elm)
	{
		Count --;
		if(Count > 0)
		{
			if(elm == NFirst)
			{
				// deletion of first item
				NFirst = elm->NNext;
				NFirst->NPrev = NULL;
			}
			else if(elm == NLast)
			{
				// deletion of last item
				NLast = elm->NPrev;
				NLast->NNext = NULL;
			}
			else
			{
				// deletion of intermediate item
				elm->NPrev->NNext = elm->NNext;
				elm->NNext->NPrev = elm->NPrev;
			}
		}
		else
		{
			// when the count becomes zero...
			NFirst = NLast = NULL;
		}
	}

	void CheckUpdateElementOrder(element *elm)
	{
		// move elm to the front of addtional order
		if(elm != NFirst)
		{
			if(NLast == elm) NLast = elm->NPrev;
			elm->NPrev->NNext = elm->NNext;
			if(elm->NNext) elm->NNext->NPrev = elm->NPrev;
			elm->NNext = NFirst;
			elm->NPrev = NULL;
			NFirst->NPrev = elm;
			NFirst = elm;
		}
	}

	static void Construct(element &elm, const KeyT &key, const ValueT &value)
	{
		::new (&elm.Key) KeyT(key);
		::new (&elm.Value) ValueT(value);
		elm.Flags |= TJS_HS_HASH_USING;
	}

	static void Destruct(element &elm)
	{
		((KeyT*)(&elm.Key)) -> ~KeyT();
		((ValueT*)(&elm.Value)) -> ~ValueT();
		elm.Flags &= ~TJS_HS_HASH_USING;
	}
};
//---------------------------------------------------------------------------
// tTJSHashCache : cache algorithm via tTJSHashTable
//---------------------------------------------------------------------------
template <typename KeyT, typename ValueT, typename HashFuncT = tTJSHashFunc<KeyT>,
	tjs_int HashSize = TJS_HS_DEFAULT_HASH_SIZE >
class tTJSHashCache : public tTJSHashTable<KeyT, ValueT, HashFuncT, HashSize>
{
	typedef tTJSHashTable<KeyT, ValueT, HashFuncT, HashSize> inherited;

	tjs_uint MaxCount;

public:
	tTJSHashCache(tjs_uint maxcount) { MaxCount = maxcount; }

	void Add(const KeyT &key, const ValueT &value)
	{
		inherited::Add(key, value);
		if (this->GetCount() > MaxCount)
		{
			this->ChopLast(this->GetCount() - MaxCount);
		}
	}

	void AddWithHash(const KeyT &key, tjs_uint32 hash, const ValueT &value)
	{
		inherited::AddWithHash(key, hash, value);
		if (this->GetCount() > MaxCount)
		{
			this->ChopLast(this->GetCount() - MaxCount);
		}
	}

	void SetMaxCount(tjs_uint maxcount)
	{
		MaxCount = maxcount;
		if (this->GetCount() > MaxCount)
		{
			this->ChopLast(this->GetCount() - MaxCount);
		}
	}

	tjs_uint GetMaxCount() { return MaxCount; }
};

// for std::hash
struct ttstr_hasher {
	tjs_uint32 operator ()(const ttstr &str) const {
		tjs_uint32 *hint = const_cast<ttstr*>(&str)->GetHint();
		if (!hint) return 0;
		if (*hint) return *hint;
		return (*hint = tTJSHashFunc<ttstr>::Make(str));
	}
};

//---------------------------------------------------------------------------
} // namespace TJS
//---------------------------------------------------------------------------

#endif

