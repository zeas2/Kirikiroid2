//---------------------------------------------------------------------------
/*
	TJS2 Script Engine
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// utility functions
//---------------------------------------------------------------------------
#ifndef tjsUtilsH
#define tjsUtilsH

#include "tjsVariant.h"
#include "tjsString.h"

namespace TJS
{
//---------------------------------------------------------------------------
// tTJSCriticalSection ( implement on each platform for multi-threading support )
//---------------------------------------------------------------------------
struct tTJSCriticalSectionImpl;
struct tTJSCriticalSection {
	tTJSCriticalSectionImpl *_impl;
	tTJSCriticalSection();
	~tTJSCriticalSection();
	bool lock();
	void unlock();
};
//---------------------------------------------------------------------------
// interlocked operation ( implement on each platform for multi-threading support )
//---------------------------------------------------------------------------
/*
#ifdef __WIN32__
#include <Windows.h>

// we assume that sizeof(tjs_uint) is 4 on TJS2/win32.

inline tjs_uint TJSInterlockedIncrement(tjs_uint & value)
{
	return InterlockedIncrement((long*)&value);
}

#else

inline

#endif
*/
//---------------------------------------------------------------------------
// tTJSCriticalSectionHolder
//---------------------------------------------------------------------------
class tTJSCriticalSectionHolder {
	tTJSCriticalSection *_cs;

public:
	tTJSCriticalSectionHolder(tTJSCriticalSection& cs) {
		if (cs.lock()) {
			_cs = &cs;
		} else {
			_cs = nullptr;
	}
	}
	~tTJSCriticalSectionHolder() {
		if (_cs) _cs->unlock();
	}
};
typedef tTJSCriticalSectionHolder tTJSCSH;
//---------------------------------------------------------------------------
typedef tTJSCriticalSection tTJSStaticCriticalSection;

struct tTJSSpinLock {
	std::atomic_flag atom_lock;
	tTJSSpinLock();
	void lock(); // will stuck if locked in same thread!
	void unlock();
};

class tTJSSpinLockHolder {
	tTJSSpinLock* Lock;
public:
	tTJSSpinLockHolder(tTJSSpinLock &lock);

	~tTJSSpinLockHolder();
};

//---------------------------------------------------------------------------
// tTJSAtExit / tTJSAtStart
//---------------------------------------------------------------------------
class tTJSAtExit
{
	void (*Function)();
public:
	tTJSAtExit(void (*func)()) { Function = func; };
	~tTJSAtExit() { Function(); }
};
//---------------------------------------------------------------------------
class tTJSAtStart
{
public:
	tTJSAtStart(void (*func)()) { func(); };
};
//---------------------------------------------------------------------------
class iTJSDispatch2;
extern iTJSDispatch2 * TJSObjectTraceTarget;

#define TJS_DEBUG_REFERENCE_BREAK \
	if(TJSObjectTraceTarget == (iTJSDispatch2*)this) TJSNativeDebuggerBreak()
#define TJS_SET_REFERENCE_BREAK(x) TJSObjectTraceTarget=(x)
//---------------------------------------------------------------------------
TJS_EXP_FUNC_DEF(const tjs_char *, TJSVariantTypeToTypeString, (tTJSVariantType type));
	// convert given variant type to type string ( "void", "int", "object" etc.)

TJS_EXP_FUNC_DEF(tTJSString, TJSVariantToReadableString, (const tTJSVariant &val, tjs_int maxlen = 512));
	// convert given variant to human-readable string
	// ( eg. "(string)\"this is a\\nstring\"" )
TJS_EXP_FUNC_DEF(tTJSString, TJSVariantToExpressionString, (const tTJSVariant &val));
	// convert given variant to string which can be interpret as an expression.
	// this function does not convert objects ( returns empty string )

//---------------------------------------------------------------------------



/*[*/
//---------------------------------------------------------------------------
// tTJSRefHolder : a object holder for classes that has AddRef and Release methods
//---------------------------------------------------------------------------
template <typename T>
class tTJSRefHolder
{
private:
	T *Object;
public:
	tTJSRefHolder(T * ref) { Object = ref; Object->AddRef(); }
	tTJSRefHolder(const tTJSRefHolder<T> &ref)
	{
		Object = ref.Object;
		Object->AddRef();
	}
	~tTJSRefHolder() { Object->Release(); }

	T* GetObject() { Object->AddRef(); return Object; }
	T* GetObjectNoAddRef() const { return Object; }

	const tTJSRefHolder & operator = (const tTJSRefHolder & rhs)
	{
		if(rhs.Object != Object)
		{
			Object->Release();
			Object = rhs.Object;
			Object->AddRef();
		}
		return *this;
	}
};

