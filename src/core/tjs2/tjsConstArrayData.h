//---------------------------------------------------------------------------
/*
	TJS2 Script Engine( Byte Code )
	Copyright (c), Takenori Imoto

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------

#ifndef tjsConstArrayDataH
#define tjsConstArrayDataH

#include "tjsTypes.h"
#include <vector>
#include "tjsVariant.h"
#include "tjsScriptBlock.h"
#include <map>
#include <string>

namespace TJS
{
/**
 * TJS2 バイトコード書き出しで Variant 型を分離し、固有型で保持するためのクラス
 */
class tjsConstArrayData {
private:
	std::vector<tjs_int8> Byte;
	std::vector<tjs_int16> Short;
	std::vector<tjs_int32> Integer;
	std::vector<tjs_int64> Long;
	std::vector<double> Double;
	std::vector<std::basic_string<tjs_char> > String;
	std::vector<std::vector<tjs_uint8>* > ByteBuffer;

	// 保持したかどうか判定するためのハッシュ
	std::map<tjs_int8,int> ByteHash;
	std::map<tjs_int16,int> ShortHash;
	std::map<tjs_int32,int> IntegerHash;
	std::map<tjs_int64,int> LongHash;
	std::map<double,int> DoubleHash;
	std::map<std::basic_string<tjs_char>,int> StringHash;
	// オクテット型の時はハッシュを使っていない

	static const tjs_uint8 TYPE_VOID = 0;
	static const tjs_uint8 TYPE_OBJECT = 1;
	static const tjs_uint8 TYPE_INTER_OBJECT = 2;
	static const tjs_uint8 TYPE_STRING = 3;
	static const tjs_uint8 TYPE_OCTET = 4;
	static const tjs_uint8 TYPE_REAL = 5;
	static const tjs_uint8 TYPE_BYTE = 6;
	static const tjs_uint8 TYPE_SHORT = 7;
	static const tjs_uint8 TYPE_INTEGER = 8;
	static const tjs_uint8 TYPE_LONG = 9;
	static const tjs_uint8 TYPE_UNKNOWN = -1;

	/**
	 * オクテット型の値を格納する
	 */
	int PutByteBuffer( tTJSVariantOctet* val );

	/**
	 * 1バイトの値を格納する
	 */
	int PutByte( tjs_int8 b );
	
	/**
	 * 2バイトの値を格納する
	 */
	int PutShort( tjs_int16 b );
	
	/**
	 * 4バイトの値を格納する
	 */
	int PutInteger( tjs_int32 b );
	
	/**
	 * 8バイトの値を格納する
	 */
	int PutLong( tjs_int64 b );
	
	/**
	 * 浮動小数点値を格納する
	 */
	int PutDouble( double b );

	static inline void Add8ByteToVector( std::vector<tjs_uint8>* array, tjs_int64 value ) {
		array->push_back( (tjs_uint8)((value>>0)&0xff) );
		array->push_back( (tjs_uint8)((value>>8)&0xff) );
		array->push_back( (tjs_uint8)((value>>16)&0xff) );
		array->push_back( (tjs_uint8)((value>>24)&0xff) );
		array->push_back( (tjs_uint8)((value>>32)&0xff) );
		array->push_back( (tjs_uint8)((value>>40)&0xff) );
		array->push_back( (tjs_uint8)((value>>48)&0xff) );
		array->push_back( (tjs_uint8)((value>>56)&0xff) );
	}
	static inline void Add4ByteToVector( std::vector<tjs_uint8>* array, int value ) {
		array->push_back( (tjs_uint8)((value>>0)&0xff) );
		array->push_back( (tjs_uint8)((value>>8)&0xff) );
		array->push_back( (tjs_uint8)((value>>16)&0xff) );
		array->push_back( (tjs_uint8)((value>>24)&0xff) );
	}
	static inline void Add2ByteToVector( std::vector<tjs_uint8>* array, tjs_int16 value ) {
		array->push_back( (tjs_uint8)((value>>0)&0xff) );
		array->push_back( (tjs_uint8)((value>>8)&0xff) );
	}
public:
	tjsConstArrayData() {}
	~tjsConstArrayData();

	/**
	 * 文字列を格納する
	 */
	int PutString( const tjs_char* val );

	/**
	 * バイアント型が格納している型を得る
	 */
	int GetType( tTJSVariant& v, tTJSScriptBlock* block );

	/**
	 * バイアント値を格納する
	 */
	int PutVariant( tTJSVariant& v, tTJSScriptBlock* block );

	/**
	 * 保持されている値をバイト列にして取り出す
	 */
	std::vector<tjs_uint8>* ExportBuffer();
};

} // namespace
#endif // tjsConstArrayDataH

