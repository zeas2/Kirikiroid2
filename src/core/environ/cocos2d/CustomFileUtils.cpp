#include "CustomFileUtils.h"

NS_CC_BEGIN

CustomFileUtils::CustomFileUtils()
{
}

void CustomFileUtils::addAutoSearchArchive(const std::string& path)
{
	if (!this->isFileExist(path)) return;
	unzFile file = nullptr;
	file = unzOpen(FileUtils::getInstance()->getSuitableFOpen(path).c_str());
	unz_file_info file_info;
	do {
		unz_file_pos entry;
		if (unzGetFilePos(file, &entry) == UNZ_OK) {
			char filename_inzip[1024];
			if (unzGetCurrentFileInfo(file, &file_info, filename_inzip, sizeof(filename_inzip), NULL, 0, NULL, 0) == UNZ_OK) {
				_autoSearchArchive[filename_inzip] = std::make_pair(file, entry);
			}
		}
	} while (unzGoToNextFile(file) == UNZ_OK);
}

std::string CustomFileUtils::fullPathForFilename(const std::string &filename) const
{
	auto it = _autoSearchArchive.find(filename);
	if (_autoSearchArchive.end() != it) {
		return filename;
	}
	return FileUtilsInherit::fullPathForFilename(filename);
}

unsigned char* CustomFileUtils::getFileData(const std::string& filename, const char* mode, ssize_t *size)
{
	unsigned char* ret = getFileDataFromArchive(filename, size);
	if (ret) return ret;
	return FileUtilsInherit::getFileData(filename, mode, size);
}

bool CustomFileUtils::isFileExistInternal(const std::string& strFilePath) const
{
	auto it = _autoSearchArchive.find(strFilePath);
	if (_autoSearchArchive.end() != it) {
		return true;
	}
	return FileUtilsInherit::isFileExistInternal(strFilePath);
}

bool CustomFileUtils::isDirectoryExistInternal(const std::string& dirPath) const
{
	for (auto &it : _autoSearchArchive) {
		if (it.first.size() <= dirPath.size()) continue;
		if (!strncmp(it.first.c_str(), dirPath.c_str(), dirPath.size()) && it.first[dirPath.size()] == '/') {
			return true;
		}
	}
	return FileUtilsInherit::isDirectoryExistInternal(dirPath);
}

unsigned char* CustomFileUtils::getFileDataFromArchive(const std::string& filename, ssize_t *size)
{
	auto it = _autoSearchArchive.find(filename);
	if (_autoSearchArchive.end() != it) {
		_lock.lock();
		if (unzGoToFilePos(it->second.first, &it->second.second) != UNZ_OK) return nullptr;
		unz_file_info fileInfo;
		if (unzGetCurrentFileInfo(it->second.first, &fileInfo, NULL, 0, NULL, 0, NULL, 0) != UNZ_OK) return nullptr;
		unsigned char *buffer = (unsigned char*)malloc(fileInfo.uncompressed_size);
		int readedSize = unzReadCurrentFile(it->second.first, buffer, static_cast<unsigned>(fileInfo.uncompressed_size));
		_lock.unlock();
		CCASSERT(readedSize == 0 || readedSize == (int)fileInfo.uncompressed_size, "the file size is wrong");
		*size = fileInfo.uncompressed_size;
		return buffer;
	}
	return nullptr;
}

cocos2d::Data CustomFileUtils::getDataFromFile(const std::string& filename)
{
	ssize_t size;
	unsigned char* buffer = getFileDataFromArchive(filename, &size);
	if (buffer) {
		Data ret;
		ret.fastSet(buffer, size);
		return ret;
	}
	return FileUtilsInherit::getDataFromFile(filename);
}

std::string CustomFileUtils::getStringFromFile(const std::string& filename)
{
	Data data = getDataFromFile(filename);
	if (data.isNull())
		return "";

	std::string ret((const char*)data.getBytes());
	return ret;
}

NS_CC_END

cocos2d::CustomFileUtils *TVPCreateCustomFileUtils() {
	cocos2d::CustomFileUtils *ret = new cocos2d::CustomFileUtils;
	ret->init();
	return ret;
}

#include "StorageImpl.h"
#include "Platform.h"
static bool TVPCopyFolder(const std::string &from, const std::string &to) {
	if (!TVPCheckExistentLocalFolder(to) && !TVPCreateFolders(to)) {
		return false;
	}

	bool success = true;
	TVPListDir(from, [&](const std::string &_name, int mask) {
		if (_name == "." || _name == "..") return;
		if (!success) return;
		if (mask & S_IFREG) {
			success = TVPCopyFile(from + "/" + _name, to + "/" + _name);
		}
		else if (mask & S_IFDIR) {
			success = TVPCopyFolder(from + "/" + _name, to + "/" + _name);
		}
	});
	return success;
}

bool TVPCopyFile(const std::string &from, const std::string &to)
{
	FILE * ffrom = fopen(from.c_str(), "rb");
	if (!ffrom) { // try folder copy
		return TVPCopyFolder(from, to);
	}
	FILE * fto = fopen(to.c_str(), "wb");
	if (!fto) {
		if (ffrom) fclose(ffrom);
		return false;
	}
	const int bufSize = 1 * 1024 * 1024;
	std::vector<char> buffer; buffer.resize(bufSize);
	int readed = 0;
	while ((readed = fread(&buffer.front(), 1, bufSize, ffrom))) {
		fwrite(&buffer.front(), 1, readed, fto);
	}
	fclose(ffrom);
	fclose(fto);
	return true;
}
