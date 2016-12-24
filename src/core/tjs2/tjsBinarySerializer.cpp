//---------------------------------------------------------------------------
/*
	TJS2 Script Engine( Byte Code )
	Copyright (c), Takenori Imoto

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
#include "tjsCommHead.h"

#include "tjs.h"
#include "tjsBinarySerializer.h"
#include "tjsDictionary.h"
#include "tjsArray.h"

namespace TJS
{

const tjs_uint8 tTJSBinarySerializer::HEADER[tTJSBinarySerializer::HEADER_LENGTH] = {
	'K','B','A','D', '1','0','0', 0
};
bool tTJSBinarySerializer::IsBinary( const tjs_uint8 header[tTJSBinarySerializer::HEADER_LENGTH] ) {
	return memcmp( HEADER, header, tTJSBinarySerializer::HEADER_LENGTH) == 0;
}
/**
 * ƒoƒCƒAƒ“ƒg’l‚ðŠi”[‚·‚é
 */
void tTJSBinarySerializer::PutVariant( tTJSBinaryStream* stream, tTJSVariant& v )
{
	tTJSVariantType type = v.Type();
	switch( type ) {
	case tvtVoid: {
		tjs_uint8 tmp[1];
		tmp[0] = TYPE_VOID;
		stream->Write( tmp, sizeof(tmp) );
		break;
	}
	case tvtObject:
		break;
/*
		{
		iTJSDispatch2* obj = v.AsObjectNoAddRef();
		iTJSDispatch2* objthis = v.AsObjectThisNoAddRef();
		if( obj == NULL && objthis == NULL ) {
			Put( TYPE_NIL );
		} else {
			SaveStructured
		}
		break;
	}
*/
	case tvtString:
		PutString( stream, v.AsStringNoAddRef() );
		break;
	case tvtOctet:
		PutOctet( stream, v.AsOctetNoAddRef() );
		break;
	case tvtInteger:
		PutInteger( stream, v.AsInteger() );
		break;
	case tvtReal:
		PutDouble( stream, v.AsReal() );
		break;
	default:
		break;
	}
}

tTJSBinarySerializer::tTJSBinarySerializer() : DicClass(NULL), RootDictionary(NULL), RootArray(NULL) {
}
tTJSBinarySerializer::tTJSBinarySerializer( class tTJSDictionaryObject* root ) : DicClass(NULL), RootDictionary(root), RootArray(NULL) {
}
tTJSBinarySerializer::tTJSBinarySerializer( class tTJSArrayObject* root ) : DicClass(NULL), RootDictionary(NULL), RootArray(root) {
}
tTJSBinarySerializer::~tTJSBinarySerializer() {
	if( DicClass ) DicClass->Release();
	DicClass = NULL;
}

