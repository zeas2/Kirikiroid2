//---------------------------------------------------------------------------
/*
	TJS2 Script Engine( Byte Code )
	Copyright (c), Takenori Imoto

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------

#ifndef tjsByteCodeLoaderH
#define tjsByteCodeLoaderH

#include "tjsTypes.h"
#include <vector>
#include "tjsVariant.h"
#include "tjsScriptBlock.h"

namespace TJS
{
/**
 * TJS2 バイトコードを読み込んで、ScriptBlock を返す
 *
 */
class tTJSByteCodeLoader {
public:
	static const tjs_uint32 FILE_TAG_LE = ('T') | ('J'<<8) | ('S'<<16) | ('2'<<24);
	static const tjs_uint32 VER_TAG_LE = ('1') | ('0'<<8) | ('0'<<16) | (0<<24);

private:
	template<typename T> struct array_wrap {
		T* array_;
		size_t length_;
		array_wrap() : array_(NULL), length_(0) {}
		T operator[]( int index ) const { return array_[index]; }
		size_t size() const { return length_; }
		void set( T* array, size_t length ) {
			array_ = array;
			length_ = length;
		}
	};
	/**
	 * InterCodeObject へ置換するために一時的に覚えておくクラス
	 */
	struct VariantRepalace {
		tTJSVariant* Work;
		int Index;
		VariantRepalace( tTJSVariant* w, int i ) {
			Work = w;
			Index = i;
		}
		VariantRepalace() : Work(NULL), Index(0) {}
		void set( tTJSVariant* w, int i ) {
			Work = w;
			Index = i;
		}
	};

	static const tjs_uint32 OBJ_TAG_LE = ('O') | ('B'<<8) | ('J'<<16) | ('S'<<24);
	static const tjs_uint32 DATA_TAG_LE = ('D') | ('A'<<8) | ('T'<<16) | ('A'<<24);

	static const tjs_int32 TYPE_VOID = 0;
	static const tjs_int32 TYPE_OBJECT = 1;
	static const tjs_int32 TYPE_INTER_OBJECT = 2;
	static const tjs_int32 TYPE_STRING = 3;
	static const tjs_int32 TYPE_OCTET = 4;
	static const tjs_int32 TYPE_REAL = 5;
	static const tjs_int32 TYPE_BYTE = 6;
	static const tjs_int32 TYPE_SHORT = 7;
	static const tjs_int32 TYPE_INTEGER = 8;
	static const tjs_int32 TYPE_LONG = 9;
	static const tjs_int32 TYPE_INTER_GENERATOR = 10; // temporary
	static const tjs_int32 TYPE_UNKNOWN = -1;

	array_wrap<tjs_int8> ByteArray;
	std::vector<tjs_int16> ShortArray;
	std::vector<tjs_int32> LongArray;
	std::vector<tjs_int64> LongLongArray;
	std::vector<double> DoubleArray;
	std::vector<ttstr> StringArray; // typedef tTJSString ttstr
	std::vector<tTJSVariantOctet*> OctetArray;

	const tjs_uint8* ReadBuffer;
	tjs_uint32 ReadIndex;
	tjs_uint32 ReadSize;

	static inline tjs_uint16 read2byte( const tjs_uint8* x ) {
		return  ( (tjs_uint16)(x[0]) | ((tjs_uint16)(x[1])<<8) );
	}
	static inline int read4byte( const tjs_uint8* x ) {
		return  ( ((x)[0]) | (((x)[1])<<8) | (((x)[2])<<16) | (((x)[3])<<24) );
	}
	static inline tjs_uint64 read8byte( const tjs_uint8* x ) {
		return  ( (tjs_uint64)(x[0]) | ((tjs_uint64)(x[1])<<8) | ((tjs_uint64)(x[2])<<16) | ((tjs_uint64)(x[3])<<24)
			| ((tjs_uint64)(x[4])<<32) | ((tjs_uint64)(x[5])<<40) | ((tjs_uint64)(x[6])<<48) | ((tjs_uint64)(x[7])<<56) );
	}

public:
	tTJSByteCodeLoader() : ReadBuffer(NULL), ReadIndex(0), ReadSize(0) {}
	~tTJSByteCodeLoader() {
		size_t len = OctetArray.size();
		for( size_t i = 0; i < len; i++ ) {
			tTJSVariantOctet* o = OctetArray[i];
			o->Release();
			OctetArray[i] = NULL;
		}
	}
	tTJSScriptBlock* ReadByteCode( tTJS* owner, const tjs_char* name, const tjs_uint8* buf, size_t size );

	/**
	 * @param buff : 8バイト以上のサイズ
	 */
	static bool IsTJS2ByteCode( const tjs_uint8* buff );

private:
	void ReadDataArea( const tjs_uint8* buff, int offset, size_t size );
	void ReadObjects( tTJSScriptBlock* block, const tjs_uint8* buff, int offset, int size );
	void TranslateCodeAddress( tTJSScriptBlock* block, tjs_int32* code, const tjs_int32 size );
};

} // namespace
#endif // tjsByteCodeLoaderH

