//---------------------------------------------------------------------------
/*
	TJS2 Script Engine
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// TJS Array class implementation
//---------------------------------------------------------------------------


#include "tjsCommHead.h"

#include <algorithm>
#include <functional>
#include "tjsArray.h"
#include "tjsDictionary.h"
#include "tjsUtils.h"
#include "tjsBinarySerializer.h"
#include "tjsOctPack.h"

#ifndef TJS_NO_REGEXP
#include "tjsRegExp.h"
#endif

// TODO: Check the deque's exception handling on deleting


#define TJS_ARRAY_BASE_HASH_BITS 3
	/* hash bits for base "Object" hash */



namespace TJS
{
//---------------------------------------------------------------------------
static tjs_int32 ClassID_Array;
//---------------------------------------------------------------------------
static bool inline TJS_iswspace(tjs_char ch)
{
	// the standard iswspace misses when non-zero page code
	if(ch&0xff00) return false; else return 0!=isspace(ch);
}
//---------------------------------------------------------------------------
static bool inline TJS_iswdigit(tjs_char ch)
{
	// the standard iswdigit misses when non-zero page code
	if(ch&0xff00) return false; else return 0!=isdigit(ch);
}
//---------------------------------------------------------------------------
// Utility Function(s)
//---------------------------------------------------------------------------
static bool IsNumber(const tjs_char *str, tjs_int &result)
{
	// when str indicates a number, this function converts it to
	// number and put to result, and returns true.
	// otherwise returns false.
	if(!str) return false;
	const tjs_char *orgstr = str;

	if(!*str) return false;
	while(*str && TJS_iswspace(*str)) str++;
	if(!*str) return false;
	if(*str == TJS_W('-')) str++; // sign
	else if(*str == TJS_W('+')) str++, orgstr = str; // sign, but skip
	if(!*str) return false;
	while(*str && TJS_iswspace(*str)) str++;
	if(!*str) return false;
	if(!TJS_iswdigit(*str)) return false;
	while(*str && (TJS_iswdigit(*str) || *str == '.')) str++;
	while(*str && TJS_iswspace(*str)) str++;
	if(*str == 0)
	{
		result = TJS_atoi(orgstr);
		return true;
	}
	return false;
}
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// tTJSStringAppender
//---------------------------------------------------------------------------
#define TJS_STRINGAPPENDER_DATA_INC 8192
tTJSStringAppender::tTJSStringAppender()
{
	Data = NULL;
	DataLen = 0;
	DataCapacity = 0;
}
//---------------------------------------------------------------------------
tTJSStringAppender::~tTJSStringAppender()
{
	if(Data) TJSVS_free(Data);
}
//---------------------------------------------------------------------------
void tTJSStringAppender::Append(const tjs_char *string, tjs_int len)
{
	if(!Data)
	{
		if(len > 0)
		{
			Data = TJSVS_malloc(DataCapacity = len * sizeof(tjs_char));
			memcpy(Data, string, DataCapacity);
			DataLen = len;
		}
	}
	else
	{
		if(DataLen + len > DataCapacity)
		{
			DataCapacity = DataLen + len + TJS_STRINGAPPENDER_DATA_INC;
			Data = TJSVS_realloc(Data, DataCapacity * sizeof(tjs_char));
		}
		memcpy(Data + DataLen, string, len * sizeof(tjs_char));
		DataLen += len;
	}
}
//---------------------------------------------------------------------------






//---------------------------------------------------------------------------
// tTJSArraySortCompare  : a class for comarison operator
//---------------------------------------------------------------------------
class tTJSArraySortCompare_NormalAscending :
	public std::binary_function<const tTJSVariant &, const tTJSVariant &, bool>
{
public:
	result_type operator () (first_argument_type lhs, second_argument_type rhs) const
	{
		return (lhs < rhs).operator bool();
	}
};
class tTJSArraySortCompare_NormalDescending :
	public std::binary_function<const tTJSVariant &, const tTJSVariant &, bool>
{
public:
	result_type operator () (first_argument_type lhs, second_argument_type rhs) const
	{
		return (lhs > rhs).operator bool();
	}
};
class tTJSArraySortCompare_NumericAscending :
	public std::binary_function<const tTJSVariant &, const tTJSVariant &, bool>
{
public:
	result_type operator () (first_argument_type lhs, second_argument_type rhs) const
	{
		if(lhs.Type() == tvtString && rhs.Type() == tvtString)
		{
			tTJSVariant ltmp(lhs), rtmp(rhs);
			ltmp.tonumber();
			rtmp.tonumber();
			return (ltmp < rtmp).operator bool();
		}
		return (lhs < rhs).operator bool();
	}
};
class tTJSArraySortCompare_NumericDescending :
	public std::binary_function<const tTJSVariant &, const tTJSVariant &, bool>
{
public:
	result_type operator () (first_argument_type lhs, second_argument_type rhs) const
	{
		if(lhs.Type() == tvtString && rhs.Type() == tvtString)
		{
			tTJSVariant ltmp(lhs), rtmp(rhs);
			ltmp.tonumber();
			rtmp.tonumber();
			return (ltmp > rtmp).operator bool();
		}
		return (lhs > rhs).operator bool();
	}
};
class tTJSArraySortCompare_StringAscending :
	public std::binary_function<const tTJSVariant &, const tTJSVariant &, bool>
{
public:
	result_type operator () (first_argument_type lhs, second_argument_type rhs) const
	{
		if(lhs.Type() == tvtString && rhs.Type() == tvtString)
			return (lhs < rhs).operator bool();
		return (ttstr)lhs < (ttstr)rhs;
	}
};
class tTJSArraySortCompare_StringDescending :
	public std::binary_function<const tTJSVariant &, const tTJSVariant &, bool>
{
public:
	result_type operator () (first_argument_type lhs, second_argument_type rhs) const
	{
		if(lhs.Type() == tvtString && rhs.Type() == tvtString)
			return (lhs > rhs).operator bool();
		return (ttstr)lhs > (ttstr)rhs;
	}
};
class tTJSArraySortCompare_Functional :
	public std::binary_function<const tTJSVariant &, const tTJSVariant &, bool>
{
	tTJSVariantClosure Closure;
public:
	tTJSArraySortCompare_Functional(const tTJSVariantClosure &clo) :
		Closure(clo)
	{
	}

	result_type operator () (first_argument_type lhs, second_argument_type rhs) const
	{

		tTJSVariant result;

		tjs_error hr;
		tTJSVariant *param[] = {
			const_cast<tTJSVariant *>(&lhs), // note that doing cast to non-const pointer
			const_cast<tTJSVariant *>(&rhs) };

		hr = Closure.FuncCall(0, NULL, NULL, &result, 2, param, NULL);

		if(TJS_FAILED(hr))
			TJSThrowFrom_tjs_error(hr);

		return result.operator bool();
	}
};
//---------------------------------------------------------------------------





//---------------------------------------------------------------------------
// tTJSArrayClass : tTJSArray class
//---------------------------------------------------------------------------
tjs_uint32 tTJSArrayClass::ClassID = (tjs_uint32)-1;
tTJSArrayClass::tTJSArrayClass() :
	tTJSNativeClass(TJS_W("Array"))
{
	// class constructor

	TJS_BEGIN_NATIVE_MEMBERS(/* TJS class name */Array)

//---------------------------------------------------------------------------
TJS_BEGIN_NATIVE_CONSTRUCTOR_DECL(/* var. name */_this, /* var. type */tTJSArrayNI,
	/* TJS class name */ Array)
{
	return TJS_S_OK;
}
TJS_END_NATIVE_CONSTRUCTOR_DECL(/*TJS class name*/Array)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/* func. name */load)
{
	// loads a file into this array.
	// each a line becomes an array element.

	TJS_GET_NATIVE_INSTANCE(/* var. name */ni, /* var. type */tTJSArrayNI);

	if(numparams < 1) return TJS_E_BADPARAMCOUNT;

	ttstr name(*param[0]);
	ttstr mode;
	if(numparams >= 2 && param[1]->Type() != tvtVoid) mode =*param[1];

	iTJSTextReadStream * stream = TJSCreateTextStreamForRead(name, mode);
	try
	{
		ni->Items.clear();
		ttstr content;
		stream->Read(content, 0);
		const tjs_char * p = content.c_str();
		const tjs_char * sp = p;
		tjs_uint l;

		// count lines
		tjs_uint lines = 0;
		while(*p)
		{
			if(*p == TJS_W('\r') || *p == TJS_W('\n'))
			{
				tjs_uint l = (tjs_uint)(p - sp);

				p++;
				if(p[-1] == TJS_W('\r') && p[0] == TJS_W('\n')) p++;

				lines++;

				sp = p;
				continue;
			}
			p++;
		}

		l = (tjs_uint)(p - sp);
		if(l)
		{
			lines++;
		}

		ni->Items.resize(lines);

		// split to each line
		p = content.c_str();
		sp = p;
		lines = 0;

		tTJSVariantString *vs;

		try
		{
			while(*p)
			{
				if(*p == TJS_W('\r') || *p == TJS_W('\n'))
				{
					tjs_uint l = (tjs_uint)(p - sp);

					p++;
					if(p[-1] == TJS_W('\r') && p[0] == TJS_W('\n')) p++;

					vs = TJSAllocVariantString(sp, l);
					ni->Items[lines++] = vs;
					if(vs) vs->Release(), vs = NULL;

					sp = p;
					continue;
				}
				p++;
			}

			l = (tjs_uint)(p - sp);
			if(l)
			{
				vs = TJSAllocVariantString(sp, l);
				ni->Items[lines] = vs;
				if(vs) vs->Release(), vs = NULL;
			}
		}
		catch(...)
		{
			if(vs) vs->Release();
			throw;
		}


	}
	catch(...)
	{
		stream->Destruct();
		throw;
	}
	stream->Destruct();

	if(result) *result = tTJSVariant(objthis, objthis);

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/* func.name */load)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/loadStruct)
{
	TJS_GET_NATIVE_INSTANCE(/* var. name */ni, /* var. type */tTJSArrayNI);

	if(numparams<1) return TJS_E_BADPARAMCOUNT;

	ttstr name(*param[0]);
	ttstr mode;
	if(numparams >= 2 && param[1]->Type() != tvtVoid) mode =*param[1];

	ni->Items.clear();

	tTJSBinaryStream* stream = TJSCreateBinaryStreamForRead(name, mode);
	if( !stream ) return TJS_E_INVALIDPARAM;

	bool isbin = false;
	try {
		tjs_uint64 streamlen = stream->GetSize();
		if( streamlen >= tTJSBinarySerializer::HEADER_LENGTH ) {
			tjs_uint8 header[tTJSBinarySerializer::HEADER_LENGTH];
			stream->Read( header, tTJSBinarySerializer::HEADER_LENGTH );
			if( tTJSBinarySerializer::IsBinary( header ) ) {
				tTJSBinarySerializer binload((tTJSArrayObject*)objthis);
				tTJSVariant* var = binload.Read( stream );
				if( var ) {
					if( result ) *result = *var;
					delete var;
					isbin = true;
				}
			}
		}
	} catch(...) {
		delete stream;
		throw;
	}
	delete stream;
	if( isbin ) return TJS_S_OK;
	return TJS_E_INVALIDPARAM;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/loadStruct)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/* func. name */save)
{
	// saves the array into a file.
	// only string and number stuffs are stored.

	TJS_GET_NATIVE_INSTANCE(/* var. name */ni, /* var. type */tTJSArrayNI);

	if(numparams < 1) return TJS_E_BADPARAMCOUNT;

	ttstr name(*param[0]);
	ttstr mode;
	if(numparams >= 2 && param[1]->Type() != tvtVoid) mode = *param[1];
	iTJSTextWriteStream * stream = TJSCreateTextStreamForWrite(name, mode);
	try
	{
		tTJSArrayNI::tArrayItemIterator i = ni->Items.begin();
#ifdef TJS_TEXT_OUT_CRLF
		const static ttstr cr(TJS_W("\r\n"));
#else
		const static ttstr cr(TJS_W("\n"));
#endif

		while( i != ni->Items.end())
		{
			tTJSVariantType type = i->Type();
			if(type == tvtString || type == tvtInteger || type == tvtReal)
			{
				stream->Write(*i);
			}
			stream->Write(cr);
			i++;
		}
	}
	catch(...)
	{
		stream->Destruct();
		throw;
	}
	stream->Destruct();

	if(result) *result = tTJSVariant(objthis, objthis);

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/* func.name */save)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/* func. name */saveStruct)
{
	// saves the array into a file, that can be interpret as an expression.

	TJS_GET_NATIVE_INSTANCE(/* var. name */ni, /* var. type */tTJSArrayNI);

	if(numparams < 1) return TJS_E_BADPARAMCOUNT;

	ttstr name(*param[0]);
	ttstr mode;
	if(numparams >= 2 && param[1]->Type() != tvtVoid) mode = *param[1];

	if( TJS_strchr(mode.c_str(), TJS_W('b')) != NULL ) {
		tTJSBinaryStream* stream = TJSCreateBinaryStreamForWrite(name, mode);
		try {
			stream->Write( tTJSBinarySerializer::HEADER, tTJSBinarySerializer::HEADER_LENGTH );
			std::vector<iTJSDispatch2 *> stack;
			stack.push_back(objthis);
			ni->SaveStructuredBinary(stack, *stream);
		} catch(...) {
			delete stream;
			throw;
		}
		delete stream;
	} else {
		iTJSTextWriteStream * stream = TJSCreateTextStreamForWrite(name, mode);
		try
		{
			std::vector<iTJSDispatch2 *> stack;
			stack.push_back(objthis);
			ni->SaveStructuredData(stack, *stream, TJS_W(""));
		}
		catch(...)
		{
			stream->Destruct();
			throw;
		}
		stream->Destruct();
	}

	if(result) *result = tTJSVariant(objthis, objthis);

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/* func.name */saveStruct)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/* func.name */split)
{
	// split string with given delimiters.

	// arguments are : <pattern/delimiter>, <string>, [<reserved>],
	// [<whether ignore empty element>]

	TJS_GET_NATIVE_INSTANCE(/* var. name */ni, /* var. type */tTJSArrayNI);

	if(numparams < 2) return TJS_E_BADPARAMCOUNT;

	ni->Items.resize(0);
	tTJSString string = *param[1];
	bool purgeempty = false;

	if(numparams >= 4 && param[3]->Type() != tvtVoid)
		purgeempty = param[3]->operator bool();

#ifndef TJS_NO_REGEXP
	if(param[0]->Type() == tvtObject)
	{
		tTJSNI_RegExp * re = NULL;
		tTJSVariantClosure clo = param[0]->AsObjectClosureNoAddRef();
		if(clo.Object)
		{
			if(TJS_SUCCEEDED(clo.Object->NativeInstanceSupport(
				TJS_NIS_GETINSTANCE,
				tTJSNC_RegExp::ClassID, (iTJSNativeInstance**)&re)))
			{
				// param[0] is regexp
				iTJSDispatch2 *array = objthis;
				re->Split(&array, string, purgeempty);

				if(result) *result = tTJSVariant(objthis, objthis);

				return TJS_S_OK;
			}
		}
	}
#endif

	tTJSString pattern = *param[0];


	// split with delimiter
	const tjs_char *s = string.c_str();
	const tjs_char *delim = pattern.c_str();
	const tjs_char *sstart = s;
	while(*s)
	{
		if(TJS_strchr(delim, *s) != NULL)
		{
			// delimiter found
			if(!purgeempty || (purgeempty && (s-sstart)!=0) )
			{
				ni->Items.push_back(tTJSString(sstart, (int)(s-sstart)));
			}
			s++;
			sstart = s;
		}
		else
		{
			s++;
		}
	}

	if(!purgeempty || (purgeempty && (s-sstart)!=0) )
	{
		ni->Items.push_back(tTJSString(sstart, (int)(s-sstart)));
	}

	if(result) *result = tTJSVariant(objthis, objthis);

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/* func.name */split)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/* func.name */join)
{
	// join string with given delimiters.

	// arguments are : <delimiter>, [<reserved>],
	// [<whether ignore empty element>]

	TJS_GET_NATIVE_INSTANCE(/* var. name */ni, /* var. type */tTJSArrayNI);

	if(numparams < 1) return TJS_E_BADPARAMCOUNT;

	tTJSString delimiter = *param[0];

	bool purgeempty = false;

	if(numparams >= 3 && param[2]->Type() != tvtVoid)
		purgeempty = param[2]->operator bool();

	// join with delimiter
	bool first = true;
	tTJSString out;
	tTJSArrayNI::tArrayItemIterator i;
	for(i = ni->Items.begin(); i != ni->Items.end(); i++)
	{
		if(purgeempty && i->Type() == tvtVoid) continue;

		if(!first) out += delimiter;
		first = false;
		out += (tTJSString)(*i);
	}

	if(result) *result = out;

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/* func.name */join)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/* func.name */sort)
{
	// sort array items.

	// arguments are : [<sort order/comparison function>], [<whether to do stable sort>]

	// sort order is one of:
	// '+' (default)   :  Normal ascending  (comparison by tTJSVariant::operator < )
	// '-'             :  Normal descending (comparison by tTJSVariant::operator < )
	// '0'             :  Numeric value ascending
	// '9'             :  Numeric value descending
	// 'a'             :  String ascending
	// 'z'             :  String descending

	TJS_GET_NATIVE_INSTANCE(/* var. name */ni, /* var. type */tTJSArrayNI);

	tjs_nchar method = TJS_N('+');
	bool do_stable_sort = false;
	tTJSVariantClosure closure;

	if(numparams >= 1 && param[0]->Type() != tvtVoid)
	{
		// check first argument
		if(param[0]->Type() == tvtObject)
		{
			// comarison function object
			closure = param[0]->AsObjectClosureNoAddRef();
			method = 0;
		}
		else
		{
			// sort order letter
			ttstr me = *param[0];
			method = (tjs_nchar)(me.c_str()[0]);

			switch(method)
			{
			case TJS_N('+'):
			case TJS_N('-'):
			case TJS_N('0'):
			case TJS_N('9'):
			case TJS_N('a'):
			case TJS_N('z'):
				break;
			default:
				method = TJS_N('+');
				break;
			}
		}
	}

	if(numparams >= 2 && param[1]->Type() != tvtVoid)
	{
		// whether to do a stable sort
		do_stable_sort = param[1]->operator bool();
	}


	// sort
	switch(method)
	{
	case TJS_N('+'):
		if(do_stable_sort)
			std::stable_sort(ni->Items.begin(), ni->Items.end(),
				tTJSArraySortCompare_NormalAscending());
		else
			std::sort(ni->Items.begin(), ni->Items.end(),
				tTJSArraySortCompare_NormalAscending());
		break;
	case TJS_N('-'):
		if(do_stable_sort)
			std::stable_sort(ni->Items.begin(), ni->Items.end(),
				tTJSArraySortCompare_NormalDescending());
		else
			std::sort(ni->Items.begin(), ni->Items.end(),
				tTJSArraySortCompare_NormalDescending());
		break;
	case TJS_N('0'):
		if(do_stable_sort)
			std::stable_sort(ni->Items.begin(), ni->Items.end(),
				tTJSArraySortCompare_NumericAscending());
		else
			std::sort(ni->Items.begin(), ni->Items.end(),
				tTJSArraySortCompare_NumericAscending());
		break;
	case TJS_N('9'):
		if(do_stable_sort)
			std::stable_sort(ni->Items.begin(), ni->Items.end(),
				tTJSArraySortCompare_NumericDescending());
		else
			std::sort(ni->Items.begin(), ni->Items.end(),
				tTJSArraySortCompare_NumericDescending());
		break;
	case TJS_N('a'):
		if(do_stable_sort)
			std::stable_sort(ni->Items.begin(), ni->Items.end(),
				tTJSArraySortCompare_StringAscending());
		else
			std::sort(ni->Items.begin(), ni->Items.end(),
				tTJSArraySortCompare_StringAscending());
		break;
	case TJS_N('z'):
		if(do_stable_sort)
			std::stable_sort(ni->Items.begin(), ni->Items.end(),
				tTJSArraySortCompare_StringDescending());
		else
			std::sort(ni->Items.begin(), ni->Items.end(),
				tTJSArraySortCompare_StringDescending());
		break;
	case 0:
		if(do_stable_sort)
			std::stable_sort(ni->Items.begin(), ni->Items.end(),
				tTJSArraySortCompare_Functional(closure));
		else
			std::sort(ni->Items.begin(), ni->Items.end(),
				tTJSArraySortCompare_Functional(closure));
		break;
	}

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/* func.name */sort)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/* func.name */reverse)
{
	TJS_GET_NATIVE_INSTANCE(/* var. name */ni, /* var. type */tTJSArrayNI);

	// reverse array
	std::reverse(ni->Items.begin(), ni->Items.end());

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/* func.name */reverse)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/* func.name */assign)
{
	TJS_GET_NATIVE_INSTANCE(/* var. name */ni, /* var. type */tTJSArrayNI);

	if(numparams < 1) return TJS_E_BADPARAMCOUNT;

	((tTJSArrayObject*)objthis)->Clear(ni);

	tTJSVariantClosure clo = param[0]->AsObjectClosureNoAddRef();
	if(clo.ObjThis)
		ni->Assign(clo.ObjThis);
	else if(clo.Object)
		ni->Assign(clo.Object);
	else TJS_eTJSError(TJSNullAccess);

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/* func.name */assign)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/* func.name */assignStruct)
{
	TJS_GET_NATIVE_INSTANCE(/* var. name */ni, /* var. type */tTJSArrayNI);

	if(numparams < 1) return TJS_E_BADPARAMCOUNT;

	((tTJSArrayObject*)objthis)->Clear(ni);
	std::vector<iTJSDispatch2 *> stack;

	tTJSVariantClosure clo = param[0]->AsObjectClosureNoAddRef();
	if(clo.ObjThis)
		ni->AssignStructure(clo.ObjThis, stack);
	else if(clo.Object)
		ni->AssignStructure(clo.Object, stack);
	else TJS_eTJSError(TJSNullAccess);

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/* func.name */assignStruct)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/* func.name */clear)
{
	TJS_GET_NATIVE_INSTANCE(/* var. name */ni, /* var. type */tTJSArrayNI);

	((tTJSArrayObject*)objthis)->Clear(ni);

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/* func.name */clear)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/* func.name */erase)
{
	// remove specified item number from the array

	TJS_GET_NATIVE_INSTANCE(/* var. name */ni, /* var. type */tTJSArrayNI);

	if(numparams < 1) return TJS_E_BADPARAMCOUNT;

	tjs_int num = *param[0];

	((tTJSArrayObject*)objthis)->Erase(ni, num);

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/* func.name */erase)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/* func.name */remove)
{
	// remove specified item from the array wchich appears first or all

	TJS_GET_NATIVE_INSTANCE(/* var. name */ni, /* var. type */tTJSArrayNI);

	if(numparams < 1) return TJS_E_BADPARAMCOUNT;

	bool eraseall;
	if(numparams >= 2)
		eraseall = param[1]->operator bool();
	else
		eraseall = true;

	tTJSVariant & val = *param[0];

	tjs_int count = ((tTJSArrayObject*)objthis)->Remove(ni, val, eraseall);

	if(result) *result = count;

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/* func.name */remove)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/* func.name */insert)
{
	// insert item at specified position

	TJS_GET_NATIVE_INSTANCE(/* var. name */ni, /* var. type */tTJSArrayNI);

	if(numparams < 2) return TJS_E_BADPARAMCOUNT;

	tjs_int num = *param[0];

	((tTJSArrayObject*)objthis)->Insert(ni, *param[1], num);

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/* func.name */insert)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/* func.name */add)
{
	// add item at last

	TJS_GET_NATIVE_INSTANCE(/* var. name */ni, /* var. type */tTJSArrayNI);

	if(numparams < 1) return TJS_E_BADPARAMCOUNT;

	((tTJSArrayObject*)objthis)->Add(ni, *param[0]);

	if(result) *result = (signed)ni->Items.size() -1;

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/* func.name */add)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/* func.name */push)
{
	// add item(s) at last

	TJS_GET_NATIVE_INSTANCE(/* var. name */ni, /* var. type */tTJSArrayNI);

	((tTJSArrayObject*)objthis)->Insert(ni, param, numparams, (tjs_int)ni->Items.size());

	if(result) *result = (tTVInteger)(ni->Items.size());

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/* func.name */push)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/* func.name */pop)
{
	// pop item from last

	TJS_GET_NATIVE_INSTANCE(/* var. name */ni, /* var. type */tTJSArrayNI);

	if(ni->Items.empty())
	{
		if(result) result->Clear();
	}
	else
	{
		if(result) *result = ni->Items[ni->Items.size() - 1];
		((tTJSArrayObject*)objthis)->Erase(ni, (tjs_int)(ni->Items.size() - 1));
	}

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/* func.name */pop)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/* func.name */shift)
{
	// shift item at head

	TJS_GET_NATIVE_INSTANCE(/* var. name */ni, /* var. type */tTJSArrayNI);

	if(ni->Items.empty())
	{
		if(result) result->Clear();
	}
	else
	{
		if(result) *result = ni->Items[0];
		((tTJSArrayObject*)objthis)->Erase(ni, 0);
	}

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/* func.name */shift)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/* func.name */unshift)
{
	// add item(s) at head

	TJS_GET_NATIVE_INSTANCE(/* var. name */ni, /* var. type */tTJSArrayNI);

	((tTJSArrayObject*)objthis)->Insert(ni, param, numparams, 0);

	if(result) *result = (tTVInteger)(ni->Items.size());

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/* func.name */unshift)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/* func.name */find)
{
	// find item in the array,
	// return an index which points the item that appears first.

	TJS_GET_NATIVE_INSTANCE(/* var. name */ni, /* var. type */tTJSArrayNI);

	if(numparams < 1) return TJS_E_BADPARAMCOUNT;
	if(result)
	{
		tTJSVariant & val = *param[0];
		tjs_int start = 0;
		if(numparams >= 2) start = *param[1];
		if(start < 0) start += (tjs_int)ni->Items.size();
		if(start < 0) start = 0;
		if(start >= (tjs_int)ni->Items.size()) { *result = -1; return TJS_S_OK; }

		tTJSArrayNI::tArrayItemIterator i;
		for(i = ni->Items.begin() + start; i != ni->Items.end(); i++)
		{
			if(val.DiscernCompare(*i)) break;
		}
		if(i == ni->Items.end())
			*result = -1;
		else
			result->operator=((tjs_int)(i - ni->Items.begin()));
	}

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/* func.name */find)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/* func.name */pack)
{
	TJS_GET_NATIVE_INSTANCE(/* var. name */ni, /* var. type */tTJSArrayNI);

	if(numparams < 1) return TJS_E_BADPARAMCOUNT;
	if(result) return TJSOctetPack( param, numparams, ni->Items, result );

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/* func.name */pack)
//----------------------------------------------------------------------


