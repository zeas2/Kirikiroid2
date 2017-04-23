#include "XP3ArchiveRepack.h"
#include <functional>
#include "7zip/CPP/7zip/Archive/7z/7zOut.h"
#include "7zip/CPP/7zip/Common/StreamObjects.h"
extern "C" {
#include "7zip/C/7zCrc.h"
}
#include "TextStream.h"
#include "StorageImpl.h"
#include "XP3Archive.h"
#include <time.h>
#include <assert.h>
#include <thread>
#include <fcntl.h>
#include <unistd.h>
#include "win32io.h"
#include "GraphicsLoaderIntf.h"
#include "ogl/etcpak.h"
#include <unordered_map>
#include "LayerIntf.h"
#include "MsgIntf.h"
#include "ncbind/ncbind.hpp"
#include "visual/tvpgl_asm_init.h"
#include "DetectCPU.h"
#include <set>

using namespace TJS;
using namespace NArchive::N7z;

#define COMPRESS_THRESHOLD 4 * 1024 * 1024
#define S_INTERRUPT ((HRESULT)0x114705)

void TVPSetXP3FilterScript(ttstr content);
void TVPSavePVRv3(void* formatdata, tTJSBinaryStream* dst, const iTVPBaseBitmap* image, const ttstr & mode, iTJSDispatch2* meta);

class tTVPXP3ArchiveEx : public tTVPXP3Archive {
public:
	tTVPXP3ArchiveEx(const ttstr & name, tTJSBinaryStream *st) : tTVPXP3Archive(name, 0) {
		Init(st, -1, false);
	}

	uint64_t TotalSize = 0;
};

class OutStreamFor7z : public IOutStream {
	long fp;
	unsigned int RefCount = 1;

public:
	OutStreamFor7z(const std::string &path) {
		fp = open(path.c_str(), O_RDWR | O_CREAT, 0666);
	}
	~OutStreamFor7z() {
		close(fp);
	}

	ULONG AddRef() { return ++RefCount; }
	ULONG Release() {
		if (RefCount == 1) {
			delete this;
			return 0;
		}
		return --RefCount;
	}
	HRESULT QueryInterface(REFIID iid, void **ppOut) {
		if (!memcmp(&iid, &IID_IOutStream, sizeof(IID_IOutStream))) {
			*ppOut = (IOutStream*)this;
			return S_OK;
		}
		*ppOut = nullptr;
		return E_NOTIMPL;
	}

	HRESULT Write(const void *data, UInt32 size, UInt32 *processedSize) {
		*processedSize = write(fp, data, size);
		return S_OK;
	}
	HRESULT Seek(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition) override {
		int64_t pos = lseek64(fp, offset, seekOrigin);
		if (newPosition) *newPosition = pos;
		return S_OK;
	}
	HRESULT SetSize(UInt64 newSize) {
		return S_OK;
	}
};

class OutStreamMemory : public IOutStream, public tTVPMemoryStream {
	unsigned int RefCount = 1;

public:
	ULONG AddRef() { return ++RefCount; }
	ULONG Release() {
		if (RefCount == 1) {
			delete this;
			return 0;
		}
		return --RefCount;
	}
	HRESULT QueryInterface(REFIID iid, void **ppOut) {
		if (!memcmp(&iid, &IID_IOutStream, sizeof(IID_IOutStream))) {
			*ppOut = (IOutStream*)this;
			return S_OK;
		}
		*ppOut = nullptr;
		return E_NOTIMPL;
	}

	HRESULT Write(const void *data, UInt32 size, UInt32 *processedSize) override {
		*processedSize = tTVPMemoryStream::Write(data, size);
		return S_OK;
	}
	HRESULT Seek(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition) override  {
		*newPosition = tTVPMemoryStream::Seek(offset, seekOrigin);
		return S_OK;
	}
	HRESULT SetSize(UInt64 newSize) {
		tTVPMemoryStream::SetSize(newSize);
		return S_OK;
	}
};

class SequentialInStreamFor7z : public ISequentialInStream {
	tTJSBinaryStream *fp;
	unsigned int RefCount = 1;

public:
	SequentialInStreamFor7z(tTJSBinaryStream *s) {
		fp = s;
	}
	~SequentialInStreamFor7z() {
		fp = nullptr;
	}