template <typename T>
class tRefHolder
{
	struct _RefCountWrapper
	{
		T* _ptr = nullptr;
		int RefCount = 1;

		_RefCountWrapper() = default;
		_RefCountWrapper(T* p) : _ptr(p) {}
		~_RefCountWrapper() { if (_ptr) delete _ptr; }
		void AddRef() { RefCount++; }
		void Release()
		{
			if (RefCount == 1) {
				delete this;
			} else {
				RefCount--;
			}
		}
	};
	_RefCountWrapper *_ptr;

public:
	tRefHolder() : _ptr(nullptr) {}
	tRefHolder(T* p) : _ptr(nullptr) { if (p) _ptr = new _RefCountWrapper(p); }
	tRefHolder(const tRefHolder<T> &ref) { _ptr = ref._ptr; if (_ptr) _ptr->AddRef(); }
	~tRefHolder() { if (_ptr) _ptr->Release(); }
	const tRefHolder & operator = (const tRefHolder & rhs) {
		if (rhs._ptr != _ptr) {
			if (_ptr)_ptr->Release();
			_ptr = rhs._ptr;
			if (_ptr) _ptr->AddRef();
		}
		return *this;
	}
	T* get() const { if (_ptr) return _ptr->_ptr; return nullptr; }
	operator T*() const { return get(); }
	T* operator->() const { return get(); }
};


/*]*/

//---------------------------------------------------------------------------

#define VECLIST_MIN_CAPACITY 32
#define VECLIST_MAX_CAPACITY_INCREASE 2048

template<typename T>
class tVectorList {
protected:
	struct _tVectorList_Node {
		T Data;
		int prevIndex;
		int nextIndex;
	} *PointerBuffPtr;
	std::vector<int> UnusedIndexStack;
	unsigned int nCpacity;
	unsigned int nQuantity;
	int lastIndex;
	int firstIndex;

	tVectorList(const tVectorList&);//不可被复制
	tVectorList& operator=(const tVectorList&);//不可被赋值

	void _erase(unsigned int index) {
		_tVectorList_Node &Node = PointerBuffPtr[index];
		if (Node.prevIndex != -1) {
			PointerBuffPtr[Node.prevIndex].nextIndex = Node.nextIndex;
		} else {
			firstIndex = Node.nextIndex;
		}
		if (Node.nextIndex != -1) {
			PointerBuffPtr[Node.nextIndex].prevIndex = Node.prevIndex;
		} else {
			lastIndex = Node.prevIndex;
		}
		UnusedIndexStack.push_back(index);
		--nQuantity;
	}

	int _find(T &_Val) const {
		int i = firstIndex;
		for (; i != -1; i = PointerBuffPtr[i].nextIndex) {
			if (PointerBuffPtr[i].Data == _Val)
				break;
		}
		return i;
	}

	inline void _ensureCapacity() {
		if (UnusedIndexStack.empty())
		{
			int nNewCapacity;
			if (!PointerBuffPtr) {
				nNewCapacity = VECLIST_MIN_CAPACITY;
				PointerBuffPtr = (_tVectorList_Node*)new char[VECLIST_MIN_CAPACITY * sizeof(_tVectorList_Node)];
			} else {
				nNewCapacity = (nCpacity < VECLIST_MAX_CAPACITY_INCREASE) ?
					nCpacity * 2 : nCpacity + VECLIST_MAX_CAPACITY_INCREASE;
				_tVectorList_Node* newPtr = (_tVectorList_Node*)new char[nNewCapacity * sizeof(_tVectorList_Node)];
				memcpy(newPtr, PointerBuffPtr, sizeof(_tVectorList_Node)* nCpacity);
				delete[](char*)PointerBuffPtr;
				PointerBuffPtr = newPtr;
			}
			for (int i = nNewCapacity - 1; i >= (int)nCpacity; --i) {
				UnusedIndexStack.push_back(i);
			}
			nCpacity = nNewCapacity;
		}
	}

public:
	tVectorList()
		: nCpacity(0)
		, nQuantity(0)
		, PointerBuffPtr(0)
		, lastIndex(-1)
		, firstIndex(-1)
	{
	}

	~tVectorList() {
		clear();
	}

