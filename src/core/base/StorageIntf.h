//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Universal Storage System
//---------------------------------------------------------------------------
#ifndef StorageIntfH
#define StorageIntfH

#include "tjsNative.h"
#include "tjsHashSearch.h"
#include <vector>

//---------------------------------------------------------------------------
// archive delimiter
//---------------------------------------------------------------------------
extern tjs_char  TVPArchiveDelimiter; //  = '>';



//---------------------------------------------------------------------------
// utilities
//---------------------------------------------------------------------------
ttstr TVPStringFromBMPUnicode(const tjs_uint16 *src, tjs_int maxlen = -1);
//---------------------------------------------------------------------------




//---------------------------------------------------------------------------
// tTVPArchive base archive class
//---------------------------------------------------------------------------
class tTVPArchive
{
private:
	tjs_uint RefCount;

public:
	//-- constructor
	tTVPArchive(const ttstr & name)
		{ ArchiveName = name; Init = false; RefCount = 1; }
	virtual ~tTVPArchive() { ; }

	//-- AddRef and Release
	void AddRef() { RefCount++; }
	void Release() { if(RefCount == 1) delete this; else RefCount--; }

	//-- must be implemented by delivered class
	virtual tjs_uint GetCount() = 0;
	virtual ttstr GetName(tjs_uint idx) = 0;
		// returned name must be already normalized using NormalizeInArchiveStorageName
		// and the index must be sorted by its name, using ttstr::operator < .
		// this is needed by fast directory search.

	virtual tTJSBinaryStream * CreateStreamByIndex(tjs_uint idx) = 0;

	//-- others, implemented in this class
private:

	tTJSHashTable<ttstr, tjs_uint, tTJSHashFunc<ttstr>, 1024> Hash;
	bool Init;

public:
	ttstr ArchiveName;

public:
	static void NormalizeInArchiveStorageName(ttstr & name);

private:
	void AddToHash();
public:
	tTJSBinaryStream * CreateStream(const ttstr & name);
	bool IsExistent(const ttstr & name);

	tjs_int GetFirstIndexStartsWith(const ttstr & prefix);
		// returns first index which have 'prefix' at start of the name.
};
//---------------------------------------------------------------------------




/*[*/
//---------------------------------------------------------------------------
// iTVPStorageMedia
//---------------------------------------------------------------------------
/*
	abstract class for managing media ( like file: http: etc.)
*/
/*]*/
#if 0
/*[*/
	// for plug-in
class tTJSBinaryStream;
/*]*/
#endif
/*[*/
//---------------------------------------------------------------------------
class iTVPStorageLister // callback class for GetListAt
{
public:
	virtual void TJS_INTF_METHOD Add(const ttstr &file) = 0;
};
//---------------------------------------------------------------------------
class iTVPStorageMedia
{
public:
	virtual ~iTVPStorageMedia() {} // add by ZeaS
	virtual void TJS_INTF_METHOD AddRef() = 0;
	virtual void TJS_INTF_METHOD Release() = 0;

	virtual void TJS_INTF_METHOD GetName(ttstr &name) = 0;
		// returns media name like "file", "http" etc.

//	virtual bool TJS_INTF_METHOD IsCaseSensitive() = 0;
		// returns whether this media is case sensitive or not

	virtual void TJS_INTF_METHOD NormalizeDomainName(ttstr &name) = 0;
		// normalize domain name according with the media's rule

	virtual void TJS_INTF_METHOD NormalizePathName(ttstr &name) = 0;
		// normalize path name according with the media's rule

	// "name" below is normalized but does not contain media, eg.
	// not "media://domain/path" but "domain/path"

	virtual bool TJS_INTF_METHOD CheckExistentStorage(const ttstr &name) = 0;
		// check file existence

	virtual tTJSBinaryStream * TJS_INTF_METHOD Open(const ttstr & name, tjs_uint32 flags) = 0;
		// open a storage and return a tTJSBinaryStream instance.
		// name does not contain in-archive storage name but
		// is normalized.

	virtual void TJS_INTF_METHOD GetListAt(const ttstr &name, iTVPStorageLister * lister) = 0;
		// list files at given place