	ULONG AddRef() { return ++RefCount; }
	ULONG Release() {
		if (RefCount == 1) {
			delete this;
			return 0;
		}
		return --RefCount;
	}
	HRESULT QueryInterface(REFIID iid, void **ppOut) {
		if (!memcmp(&iid, &IID_IInStream, sizeof(IID_IInStream))) {
			*ppOut = (ISequentialInStream*)this;
			return S_OK;
		}
		*ppOut = nullptr;
		return E_NOTIMPL;
	}

	HRESULT Read(void *data, UInt32 size, UInt32 *processedSize) {
		*processedSize = fp->Read(data, size);
		return S_OK;
	}
};

class XP3ArchiveRepackAsyncImpl
	: ICompressProgressInfo
{
public:
	XP3ArchiveRepackAsyncImpl();
	~XP3ArchiveRepackAsyncImpl();
	void Clear();
	void Start();
	void Stop() { bStopRequired = true; }
	void DoConv();
	uint64_t AddTask(const std::string &src);
	void SetXP3Filter(const std::string &xp3filter);
	void SetCallback(
		const std::function<void(int, uint64_t, const std::string &)> &onNewFile,
		const std::function<void(int, uint64_t, const std::string &)> &onNewArchive,
		const std::function<void(uint64_t, uint64_t, uint64_t)> &onProgress,
		const std::function<void(int, const std::string &)> &onError,
		const std::function<void()> &onEnded) {
		OnNewFile = onNewFile;
		OnNewArchive = onNewArchive;
		OnProgress = onProgress;
		OnError = onError;
		OnEnded = onEnded;
	}
	void SetOption(const std::string &name, bool v);

public:
	bool OptionUsingETC2 = false;
	bool OptionMergeMaskImg = true;

private:
	HRESULT AddTo7zArchive(tTJSBinaryStream* s, const ttstr &_naem);

	// ICompressProgressInfo
	virtual HRESULT SetRatioInfo(const UInt64 *inSize, const UInt64 *outSize) override;
	// IUnknown
	virtual HRESULT QueryInterface(REFIID riid, void **ppvObject) override { return E_NOTIMPL; }
	virtual ULONG AddRef(void) override { return 0; }
	virtual ULONG Release(void) override { return 0; }

	std::vector<std::pair<tTVPXP3ArchiveEx*, std::string> > ConvFileList;
	std::thread* Thread = nullptr;
	CCreatedCoder CoderCompress, CoderCopy;
	static const int nCodecMethod = 0x30101, // LZMA
		nCodecMethodCopy = 0;
	CArchiveDatabaseOut newDatabase;
	OutStreamFor7z *strout;
	CByteBuffer props;
	bool bStopRequired = false;
	uint64_t nTotalSize = 0, nArcSize = 0;

	std::function<void(int, uint64_t, const std::string &)> OnNewFile;
	std::function<void(int, uint64_t, const std::string &)> OnNewArchive;
	std::function<void(uint64_t, uint64_t, uint64_t)> OnProgress;
	std::function<void(int, const std::string &)> OnError;
	std::function<void()> OnEnded;
};

XP3ArchiveRepackAsync::XP3ArchiveRepackAsync()
	: _impl(new XP3ArchiveRepackAsyncImpl())
{
	TVPDetectCPU();
	TVPGL_ASM_Init();
}

XP3ArchiveRepackAsync::~XP3ArchiveRepackAsync() {
	delete _impl;
}

uint64_t XP3ArchiveRepackAsync::AddTask(const std::string &src) {
	return _impl->AddTask(src);
}

void XP3ArchiveRepackAsync::Start()
{
	_impl->Start();
}

void XP3ArchiveRepackAsync::Stop()
{
	_impl->Stop();
}

void XP3ArchiveRepackAsync::SetXP3Filter(const std::string &xp3filter)
{
	_impl->SetXP3Filter(xp3filter);
}

void XP3ArchiveRepackAsync::SetCallback(
	const std::function<void(int, uint64_t, const std::string &)> &onNewFile,
	const std::function<void(int, uint64_t, const std::string &)> &onNewArchive,
	const std::function<void(uint64_t, uint64_t, uint64_t)> &onProgress,
	const std::function<void(int, const std::string &)> &onError,
	const std::function<void()> &onEnded)
{
	_impl->SetCallback(onNewFile, onNewArchive, onProgress, onError, onEnded);
}