	unsigned int capacity() const { return nCpacity; }
	unsigned int size() const { return nQuantity; }
	//     void resize(unsigned int newsize) {
	//         if(newsize < nQuantity) nQuantity = newsize;
	//     }
	bool empty() const { return nQuantity == 0; }
	//T* data() { return PointerBuffPtr; }
	void push_front(T &_Val) {
		_ensureCapacity();
		int currentIndex = UnusedIndexStack.back();
		UnusedIndexStack.pop_back();
		_tVectorList_Node &Node = PointerBuffPtr[currentIndex];
		Node.Data = _Val;
		Node.prevIndex = -1;
		Node.nextIndex = firstIndex;
		if (firstIndex != -1) {
			_tVectorList_Node &nextNode = PointerBuffPtr[firstIndex];
			nextNode.prevIndex = currentIndex;
		} else {
			lastIndex = currentIndex;
		}
		firstIndex = currentIndex;
		++nQuantity;
	}
	void push_back(const T &_Val) {
		_ensureCapacity();
		int currentIndex = UnusedIndexStack.back();
		UnusedIndexStack.pop_back();
		_tVectorList_Node &Node = PointerBuffPtr[currentIndex];
		Node.Data = _Val;
		Node.prevIndex = lastIndex;
		Node.nextIndex = -1;
		if (lastIndex != -1) {
			_tVectorList_Node &prevNode = PointerBuffPtr[lastIndex];
			prevNode.nextIndex = currentIndex;
		} else {
			firstIndex = currentIndex;
		}
		lastIndex = currentIndex;
		++nQuantity;
	}
	T& pop_back() {
		_tVectorList_Node &Node = PointerBuffPtr[lastIndex];
		_erase(lastIndex);
		return Node.Data;
	}
	T& pop_front() {
		_tVectorList_Node &Node = PointerBuffPtr[firstIndex];
		_erase(firstIndex);
		return Node.Data;
	}
	T& back() const { return PointerBuffPtr[lastIndex].Data; }
	T& front() const { return PointerBuffPtr[firstIndex].Data; }
	bool remove(T &arg) {
		int i = _find(arg);
		if (i != -1) {
			_erase(i);
			return true;
		}
		return false;
	}
	bool contains(T &arg) const { return _find(arg) != -1; }
	void clean() {
		if (PointerBuffPtr) delete[](char*)PointerBuffPtr;
		PointerBuffPtr = 0;
		nCpacity = 0;
		nQuantity = 0;
		lastIndex = firstIndex = -1;
		UnusedIndexStack.clear();
	}

	void clear() {
		if (!nCpacity) return;
		if (nCpacity <= VECLIST_MIN_CAPACITY * 2) {
			nQuantity = 0;
			lastIndex = firstIndex = -1;
			UnusedIndexStack.resize(nCpacity);
			int *p = &UnusedIndexStack.front();
			for (int i = nCpacity - 1; i >= 0; --i) {
				*p++ = i;
			}
		} else {
			clean();
		}
	}

	class iterator {
		friend class tVectorList<T>;
	private:
		_tVectorList_Node** m_pNodeBuffer;
		int index;

	public:
		iterator() :m_pNodeBuffer(0), index(-1){}
		iterator(_tVectorList_Node** _p, int _index) :m_pNodeBuffer(_p), index(_index){}

		bool operator== (iterator rhs){ return m_pNodeBuffer == rhs.m_pNodeBuffer && index == rhs.index; }
		bool operator!= (iterator rhs){ return m_pNodeBuffer != rhs.m_pNodeBuffer || index != rhs.index; }
		iterator&  operator++(){ // ++ prefix
			if (!end()) {
				index = (*m_pNodeBuffer)[index].nextIndex;
			}
			return *this;
		}
		const iterator operator++(int _zero) { // ++ suffix
			iterator ret = *this;
			if (!end()) {
				index = (*m_pNodeBuffer)[index].nextIndex;
			}
			return ret;
		}
		iterator&  operator--(){ // -- prefix
			if (!end())
				index = (*m_pNodeBuffer)[index].prevIndex;
			return *this;
		}
		const iterator operator--(int _zero){ // -- suffix
			iterator ret = *this;
			if (!end())
				index = (*m_pNodeBuffer)[index].prevIndex;
			return ret;
		}
		T& operator*() {
#if (defined(WIN32) || defined(_WIN32)) && defined(_DEBUG)
			if (end())
				throw;
#endif
			return (*m_pNodeBuffer)[index].Data;
		}
		T* operator->() {
#if (defined(WIN32) || defined(_WIN32)) && defined(_DEBUG)
			if (end())
				throw;
#endif
			return &(*m_pNodeBuffer)[index].Data;
		}
		operator bool() { return !end(); }
		bool end() { return index < 0; }
	};
	iterator begin() { return iterator(&PointerBuffPtr, firstIndex); }
	iterator rbegin() { return iterator(&PointerBuffPtr, lastIndex); }
	iterator end() { return iterator(&PointerBuffPtr, -1); }
	iterator rend() { return iterator(&PointerBuffPtr, -1); }
	iterator find(T &_Val) { return iterator(&PointerBuffPtr, _find(_Val)); }
	typedef iterator const_iterator;