	virtual void TJS_INTF_METHOD GetLocallyAccessibleName(ttstr &name) = 0;
		// basically the same as above,
		// check wether given name is easily accessible from local OS filesystem.
		// if true, returns local OS native name. otherwise returns an empty string.
};
//---------------------------------------------------------------------------
/*]*/



//---------------------------------------------------------------------------
// must be implemented in each platform
//---------------------------------------------------------------------------
extern tTVPArchive * TVPOpenArchive(const ttstr & name, bool normalizeFileName);
	// open archive and return tTVPArchive instance.

TJS_EXP_FUNC_DEF(ttstr, TVPGetTemporaryName, ());
	// retrieve file name to store temporary data ( must be unique, local name )

TJS_EXP_FUNC_DEF(ttstr, TVPGetAppPath, ());
	// retrieve program path, in normalized storage name

void TVPPreNormalizeStorageName(ttstr &name);
		// called by TVPNormalizeStorageName before it process the storage name.
		// user may pass the OS's native filename to the TVP storage system,
		// so that this function must convert it to the TVP storage name rules.

iTVPStorageMedia * TVPCreateFileMedia();
	// create basic default "file:" storage media
/*
extern void TVPPreNormalizeStorageName(ttstr &name);

extern tTJSBinaryStream * TVPOpenStream(const ttstr & name, tjs_uint32 flags);
	// open a storage and return a tTJSBinaryStream instance.
	// name does not contain in-archive storage name but
	// is normalized.

extern bool TVPCheckExistentStorage(const ttstr &name);
	// check file existence

extern void TVPGetStorageListAt(const ttstr &name, std::vector<ttstr> & list);

extern ttstr TVPGetMediaCurrent(const ttstr & name);
extern void TVPSetMediaCurrent(const ttstr & name, const ttstr & dir);

extern ttstr TVPGetNativeName(const ttstr &name);
	// retrieve OS native name

extern ttstr TVPGetLocallyAccessibleName(const ttstr &name);
	// check wether given name is easily accessible from local OS filesystem.
	// if true, returns local OS native name. otherwise returns an empty string.

*/
extern bool TVPRemoveFile(const ttstr &name);
	// remove local file ( "name" is a local *native* name )
	// this must not throw an exception ( return false if error )
extern bool TVPRemoveFolder(const ttstr &name);
	// remove local directory ( "name" is a local *native* name )
	// this must not throw an exception ( return false if error )
bool TVPCreateFolders(const ttstr &folder);
	// create folder along with the argument recursively (like mkdir -p).
	// 'folder' must be a local native name.

//---------------------------------------------------------------------------





//---------------------------------------------------------------------------
// implementation in this unit
//---------------------------------------------------------------------------
TJS_EXP_FUNC_DEF(void, TVPRegisterStorageMedia, (iTVPStorageMedia *media));
	// register storage media
TJS_EXP_FUNC_DEF(void, TVPUnregisterStorageMedia, (iTVPStorageMedia *media));
	// remove storage media


extern tTJSBinaryStream * TVPCreateStream(const ttstr & name, tjs_uint32 flags = 0);
	// open "name" and return tTJSBinaryStream instance.
	// name will be local storage, network storage, in-archive storage, etc...

TJS_EXP_FUNC_DEF(bool, TVPIsExistentStorageNoSearch, (const ttstr &name));
	// if "name" is exists, return true. otherwise return false.
	// this does not search any auto search path.

TJS_EXP_FUNC_DEF(bool, TVPIsExistentStorageNoSearchNoNormalize, (const ttstr &name));

TJS_EXP_FUNC_DEF(ttstr, TVPNormalizeStorageName, (const ttstr & name));

TJS_EXP_FUNC_DEF(void, TVPSetCurrentDirectory, (const ttstr & name));
	// set system current directory.
	// directory must end with path delimiter '/',
	// or archive delimiter '>'.

TJS_EXP_FUNC_DEF(void, TVPGetLocalName, (ttstr &name));

ttstr TVPGetLocallyAccessibleName(const ttstr &name);


TJS_EXP_FUNC_DEF(ttstr, TVPExtractStorageExt, (const ttstr & name));
	// extract "name"'s extension and return it.


TJS_EXP_FUNC_DEF(ttstr, TVPExtractStorageName, (const ttstr & name));
	// extract "name"'s storage name ( excluding path ) and return it.