void XP3ArchiveRepackAsync::SetOption(const std::string &name, bool v)
{
	_impl->SetOption(name, v);
}

XP3ArchiveRepackAsyncImpl::XP3ArchiveRepackAsyncImpl()
{
	if (!g_CrcTable[1]) CrcGenerateTable();
	CreateCoder(nCodecMethod, true, CoderCompress);
	CreateCoder(nCodecMethodCopy, true, CoderCopy);

	CMyComPtr<ICompressWriteCoderProperties> writeCoderProperties;
	CMyComPtr<ICompressSetCoderProperties> setCoderProperties;
	CoderCompress.Coder->QueryInterface(IID_ICompressWriteCoderProperties, (void **)&writeCoderProperties);
	CoderCompress.Coder->QueryInterface(IID_ICompressSetCoderProperties, (void **)&setCoderProperties);

	if (setCoderProperties) {
		const int nProp = 1;
		PROPVARIANT propvars[nProp];
		PROPID props[nProp] = {
			NCoderPropID::kDictionarySize,
		};
		propvars[0].vt = VT_UI4;
		propvars[0].ulVal = 4 * 1024 * 1024;
		setCoderProperties->SetCoderProperties(props, propvars, nProp);
	}
	if (writeCoderProperties) {
		CDynBufSeqOutStream *outStreamSpec = new CDynBufSeqOutStream;
		CMyComPtr<ISequentialOutStream> dynOutStream(outStreamSpec);
		outStreamSpec->Init();
		writeCoderProperties->WriteCoderProperties(dynOutStream);
		outStreamSpec->CopyToBuffer(props);
	}
}

XP3ArchiveRepackAsyncImpl::~XP3ArchiveRepackAsyncImpl()
{
	std::thread* t = Thread;
	if (t && t->joinable()) {
		t->join();
		delete t;
	}
	Clear();
}

uint64_t XP3ArchiveRepackAsyncImpl::AddTask(const std::string &src/*, const std::string &dst*/)
{
	ttstr path(src);
	tTVPLocalFileStream *str = new tTVPLocalFileStream(path, path, TJS_BS_READ);
	tTVPXP3ArchiveEx *xp3arc = new tTVPXP3ArchiveEx(path, str);
	for (const tTVPXP3Archive::tArchiveItem &item : xp3arc->ItemVector) {
		xp3arc->TotalSize += item.OrgSize;
	}
	ConvFileList.emplace_back(xp3arc, src);
	return xp3arc->TotalSize;
}

void XP3ArchiveRepackAsyncImpl::SetXP3Filter(const std::string &xp3filter)
{
	ttstr xp3filterScript;
	iTJSTextReadStream * stream = TVPCreateTextStreamForRead(xp3filter, "");
	stream->Read(xp3filterScript, 0);
	delete stream;
	TVPSetXP3FilterScript(xp3filterScript);
}

void XP3ArchiveRepackAsyncImpl::SetOption(const std::string &name, bool v)
{
	if (name == "merge_mask_img") {
		OptionMergeMaskImg = v;
	} else if (name == "conv_etc2") {
		OptionUsingETC2 = v;
	}
}

void XP3ArchiveRepackAsyncImpl::Clear()
{
	for (auto & it : ConvFileList) {
		if (it.first)
			delete it.first;
	}
	ConvFileList.clear();
	TVPSetXP3FilterScript(TJS_W(""));
}

void XP3ArchiveRepackAsyncImpl::Start()
{
	if (!Thread) {
		Thread = new std::thread(std::bind(&XP3ArchiveRepackAsyncImpl::DoConv, this));
	}
}

static bool CheckIsImage(tTJSBinaryStream *s) {
	// convert image > 8k
	tjs_uint8 header[16];
	tjs_uint readed = s->Read(header, 16);
	s->SetPosition(0);
	if (readed != 16) return false;
	if (!memcmp(header, "BM", 2)) {
		return true;
	} else if (!memcmp(header, "\x89PNG", 4)) {
		return true;
	} else if (!memcmp(header, "\xFF\xD8\xFF", 3)) { // JPEG
		return true;
	} else if (!memcmp(header, "TLG", 3)) {
		return true;
	} else if (!memcmp(header, "\x49\x49\xbc\x01", 4)) { // JXR
		return true;
	} else if (!memcmp(header, "BPG", 3)) {
		return true;
	} else if (!memcmp(header, "RIFF", 4)) {
		if (!memcmp(header + 8, "WEBPVP8", 7)) {
			return true;
		}
	}
	return false;
}

