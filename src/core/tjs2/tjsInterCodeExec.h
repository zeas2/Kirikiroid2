//---------------------------------------------------------------------------
/*
	TJS2 Script Engine
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Intermediate Code Execution
//---------------------------------------------------------------------------

#ifndef tjsInterCodeExecH
#define tjsInterCodeExecH


namespace TJS
{
//---------------------------------------------------------------------------
#if 0
extern void TJSVariantArrayStackAddRef();
extern void TJSVariantArrayStackRelease();
#endif
extern void TJSVariantArrayStackCompact();
extern void TJSVariantArrayStackCompactNow();

class tTJSVariantArrayStack
{
	//	tTJSCriticalSection CS;

	struct tVariantArray
	{
		tTJSVariant *Array;
		tjs_int Using;
		tjs_int Allocated;
	};

	tVariantArray * Arrays; // array of array
	tjs_int NumArraysAllocated;
	tjs_int NumArraysUsing;
	tVariantArray * Current;
	tjs_int CompactVariantArrayMagic;
	tjs_int OperationDisabledCount;

	void IncreaseVariantArray(tjs_int num);

	void DecreaseVariantArray(void);

	void InternalCompact(void);


public:
	tTJSVariantArrayStack();
	~tTJSVariantArrayStack();

	tTJSVariant * Allocate(tjs_int num);

	void Deallocate(tjs_int num, tTJSVariant *ptr);

	void Compact() { InternalCompact(); }

}/* *TJSVariantArrayStack = NULL*/;
//---------------------------------------------------------------------------
}


#endif