//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(count)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/* var. name */ni, /* var. type */tTJSArrayNI);
		if(result) *result = (tTVInteger)(ni->Items.size());
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/* var. name */ni, /* var. type */tTJSArrayNI);
		ni->Items.resize((tjs_uint)(tTVInteger)*param);
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(count)
//----------------------------------------------------------------------
// same as count
TJS_BEGIN_NATIVE_PROP_DECL(length)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/* var. name */ni, /* var. type */tTJSArrayNI);
		if(result) *result = (tTVInteger)(ni->Items.size());
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/* var. name */ni, /* var. type */tTJSArrayNI);
		ni->Items.resize((tjs_uint)(tTVInteger)*param);
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(length)
//----------------------------------------------------------------------

	ClassID_Array = TJS_NCM_CLASSID;
	TJS_END_NATIVE_MEMBERS
}
//---------------------------------------------------------------------------
tTJSArrayClass::~tTJSArrayClass()
{
}
//---------------------------------------------------------------------------
tTJSNativeInstance *tTJSArrayClass::CreateNativeInstance()
{
	return new tTJSArrayNI(); // return a native object instance.
}
//---------------------------------------------------------------------------
iTJSDispatch2 *tTJSArrayClass::CreateBaseTJSObject()
{
	return new tTJSArrayObject();
}
//---------------------------------------------------------------------------




