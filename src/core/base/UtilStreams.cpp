//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Stream related utilities / utility streams
//---------------------------------------------------------------------------
#include "tjsCommHead.h"

#include "UtilStreams.h"
#include "MsgIntf.h"
#include "DebugIntf.h"
#include "EventIntf.h"
#include "StorageIntf.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include "Platform.h"



//---------------------------------------------------------------------------
// tTVPLocalTempStorageHolder
//---------------------------------------------------------------------------
#define TVP_LOCAL_TEMP_COPY_BLOCK_SIZE 65536*2
tTVPLocalTempStorageHolder::tTVPLocalTempStorageHolder(const ttstr & name)
{
	// name must be normalized !!!

	FileMustBeDeleted = false;
	FolderMustBeDeleted = false;
	LocalName = TVPGetLocallyAccessibleName(name);
	if(LocalName.IsEmpty())
	{
		// file must be copied to local filesystem

		// note that the basename is much more important than the directory
		// which the file is to be in, so we create a temporary folder and
		// store the file in it.

		LocalFolder = TVPGetTemporaryName();
		LocalName = LocalFolder + TJS_W("/") + TVPExtractStorageName(name);
		TVPCreateFolders(LocalFolder); // create temporary folder
		FolderMustBeDeleted = true;
		FileMustBeDeleted = true;

		try
		{
			// copy to local file
			tTVPStreamHolder src(name);
			tTVPStreamHolder dest(LocalName, TJS_BS_WRITE|TJS_BS_DELETE_ON_CLOSE);
			tjs_uint8 * buffer = new tjs_uint8[TVP_LOCAL_TEMP_COPY_BLOCK_SIZE];
			try
			{
				tjs_uint read;
				while(true)
				{
					read = src->Read(buffer, TVP_LOCAL_TEMP_COPY_BLOCK_SIZE);
					if(read == 0) break;
					dest->WriteBuffer(buffer, read);
				}
			}
			catch(...)
			{
				delete [] buffer;
				throw;
			}
			delete [] buffer;
		}
		catch(...)
		{
			if(FileMustBeDeleted) TVPRemoveFile(LocalName);
			if(FolderMustBeDeleted) TVPRemoveFolder(LocalFolder);
			throw;
		}
	}
}
//---------------------------------------------------------------------------
tTVPLocalTempStorageHolder::~tTVPLocalTempStorageHolder()
{
	if(FileMustBeDeleted) TVPRemoveFile(LocalName);
	if(FolderMustBeDeleted) TVPRemoveFolder(LocalFolder);
}
//---------------------------------------------------------------------------







