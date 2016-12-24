//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Complex Rectangle Class
//---------------------------------------------------------------------------

#ifndef ComplexRectUnitH
#define ComplexRectUnitH

#include <stdlib.h>
#include "tjsTypes.h"

//---------------------------------------------------------------------------
// tTVPRect - intersection and union
//---------------------------------------------------------------------------
struct tTVPRect;
extern bool TVPIntersectRect(tTVPRect *dest, const tTVPRect &src1,
	const tTVPRect &src2);
extern bool TVPUnionRect(tTVPRect *dest, const tTVPRect &src1, const tTVPRect &src2);
//---------------------------------------------------------------------------


/*[*/

//---------------------------------------------------------------------------
// tTVPRect - simple rectangle structure
//---------------------------------------------------------------------------
#pragma pack(push, 4)
struct tTVPPoint
{
	tjs_int x;
	tjs_int y;
};
#pragma pack(pop)
//---------------------------------------------------------------------------
struct tTVPPointD
{
	double x;
	double y;
};
//---------------------------------------------------------------------------
struct tTVPRect
{
	tTVPRect(tjs_int l, tjs_int t, tjs_int r, tjs_int b)
		{ left = l, top = t, right = r, bottom =b; }

	tTVPRect() {};

	union
	{
		struct
		{
			tjs_int left;
			tjs_int top;
			tjs_int right;
			tjs_int bottom;
		};

		struct
		{
			// capital style
			tjs_int Left;
			tjs_int Top;
			tjs_int Right;
			tjs_int Bottom;
		};

		struct
		{
			tTVPPoint upper_left;
			tTVPPoint bottom_right;
		};

		tjs_int array[4];
	};

	tjs_int get_width() const { return right - left; }
	tjs_int get_height() const { return bottom - top; }

	void set_width(tjs_int w) { right = left + w; }
	void set_height(tjs_int h) { bottom = top + h; }

	void add_offsets(tjs_int x, tjs_int y)
	{
		left += x; right += x;
		top += y; bottom += y;
	}

	void set_offsets(tjs_int x, tjs_int y)
	{
		tjs_int w = get_width();
		tjs_int h = get_height();
		left = x;
		top = y;
		right = x + w;
		bottom = y + h;
	}

	void set_size(tjs_int w, tjs_int h)
	{
		right = left + w;
		bottom = top + h;
	}

	void clear()
	{
		left = top = right = bottom = 0;
	}

	bool is_empty() const
	{
		return left >= right || top >= bottom;
	}

	bool do_union(const tTVPRect & ref)
	{
		if(ref.is_empty()) return false;
		if(left > ref.left) left = ref.left;
		if(top > ref.top) top = ref.top;
		if(right < ref.right) right = ref.right;
		if(bottom < ref.bottom) bottom = ref.bottom;
		return true;
	}
#ifndef __TP_STUB_H__
	bool clip(const tTVPRect &ref)
	{
		// Clip (take the intersection of) the rectangle with rectangle. 
		// returns whether the rectangle remains.
		return TVPIntersectRect(this, *this, ref);
	}
#endif
	bool intersects_with_no_empty_check(const tTVPRect & ref) const
	{
		// returns wether this has intersection with "ref"
		return !(
			left >= ref.right ||
			top >= ref.bottom ||
			right <= ref.left ||
			bottom <= ref.top );
	}

	bool intersects_with(const tTVPRect & ref) const
	{
		// returns wether this has intersection with "ref"
		if(ref.is_empty() || is_empty()) return false;
		return intersects_with_no_empty_check(ref);
	}

	bool included_in_no_empty_check(const tTVPRect & ref) const
	{
		// returns wether this is included in "ref"
		return
			ref.left <= left &&
			ref.top <= top &&
			ref.right >= right &&
			ref.bottom >= bottom;
	}

	bool included_in(const tTVPRect & ref) const
	{
		// returns wether this is included in "ref"
		if(ref.is_empty() || is_empty()) return false;
		return included_in_no_empty_check(ref);
	}

public: // comparison operators for sorting
	bool operator < (const tTVPRect & rhs) const
		{ return top < rhs.top || (top == rhs.top && left < rhs.left); }
	bool operator > (const tTVPRect & rhs) const
		{ return top > rhs.top || (top == rhs.top && left > rhs.left); }

	// comparison methods
	bool operator == (const tTVPRect & rhs) const
		{ return top == rhs.top && left == rhs.left && right == rhs.right && bottom == rhs.bottom; }
	bool operator != (const tTVPRect & rhs) const { return !this->operator ==(rhs); }
};
//---------------------------------------------------------------------------

/*]*/


//---------------------------------------------------------------------------
// tTVPRegionRect : a class for a rectangle in region
//---------------------------------------------------------------------------
class tTVPRegionRect;
extern tTVPRegionRect * TVPAllocateRegionRect();
extern void TVPDeallocateRegionRect(tTVPRegionRect * rect);
//---------------------------------------------------------------------------
class tTVPRegionRect : public tTVPRect
{
public: // data members
	tTVPRegionRect * Prev; // previous link
	tTVPRegionRect * Next; // next link

public: // new and delete
	void * operator new (size_t size) { return TVPAllocateRegionRect(); }
	void operator delete (void * p) { TVPDeallocateRegionRect((tTVPRegionRect*)p); }

public: // constructors and destructor
	tTVPRegionRect() {;};
	tTVPRegionRect(const tTVPRect &r) : tTVPRect(r) {;};
	~tTVPRegionRect() {;};

public: // link operations
	void LinkAfter(tTVPRegionRect * r)
	{
		// Insert this after r.
		tTVPRegionRect * n = r->Next;
		r->Next = this;
		n->Prev = this;
		Prev = r;
		Next = n;
	}