//---------------------------------------------------------------------------
// tTJSArrayNI
//---------------------------------------------------------------------------
tTJSArrayNI::tTJSArrayNI()
{
	// constructor
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD tTJSArrayNI::Construct(tjs_int numparams, tTJSVariant **params,
	iTJSDispatch2 * tjsobj)
{
	// called by TJS constructor
	if(numparams!=0) return TJS_E_BADPARAMCOUNT;
	return TJS_S_OK;
}
//---------------------------------------------------------------------------
void tTJSArrayNI::Assign(iTJSDispatch2 * dsp)
{
	// copy members from "dsp" to "Owner"

	// determin dsp's object type
	tTJSArrayNI *arrayni = NULL;
	if(TJS_SUCCEEDED(dsp->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
		ClassID_Array, (iTJSNativeInstance**)&arrayni)) )
	{
		// copy from array
		Items.clear();

		Items.assign(arrayni->Items.begin(), arrayni->Items.end());
	}
	else
	{
		// convert from dictionary or others
		Items.clear();
		tDictionaryEnumCallback callback;
		callback.Items = &Items;
        tTJSVariantClosure clo(&callback, NULL);
		dsp->EnumMembers(TJS_IGNOREPROP, &clo, dsp);

	}
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD tTJSArrayNI::tDictionaryEnumCallback::FuncCall(
	tjs_uint32 flag, const tjs_char * membername, tjs_uint32 *hint,
	tTJSVariant *result, tjs_int numparams, tTJSVariant **param,
	iTJSDispatch2 *objthis)
{
	// called from tTJSCustomObject::EnumMembers

	if(numparams < 3) return TJS_E_BADPARAMCOUNT;

	// hidden members are not processed
	tjs_uint32 flags = (tjs_int)*param[1];
	if(flags & TJS_HIDDENMEMBER)
	{
		if(result) *result = (tjs_int)1;
		return TJS_S_OK;
	}

	// push items

	Items->push_back(*param[0]);
	Items->push_back(*param[2]);

	if(result) *result = (tjs_int)1;
	return TJS_S_OK;
}
//---------------------------------------------------------------------------
void tTJSArrayNI::SaveStructuredData(std::vector<iTJSDispatch2 *> &stack,
                                     iTJSTextWriteStream &stream, const ttstr &indentstr)
{
#ifdef TJS_TEXT_OUT_CRLF
	stream.Write(TJS_W("(const) [\r\n"));
#else
	stream>Write(TJS_W("(const) [\n"));
#endif

	ttstr indentstr2 = indentstr + TJS_W(" ");

	tArrayItemIterator i;
	tjs_uint c = 0;
	for(i = Items.begin(); i != Items.end(); i++)
	{
		stream.Write(indentstr2);
		tTJSVariantType type = i->Type();
		if(type == tvtObject)
		{
			// object
			tTJSVariantClosure clo = i->AsObjectClosureNoAddRef();
			SaveStructuredDataForObject(clo.SelectObjectNoAddRef(),
				stack, stream, indentstr2);
		}
		else
		{
			stream.Write(TJSVariantToExpressionString(*i));
		}
#ifdef TJS_TEXT_OUT_CRLF
		if(c != Items.size() -1) // unless last
			stream.Write(TJS_W(",\r\n"));
		else
			stream.Write(TJS_W("\r\n"));
#else
		if(c != Items.size() -1) // unless last
			stream.Write(TJS_W(",\n"));
		else
			stream.Write(TJS_W("\n"));
#endif

		c++;
	}

	stream.Write(indentstr);
	stream.Write(TJS_W("]"));
}
//---------------------------------------------------------------------------
void tTJSArrayNI::SaveStructuredDataForObject(iTJSDispatch2 *dsp,
		std::vector<iTJSDispatch2 *> &stack, iTJSTextWriteStream &stream,
		const ttstr &indentstr)
{
	// check object recursion
	std::vector<iTJSDispatch2 *>::iterator i;
	for(i = stack.begin(); i!=stack.end(); i++)
	{
		if(*i == dsp)
		{
			// object recursion detected
                        stream.Write(TJS_W("null /* object recursion detected */"));
			return;
		}
	}

	// determin dsp's object type
	tTJSDictionaryNI *dicni = NULL;
	tTJSArrayNI *arrayni = NULL;
	if(dsp && TJS_SUCCEEDED(dsp->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
		TJSGetDictionaryClassID(), (iTJSNativeInstance**)&dicni)) )
	{
		// dictionary
		stack.push_back(dsp);
		dicni->SaveStructuredData(stack, stream, indentstr);
		stack.pop_back();
	}
	else if(dsp && TJS_SUCCEEDED(dsp->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
		ClassID_Array, (iTJSNativeInstance**)&arrayni)) )
	{
		// array
		stack.push_back(dsp);
		arrayni->SaveStructuredData(stack, stream, indentstr);
		stack.pop_back();
	}
	else if(dsp != NULL)
	{
		// other objects
		stream.Write(TJS_W("null /* (object) \"")); // stored as a null
		tTJSVariant val(dsp,dsp);
		stream.Write(ttstr(val).EscapeC());
		stream.Write(TJS_W("\" */"));
	}
	else
	{
		// null
		stream.Write(TJS_W("null"));
	}
}
//---------------------------------------------------------------------------
void tTJSArrayNI::SaveStructuredBinary(std::vector<iTJSDispatch2 *> &stack, tTJSBinaryStream &stream )
{
	tjs_uint count = (tjs_uint)Items.size();
	tTJSBinarySerializer::PutStartArray( &stream, count );

	tArrayItemIterator i;
	for( i = Items.begin(); i != Items.end(); i++ ) {
		tTJSVariantType type = i->Type();
		if( type == tvtObject ){
			// object
			tTJSVariantClosure clo = i->AsObjectClosureNoAddRef();
			SaveStructuredBinaryForObject( clo.SelectObjectNoAddRef(), stack, stream );
		} else {
			tTJSBinarySerializer::PutVariant( &stream, *i );
		}
	}
}
//---------------------------------------------------------------------------
void tTJSArrayNI::SaveStructuredBinaryForObject(iTJSDispatch2 *dsp,
		std::vector<iTJSDispatch2 *> &stack, tTJSBinaryStream &stream )
{
	// check object recursion
	std::vector<iTJSDispatch2 *>::iterator i;
	for( i = stack.begin(); i != stack.end(); i++ ) {
		if( *i == dsp ) {
			// object recursion detected
			tTJSBinarySerializer::PutNull( &stream );
			return;
		}
	}

	// determin dsp's object type
	tTJSDictionaryNI *dicni = NULL;
	tTJSArrayNI *arrayni = NULL;
	if(dsp && TJS_SUCCEEDED(dsp->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
		TJSGetDictionaryClassID(), (iTJSNativeInstance**)&dicni)) ) {
		// dictionary
		stack.push_back(dsp);
		dicni->SaveStructuredBinary( stack, stream );
		stack.pop_back();
	} else if(dsp && TJS_SUCCEEDED(dsp->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
		ClassID_Array, (iTJSNativeInstance**)&arrayni)) ) {
		// array
		stack.push_back(dsp);
		arrayni->SaveStructuredBinary( stack, stream );
		stack.pop_back();
	} else if(dsp != NULL) {
		// other objects
		tTJSBinarySerializer::PutNull( &stream );
	} else {
		// null
		tTJSBinarySerializer::PutNull( &stream );
	}
}
//---------------------------------------------------------------------------
void tTJSArrayNI::AssignStructure(iTJSDispatch2 * dsp,
	std::vector<iTJSDispatch2 *> &stack)
{
	// assign structured data from dsp
	tTJSArrayNI *arrayni = NULL;
	if(TJS_SUCCEEDED(dsp->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
		ClassID_Array, (iTJSNativeInstance**)&arrayni)) )
	{
		// copy from array
		stack.push_back(dsp);
		try
		{
			Items.clear();

			tArrayItemIterator i;
			for(i = arrayni->Items.begin(); i != arrayni->Items.end(); i++)
			{
				tTJSVariantType type = i->Type();
				if(type == tvtObject)
				{
					// object
					iTJSDispatch2 *dsp = i->AsObjectNoAddRef();
					// determin dsp's object type

					tTJSDictionaryNI *dicni = NULL;
					tTJSArrayNI *arrayni = NULL;

					if(dsp && TJS_SUCCEEDED(dsp->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
						TJSGetDictionaryClassID(), (iTJSNativeInstance**)&dicni)) )
					{
						// dictionary
						bool objrec = false;
						std::vector<iTJSDispatch2 *>::iterator i;
						for(i = stack.begin(); i!=stack.end(); i++)
						{
							if(*i == dsp)
							{
								// object recursion detected
								objrec = true;
								break;
							}
						}
						if(objrec)
						{
							Items.push_back(tTJSVariant((iTJSDispatch2*)NULL)); // becomes null
						}
						else
						{
							iTJSDispatch2 * newobj = TJSCreateDictionaryObject();
							Items.push_back(tTJSVariant(newobj, newobj));
							newobj->Release();
							tTJSDictionaryNI * newni = NULL;
							if(TJS_SUCCEEDED(newobj->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
								TJSGetDictionaryClassID(), (iTJSNativeInstance**)&newni)) )
							{
								newni->AssignStructure(dsp, stack);
							}
						}
					}
					else if(dsp && TJS_SUCCEEDED(dsp->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
						TJSGetArrayClassID(), (iTJSNativeInstance**)&arrayni)) )
					{
						// array
						bool objrec = false;
						std::vector<iTJSDispatch2 *>::iterator i;
						for(i = stack.begin(); i!=stack.end(); i++)
						{
							if(*i == dsp)
							{
								// object recursion detected
								objrec = true;
								break;
							}
						}
						if(objrec)
						{
							Items.push_back(tTJSVariant((iTJSDispatch2*)NULL)); // becomes null
						}
						else
						{
							iTJSDispatch2 * newobj = TJSCreateArrayObject();
							Items.push_back(tTJSVariant(newobj, newobj));
							newobj->Release();
							tTJSArrayNI * newni = NULL;
							if(TJS_SUCCEEDED(newobj->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
								TJSGetArrayClassID(), (iTJSNativeInstance**)&newni)) )
							{
								newni->AssignStructure(dsp, stack);
							}
						}
					}
					else
					{
						// other object types
						Items.push_back(*i);
					}
				}
				else
				{
					// others
					Items.push_back(*i);
				}
			}
		}
		catch(...)
		{
			stack.pop_back();
			throw;
		}
		stack.pop_back();
	}
	else
	{
		TJS_eTJSError(TJSSpecifyDicOrArray);
	}
}
//---------------------------------------------------------------------------