tTJSDictionaryObject* tTJSBinarySerializer::CreateDictionary( tjs_uint count ) {
	if( RootDictionary ) {
		tTJSDictionaryObject* ret = RootDictionary;
		RootDictionary = NULL;
		ret->RebuildHash( (tjs_int)count );
		ret->AddRef();
		return ret;
	}
	if( RootArray ) {
		TJSThrowFrom_tjs_error(TJS_E_INVALIDPARAM);	// Œ^‚ªˆá‚¤
	}
	if( DicClass == NULL ) {
		iTJSDispatch2* dsp = TJSCreateDictionaryObject(&DicClass);
		dsp->Release();
	}
	tTJSDictionaryObject* dic;
	tTJSVariant param[1] = { (tjs_int)count };
	tTJSVariant *pparam[1] = { param };
	DicClass->CreateNew( 0, NULL,  NULL, (iTJSDispatch2**)&dic, 1, pparam, DicClass );
	return dic;
}
tTJSArrayObject* tTJSBinarySerializer::CreateArray( tjs_uint count ) {
	if( RootArray ) {
		tTJSArrayObject* ret = RootArray;
		RootArray = NULL;
		ret->AddRef();
		return ret;
	}
	if( RootDictionary ) {
		TJSThrowFrom_tjs_error(TJS_E_INVALIDPARAM);	// Œ^‚ªˆá‚¤
	}
	tTJSArrayObject* array = (tTJSArrayObject*)TJSCreateArrayObject();
	return array;
}
void tTJSBinarySerializer::AddDictionary( tTJSDictionaryObject* dic, tTJSVariantString* name, tTJSVariant* value ) {
	if( name == NULL || value == NULL ) TJS_eTJSError( TJSReadError );
	dic->PropSetByVS( TJS_MEMBERENSURE, name, value, dic );
}
void tTJSBinarySerializer::InsertArray( tTJSArrayObject* array, tjs_uint index, tTJSVariant* value ) {
	if( value == NULL ) TJS_eTJSError( TJSReadError );
	tTJSArrayNI* ni = NULL;
	tjs_error hr = array->NativeInstanceSupport(TJS_NIS_GETINSTANCE, TJSGetArrayClassID(), (iTJSNativeInstance**)&ni );
	if( TJS_SUCCEEDED(hr) ) {
		// array->Insert( ni, *value, index );
		array->Add( ni, *value );
	}
}
tTJSVariant* tTJSBinarySerializer::ReadBasicType( const tjs_uint8* buff, const tjs_uint size, tjs_uint& index ) {
	if( index > size ) return NULL;
	tjs_uint8 type = buff[index];
	index++;
	switch( type  ) {
	case TYPE_NIL:
		return new tTJSVariant((iTJSDispatch2*)NULL);
	case TYPE_VOID:
		return new tTJSVariant();
	case TYPE_TRUE:
		return new tTJSVariant((tjs_int)1);
	case TYPE_FALSE:
		return new tTJSVariant((tjs_int)0);
	case TYPE_STRING8: {
		if( (index+sizeof(tjs_uint8)) > size ) TJS_eTJSError( TJSReadError );
		tjs_uint8 len = buff[index]; index++;
		if( (index+(len*sizeof(tjs_char))) > size ) TJS_eTJSError( TJSReadError );
		return ReadStringVarint( buff, len, index );
	}
	case TYPE_STRING16: {
		if( (index+sizeof(tjs_uint16)) > size ) TJS_eTJSError( TJSReadError );
		tjs_uint16 len = Read16( buff, index );
		if( (index+(len*sizeof(tjs_char))) > size ) TJS_eTJSError( TJSReadError );
		return ReadStringVarint( buff, len, index );
	}
	case TYPE_STRING32: {
		if( (index+sizeof(tjs_uint32)) > size ) TJS_eTJSError( TJSReadError );
		tjs_uint32 len = Read32( buff, index );
		if( (index+(len*sizeof(tjs_char))) > size ) TJS_eTJSError( TJSReadError );
		return ReadStringVarint( buff, len, index );
	}
	case TYPE_FLOAT: {
			if( (index+sizeof(float)) > size ) TJS_eTJSError( TJSReadError );
			tjs_uint32 t = Read32( buff, index );
			return new tTJSVariant(*(float*)&t);
		}
	case TYPE_DOUBLE: {
			if( (index+sizeof(double)) > size ) TJS_eTJSError( TJSReadError );
			tjs_uint64 t = Read64( buff, index );
			return new tTJSVariant(*(double*)&t);
		}
	case TYPE_UINT8: {
			if( (index+sizeof(tjs_uint8)) > size ) TJS_eTJSError( TJSReadError );
			tjs_uint8 t = buff[index]; index++;
			return new tTJSVariant( t );
		}
	case TYPE_UINT16: {
			if( (index+sizeof(tjs_uint16)) > size ) TJS_eTJSError( TJSReadError );
			tjs_uint16 t = Read16( buff, index );
			return new tTJSVariant( t );
		}
	case TYPE_UINT32: {
			if( (index+sizeof(tjs_uint32)) > size ) TJS_eTJSError( TJSReadError );
			tjs_uint32 t = Read32( buff, index );
			return new tTJSVariant( (tjs_int64)t );
		}
	case TYPE_UINT64: {
			if( (index+sizeof(tjs_uint64)) > size ) TJS_eTJSError( TJSReadError );
			tjs_uint64 t = Read64( buff, index );
			return new tTJSVariant( (tjs_int64)t );
		}
	case TYPE_INT8: {
			if( (index+sizeof(tjs_uint8)) > size ) TJS_eTJSError( TJSReadError );
			tjs_uint8 t = buff[index]; index++;
			return new tTJSVariant( (tjs_int8)t );
		}
	case TYPE_INT16: {
			if( (index+sizeof(tjs_uint16)) > size ) TJS_eTJSError( TJSReadError );
			tjs_uint16 t = Read16( buff, index );
			return new tTJSVariant( (tjs_int16)t );
		}
	case TYPE_INT32: {
			if( (index+sizeof(tjs_uint32)) > size ) TJS_eTJSError( TJSReadError );
			tjs_uint32 t = Read32( buff, index );
			return new tTJSVariant( (tjs_int32)t );
		}
	case TYPE_INT64: {
			if( (index+sizeof(tjs_uint64)) > size ) TJS_eTJSError( TJSReadError );
			tjs_uint64 t = Read64( buff, index );
			return new tTJSVariant( (tjs_int64)t );
		}
	case TYPE_RAW16: {
		if( (index+sizeof(tjs_uint16)) > size ) TJS_eTJSError( TJSReadError );
		tjs_uint16 len = Read16( buff, index );
		if( (index+len) > size ) TJS_eTJSError( TJSReadError );
		return ReadOctetVarint( buff, len, index );
	}
	case TYPE_RAW32: {
		if( (index+sizeof(tjs_uint32)) > size ) TJS_eTJSError( TJSReadError );
		tjs_uint32 len = Read32( buff, index );
		if( (index+len) > size ) TJS_eTJSError( TJSReadError );
		return ReadOctetVarint( buff, len, index );
	}
	case TYPE_ARRAY16: {
		if( (index+sizeof(tjs_uint16)) > size ) TJS_eTJSError( TJSReadError );
		tjs_uint16 count = Read16( buff, index );
		return ReadArray( buff, size, count, index );
	}
	case TYPE_ARRAY32: {
		if( (index+sizeof(tjs_uint32)) > size ) TJS_eTJSError( TJSReadError );
		tjs_uint32 count = Read32( buff, index );
		return ReadArray( buff, size, count, index );
	}
	case TYPE_MAP16: {
		if( (index+sizeof(tjs_uint16)) > size ) TJS_eTJSError( TJSReadError );
		tjs_uint16 count = Read16( buff, index );
		return ReadDictionary( buff, size, count, index );
	}
	case TYPE_MAP32: {
		if( (index+sizeof(tjs_uint32)) > size ) TJS_eTJSError( TJSReadError );
		tjs_uint32 count = Read32( buff, index );
		return ReadDictionary( buff, size, count, index );
	}
	default: {
		if( type >= TYPE_POSITIVE_FIX_NUM_MIN && type <= TYPE_POSITIVE_FIX_NUM_MAX ) {
			tjs_int value = type;
			return new tTJSVariant(value);
		} else if( type >= TYPE_NEGATIVE_FIX_NUM_MIN && type <= TYPE_NEGATIVE_FIX_NUM_MAX ) {
			tjs_int value = type;
			return new tTJSVariant(value);
		} else if( type >= TYPE_FIX_RAW_MIN && type <= TYPE_FIX_RAW_MAX ) { // octet
			tjs_int len = type - TYPE_FIX_RAW_MIN;
			if( (len*sizeof(tjs_uint8)+index) > size ) TJS_eTJSError( TJSReadError );
			return ReadOctetVarint( buff, len, index );
		} else if( type >= TYPE_FIX_STRING_MIN && type <= TYPE_FIX_STRING_MAX ) {
			tjs_int len = type - TYPE_FIX_STRING_MIN;
			if( (len*sizeof(tjs_char)+index) > size ) TJS_eTJSError( TJSReadError );
			return ReadStringVarint( buff, len, index );
		} else if( type >= TYPE_FIX_ARRAY_MIN && type <= TYPE_FIX_ARRAY_MAX ) {
			tjs_int count = type - TYPE_FIX_ARRAY_MIN;
			return ReadArray( buff, size, count, index );
		} else if( type >= TYPE_FIX_MAP_MIN && type <= TYPE_FIX_MAP_MAX ) {
			tjs_int count = type - TYPE_FIX_MAP_MIN;
			return ReadDictionary( buff, size, count, index );
		} else {
			TJS_eTJSError( TJSReadError );
			return NULL;
		}
	}
	}
}
tTJSVariant* tTJSBinarySerializer::ReadArray( const tjs_uint8* buff, const tjs_uint size, const tjs_uint count, tjs_uint& index ) {
	if( index > size ) return NULL;

	tTJSArrayObject* array = CreateArray( count );
	for( tjs_uint i = 0; i < count; i++ ) {
		tTJSVariant* value = ReadBasicType( buff, size, index );
		InsertArray( array, i, value );
		delete value;
	}
	tTJSVariant* ret = new tTJSVariant( array, array );
	array->Release();
	return ret;
}
tTJSVariant* tTJSBinarySerializer::ReadDictionary( const tjs_uint8* buff, const tjs_uint size, const tjs_uint count, tjs_uint& index ) {
	if( index > size ) return NULL;

	tTJSDictionaryObject* dic = CreateDictionary( count );
	for( tjs_uint i = 0; i < count; i++ ) {
		tjs_uint8 type = buff[index];
		index++;
		// Å‰‚É•¶Žš‚ð“Ç‚Þ
		tTJSVariantString* name = NULL;
		switch( type ) {
		case TYPE_STRING8: {
			if( (index+sizeof(tjs_uint8)) > size ) TJS_eTJSError( TJSReadError );
			tjs_uint8 len = buff[index]; index++;
			if( (index+(len*sizeof(tjs_char))) > size ) TJS_eTJSError( TJSReadError );
			name = ReadString( buff, len, index );
			break;
		}
		case TYPE_STRING16: {
			if( (index+sizeof(tjs_uint16)) > size ) TJS_eTJSError( TJSReadError );
			tjs_uint16 len = Read16( buff, index );
			if( (index+(len*sizeof(tjs_char))) > size ) TJS_eTJSError( TJSReadError );
			name = ReadString( buff, len, index );
			break;
		}
		case TYPE_STRING32: {
			if( (index+sizeof(tjs_uint32)) > size ) TJS_eTJSError( TJSReadError );
			tjs_uint32 len = Read32( buff, index );
			if( (index+(len*sizeof(tjs_char))) > size ) TJS_eTJSError( TJSReadError );
			name = ReadString( buff, len, index );
			break;
		}
		default:
			if( type >= TYPE_FIX_STRING_MIN && type <= TYPE_FIX_STRING_MAX ) {
				tjs_int len = type - TYPE_FIX_STRING_MIN;
				if( (len*sizeof(tjs_char)+index) > size ) TJS_eTJSError( TJSReadError );
				name = ReadString( buff, len, index );
			} else { // DictionaryŒ`Ž®‚Ìê‡AÅ‰‚É•¶Žš—ñ‚ª‚±‚È‚¢‚Æ‚¢‚¯‚È‚¢
				 TJS_eTJSError( TJSReadError );
			}
			break;
		}
		// ŽŸ‚É—v‘f‚ð“Ç‚Þ
		tTJSVariant* value = ReadBasicType( buff, size, index );
		AddDictionary( dic, name, value );
		delete value;
		if( name ) name->Release();
	}
	tTJSVariant* ret = new tTJSVariant( dic, dic );
	dic->Release();
	return ret;
}
tTJSVariant* tTJSBinarySerializer::Read( tTJSBinaryStream* stream )
{
	tjs_uint64 pos = stream->GetPosition();
	tjs_uint size = (tjs_uint)( stream->GetSize() - pos );
	tjs_uint8* buffstart = new tjs_uint8[size];
	if( size != stream->Read( buffstart, size ) ) {
		TJS_eTJSError( TJSReadError );
	}
	tjs_uint index = 0;
	tTJSVariant* ret = ReadBasicType( buffstart, size, index );
	delete[] buffstart;
	return ret;
}

} // namespace