TJS_EXP_FUNC_DEF(ttstr, TVPExtractStoragePath, (const ttstr & name));
	// extract "name"'s path ( including last delimiter ) and return it.

TJS_EXP_FUNC_DEF(ttstr, TVPChopStorageExt, (const ttstr & name));
	// chop storage's extension and return it.
	// extensition delimiter '.' will not be held.


TJS_EXP_FUNC_DEF(void, TVPAddAutoPath, (const ttstr & name));
	// add given path to auto search path

TJS_EXP_FUNC_DEF(void, TVPRemoveAutoPath, (const ttstr &name));
	// remove given path from auto search path

TJS_EXP_FUNC_DEF(ttstr, TVPGetPlacedPath, (const ttstr & name));
	// search path and return the path which the "name" is placed.

extern ttstr TVPSearchPlacedPath(const ttstr & name);
	// the same as TVPGetPlacedPath, except for rising exception when the storage
	// is not found.

TJS_EXP_FUNC_DEF(bool, TVPIsExistentStorage, (const ttstr &name));
	// if "name" is exists, return true. otherwise return false.
	// this searches auto search path.

TJS_EXP_FUNC_DEF(void, TVPClearStorageCaches, ());
	// clear all internal storage related caches.
void TVPRemoveFromStorageCache(const ttstr &name);
extern tjs_uint TVPSegmentCacheLimit; // XP3 segment cache limit, in bytes.

//---------------------------------------------------------------------------





//---------------------------------------------------------------------------
// tTJSNC_Storages : TJS Storages class
//---------------------------------------------------------------------------
class tTJSNC_Storages : public tTJSNativeClass
{
	typedef tTJSNativeClass inherited;

public:
	tTJSNC_Storages();
	static tjs_uint32 ClassID;

protected:
	tTJSNativeInstance *CreateNativeInstance();
};
//---------------------------------------------------------------------------
extern tTJSNativeClass * TVPCreateNativeClass_Storages();
//---------------------------------------------------------------------------

class tTVPStorageMedia : public iTVPStorageMedia {
protected:
	tjs_int refCount;
	tTVPStorageMedia() : refCount(1) {}

	virtual void TJS_INTF_METHOD AddRef() override { refCount++; }
	virtual void TJS_INTF_METHOD Release() override {
		if (refCount == 1) {
			delete this;
		} else {
			refCount--;
		}
	}
	virtual void TJS_INTF_METHOD NormalizeDomainName(ttstr &name) override {}
	virtual void TJS_INTF_METHOD NormalizePathName(ttstr &name) override {}
	virtual void TJS_INTF_METHOD GetLocallyAccessibleName(ttstr &name) override {}
};

class TArchiveStream : public tTJSBinaryStream {
	tTVPArchive *Owner;
	tjs_int64 CurrentPos;
	tjs_uint64 StartPos, DataLength;
	tTJSBinaryStream *_instr;

public:
	TArchiveStream(tTVPArchive *owner, tjs_uint64 off, tjs_uint64 len);
	virtual ~TArchiveStream();
	virtual tjs_uint64 TJS_INTF_METHOD Seek(tjs_int64 offset, tjs_int whence) {
		switch (whence) {
		case TJS_BS_SEEK_SET:
			CurrentPos = offset;
			break;

		case TJS_BS_SEEK_CUR:
			CurrentPos = offset + CurrentPos;
			break;

		case TJS_BS_SEEK_END:
			CurrentPos = offset + DataLength;
			break;
		}
		if (CurrentPos < 0) CurrentPos = 0;
		else if (CurrentPos > (tjs_int64)DataLength) CurrentPos = DataLength;
		_instr->SetPosition(CurrentPos + StartPos);
		return CurrentPos;
	}
	virtual tjs_uint TJS_INTF_METHOD Read(void *buffer, tjs_uint read_size) {
		if (CurrentPos + read_size >= (tjs_int64)DataLength) {
			read_size = (tjs_uint)(DataLength - CurrentPos);
		}

		_instr->ReadBuffer(buffer, read_size);

		CurrentPos += read_size;

		return read_size;
	}
	virtual tjs_uint TJS_INTF_METHOD Write(const void *buffer, tjs_uint write_size) {
		return 0;
	}
	virtual tjs_uint64 TJS_INTF_METHOD GetSize() {
		return DataLength;
	}
};
#endif
