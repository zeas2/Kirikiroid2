//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Universal Storage System
//---------------------------------------------------------------------------
#include "tjsCommHead.h"

// #include <cderr.h>
// #include <objbase.h>

#include "MsgIntf.h"

#include "StorageImpl.h"
#include "WindowImpl.h"
#include "GraphicsLoaderImpl.h"
#include "SysInitIntf.h"
#include "DebugIntf.h"
#include "Random.h"
#include "XP3Archive.h"
//#include "SusieArchive.h"
#include "FileSelector.h"
#include "Random.h"

#include <time.h>

#include "Application.h"
#include "StringUtil.h"
#include "FilePathUtil.h"
#include "Platform.h"
#include "platform/CCPlatformConfig.h"
#include "dirent.h"
#include "TickCount.h"
#include <fcntl.h>
#include <unistd.h>
#include "combase.h"
#include "win32io.h"

//---------------------------------------------------------------------------
// tTVPFileMedia
//---------------------------------------------------------------------------
class tTVPFileMedia : public iTVPStorageMedia
{
	tjs_uint RefCount;

public:
	tTVPFileMedia() { RefCount = 1; }
	~tTVPFileMedia() {;}

	void TJS_INTF_METHOD AddRef() { RefCount ++; }
	void TJS_INTF_METHOD Release()
	{
		if(RefCount == 1)
			delete this;
		else
			RefCount --;
	}

	void TJS_INTF_METHOD GetName(ttstr &name) { name = TJS_W("file"); }

	void TJS_INTF_METHOD NormalizeDomainName(ttstr &name);
	void TJS_INTF_METHOD NormalizePathName(ttstr &name);
	bool TJS_INTF_METHOD CheckExistentStorage(const ttstr &name);
	tTJSBinaryStream * TJS_INTF_METHOD Open(const ttstr & name, tjs_uint32 flags);
	void TJS_INTF_METHOD GetListAt(const ttstr &name, iTVPStorageLister *lister);
	void TJS_INTF_METHOD GetLocallyAccessibleName(ttstr &name);

public:
	void TJS_INTF_METHOD GetLocalName(ttstr &name);
};
//---------------------------------------------------------------------------
void TJS_INTF_METHOD tTVPFileMedia::NormalizeDomainName(ttstr &name)
{
	// normalize domain name
	// make all characters small
	tjs_char *p = name.Independ();
	while(*p)
	{
		if(*p >= TJS_W('A') && *p <= TJS_W('Z'))
			*p += TJS_W('a') - TJS_W('A');
		p++;
	}
}
//---------------------------------------------------------------------------
void TJS_INTF_METHOD tTVPFileMedia::NormalizePathName(ttstr &name)
{
	// normalize path name
	// make all characters small
	tjs_char *p = name.Independ();
	while(*p)
	{
		if(*p >= TJS_W('A') && *p <= TJS_W('Z'))
			*p += TJS_W('a') - TJS_W('A');
		p++;
	}
}
//---------------------------------------------------------------------------
bool TJS_INTF_METHOD tTVPFileMedia::CheckExistentStorage(const ttstr &name)
{
	if(name.IsEmpty()) return false;

	ttstr _name(name);
	GetLocalName(_name);

	return TVPCheckExistentLocalFile(_name);
}
//---------------------------------------------------------------------------
tTJSBinaryStream * TJS_INTF_METHOD tTVPFileMedia::Open(const ttstr & name, tjs_uint32 flags)
{
	// open storage named "name".
	// currently only local/network(by OS) storage systems are supported.
	if(name.IsEmpty())
		TVPThrowExceptionMessage(TVPCannotOpenStorage, TJS_W("\"\""));

	ttstr origname = name;
	ttstr _name(name);
	GetLocalName(_name);

	return new tTVPLocalFileStream(origname, _name, flags);
}

void TVPListDir(const std::string &folder, std::function<void(const std::string&, int)> cb) {
	DIR *dirp;
	struct dirent *direntp;
	tTVP_stat stat_buf;
	if ((dirp = opendir(folder.c_str())))
	{
		while ((direntp = readdir(dirp)) != NULL)
		{
			std::string fullpath = folder + "/" + direntp->d_name;
			if (!TVP_stat(fullpath.c_str(), stat_buf))
				continue;
			cb(direntp->d_name, stat_buf.st_mode);
		}
		closedir(dirp);
	}
}

