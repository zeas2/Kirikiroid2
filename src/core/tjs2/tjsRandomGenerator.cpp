//---------------------------------------------------------------------------
/*
	TJS2 Script Engine
	Copyright (C) 2000-2005	 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Math.RandomGenerator implementation
//---------------------------------------------------------------------------
#include "tjsCommHead.h"


#include <time.h>
#include "tjsError.h"
#include "tjsRandomGenerator.h"
#include "tjsDictionary.h"
#include "tjsLex.h"


namespace TJS
{
//---------------------------------------------------------------------------
void (*TJSGetRandomBits128)(void *dest) = NULL;
	// retrives 128-bits (16bytes) random bits for random seed.
	// this can be override application-specified routine, otherwise
	// TJS2 uses current time as a random seed.
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// tTJSNI_RandomGenerator : TJS Native Instance : RandomGenerator
//---------------------------------------------------------------------------
tTJSNI_RandomGenerator::tTJSNI_RandomGenerator()
{
	// C++ constructor
	Generator = NULL;
}
//---------------------------------------------------------------------------
tTJSNI_RandomGenerator::~tTJSNI_RandomGenerator()
{
	// C++ destructor
	if(Generator) delete Generator;
}
//---------------------------------------------------------------------------
void tTJSNI_RandomGenerator::Randomize(tTJSVariant ** param, tjs_int numparams)
{
	if(numparams == 0)
	{
		// parametor not given
		if(TJSGetRandomBits128)
		{
			// another random generator is given
			tjs_uint8 buf[32];
			unsigned long tmp[32];
			TJSGetRandomBits128(buf);
			TJSGetRandomBits128(buf+16);
			for(tjs_int i = 0; i < 32; i++)
				tmp[i] = buf[i] + (buf[i] << 8) +
				(buf[1] << 16) + (buf[i] << 24);

			if(Generator) delete Generator, Generator = NULL;
			Generator = new tTJSMersenneTwister(tmp, 32);
		}
		else
		{
			time_t tm;

			if(Generator) delete Generator, Generator = NULL;
			Generator = new tTJSMersenneTwister((unsigned long)time(&tm));
		}
	}
	else if(numparams >= 1)
	{
		if(param[0]->Type() == tvtObject)
		{
			tTJSMersenneTwisterData *data = NULL;
			try
			{
				// may be a reconstructible information
				tTJSVariantClosure clo = param[0]->AsObjectClosureNoAddRef();
				if(!clo.Object) TJS_eTJSError(TJSNullAccess);


				ttstr state;
				tTJSVariant val;

				data = new tTJSMersenneTwisterData;

				// get state array
				TJS_THROW_IF_ERROR(clo.PropGet(TJS_MEMBERMUSTEXIST, TJS_W("state"),
					NULL, &val, NULL));

				state = val;
				if(state.GetLen() != TJS_MT_N * 8)
					TJS_eTJSError(TJSNotReconstructiveRandomizeData);

				const tjs_char *p = state.c_str();

				for(tjs_int i = 0; i < TJS_MT_N; i++)
				{
					tjs_uint32 n = 0;
					tjs_int tmp;

					for(tjs_int j = 0; j < 8; j++)
					{
						tmp = TJSHexNum(p[j]);
						if(tmp == -1)
							TJS_eTJSError(TJSNotReconstructiveRandomizeData);
						else
							n <<= 4, n += tmp;
					}

					p += 8;
					data->state[i] = (unsigned long)(n);
				}

				// get other members
				TJS_THROW_IF_ERROR(clo.PropGet(TJS_MEMBERMUSTEXIST, TJS_W("left"), NULL,
					&val, NULL));
				data->left = (tjs_int)val;

				TJS_THROW_IF_ERROR(clo.PropGet(TJS_MEMBERMUSTEXIST, TJS_W("next"), NULL,
					&val, NULL));
				data->next = (tjs_int)val + data->state;

				if(Generator) delete Generator, Generator = NULL;
				Generator = new tTJSMersenneTwister(*data);
			}
			catch(...)
			{
				if(data) delete data;
				TJS_eTJSError(TJSNotReconstructiveRandomizeData);
			}
			delete data;
		}
		else
		{
			tjs_uint64 n = (tjs_int64)*param[0];
			unsigned long tmp[2];
			tmp[0] = (unsigned long)n;
			tmp[1] = (unsigned long)(n>>32);
  			if(Generator) delete Generator, Generator = NULL;
			Generator = new tTJSMersenneTwister(tmp, 2);
		}
	}

}
//---------------------------------------------------------------------------
iTJSDispatch2 * tTJSNI_RandomGenerator::Serialize()
{
	// create dictionary object which has reconstructible information
	// which can be passed into constructor or randomize method.
	if(!Generator) return NULL;


	iTJSDispatch2 * dic = NULL;
	tTJSVariant val;

	// retrive tTJSMersenneTwisterData
	const tTJSMersenneTwisterData & data = Generator->GetData();

	try
	{
		// create 'state' string
		ttstr state;
		tjs_char *p = state.AllocBuffer(TJS_MT_N * 8);
		for(tjs_int i = 0; i < TJS_MT_N; i++)
		{
			const static tjs_char hex[] = TJS_W("0123456789abcdef");
			p[0] = hex[(data.state[i]  >> 28) & 0x000f];
			p[1] = hex[(data.state[i]  >> 24) & 0x000f];
			p[2] = hex[(data.state[i]  >> 20) & 0x000f];
			p[3] = hex[(data.state[i]  >> 16) & 0x000f];
			p[4] = hex[(data.state[i]  >> 12) & 0x000f];
			p[5] = hex[(data.state[i]  >>  8) & 0x000f];
			p[6] = hex[(data.state[i]  >>  4) & 0x000f];
			p[7] = hex[(data.state[i]  >>  0) & 0x000f];
			p += 8;
		}
		state.FixLen();

		// create dictionary and store information
		dic = TJSCreateDictionaryObject();

		val = state;
		dic->PropSet(TJS_MEMBERENSURE, TJS_W("state"), NULL, &val, dic);

		val = (tjs_int)data.left;
		dic->PropSet(TJS_MEMBERENSURE, TJS_W("left"), NULL, &val, dic);

		val = (tjs_int)(data.next - data.state);
		dic->PropSet(TJS_MEMBERENSURE, TJS_W("next"), NULL, &val, dic);
	}
	catch(...)
	{
		if(dic) dic->Release();
		throw;
	}
	return dic;
}
//---------------------------------------------------------------------------
double tTJSNI_RandomGenerator::Random()
{
	// returns double precision random value x, x is in 0 <= x < 1
	if(!Generator) return 0;
	return Generator->rand_double();

}
//---------------------------------------------------------------------------
tjs_uint32 tTJSNI_RandomGenerator::Random32()
{
	// returns 63 bit integer random value
	if(!Generator) return 0;

	return Generator->int32();;
}
//---------------------------------------------------------------------------
tjs_int64 tTJSNI_RandomGenerator::Random63()
{
	// returns 63 bit integer random value
	if(!Generator) return 0;

	tjs_uint64 v;
	((tjs_uint32 *)&v)[0]   = Generator->int32();
	((tjs_uint32 *)&v)[1]   = Generator->int32();

	return v & TJS_UI64_VAL(0x7fffffffffffffff);
}
//---------------------------------------------------------------------------
tjs_int64 tTJSNI_RandomGenerator::Random64()
{
	// returns 64 bit integer random value
	if(!Generator) return 0;

	tjs_uint64 v;
	((tjs_uint32 *)&v)[0]   = Generator->int32();
	((tjs_uint32 *)&v)[1]   = Generator->int32();

	return v;
}
//---------------------------------------------------------------------------













//---------------------------------------------------------------------------
// tTJSNC_RandomGenerator : TJS Native Class : RandomGenerator
//---------------------------------------------------------------------------
tjs_uint32 tTJSNC_RandomGenerator::ClassID = (tjs_uint32)-1;
tTJSNC_RandomGenerator::tTJSNC_RandomGenerator() :
	tTJSNativeClass(TJS_W("RandomGenerator"))
{
	// class constructor

	TJS_BEGIN_NATIVE_MEMBERS(/*TJS class name*/RandomGenerator)
	TJS_DECL_EMPTY_FINALIZE_METHOD
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_CONSTRUCTOR_DECL(/*var. name*/_this, /*var. type*/tTJSNI_RandomGenerator,
	/*TJS class name*/ RandomGenerator)
{
	_this->Randomize(param, numparams);

	return TJS_S_OK;
}
TJS_END_NATIVE_CONSTRUCTOR_DECL(/*TJS class name*/RandomGenerator)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/randomize)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_RandomGenerator);

	_this->Randomize(param, numparams);

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/randomize)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/random)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_RandomGenerator);

	// returns 53-bit precision random value x, x is in 0 <= x < 1

	if(result)
		*result = _this->Random();
	else
		_this->Random(); // discard result

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/random)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/random32)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_RandomGenerator);

	// returns 32-bit precision integer value x, x is in 0 <= x <= 4294967295

	if(result)
		*result = (tjs_int64)_this->Random32();
	else
		_this->Random32(); // discard result

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/random32)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/random63)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_RandomGenerator);

	// returns 63-bit precision integer value x, x is in 0 <= x <= 9223372036854775807

	if(result)
		*result = _this->Random63();
	else
		_this->Random63(); // discard result

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/random63)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/random64)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_RandomGenerator);

	// returns 64-bit precision integer value x, x is in
	// -9223372036854775808 <= x <= 9223372036854775807

	if(result)
		*result = _this->Random64();
	else
		_this->Random64(); // discard result

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/random64)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/serialize)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_RandomGenerator);

	if(result)
	{
		iTJSDispatch2 * dsp = _this->Serialize();
		*result = tTJSVariant(dsp, dsp);
		dsp->Release();
	}

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/serialize)
//----------------------------------------------------------------------
	TJS_END_NATIVE_MEMBERS
}
//---------------------------------------------------------------------------







//---------------------------------------------------------------------------
tTJSNativeInstance *tTJSNC_RandomGenerator::CreateNativeInstance()
{
	return new tTJSNI_RandomGenerator();
}
//---------------------------------------------------------------------------
} // namespace TJS

//---------------------------------------------------------------------------

