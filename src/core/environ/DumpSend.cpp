#include "network/HttpRequest.h"
#include "network/HttpClient.h"
#include "base/CCDirector.h"
#include "base/CCScheduler.h"
#include "base/base64.h"
#include "Platform.h"
#include "SysInitIntf.h"
#include "ConfigManager/LocaleConfigManager.h"
#include "StorageImpl.h"
#include "minizip/ioapi.h"
#include "minizip/zip.h"
#include <sstream>
#include <iomanip>
#include <condition_variable>

static void ClearDumps(const std::string &dumpdir, std::vector<std::string> &allDumps) {
	for (const std::string &path : allDumps) {
		remove((dumpdir + "/" + path).c_str());
	}
	//allDumps.clear();
}

static std::map<std::string, tTVPMemoryStream*> _inmemFiles;

struct zlib_inmem_func64 : public zlib_filefunc64_def {
	static voidpf ZCALLBACK fopen64_file_func(voidpf opaque, const void* filename, int mode) {
		tTVPMemoryStream *str = new tTVPMemoryStream;
		_inmemFiles[(const char*)filename] = str;
		return str;
	}
	
	static uLong ZCALLBACK fread_file_func(voidpf opaque, voidpf stream, void* buf, uLong size) {
		tTVPMemoryStream* str = (tTVPMemoryStream*)stream;
		return str->Read(buf, size);
	}

	static uLong ZCALLBACK fwrite_file_func(voidpf opaque, voidpf stream, const void* buf, uLong size) {
		tTVPMemoryStream* str = (tTVPMemoryStream*)stream;
		return str->Write(buf, size);
	}

	static ZPOS64_T ZCALLBACK ftell64_file_func(voidpf opaque, voidpf stream) {
		tTVPMemoryStream* str = (tTVPMemoryStream*)stream;
		return str->GetPosition();
	}

	static long ZCALLBACK fseek64_file_func(voidpf  opaque, voidpf stream, ZPOS64_T offset, int origin)
	{
		int fseek_origin = TJS_BS_SEEK_SET;
		switch (origin)
		{
		case ZLIB_FILEFUNC_SEEK_CUR:
			fseek_origin = TJS_BS_SEEK_CUR;
			break;
		case ZLIB_FILEFUNC_SEEK_END:
			fseek_origin = TJS_BS_SEEK_END;
			break;
		case ZLIB_FILEFUNC_SEEK_SET:
			fseek_origin = TJS_BS_SEEK_SET;
			break;
		default: return -1;
		}
		tTVPMemoryStream* str = (tTVPMemoryStream*)stream;
		str->Seek(offset, fseek_origin);
		return 0;
	}

	static int ZCALLBACK fclose_file_func(voidpf opaque, voidpf stream)
	{
		return 0;
	}

	static int ZCALLBACK ferror_file_func(voidpf opaque, voidpf stream)
	{
		return 0;
	}

	zlib_inmem_func64() {
		zopen64_file = fopen64_file_func;
		zread_file = fread_file_func;
		zwrite_file = fwrite_file_func;
		ztell64_file = ftell64_file_func;
		zseek64_file = fseek64_file_func;
		zclose_file = fclose_file_func;
		zerror_file = ferror_file_func;
		opaque = NULL;
	}
};

static zlib_filefunc64_def* GetZlibIOFunc() {
	static zlib_inmem_func64 func;
	return &func;
}

static std::string url_encode(const std::string &value) {
	std::ostringstream escaped;
	escaped.fill('0');
	escaped << std::hex;

	for (std::string::value_type c : value) {
		// Keep alphanumeric and other accepted characters intact
		if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
			escaped << c;
			continue;
		}

		// Any other characters are percent-encoded
		escaped << '%' << std::setw(2) << int((unsigned char)c);
	}

	return escaped.str();
}

