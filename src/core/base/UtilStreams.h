//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Stream related utilities / utility streams
//---------------------------------------------------------------------------
#ifndef UtilStreamsH
#define UtilStreamsH


#include "StorageIntf.h"
#include <functional>



//---------------------------------------------------------------------------
// tTVPStreamHolder
//---------------------------------------------------------------------------
class tTVPStreamHolder
{
	tTJSBinaryStream *Stream;
public:
	tTVPStreamHolder()
	{
		Stream = NULL;
	}

	tTVPStreamHolder(const ttstr& name, tjs_uint32 mode = 0)
	{
		Stream = TVPCreateStream(name, mode);
	}

	~tTVPStreamHolder()
	{
		if(Stream) delete Stream;
	}

	tTJSBinaryStream * operator ->() const { return Stream; }

	tTJSBinaryStream * Get() const { return Stream; }

	void Close()
	{
		if(Stream)
		{
			delete Stream;
			Stream = NULL;
		}
	}

	void Disown()
	{
		Stream = NULL;
	}

	void Open(const ttstr & name, tjs_uint32 flag = 0)
	{
		if(Stream) delete Stream, Stream = NULL;
		Stream = TVPCreateStream(name, flag);
	}
};
//---------------------------------------------------------------------------







//---------------------------------------------------------------------------
// tTVPLocalTempStorageHolder
//---------------------------------------------------------------------------
// this class holds storage as local filesystem file ( does not open it ).
// storage is copied to local fliesystem if needed.
class tTVPLocalTempStorageHolder
{
	bool FileMustBeDeleted;
	bool FolderMustBeDeleted;
	ttstr LocalName;
	ttstr LocalFolder;
public:
	tTVPLocalTempStorageHolder(const ttstr & name);
	~tTVPLocalTempStorageHolder();

	bool IsTemporaryFile() const { return FileMustBeDeleted; }

	const ttstr & GetLocalName() const { return LocalName; }
};
//---------------------------------------------------------------------------







//---------------------------------------------------------------------------
// tTVPMemoryStream
//---------------------------------------------------------------------------
/*
	this class provides a tTJSBinaryStream based access method for a memory block.
*/
class tTVPMemoryStream : public tTJSBinaryStream
{
protected:
	void * Block;
	bool Reference;
	tjs_uint Size;
	tjs_uint AllocSize;
	tjs_uint CurrentPos;

public:
	tTVPMemoryStream();
	tTVPMemoryStream(const void * block, tjs_uint size);
	~tTVPMemoryStream();

	tjs_uint64 TJS_INTF_METHOD Seek(tjs_int64 offset, tjs_int whence);

	tjs_uint TJS_INTF_METHOD Read(void *buffer, tjs_uint read_size);
	tjs_uint TJS_INTF_METHOD Write(const void *buffer, tjs_uint write_size);
	void TJS_INTF_METHOD SetEndOfStorage();

	tjs_uint64 TJS_INTF_METHOD GetSize() { return Size; }

	// non-tTJSBinaryStream based methods
	void * GetInternalBuffer()  const { return Block; }
	void Clear(void);
	void SetSize(tjs_uint size);

protected:
	void Init();

protected:
	virtual void * Alloc(size_t size);
	virtual void * Realloc(void *orgblock, size_t size);
	virtual void Free(void *block);
};
//---------------------------------------------------------------------------





//---------------------------------------------------------------------------
// tTVPPartialStream
//---------------------------------------------------------------------------
/*
	this class offers read-only accesses for a partial of the other stream,
	limited by given start position for original stream and given limited size.
*/
//---------------------------------------------------------------------------
class tTVPPartialStream : public tTJSBinaryStream
{
private:
	tTJSBinaryStream *Stream;
	tjs_uint64 Start;
	tjs_uint64 Size;
	tjs_uint64 CurrentPos;

public:
	tTVPPartialStream(tTJSBinaryStream *stream, tjs_uint64 start, tjs_uint64 size);
	~tTVPPartialStream();

	tjs_uint64 TJS_INTF_METHOD Seek(tjs_int64 offset, tjs_int whence);
	tjs_uint TJS_INTF_METHOD Read(void *buffer, tjs_uint read_size);
	tjs_uint TJS_INTF_METHOD Write(const void *buffer, tjs_uint write_size);

	// void SetEndOfStorage(); // use default behavior

	tjs_uint64 TJS_INTF_METHOD GetSize();

};
//---------------------------------------------------------------------------
class tTVPUnpackArchiveImpl;
class tTVPUnpackArchive {
public:
	tTVPUnpackArchive();
	virtual ~tTVPUnpackArchive(); // must ve deconstructed from main thread
	int Prepare(const std::string &path, const std::string &_outpath, tjs_uint64 *totalSize);
	void Start();
	void Stop();

	void SetCallback(
		const std::function<void()> &funcOnEnded,
		const std::function<void(int, const char *)> &funcOnError,
		const std::function<void(tjs_uint64, tjs_uint64)> &funcOnProgress,
		const std::function<void(int, const char*, tjs_uint64)> &funcOnNewFile) {
		FuncOnEnded = funcOnEnded;
		FuncOnError = funcOnError;
		FuncOnProgress = funcOnProgress;
		FuncOnNewFile = funcOnNewFile;
	}

protected:
	// these callbacks are not in main thread !
	virtual void OnEnded() {
		if (FuncOnEnded)
			FuncOnEnded();
	}
	virtual void OnError(int err, const char *msg) {
		if (FuncOnError)
			FuncOnError(err, msg);
	}
	virtual void OnProgress(tjs_uint64 total_size, tjs_uint64 file_size) {
		if (FuncOnProgress)
			FuncOnProgress(total_size, file_size);
	}
	virtual void OnNewFile(int idx, const char * utf8name, tjs_uint64 file_size) {
		if (FuncOnNewFile)
			FuncOnNewFile(idx, utf8name, file_size);
	}

private:
	void Process();
	FILE *FpIn = nullptr;
	struct archive *ArcObj = nullptr;
	std::string OutPath;
	friend class tTVPUnpackArchiveImpl;
	tTVPUnpackArchiveImpl *Impl = nullptr;
	tTVPArchive *pTVPArc = nullptr;
	bool StopRequired = false;

	std::function<void()> FuncOnEnded;
	std::function<void(int , const char *)> FuncOnError;
	std::function<void(tjs_uint64 , tjs_uint64 )> FuncOnProgress;
	std::function<void(int , const char * , tjs_uint64 )> FuncOnNewFile;
};

#endif