//---------------------------------------------------------------------------
// tTVPMemoryStream
//---------------------------------------------------------------------------
tTVPMemoryStream::tTVPMemoryStream()
{
	Init();
}
//---------------------------------------------------------------------------
tTVPMemoryStream::tTVPMemoryStream(const void * block, tjs_uint size)
{
	Init();
	Block = (void*)block;
	if(!Block)
	{
		Block = Alloc(size);
		if(!Block) TVPThrowExceptionMessage(TVPInsufficientMemory);
	}
	else
	{
		Reference = true; // memory block was given
	}
	Size = size;
	AllocSize = size;
	CurrentPos = 0;
}
//---------------------------------------------------------------------------
tTVPMemoryStream::~tTVPMemoryStream()
{
	if(Block && !Reference) Free(Block);
}
//---------------------------------------------------------------------------
tjs_uint64 TJS_INTF_METHOD tTVPMemoryStream::Seek(tjs_int64 offset, tjs_int whence)
{
	tjs_int64 newpos;
	switch(whence)
	{
	case TJS_BS_SEEK_SET:
		if(offset >= 0)
		{
			if(offset <= Size) CurrentPos = static_cast<tjs_uint>(offset);
		}
		return CurrentPos;

	case TJS_BS_SEEK_CUR:
		if((newpos = offset + (tjs_int64)CurrentPos) >= 0)
		{
			tjs_uint np = (tjs_uint)newpos;
			if(np <= Size) CurrentPos = np;
		}
		return CurrentPos;

	case TJS_BS_SEEK_END:
		if((newpos = offset + (tjs_int64)Size) >= 0)
		{
			tjs_uint np = (tjs_uint)newpos;
			if(np <= Size) CurrentPos = np;
		}
		return CurrentPos;
	}
	return CurrentPos;
}
//---------------------------------------------------------------------------
tjs_uint TJS_INTF_METHOD tTVPMemoryStream::Read(void *buffer, tjs_uint read_size)
{
	if(CurrentPos + read_size >= Size)
	{
		read_size = Size - CurrentPos;
	}

	memcpy(buffer, (tjs_uint8*)Block + CurrentPos, read_size);

	CurrentPos += read_size;

	return read_size;
}
//---------------------------------------------------------------------------
tjs_uint TJS_INTF_METHOD tTVPMemoryStream::Write(const void *buffer, tjs_uint write_size)
{
	// writing may increase the internal buffer size.
	if(Reference) TVPThrowExceptionMessage(TVPWriteError);

	tjs_uint newpos = CurrentPos + write_size;
	if(newpos >= AllocSize)
	{
		// exceeds AllocSize
		tjs_uint onesize;
		if(AllocSize < 64*1024) onesize = 4*1024;
		else if(AllocSize < 512*1024) onesize = 16*1024;
		else if(AllocSize < 4096*1024) onesize = 256*1024;
		else onesize = 2024*1024;
		AllocSize += onesize;

		if(CurrentPos + write_size >= AllocSize) // still insufficient ?
		{
			AllocSize = CurrentPos + write_size;
		}

		Block = Realloc(Block, AllocSize);

		if(AllocSize && !Block)
			TVPThrowExceptionMessage(TVPInsufficientMemory);
			// this exception cannot be repaird; a fatal error.
	}

	memcpy((tjs_uint8*)Block + CurrentPos, buffer, write_size);

	CurrentPos = newpos;

	if(CurrentPos > Size) Size = CurrentPos;

	return write_size;
}
//---------------------------------------------------------------------------
void TJS_INTF_METHOD tTVPMemoryStream::SetEndOfStorage()
{
	if(Reference) TVPThrowExceptionMessage(TVPWriteError);

	Size = CurrentPos;
	AllocSize = Size;
	Block = Realloc(Block, Size);
	if(Size && !Block)
		TVPThrowExceptionMessage(TVPInsufficientMemory);
}
//---------------------------------------------------------------------------
void tTVPMemoryStream::Clear(void)
{
	if(Block && !Reference) Free(Block);
	Init();
}
//---------------------------------------------------------------------------
void tTVPMemoryStream::SetSize(tjs_uint size)
{
	if(Reference) TVPThrowExceptionMessage(TVPWriteError);

	if(Size > size)
	{
		// decrease
		Size = size;
		AllocSize = size;
		Block = Realloc(Block, size);
		if(CurrentPos > Size) CurrentPos = Size;
		if(size && !Block)
			TVPThrowExceptionMessage(TVPInsufficientMemory);
	}
	else
	{
		// increase
		AllocSize = size;
		Size = size;
		Block = Realloc(Block, size);
		if(size && !Block)
			TVPThrowExceptionMessage(TVPInsufficientMemory);

	}
}
//---------------------------------------------------------------------------
void tTVPMemoryStream::Init()
{
	Block = NULL;
	Reference = false;
	Size = 0;
	AllocSize = 0;
	CurrentPos = 0;
}
//---------------------------------------------------------------------------
void * tTVPMemoryStream::Alloc(size_t size)
{
	return TJS_malloc(size);
}
//---------------------------------------------------------------------------
void * tTVPMemoryStream::Realloc(void *orgblock, size_t size)
{
	return TJS_realloc(orgblock, size);
}
//---------------------------------------------------------------------------
void tTVPMemoryStream::Free(void *block)
{
	TJS_free(block);
}
//---------------------------------------------------------------------------





