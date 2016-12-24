//---------------------------------------------------------------------------
/*
	TJS2 Script Engine
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Math.RandomGenerator implementation
//---------------------------------------------------------------------------

#ifndef tjsRandomGeneratorH
#define tjsRandomGeneratorH

#include <time.h>
#include "tjsNative.h"
#include "tjsMT19937ar-cok.h"

namespace TJS
{
//---------------------------------------------------------------------------
extern void (*TJSGetRandomBits128)(void *dest);
    // retrives 128-bits (16bytes) random bits for random seed.
    // this can be override application-specified routine, otherwise
    // TJS2 uses current time as a random seed.
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
class tTJSNI_RandomGenerator : public tTJSNativeInstance
{
public:
	tTJSNI_RandomGenerator();
    ~tTJSNI_RandomGenerator();
private:
	tTJSMersenneTwister *Generator;

public:
	iTJSDispatch2 * Serialize();

	void Randomize(tTJSVariant ** param, tjs_int numparams);
	double Random();
	tjs_uint32 Random32();
	tjs_int64 Random63();
	tjs_int64 Random64();
};
//---------------------------------------------------------------------------




//---------------------------------------------------------------------------
class tTJSNC_RandomGenerator : public tTJSNativeClass
{
public:
	tTJSNC_RandomGenerator();

	static tjs_uint32 ClassID;

private:
	tTJSNativeInstance *CreateNativeInstance();
};
//---------------------------------------------------------------------------
} // namespace TJS

#endif