//---------------------------------------------------------------------------
// tTJSArrayObject
//---------------------------------------------------------------------------
tTJSArrayObject::tTJSArrayObject() : tTJSCustomObject(TJS_ARRAY_BASE_HASH_BITS)
{
	CallFinalize = false;
}
//---------------------------------------------------------------------------
tTJSArrayObject::~tTJSArrayObject()
{
}
//---------------------------------------------------------------------------

#define ARRAY_GET_NI \
	tTJSArrayNI *ni; \
	if(TJS_FAILED(objthis->NativeInstanceSupport(TJS_NIS_GETINSTANCE, \
		ClassID_Array, (iTJSNativeInstance**)&ni))) \
			return TJS_E_NATIVECLASSCRASH;
static tTJSVariant VoidValue;
#define ARRAY_GET_VAL \
	tjs_int membercount = (tjs_int)(ni->Items.size()); \
	if(num < 0) num = membercount + num; \
	if((flag & TJS_MEMBERMUSTEXIST) && (num < 0 || membercount <= num)) \
		return TJS_E_MEMBERNOTFOUND; \
	tTJSVariant val ( (membercount<=num || num < 0) ?VoidValue:ni->Items[num]);
		// Do not take reference of the element (because the element *might*
		// disappear in function call, property handler etc.) So here must be
		// an object copy, not reference.