//---------------------------------------------------------------------------
// tTVPPartialStream
//---------------------------------------------------------------------------
tTVPPartialStream::tTVPPartialStream(tTJSBinaryStream *stream, tjs_uint64 start,
	tjs_uint64 size)
{
	// the stream given as a argument here will be owned by this instance,
	// and freed at the destruction.

	Stream = stream;
	Start = start;
	Size = size;
	CurrentPos = 0;

	try
	{
		Stream->SetPosition(Start);
	}
	catch(...)
	{
		delete Stream;
		Stream = NULL;
		throw;
	}
}
//---------------------------------------------------------------------------
tTVPPartialStream::~tTVPPartialStream()
{
	if(Stream) delete Stream;
}
//---------------------------------------------------------------------------
tjs_uint64 TJS_INTF_METHOD tTVPPartialStream::Seek(tjs_int64 offset, tjs_int whence)
{
	tjs_int64 newpos;
	switch(whence)
	{
	case TJS_BS_SEEK_SET:
		newpos = offset;
		if(offset >= 0 && offset <= static_cast<tjs_int64>(Size) )
		{
			newpos += Start;
			CurrentPos = Stream->Seek(newpos, TJS_BS_SEEK_SET) - Start;
		}
		return CurrentPos;

	case TJS_BS_SEEK_CUR:
		newpos = offset + CurrentPos;
		if(offset >= 0 && offset <= static_cast<tjs_int64>(Size) )
		{
			newpos += Start;
			CurrentPos = Stream->Seek(newpos, TJS_BS_SEEK_SET) - Start;
		}
		return CurrentPos;

	case TJS_BS_SEEK_END:
		newpos = offset + Size;
		if(offset >= 0 && offset <= static_cast<tjs_int64>(Size) )
		{
			newpos += Start;
			CurrentPos = Stream->Seek(newpos, TJS_BS_SEEK_SET) - Start;
		}
		return CurrentPos;
	}
	return CurrentPos;
}
//---------------------------------------------------------------------------
tjs_uint TJS_INTF_METHOD tTVPPartialStream::Read(void *buffer, tjs_uint read_size)
{
	if(CurrentPos + read_size >= Size)
	{
		read_size = static_cast<tjs_uint>(Size - CurrentPos);
	}

	tjs_uint read = Stream->Read(buffer, read_size);

	CurrentPos += read;

	return read;
}
//---------------------------------------------------------------------------
tjs_uint TJS_INTF_METHOD tTVPPartialStream::Write(const void *buffer, tjs_uint write_size)
{
	return 0;
}
//---------------------------------------------------------------------------
tjs_uint64 TJS_INTF_METHOD tTVPPartialStream::GetSize()
{
	return Size;
}
//---------------------------------------------------------------------------



extern "C" {
#include "libarchive/archive.h"
#include "libarchive/archive_entry.h"
}
#if 0
class LibArchive_Archive : public tTVPArchive {
	struct archive *_arc;
	tTJSBinaryStream *_stream;
	static const int BUFFER_SIZE = 16 * 1024;
	tjs_uint8 *_buffer = new tjs_uint8[BUFFER_SIZE];
	std::vector<std::pair<ttstr, struct archive_entry *> > _filelist;

	void Clear() {
		if (_arc) {
			archive_read_free(_arc);
			_arc = nullptr;
		}
		for (std::pair<ttstr, struct archive_entry *> &it : _filelist) {
			archive_entry_free(it.second);
		}
		_filelist.clear();
	}

public:
	LibArchive_Archive(const ttstr & name, tTJSBinaryStream *st) : tTVPArchive(name), _stream(st) {
		_arc = archive_read_new();
	}
	~LibArchive_Archive() {
		Clear();
		if (_stream)
			delete _stream;
		if (_buffer)
			delete[] _buffer;
	}

	virtual tjs_uint GetCount() { return _filelist.size(); }
	virtual ttstr GetName(tjs_uint idx) { return _filelist[idx].first; }
	virtual tTJSBinaryStream * CreateStreamByIndex(tjs_uint idx) {
		struct archive_entry * entry = _filelist[idx].second;
		tjs_uint64 fileSize = archive_entry_size(entry);
		if (fileSize <= 0) return new tTVPMemoryStream();
		return nullptr;
	}

	bool Open(bool normalizeFileName) {
		archive_read_support_filter_all(_arc);
		archive_read_support_format_all(_arc);
		archive_read_set_seek_callback(_arc, seek_callback);
		archive_read_open2(_arc, this, nullptr, read_callback, nullptr, nullptr);

		struct archive_entry *entry = archive_entry_new2(_arc);
		while (archive_read_next_header2(_arc, entry) == ARCHIVE_OK) {
			ttstr filename(archive_entry_pathname_utf8(entry));
			if (normalizeFileName)
				NormalizeInArchiveStorageName(filename);
			_filelist.emplace_back(filename, entry);
			entry = archive_entry_new2(_arc);
		}
		archive_entry_free(entry);
		if (normalizeFileName) {
			std::sort(_filelist.begin(), _filelist.end(), [](const std::pair<ttstr, struct archive_entry*>& a, const std::pair<ttstr, struct archive_entry*>& b) {
				return a.first < b.first;
			});
		}
	}