void TVPGetLocalFileListAt(const ttstr &name, const std::function<void(const ttstr&, tTVPLocalFileInfo*)>& cb) {
	DIR *dirp;
	struct dirent *direntp;
	tTVP_stat stat_buf;
	std::string folder(name.AsNarrowStdString());
	if ((dirp = opendir(folder.c_str())))
	{
		while ((direntp = readdir(dirp)) != NULL)
		{
			std::string fullpath = folder + "/" + direntp->d_name;
			if (!TVP_stat(fullpath.c_str(), stat_buf))
				continue;
			ttstr file(direntp->d_name);
			if (file.length() <= 2) {
				if (file == TJS_W(".") || file == TJS_W(".."))
					continue;
			}
			tjs_char *p = file.Independ();
			while (*p)
			{
				// make all characters small
				if (*p >= TJS_W('A') && *p <= TJS_W('Z'))
					*p += TJS_W('a') - TJS_W('A');
				p++;
			}
			tTVPLocalFileInfo info;
			info.NativeName = direntp->d_name;
			info.Mode = stat_buf.st_mode;
			info.Size = stat_buf.st_size;
			info.AccessTime = stat_buf.st_atime;
			info.ModifyTime = stat_buf.st_mtime;
			info.CreationTime = stat_buf.st_ctime;
			cb(file, &info);
		}
		closedir(dirp);
	}
}
//---------------------------------------------------------------------------
void TJS_INTF_METHOD tTVPFileMedia::GetListAt(const ttstr &_name, iTVPStorageLister *lister)
{
	ttstr name(_name);
	GetLocalName(name);
#if 0
	name += TJS_W("*.*");

	// perform UNICODE operation
	WIN32_FIND_DATAW ffd;
	HANDLE handle = ::FindFirstFile(name.c_str(), &ffd);
	if(handle != INVALID_HANDLE_VALUE)
	{
		BOOL cont;
		do
		{
			if(!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			{
				ttstr file(ffd.cFileName);
				tjs_char *p = file.Independ();
				while(*p)
				{
					// make all characters small
					if(*p >= TJS_W('A') && *p <= TJS_W('Z'))
						*p += TJS_W('a') - TJS_W('A');
					p++;
				}
				lister->Add(file);
			}

			cont = ::FindNextFile(handle, &ffd);
		} while(cont);
		FindClose(handle);
	}
#endif
	TVPGetLocalFileListAt(name, [lister](const ttstr &name, tTVPLocalFileInfo* s) {
		if (s->Mode & (S_IFREG)) {
			lister->Add(name);
		}
	});
}

static int _utf8_strcasecmp(const char *a, const char *b) {
    for(; *a && *b; ++a, ++b) {
        int ca = *a, cb = *b;
        if('A' <= ca && ca <= 'Z') ca += 'a' - 'A';
        if('A' <= cb && cb <= 'Z') cb += 'a' - 'A';
        int ret = ca - cb;
        if(ret) return ret;
    }
    return *a - *b;
}

#if CC_TARGET_PLATFORM == CC_PLATFORM_IOS
const std::vector<std::string> &TVPGetApplicationHomeDirectory();
const std::vector<ttstr> &_getPrefixPath() {
	static std::vector<ttstr> ret;
	if (ret.empty()) {
		for (const std::string &path : TVPGetApplicationHomeDirectory()) {
			ret.emplace_back(path);
		}
	}
	return ret;
}
const std::vector<std::string> &_getHomeDir() {
	static std::vector<std::string> ret;
	if (ret.empty()) {
		for (const std::string &path : TVPGetApplicationHomeDirectory()) {
			ret.emplace_back(path + "/");
		}
	}
	return ret;
}
#endif

//---------------------------------------------------------------------------
void TJS_INTF_METHOD tTVPFileMedia::GetLocallyAccessibleName(ttstr &name)
{
	ttstr newname;

	const tjs_char *ptr = name.c_str();

#ifdef WIN32
	if(TJS_strncmp(ptr, TJS_W("./"), 2))
	{
		// differs from "./",
		// this may be a UNC file name.
		// UNC first two chars must be "\\\\" ?
		// AFAIK 32-bit version of Windows assumes that '/' can be used as a path
		// delimiter. Can UNC "\\\\" be replaced by "//" though ?

		newname = ttstr(TJS_W("\\\\")) + ptr;
	}
	else
	{
		ptr += 2;  // skip "./"
		if(!*ptr) {
			newname = TJS_W("");
		} else {
			tjs_char dch = *ptr;
			if(*ptr < TJS_W('a') || *ptr > TJS_W('z')) {
				newname = TJS_W("");
			} else {
				ptr++;
				if(*ptr != TJS_W('/')) {
					newname = TJS_W("");
				} else {
					newname = ttstr(dch) + TJS_W(":") + ptr;
				}
			}
		}
	}

	// change path delimiter to '\\'
	tjs_char *pp = newname.Independ();
	while(*pp)
	{
		if(*pp == TJS_W('/')) *pp = TJS_W('\\');
		pp++;
	}
#else // posix
    if(!TJS_strncmp(ptr, TJS_W("./"), 2)) {
        ptr += 2;  // skip "./"
        newname.Clear();
    }
#if CC_TARGET_PLATFORM == CC_PLATFORM_IOS
    {
        std::string prefix = "/";
        prefix += tTJSNarrowStringHolder(ptr).Buf;
        static const std::vector<ttstr> &prefixPath = _getPrefixPath();
		static const std::vector<std::string> &homeDir = _getHomeDir();
		for (int i = 0; i < prefixPath.size(); ++i) {
			const std::string &dir = homeDir[i];
			if (prefix.length() < dir.length()) continue;
			std::string actualPrefix = prefix.substr(0, dir.length());
			if (!_utf8_strcasecmp(actualPrefix.c_str(), dir.c_str())) {
				newname = prefixPath[i];
				ptr += prefixPath[i].length();
				while (*ptr && *ptr == TJS_W('/')) ++ptr;
				break;
			}
		}
    }
#endif
    while(*ptr) {
    	const tjs_char *ptr_end = ptr;
    	while(*ptr_end && *ptr_end != TJS_W('/')) ++ptr_end;
    	if(ptr_end == ptr) break;
        const tjs_char *ptr_cur = ptr;
    	tTJSNarrowStringHolder walker(ttstr(ptr, ptr_end - ptr).c_str());
    	while(*ptr_end && *ptr_end == TJS_W('/')) ++ptr_end;
    	ptr = ptr_end;

        DIR *dirp;
        struct dirent *direntp;
		newname += "/";
        if ((dirp = opendir( tTJSNarrowStringHolder(newname.c_str()) ))) {
        	bool found = false;
            while ((direntp = readdir( dirp)) != NULL) {
            	if(!_utf8_strcasecmp(walker, direntp->d_name)) {
            		newname += direntp->d_name;
            		found = true;
            		break;
            	}
            }
            closedir(dirp);
            if(!found) {
                newname += ptr_cur;
            	break;
            }
        } else {
            newname += ptr_cur;
            break;
        }
    }

#endif
	name = newname;
}
//---------------------------------------------------------------------------
void TJS_INTF_METHOD tTVPFileMedia::GetLocalName(ttstr &name)
{
	ttstr tmp = name;
	GetLocallyAccessibleName(tmp);
	if(tmp.IsEmpty()) TVPThrowExceptionMessage(TVPCannotGetLocalName, name);
	name = tmp;
}
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
iTVPStorageMedia * TVPCreateFileMedia()
{
	return new tTVPFileMedia;
}
//---------------------------------------------------------------------------





//---------------------------------------------------------------------------
// TVPPreNormalizeStorageName
//---------------------------------------------------------------------------
void TVPPreNormalizeStorageName(ttstr &name)
{
	// if the name is an OS's native expression, change it according with the
	// TVP storage system naming rule.
	tjs_int namelen = name.GetLen();
	if(namelen == 0) return;
#ifdef WIN32
	if(namelen >= 2)
	{
		if((name[0] >= TJS_W('a') && name[0]<=TJS_W('z') ||
			name[0] >= TJS_W('A') && name[0]<=TJS_W('Z') ) &&
			name[1] == TJS_W(':'))
		{
			// Windows drive:path expression
			ttstr newname(TJS_W("file://./"));
			newname += name[0];
			newname += (name.c_str()+2);
            name = newname;
			return;
		}
	}

	if(namelen>=3)
	{
		if(name[0] == TJS_W('\\') && name[1] == TJS_W('\\') ||
			name[0] == TJS_W('/') && name[1] == TJS_W('/'))
		{
			// unc expression
			name = ttstr(TJS_W("file:")) + name;
			return;
		}
	}
#else // posix
    if(namelen>=1) {
        if(name[0] == TJS_W('/')) {
            name = ttstr(TJS_W("file://.")) + name;
            return;
        }
    }
#endif
}
//---------------------------------------------------------------------------




//---------------------------------------------------------------------------
// TVPGetTemporaryName
//---------------------------------------------------------------------------
static tjs_int TVPTempUniqueNum = 0;
static tTJSCriticalSection TVPTempUniqueNumCS;
static ttstr TVPTempPath;
bool TVPTempPathInit = false;
static tjs_int TVPProcessID;
ttstr TVPGetTemporaryName()
{
	static tjs_int TVPTempUniqueNum = (tjs_int)TVPGetRoughTickCount32();
	tjs_int num = TVPTempUniqueNum++;
	ttstr TVPTempPath = TVPGetAppPath();
#if 0
	tjs_int num;

	{
		tTJSCriticalSectionHolder holder(TVPTempUniqueNumCS);

		if(!TVPTempPathInit)
		{
			wchar_t tmp[MAX_PATH+1];
			::GetTempPath(MAX_PATH, tmp);
			TVPTempPath = tmp;

			if(TVPTempPath.GetLastChar() != TJS_W('\\')) TVPTempPath += TJS_W("\\");
			TVPProcessID = (tjs_int) GetCurrentProcessId();
			TVPTempUniqueNum = (tjs_int) GetTickCount();
			TVPTempPathInit = true;
		}
		num = TVPTempUniqueNum ++;
	}
#endif
	unsigned char buf[16];
	TVPGetRandomBits128(buf);
	tjs_char random[128];
	TJS_snprintf(random, sizeof(random)/sizeof(tjs_char), TJS_W("%02x%02x%02x%02x%02x%02x"),
		buf[0], buf[1], buf[2], buf[3],
		buf[4], buf[5]);

	return TVPTempPath + TJS_W("krkr_") + ttstr(random) +
		TJS_W("_") + ttstr(num) + TJS_W("_") + ttstr(TVPProcessID);
}
//---------------------------------------------------------------------------






//---------------------------------------------------------------------------
// TVPRemoveFile
//---------------------------------------------------------------------------
bool TVPRemoveFile(const ttstr &name)
{
    tTJSNarrowStringHolder holder(name.c_str());
    return !remove(holder);
}
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// TVPRemoveFolder
//---------------------------------------------------------------------------
bool TVPRemoveFolder(const ttstr &name)
{
    tTJSNarrowStringHolder holder(name.c_str());
    return !unlink(holder);
}
//---------------------------------------------------------------------------




//---------------------------------------------------------------------------
// TVPGetAppPath
//---------------------------------------------------------------------------
ttstr TVPGetAppPath()
{
#if 0
	static ttstr exepath(TVPExtractStoragePath(TVPNormalizeStorageName(ExePath())));
	return exepath;
#endif
	static ttstr apppath(TVPExtractStoragePath(TVPProjectDir));
	return apppath;
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// TVPOpenStream
//---------------------------------------------------------------------------
tTJSBinaryStream * TVPOpenStream(const ttstr & _name, tjs_uint32 flags)
{
	// open storage named "name".
	// currently only local/network(by OS) storage systems are supported.
	if(_name.IsEmpty())
		TVPThrowExceptionMessage(TVPCannotOpenStorage, TJS_W("\"\""));

	ttstr origname = _name;
	ttstr name(_name);
	TVPGetLocalName(name);

	return new tTVPLocalFileStream(origname, name, flags);
}
//---------------------------------------------------------------------------





//---------------------------------------------------------------------------
// TVPCheckExistantLocalFile
//---------------------------------------------------------------------------
bool TVPCheckExistentLocalFile(const ttstr &name)
{
#if 0
	DWORD attrib = ::GetFileAttributes(name.c_str());
	if(attrib == 0xffffffff || (attrib & FILE_ATTRIBUTE_DIRECTORY))
		return false; // not a file
	else
		return true; // a file
#endif
	tTVP_stat s;
    if(!TVP_stat(name.c_str(), s)) {
        return false; // not exist
    }
    return s.st_mode & S_IFREG;
}
//---------------------------------------------------------------------------




//---------------------------------------------------------------------------
// TVPCheckExistantLocalFolder
//---------------------------------------------------------------------------
bool TVPCheckExistentLocalFolder(const ttstr &name)
{
#if 0
	DWORD attrib = GetFileAttributes(name.c_str());
	if(attrib != 0xffffffff && (attrib & FILE_ATTRIBUTE_DIRECTORY))
		return true; // a folder
	else
		return false; // not a folder
#endif
	tTVP_stat s;
	if (!TVP_stat(name.c_str(), s)) {
        return false; // not exist
    }

    return s.st_mode & S_IFDIR;
}
//---------------------------------------------------------------------------



tTVPArchive * TVPOpenZIPArchive(const ttstr & name, tTJSBinaryStream *st, bool normalizeFileName);
tTVPArchive * TVPOpen7ZArchive(const ttstr & name, tTJSBinaryStream *st, bool normalizeFileName);
tTVPArchive * TVPOpenTARArchive(const ttstr & name, tTJSBinaryStream *st, bool normalizeFileName);
static tTVPArchive*(*ArchiveCreators[])(const ttstr & name, tTJSBinaryStream *st, bool normalizeFileName) = {
	TVPOpenZIPArchive,
	TVPOpen7ZArchive,
	TVPOpenTARArchive,
	tTVPXP3Archive::Create
};

//---------------------------------------------------------------------------
// TVPOpenArchive
//---------------------------------------------------------------------------
tTVPArchive * TVPOpenArchive(const ttstr & name, bool normalizeFileName)
{
#if 0
	tTVPArchive * archive = TVPOpenSusieArchive(name); // in SusieArchive.h
	if(!archive) return new tTVPXP3Archive(name); else return archive;
#endif
	tTJSBinaryStream *st = TVPCreateStream(name);
	if (!st) return nullptr;
	for (int i = 0; i < sizeof(ArchiveCreators) / sizeof(ArchiveCreators[0]); ++i) {
		tTVPArchive*(*creator)(const ttstr &, tTJSBinaryStream*, bool) = ArchiveCreators[i];
		tTVPArchive * archive = creator(name, st, normalizeFileName);
		if (archive) return archive;
		st->SetPosition(0);
	}
	delete st;
	return nullptr;
}
//---------------------------------------------------------------------------
int TVPCheckArchive(const ttstr &localname)
{
	tTVPArchive *arc = nullptr;
	int validArchive = 2; // archive but no startup.tjs
	try {
		arc = TVPOpenArchive(TVPNormalizeStorageName(localname), false);
		if (arc) {
			tjs_uint count = arc->GetCount();
			ttstr str_startup_tjs = TJS_W("startup.tjs");
			//ttstr str_sys_init_tjs = TJS_W("system/initialize.tjs");
			for (int i = 0; i < count; ++i) {
				ttstr name = arc->GetName(i);
				if (name.length() == str_startup_tjs.length()) {
					arc->NormalizeInArchiveStorageName(name);
					if (name == str_startup_tjs) {
						validArchive = 1;
						break;
					}
				}
// 				else if (name.length() == str_sys_init_tjs.length()) {
// 					arc->NormalizeInArchiveStorageName(name);
// 					if (name == str_sys_init_tjs) {
// 						validArchive = true;
// 						break;
// 					}
// 				}
			}
		}
	} catch (eTJSError e) {
		//arc = nullptr;
	}
	if (arc) {
		delete arc;
		return validArchive;
	}
	return 0; // not archive
}




//---------------------------------------------------------------------------
// TVPLocalExtrectFilePath
//---------------------------------------------------------------------------
ttstr TVPLocalExtractFilePath(const ttstr & name)
{
	// this extracts given name's path under local filename rule
	const tjs_char *p = name.c_str();
	tjs_int i = name.GetLen() -1;
	for(; i >= 0; i--)
	{
		if(p[i] == TJS_W(':') || p[i] == TJS_W('/') ||
			p[i] == TJS_W('\\'))
			break;
	}
	return ttstr(p, i + 1);
}
//---------------------------------------------------------------------------



#if 0
//---------------------------------------------------------------------------
// TVPCreateFolders
//---------------------------------------------------------------------------
static bool _TVPCreateFolders(const ttstr &folder)
{
	// create directories along with "folder"
	if(folder.IsEmpty()) return true;

	if(TVPCheckExistentLocalFolder(folder))
		return true; // already created

	const tjs_char *p = folder.c_str();
	tjs_int i = folder.GetLen() - 1;

	if(p[i] == TJS_W(':')) return true;

	while(i >= 0 && (p[i] == TJS_W('/') || p[i] == TJS_W('\\'))) i--;

	if(i >= 0 && p[i] == TJS_W(':')) return true;

	for(; i >= 0; i--)
	{
		if(p[i] == TJS_W(':') || p[i] == TJS_W('/') ||
			p[i] == TJS_W('\\'))
			break;
	}

	ttstr parent(p, i + 1);

	if(!_TVPCreateFolders(parent)) return false;

	BOOL res = ::CreateDirectory(folder.c_str(), NULL);
	return 0!=res;
}

bool TVPCreateFolders(const ttstr &folder)
{
	if(folder.IsEmpty()) return true;

	const tjs_char *p = folder.c_str();
	tjs_int i = folder.GetLen() - 1;

	if(p[i] == TJS_W(':')) return true;

	if(p[i] == TJS_W('/') || p[i] == TJS_W('\\')) i--;

	return _TVPCreateFolders(ttstr(p, i+1));
}
//---------------------------------------------------------------------------
#endif




//---------------------------------------------------------------------------
// tTVPLocalFileStream
//---------------------------------------------------------------------------
tTVPLocalFileStream::tTVPLocalFileStream(const ttstr &origname,
	const ttstr &localname, tjs_uint32 flag)
    : MemBuffer(nullptr), FileName(localname), Handle(-1)
{
	tjs_uint32 access = flag & TJS_BS_ACCESS_MASK;
    if(access == TJS_BS_WRITE) {
        if (TVPCheckExistentLocalFile(localname)) {
        } else {
			ttstr dirpath = TVPLocalExtractFilePath(localname);
			const tjs_char *p = dirpath.c_str();
			tjs_int i = dirpath.GetLen();
			if (p[i-1] == TJS_W('/') || p[i-1] == TJS_W('\\')) i--;
			dirpath = dirpath.SubString(0, i);
			if (!TVPCheckExistentLocalFolder(dirpath) && !TVPCreateFolders(dirpath)) {
				TVPThrowExceptionMessage(TVPCannotOpenStorage, origname);
			}
//			_lastFileSystemChanged = true;
        }
		MemBuffer = new tTVPMemoryStream();
		return;
	}

	unsigned int rw = 0;
	switch(access)
	{
	case TJS_BS_READ:
		rw |= O_RDONLY;				break;
	case TJS_BS_WRITE:
		rw |= O_RDWR | O_CREAT | O_TRUNC;	break;
	case TJS_BS_APPEND:
        rw |= O_APPEND;	    break;
	case TJS_BS_UPDATE:
		rw |= O_RDWR;			    break;
	}

	tTJSNarrowStringHolder holder(localname.c_str());
	Handle = open(holder, rw, 0666);
	if (Handle < 0) {
		if (access == TJS_BS_APPEND || access == TJS_BS_UPDATE) {
			// use whole file writing
			Handle = open(holder, O_RDONLY, 0666);
			if (Handle >= 0) {
				tjs_uint64 size = GetSize();
				if (size < 4 * 1024 * 1024) { // only support file size <= 4M
					MemBuffer = new tTVPMemoryStream();
					MemBuffer->SetSize(size);
					read(Handle, MemBuffer->GetInternalBuffer(), size);
		}
				close(Handle);
				Handle = -1;
			}
		}
		if (!MemBuffer)
		TVPThrowExceptionMessage(TVPCannotOpenStorage, origname);
	}
	// push current tick as an environment noise
	uint32_t tick = TVPGetRoughTickCount32();
	TVPPushEnvironNoise(&tick, sizeof(tick));
}
//---------------------------------------------------------------------------
bool TVPWriteDataToFile(const ttstr &filepath, const void *data, unsigned int len);
tTVPLocalFileStream::~tTVPLocalFileStream()
{
    if(MemBuffer) {
		if (!TVPWriteDataToFile(FileName, MemBuffer->GetInternalBuffer(), MemBuffer->GetSize())) {
			delete MemBuffer;
			ttstr filename(FileName);
			FileName.~tTJSString();
			free(this);
			TVPThrowExceptionMessage(TJS_W("File Writing Error: %1"), filename);
		}
		delete MemBuffer;
    }
    if (Handle >= 0) {
		close(Handle);
	}

	// push current tick as an environment noise
	// (timing information from file accesses may be good noises)
	uint32_t tick = TVPGetRoughTickCount32();
	TVPPushEnvironNoise(&tick, sizeof(tick));
}
//---------------------------------------------------------------------------
tjs_uint64 TJS_INTF_METHOD tTVPLocalFileStream::Seek(tjs_int64 offset, tjs_int whence)
{
    if(MemBuffer) {
        return MemBuffer->Seek(offset, whence);
	}
	return lseek64(Handle, offset, whence);
}
//---------------------------------------------------------------------------
tjs_uint TJS_INTF_METHOD tTVPLocalFileStream::Read(void *buffer, tjs_uint read_size)
{
    if(MemBuffer) {
        return MemBuffer->Read(buffer, read_size);
	}
    return read(Handle, buffer, read_size);
}
//---------------------------------------------------------------------------
tjs_uint TJS_INTF_METHOD tTVPLocalFileStream::Write(const void *buffer, tjs_uint write_size)
{
    if(MemBuffer) {
        return MemBuffer->Write(buffer, write_size);
	}
    return write(Handle, buffer, write_size);
}
//---------------------------------------------------------------------------
void TJS_INTF_METHOD tTVPLocalFileStream::SetEndOfStorage()
{
    if(MemBuffer) {
        return MemBuffer->SetEndOfStorage();
	}
    lseek64(Handle, 0, SEEK_END);
}
//---------------------------------------------------------------------------
tjs_uint64 TJS_INTF_METHOD tTVPLocalFileStream::GetSize()
{
    if(MemBuffer) {
        return MemBuffer->GetSize();
    }
	tjs_uint64 ret;
    tjs_int64 curpos = lseek64(Handle, 0, SEEK_CUR);
    ret = lseek64(Handle, 0, SEEK_END);
    lseek64(Handle, curpos, SEEK_SET);
	return ret;
}
//---------------------------------------------------------------------------





#ifdef TJS_SUPPORT_VCL
//---------------------------------------------------------------------------
// TTVPStreamAdapter
//---------------------------------------------------------------------------
__fastcall TTVPStreamAdapter::TTVPStreamAdapter(tTJSBinaryStream *ref)
{
	Stream = ref;
	Stream->SetPosition(0);
}
//---------------------------------------------------------------------------
__fastcall TTVPStreamAdapter::~TTVPStreamAdapter()
{
	delete Stream;
}
//---------------------------------------------------------------------------
int __fastcall TTVPStreamAdapter::Read(void *Buffer, int Count)
{
	return (int)Stream->Read(Buffer, Count);
}
//---------------------------------------------------------------------------
int __fastcall TTVPStreamAdapter::Seek(int Offset, WORD Origin)
{
	tjs_int whence;
	switch(Origin)
	{
	case soFromBeginning:	whence = TJS_BS_SEEK_SET;		break;
	case soFromCurrent:		whence = TJS_BS_SEEK_CUR;		break;
	case soFromEnd:			whence = TJS_BS_SEEK_END;		break;
	}

	return (int)Stream->Seek(Offset, whence);
}
//---------------------------------------------------------------------------
int __fastcall TTVPStreamAdapter::Write(const void *Buffer,int Count)
{
	return (int)Stream->Write(Buffer, Count);
}
//---------------------------------------------------------------------------
#endif



#if 1
//---------------------------------------------------------------------------
// tTVPIStreamAdapter
//---------------------------------------------------------------------------
/*
this class provides COM's IStream adapter for tTJSBinaryStream
*/
class tTVPIStreamAdapter : public IStream
{
private:
	tTJSBinaryStream *Stream;
	ULONG RefCount;

public:
	tTVPIStreamAdapter(tTJSBinaryStream *ref);
	/*
	the stream passed by argument here is freed by this instance'
	destruction.
	*/

	~tTVPIStreamAdapter();


	// IUnknown
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid,
		void **ppvObject);
	ULONG STDMETHODCALLTYPE AddRef(void);
	ULONG STDMETHODCALLTYPE Release(void);

	// ISequentialStream
	HRESULT STDMETHODCALLTYPE Read(void *pv, ULONG cb, ULONG *pcbRead);
	HRESULT STDMETHODCALLTYPE Write(const void *pv, ULONG cb,
		ULONG *pcbWritten);

	// IStream
	HRESULT STDMETHODCALLTYPE Seek(LARGE_INTEGER dlibMove,
		DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition);
	HRESULT STDMETHODCALLTYPE SetSize(ULARGE_INTEGER libNewSize);
	HRESULT STDMETHODCALLTYPE CopyTo(IStream *pstm, ULARGE_INTEGER cb,
		ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten);
	HRESULT STDMETHODCALLTYPE Commit(DWORD grfCommitFlags);
	HRESULT STDMETHODCALLTYPE Revert(void);
	HRESULT STDMETHODCALLTYPE LockRegion(ULARGE_INTEGER libOffset,
		ULARGE_INTEGER cb, DWORD dwLockType);
	HRESULT STDMETHODCALLTYPE UnlockRegion(ULARGE_INTEGER libOffset,
		ULARGE_INTEGER cb, DWORD dwLockType);
	HRESULT STDMETHODCALLTYPE Stat(STATSTG *pstatstg, DWORD grfStatFlag);
	HRESULT STDMETHODCALLTYPE Clone(IStream **ppstm);

	void ClearStream() {
		Stream = NULL;
	}
};
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// tTVPIStreamAdapter
//---------------------------------------------------------------------------
/*
	this class provides adapter for COM's IStream
*/
tTVPIStreamAdapter::tTVPIStreamAdapter(tTJSBinaryStream *ref)
{
	Stream = ref;
	RefCount = 1;
}
//---------------------------------------------------------------------------
tTVPIStreamAdapter::~tTVPIStreamAdapter()
{
	delete Stream;
}
//---------------------------------------------------------------------------
extern "C" const IID IID_IUnknown;
extern "C" const IID IID_IStream;
extern "C" const IID IID_ISequentialStream;
HRESULT STDMETHODCALLTYPE tTVPIStreamAdapter::QueryInterface(REFIID riid,
		void **ppvObject)
{
	if(!ppvObject) return E_INVALIDARG;

	*ppvObject=NULL;
	if(!memcmp(&riid,&IID_IUnknown,16))
		*ppvObject=(IUnknown*)this;
	else if(!memcmp(&riid,&IID_ISequentialStream,16))
		*ppvObject=(ISequentialStream*)this;
	else if(!memcmp(&riid,&IID_IStream,16))
		*ppvObject=(IStream*)this;

	if(*ppvObject)
	{
		AddRef();
		return S_OK;
	}
	return E_NOINTERFACE;
}
//---------------------------------------------------------------------------
ULONG STDMETHODCALLTYPE tTVPIStreamAdapter::AddRef(void)
{
	return ++ RefCount;
}
//---------------------------------------------------------------------------
ULONG STDMETHODCALLTYPE tTVPIStreamAdapter::Release(void)
{
	if(RefCount == 1)
	{
		delete this;
		return 0;
	}
	else
	{
		return --RefCount;
	}
}
//---------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE tTVPIStreamAdapter::Read(void *pv, ULONG cb, ULONG *pcbRead)
{
	try
	{
		ULONG read;
		read = Stream->Read(pv, cb);
		if(pcbRead) *pcbRead = read;
	}
	catch(...)
	{
		return E_FAIL;
	}
	return S_OK;
}
//---------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE tTVPIStreamAdapter::Write(const void *pv, ULONG cb,
		ULONG *pcbWritten)
{
	try
	{
		ULONG written;
		written = Stream->Write(pv, cb);
		if(pcbWritten) *pcbWritten = written;
	}
	catch(...)
	{
		return E_FAIL;
	}
	return S_OK;
}
//---------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE tTVPIStreamAdapter::Seek(LARGE_INTEGER dlibMove,
	DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition)
{
	try
	{
		switch(dwOrigin)
		{
		case STREAM_SEEK_SET:
			if(plibNewPosition)
				(*plibNewPosition).QuadPart =
					Stream->Seek(dlibMove.QuadPart, TJS_BS_SEEK_SET);
			else
					Stream->Seek(dlibMove.QuadPart, TJS_BS_SEEK_SET);
			break;
		case STREAM_SEEK_CUR:
			if(plibNewPosition)
				(*plibNewPosition).QuadPart =
					Stream->Seek(dlibMove.QuadPart, TJS_BS_SEEK_CUR);
			else
					Stream->Seek(dlibMove.QuadPart, TJS_BS_SEEK_CUR);
			break;
		case STREAM_SEEK_END:
			if(plibNewPosition)
				(*plibNewPosition).QuadPart =
					Stream->Seek(dlibMove.QuadPart, TJS_BS_SEEK_END);
			else
					Stream->Seek(dlibMove.QuadPart, TJS_BS_SEEK_END);
			break;
		default:
			return E_FAIL;
		}
	}
	catch(...)
	{
		return E_FAIL;
	}
	return S_OK;
}
//---------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE tTVPIStreamAdapter::SetSize(ULARGE_INTEGER libNewSize)
{
	return E_NOTIMPL;
}
//---------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE tTVPIStreamAdapter::CopyTo(IStream *pstm, ULARGE_INTEGER cb,
	ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten)
{
	return E_NOTIMPL;
}
//---------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE tTVPIStreamAdapter::Commit(DWORD grfCommitFlags)
{
	return E_NOTIMPL;
}
//---------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE tTVPIStreamAdapter::Revert(void)
{
	return E_NOTIMPL;
}
//---------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE tTVPIStreamAdapter::LockRegion(ULARGE_INTEGER libOffset,
	ULARGE_INTEGER cb, DWORD dwLockType)
{
	return E_NOTIMPL;
}
//---------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE tTVPIStreamAdapter::UnlockRegion(ULARGE_INTEGER libOffset,
	ULARGE_INTEGER cb, DWORD dwLockType)
{
	return E_NOTIMPL;
}
//---------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE tTVPIStreamAdapter::Stat(STATSTG *pstatstg, DWORD grfStatFlag)
{
	// This method imcompletely fills the target structure, because some
	// informations like access mode or stream name are already lost
	// at this point.

	if(pstatstg)
	{
		memset(pstatstg, 0, sizeof(*pstatstg));

		// pwcsName
		// this object's storage pointer does not have a name ...
		if(!(grfStatFlag &  STATFLAG_NONAME))
		{
			// anyway returns an empty string
			pstatstg->pwcsName = (LPOLESTR)TJS_W("");
		}

		// type
		pstatstg->type = STGTY_STREAM;

		// cbSize
		pstatstg->cbSize.QuadPart = Stream->GetSize();

		// mtime, ctime, atime unknown

		// grfMode unknown
		pstatstg->grfMode = STGM_DIRECT | STGM_READWRITE | STGM_SHARE_DENY_WRITE ;
			// Note that this method always returns flags above, regardless of the
			// actual mode.
			// In the return value, the stream is to be indicated that the
			// stream can be written, but of cource, the Write method will fail
			// if the stream is read-only.

		// grfLockSuppoted
		pstatstg->grfLocksSupported = 0;

		// grfStatBits unknown
	}
	else
	{
		return E_INVALIDARG;
	}

	return S_OK;
}
//---------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE tTVPIStreamAdapter::Clone(IStream **ppstm)
{
	return E_NOTIMPL;
}
//---------------------------------------------------------------------------





