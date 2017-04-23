#include "tjsCommHead.h"
#include "StorageIntf.h"
#include "UtilStreams.h"
#include "Platform.h"
#include "MsgIntf.h"
#include <algorithm>

void TVPFreeArchiveHandlePoolByPointer(void * pointer);
void TVPReleaseCachedArchiveHandle(void * pointer, tTJSBinaryStream * stream);

void storeFilename(ttstr &name, const char *narrowName, const ttstr &filename)
{
	tjs_int len = TJS_narrowtowidelen(narrowName);
	if (len == -1) {
		ttstr msg("Filename is not encoded in UTF8 in archive:\n");
		TVPShowSimpleMessageBox(msg + filename, TJS_W("Error"));
		TVPThrowExceptionMessage(TJS_W("Invalid archive entry name"));
	}
	tjs_char *p = name.AllocBuffer(len);
	p[TJS_narrowtowide(p, narrowName, len)] = 0;
	name.FixLen();
}

//---------------------------------------------------------------------------
// tar archive
//---------------------------------------------------------------------------
tjs_uint64 parseOctNum(const char *oct, int length)
{
	tjs_uint64 num = 0;
	for (int i = 0; i < length; i++){
		char c = oct[i];
		if ('0' <= c && c <= '9'){
			num = num * 8 + (c - '0');
		}
	}
	return num;
}
#include "tar.h"
class TARArchive : public tTVPArchive {
	struct EntryInfo {
		ttstr filename;
		tjs_uint64 offset;
		tjs_uint64 size;
	};
	std::vector<EntryInfo> filelist;
	friend class TARArchiveStream;

public:
	TARArchive(const ttstr & arcname) : tTVPArchive(arcname) {
	}
	~TARArchive() {
		TVPFreeArchiveHandlePoolByPointer(this);
	}
	bool init(tTJSBinaryStream * _instr, bool normalizeFileName) {
		if (_instr) {
			tjs_uint64 archiveSize = _instr->GetSize();
			TAR_HEADER tar_header;
			// check first header
			if (_instr->Read(&tar_header, sizeof(tar_header)) != sizeof(tar_header)) {
				//delete _instr;
				return false;
			}
			unsigned int checksum = parseOctNum(tar_header.dbuf.chksum, sizeof(tar_header.dbuf.chksum));
			if (checksum != tar_header.compsum() && (int)checksum != tar_header.compsum_oldtar()) {
				//delete _instr;
				return false;
			}
			_instr->SetPosition(0);
			while (_instr->GetPosition() <= archiveSize - sizeof(tar_header)) {
				if (_instr->Read(&tar_header, sizeof(tar_header)) != sizeof(tar_header)) break;
				tjs_uint64 original_size = parseOctNum(tar_header.dbuf.size, sizeof(tar_header.dbuf.size));
				std::vector<char> filename;
				if (tar_header.dbuf.typeflag == LONGLINK) {	// tar_header.dbuf.name == "././@LongLink"
					unsigned int readsize = (original_size + (TBLOCK - 1)) & ~(TBLOCK - 1);
					filename.resize(readsize + 1);	//TODO:size lost
					if (_instr->Read(&filename[0], readsize) != readsize) break;
					filename[readsize] = 0;
					if (_instr->Read(&tar_header, sizeof(tar_header)) != sizeof(tar_header)) break;
					original_size = parseOctNum(tar_header.dbuf.size, sizeof(tar_header.dbuf.size));
				}
				if (tar_header.dbuf.typeflag != REGTYPE) continue; // only accept regular file
				if (filename.empty()) {
					filename.resize(101);
					memcpy(&filename[0], tar_header.dbuf.name, sizeof(tar_header.dbuf.name));
					filename[100] = 0;
				}
				EntryInfo item;
				storeFilename(item.filename, &filename[0], ArchiveName);
				if (normalizeFileName)
					NormalizeInArchiveStorageName(item.filename);
				item.size = original_size;
				item.offset = _instr->GetPosition();
				filelist.emplace_back(item);
				tjs_uint64 readsize = (original_size + (TBLOCK - 1)) & ~(TBLOCK - 1);
				_instr->SetPosition(item.offset + readsize);
			}
			if (normalizeFileName) {
				std::sort(filelist.begin(), filelist.end(), [](const EntryInfo& a, const EntryInfo& b) {
					return a.filename < b.filename;
				});
			}
			TVPReleaseCachedArchiveHandle(this, _instr);
			return true;
		}
		return false;
	}
	virtual tjs_uint GetCount() { return filelist.size(); }
	virtual ttstr GetName(tjs_uint idx) { return filelist[idx].filename; }
	virtual tTJSBinaryStream * CreateStreamByIndex(tjs_uint idx);
};

tTJSBinaryStream * TARArchive::CreateStreamByIndex(tjs_uint idx) {
	const EntryInfo &info = filelist[idx];
	return new TArchiveStream(this, info.offset, info.size);
}

tTVPArchive * TVPOpenTARArchive(const ttstr & name, tTJSBinaryStream * st, bool normalizeFileName) {
	TARArchive *arc = new TARArchive(name);
	if (!arc->init(st, normalizeFileName)) {
		delete arc;
		return nullptr;
	}
	return arc;
}