#define FLAG_UTF8 (1<<11)
static void SendDumps(std::string dumpdir, std::vector<std::string> allDumps, std::string packageName, std::string versionStr) {
	std::mutex _mutex;
	std::condition_variable _cond;
	for (const std::string &filename : allDumps) {
		std::string fullpath = dumpdir + "/" + filename;
		do {
			struct tTVP_stat stat_buf;
			if (!TVP_stat(fullpath.c_str(), stat_buf)) {
				break;
			}

			FILE *fp = fopen(fullpath.c_str(), "rb");
			if (!fp) {
				break;
			}
			std::vector<unsigned char> buf;
			buf.resize(stat_buf.st_size);
			fseek(fp, 0, SEEK_SET);
			fread(&buf[0], 1, stat_buf.st_size, fp);
			fclose(fp);

			zip_fileinfo zi;
			memset(&zi, 0, sizeof zi);

			time_t _t = stat_buf.st_mtime;
			struct tm *time = localtime(&_t);
			zi.tmz_date.tm_year = time->tm_year;
			zi.tmz_date.tm_mon = time->tm_mon;
			zi.tmz_date.tm_mday = time->tm_mday;
			zi.tmz_date.tm_hour = time->tm_hour;
			zi.tmz_date.tm_min = time->tm_min;
			zi.tmz_date.tm_sec = time->tm_sec;

			// CRCÓ‹Ëã
			unsigned long crcFile = 0;
			crcFile = crc32(crcFile, (const Bytef *)&buf[0], buf.size());
			// ¥Õ¥¡¥¤¥ë¤Î×·¼Ó
			// UTF8¤Ç¸ñ¼{¤¹¤ë
			zipFile zf = zipOpen2_64((const void*)filename.c_str(), 0, NULL, GetZlibIOFunc());
			if (zf == NULL) {
				break;
			}
			if (zipOpenNewFileInZip4(zf, filename.c_str(), &zi,
				NULL, 0, NULL, 0, NULL /* comment*/,
				Z_DEFLATED, 9, 0,
				-MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY,
				NULL,
				crcFile, 0, FLAG_UTF8) != ZIP_OK) {
				zipClose(zf, NULL);
				break;
			}
			zipWriteInFileInZip(zf, (const Bytef *)&buf[0], buf.size());
			zipCloseFileInZip(zf);
			zipClose(zf, NULL);

			cocos2d::network::HttpRequest* pRequest = new cocos2d::network::HttpRequest();
			std::string strUrl = 
#ifdef _DEBUG
				"http://127.0.0.1:7777/upload_dump.php"
#else
				"http://avgfun.net/upload_dump.php"
#endif
				;
			pRequest->setUrl(strUrl.c_str());
			pRequest->setRequestType(cocos2d::network::HttpRequest::Type::POST);
			std::ostringstream postData;
			postData << "appid=" << url_encode(packageName);
			postData << "&version=" << url_encode(versionStr);
			postData << "&time=" << _t;
			tTVPMemoryStream *bs = _inmemFiles.begin()->second;
			char *base64str;
			cocos2d::base64Encode((const unsigned char *)bs->GetInternalBuffer(), bs->GetSize(), &base64str);
			postData << "&data=" << url_encode(base64str);
			free(base64str);
			std::string postStr = postData.str();
			pRequest->setRequestData(postStr.c_str(), postStr.length());
			pRequest->setResponseCallback([&](cocos2d::network::HttpClient* client, cocos2d::network::HttpResponse* response){
				_cond.notify_one();
			});
			pRequest->setTag("POST");
			std::unique_lock<std::mutex> lk(_mutex);
			cocos2d::network::HttpClient::getInstance()->send(pRequest);
			_cond.wait(lk);
			pRequest->release();
		} while (false);
		remove(fullpath.c_str());
		for (auto it : _inmemFiles) {
			delete it.second;
		}
		_inmemFiles.clear();
	}
	//allDumps.clear();
}

void TVPCheckAndSendDumps(const std::string &dumpdir, const std::string &packageName, const std::string &versionStr) {
	std::vector<std::string> allDumps;
	TVPListDir(dumpdir, [&](const std::string &name, int mask) {
		if (mask & (S_IFREG | S_IFDIR)) {
			if (name.size() <= 4) return;
			if (name.substr(name.size() - 4) != ".dmp") return;
			allDumps.emplace_back(name);
		}
	});
	if (!allDumps.empty()) {
		std::string title = LocaleConfigManager::GetInstance()->GetText("crash_report");
		std::string msgfmt = LocaleConfigManager::GetInstance()->GetText("crash_report_msg");
		char buf[256];
		sprintf(buf, msgfmt.c_str(), allDumps.size());
		if (TVPShowSimpleMessageBoxYesNo(buf, title) == 0) {
			static std::thread dumpthread;
			dumpthread = std::thread(std::bind(SendDumps, dumpdir, allDumps, packageName, versionStr));
		} else {
			ClearDumps(dumpdir, allDumps);
		}
	}
}