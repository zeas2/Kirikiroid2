//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Complex Rectangle Class
//---------------------------------------------------------------------------
#define NOMINMAX
#include "tjsCommHead.h"

#include "ComplexRect.h"
#include <vector>
#include <algorithm>

// Some algorithms and ideas are based on implementation of Mozilla,
// gfx/src/nsRegion.cpp.



//---------------------------------------------------------------------------
// TVPIntersectRect
//---------------------------------------------------------------------------
bool TVPIntersectRect(tTVPRect *dest, const tTVPRect &src1,
	const tTVPRect &src2)
{
	tjs_int left =		std::max(src1.left,		src2.left);
	tjs_int top =		std::max(src1.top,		src2.top);
	tjs_int right =		std::min(src1.right,	src2.right);
	tjs_int bottom =	std::min(src1.bottom,	src2.bottom);

	if(right > left && bottom > top)
	{
		if(dest)
		{
			dest->left =	left;
			dest->top =		top;
			dest->right =	right;
			dest->bottom =	bottom;
		}
		return true;
	}
	return false;
}
//---------------------------------------------------------------------------




//---------------------------------------------------------------------------
// TVPUnionRect
//---------------------------------------------------------------------------
bool TVPUnionRect(tTVPRect *dest, const tTVPRect &src1, const tTVPRect &src2)
{
	tjs_int left =		std::min(src1.left,		src2.left);
	tjs_int top =		std::min(src1.top,		src2.top);
	tjs_int right =		std::max(src1.right,	src2.right);
	tjs_int bottom =	std::max(src1.bottom,	src2.bottom);

	if(right > left && bottom > top)
	{

		if(dest)
		{
			dest->left =	left;
			dest->top =		top;
			dest->right =	right;
			dest->bottom =	bottom;
		}
		return true;
	}

	return false;
}
//---------------------------------------------------------------------------







//---------------------------------------------------------------------------
// tTVPRegionRect allocation routines
//---------------------------------------------------------------------------
// variables
static tjs_uint TVPRegionFreeMaxCount = 0;
	// maximum count of free rectangles in TVPRegionFreeRects
static std::vector<tTVPRegionRect *>  * TVPRegionFreeRects = NULL;
#define TVP_REGIONRECT_ALLOC_UNITS   256
	// Number of rectangles which to be allocated in a call of
	// TVPPrepareRegionRectangle()
//---------------------------------------------------------------------------
class tTVPRegionRectInitialHolder
{
	// Initial rectangle holder to prevent unnecessary freeing of the vector.
	// This holds a rectangle from program start to end.
	tTVPRegionRect * InitialRect;
public:
	tTVPRegionRectInitialHolder() { InitialRect = NULL; }
	void Hold() { if(!InitialRect) { InitialRect = (tTVPRegionRect *)-1;
		InitialRect = TVPAllocateRegionRect(); } }
	~tTVPRegionRectInitialHolder() { if(InitialRect) TVPDeallocateRegionRect(InitialRect); }
};
static tTVPRegionRectInitialHolder TVPRegionRectInitialHolder;
//---------------------------------------------------------------------------
static void TVPPrepareRegionRectangle()
{
	// Prepare rectangles and insert them into free rectangle list
	for(tjs_int i = 0; i < TVP_REGIONRECT_ALLOC_UNITS; i++)
	{
		TVPRegionFreeRects->push_back(
			(tTVPRegionRect*)(new char[sizeof (tTVPRegionRect)]));
		TVPRegionFreeMaxCount++;
	}
}
//---------------------------------------------------------------------------
tTVPRegionRect * TVPAllocateRegionRect()
{
	// Allocate a region rectangle. Note that this function does not clear
	// nor initialize the rectangle object.

	// Create a vector of free rectangle list if it does not exists
	if(!TVPRegionFreeRects)
		TVPRegionFreeRects = new std::vector<tTVPRegionRect *>();

	// Prepare free rectangles if the free list is empty
	if(TVPRegionFreeRects->size() == 0)
		TVPPrepareRegionRectangle();

	// Take a free rectangle from last of the free rectangle list
	tTVPRegionRect * r = TVPRegionFreeRects->back();
	TVPRegionFreeRects->pop_back();

	// Hold initial rectangle
	TVPRegionRectInitialHolder.Hold();

	// Return the rectangle
	return r;
}
//---------------------------------------------------------------------------
void TVPDeallocateRegionRect(tTVPRegionRect * rect)
{
	// Deallocate a region rectangle allocated in TVPAllocateRegionRect().

	// Append to TVPRegionFreeRects
//	if(!TVPRegionFreeRects) TVPThrowInternalError();  //----------------- geee
	TVPRegionFreeRects->push_back(rect);

	// Full-Free check
	if(TVPRegionFreeRects->size() == TVPRegionFreeMaxCount)
	{
		// Free all rectangles
		for(std::vector<tTVPRegionRect *>::iterator i = TVPRegionFreeRects->begin();
			i != TVPRegionFreeRects->end(); i++)
			delete [] (char *)(*i);

		// Free the vector
		delete TVPRegionFreeRects, TVPRegionFreeRects = NULL;
		TVPRegionFreeMaxCount = 0;
	}
}
//---------------------------------------------------------------------------