#endif

IStream * TVPCreateIStream(tTJSBinaryStream *s) { return new tTVPIStreamAdapter(s); }
//---------------------------------------------------------------------------
// IStream creator
//---------------------------------------------------------------------------
IStream * TVPCreateIStream(const ttstr &name, tjs_uint32 flags)
{
	// convert tTJSBinaryStream to IStream thru TStream

	tTJSBinaryStream *stream0 = NULL;
	try
	{
		stream0 = TVPCreateStream(name, flags);
	}
	catch(...)
	{
		if(stream0) delete stream0;
		return NULL;
	}

	IStream *istream = new tTVPIStreamAdapter(stream0);

	return istream;
}
//---------------------------------------------------------------------------




#if 0
//---------------------------------------------------------------------------
// tTVPBinaryStreamAdapter
//---------------------------------------------------------------------------
tTVPBinaryStreamAdapter::tTVPBinaryStreamAdapter(IStream *ref)
{
	Stream = ref;
	Stream->AddRef();
}
//---------------------------------------------------------------------------
tTVPBinaryStreamAdapter::~tTVPBinaryStreamAdapter()
{
	Stream->Release();
}
//---------------------------------------------------------------------------
tjs_uint64 TJS_INTF_METHOD tTVPBinaryStreamAdapter::Seek(tjs_int64 offset, tjs_int whence)
{
	DWORD origin;

	switch(whence)
	{
	case TJS_BS_SEEK_SET:			origin = STREAM_SEEK_SET;		break;
	case TJS_BS_SEEK_CUR:			origin = STREAM_SEEK_CUR;		break;
	case TJS_BS_SEEK_END:			origin = STREAM_SEEK_END;		break;
	default:						origin = STREAM_SEEK_SET;		break;
	}

	LARGE_INTEGER ofs;
	ULARGE_INTEGER newpos;

	ofs.QuadPart = 0;
	HRESULT hr = Stream->Seek(ofs, STREAM_SEEK_CUR, &newpos);
	bool orgpossaved;
	LARGE_INTEGER orgpos;
	if(FAILED(hr))
	{
		orgpossaved = false;
	}
	else
	{
		orgpossaved = true;
		*(LARGE_INTEGER*)&orgpos = *(LARGE_INTEGER*)&newpos;
	}

	ofs.QuadPart = offset;

	hr = Stream->Seek(ofs, origin, &newpos);
	if(FAILED(hr))
	{
		if(orgpossaved)
		{
			Stream->Seek(orgpos, STREAM_SEEK_SET, &newpos);
		}
		else
		{
			TVPThrowExceptionMessage(TVPSeekError);
		}
	}

	return newpos.QuadPart;
}
//---------------------------------------------------------------------------
tjs_uint TJS_INTF_METHOD tTVPBinaryStreamAdapter::Read(void *buffer, tjs_uint read_size)
{
	ULONG cb = read_size;
	ULONG read;
	HRESULT hr = Stream->Read(buffer, cb, &read);
	if(FAILED(hr)) read = 0;
	return read;
}
//---------------------------------------------------------------------------
tjs_uint TJS_INTF_METHOD tTVPBinaryStreamAdapter::Write(const void *buffer, tjs_uint write_size)
{
	ULONG cb = write_size;
	ULONG written;
	HRESULT hr = Stream->Write(buffer, cb, &written);
	if(FAILED(hr)) written = 0;
	return written;
}
//---------------------------------------------------------------------------
tjs_uint64 TJS_INTF_METHOD tTVPBinaryStreamAdapter::GetSize()
{
	HRESULT hr;
	STATSTG stg;

	hr = Stream->Stat(&stg, STATFLAG_NONAME);
	if(FAILED(hr))
	{
		return inherited::GetSize(); // use default routine
	}

	return stg.cbSize.QuadPart;
}
//---------------------------------------------------------------------------
void TJS_INTF_METHOD tTVPBinaryStreamAdapter::SetEndOfStorage()
{
	ULARGE_INTEGER pos;
	pos.QuadPart = GetPosition();
	HRESULT hr = Stream->SetSize(pos);
	if(FAILED(hr)) TVPThrowExceptionMessage(TVPTruncateError);
}
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
// TVPCreateBinaryStreamAdapter
//---------------------------------------------------------------------------
tTJSBinaryStream * TVPCreateBinaryStreamAdapter(IStream *refstream)
{
	return new  tTVPBinaryStreamAdapter(refstream);
}
//---------------------------------------------------------------------------
#endif








