#pragma once
#include "Ref.h"
#include <string>
#include <cstdint>

struct IStream;

NS_KRMOVIE_BEGIN
class InputStream : public IRef<InputStream> {
	IStream *m_pSource;
	uint64_t m_nFileSize;
	std::string m_strFileName;

public:
	InputStream(IStream *s, const std::string &filename);
	~InputStream();

	uint64_t GetLength() { return m_nFileSize; }
	int Read(uint8_t* buf, int buf_size);
	int64_t Seek(int64_t offset, int whence);
	const std::string &GetFileName() { return m_strFileName; }
};

NS_KRMOVIE_END