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

using namespace TJS;
using namespace NArchive::N7z;

#define COMPRESS_THRESHOLD 4 * 1024 * 1024

void TVPSetXP3FilterScript(ttstr content);

class tTVPXP3ArchiveEx : public tTVPXP3Archive {
public:
	tTVPXP3ArchiveEx(const ttstr & name, tTJSBinaryStream *st) : tTVPXP3Archive(name, 0) {
		Init(st, -1, false);
	}
};

class OutStreamFor7z : public IOutStream {
	long fp;
	unsigned int RefCount = 1;

public:
	OutStreamFor7z(const std::string &path) {
		open(path.c_str(), O_RDWR | O_CREAT, 0666);
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

XP3ArchiveRepackAsync::XP3ArchiveRepackAsync(const std::string &xp3filter)
{
	if (!g_CrcTable[1]) CrcGenerateTable();
	ttstr xp3filterScript;
	iTJSTextReadStream * stream = TVPCreateTextStreamForRead(xp3filter, "");
	stream->Read(xp3filterScript, 0);
	delete stream;
	TVPSetXP3FilterScript(xp3filterScript);
}

XP3ArchiveRepackAsync::~XP3ArchiveRepackAsync()
{
	std::thread* t = (std::thread*)Thread;
	if (t && t->joinable()) {
		t->join();
		delete t;
	}
	Clear();
}

uint64_t XP3ArchiveRepackAsync::AddTask(const std::string &src/*, const std::string &dst*/)
{
	uint64_t totalSize = 0;
	ttstr path(src);
	tTVPLocalFileStream *str = new tTVPLocalFileStream(path, path, TJS_BS_READ);
	tTVPXP3ArchiveEx *xp3arc = new tTVPXP3ArchiveEx(path, str);
	for (const tTVPXP3Archive::tArchiveItem &item : xp3arc->ItemVector) {
		totalSize += item.OrgSize;
	}
	ConvFileList.emplace_back(xp3arc, src);
	return totalSize;
}

void XP3ArchiveRepackAsync::Clear()
{
	for (auto & it : ConvFileList) {
		delete it.first;
	}
	ConvFileList.clear();
	TVPSetXP3FilterScript(TJS_W(""));
}

void XP3ArchiveRepackAsync::Start()
{
	if (!Thread) {
		Thread = new std::thread(std::bind(&XP3ArchiveRepackAsync::DoConv, this));
	}
}

void XP3ArchiveRepackAsync::DoConv()
{
	for (auto &it : ConvFileList) {
		tTVPXP3ArchiveEx *xp3arc = it.first;
		tjs_uint totalFile = xp3arc->GetCount();

		std::string tmpname = it.second + ".7z";
		OutStreamFor7z *strout = new OutStreamFor7z(tmpname);

		const int nCodecMethod = 0x30101, // LZMA
			nCodecMethodCopy = 0;
		CCreatedCoder coder, coderCopy;
		CreateCoder(nCodecMethod, true, coder);
		CreateCoder(nCodecMethodCopy, true, coderCopy);
		CMyComPtr<ICompressWriteCoderProperties> writeCoderProperties;
		CMyComPtr<ICompressSetCoderProperties> setCoderProperties;
		coder.Coder->QueryInterface(IID_ICompressWriteCoderProperties, (void **)&writeCoderProperties);
		coder.Coder->QueryInterface(IID_ICompressSetCoderProperties, (void **)&setCoderProperties);
		CByteBuffer props;

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

		COutArchive archive;
		archive.Create(strout, false);
		archive.SkipPrefixArchiveHeader();
		CArchiveDatabaseOut newDatabase;

		for (const tTVPXP3Archive::tArchiveItem &item : xp3arc->ItemVector) {
			if (!item.OrgSize) {
				CFileItem file;
				CFileItem2 file2;
				file2.Init();
				UString name(item.Name.c_str());
				file.Size = item.OrgSize;
				file.HasStream = false;
				newDatabase.AddFile(file, file2, name);
			}
		}
		for (const tTVPXP3Archive::tArchiveItem &item : xp3arc->ItemVector) {
			if (item.OrgSize) {
				CFileItem file;
				CFileItem2 file2;
				file2.Init();
				UString name(item.Name.c_str());
				file.Size = item.OrgSize;
				std::auto_ptr<tTJSBinaryStream> s(xp3arc->CreateStreamByIndex(&item - &xp3arc->ItemVector.front()));
				if (UsingETC2 && item.OrgSize > 8192) {
					// convert image > 8k
					tjs_uint8 header[16];
					s->Read(header, 16);
					bool isImage = false, isOpaque = false;
					if (!memcmp(header, "BM", 2)) {
						isImage = true;
					} else if (!memcmp(header, "\x89PNG", 4)) {
						isImage = true;
					} else if (!memcmp(header, "\xFF\xD8\xFF", 3)) { // JPEG
						isImage = true;
					} else if (!memcmp(header, "TLG", 3)) {
						isImage = true;
					} else if (!memcmp(header, "\x49\x49\xbc\x01", 4)) { // JXR
						isImage = true;
					} else if (!memcmp(header, "BPG", 3)) {
						isImage = true;
					} else if (!memcmp(header, "RIFF", 4)) {
						if (!memcmp(header + 8, "WEBPVP8", 7)) {
							isImage = true;
						}
					}
					// actual TVPLoadGraphicRouter
					static tTVPGraphicHandlerType *handler = TVPGetGraphicLoadHandler(TJS_W(".png"));
					if (isImage) {
						// skip 8-bit image
						s->SetPosition(0);
						iTJSDispatch2 *dic = nullptr;
						handler->Header(s.get(), &dic);
						if (dic) {
							dic->Release();
							tTJSVariant val;
							dic->PropGet(0, TJS_W("bpp"), 0, &val, dic);
							int bpp = val.AsInteger();
							isOpaque = bpp == 24;
							isImage = isOpaque || bpp == 32;
						} else {
							isImage = false; // unknown error, skip
						}
					}
					if (isImage) {
						// TODO merge mask file
						s->SetPosition(0);
						typedef std::tuple<int, int, int, std::vector<tjs_uint8> > PicInfo;
						PicInfo picinfo;
						handler->Load(nullptr, &picinfo, [](void *callbackdata, tjs_uint w, tjs_uint h, tTVPGraphicPixelFormat fmt)->int {
							PicInfo &picinfo = *(PicInfo *)callbackdata;
							std::get<0>(picinfo) = w;
							std::get<1>(picinfo) = h;
							int pitch = w * 4;
							std::get<2>(picinfo) = pitch;
							std::get<3>(picinfo).resize(pitch * h);
							return w * 4;
						}, [](void *callbackdata, int y)->void* {
							PicInfo &picinfo = *(PicInfo *)callbackdata;
							int pitch = std::get<2>(picinfo);
							return &std::get<3>(picinfo)[pitch * y];
						}, [](void *callbackdata, const ttstr & name, const ttstr & value) {
						}, s.get(), 0, glmNormal);
						
						int w = std::get<0>(picinfo);
						int h = std::get<1>(picinfo);
						int pitch = std::get<2>(picinfo);
						uint8_t *imgdata = nullptr;
						size_t datalen;
						uint64_t pvr_pixfmt;
						if (isOpaque) {
							imgdata = (uint8_t *)ETCPacker::convert(&std::get<3>(picinfo).front(), w, h, pitch, true, datalen);
							pvr_pixfmt = 22; // ETC2_RGB
						} else {
							imgdata = (uint8_t *)ETCPacker::convertWithAlpha(&std::get<3>(picinfo).front(), w, h, pitch, datalen);
							pvr_pixfmt = 23; // ETC2_RGBA
						}

						tTVPMemoryStream * memstr = new tTVPMemoryStream;
						// in pvr v3 container
						memstr->WriteBuffer("PVR\3", 4);
						memstr->WriteBuffer("\0\0\0\0", 4); // no premultiply alpha
						memstr->WriteBuffer(&pvr_pixfmt, sizeof(pvr_pixfmt));
						memstr->WriteBuffer("\0\0\0\0", 4); // colorSpace
						memstr->WriteBuffer("\0\0\0\0", 4); // channelType
						memstr->WriteBuffer(&h, sizeof(h));
						memstr->WriteBuffer(&w, sizeof(w));
						memstr->WriteBuffer("\1\0\0\0", 4); // depth
						memstr->WriteBuffer("\1\0\0\0", 4); // numberOfSurfaces
						memstr->WriteBuffer("\1\0\0\0", 4); // numberOfFaces
						memstr->WriteBuffer("\1\0\0\0", 4); // numberOfMipmaps
						memstr->WriteBuffer("\0\0\0\0", 4); // metadataLength
						memstr->WriteBuffer(imgdata, datalen);
						delete[] imgdata;
						memstr->SetPosition(0);
						s.reset(memstr);
					}
				}

				SequentialInStreamFor7z str(s.get());
				tjs_uint64 origSize = s->GetSize();
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
				if (compressable) {
					UInt64 expectedSize = origSize * 4 / 5;
					OutStreamMemory tmp;
					coder.Coder->Code(&str, &tmp, &inSizeForReduce, &newSize, nullptr);
					if (tmp.GetSize() < expectedSize) {
						UInt32 writed;
						strout->Write(tmp.GetInternalBuffer(), tmp.GetSize(), &writed);
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
						continue;
					}
					s->SetPosition(0);
				}
				// direct copy
				newSize = origSize;
				coderCopy.Coder->Code(&str, strout, &inSizeForReduce, &newSize, nullptr);
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
			}
		}
		CCompressionMethodMode mode;
		CHeaderOptions hdropt;
		hdropt.CompressMainHeader = true;
		archive.WriteDatabase(newDatabase, &mode, hdropt);
		archive.Close();
		delete xp3arc;
		remove(it.second.c_str());
		rename(tmpname.c_str(), it.second.c_str());
	}
	ConvFileList.clear();
}