//---------------------------------------------------------------------------
// tTVPPluginHolder
//---------------------------------------------------------------------------
tTVPPluginHolder::tTVPPluginHolder(const ttstr &aname)
{
	LocalTempStorageHolder = NULL;

	// search in TVP storage system
	ttstr place(TVPGetPlacedPath(aname));
	if(!place.IsEmpty())
	{
		LocalTempStorageHolder = new tTVPLocalTempStorageHolder(place);
	}
	else
	{
		// not found in TVP storage system; search exepath, exepath\system, exepath\plugin
#if 0
		ttstr exepath =
			IncludeTrailingBackslash(ExtractFileDir(ExePath()));
		ttstr pname = exepath + aname;
		if(TVPCheckExistentLocalFile(pname))
		{
			LocalName = pname;
			return;
		}

		pname = exepath + TJS_W("system\\") + aname;
		if(TVPCheckExistentLocalFile(pname))
		{
			LocalName = pname;
			return;
		}

#ifdef TJS_64BIT_OS
		pname = exepath + TJS_W("plugin64\\") + aname;
#else
		pname = exepath + TJS_W("plugin\\") + aname;
#endif
		if(TVPCheckExistentLocalFile(pname))
		{
			LocalName = pname;
			return;
		}
#endif
	}
}
//---------------------------------------------------------------------------
tTVPPluginHolder::~tTVPPluginHolder()
{
	if(LocalTempStorageHolder)
	{
		delete LocalTempStorageHolder;
	}
}
//---------------------------------------------------------------------------
const ttstr & tTVPPluginHolder::GetLocalName() const
{
	if(LocalTempStorageHolder) return LocalTempStorageHolder->GetLocalName();
	return LocalName;
}
//---------------------------------------------------------------------------