	void LinkBefore(tTVPRegionRect * r)
	{
		// Insert this before r.
		tTVPRegionRect * p = r->Prev;
		r->Prev = this;
		p->Next = this;
		Prev = p;
		Next = r;
	}

	void Unlink()
	{
		// unchain from the link list
		tTVPRegionRect *prev = Prev;
		tTVPRegionRect *next = Next;
		prev->Next = next;
		next->Prev = prev;
	}

};
//---------------------------------------------------------------------------




//---------------------------------------------------------------------------
// tTVPComplexRect
//---------------------------------------------------------------------------
class tTVPComplexRect
{
public: // iterator
	class tIterator
	{
	private: // data members
		const tTVPRegionRect * Head;
		const tTVPRegionRect * Current;

	public: // constructor and destructor
		tIterator() : Head(NULL), Current(NULL) {;}
		tIterator(const tTVPRegionRect * head) : Head(head), Current(NULL) {;}

	public: // operator function (data access)
		const tTVPRect & operator * () const { return *Current; }
		const tTVPRect * operator -> () const { return Current; }

		const tTVPRegionRect & Get() const { return *Current; }

	public: // stepping forward; this object supports only forward step.
			// method step returns true if stepping successful,
			// otherwise returns false (when already at last of the list)
			// sample:
			//   tTVPComplexRect::tIterator it = rects.GetIterator();
			//   while(it.Step()) { .. do something with it .. }
		bool Step()
		{
			// Step forward
			if(!Head) return false;
			if(!Current) { Current = Head; return true; }
			if(Current->Next == Head) { return false; }
			Current = Current->Next;
			return true;
		}
	};

private: // data members
	tTVPRegionRect * Head; // head of the link list
	tTVPRegionRect * Current; // a rectangle which is touched last time
	tjs_int Count; // rectangle Count
	tTVPRect Bound; // bounding rectangle
	bool BoundValid; // whether the bounding rectangle is ready to use

public: // constructors and destructors
	tTVPComplexRect();
	tTVPComplexRect(const tTVPComplexRect & ref);
	~tTVPComplexRect();

public: // storage management
	void Clear();

private: // storage management
	void FreeAllRectangles(); // free all rectangles
	void Init(); // initialize internal states
	void SetCount(tjs_int count); // grow or shrink rectangle storage area
	bool Insert(const tTVPRect & rect); // insert inplace
	void Remove(tTVPRegionRect * rect); // remove a rectangle
	void Merge(const tTVPComplexRect & rects); // merge non-overlaped complex rectangle
public:
	tjs_int GetCount() const { return Count; }

public: // logical operations
	void Or(const tTVPRect &r);
	void Or(const tTVPComplexRect &ref);
	void Sub(const tTVPRect &r);
	void Sub(const tTVPComplexRect &ref);
	void And(const tTVPRect &r);

public: // operation utilities
	void CopyWithOffsets(const tTVPComplexRect &ref, const tTVPRect &clip,
		tjs_int ofsx, tjs_int ofsy); 

public: // bounding rectangle
	const tTVPRect & GetBound() const
		{ (const_cast<tTVPComplexRect *>(this))->EnsureBound(); return Bound; }

	void Unite()
	{
		// make union (bounding) one rectangle
		tTVPRect r(GetBound());
		Clear();
		Or(r);
	}

private: 
	void EnsureBound() { if(!BoundValid) CalcBound(); }
	void CalcBound();

private: // geometric rectangle operations
	tjs_int GetRectangleIntersectionCode(const tTVPRect &r, const tTVPRect &rr)
	{
		// Retrieve condition code which represents
		// how two rectangles have the intersection.
		tjs_int cond;
		if(rr.left   <= r.left   && rr.right   >= r.left   )
			cond =  8; else cond = 0;
		if(rr.left   <= r.right  && rr.right   >= r.right  )
			cond |= 4;
		if(rr.top    <= r.top    && rr.bottom  >= r.top    )
			cond |= 2;
		if(rr.top    <= r.bottom && rr.bottom  >= r.bottom )
			cond |= 1;
			/*
					   +8             +4

					+------+ +---+ +------+
					|rr    | |rr | |    rr|     +2
					|    +-----------+    |
					+----|-+ +---+ +-|----+
					+----|-+       +-|----+
					|rr  | |   r   | |  rr|
					+----|-+       +-|----+
					+----|-+ +---+ +-|----+
					|    +-----------+    |     +1
					|rr    | | rr| |    rr|
					+------+ +---+ +------+
			*/
		return cond;
	}

	void RectangleSub(tTVPRegionRect *r, const tTVPRect *rr);

public:
	void AddOffsets(tjs_int x, tjs_int y);

public: // iterator
	tIterator GetIterator() const
		{ if(Count) return tIterator(Head); else return tIterator(NULL); }

public: // debug
	void DumpChain();
};
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// tTVPComplexRectIterator : iterator for walking over rectangles
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------


#endif