	void Detach() {
		Clear();
		_stream = nullptr;
	}

	static la_ssize_t read_callback(struct archive *, void *_client_data, const void **_buffer) {
		LibArchive_Archive *_this = (LibArchive_Archive *)_client_data;
		*_buffer = _this->_buffer;
		return _this->_stream->Read(_this->_buffer, BUFFER_SIZE);
	}

	static la_int64_t seek_callback(struct archive *, void *_client_data, la_int64_t offset, int whence) {
		LibArchive_Archive *_this = (LibArchive_Archive *)_client_data;
		return _this->_stream->Seek(offset, whence);
	}

	static la_int64_t skip_callback(struct archive *, void *_client_data, la_int64_t request) {
		LibArchive_Archive *_this = (LibArchive_Archive *)_client_data;
		return _this->_stream->Seek(request, TJS_BS_SEEK_CUR);
	}
};

tTVPArchive *TVPOpenLibArchive(const ttstr & name, tTJSBinaryStream *st, bool normalizeFileName) {
	LibArchive_Archive *arc = new LibArchive_Archive(name, st);
	if (arc->Open(normalizeFileName)) {
		return arc;
	}
	arc->Detach();
	delete arc;
	return nullptr;
}
#endif

static FILE *_fileopen(ttstr path) {
	std::string strpath = path.AsStdString();
	FILE *fp = fopen(strpath.c_str(), "wb");
	if (!fp) { // make dirs
		path = TVPExtractStoragePath(path);
		TVPCreateFolders(path);
		fp = fopen(strpath.c_str(), "wb");
	}
	return fp;
}

class tTVPUnpackArchiveThread {
	std::thread *ThreadObj;
	std::mutex Mutex;
	std::condition_variable Cond;
	tTVPUnpackArchive *Owner;

	void Entry() {
		{
			std::unique_lock<std::mutex> lk(Mutex);
			Cond.wait(lk);
		}
		Owner->Process();
	}

public:
	tTVPUnpackArchiveThread(tTVPUnpackArchive *owner) : Owner(owner) {
		ThreadObj = new std::thread(&tTVPUnpackArchiveThread::Entry, this);
	}

	~tTVPUnpackArchiveThread() {
		if (ThreadObj->joinable()) {
			ThreadObj->join();
		}
		delete ThreadObj;
	}

	void Start() {
		Cond.notify_all();
	}
};

class tTVPUnpackArchiveImplLibArchive : public iTVPUnpackArchiveImpl {
	struct archive* ArcObj = nullptr;
	tTVPArchive *pTVPArc = nullptr;
	tjs_int64 _totalSize = 0;
	int _totalFileCount = 0;
	FILE *FpIn = nullptr;

	static const char * _onPassphraseCallback(struct archive *, void *clientdata);
	std::string onPassphraseCallback();
public:
	virtual ~tTVPUnpackArchiveImplLibArchive() {
		if (pTVPArc)
			pTVPArc->Release();
		if (FpIn) {
			fclose(FpIn);
			FpIn = nullptr;
		}
		if (ArcObj) {
			archive_read_free(ArcObj);
			ArcObj = nullptr;
		}
	}

