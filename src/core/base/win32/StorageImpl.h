//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Universal Storage System
//---------------------------------------------------------------------------
#ifndef StorageImplH
#define StorageImplH
//---------------------------------------------------------------------------
#include "StorageIntf.h"
#include "UtilStreams.h"
#include <functional>

//#include <objidl.h> // for IStream

#if 0
//---------------------------------------------------------------------------
// Susie plug-in related
//---------------------------------------------------------------------------
void TVPLoadArchiveSPI(HINSTANCE inst);
void TVPUnloadArchiveSPI(HINSTANCE inst);
//---------------------------------------------------------------------------
#endif

#ifndef S_IFMT
#define S_IFDIR  0x4000 // Directory
#define S_IFREG  0x8000 // Regular
#endif

struct tTVPLocalFileInfo {
	const char * NativeName;
	unsigned short Mode; // S_IFMT
	tjs_uint64         Size;
	time_t         AccessTime;
	time_t         ModifyTime;
	time_t         CreationTime;
};

void TVPGetLocalFileListAt(const ttstr &name, const std::function<void(const ttstr&, tTVPLocalFileInfo*)>& cb);

//---------------------------------------------------------------------------
// tTVPLocalFileStream
//---------------------------------------------------------------------------
class tTVPLocalFileStream : public tTJSBinaryStream
{
private:
	//HANDLE Handle;
    int Handle;
    tTVPMemoryStream *MemBuffer = nullptr;
    ttstr FileName;

public:
	tTVPLocalFileStream(const ttstr &origname, const ttstr & localname,
		tjs_uint32 flag);
	~tTVPLocalFileStream();

	tjs_uint64 TJS_INTF_METHOD Seek(tjs_int64 offset, tjs_int whence);

	tjs_uint TJS_INTF_METHOD Read(void *buffer, tjs_uint read_size);
	tjs_uint TJS_INTF_METHOD Write(const void *buffer, tjs_uint write_size);

	void TJS_INTF_METHOD SetEndOfStorage();

	tjs_uint64 TJS_INTF_METHOD GetSize();

	int GetHandle() const { return Handle; }
};
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
TJS_EXP_FUNC_DEF(bool, TVPCheckExistentLocalFolder, (const ttstr &name));
	/* name must be an OS's NATIVE folder name */

TJS_EXP_FUNC_DEF(bool, TVPCheckExistentLocalFile, (const ttstr &name));
	/* name must be an OS's NATIVE file name */

TJS_EXP_FUNC_DEF(bool, TVPCreateFolders, (const ttstr &folder));
	/* make folders recursively, like mkdir -p. folder must be OS NATIVE folder name */
//---------------------------------------------------------------------------





#ifdef TJS_SUPPORT_VCL
//---------------------------------------------------------------------------
// TTVPStreamAdapter
//---------------------------------------------------------------------------
/*
	this class provides VCL's TStream adapter for tTJSBinaryStream
*/
class TTVPStreamAdapter : public TStream
{
private:
	tTJSBinaryStream *Stream;

public:
	__fastcall TTVPStreamAdapter(tTJSBinaryStream *ref);
	/*
		the stream passed by argument here is freed by this instance'
		destruction.
	*/

	__fastcall ~TTVPStreamAdapter();

	int __fastcall Read(void *Buffer,int Count);
		// read
	int __fastcall Seek(int Offset,WORD Origin);
		// seek
	int __fastcall Write(const void *Buffer,int Count);

	__property Size;
	__property Position;
};
//---------------------------------------------------------------------------
#endif

class tTVPIStreamAdapter;


struct IStream;
//---------------------------------------------------------------------------
// IStream creator
//---------------------------------------------------------------------------
TJS_EXP_FUNC_DEF(IStream *, TVPCreateIStream, (const ttstr &name, tjs_uint32 flags));
TJS_EXP_FUNC_DEF(IStream *, TVPCreateIStream, (tTJSBinaryStream *));
//---------------------------------------------------------------------------
#if 0




//---------------------------------------------------------------------------
// tTVPBinaryStreamAdapter
//---------------------------------------------------------------------------
/*
	this class provides tTJSBinaryStream adapter for IStream
*/
class tTVPBinaryStreamAdapter : public tTJSBinaryStream
{
	typedef tTJSBinaryStream inherited;

private:
	IStream *Stream;

public:
	tTVPBinaryStreamAdapter(IStream *ref);
	/*
		the stream passed by argument here is freed by this instance'
		destruction.
	*/

	~tTVPBinaryStreamAdapter();

	tjs_uint64 TJS_INTF_METHOD Seek(tjs_int64 offset, tjs_int whence);
	tjs_uint TJS_INTF_METHOD Read(void *buffer, tjs_uint read_size);
	tjs_uint TJS_INTF_METHOD Write(const void *buffer, tjs_uint write_size);
	tjs_uint64 TJS_INTF_METHOD GetSize();
	void TJS_INTF_METHOD SetEndOfStorage();
};
//---------------------------------------------------------------------------




//---------------------------------------------------------------------------
// tTVPBinaryStreamAdapter creator
//---------------------------------------------------------------------------
TJS_EXP_FUNC_DEF(tTJSBinaryStream *, TVPCreateBinaryStreamAdapter, (IStream *refstream));
//---------------------------------------------------------------------------
#endif


//---------------------------------------------------------------------------
// tTVPPluginHolder
//---------------------------------------------------------------------------
/*
	tTVPPluginHolder holds plug-in. if the plug-in is not a local storage,
	plug-in is to be extracted to temporary directory and be held until
	the program done using it.
*/
class tTVPPluginHolder
{
private:
	ttstr LocalName;
	tTVPLocalTempStorageHolder * LocalTempStorageHolder;

public:
	tTVPPluginHolder(const ttstr &aname);
	~tTVPPluginHolder();

	const ttstr & GetLocalName() const;
};
//---------------------------------------------------------------------------

void TVPListDir(const std::string &folder, std::function<void(const std::string&, int)> cb);
bool TVPSaveStreamToFile(tTJSBinaryStream *st, tjs_uint64 offset, tjs_uint64 size, ttstr outpath);
#endif