HRESULT XP3ArchiveRepackAsyncImpl::AddTo7zArchive(tTJSBinaryStream* s, const ttstr &_name) {
	CFileItem file;
	CFileItem2 file2;
	SequentialInStreamFor7z str(s);
	tjs_uint64 origSize = s->GetSize();
	file.Size = origSize;
	file2.Init();
	UString name(_name.c_str());
	UInt64 inSizeForReduce = origSize;
	UInt64 newSize = origSize;
	bool compressable = origSize > 64 && origSize < 4 * 1024 * 1024;
	if (compressable) {
		// compressable quick check
		tjs_uint8 header[16];
		s->Read(header, 16);
		if (!memcmp(header, "\x89PNG", 4)) {
			compressable = false;
		} else if (!memcmp(header, "OggS\x00", 5)) {
			compressable = false;
		} else if (!memcmp(header, "\xFF\xD8\xFF", 3)) { // JPEG
			compressable = false;
		} else if (!memcmp(header, "TLG", 3)) {
			compressable = false;
		} else if (!memcmp(header, "0&\xB2\x75\x8E\x66\xCF\x11", 8)) { // wmv/wma
			compressable = false;
		} else if (!memcmp(header, "AJPM", 4)) { // AMV
			compressable = false;
		} else if (!memcmp(header, "\x49\x49\xbc\x01", 4)) { // JXR
			compressable = false;
		} else if (!memcmp(header, "BPG", 3)) {
			compressable = false;
		} else if (!memcmp(header, "RIFF", 4)) {
			if (!memcmp(header + 8, "WEBPVP8", 7)) {
				compressable = false;
			}
		}
		s->SetPosition(0);
	}
	HRESULT ret;
	if (compressable) {
		UInt64 expectedSize = origSize * 4 / 5;
		OutStreamMemory tmp;
		if ((ret = CoderCompress.Coder->Code(&str, &tmp, &inSizeForReduce, &newSize, this)) != S_OK)
			return ret;
		if (tmp.GetSize() < expectedSize) {
			UInt32 writed;
			if ((ret = strout->Write(tmp.GetInternalBuffer(), tmp.GetSize(), &writed)) != S_OK) {
				return ret;
			}
			if (writed != tmp.GetSize())
				return E_FAIL;
			newDatabase.PackSizes.Add(tmp.GetSize());
			newDatabase.CoderUnpackSizes.Add(origSize);
			newDatabase.NumUnpackStreamsVector.Add(1);
			CFolder &folder = newDatabase.Folders.AddNew();
			folder.Bonds.SetSize(0);
			folder.Coders.SetSize(1);
			CCoderInfo &cod = folder.Coders[0];
			cod.MethodID = nCodecMethod;
			cod.NumStreams = 1;
			cod.Props = props;
			folder.PackStreams.SetSize(1);
			folder.PackStreams[0] = 0; // only 1 stream
			newDatabase.AddFile(file, file2, name);
			return S_OK;
		}
		s->SetPosition(0);
	}
	// direct copy
	newSize = origSize;
	if ((ret = CoderCopy.Coder->Code(&str, strout, &inSizeForReduce, &newSize, this)) != S_OK) {
		return ret;
	}
	newDatabase.PackSizes.Add(s->GetSize());
	newDatabase.CoderUnpackSizes.Add(origSize);
	newDatabase.NumUnpackStreamsVector.Add(1);
	CFolder &folder = newDatabase.Folders.AddNew();
	folder.Bonds.SetSize(0);
	folder.Coders.SetSize(1);
	CCoderInfo &cod = folder.Coders[0];
	cod.MethodID = nCodecMethodCopy;
	cod.NumStreams = 1;
	folder.PackStreams.SetSize(1);
	folder.PackStreams[0] = 0; // only 1 stream
	newDatabase.AddFile(file, file2, name);
	return S_OK;
}