//---------------------------------------------------------------------------
// TVPSearchCD
//---------------------------------------------------------------------------
ttstr TVPSearchCD(const ttstr & name)
{
	// search CD which has specified volume label name.
	// return drive letter ( such as 'A' or 'B' )
	// return empty string if not found.
#if 0
	std::wstring narrow_name = name.AsStdString();

	wchar_t dr[4];
	for(dr[0]=L'A',dr[1]=L':',dr[2]=L'\\',dr[3]=0;dr[0]<=L'Z';dr[0]++)
	{
		if(::GetDriveType(dr) == DRIVE_CDROM)
		{
			wchar_t vlabel[256];
			wchar_t fs[256];
			DWORD mcl = 0,sfs = 0;
			GetVolumeInformation(dr, vlabel, 255, NULL, &mcl, &sfs, fs, 255);
			if( icomp(std::wstring(vlabel),narrow_name) )
			//if(std::string(vlabel).AnsiCompareIC(narrow_name)==0)
				return ttstr((tjs_char)dr[0]);
		}
	}
#endif
	return ttstr();
}
//---------------------------------------------------------------------------


tTJSBinaryStream * TVPGetCachedArchiveHandle(void * pointer, const ttstr & name);
void TVPReleaseCachedArchiveHandle(void * pointer, tTJSBinaryStream * stream);
TArchiveStream::TArchiveStream(tTVPArchive *owner, tjs_uint64 off, tjs_uint64 len) : Owner(owner), StartPos(off), DataLength(len) {
	Owner->AddRef();
	_instr = TVPGetCachedArchiveHandle(Owner, Owner->ArchiveName);
	CurrentPos = 0;
	_instr->SetPosition(off);
}

