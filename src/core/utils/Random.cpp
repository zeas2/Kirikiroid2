//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Simple Pseudo Random Generator
//---------------------------------------------------------------------------
#include "tjsCommHead.h"

#include "Random.h"
#include "md5.h"
#include "tjsUtils.h"

// this implements simple environment-noise based pseudo random generator using MD5.
// may not be suitable for high security usage.

//---------------------------------------------------------------------------
tjs_uint8 TVPRandomSeedPool[0x1000 + 512];
	// 512 is to avoid buffer over-run posibility in multi-threaded access 
tjs_int TVPRandomSeedPoolPos = 0;
tjs_uint8 TVPRandomSeedAtom; // need not to initialize
//---------------------------------------------------------------------------
void TVPPushEnvironNoise(const void *buf, tjs_int bufsize)
{
	const tjs_uint8 *p = (const tjs_uint8 *)buf;
	for(int i = 0; i < bufsize; i++)
	{
		TVPRandomSeedPool[TVPRandomSeedPoolPos ++] ^=
			(TVPRandomSeedAtom ^= p[i]);
		TVPRandomSeedPoolPos &= 0xfff;
	}
	TVPRandomSeedPoolPos += (p[0]&1);
	TVPRandomSeedPoolPos &= 0xfff;
}
//---------------------------------------------------------------------------
void TVPGetRandomBits128(void *dest)
{
	// retrieve random bits

	// add some noise
	tjs_uint8 buf[16];
	TVPPushEnvironNoise(buf, 16); // dare to use uninitialized buffer
	TVPPushEnvironNoise(&TVPRandomSeedPoolPos, sizeof(TVPRandomSeedPoolPos));

	// make 128bit hash of TVPRandomSeedPool, using MD5 message digest
	md5_state_t state;
	md5_init(&state);
	md5_append(&state, TVPRandomSeedPool, 0x1000);
	md5_finish(&state, buf);

	memcpy(dest, buf, 16);

	// push hash itself
	TVPPushEnvironNoise(buf, 16);
}
//---------------------------------------------------------------------------