//---------------------------------------------------------------------------
// tTVPComplexRect
//---------------------------------------------------------------------------
tTVPComplexRect::tTVPComplexRect()
{
	// normal constructor
	Init();
}
//---------------------------------------------------------------------------
tTVPComplexRect::tTVPComplexRect(const tTVPComplexRect & ref)
: Bound(ref.Bound)
{
	// copy constructor
	Init();

	// allocate storage
	SetCount(ref.Count);

	// copy rectangles
	if(Count)
	{
		tTVPRegionRect * this_cur = Head;
		tTVPRegionRect * ref_cur = ref.Head;
		do
		{
			*(tTVPRect*)this_cur = *(tTVPRect*)ref_cur; // copy as tTVPRect
			this_cur = this_cur->Next;
			ref_cur = ref_cur->Next;
		} while(ref_cur != ref.Head);
	}
}
//---------------------------------------------------------------------------
tTVPComplexRect::~tTVPComplexRect()
{
	// destructor
	FreeAllRectangles();
}
//---------------------------------------------------------------------------
void tTVPComplexRect::Clear()
{
	FreeAllRectangles();
	Init();
}
//---------------------------------------------------------------------------
void tTVPComplexRect::FreeAllRectangles()
{
	// free all rectangles
	if(Count)
	{
		tTVPRegionRect *cur = Head;
		do
		{
			tTVPRegionRect *n  = cur->Next;
			delete cur;
			cur = n;
		} while(cur != Head);
	}
}
//---------------------------------------------------------------------------
void tTVPComplexRect::Init()
{
	// initialize internal states
	Head = NULL;
	Current = NULL;
	Count = 0;
	Bound.left = Bound.top = Bound.right = Bound.bottom = 0;
	BoundValid = false;
}
//---------------------------------------------------------------------------
void tTVPComplexRect::SetCount(tjs_int count)
{
	// grow or shrink rectangle storage area
	if(count > Count)
	{
		// grow
		tjs_int add_count = count - Count;
		if(!Count)
		{
			// no existent rectangle
			Head = new tTVPRegionRect();
			Head->Prev = Head;
			Head->Next = Head;
			add_count --;
		}

		tTVPRegionRect * cur = Head->Prev;
		Current = cur;
		while(add_count--)
		{
			tTVPRegionRect * newrect = new tTVPRegionRect();
			newrect->LinkBefore(Head);
		}
		Count = count;
	}
	else if(count < Count)
	{
		// shrink
		if(Count)
		{
			tjs_int remove_count = Count - count;
			tTVPRegionRect * cur = Head->Prev;
			while(remove_count--)
			{
				tTVPRegionRect * prev = cur->Prev;
				delete cur;
				cur = prev;
			}
			Count = count;
			if(Count)
				cur->Next = Head, Current = Head, Head->Prev = cur;
			else
				Head = NULL, Current = NULL;
		}
	}
}
//---------------------------------------------------------------------------
bool tTVPComplexRect::Insert(const tTVPRect & rect)
{
	// Insert a region rectangle inplace.
	// Note that this function does not update the bounding rectangle.
	// This does empty check.

	if(rect.is_empty()) return false;

	if(!Count)
	{
		// first insertion
		Head = new tTVPRegionRect(rect);
		Head->Prev = Head->Next = Head;
		Current = Head;
		Count = 1;
		return true;
	}

	// Search insertion point and insert there.
	// Link list is sorted by upper-left point of the rectangle,
	// Search starts from "Current", which is the most recently touched rectangle.
	if(*Current > rect)
	{
		// insertion point is before Current
		tTVPRegionRect *prev;
		while(true)
		{
			if(Current == Head)
			{
				// insert before head
				if(Head->top == rect.top && Head->bottom == rect.bottom &&
					Head->Left == rect.right)
				{
					Head->Left = rect.left; // unite Head
					break;
				}
				else
				{
					tTVPRegionRect * new_rect = new tTVPRegionRect(rect);
					new_rect->LinkBefore(Head);
					Count ++;
					Head = new_rect;
					break;
				}
			}
			prev = Current->Prev;
			if(*prev < rect)
			{

				// insert between prev and Current
				if(Current->top == rect.top && Current->bottom == rect.bottom &&
					Current->left == rect.right)
				{
					Current->left = rect.left; // unite right
					break;
				}
				else 
				{
					tTVPRegionRect * new_rect = new tTVPRegionRect(rect);
					new_rect->LinkBefore(Current);
					Count ++;
					break;
				}

			}
			Current = prev;
		}
	}
	else
	{
		// insertion point is after Current
		tTVPRegionRect *next;
		while(true)
		{
			next = Current->Next;
			if(next == Head)
			{
				// insert after last of the link list
				// (that is, before the Head)
				if(Current->top == rect.top && Current->bottom == rect.bottom &&
					Current->right == rect.left)
				{
					Current->right = rect.right; // unite right
					break;
				}
				else
				{
					tTVPRegionRect * new_rect = new tTVPRegionRect(rect);
					new_rect->LinkBefore(Head);
					Count ++;
					break;
				}
			}
			if(*next > rect)
			{
				// insert between next and Current
				if(next->top == rect.top && next->bottom == rect.bottom &&
					next->left == rect.right)
				{
					next->left = rect.left; // unite right
					if(Current->top == rect.top && Current->bottom == rect.bottom &&
						Current->right == rect.left)
					{
						next->left = Current->left; // unite left
						Remove(Current);
					}
					break;
				}
				else
				{
					tTVPRegionRect * new_rect = new tTVPRegionRect(rect);
					new_rect->LinkBefore(next);
					Count ++;
					break;
				}
			}
			Current = next;
		}
	}


	return true;
}
//---------------------------------------------------------------------------
void tTVPComplexRect::Remove(tTVPRegionRect * rect)
{
	// Remove a rectangle.
	// Note that this function does not update the bounding rectangle.
	if(rect == Head) Head = rect->Next;

	Count --;
	if(!Count)
	{
		// no more rectangles
		Current = Head = NULL;
	}
	else
	{
		if(rect == Current) Current = rect->Prev;
	}
	rect->Unlink();
	delete rect;
}
//---------------------------------------------------------------------------
void tTVPComplexRect::Merge(const tTVPComplexRect & rects)
{
	// Merge non-overlaped complex rectangle

	// Calculate bounding rectangle
	(const_cast<tTVPComplexRect *>(&rects))->EnsureBound();
	EnsureBound();
	if(Count)
	{
		if(rects.Count)
			Bound.do_union(rects.Bound);
		else
			; // do nothing
	}
	else
	{
		if(rects.Count)
			Bound = rects.Bound;
		else
			return; // both empty; do nothing
	}

	// merge per a rectangle
	tTVPRegionRect * ref_cur = rects.Head;
	do
	{
		Insert(*ref_cur);
		ref_cur = ref_cur->Next;
	} while(ref_cur != rects.Head);
}
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
void tTVPComplexRect::Or(const tTVPRect &r)
{
	// OR operation

	// empty check
	if(r.is_empty()) return;

	// simply insert when no rectangle exists
	if(Count == 0)
	{
		if(Insert(r))
			Bound = r, BoundValid = true;
		return;
	}

	// Check for bounding rectangle
	EnsureBound();
	if(!Bound.intersects_with_no_empty_check(r))
	{
		// Out of the Bouding rectangle; Simply insert
		if(Insert(r))
			Bound.do_union(r);
		return;
	}

	// Check for null rectangle
	if(r.left >= r.right || r.top >= r.bottom) return; // null rect

	// Update bounding rectangle
	Bound.do_union(r);

	// Walk through rectangles
	tTVPRegionRect *c = Head;
	while(c->top < r.bottom)
	{
		// Check inclusion
		if(r.included_in_no_empty_check(*c))
		{
			// r is completely included in c
			return; //---                         returns here
		}

		// Check intersection
		bool next_is_head;
		if(c->intersects_with_no_empty_check(r))
		{
			// Do OR operation. This may increase the rectangle count.
			tTVPRegionRect *next = c->Next;
			next_is_head = next == Head;
			Current = c;
			RectangleSub(c, &r);
			c = next_is_head ? Head : next;
				// Re-assign head since the "Head" may change in "RectangleSub"
		}
		else
		{
			// Step next
			c = c->Next;
			next_is_head = c == Head;
		}

		if(!Head || !c || next_is_head) break;
	}

	// finally insert r
	Insert(r);
}
//---------------------------------------------------------------------------
void tTVPComplexRect::Or(const tTVPComplexRect &ref)
{
	// OR operation
	if(ref.Count == 0) return; // nothing to do

	EnsureBound();
	(const_cast<tTVPComplexRect *>(&ref))->EnsureBound();
	if(!Bound.intersects_with_no_empty_check(ref.Bound))
	{
		// Out of the Bouding rectangle; Simply marge
		Merge(ref);
		return;
	}

	tTVPRegionRect *c = ref.Head;
	while(true)
	{
		Or(*c);
		c = c->Next;
		if(c == ref.Head) break;
	}
}
//---------------------------------------------------------------------------
void tTVPComplexRect::Sub(const tTVPRect &r)
{
	// Subtraction operation

	// Check for null rectangle
	if(r.left >= r.right || r.top >= r.bottom) return; // null rect

	// check bounding rectangle
	EnsureBound();
	if(!Bound.intersects_with_no_empty_check(r))
	{
		// Out of the Bouding rectangle; nothing to do
		return;
	}

	switch(GetRectangleIntersectionCode(Bound, r))
	{
	case 8 + 4 + 2 + 1: // r overlaps Bound
		Clear(); // nothing remains
		return;
	case 8 + 4 + 2: // r overlaps upper of Bound
		Bound.top = r.bottom;
		break;
	case 4 + 2 + 1: // r overlaps right of Bound
		Bound.right = r.left;
		break;
	case 8 + 4 + 1: // r overlaps bottom of Bound
		Bound.bottom = r.top;
		break;
	case 8 + 2 + 1: // r overlaps left of Bound
		Bound.left = r.right;
		break;
	}

	// Walk through rectangles
	tTVPRegionRect *c = Head;
	while(c->top < r.bottom)
	{
		// Check intersection
		bool next_is_head;
		if(c->intersects_with_no_empty_check(r))
		{
			// check bounding rectangle
			if(c->left == Bound.left ||
				c->top == Bound.top ||
				c->right == Bound.top ||
				c->bottom == Bound.bottom)
			{
				// one of the rectangle edge touches bounding rectangle
				BoundValid = false; // invalidate bounding rectangle
			}

			// Do Subtract operation. This may increase the rectangle count.
			tTVPRegionRect *next = c->Next;
			next_is_head = next == Head;
			Current = c;
			RectangleSub(c, &r);
			c = next_is_head ? Head : next;
				// Re-assign head since the "Head" may change in "RectangleSub"
		}
		else
		{
			// Step next
			c = c->Next;
			next_is_head = c == Head;
		}

		if(!Head || !c || next_is_head) break;
	}
}
//---------------------------------------------------------------------------
void tTVPComplexRect::Sub(const tTVPComplexRect &ref)
{
	// Subtract operation
	if(ref.Count == 0) return; // nothing to do

	// check bounding rectangle
	EnsureBound();
	(const_cast<tTVPComplexRect *>(&ref))->EnsureBound();
	if(!Bound.intersects_with_no_empty_check(ref.Bound))
	{
		// Out of the Bouding rectangle; nothing to do
		return;
	}

	tTVPRegionRect *c = ref.Head;
	bool boundvalid = true;
	while(true)
	{
		// subtract a rectangle
		Sub(*c);

		// check bounding rectangle validity
		boundvalid = BoundValid && boundvalid;
		BoundValid = true; // force validate bounding rectangle not to recalculate it.

		// step next
		c = c->Next;
		if(c == ref.Head) break;
	}

	BoundValid = boundvalid;
}
//---------------------------------------------------------------------------
void tTVPComplexRect::And(const tTVPRect &r)
{
	// Do "logical and" operation
	if(Count == 0) return; // nothing to do

	// Check for null rectangle
	if(r.left >= r.right || r.top >= r.bottom)
	{
		Clear(); // null rectangle; nothing remains
		return;
	}

	// check bounding rectangle
	EnsureBound();
	if(!Bound.intersects_with_no_empty_check(r))
	{
		// Out of the Bouding rectangle; nothing remains
		Clear();
		return;
	}

	switch(GetRectangleIntersectionCode(Bound, r))
	{
	case 8 + 4 + 2 + 1: // r overlaps Bound
		// nothing to do
		return;
	}

	bool is_first = true;
	bool next_is_head;
	tTVPRegionRect * c = Head, *cc;
	while(true)
	{
		c = (cc = c)->Next;
		next_is_head = c == Head;
		if(cc->clip(r)) // clip with r
		{
			if(is_first)
				Bound = *cc, is_first = false;
			else
				Bound.do_union(*cc);
		}
		else
		{
			Remove(cc);
		}

		if(next_is_head) break;
	}

	if(is_first)
		BoundValid = false; // Nothing remains
	else
		BoundValid = true;

}
//---------------------------------------------------------------------------
void tTVPComplexRect::CopyWithOffsets(const tTVPComplexRect &ref, const tTVPRect &clip,
	tjs_int ofsx, tjs_int ofsy)
{
	// Copy "ref" to this.
	// "ref" is to be added offsets, and is to be done logical-and with the "clip",
	// Note that this function must be called immediately after the construction or
	// the "Clear" function (This function never clears the rectangles).

	// copy rectangles
	if(ref.Count)
	{
		tTVPRegionRect * ref_cur = ref.Head;
		bool is_first = true;
		do
		{
			tTVPRect rect(*ref_cur);
			rect.add_offsets(ofsx, ofsy);
			if(TVPIntersectRect(&rect, rect, clip))
			{
				// has intersection
				Insert(rect);
				if(is_first)
					Bound = rect, is_first = false;
				else
					Bound.do_union(rect);
			}
			ref_cur = ref_cur->Next;
		} while(ref_cur != ref.Head);

		if(is_first)
			BoundValid = false; // Nothing remains
		else
			BoundValid = true;
	}
}
//---------------------------------------------------------------------------
void tTVPComplexRect::AddOffsets(tjs_int x, tjs_int y)
{
	// Add offsets to rectangles
	if(Count == 0) return; // nothing to do

	// for bounding rectangle
	Bound.add_offsets(x, y);

	// process per a rectangle
	tTVPRegionRect * cur = Head;
	do
	{
		cur->add_offsets(x, y);
		cur = cur->Next;
	} while(cur != Head);
}
//---------------------------------------------------------------------------
void tTVPComplexRect::CalcBound()
{
	// Calculate bounding rectangle
	if(Count)
	{
		tTVPRegionRect * c = Head;
		Bound = *Head;
		c = c->Next;
		while(c != Head)
		{
			Bound.do_union(*c);
			c = c->Next;
		}
	}
	else
	{
		// no rectangles; bounding rectangle is not valid
		Bound.left = Bound.top = Bound.right = Bound.bottom = 0;
	}
}
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
void tTVPComplexRect::RectangleSub(tTVPRegionRect *r, const tTVPRect *rr)
{
	// Subtract rr from r
	tjs_int cond = GetRectangleIntersectionCode(*r, *rr);

	tTVPRect nr;
	switch(cond)
	{
//--------------------------------------------------
	case 0:
/*
     +-----------+     
	 |     r     |
	 +---+---+---+
     |nr2|rr |nr3|     
     +---+---+---+     
     |    nr4    |     
	 +-----------+
*/
		// nr2
		nr.left    = r->left;
		nr.top     = rr->top;
		nr.right   = rr->left;
		nr.bottom  = rr->bottom;
		Insert(nr);
		// nr3
		nr.left    = rr->right;
		nr.top     = rr->top;
		nr.right   = r->right;
		nr.bottom  = rr->bottom;
		Insert(nr);
		// nr4
		nr.left    = r->left;
		nr.top     = rr->bottom;
		nr.right   = r->right;
		nr.bottom  = r->bottom;
		Insert(nr);

		// r
		r->bottom  = rr->top;

		break;
//--------------------------------------------------
	case 8 + 4 + 2 + 1:
/*
+---------------------+
|                     |
|                     |
|                     |
|                     |
|         rr          |
|                     |
|                     |
|                     |
|                     |
+---------------------+
*/
		Remove(r);
		break;
//--------------------------------------------------
	case 8 + 4:
/*
     +-----------+     
	 |     r     |
+----+-----------+----+
|         rr          |
+----+-----------+----+
     |    nr2    |     
     +-----------+     
*/
		// nr2
		nr.left    = r->left;
		nr.top     = rr->bottom;
		nr.right   = r->right;
		nr.bottom  = r->bottom;
		Insert(nr);

		// r
		r->bottom  = rr->top;

		break;
//--------------------------------------------------
	case 1 + 2:
/*
         +---+         
         |   |         
     +---+   +---+     
     |   |   |   |     
     |   |   |   |     
	 | r |rr |nr2|
     |   |   |   |     
     |   |   |   |     
     +---+   +---+     
         |   |         
         +---+         
*/
		// nr2
		nr.left    = rr->right;
		nr.top     = r->top;
		nr.right   = r->right;
		nr.bottom  = r->bottom;
		Insert(nr);

		// r
		r->right   = rr->left;

		break;
//--------------------------------------------------
	case 8 + 2:
/*
+------+               
|      |               
|      +---------+     
|  rr  |         |     
|      |   nr1   |     
|      |         |     
+----+-+---------+     
     |     nr2   |     
     +-----------+     
*/
		// nr1
		nr.left    = rr->right;
		nr.top     = r->top;
		nr.right   = r->right;
		nr.bottom  = rr->bottom;
		Insert(nr);
		// nr2
		nr.left    = r->left;
		nr.top     = rr->bottom;
		nr.right   = r->right;
		nr.bottom  = r->bottom;
		Insert(nr);

		Remove(r);
		break;
//--------------------------------------------------
	case 4 + 2:
/*
               +------+
               |      |
     +---------+      |
     |         |  rr  |
	 |    r    |      |
     |         |      |
     +---------+-+----+
     |   nr2     |     
     +-----------+     
*/
		// nr2
		nr.left    = r->left;
		nr.top     = rr->bottom;
		nr.right   = r->right;
		nr.bottom  = r->bottom;
		Insert(nr);

		// r
		r->bottom  = rr->bottom;
		r->right   = rr->left;

		break;
//--------------------------------------------------
	case 4 + 1:
/*
	 +-----------+
	 |     r     |
	 +---------+-+----+
	 |         |      |
	 |    nr2  |      |
	 |         |  rr  |
	 +---------+      |
			   |      |
			   +------+
*/
		// nr2
		nr.left    = r->left;
		nr.top     = rr->top;
		nr.right   = rr->left;
		nr.bottom  = r->bottom;
		Insert(nr);

		// r
		r->bottom  = rr->top;

		break;
//--------------------------------------------------
	case 8 + 1:
/*
     +-----------+     
	 |      r    |
+----+-+---------+     
|      |         |     
|      |   nr2   |     
|  rr  |         |     
|      +---------+     
|      |               
+------+               
*/
		// nr2
		nr.left    = rr->right;
		nr.top     = rr->top;
		nr.right   = r->right;
		nr.bottom  = r->bottom;
		Insert(nr);

		// r
		r->bottom  = rr->top;

		break;
//--------------------------------------------------
	case 8 + 4 + 2:
/*
+---------------------+
|                     |
|                     |
|          rr         |
|                     |
|                     |
+----+-----------+----+
	 |     nr    |
	 +-----------+
*/
		// nr
		nr.left    = r->left;
		nr.top     = rr->bottom;
		nr.right   = r->right;
		nr.bottom  = r->bottom;
		Insert(nr);

		Remove(r);
		break;
//--------------------------------------------------
	case 4 + 2 + 1:
/*
		 +------------+
		 |            |
	 +---+            |
	 |   |            |
	 |   |            |
	 | r |     rr     |
	 |   |            |
	 |   |            |
	 +---+            |
		 |            |
		 +------------+
*/
		// r
		r->right   = rr->left;

		break;
//--------------------------------------------------
	case 8 + 4 + 1:
/*
	 +-----------+
	 |     r     |
+----+-----------+----+
|                     |
|                     |
|          rr         |
|                     |
|                     |
+---------------------+
*/
		// r
		r->bottom  = rr->top;

		break;
//--------------------------------------------------
	case 8 + 2 + 1:
/*
+------------+         
|            |         
|            +---+     
|            |   |     
|            |   |     
|    rr      |nr |     
|            |   |     
|            |   |     
|            +---+     
|            |         
+------------+         
*/
		// nr
		nr.left    = rr->right;
		nr.top     = r->top;
		nr.right   = r->right;
		nr.bottom  = r->bottom;
		Insert(nr);

		Remove(r);
		break;
//--------------------------------------------------
	case 8:
/*
     +-----------+     
	 |      r    |
+----+-+---------+     
|  rr  |   nr2   |     
+----+-+---------+     
     |     nr3   |     
     +-----------+     
*/
		// nr2
		nr.left    = rr->right;
		nr.top     = rr->top;
		nr.right   = r->right;
		nr.bottom  = rr->bottom;
		Insert(nr);
		// nr3
		nr.left    = r->left;
		nr.top     = rr->bottom;
		nr.right   = r->right;
		nr.bottom  = r->bottom;
		Insert(nr);
		// r
		r->bottom  = rr->top;

		break;
//--------------------------------------------------
	case 4:
/*
     +-----------+     
	 |    r      |
	 +---------+-+----+
     |   nr2   |  rr  |
     +---------+-+----+
     |   nr3     |     
     +-----------+     
*/
		// nr2
		nr.left    = r->left;
		nr.top     = rr->top;
		nr.right   = rr->left;
		nr.bottom  = rr->bottom;
		Insert(nr);
		// nr3
		nr.left    = r->left;
		nr.top     = rr->bottom;
		nr.right   = r->right;
		nr.bottom  = r->bottom;
		Insert(nr);

		// r
		r->bottom  = rr->top;

		break;
//--------------------------------------------------
	case 2:
/*
         +---+         
         |   |         
     +---+ rr+---+     
	 | r |   |nr2|
     +---+---+---+     
     |           |     
     |    nr3    |     
     |           |     
     +-----------+     
*/
		// nr2
		nr.left    = rr->right;
		nr.top     = r->top;
		nr.right   = r->right;
		nr.bottom  = rr->bottom;
		Insert(nr);
		// nr3
		nr.left    = r->left;
		nr.top     = rr->bottom;
		nr.right   = r->right;
		nr.bottom  = r->bottom;
		Insert(nr);

		// r
		r->right   = rr->left;
		r->bottom  = rr->bottom;

		break;
//--------------------------------------------------
	case 1:
/*
     +-----------+     
     |           |     
	 |     r     |
     |           |     
     +---+---+---+     
     |nr2|   |nr3|     
     +---+rr +---+     
         |   |         
         +---+         
*/
		// nr2
		nr.left    = r->left;
		nr.top     = rr->top;
		nr.right   = rr->left;
		nr.bottom  = r->bottom;
		Insert(nr);
		// nr3
		nr.left    = rr->right;
		nr.top     = rr->top;
		nr.right   = r->right;
		nr.bottom  = r->bottom;
		Insert(nr);

		// r
		r->bottom  = rr->top;

		break;

	default:
		return  ;
	}

}
//---------------------------------------------------------------------------

namespace TJS {
	void TVPConsoleLog(const tjs_char *l);
}
#define OutputDebugString TVPConsoleLog
//---------------------------------------------------------------------------
void tTVPComplexRect::DumpChain()
{
	ttstr str;
	tIterator it = GetIterator();
	while(it.Step()) {
		tjs_char tmp[200];
		TJS_snprintf(tmp, 200, TJS_W("%p (%p) %p : "), it.Get().Prev, &(it.Get()), it.Get().Next);
		str += tmp;
	}
	OutputDebugString(str.c_str());
}
//---------------------------------------------------------------------------

