//---------------------------------------------------------------------------
/*
	TJS2 Script Engine( Byte Code )
	Copyright (c), Takenori Imoto

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
#include "tjsCommHead.h"

#include "tjs.h"
#include "tjsConstArrayData.h"
#include <limits.h>

namespace TJS
{

tjsConstArrayData::~tjsConstArrayData() {
	for( std::vector<std::vector<tjs_uint8>* >::iterator i = ByteBuffer.begin(); i != ByteBuffer.end(); ++i ) {
		delete (*i);
	}
}

int tjsConstArrayData::PutByteBuffer( tTJSVariantOctet* val ) {
	// オクテットの時は全チェック
	tjs_uint len = 0;
	const tjs_uint8* data = NULL;
	if( val ) {
		len = val->GetLength();
		data =  val->GetData();
	}
	tjs_uint size = (tjs_uint)ByteBuffer.size();
	int index = -1;
	for( tjs_uint i = 0; i < size; i++ ) {
		tjs_uint datalen = (tjs_uint)ByteBuffer[i]->size();
		if( len == datalen ) {
			if( len == 0 ) {
				index = i;
				break;
			}
			std::vector<tjs_uint8>* buf = ByteBuffer[i];
			if( TJS_octetcmp( data, &((*buf)[0]), len ) == 0 ) {
				index = i;
				break;
			}
		}
	}
	if( index >= 0 ) return index;
	index = (int)ByteBuffer.size();
	std::vector<tjs_uint8>* buf = new std::vector<tjs_uint8>();
	buf->reserve( len );
	for( tjs_uint i = 0; i < len; i++ ) {
		buf->push_back( data[i] );
	}
	ByteBuffer.push_back( buf );
	return index;
}
int tjsConstArrayData::PutByte( tjs_int8 b ) {
	std::map<tjs_int8,int>::const_iterator index = ByteHash.find( b );
	if( index == ByteHash.end() ) {
		int idx = (int)Byte.size();
		Byte.push_back( b );
		ByteHash.insert( std::pair<tjs_int8,int>(b,idx) );
		return idx;
	} else {
		return index->second;
	}
}
int tjsConstArrayData::PutShort( tjs_int16 b ) {
	std::map<tjs_int16,int>::const_iterator index = ShortHash.find( b );
	if( index == ShortHash.end() ) {
		int idx = (int)Short.size();
		Short.push_back( b );
		ShortHash.insert( std::pair<tjs_int16,int>(b,idx) );
		return idx;
	} else {
		return index->second;
	}
}
int tjsConstArrayData::PutInteger( tjs_int32 b ) {
	std::map<tjs_int32,int>::const_iterator index = IntegerHash.find( b );
	if( index == IntegerHash.end() ) {
		int idx = (int)Integer.size();
		Integer.push_back( b );
		IntegerHash.insert( std::pair<tjs_int32,int>(b,idx) );
		return idx;
	} else {
		return index->second;
	}
}
int tjsConstArrayData::PutLong( tjs_int64 b ) {
	std::map<tjs_int64,int>::const_iterator index = LongHash.find( b );
	if( index == LongHash.end() ) {
		int idx = (int)Long.size();
		Long.push_back( b );
		LongHash.insert( std::pair<tjs_int64,int>(b,idx) );
		return idx;
	} else {
		return index->second;
	}
}
int tjsConstArrayData::PutDouble( double b ) {
	std::map<double,int>::const_iterator index = DoubleHash.find( b );
	if( index == DoubleHash.end() ) {
		int idx = (int)Double.size();
		Double.push_back( b );
		DoubleHash.insert( std::pair<double,int>(b,idx) );
		return idx;
	} else {
		return index->second;
	}
}


int tjsConstArrayData::PutString( const tjs_char* val ) {
	std::basic_string<tjs_char> b;
	if( val ) b = std::basic_string<tjs_char>(val);
	std::map<std::basic_string<tjs_char>,int>::const_iterator index = StringHash.find( b );
	if( index == StringHash.end() ) {
		int idx = (int)String.size();
		String.push_back( b );
		StringHash.insert( std::pair<std::basic_string<tjs_char>,int>(b,idx) );
		return idx;
	} else {
		return index->second;
	}
}
int tjsConstArrayData::GetType( tTJSVariant& v, tTJSScriptBlock* block ) {
	tTJSVariantType type = v.Type();
	switch( type ) {
	case tvtVoid:
		return TYPE_VOID;
	case tvtObject: {
		iTJSDispatch2* obj = v.AsObjectNoAddRef();
		int index = block->GetCodeIndex( (const tTJSInterCodeContext*)obj );
		if( index >= 0 ) {
			return TYPE_INTER_OBJECT;
		} else {
			return TYPE_OBJECT;
		}
	}
	case tvtString:
		return TYPE_STRING;
	case tvtOctet:
		return TYPE_OCTET;
	case tvtInteger: {
		tjs_int64 val = v.AsInteger();
		if( val >= SCHAR_MIN && val <= SCHAR_MAX ) {
			return TYPE_BYTE;
		} else if( val >= SHRT_MIN && val <= SHRT_MAX ) {
			return TYPE_SHORT;
		} else if( val >= LONG_MIN && val <= LONG_MAX ) {
			return TYPE_INTEGER;
		} else {
			return TYPE_LONG;
		}
	}
	case tvtReal:
		return TYPE_REAL;
	default:
		return TYPE_UNKNOWN;
	}
}
int tjsConstArrayData::PutVariant( tTJSVariant& v, tTJSScriptBlock* block ) {
	int type = GetType( v, block );
	switch( type ) {
	case TYPE_VOID:
		return 0; // 常に0
	case TYPE_OBJECT: {
		iTJSDispatch2* obj = v.AsObjectNoAddRef();
		iTJSDispatch2* objthis = v.AsObjectThisNoAddRef();
		if( obj == NULL && objthis == NULL ) {
			return 0; // null の VariantClosure は受け入れる
		} else {
			return -1; // その他は入れない。
		}
	}
	case TYPE_INTER_OBJECT: {
		iTJSDispatch2* obj = v.AsObjectNoAddRef();
		return block->GetCodeIndex( (const tTJSInterCodeContext*)obj );
	}
	case TYPE_STRING:
		return PutString( v.GetString() );
	case TYPE_OCTET:
		return PutByteBuffer( v.AsOctet() );
	case TYPE_REAL:
		return PutDouble( v.AsReal() );
	case TYPE_BYTE:
		return PutByte( static_cast<tjs_int8>(v.AsInteger()) );
	case TYPE_SHORT:
		return PutShort( static_cast<tjs_int16>(v.AsInteger()) );
	case TYPE_INTEGER:
		return PutInteger( static_cast<tjs_int32>(v.AsInteger()) );
	case TYPE_LONG:
		return PutLong( v.AsInteger() );
	case TYPE_UNKNOWN:
		return -1;
	}
	return -1;
}
std::vector<tjs_uint8>* tjsConstArrayData::ExportBuffer() {
	int size = 0;
	int stralllen = 0;
	// string
	int count = (int)String.size();
	for( int i = 0; i < count; i++ ) {
		int len = (int)String[i].length();
		len = ((len + 1) / 2) * 2;
		stralllen += len * 2;
	}
	stralllen = ((stralllen+3) / 4) * 4; // アライメント
	size += stralllen + count*4 + 4;

	// byte buffer
	int bytealllen = 0;
	count = (int)ByteBuffer.size();
	for( int i = 0; i < count; i++ ) {
		int len = (int)ByteBuffer[i]->size();
		len = ((len+3)/4)*4;
		bytealllen += len;
	}
	bytealllen = ((bytealllen+3) / 4) * 4; // アライメント
	size += bytealllen + count*4 + 4;

	// byte
	count = (int)Byte.size();
	count = ((count+3) / 4) * 4; // アライメント
	size += count + 4;

	// short
	count = (int)(Short.size() * 2);
	count = ((count+3) / 4) * 4; // アライメント
	size += count + 4;

	// int
	size += (int)(Integer.size()*4 + 4);

	// long
	size += (int)(Long.size()*8 + 4);

	// double
	size += (int)(Double.size()*8 + 4);

	std::vector<tjs_uint8>* buf = new std::vector<tjs_uint8>();
	buf->reserve( size );

	// byte write
	count = (int)Byte.size();
	Add4ByteToVector( buf, count );
	for( int i = 0; i < count; i++ ) {
		buf->push_back( Byte[i] );
	}
	count = (((count+3) / 4) * 4) - count; // アライメント差分
	for( int i = 0; i < count; i++ ) {
		buf->push_back( 0 );
	}

	// short write
	count = (int)Short.size();
	Add4ByteToVector( buf, count );
	for( int i = 0; i < count; i++ ) {
		Add2ByteToVector( buf, Short[i] );
	}
	count *= 2;
	count = (((count+3) / 4) * 4) - count; // アライメント差分
	for( int i = 0; i < count; i++ ) {
		buf->push_back( 0 );
	}

	// int write
	count = (int)Integer.size();
	Add4ByteToVector( buf, count );
	for( int i = 0; i < count; i++ ) {
		Add4ByteToVector( buf, Integer[i] );
	}

	// long write
	count = (int)Long.size();
	Add4ByteToVector( buf, count );
	for( int i = 0; i < count; i++ ) {
		Add8ByteToVector( buf, Long[i] );
	}

	// double write
	count = (int)Double.size();
	Add4ByteToVector( buf, count );
	for( int i = 0; i < count; i++ ) {
		double dval = Double[i];
		Add8ByteToVector( buf, *(tjs_int64*)&dval );
	}

	// string write
	count = (int)String.size();
	Add4ByteToVector( buf, count );
	for( int i = 0; i < count; i++ ) {
		std::basic_string<tjs_char>& str = String[i];
		int len = (int)str.length();
		Add4ByteToVector( buf, len );
		for( int s = 0; s < len; s++ ) {
			Add2ByteToVector( buf, str[s] );
		}
		if( (len % 2) == 1 ) { // アライメント差分
			Add2ByteToVector( buf, 0 );
		}
	}

	// byte buffer write
	count = (int)ByteBuffer.size();
	Add4ByteToVector( buf, count );
	for( int i = 0; i < count; i++ ) {
		std::vector<tjs_uint8>* by = ByteBuffer[i];
		int cap = (int)by->size();
		Add4ByteToVector( buf, cap );
		for( int b = 0; b < cap; b++ ) {
			buf->push_back( (*by)[b] );
		}
		cap = ((cap+3)/4)*4 - cap; // アライメント差分
		for( int b = 0; b < cap; b++ ) {
			buf->push_back( 0 );
		}
	}
	return buf;
}

} // namespace
