#include "InputStream.h"
#include "combase.h"

NS_KRMOVIE_BEGIN
InputStream::InputStream(IStream *s, const std::string &filename)
	: m_pSource(s)
	, m_strFileName(filename)
{
	s->AddRef();

	STATSTG stg;
	s->Stat(&stg, STATFLAG_NONAME);
	m_nFileSize = stg.cbSize.QuadPart;
}

InputStream::~InputStream() {
	m_pSource->Release();
}

int InputStream::Read(uint8_t* buf, int buf_size) {
	ULONG readed;
	HRESULT ret = m_pSource->Read(buf, buf_size, &readed);
	if (ret != S_OK) return -1;
	return readed;
}

int64_t InputStream::Seek(int64_t offset, int whence) {
	LARGE_INTEGER pos; pos.QuadPart = offset;
	ULARGE_INTEGER newpos;
	HRESULT ret = m_pSource->Seek(pos, whence, &newpos);
	if (ret != S_OK) return -1;
	return newpos.QuadPart;
}

NS_KRMOVIE_END