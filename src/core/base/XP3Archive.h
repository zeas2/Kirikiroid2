//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// XP3 virtual file system support
//---------------------------------------------------------------------------

#ifndef XP3ArchiveH
#define XP3ArchiveH


#include "StorageIntf.h"



/*[*/
//---------------------------------------------------------------------------
// Extraction filter related
//---------------------------------------------------------------------------
#pragma pack(push, 4)
struct tTVPXP3ExtractionFilterInfo
{
	const tjs_uint SizeOfSelf; // structure size of tTVPXP3ExtractionFilterInfo itself
	const tjs_uint64 Offset; // offset of the buffer data in uncompressed stream position
	void * Buffer; // target data buffer
	const tjs_uint BufferSize; // buffer size in bytes pointed by "Buffer"
	const tjs_uint32 FileHash; // hash value of the file (since inteface v2)
	const ttstr &FileName;

	tTVPXP3ExtractionFilterInfo(tjs_uint64 offset, void *buffer,
		tjs_uint buffersize, tjs_uint32 filehash, const ttstr& filename) :
			Offset(offset), Buffer(buffer), BufferSize(buffersize),
			FileHash(filehash), FileName(filename),
			SizeOfSelf(sizeof(tTVPXP3ExtractionFilterInfo)) {;}
};
#pragma pack(pop)

#ifndef TVP_tTVPXP3ArchiveExtractionFilter_CONVENTION
	#ifdef _WIN32
		#define	TVP_tTVPXP3ArchiveExtractionFilter_CONVENTION _stdcall
	#else
		#define TVP_tTVPXP3ArchiveExtractionFilter_CONVENTION
	#endif
#endif
	// TVP_tTVPXP3ArchiveExtractionFilter_CONV is _stdcall on win32 platforms,
	// for backward application compatibility.

typedef void (TVP_tTVPXP3ArchiveExtractionFilter_CONVENTION *
	tTVPXP3ArchiveExtractionFilter)(tTVPXP3ExtractionFilterInfo *info, tTJSVariant *ctx);
typedef tjs_int (TVP_tTVPXP3ArchiveExtractionFilter_CONVENTION *
	tTVPXP3ArchiveContentFilter)(const ttstr &filepath, const ttstr &archivename, tjs_uint64 filesize, tTJSVariant *ctx);

/*]*/
//---------------------------------------------------------------------------
TJS_EXP_FUNC_DEF(void, TVPSetXP3ArchiveExtractionFilter, (tTVPXP3ArchiveExtractionFilter filter));
TJS_EXP_FUNC_DEF(void, TVPSetXP3ArchiveContentFilter, (tTVPXP3ArchiveContentFilter filter));
//---------------------------------------------------------------------------





//---------------------------------------------------------------------------
// tTVPXP3Archive  : XP3 ( TVP's native archive format ) Implmentation
//---------------------------------------------------------------------------
#define TVP_XP3_INDEX_ENCODE_METHOD_MASK 0x07
#define TVP_XP3_INDEX_ENCODE_RAW      0
#define TVP_XP3_INDEX_ENCODE_ZLIB     1

#define TVP_XP3_INDEX_CONTINUE   0x80

#define TVP_XP3_FILE_PROTECTED (1<<31)

#define TVP_XP3_SEGM_ENCODE_METHOD_MASK  0x07
#define TVP_XP3_SEGM_ENCODE_RAW       0
#define TVP_XP3_SEGM_ENCODE_ZLIB      1

//---------------------------------------------------------------------------
extern bool TVPIsXP3Archive(const ttstr &name); // check XP3 archive
extern void TVPClearXP3SegmentCache(); // clear XP3 segment cache
//---------------------------------------------------------------------------
struct tTVPXP3ArchiveSegment
{
	tjs_uint64 Start;  // start position in archive storage
	tjs_uint64 Offset; // offset in in-archive storage (in uncompressed offset)
	tjs_uint64 OrgSize; // original segment (uncompressed) size
	tjs_uint64 ArcSize; // in-archive segment (compressed) size
	bool IsCompressed; // is compressed ?
};
//---------------------------------------------------------------------------
class tTVPXP3Archive : public tTVPArchive
{
public:
	struct tArchiveItem
	{
		ttstr Name;
		tjs_uint32 FileHash;
		tjs_uint64 OrgSize; // original ( uncompressed ) size
		tjs_uint64 ArcSize; // in-archive size
		std::vector<tTVPXP3ArchiveSegment> Segments;
		bool operator < (const tArchiveItem &rhs) const
		{
			return this->Name < rhs.Name;
		}
	};