HRESULT XP3ArchiveRepackAsyncImpl::SetRatioInfo(const UInt64 *inSize, const UInt64 *outSize)
{
	if (OnProgress) {
		OnProgress(nTotalSize + *inSize, nArcSize + *inSize, *inSize);
	}
	return bStopRequired ? S_INTERRUPT : S_OK;
}

void XP3ArchiveRepackAsyncImpl::DoConv()
{
	nTotalSize = 0;
	for (unsigned int arcidx = 0; arcidx < ConvFileList.size(); ++arcidx) {
		auto &it = ConvFileList[arcidx];
		if (bStopRequired) break;
		if (OnNewArchive)
			OnNewArchive(arcidx, it.first->TotalSize, it.second);

		tTVPXP3ArchiveEx *xp3arc = it.first;
		tjs_uint totalFile = xp3arc->GetCount();

		std::string tmpname = it.second + ".7z";
		strout = new OutStreamFor7z(tmpname);

		COutArchive archive;
		if (archive.Create(strout, false) != S_OK) {
			if (OnError)
				OnError(-1, "Fail to create output archive.");
			break;
		}
		archive.SkipPrefixArchiveHeader();
		newDatabase.Clear();

		nArcSize = 0;

		// filter image files <idx, mask_img_idx>, img_mask = -1 means no mask available or normal file
		std::map<tjs_uint, tjs_uint> imglist;
		std::vector<tjs_uint> filelist; // normal files
		{
			std::unordered_multimap<ttstr, const tTVPXP3Archive::tArchiveItem*, ttstr_hasher> allFileName;
			std::vector<tjs_uint> allfilelist;
			std::set<tjs_uint> masklist;
			for (tjs_uint idx = 0; idx < xp3arc->ItemVector.size(); ++idx) {
				const tTVPXP3Archive::tArchiveItem &item = xp3arc->ItemVector[idx];
				if (!item.OrgSize) {
					// add empty files first
					CFileItem file;
					CFileItem2 file2;
					file2.Init();
					UString name(item.Name.c_str());
					file.Size = item.OrgSize;
					file.HasStream = false;
					newDatabase.AddFile(file, file2, name);
					continue;
				}
				if (item.Name.length() > 256 && item.Name.StartsWith(TJS_W("$$$")))
					continue;
				ttstr name = TVPChopStorageExt(item.Name);
				allFileName.emplace(name, &item);
				allfilelist.emplace_back(idx);
			}
			if(OptionMergeMaskImg)
			for (tjs_uint idx : allfilelist) {
				const tTVPXP3Archive::tArchiveItem &item = xp3arc->ItemVector[idx];
				ttstr name = TVPChopStorageExt(item.Name);
				tjs_int pos = name.length() - 2;
				if (pos > 0 && name.SubString(pos, 2) == TJS_W("_m")) {
					ttstr prefix = name.SubString(0, pos);
					int count = 0;
					auto itpair = allFileName.equal_range(prefix);
					for (auto it = itpair.first; it != itpair.second; ++it) {
						++count;
					}
					if (count == 1) { // assume that one mask associate to one image
						const tTVPXP3Archive::tArchiveItem* imgItem = itpair.first->second;
						allFileName.erase(itpair.first);
						allFileName.erase(name);
						imglist.emplace(imgItem - &xp3arc->ItemVector.front(), idx);
						masklist.emplace(idx);
						continue;
					}
				}
			}
			for (tjs_uint idx : allfilelist) {
				if (imglist.find(idx) != imglist.end() || masklist.find(idx) != masklist.end())
					continue;
				filelist.push_back(idx);
			}
		}

		// merge image with mask
		// actual TVPLoadGraphicRouter
		static tTVPGraphicHandlerType *handler = TVPGetGraphicLoadHandler(TJS_W(".png"));
		for (const auto &it : imglist) {
			if (bStopRequired) break;
			const tTVPXP3Archive::tArchiveItem &imgItem = xp3arc->ItemVector[it.first];
			const tTVPXP3Archive::tArchiveItem &maskItem = xp3arc->ItemVector[it.second];
			std::auto_ptr<tTJSBinaryStream> strImg(xp3arc->CreateStreamByIndex(&imgItem - &xp3arc->ItemVector.front()));
			std::auto_ptr<tTJSBinaryStream> strMask(xp3arc->CreateStreamByIndex(&maskItem - &xp3arc->ItemVector.front()));
			bool isImage = CheckIsImage(strImg.get()) && CheckIsImage(strMask.get());
			if (isImage) {
				// skip 8-bit image
				iTJSDispatch2 *dic = nullptr;
				handler->Header(strImg.get(), &dic);
				strImg->SetPosition(0);
				if (dic) {
					tTJSVariant val;
					dic->PropGet(0, TJS_W("bpp"), 0, &val, dic);
					dic->Release();
					int bpp = val.AsInteger();
					isImage = bpp == 24 || bpp == 32;
				} else {
					isImage = false; // unknown error, skip
				}
			}
			if (!isImage) {
				filelist.push_back(it.first);
				filelist.push_back(it.second);
				continue;
			}
			struct BmpInfoWithMask {
				tTVPBitmap* bmp = nullptr;
				tTVPBitmap* bmpForMask = nullptr;
				std::unordered_map<ttstr, ttstr, ttstr_hasher> metainfo;
				~BmpInfoWithMask() {
					if (bmp) delete bmp;
					if (bmpForMask) delete bmpForMask;
				}
			} data;

			if (OnNewFile)
				OnNewFile(it.first, imgItem.OrgSize, imgItem.Name.AsStdString());

			// main part
			handler->Load(handler->FormatData, &data,
				[](void *callbackdata, tjs_uint w, tjs_uint h, tTVPGraphicPixelFormat fmt)->int{
				BmpInfoWithMask * data = (BmpInfoWithMask *)callbackdata;
				if (!data->bmp) {
					data->bmp = new tTVPBitmap(w, h, 32);
				}
				return data->bmp->GetPitch();
			}, [](void *callbackdata, tjs_int y)->void* {
				BmpInfoWithMask * data = (BmpInfoWithMask *)callbackdata;
				if (y >= 0) {
					return data->bmp->GetScanLine(y);
				}
				return nullptr;
			}, [](void *callbackdata, const ttstr & name, const ttstr & value) {
				BmpInfoWithMask * data = (BmpInfoWithMask *)callbackdata;
				data->metainfo.emplace(name, value);
			}, strImg.get(), TVP_clNone, glmNormal);

			// mask part
			handler->Load(handler->FormatData, &data,
				[](void *callbackdata, tjs_uint w, tjs_uint h, tTVPGraphicPixelFormat fmt)->int {
				BmpInfoWithMask * data = (BmpInfoWithMask *)callbackdata;
				if (data->bmp->GetWidth() != w || data->bmp->GetHeight() != h)
					TVPThrowExceptionMessage(TVPMaskSizeMismatch);
				if (!data->bmpForMask) {
					data->bmpForMask = new tTVPBitmap(w, h, 8);
				}
				return data->bmpForMask->GetPitch();
			}, [](void *callbackdata, tjs_int y)->void* {
				BmpInfoWithMask * data = (BmpInfoWithMask *)callbackdata;
				if (y >= 0) {
					return data->bmpForMask->GetScanLine(y);
				}
				return nullptr;
			}, [](void *callbackdata, const ttstr & name, const ttstr & value) {
				// no metainfo for mask
			}, strMask.get(), TVP_clNone, glmGrayscale);

			for (tjs_uint y = 0; y < data.bmp->GetHeight(); ++y) {
				TVPBindMaskToMain((tjs_uint32*)data.bmp->GetScanLine(y), (tjs_uint8*)data.bmpForMask->GetScanLine(y), data.bmp->GetWidth());
			}
			delete data.bmpForMask;
			data.bmpForMask = nullptr;
			ncbDictionaryAccessor meta;
			for (const auto &it : data.metainfo) {
				meta.SetValue(it.first.c_str(), it.second);
			}
			tTVPMemoryStream memstr;
			tTVPBaseBitmap *bmp = new tTVPBaseBitmap(data.bmp->GetWidth(), data.bmp->GetHeight());
			bmp->AssignBitmap(data.bmp);

			if (OptionUsingETC2) {
				TVPSavePVRv3(nullptr, &memstr, bmp, TJS_W("ETC2_RGBA"), meta.GetDispatch());
			} else {
				// convert to tlg5 for better performance
				TVPSaveAsTLG(nullptr, &memstr, bmp, TJS_W("tlg5"), meta.GetDispatch());
			}
			delete bmp;
			memstr.SetPosition(0);
			HRESULT ret = AddTo7zArchive(&memstr, imgItem.Name);
			if (OnProgress) {
				uint64_t size = imgItem.OrgSize + maskItem.OrgSize;
				nArcSize += size;
				nTotalSize += size;
				OnProgress(nTotalSize, nArcSize, size);
			}
			if (ret != S_OK) {
				if (ret != S_INTERRUPT && OnError) {
					OnError(ret, "Write to file fail.");
				}
				bStopRequired = true;
				break;
			}
		}
		for (tjs_uint idx : filelist) {
			const tTVPXP3Archive::tArchiveItem& item = xp3arc->ItemVector[idx];
			
			if (bStopRequired) break;
			std::auto_ptr<tTJSBinaryStream> s(xp3arc->CreateStreamByIndex(idx));

			if (OnNewFile)
				OnNewFile(&item - &xp3arc->ItemVector.front(), item.OrgSize, item.Name.AsStdString());

			if (OptionUsingETC2 && item.OrgSize > 8192) {
				// convert image > 8k
				bool isImage = CheckIsImage(s.get());
				int width = 0, height = 0;
				struct BmpInfo {
					tTVPBitmap* bmp = nullptr;
					std::unordered_map<ttstr, ttstr, ttstr_hasher> metainfo;
					tTVPGraphicPixelFormat fmt = gpfLuminance;
					~BmpInfo() {
						if (bmp) delete bmp;
					}
				} data;
				
				if (isImage) {
					// main part
					handler->Load(handler->FormatData, &data,
						[](void *callbackdata, tjs_uint w, tjs_uint h, tTVPGraphicPixelFormat fmt)->int {
						BmpInfo * data = (BmpInfo *)callbackdata;
						if (!data->bmp) {
							data->bmp = new tTVPBitmap(w, h, 32);
							data->fmt = fmt;
						}
						return data->bmp->GetPitch();
					}, [](void *callbackdata, tjs_int y)->void* {
						BmpInfo * data = (BmpInfo *)callbackdata;
						if (y >= 0) {
							return data->bmp->GetScanLine(y);
						}
						return nullptr;
					}, [](void *callbackdata, const ttstr & name, const ttstr & value) {
						BmpInfo * data = (BmpInfo *)callbackdata;
						data->metainfo.emplace(name, value);
					}, s.get(), TVP_clNone, glmNormal);

					// skip 8-bit image
					if (data.fmt != gpfRGB || data.fmt != gpfRGBA) {
						isImage = false;
					}
				}
				if (isImage) {
					tTVPBaseBitmap *bmp = new tTVPBaseBitmap(data.bmp->GetWidth(), data.bmp->GetHeight());
					bmp->AssignBitmap(data.bmp);
					s.reset(new tTVPMemoryStream);
					ncbDictionaryAccessor meta;
					for (const auto &it : data.metainfo) {
						meta.SetValue(it.first.c_str(), it.second);
					}
					TVPSavePVRv3(nullptr, s.get(), bmp,
						data.fmt == gpfRGB ? TJS_W("ETC2_RGB") : TJS_W("ETC2_RGBA"),
						meta.GetDispatch());
					delete bmp;
					s->SetPosition(0);
				}
			}
			HRESULT ret = AddTo7zArchive(s.get(), item.Name);
			nArcSize += item.OrgSize;
			nTotalSize += item.OrgSize;
			if (ret != S_OK) {
				if (ret != S_INTERRUPT && OnError) {
					OnError(ret, "Write to file fail.");
				}
				bStopRequired = true;
				break;
			}
		}
		if (bStopRequired)
			break;
		CCompressionMethodMode mode;
		CHeaderOptions hdropt;
		hdropt.CompressMainHeader = true;
		archive.WriteDatabase(newDatabase, &mode, hdropt);
		archive.Close();
		strout = nullptr;
		delete xp3arc;
		it.first = nullptr;
		remove(it.second.c_str());
		rename(tmpname.c_str(), it.second.c_str());
	}
	Clear();
	if (OnEnded) OnEnded();
}