	virtual bool Open(const std::string &path) {
		FpIn = fopen(path.c_str(), "rb");
		int file_count = 0;
		tjs_uint64 size_count = 0;
		ArcObj = archive_read_new();
		archive_read_support_filter_all(ArcObj);
		archive_read_support_format_all(ArcObj);
		archive_read_set_passphrase_callback(ArcObj, this, _onPassphraseCallback);
		int r = archive_read_open_FILE(ArcObj, FpIn);

		if (r < ARCHIVE_OK) {
			fclose(FpIn);
			FpIn = nullptr;
			archive_read_free(ArcObj);
			ArcObj = nullptr;
			// try TVPArchive
			pTVPArc = TVPOpenArchive(path, false);
			if (pTVPArc) {
				file_count = pTVPArc->GetCount();
				_totalSize = 0;
				for (int i = 0; i < file_count; ++i) {
					tTJSBinaryStream *str = pTVPArc->CreateStreamByIndex(i);
					if (str) {
						_totalSize += str->GetSize();
						delete str;
					}
				}
				_totalFileCount = file_count;
				return true;
			}
			return false;
		}
		if (archive_read_has_encrypted_entries(ArcObj) > 0) {
			std::string psw = onPassphraseCallback();
			if (psw.empty()) {
				return false;
			}
			archive_read_add_passphrase(ArcObj, psw.c_str());
		}
		r = 0;
		while (true) {
			struct archive_entry *entry;
			int r = archive_read_next_header(ArcObj, &entry);
			if (r == ARCHIVE_EOF) {
				r = ARCHIVE_OK;
				break;
			}
			if (r < ARCHIVE_OK)
				_callbacks->FuncOnError(r, archive_error_string(ArcObj));
			if (r < ARCHIVE_WARN)
				break;
			++file_count;
			size_count += archive_entry_size(entry);
		}

		_totalSize = size_count;
		_totalFileCount = file_count;
		archive_read_close(ArcObj);
		archive_read_free(ArcObj);
		ArcObj = nullptr;
		return true;
	}

	virtual int GetFileCount() override { return _totalFileCount; }
	virtual tjs_int64 GetTotalSize() override { return _totalSize; }

	virtual void ExtractTo(const std::string &OutPath) {
		tjs_uint64 total_size = 0;
		if (pTVPArc) {
			int file_count = pTVPArc->GetCount();
			std::vector<char> buffer; buffer.resize(4 * 1024 * 1024);
			for (int index = 0; index < file_count && !StopRequired; ++index) {
				tjs_uint64 file_size = 0;
				std::string filename = pTVPArc->GetName(index).AsStdString();
				if (filename.size() > 600) continue;
				std::string fullpath = OutPath + filename;
				FILE *fp = _fileopen(fullpath);
				if (!fp) {
					_callbacks->FuncOnError(ARCHIVE_FAILED, "Cannot open output file");
					break;
				}
				tTJSBinaryStream *str = pTVPArc->CreateStreamByIndex(index);
				if (!str) {
					_callbacks->FuncOnError(ARCHIVE_FAILED, "Cannot open archive stream");
					fclose(fp);
					break;
				}
				_callbacks->FuncOnNewFile(index, filename.c_str(), str->GetSize());
				while (!StopRequired) {
					tjs_uint readed = str->Read(&buffer.front(), buffer.size());
					if (readed == 0) break;
					if (readed != fwrite(&buffer.front(), 1, readed, fp)) {
						_callbacks->FuncOnError(ARCHIVE_FAILED, "Fail to write file.\nPlease check the disk space.");
						break;
					}
					file_size += readed;
					total_size += readed;
					_callbacks->FuncOnProgress(total_size, file_size);
					if (readed < buffer.size())
						break;
				}
				delete str;
				fclose(fp);
			}
			pTVPArc->Release();
			pTVPArc = nullptr;
			_callbacks->FuncOnEnded();
			return;
		}

		fseek(FpIn, 0, SEEK_SET);
		ArcObj = archive_read_new();
		archive_read_support_filter_all(ArcObj);
		archive_read_support_format_all(ArcObj);
		int r = archive_read_open_FILE(ArcObj, FpIn);
		if (r < ARCHIVE_OK) {
			_callbacks->FuncOnError(r, archive_error_string(ArcObj));
			_callbacks->FuncOnEnded();
			return;
		}
		for (int index = 0; !StopRequired; ++index) {
			struct archive_entry *entry;
			int r = archive_read_next_header(ArcObj, &entry);
			if (r == ARCHIVE_EOF) {
				r = ARCHIVE_OK;
				break;
			}
			if (r < ARCHIVE_OK)
				_callbacks->FuncOnError(r, archive_error_string(ArcObj));
			if (r < ARCHIVE_WARN)
				break;
			const char *filename = archive_entry_pathname_utf8(entry);
			std::string fullpath = OutPath + filename;
			FILE *fp = _fileopen(fullpath);
			if (!fp) {
				_callbacks->FuncOnError(ARCHIVE_FAILED, "Cannot open output file");
				break;
			}
			_callbacks->FuncOnNewFile(index, filename, archive_entry_size(entry));

			const void *buff;
			size_t size;
			la_int64_t offset;
			tjs_uint64 file_size = 0;
			const char *errmsg;
			while (!StopRequired) {
				r = archive_read_data_block(ArcObj, &buff, &size, &offset);
				if (r == ARCHIVE_EOF) {
					r = ARCHIVE_OK;
					break;
				}
				if (r < ARCHIVE_OK) {
					errmsg = archive_error_string(ArcObj);
					break;
				}
				if (size != fwrite(buff, 1, size, fp)) {
					r = ARCHIVE_FAILED;
					errmsg = "Fail to write file.\nPlease check the disk space.";
					break;
				}
				file_size += size;
				total_size += size;
				_callbacks->FuncOnProgress(total_size, file_size);
			}
			fclose(fp);
			if (r < ARCHIVE_OK)
				_callbacks->FuncOnError(r, errmsg);
			if (r < ARCHIVE_WARN)
				break;
			if (archive_entry_mtime_is_set(entry)) {
				TVP_utime(fullpath.c_str(), archive_entry_mtime(entry));
			}
		}
		archive_read_close(ArcObj);
		_callbacks->FuncOnEnded();
	}
};