	tjs_int Count = 0;

	std::vector<tArchiveItem> ItemVector;
	void Init(tTJSBinaryStream *st, tjs_int64 offset, bool normalizeName = true);

public:
	tTVPXP3Archive(const ttstr & name, int) : tTVPArchive(name) {}
	tTVPXP3Archive(const ttstr & name, tTJSBinaryStream *st = nullptr, tjs_int64 offset = -1, bool normalizeFileName = true);
	~tTVPXP3Archive();

	static tTVPArchive *Create(const ttstr & name, tTJSBinaryStream *st = nullptr, bool normalizeFileName = true);

	tjs_uint GetCount() { return Count; }
	const ttstr & GetName(tjs_uint idx) const { return ItemVector[idx].Name; }
	tjs_uint32 GetFileHash(tjs_uint idx) const { return ItemVector[idx].FileHash; }
	ttstr GetName(tjs_uint idx) { return ItemVector[idx].Name; }

	const ttstr & GetName() const { return ArchiveName; }

	tTJSBinaryStream * CreateStreamByIndex(tjs_uint idx);

private:
	static bool FindChunk(const tjs_uint8 *data, const tjs_uint8 * name,
		tjs_uint &start, tjs_uint &size);
	static tjs_int16 ReadI16FromMem(const tjs_uint8 *mem);
	static tjs_int32 ReadI32FromMem(const tjs_uint8 *mem);
	static tjs_int64 ReadI64FromMem(const tjs_uint8 *mem);
};
//---------------------------------------------------------------------------





//---------------------------------------------------------------------------
// tTVPXP3ArchiveStream  : XP3 In-Archive Stream Implmentation
//---------------------------------------------------------------------------
class tTVPSegmentData;
class tTVPXP3ArchiveStream : public tTJSBinaryStream
{
	tTVPXP3Archive * Owner;

	tjs_int StorageIndex; // index in archive

	std::vector<tTVPXP3ArchiveSegment> * Segments;
	tTJSBinaryStream * Stream;
	tjs_uint64 OrgSize; // original storage size

	tjs_int CurSegmentNum;
	tTVPXP3ArchiveSegment *CurSegment;
		// currently opened segment ( NULL for not opened )

	tjs_int LastOpenedSegmentNum;

	tjs_uint64 CurPos; // current position in absolute file position

	tjs_uint64 SegmentRemain; // remain bytes in current segment
	tjs_uint64 SegmentPos; // offset from current segment's start

	tTVPSegmentData *SegmentData; // uncompressed segment data

	bool SegmentOpened;
	tTJSVariant FilterContext;

public:
	tTVPXP3ArchiveStream(tTVPXP3Archive *owner, tjs_int storageindex,
		std::vector<tTVPXP3ArchiveSegment> *segments, tTJSBinaryStream *stream,
			tjs_uint64 orgsize);
	~tTVPXP3ArchiveStream();
	tTJSVariant &GetFilterContext() { return FilterContext; }

private:
	void EnsureSegment(); // ensure accessing to current segment
	void SeekToPosition(tjs_uint64 pos); // open segment at 'pos' and seek
	bool OpenNextSegment();


public:
	tjs_uint64 TJS_INTF_METHOD Seek(tjs_int64 offset, tjs_int whence);
	tjs_uint TJS_INTF_METHOD Read(void *buffer, tjs_uint read_size);
	tjs_uint TJS_INTF_METHOD Write(const void *buffer, tjs_uint write_size);
	tjs_uint64 TJS_INTF_METHOD GetSize();

};
//---------------------------------------------------------------------------





#endif