//---------------------------------------------------------------------------
void tTJSArrayObject::Finalize()
{
	tTJSArrayNI *ni;
	if(TJS_FAILED(NativeInstanceSupport(TJS_NIS_GETINSTANCE,
		ClassID_Array, (iTJSNativeInstance**)&ni)))
			TJS_eTJSError(TJSNativeClassCrash);
	Clear(ni);

	inherited::Finalize();
}
//---------------------------------------------------------------------------
void tTJSArrayObject::Clear(tTJSArrayNI * ni)
{
	// clear members

	std::vector<iTJSDispatch2*> vector;
	try
	{
		tjs_uint i;
		for(i=0; i<ni->Items.size(); i++)
		{
			if(ni->Items[i].Type() == tvtObject)
			{
				CheckObjectClosureRemove(ni->Items[i]);
				tTJSVariantClosure clo =
					ni->Items[i].AsObjectClosureNoAddRef();
				clo.AddRef();
				if(clo.Object) vector.push_back(clo.Object);
				if(clo.ObjThis) vector.push_back(clo.ObjThis);
				ni->Items[i].Clear();
			}
		}
		ni->Items.clear();
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
void tTJSArrayObject::Add(tTJSArrayNI * ni, const tTJSVariant &val)
{
	ni->Items.push_back(val);
	CheckObjectClosureAdd(ni->Items[ni->Items.size() -1]);
}
//---------------------------------------------------------------------------
tjs_int tTJSArrayObject::Remove(tTJSArrayNI * ni, const tTJSVariant &ref, bool removeall)
{
	tjs_int count = 0;
	std::vector<tjs_int> todelete;
	tjs_int num = 0;
	for(tTJSArrayNI::tArrayItemIterator i = ni->Items.begin();
		i != ni->Items.end(); i++)
	{
		if(ref.DiscernCompare(*i))
		{
			count ++;
			todelete.push_back(num);
			if(!removeall) break;
		}
		num++;
	}

	std::vector<iTJSDispatch2*> vector;
	try
	{
		// list objects up
		for(std::vector<tjs_int>::iterator i = todelete.begin();
			i != todelete.end(); i++)
		{
			if(ni->Items[*i].Type() == tvtObject)
			{
				CheckObjectClosureRemove(ni->Items[*i]);
				tTJSVariantClosure clo =
					ni->Items[*i].AsObjectClosureNoAddRef();
				clo.AddRef();
				if(clo.Object) vector.push_back(clo.Object);
				if(clo.ObjThis) vector.push_back(clo.ObjThis);
				ni->Items[*i].Clear();
			}
		}

		// remove items found
		for(tjs_int i = (tjs_int)todelete.size() -1; i>=0; i--)
		{
			ni->Items.erase(ni->Items.begin() + todelete[i]);
		}
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

	return count;
}
//---------------------------------------------------------------------------
void tTJSArrayObject::Erase(tTJSArrayNI * ni, tjs_int num)
{
	if(num < 0) num += (tjs_int)ni->Items.size();
	if(num < 0) TJS_eTJSError(TJSRangeError);
	if((unsigned)num >= ni->Items.size()) TJS_eTJSError(TJSRangeError);

	CheckObjectClosureRemove(ni->Items[num]);
	ni->Items.erase(ni->Items.begin() + num);
}
//---------------------------------------------------------------------------
void tTJSArrayObject::Insert(tTJSArrayNI *ni, const tTJSVariant &val, tjs_int num)
{
	if(num < 0) num += (tjs_int)ni->Items.size();
	if(num < 0) TJS_eTJSError(TJSRangeError);
	tjs_int count = (tjs_int)ni->Items.size();
	if(num > count) TJS_eTJSError(TJSRangeError);

	ni->Items.insert(ni->Items.begin() + num, val);
	CheckObjectClosureAdd(val);
}
//---------------------------------------------------------------------------
void tTJSArrayObject::Insert(tTJSArrayNI *ni, tTJSVariant *const *val, tjs_int numvals, tjs_int num)
{
	if(num < 0) num += (tjs_int)ni->Items.size();
	if(num < 0) TJS_eTJSError(TJSRangeError);
	tjs_int count = (tjs_int)ni->Items.size();
	if(num > count) TJS_eTJSError(TJSRangeError);

	// first initialize specified position as void, then
	// overwrite items.
	ni->Items.insert(ni->Items.begin() + num, numvals, tTJSVariant());
	for(tjs_int i = 0; i < numvals; i++)
	{
		ni->Items[num + i] = *val[i];
		CheckObjectClosureAdd(*val[i]);
	}
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD
	tTJSArrayObject::FuncCall(tjs_uint32 flag, const tjs_char * membername,
	tjs_uint32 *hint,
		tTJSVariant *result, tjs_int numparams, tTJSVariant **param,
		iTJSDispatch2 *objthis)
{
	tjs_int idx;
	if(membername && IsNumber(membername, idx))
		return FuncCallByNum(flag, idx, result, numparams, param, objthis);

	return inherited::FuncCall(flag, membername, hint, result, numparams, param, objthis);
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD
	tTJSArrayObject::FuncCallByNum(tjs_uint32 flag, tjs_int num, tTJSVariant *result,
		tjs_int numparams, tTJSVariant **param, iTJSDispatch2 *objthis)
{
	if(!GetValidity())
		return TJS_E_INVALIDOBJECT;

	ARRAY_GET_NI;
	ARRAY_GET_VAL;
	return TJSDefaultFuncCall(flag, val, result, numparams, param, objthis);
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD
	tTJSArrayObject::PropGet(tjs_uint32 flag, const tjs_char * membername,
	tjs_uint32*hint, tTJSVariant *result, iTJSDispatch2 *objthis)
{
	tjs_int idx;
	if(membername && IsNumber(membername, idx))
		return PropGetByNum(flag, idx, result, objthis);
	return inherited::PropGet(flag, membername, hint, result, objthis);
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD
	tTJSArrayObject::PropGetByNum(tjs_uint32 flag, tjs_int num,
		tTJSVariant *result, iTJSDispatch2 *objthis)
{
	if(!GetValidity())
		return TJS_E_INVALIDOBJECT;

	ARRAY_GET_NI;
	ARRAY_GET_VAL;
	return TJSDefaultPropGet(flag, val, result, objthis);
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD
	tTJSArrayObject::PropSet(tjs_uint32 flag, const tjs_char *membername,
		tjs_uint32 *hint,
		const tTJSVariant *param, iTJSDispatch2 *objthis)
{
	tjs_int idx;
	if(membername && IsNumber(membername, idx))
		return PropSetByNum(flag, idx, param, objthis);
	return inherited::PropSet(flag, membername, hint, param, objthis);
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD
	tTJSArrayObject::PropSetByNum(tjs_uint32 flag, tjs_int num,
		const tTJSVariant *param, iTJSDispatch2 *objthis)
{
	if(!GetValidity())
		return TJS_E_INVALIDOBJECT;

	ARRAY_GET_NI;
	if(num < 0) num += (tjs_int)ni->Items.size();
	if(num >= (tjs_int)ni->Items.size())
	{
		if(flag & TJS_MEMBERMUSTEXIST) return TJS_E_MEMBERNOTFOUND;
		ni->Items.resize(num+1);
	}
	if(num < 0) return TJS_E_MEMBERNOTFOUND;
	tTJSVariant &val = ni->Items[num];
	tjs_error hr;
	CheckObjectClosureRemove(val);
	try
	{
		hr = TJSDefaultPropSet(flag, val, param, objthis);
	}
	catch(...)
	{
		CheckObjectClosureAdd(val);
		throw;
	}
	CheckObjectClosureAdd(val);
	return hr;
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD
	tTJSArrayObject::PropSetByVS(tjs_uint32 flag, tTJSVariantString *membername,
		const tTJSVariant *param, iTJSDispatch2 *objthis)
{
	tjs_int idx;
	if(membername && IsNumber((const tjs_char*)(*membername), idx))
		return PropSetByNum(flag, idx, param, objthis);
	return inherited::PropSetByVS(flag, membername, param, objthis);
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD
	tTJSArrayObject::DeleteMember(tjs_uint32 flag, const tjs_char *membername,
	tjs_uint32 *hint,
	iTJSDispatch2 *objthis)
{
	tjs_int idx;
	if(membername && IsNumber(membername, idx))
		return DeleteMemberByNum(flag, idx, objthis);
	return inherited::DeleteMember(flag, membername, hint, objthis);
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD
	tTJSArrayObject::DeleteMemberByNum(tjs_uint32 flag, tjs_int num,
	iTJSDispatch2 *objthis)
{
	if(!GetValidity())
		return TJS_E_INVALIDOBJECT;

	ARRAY_GET_NI;
	if(num < 0) num += (tjs_int)ni->Items.size();
	if(num < 0 || (tjs_uint)num>=ni->Items.size()) return TJS_E_MEMBERNOTFOUND;
	CheckObjectClosureRemove(ni->Items[num]);
	std::deque<tTJSVariant>::iterator i;
	ni->Items.erase(ni->Items.begin() + num);
	return TJS_S_OK;
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD
	tTJSArrayObject::Invalidate(tjs_uint32 flag, const tjs_char *membername,
	tjs_uint32 *hint,
	iTJSDispatch2 *objthis)
{
	tjs_int idx;
	if(membername && IsNumber(membername, idx))
		return InvalidateByNum(flag, idx, objthis);
	return inherited::Invalidate(flag, membername, hint, objthis);
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD
	tTJSArrayObject::InvalidateByNum(tjs_uint32 flag, tjs_int num,
		iTJSDispatch2 *objthis)
{
	if(!GetValidity())
		return TJS_E_INVALIDOBJECT;

	ARRAY_GET_NI;
	ARRAY_GET_VAL;
	return TJSDefaultInvalidate(flag, val, objthis);
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD
	tTJSArrayObject::IsValid(tjs_uint32 flag, const tjs_char *membername,
		tjs_uint32 *hint,
		iTJSDispatch2 *objthis)
{
	tjs_int idx;
	if(membername && IsNumber(membername, idx))
		return IsValidByNum(flag, idx, objthis);
	return inherited::IsValid(flag, membername, hint, objthis);
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD
	tTJSArrayObject::IsValidByNum(tjs_uint32 flag, tjs_int num,
		iTJSDispatch2 *objthis)
{
	if(!GetValidity())
		return TJS_E_INVALIDOBJECT;

	ARRAY_GET_NI;
	ARRAY_GET_VAL;
	return TJSDefaultIsValid(flag, val, objthis);
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD
	tTJSArrayObject::CreateNew(tjs_uint32 flag, const tjs_char * membername,
		tjs_uint32 *hint,
		iTJSDispatch2 **result, tjs_int numparams, tTJSVariant **param,
		iTJSDispatch2 *objthis)
{
	tjs_int idx;
	if(membername && IsNumber(membername, idx))
		return CreateNewByNum(flag, idx, result, numparams, param, objthis);
	return inherited::CreateNew(flag, membername, hint, result, numparams, param, objthis);
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD
	tTJSArrayObject::CreateNewByNum(tjs_uint32 flag, tjs_int num, iTJSDispatch2 **result,
		tjs_int numparams, tTJSVariant **param, iTJSDispatch2 *objthis)
{
	if(!GetValidity())
		return TJS_E_INVALIDOBJECT;

	ARRAY_GET_NI;
	ARRAY_GET_VAL;
	return TJSDefaultCreateNew(flag, val, result, numparams, param, objthis);
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD
	tTJSArrayObject::IsInstanceOf(tjs_uint32 flag,const  tjs_char *membername,
		tjs_uint32 *hint,
		const tjs_char *classname, iTJSDispatch2 *objthis)
{
	tjs_int idx;
	if(membername && IsNumber(membername, idx))
		return IsInstanceOfByNum(flag, idx, classname, objthis);
	return inherited::IsInstanceOf(flag, membername, hint, classname, objthis);
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD
	tTJSArrayObject::IsInstanceOfByNum(tjs_uint32 flag, tjs_int num,
		const tjs_char *classname, iTJSDispatch2 *objthis)
{
	if(!GetValidity())
		return TJS_E_INVALIDOBJECT;

	ARRAY_GET_NI;
	ARRAY_GET_VAL;
	return TJSDefaultIsInstanceOf(flag, val, classname, objthis);
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD
	tTJSArrayObject::Operation(tjs_uint32 flag, const tjs_char *membername,
		tjs_uint32 *hint,
		tTJSVariant *result, const tTJSVariant *param, iTJSDispatch2 *objthis)
{
	tjs_int idx;
	if(membername && IsNumber(membername, idx))
		return OperationByNum(flag, idx, result, param, objthis);
	return inherited::Operation(flag, membername, hint, result, param, objthis);
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD 
	tTJSArrayObject::OperationByNum(tjs_uint32 flag, tjs_int num,
		tTJSVariant *result, const tTJSVariant *param, iTJSDispatch2 *objthis)
{
	if(!GetValidity())
		return TJS_E_INVALIDOBJECT;

	ARRAY_GET_NI;
	if(num < 0) num += (tjs_int)ni->Items.size();
	if(num >= (tjs_int)ni->Items.size())
	{
		if(flag & TJS_MEMBERMUSTEXIST) return TJS_E_MEMBERNOTFOUND;
		ni->Items.resize(num+1);
	}
	if(num < 0) return TJS_E_MEMBERNOTFOUND;
	tjs_error hr;
	tTJSVariant &val = ni->Items[num];
	CheckObjectClosureRemove(val);
	try
	{
		hr=TJSDefaultOperation(flag, val, result, param, objthis);
	}
	catch(...)
	{
		CheckObjectClosureAdd(val);
		throw;
	}
	CheckObjectClosureAdd(val);
	return hr;
}
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
// TJSGetArrayClassID
//---------------------------------------------------------------------------
tjs_int32 TJSGetArrayClassID()
{
	return ClassID_Array;
}
//---------------------------------------------------------------------------




//---------------------------------------------------------------------------
// TJSCreateArrayObject
//---------------------------------------------------------------------------
iTJSDispatch2 * TJSCreateArrayObject(iTJSDispatch2 **classout)
{
	// create an Array object
	struct tHolder
	{
		iTJSDispatch2 * Obj;
		tHolder() { Obj = new tTJSArrayClass(); }
		~tHolder() { Obj->Release(); }
	} static arrayclass;

	if(classout) *classout = arrayclass.Obj, arrayclass.Obj->AddRef();

	tTJSArrayObject *arrayobj;
	(arrayclass.Obj)->CreateNew(0, NULL, NULL, 
		(iTJSDispatch2**)&arrayobj, 0, NULL, arrayclass.Obj);
	return arrayobj;
}
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// Utility functions
//---------------------------------------------------------------------------
tjs_int TJSGetArrayElementCount(iTJSDispatch2 * dsp)
{
	// returns array element count
	tTJSArrayNI *ni;
	if(TJS_FAILED(dsp->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
		ClassID_Array, (iTJSNativeInstance**)&ni)))
			TJS_eTJSError(TJSSpecifyArray);
	return (tjs_int)ni->Items.size();
}
//---------------------------------------------------------------------------
tjs_int TJSCopyArrayElementTo(iTJSDispatch2 * dsp,
	tTJSVariant *dest, tjs_uint start, tjs_int count)
{
	// copy array elements to specified variant array.
	// returns copied element count.
	tTJSArrayNI *ni;
	if(TJS_FAILED(dsp->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
		ClassID_Array, (iTJSNativeInstance**)&ni)))
			TJS_eTJSError(TJSSpecifyArray);

	if(count < 0) count = (tjs_int)ni->Items.size();

	if(start >= ni->Items.size()) return 0;

	tjs_uint limit = start + count;

	for(tjs_uint i = start; i < limit; i++)
		*(dest++) = ni->Items[i];

	return limit - start;
}
//---------------------------------------------------------------------------





} // namespace TJS



