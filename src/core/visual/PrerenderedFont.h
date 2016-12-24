
#ifndef __TVP_PRERENDERED_FONT_H__
#define __TVP_PRERENDERED_FONT_H__

#include "tjsCommHead.h"
#include "UtilStreams.h"

#pragma pack(push, 1)
struct tTVPPrerenderedCharacterItem
{
	tjs_uint32 Offset;
	tjs_uint16 Width;
	tjs_uint16 Height;
	tjs_int16 OriginX;
	tjs_int16 OriginY;
	tjs_int16 IncX;
	tjs_int16 IncY;
	tjs_int16 Inc;
	tjs_uint16 Reserved;
};
#pragma pack(pop)

//---------------------------------------------------------------------------
// tTVPPrerenderedFont
//---------------------------------------------------------------------------
class tTVPPrerenderedFont
{
private:
	ttstr Storage;
	// HANDLE FileHandle; // tft file handle
	// HANDLE MappingHandle; // file mapping handle
	// ファイルマッピングではなく、全て最初にデータを読み込んでしまう形にする。
	// BinaryStream で読み込む
	const tjs_uint8 * Image; // tft mapped memory
	tjs_uint64 FileLength;
	tjs_uint RefCount;
//	tTVPLocalTempStorageHolder LocalStorage;

	tjs_int Version; // data version
	const tjs_char * ChIndex;
	const tTVPPrerenderedCharacterItem * Index;
	tjs_uint IndexCount;

public:
	tTVPPrerenderedFont(const ttstr &storage);
	~tTVPPrerenderedFont();
	void AddRef();
	void Release();

	const tTVPPrerenderedCharacterItem* Find(tjs_char ch); // serch character
	void Retrieve(const tTVPPrerenderedCharacterItem * item, tjs_uint8 *buffer, tjs_int bufferpitch);

};

#endif // __TVP_PRERENDERED_FONT_H__