	const_iterator begin() const { return const_iterator(&PointerBuffPtr, firstIndex); }
	const_iterator rbegin() const { return const_iterator(&PointerBuffPtr, lastIndex); }
	const_iterator end() const { return const_iterator(&PointerBuffPtr, -1); }
	const_iterator rend() const { return const_iterator(&PointerBuffPtr, -1); }
	const_iterator find(T &_Val) const { return const_iterator(&PointerBuffPtr, _find(_Val)); }
	void erase(const iterator &it) { _erase(it.index); }
	void insert_before(iterator &it, T &_Val) {
		if (it.index == -1) {
			push_back(_Val);
			return;
		}
		_tVectorList_Node &IteratorNode = PointerBuffPtr[it.index];
		if (IteratorNode.prevIndex == -1) {
			push_front(_Val);
			return;
		}
		/*   __________    ______    __________
		*  | Iterator |  | Node |  | PrevNode |
		*  |----------|  |------|  |----------|
		*  |   prev  --->| prev -->|   prev   |
		*  |   next   |<-- next |<---- next   |
		*  |__________|  |______|  |__________|
		*/

		_ensureCapacity();
		int currentIndex = UnusedIndexStack.pop_back();
		_tVectorList_Node &Node = PointerBuffPtr[currentIndex];
		_tVectorList_Node &PrevNode = PointerBuffPtr[IteratorNode.prevIndex];
		Node.Data = _Val;

		Node.prevIndex = IteratorNode.prevIndex;//PrevNode
		Node.nextIndex = it.index;
		PrevNode.nextIndex = currentIndex;
		IteratorNode.prevIndex = currentIndex;
		++nQuantity;
	}
	void insert_after(iterator &it, T &_Val) {
		if (it.index == -1) {
			push_front(_Val);
			return;
		}
		_tVectorList_Node &IteratorNode = PointerBuffPtr[it.index];
		if (IteratorNode.nextIndex == -1) {
			push_back(_Val);
			return;
		}
		/*   __________    ______    __________
		*  | Iterator |  | Node |  | NextNode |
		*  |----------|  |------|  |----------|
		*  |   prev   |<-- prev |<---- prev   |
		*  |   next  --->| next -->|   next   |
		*  |__________|  |______|  |__________|
		*/

		_ensureCapacity();
		int currentIndex = UnusedIndexStack.pop_back();
		_tVectorList_Node &Node = PointerBuffPtr[currentIndex];
		_tVectorList_Node &NextNode = PointerBuffPtr[IteratorNode.nextIndex];
		Node.Data = _Val;

		Node.prevIndex = it.index;
		Node.nextIndex = IteratorNode.nextIndex;//NextNode
		NextNode.prevIndex = currentIndex;
		IteratorNode.nextIndex = currentIndex;
		++nQuantity;
	}
};


//---------------------------------------------------------------------------
// TJSAlignedAlloc : aligned memory allocater
//---------------------------------------------------------------------------
TJS_EXP_FUNC_DEF(void *, TJSAlignedAlloc, (tjs_uint bytes, tjs_uint align_bits));
TJS_EXP_FUNC_DEF(void, TJSAlignedDealloc, (void *ptr));
//---------------------------------------------------------------------------

/*[*/
//---------------------------------------------------------------------------
// floating-point class checker
//---------------------------------------------------------------------------
// constants used in TJSGetFPClass
#define TJS_FC_CLASS_MASK 7
#define TJS_FC_SIGN_MASK 8

#define TJS_FC_CLASS_NORMAL 0
#define TJS_FC_CLASS_NAN 1
#define TJS_FC_CLASS_INF 2

#define TJS_FC_IS_NORMAL(x)  (((x)&TJS_FC_CLASS_MASK) == TJS_FC_CLASS_NORMAL)
#define TJS_FC_IS_NAN(x)  (((x)&TJS_FC_CLASS_MASK) == TJS_FC_CLASS_NAN)
#define TJS_FC_IS_INF(x)  (((x)&TJS_FC_CLASS_MASK) == TJS_FC_CLASS_INF)

#define TJS_FC_IS_NEGATIVE(x) (0!=((x) & TJS_FC_SIGN_MASK))
#define TJS_FC_IS_POSITIVE(x) (!TJS_FC_IS_NEGATIVE(x))


/*]*/
TJS_EXP_FUNC_DEF(tjs_uint32, TJSGetFPClass, (tjs_real r));
//---------------------------------------------------------------------------
}

#endif