std::string tTVPUnpackArchiveImplLibArchive::onPassphraseCallback()
{
	return _callbacks->FuncPassword();
}

const char * tTVPUnpackArchiveImplLibArchive::_onPassphraseCallback(struct archive *, void *clientdata)
{
	static std::string psw = ((tTVPUnpackArchiveImplLibArchive*)clientdata)->onPassphraseCallback();
	return psw.c_str();
}
#if 1
#include "unrar/raros.hpp"
#include "unrar/dll.hpp"
class tTVPUnpackArchiveImplUnRAR : public iTVPUnpackArchiveImpl {
	std::string _archivePath;
	std::vector<std::string> _filelist;
	tjs_int64 _totalSize, _totalProcessedBytes, _curProcessedBytes;
	std::string _lastUsedPassword;
	std::mutex _mutex;
	std::condition_variable _cond;
	
	bool _reqBreak = false;
	struct RARArc {
		RAROpenArchiveDataEx _archiveData;
		void *_handle = nullptr;
		RARArc() {}
		~RARArc() {
			Close();
		}
		bool Open(char *path, int mode) {
			memset(&_archiveData, 0, sizeof(_archiveData));
			_archiveData.ArcName = path;
			_archiveData.OpenMode = mode;
			_handle = RAROpenArchiveEx(&_archiveData);
			return !!_handle;
		}
		void Close() {
			if (_handle) RARCloseArchive(_handle);
			_handle = nullptr;
		}
	};
	int OnCallback(UINT msg, LPARAM P1, LPARAM P2) {
		switch (msg) {
		case UCM_CHANGEVOLUME:
		case UCM_CHANGEVOLUMEW:
			return -1; // manual change multi-volume file name is not supported yet
		case UCM_NEEDPASSWORD: {
			bool hasPsw = !_lastUsedPassword.empty();
			if (!hasPsw) {
				_lastUsedPassword = _callbacks->FuncPassword();
			}
			if (_lastUsedPassword.empty()) {
				return -1;
			}
			int len = _lastUsedPassword.size();
			if (len > P2) len = P2;
			strncpy((char*)P1, _lastUsedPassword.c_str(), len);
			if (hasPsw)
				_lastUsedPassword.clear();
			return 0;
		}
		case UCM_PROCESSDATA:
			_totalProcessedBytes += P2;
			_curProcessedBytes += P2;
			_callbacks->FuncOnProgress(_totalProcessedBytes, _curProcessedBytes);
			return _reqBreak ? -1 : 0;
		}
		return -1;
	}
public:
	virtual ~tTVPUnpackArchiveImplUnRAR() {
	}
	virtual bool Open(const std::string &path) override {
		_archivePath = path;
		RARArc arc;
		if (!arc.Open((char*)_archivePath.c_str(), RAR_OM_LIST)) {
			return false;
		}
		RARSetCallback(arc._handle, [](UINT msg, LPARAM UserData, LPARAM P1, LPARAM P2)->int {
			return ((tTVPUnpackArchiveImplUnRAR*)UserData)->OnCallback(msg, P1, P2);
		}, (LPARAM)this);
		
		arc._handle;
		RARHeaderData headerData;
		_totalSize = 0;
		_filelist.clear();
		while (1) {
			RARHeaderDataEx headerData;
			memset(&headerData, 0, sizeof(headerData));
			int result = RARReadHeaderEx(arc._handle, &headerData);
			if (result != 0) {
				if (result != ERAR_END_ARCHIVE) {
					return false;
				}
				break;
			}

			_totalSize += (headerData.UnpSizeHigh << 32) | headerData.UnpSize;
			_filelist.emplace_back(headerData.FileName);
			// Find next file
			result = RARProcessFile(arc._handle, RAR_SKIP, NULL, NULL);
			if (result != 0) {
				return false;
			}
		}
		return true;
	}
	virtual int GetFileCount() override { return _filelist.size(); }
	virtual tjs_int64 GetTotalSize() override { return _totalSize; }
	virtual void ExtractTo(const std::string &path) override {
		RARArc arc;
		if (!arc.Open((char*)_archivePath.c_str(), RAR_OM_EXTRACT)) {
			_callbacks->FuncOnError(1001, "Cannot open file");
			return;
		}
		RARSetCallback(arc._handle, [](UINT msg, LPARAM UserData, LPARAM P1, LPARAM P2)->int {
			return ((tTVPUnpackArchiveImplUnRAR*)UserData)->OnCallback(msg, P1, P2);
		}, (LPARAM)this);
		RARHeaderData headerData;
		
		for (int counter = 0; ;++counter) {
			RARHeaderDataEx headerData;
			memset(&headerData, 0, sizeof(headerData));
			int result = RARReadHeaderEx(arc._handle, &headerData);
			if (result != 0) {
				if (result != ERAR_END_ARCHIVE) {
					_callbacks->FuncOnError(result, "Extraction Fail");
					return;
				}
				break;
			}

			// _filelist.emplace_back(headerData.FileName);
			_callbacks->FuncOnNewFile(counter, headerData.FileName, (headerData.UnpSizeHigh << 32) | headerData.UnpSize);
			_curProcessedBytes = 0;
			// Find next file
			result = RARProcessFile(arc._handle, RAR_EXTRACT, (char*)path.c_str(), NULL);
			if (result != 0) {
				_callbacks->FuncOnError(result, "Extraction Fail");
				return;
			}
		}

		_callbacks->FuncOnEnded();
	}
};
#endif
int tTVPUnpackArchive::Prepare(const std::string &path, const std::string &_outpath, tjs_uint64 *totalSize) {
	FILE *FpIn = fopen(path.c_str(), "rb");
	if (!FpIn) return -1;
	char signature[4];
	if (fread(signature, 1, 4, FpIn) != 4) {
		fclose(FpIn);
		return -2;
	}
	OutPath = _outpath + "/";
	fclose(FpIn);
#ifdef _UNRAR_DLL_
	if (!memcmp(signature, "Rar!", 4)) {
		_impl = new tTVPUnpackArchiveImplUnRAR();
	} else
#endif
	{
		_impl = new tTVPUnpackArchiveImplLibArchive();
	}
	_impl->SetCallback(this);
	if (!_impl->Open(path)) {
		Close();
		return -2;
	}
	if (totalSize) *totalSize = _impl->GetTotalSize();
	int file_count = _impl->GetFileCount();
	if (file_count) {
		ArcThread = new tTVPUnpackArchiveThread(this);
	}
	return file_count;
}

void tTVPUnpackArchive::Start()
{
	_impl->StopRequired = false;
	ArcThread->Start();
}

void tTVPUnpackArchive::Stop()
{
	_impl->StopRequired = true;
	ArcThread->Start();
}

void tTVPUnpackArchive::Close()
{
	if (_impl) delete _impl;
	_impl = nullptr;
}

void tTVPUnpackArchive::Process()
{
	if (_impl->StopRequired)
		return;
	_impl->ExtractTo(OutPath);
}

tTVPUnpackArchive::tTVPUnpackArchive()
{
}

tTVPUnpackArchive::~tTVPUnpackArchive()
{
	if (ArcThread) delete ArcThread;
}