TArchiveStream::~TArchiveStream() {
	TVPReleaseCachedArchiveHandle(Owner, _instr);
	Owner->Release();
}

//---------------------------------------------------------------------------
// TVPCreateNativeClass_Storages
//---------------------------------------------------------------------------
tTJSNativeClass * TVPCreateNativeClass_Storages()
{
	tTJSNC_Storages *cls = new tTJSNC_Storages();


	// setup some platform-specific members
//----------------------------------------------------------------------

//-- methods

//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/searchCD)
{
	if(numparams < 1) return TJS_E_BADPARAMCOUNT;

	if(result)
		*result = TVPSearchCD(*param[0]);

	return TJS_S_OK;
}
TJS_END_NATIVE_STATIC_METHOD_DECL_OUTER(/*object to register*/cls,
	/*func. name*/searchCD)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/getLocalName)
{
	if(numparams < 1) return TJS_E_BADPARAMCOUNT;

	if(result)
	{
		ttstr str(TVPNormalizeStorageName(*param[0]));
		TVPGetLocalName(str);
		*result = str;
	}

	return TJS_S_OK;
}
TJS_END_NATIVE_STATIC_METHOD_DECL_OUTER(/*object to register*/cls,
	/*func. name*/getLocalName)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/selectFile)
{
	if(numparams < 1) return TJS_E_BADPARAMCOUNT;

	iTJSDispatch2 * dsp =  param[0]->AsObjectNoAddRef();

	bool res = TVPSelectFile(dsp);

	if(result) *result = (tjs_int)res;

	return TJS_S_OK;
}
TJS_END_NATIVE_STATIC_METHOD_DECL_OUTER(/*object to register*/cls,
	/*func. name*/selectFile)
//----------------------------------------------------------------------


	return cls;

}
//---------------------------------------------------------------------------

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

bool TVPSaveStreamToFile(tTJSBinaryStream *st, tjs_uint64 offset, tjs_uint64 size, ttstr outpath) {
	FILE *fp = _fileopen(outpath);
	if (!fp) return false;
	tjs_uint64 origpos = st->GetPosition();
	st->SetPosition(offset);
	std::vector<char> buffer; buffer.resize(2 * 1024 * 1024);
	while (size > 0) {
		unsigned int readsize = size > buffer.size() ? buffer.size() : size;
		readsize = st->Read(&buffer.front(), readsize);
		fwrite(&buffer.front(), 1, readsize, fp);
		size -= readsize;
	}
	fclose(fp);
	st->SetPosition(origpos);
	return true;
}
