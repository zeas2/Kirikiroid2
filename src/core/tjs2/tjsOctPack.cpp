
#include "tjsCommHead.h"

#include <map>
#include <vector>
#include <string>
#include "tjsArray.h"
#include "tjsError.h"

namespace TJS
{

enum OctPackType {
	OctPack_ascii,		// a : ASCII string(ヌル文字が補完される)
	OctPack_ASCII,		// A : ASCII string(スペースが補完される)
	OctPack_bitstring,	// b : bit string(下位ビットから上位ビットの順)
	OctPack_BITSTRING,	// B : bit string(上位ビットから下位ビットの順)
	OctPack_char,		// c : 符号付き1バイト数値(-128 ～ 127)
	OctPack_CHAR,		// C : 符号無し1バイト数値(0～255)
	OctPack_double,		// d : 倍精度浮動小数点
	OctPack_float,		// f : 単精度浮動小数点
	OctPack_hex,		// h : hex string(low nybble first)
	OctPack_HEX,		// H : hex string(high nybble first)
	OctPack_int,		// i : 符号付きint数値(通常4バイト)
	OctPack_INT,		// I : 符号無しint数値(通常4バイト)
	OctPack_long,		// l : 符号付きlong数値(通常4バイト)
	OctPack_LONG,		// L : 符号無しlong数値(通常4バイト)
	OctPack_noshort,	// n : short数値(ネットワークバイトオーダ) network byte order short
	OctPack_NOLONG,		// N : long数値(ネットワークバイトオーダ) network byte order long
	OctPack_pointer,	// p : 文字列へのポインタ null terminate char
	OctPack_POINTER,	// P : 構造体(固定長文字列)へのポインタ fix length char
	OctPack_short,		// s : 符号付きshort数値(通常2バイト) sign
	OctPack_SHORT,		// S : 符号無しshort数値(通常2バイト) unsign
	OctPack_leshort,	// v : リトルエンディアンによるshort値 little endian short
	OctPack_LELONG,		// V : リトルエンディアンによるlong値 little endian long
	OctPack_uuencode,	// u : uuencodeされた文字列
	OctPack_BRE,		// w : BER圧縮された整数値
	OctPack_null,		// x : ヌル文字
	OctPack_NULL,		// X : back up a byte
	OctPack_fill,		// @ : 絶対位置までヌル文字を埋める
	OctPack_base64,		// m : Base64 encode / decode
	OctPack_EOT
};
static const tjs_char OctPackChar[OctPack_EOT] = {
	L'a',
	L'A',
	L'b',
	L'B',
	L'c',
	L'C',
	L'd',
	L'f',
	L'h',
	L'H',
	L'i',
	L'I',
	L'l',
	L'L',
	L'n',
	L'N',
	L'p',
	L'P',
	L's',
	L'S',
	L'v',
	L'V',
	L'u',
	L'w',
	L'x',
	L'X',
	L'@',
	L'm',
};
static bool OctPackMapInit = false;
static std::map<tjs_char,tjs_int> OctPackMap;
static void OctPackMapInitialize() {
	if( OctPackMapInit ) return;
	for( tjs_int i = 0; i < OctPack_EOT; i++ ) {
		OctPackMap.insert( std::map<tjs_char,tjs_int>::value_type( OctPackChar[i], i ) );
	}
	OctPackMapInit = true;
}

struct OctPackTemplate {
	OctPackType	Type;
	tjs_int		Length;
};
static const tjs_char* ParseTemplateLength( OctPackTemplate& result, const tjs_char* c ) {
	if( *c ) {
		if( *c == L'*' ) {
			c++;
			result.Length = -1;	// tail list
		} else if( *c >= L'0' && *c <= L'9' ) {
			tjs_int num = 0;
			while( *c && ( *c >= L'0' && *c <= L'9' ) ) {
				num *= 10;
				num += *c - L'0';
				c++;
			}
			result.Length = num;
		} else {
			result.Length = 1;
		}
	} else {
		result.Length = 1;
	}
	return c;
}

static void ParsePackTemplate( std::vector<OctPackTemplate>& result, const tjs_char* templ ) {
	OctPackMapInitialize();

	const tjs_char* c = templ;
	while( *c ) {
		std::map<tjs_char,tjs_int>::iterator f = OctPackMap.find( *c );
		if( f == OctPackMap.end() ) {
			TJS_eTJSError( TJSUnknownPackUnpackTemplateCharcter );
		} else {
			c++;
			OctPackTemplate t;
			t.Type = static_cast<OctPackType>(f->second);
			c = ParseTemplateLength( t, c );
			result.push_back( t );
		}
	}
}
static void AsciiToBin( std::vector<tjs_uint8>& bin, const ttstr& arg, tjs_nchar fillchar, tjs_int len ) {
	const tjs_char* str = arg.c_str();
	if( len < 0 ) len = arg.length();
	tjs_int i = 0;
	for( ; i < len && *str; str++, i++ ) {
		bin.push_back( (tjs_uint8)*str );
	}
	for( ; i < len; i++ ) {
		bin.push_back( fillchar );
	}
}
// mtol : true : 上位ビットから下位ビット, false : 下位ビットから上位ビット
// 指定した数値の方が大きくても、その分は無視
static void BitStringToBin( std::vector<tjs_uint8>& bin, const ttstr& arg, bool mtol, tjs_int len ) {
	const tjs_char* str = arg.c_str();
	if( len < 0 ) len = arg.length();
	tjs_uint8 val = 0;
	tjs_int pos = 0;
	if( mtol ) {
		pos = 7;
		for( tjs_int i = 0; i < len && *str; str++, i++ ) {
			if( *str == L'0' ) {
				// val |= 0;
			} else if( *str == L'1' ) {
				val |= 1 << pos;
			} else {
				TJS_eTJSError( TJSUnknownBitStringCharacter );
			}
			if( pos == 0 ) {
				bin.push_back( val );
				pos = 7;
				val = 0;
			} else {
				pos--;
			}
		}
		if( pos < 7 ) {
			bin.push_back( val );
		}
	} else {
		for( tjs_int i = 0; i < len && *str; str++, i++ ) {
			if( *str == L'0' ) {
				// val |= 0;
			} else if( *str == L'1' ) {
				val |= 1 << pos;
			} else {
				TJS_eTJSError( TJSUnknownBitStringCharacter );
			}
			if( pos == 7 ) {
				bin.push_back( val );
				pos = val = 0;
			} else {
				pos++;
			}
		}
		if( pos ) {
			bin.push_back( val );
		}
	}
}
// mtol
static void HexToBin( std::vector<tjs_uint8>& bin, const ttstr& arg, bool mtol, tjs_int len ) {
	const tjs_char* str = arg.c_str();
	if( len < 0 ) len = arg.length();
	tjs_uint8 val = 0;
	tjs_int pos = 0;
	if( mtol ) { // 上位ニブルが先
		pos = 1;
		for( tjs_int i = 0; i < len && *str; str++, i++ ) {
			if( *str >= L'0' && *str <= L'9' ) {
				val |= (*str - L'0') << (pos*4);
			} else if( *str >= L'a' && *str <= L'f' ) {
				val |= (*str - L'a' + 10) << (pos*4);
			} else if( *str >= L'A' && *str <= L'E' ) {
				val |= (*str - L'A' + 10) << (pos*4);
			} else {
				TJS_eTJSError( TJSUnknownHexStringCharacter  );
			}
			if( pos == 0 ) {
				bin.push_back( val );
				pos = 1;
				val = 0;
			} else {
				pos--;
			}
		}
		if( pos < 1 ) {
			bin.push_back( val );
		}
	} else { // 下位ニブルが先
		for( tjs_int i = 0; i < len && *str; str++, i++ ) {
			if( *str >= L'0' && *str <= L'9' ) {
				val |= (*str - L'0') << (pos*4);
			} else if( *str >= L'a' && *str <= L'f' ) {
				val |= (*str - L'a' + 10) << (pos*4);
			} else if( *str >= L'A' && *str <= L'E' ) {
				val |= (*str - L'A' + 10) << (pos*4);
			} else {
				TJS_eTJSError( TJSUnknownHexStringCharacter  );
			}
			if( pos ) {
				bin.push_back( val );
				pos = val = 0;
			} else {
				pos++;
			}
		}
		if( pos ) {
			bin.push_back( val );
		}
	}
}
// TRet : 最終的に出力する型
// TTmp : 一時的に出力する型 variant は一時的に tjs_int にしないといけないなど
template<typename TRet, typename TTmp, int NBYTE, typename TRetTmp>
static void ReadNumberLE( std::vector<tjs_uint8>& result, const std::vector<tTJSVariant>& args, tjs_int numargs, tjs_int& argindex, tjs_int len ) {
	if( len < 0 ) len = numargs - argindex;
	if( (len+argindex) > numargs ) len = numargs - argindex;
	for( tjs_int a = 0; a < len; a++ ) {
		TRet c = (TRet)(TTmp)args[argindex+a];
		TRetTmp val = *(TRetTmp*)&c;
		for( int i = 0; i < NBYTE; i++ ) {
			TRetTmp tmp = ( val >> (i*8) ) & 0xFF;
			result.push_back( (tjs_uint8)tmp ); // little endian
		}
	}
	argindex += len-1;
}

template<typename TRet, typename TTmp, int NBYTE, typename TRetTmp>
static void ReadNumberBE( std::vector<tjs_uint8>& result, const std::vector<tTJSVariant>& args, tjs_int numargs, tjs_int& argindex, tjs_int len ) {
	if( len < 0 ) len = numargs - argindex;
	if( (len+argindex) > numargs ) len = numargs - argindex;
	for( tjs_int a = 0; a < len; a++ ) {
		TRet c = (TRet)(TTmp)args[argindex+a];
		for( int i = 0; i < NBYTE; i++ ) {
			result.push_back( ((*(TRetTmp*)&c)&(0xFF<<((NBYTE-1-i)*8)))>>((NBYTE-1-i)*8) ); // big endian
		}
	}
	argindex += len-1;
}
#if TJS_HOST_IS_BIG_ENDIAN
#	define ReadNumber ReadNumberBE
#else
#	define ReadNumber ReadNumberLE
#endif

// from base64 plug-in (C) 2009 Kiyobee
// 扱いやすいように一部書き換えている
//	inbuf の内容を base64 エンコードして、outbuf に文字列として出力
//	outbuf のサイズは、insize / 4 * 3 必要
// outbuf のサイズは、(insize+2)/3 * 4 必要
static void encodeBase64( const tjs_uint8* inbuf, tjs_uint insize, std::wstring& outbuf) {
	outbuf.reserve( outbuf.size() + ((insize+2)/3) * 4 );
	static const char* base64str = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	tjs_int	insize_3	= insize - 3;
	tjs_int outptr		= 0;
	tjs_int i;
	for(i=0; i<=insize_3; i+=3)
	{
		outbuf.push_back( base64str[ (inbuf[i  ] >> 2) & 0x3F ] );
		outbuf.push_back( base64str[((inbuf[i  ] << 4) & 0x30) | ((inbuf[i+1] >> 4) & 0x0F)] );
		outbuf.push_back( base64str[((inbuf[i+1] << 2) & 0x3C) | ((inbuf[i+2] >> 6) & 0x03)] );
		outbuf.push_back( base64str[ (inbuf[i+2]     ) & 0x3F ] );
	}
	switch(insize % 3)
	{
	case 2:
		outbuf.push_back( base64str[ (inbuf[i  ] >> 2) & 0x3F ] );
		outbuf.push_back( base64str[((inbuf[i  ] << 4) & 0x30) | ((inbuf[i+1] >> 4) & 0x0F)] );
		outbuf.push_back( base64str[ (inbuf[i+1] << 2) & 0x3C ] );
		outbuf.push_back( '=' );
		break;
	case 1:
		outbuf.push_back( base64str[ (inbuf[i  ] >> 2) & 0x3F ] );
		outbuf.push_back( base64str[ (inbuf[i  ] << 4) & 0x30 ] );
		outbuf.push_back(  '=' );
		outbuf.push_back(  '=' );
		break;
	}
}
static void decodeBase64( const ttstr& inbuf, std::vector<tjs_uint8>& outbuf ) {
	tjs_int len = (tjs_int)inbuf.length();
	const tjs_char* data = inbuf.c_str();
	if( len < 4 ) { // too short
		return;
	}
	outbuf.reserve( len / 4 * 3 );
	static const tjs_int base64tonum[] = {
		   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
		   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
		   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  62,   0,   0,   0,  63, 
		  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,   0,   0,   0,   0,   0,   0, 
		   0,   0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14, 
		  15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25,   0,   0,   0,   0,   0, 
		   0,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40, 
		  41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  51,   0,   0,   0,   0,   0, 
		   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
		   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
		   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
		   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
		   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
		   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
		   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
		   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0
	};

	tjs_int dptr = 0;
	tjs_int len_4 = len - 4;
	while( dptr < len_4 ) {
		outbuf.push_back( static_cast<tjs_uint8>( (base64tonum[data[dptr]] << 2) | (base64tonum[data[dptr+1]] >> 4) ) );
		dptr++;
		outbuf.push_back( static_cast<tjs_uint8>( (base64tonum[data[dptr]] << 4) | (base64tonum[data[dptr+1]] >> 2) ) );
		dptr++;
		outbuf.push_back( static_cast<tjs_uint8>( (base64tonum[data[dptr]] << 6) | (base64tonum[data[dptr+1]]) ) );
		dptr+=2;
	}
	outbuf.push_back( static_cast<tjs_uint8>( (base64tonum[data[dptr]] << 2) | (base64tonum[data[dptr+1]] >> 4) )) ;
	dptr++;
	tjs_uint8 tmp = static_cast<tjs_uint8>( base64tonum[data[dptr++]] << 4 );
	if( data[dptr] != L'=' ) {
		tmp |= base64tonum[data[dptr]] >> 2;
		outbuf.push_back( tmp );
		tmp =  base64tonum[data[dptr++]] << 6;
		if( data[dptr] != L'=' ) {
			tmp |= base64tonum[data[dptr]];
			outbuf.push_back( tmp );
		}
	}
}

static tTJSVariantOctet* Pack( const std::vector<OctPackTemplate>& templ, const std::vector<tTJSVariant>& args ) {
	tjs_int numargs = static_cast<tjs_int>(args.size());
	std::vector<tjs_uint8> result;
	tjs_size count = templ.size();
	tjs_int argindex = 0;
	for( tjs_size i = 0; i < count && argindex < numargs; argindex++ ) {
		OctPackType t = templ[i].Type;
		tjs_int len = templ[i].Length;
		switch( t ) {
		case OctPack_ascii:		// a : ASCII string(ヌル文字が補完される)
			AsciiToBin( result, args[argindex], '\0', len );
			break;
		case OctPack_ASCII:		// A : ASCII string(スペースが補完される)
			AsciiToBin( result, args[argindex], ' ', len );
			break;
		case OctPack_bitstring:	// b : bit string(下位ビットから上位ビットの順)
			BitStringToBin( result, args[argindex], false, len );
			break;
		case OctPack_BITSTRING:	// B : bit string(上位ビットから下位ビットの順)
			BitStringToBin( result, args[argindex], true, len );
			break;
		case OctPack_char:		// c : 符号付き1バイト数値(-128 ～ 127)
			ReadNumber<tjs_int8,tjs_int,1,tjs_int8>( result, args, numargs, argindex, len );
			break;
		case OctPack_CHAR:		// C : 符号無し1バイト数値(0～255)
			ReadNumber<tjs_uint8,tjs_int,1,tjs_uint8>( result, args, numargs, argindex, len );
			break;
		case OctPack_double:	// d : 倍精度浮動小数点
			ReadNumber<tjs_real,tjs_real,8,tjs_uint64>( result, args, numargs, argindex, len );
			break;
		case OctPack_float:		// f : 単精度浮動小数点
			ReadNumber<float,tjs_real,4,tjs_uint32>( result, args, numargs, argindex, len );
			break;
		case OctPack_hex:		// h : hex string(low nybble first)
			HexToBin( result, args[argindex], false, len );
			break;
		case OctPack_HEX:		// H : hex string(high nybble first)
			HexToBin( result, args[argindex], true, len );
			break;
		case OctPack_int:		// i : 符号付きint数値(通常4バイト)
		case OctPack_long:		// l : 符号付きlong数値(通常4バイト)
			ReadNumber<tjs_int,tjs_int,4,tjs_int32>( result, args, numargs, argindex, len );
			break;
		case OctPack_INT:		// I : 符号無しint数値(通常4バイト)
		case OctPack_LONG:		// L : 符号無しlong数値(通常4バイト)
			ReadNumber<tjs_uint,tjs_int64,4,tjs_uint32>( result, args, numargs, argindex, len );
			break;
		case OctPack_noshort:	// n : unsigned short数値(ネットワークバイトオーダ) network byte order short
			ReadNumberBE<tjs_uint16,tjs_int,2,tjs_uint16>( result, args, numargs, argindex, len );
			break;
		case OctPack_NOLONG:	// N : unsigned long数値(ネットワークバイトオーダ) network byte order long
			ReadNumberBE<tjs_uint,tjs_int64,4,tjs_uint32>( result, args, numargs, argindex, len );
			break;
		case OctPack_pointer:	// p : 文字列へのポインタ null terminate char
		case OctPack_POINTER:	// P : 構造体(固定長文字列)へのポインタ fix length char
			// TODO
			break;
		case OctPack_short:		// s : 符号付きshort数値(通常2バイト) sign
			ReadNumber<tjs_int16,tjs_int,2,tjs_int16>( result, args, numargs, argindex, len );
			break;
		case OctPack_SHORT:		// S : 符号無しshort数値(通常2バイト) unsign
			ReadNumber<tjs_uint16,tjs_int,2,tjs_uint16>( result, args, numargs, argindex, len );
			break;
		case OctPack_leshort:	// v : リトルエンディアンによるunsigned short値 little endian short
			ReadNumberLE<tjs_uint16,tjs_int,2,tjs_uint16>( result, args, numargs, argindex, len );
			break;
		case OctPack_LELONG:	// V : リトルエンディアンによるunsigned long値 little endian long
			ReadNumberLE<tjs_uint,tjs_int64,4,tjs_uint32>( result, args, numargs, argindex, len );
			break;
		case OctPack_uuencode:	// u : uuencodeされた文字列
			TJS_eTJSError( TJSNotSupportedUuencode );
			break;
		case OctPack_BRE:		// w : BER圧縮された整数値
			TJS_eTJSError( TJSNotSupportedBER );
			break;
		case OctPack_null:		// x : ヌル文字
			for( tjs_int a = 0; a < len; a++ ) {
				result.push_back( 0 );
			}
			argindex--;
			break;
		case OctPack_NULL:		// X : back up a byte
			for( tjs_int a = 0; a < len; a++ ) {
				result.pop_back();
			}
			argindex--;
			break;
		case OctPack_fill: {		// @ : 絶対位置までヌル文字を埋める
			tjs_size count = result.size();
			for( tjs_size i = count; i < (tjs_size)len; i++ ) {
				result.push_back( 0 );
			}
			argindex--;
			break;
		}
		case OctPack_base64: {	// m : Base64 encode / decode
			ttstr tmp = args[argindex];
			decodeBase64( tmp, result );
			break;
		}
		case OctPack_EOT:
			break;
		}
		if( len >= 0 ) { // '*' の時は-1が入り、リストの末尾まで読む
			i++;
		}
	}
	if( result.size() > 0 )
		return TJSAllocVariantOctet( &(result[0]), (tjs_uint)result.size() );
	else
		return NULL;
}

static void BinToAscii( const tjs_uint8 *data, const tjs_uint8 *tail, tjs_uint len, ttstr& result ) {
	//std::vector<tjs_nchar> tmp(len+1);
	std::vector<tjs_nchar> tmp;
	tmp.reserve(len+1);
	for( tjs_int i = 0; i < static_cast<tjs_int>(len) && data != tail; data++, i++ ) {
		if( (*data) != '\0' ) {
			tmp.push_back( (tjs_nchar)*data );
		}
	}
	tmp.push_back( (tjs_nchar)'\0' );
	result = tTJSString( &(tmp[0]) );
}
// mtol : true : 上位ビットから下位ビット, false : 下位ビットから上位ビット
// 指定した数値の方が大きくても、その分は無視
static void BinToBitString( const tjs_uint8 *data, const tjs_uint8 *tail, tjs_uint len, ttstr& result, bool mtol ) {
	//std::vector<tjs_char> tmp(len+1);
	std::vector<tjs_char> tmp;
	tmp.reserve(len+1);
	tjs_int pos = 0;
	if( mtol ) {
		for( ; data < tail; data++ ) {
			for( tjs_int i = 0; i < 8 && pos < static_cast<tjs_int>(len); i++, pos++ ) {
				if( (*data)&(0x01<<(7-i)) ) {
					tmp.push_back( L'1' );
				} else {
					tmp.push_back( L'0' );
				}
			}
			if( pos >= static_cast<tjs_int>(len) ) break;
		}
	} else {
		for( ; data < tail; data++ ) {
			for( tjs_int i = 0; i < 8 && pos < static_cast<tjs_int>(len); i++, pos++ ) {
				if( (*data)&(0x01<<i) ) {
					tmp.push_back( L'1' );
				} else {
					tmp.push_back( L'0' );
				}
			}
			if( pos >= static_cast<tjs_int>(len) ) break;
		}
	}
	tmp.push_back( L'\0' );
	result = tTJSString( &(tmp[0]) );
}

// TRet : 最終的に出力する型
template<typename TRet, int NBYTE>
static void BinToNumberLE( std::vector<TRet>& result, const tjs_uint8 *data, const tjs_uint8 *tail, tjs_int len ) {
	if( len < 0 ) len = (tjs_uint)(((tail - data)+NBYTE-1)/NBYTE);
	if( (data+len*NBYTE) < tail ) tail = data+len*NBYTE;
	TRet val = 0;
	tjs_uint bytes = 0;
	for( ; data < tail; data++ ) {
		val |= (*data) << (bytes*8);
		if( bytes >= (NBYTE-1) ) { // little endian
			bytes = 0;
			result.push_back( val );
			val = 0;
		} else {
			bytes++;
		}
	}
	if( bytes ) {
		result.push_back( val );
	}
}

template<typename TRet, typename TTmp, int NBYTE>
static void BinToNumberLEReal( std::vector<TRet>& result, const tjs_uint8 *data, const tjs_uint8 *tail, tjs_int len ) {
	if( len < 0 ) len = (tjs_uint)(((tail - data)+NBYTE-1)/NBYTE);
	if( (data+len*NBYTE) < tail ) tail = data+len*NBYTE;
	TTmp val = 0;
	tjs_uint bytes = 0;
	for( ; data < tail; data++ ) {
		val |= (TTmp)(*data) << (bytes*8);
		if( bytes >= (NBYTE-1) ) { // little endian
			bytes = 0;
			result.push_back( *(TRet*)&val );
			val = 0;
		} else {
			bytes++;
		}
	}
	if( bytes ) {
		result.push_back( *(TRet*)&val );
	}
}
template<typename TRet, int NBYTE>
static void BinToNumberBE( std::vector<TRet>& result, const tjs_uint8 *data, const tjs_uint8 *tail, tjs_int len ) {
	if( len < 0 ) len = (tjs_uint)(((tail - data)+NBYTE-1)/NBYTE);
	if( (data+len*NBYTE) < tail ) tail = data+len*NBYTE;
	TRet val = 0;
	tjs_uint bytes = NBYTE-1;
	for( ; data < tail; data++ ) {
		val |= (*data) << (bytes*8);
		if( bytes == 0 ) { // big endian
			bytes = NBYTE-1;
			result.push_back( val );
			val = 0;
		} else {
			bytes--;
		}
	}
	if( bytes < (NBYTE-1) ) {
		result.push_back( val );
	}
}

#if TJS_HOST_IS_BIG_ENDIAN
#	define BinToNumber BinToNumberBE
#else
#	define BinToNumber BinToNumberLE
#	define BinToReal BinToNumberLEReal
#endif

// mtol
static void BinToHex( const tjs_uint8 *data, const tjs_uint8 *tail, tjs_uint len, ttstr& result, bool mtol ) {
	if( (data+len) < tail ) tail = data+(len+1)/2;
	//std::vector<tjs_char> tmp(len+1);
	std::vector<tjs_char> tmp;
	tmp.reserve(len+1);
	tjs_int pos = 0;
	if( mtol ) { // 上位ニブルが先
		pos = 1;
		for( tjs_int i = 0; i < static_cast<tjs_int>(len) && data < tail; i++ ) {
			tjs_char ch = ((*data)&(0xF<<(pos*4)))>>(pos*4);
			if( ch > 9 ) {
				ch = L'A' + (ch-10);
			} else {
				ch = L'0' + ch;
			}
			tmp.push_back( ch );
			if( pos == 0 ) {
				pos = 1;
				data++;
			} else {
				pos--;
			}
		}
	} else { // 下位ニブルが先
		for( tjs_int i = 0; i < static_cast<tjs_int>(len) && data < tail; i++ ) {
			tjs_char ch = ((*data)&(0xF<<(pos*4)))>>(pos*4);
			if( ch > 9 ) {
				ch = L'A' + (ch-10);
			} else {
				ch = L'0' + ch;
			}
			tmp.push_back( ch );
			if( pos ) {
				pos = 0;
				data++;
			} else {
				pos++;
			}
		}
	}
	tmp.push_back( L'\0' );
	result = tTJSString( &(tmp[0]) );
}
static iTJSDispatch2* Unpack( const std::vector<OctPackTemplate>& templ, const tjs_uint8 *data, tjs_uint length ) {
	tTJSArrayObject* result = reinterpret_cast<tTJSArrayObject*>( TJSCreateArrayObject() );
	tTJSArrayNI *ni;
	if(TJS_FAILED(result->NativeInstanceSupport(TJS_NIS_GETINSTANCE, TJSGetArrayClassID(), (iTJSNativeInstance**)&ni)))
		TJS_eTJSError(TJSSpecifyArray);

	const tjs_uint8 *current = data;
	const tjs_uint8 *tail = data + length;
	tjs_size len = length;
	tjs_size count = templ.size();
	tjs_int argindex = 0;
	for( tjs_int i = 0; i < (tjs_int)count && current < tail; argindex++ ) {
		OctPackType t = templ[i].Type;
		tjs_int len = templ[i].Length;
		switch( t ) {
		case OctPack_ascii:{ 	// a : ASCII string(ヌル文字が補完される)
			if( len < 0 ) len = (tail - current);
			ttstr ret;
			BinToAscii( current, tail, (tjs_uint)len, ret );
			result->Add( ni, tTJSVariant( ret ) );
			current += len;
			break;
		}
		case OctPack_ASCII: {		// A : ASCII string(スペースが補完される)
			if( len < 0 ) len = (tail - current);
			ttstr ret;
			BinToAscii( current, tail, (tjs_uint)len, ret );
			result->Add( ni, tTJSVariant( ret ) );
			current += len;
			break;
		}
		case OctPack_bitstring: {	// b : bit string(下位ビットから上位ビットの順)
			if( len < 0 ) len = (tail - current)*8;
			ttstr ret;
			BinToBitString( current, tail, (tjs_uint)len, ret, false );
			result->Add( ni, tTJSVariant( ret ) );
			current += (len+7)/8;
			break;
		}
		case OctPack_BITSTRING: {	// B : bit string(上位ビットから下位ビットの順)
			if( len < 0 ) len = (tail - current)*8;
			ttstr ret;
			BinToBitString( current, tail, (tjs_uint)len, ret, true );
			result->Add( ni, tTJSVariant( ret ) );
			current += (len+7)/8;
			break;
		}
		case OctPack_char: {		// c : 符号付き1バイト数値(-128 ～ 127)
			if( len < 0 ) len = tail - current;
			std::vector<tjs_int8> ret;
			BinToNumber<tjs_int8,1>( ret, current, tail, (tjs_uint)len );
			for( std::vector<tjs_int8>::const_iterator iter = ret.begin(); iter != ret.end(); iter++ ) {
				result->Add( ni, tTJSVariant( (tjs_int)*iter ) );
			}
			current += len;
			break;
		}
		case OctPack_CHAR: {		// C : 符号無し1バイト数値(0～255)
			if( len < 0 ) len = tail - current;
			std::vector<tjs_uint8> ret;
			BinToNumber<tjs_uint8,1>( ret, current, tail, (tjs_uint)len );
			for( std::vector<tjs_uint8>::const_iterator iter = ret.begin(); iter != ret.end(); iter++ ) {
				result->Add( ni, tTJSVariant( (tjs_int)*iter ) );
			}
			current += len;
			break;
		}
		case OctPack_double: {	// d : 倍精度浮動小数点
			if( len < 0 ) len = (tail - current)/8;
			std::vector<tjs_real> ret;
			BinToReal<tjs_real,tjs_uint64,8>( ret, current, tail, (tjs_uint)len );
			for( std::vector<tjs_real>::const_iterator iter = ret.begin(); iter != ret.end(); iter++ ) {
				result->Add( ni, tTJSVariant( (tjs_real)*iter ) );
			}
			current += len*8;
			break;
		}
		case OctPack_float: {		// f : 単精度浮動小数点
			if( len < 0 ) len = (tail - current)/4;
			std::vector<float> ret;
			BinToReal<float,tjs_uint32,4>( ret, current, tail, (tjs_uint)len );
			for( std::vector<float>::const_iterator iter = ret.begin(); iter != ret.end(); iter++ ) {
				result->Add( ni, tTJSVariant( (tjs_real)*iter ) );
			}
			current += len*4;
			break;
		}
		case OctPack_hex: {		// h : hex string(low nybble first)
			if( len < 0 ) len = (tail - current)*2;
			ttstr ret;
			BinToHex( current, tail, (tjs_uint)len, ret, false );
			result->Add( ni, tTJSVariant( ret ) );
			current += (len+1)/2;
			break;
		}
		case OctPack_HEX: {		// H : hex string(high nybble first)
			if( len < 0 ) len = (tail - current)*2;
			ttstr ret;
			BinToHex( current, tail, (tjs_uint)len, ret, true );
			result->Add( ni, tTJSVariant( ret ) );
			current += (len+1)/2;
			break;
		}
		case OctPack_int:		// i : 符号付きint数値(通常4バイト)
		case OctPack_long: {	// l : 符号付きlong数値(通常4バイト)
			if( len < 0 ) len = (tail - current)/4;
			std::vector<tjs_int> ret;
			BinToNumber<tjs_int,4>( ret, current, tail, (tjs_uint)len );
			for( std::vector<tjs_int>::const_iterator iter = ret.begin(); iter != ret.end(); iter++ ) {
				result->Add( ni, tTJSVariant( (tjs_int)*iter ) );
			}
			current += len*4;
			break;
		}
		case OctPack_INT:		// I : 符号無しint数値(通常4バイト)
		case OctPack_LONG: {	// L : 符号無しlong数値(通常4バイト)
			if( len < 0 ) len = (tail - current)/4;
			std::vector<tjs_uint> ret;
			BinToNumber<tjs_uint,4>( ret, current, tail, (tjs_uint)len );
			for( std::vector<tjs_uint>::const_iterator iter = ret.begin(); iter != ret.end(); iter++ ) {
				result->Add( ni, tTJSVariant( (tjs_int64)*iter ) );
			}
			current += len*4;
			break;
		}
		case OctPack_noshort: {	// n : unsigned short数値(ネットワークバイトオーダ) network byte order short
			if( len < 0 ) len = (tail - current)/2;
			std::vector<tjs_uint16> ret;
			BinToNumberBE<tjs_uint16,2>( ret, current, tail, (tjs_uint)len );
			for( std::vector<tjs_uint16>::const_iterator iter = ret.begin(); iter != ret.end(); iter++ ) {
				result->Add( ni, tTJSVariant( (tjs_int)*iter ) );
			}
			current += len*2;
			break;
		}
		case OctPack_NOLONG: {	// N : unsigned long数値(ネットワークバイトオーダ) network byte order long
			if( len < 0 ) len = ((tail - current)/4);
			std::vector<tjs_uint> ret;
			BinToNumberBE<tjs_uint,4>( ret, current, tail, (tjs_uint)len );
			for( std::vector<tjs_uint>::const_iterator iter = ret.begin(); iter != ret.end(); iter++ ) {
				result->Add( ni, tTJSVariant( (tjs_int64)*iter ) );
			}
			current += len*4;
			break;
		}
		case OctPack_pointer:	// p : 文字列へのポインタ null terminate char
			TJS_eTJSError( TJSNotSupportedUnpackLP );
			break;
		case OctPack_POINTER:	// P : 構造体(固定長文字列)へのポインタ fix length char
			TJS_eTJSError( TJSNotSupportedUnpackP );
			break;
		case OctPack_short: {	// s : 符号付きshort数値(通常2バイト) sign
			if( len < 0 ) len = ((tail - current)/2);
			std::vector<tjs_int16> ret;
			BinToNumber<tjs_int16,2>( ret, current, tail, (tjs_uint)len );
			for( std::vector<tjs_int16>::const_iterator iter = ret.begin(); iter != ret.end(); iter++ ) {
				result->Add( ni, tTJSVariant( (tjs_int)*iter ) );
			}
			current += len*2;
			break;
		}
		case OctPack_SHORT: {		// S : 符号無しshort数値(通常2バイト) unsign
			if( len < 0 ) len = ((tail - current)/2);
			std::vector<tjs_uint16> ret;
			BinToNumber<tjs_uint16,2>( ret, current, tail, (tjs_uint)len );
			for( std::vector<tjs_uint16>::const_iterator iter = ret.begin(); iter != ret.end(); iter++ ) {
				result->Add( ni, tTJSVariant( (tjs_int)*iter ) );
			}
			current += len*2;
			break;
		}
		case OctPack_leshort: {	// v : リトルエンディアンによるunsigned short値 little endian short
			if( len < 0 ) len = ((tail - current)/2);
			std::vector<tjs_uint16> ret;
			BinToNumberLE<tjs_uint16,2>( ret, current, tail, (tjs_uint)len );
			for( std::vector<tjs_uint16>::const_iterator iter = ret.begin(); iter != ret.end(); iter++ ) {
				result->Add( ni, tTJSVariant( (tjs_int)*iter ) );
			}
			current += len*2;
			break;
		}
		case OctPack_LELONG: {	// V : リトルエンディアンによるunsigned long値 little endian long
			if( len < 0 ) len = ((tail - current)/4);
			std::vector<tjs_uint> ret;
			BinToNumberLE<tjs_uint,4>( ret, current, tail, (tjs_uint)len );
			for( std::vector<tjs_uint>::const_iterator iter = ret.begin(); iter != ret.end(); iter++ ) {
				result->Add( ni, tTJSVariant( (tjs_int64)*iter ) );
			}
			current += len*4;
			break;
		}
		case OctPack_uuencode:	// u : uuencodeされた文字列
			TJS_eTJSError( TJSNotSupportedUuencode );
			break;
		case OctPack_BRE:		// w : BER圧縮された整数値
			TJS_eTJSError( TJSNotSupportedBER );
			break;
		case OctPack_null:		// x : ヌル文字
			if( len < 0 ) len = (tail - current);
			for( tjs_int x = 0; x < (tjs_int)len; x++ ) {
				current++;
			}
			break;
		case OctPack_NULL:		// X : back up a byte
			if( len < 0 ) len = (current - data);
			for( tjs_int x = 0; x < (tjs_int)len; x++ ) {
				if( data != current ) current--;
				else break;
			}
			break;
		case OctPack_fill: {		// @ : 絶対位置までヌル文字を埋める
			if( len < 0 ) len = (tail - current);
			current = &(data[len]);
			break;
		}
		case OctPack_base64: {	// m : Base64 encode / decode
			std::wstring ret;
			encodeBase64( current, (tjs_uint)(tail-current), ret );
			result->Add( ni, tTJSVariant( ret.c_str() ) );
			current = tail;
			break;
		}
		case OctPack_EOT:
			break;
		}
		i++;
	}
	return result;
}



tjs_error TJSOctetPack( tTJSVariant **args, tjs_int numargs, const std::vector<tTJSVariant>& items, tTJSVariant *result ) {
	if( numargs < 1 ) return TJS_E_BADPARAMCOUNT;
	if( args[0]->Type() != tvtString ) return TJS_E_INVALIDPARAM;

	if( result ) {
		std::vector<OctPackTemplate> templ;
		ParsePackTemplate( templ, ((ttstr)*args[0]).c_str() );
		tTJSVariantOctet* oct = Pack( templ, items );
		*result = oct;
		if( oct ) oct->Release();
		else *result = tTJSVariant((iTJSDispatch2*)NULL,(iTJSDispatch2*)NULL);
	}
	return TJS_S_OK;
}
tjs_error TJSOctetUnpack( const tTJSVariantOctet * target, tTJSVariant **args, tjs_int numargs, tTJSVariant *result ) {
	if( numargs < 1 ) return TJS_E_BADPARAMCOUNT;
	if( args[0]->Type() != tvtString ) return TJS_E_INVALIDPARAM;
	if( !target ) return TJS_E_INVALIDPARAM;

	if( result ) {
		std::vector<OctPackTemplate> templ;
		ParsePackTemplate( templ, ((ttstr)*args[0]).c_str() );
		iTJSDispatch2* disp = Unpack( templ, target->GetData(), target->GetLength() );
		*result = tTJSVariant(disp,disp);
		if( disp ) disp->Release();
	}
	return TJS_S_OK;
}

} // namespace TJS
