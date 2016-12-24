#include "DemuxPacket.h"
#include <memory>
#include "tp_stub.h"
#include "Clock.h"
extern "C" {
#include "libavcodec/avcodec.h"
}

NS_KRMOVIE_BEGIN
void DemuxPacket::Free(DemuxPacket *pPacket)
{
	if (pPacket)
	{
		try {
			if (pPacket->pData) TJSAlignedDealloc(pPacket->pData);
			delete pPacket;
		} catch (...) {
//			CLog::Log(LOGERROR, "%s - Exception thrown while freeing packet", __FUNCTION__);
		}
	}
}

DemuxPacket * DemuxPacket::Allocate(int iDataSize /*= 0*/)
{
	DemuxPacket* pPacket = new DemuxPacket;
	if (!pPacket) return nullptr;

	try
	{
		memset(pPacket, 0, sizeof(DemuxPacket));

		if (iDataSize > 0)
		{
			// need to allocate a few bytes more.
			// From avcodec.h (ffmpeg)
			/**
			* Required number of additionally allocated bytes at the end of the input bitstream for decoding.
			* this is mainly needed because some optimized bitstream readers read
			* 32 or 64 bit at once and could read over the end<br>
			* Note, if the first 23 bits of the additional bytes are not 0 then damaged
			* MPEG bitstreams could cause overread and segfault
			*/
			pPacket->pData = (uint8_t*)TJSAlignedAlloc(iDataSize + FF_INPUT_BUFFER_PADDING_SIZE, 4);
			if (!pPacket->pData)
			{
				Free(pPacket);
				return NULL;
			}

			// reset the last 8 bytes to 0;
			memset(pPacket->pData + iDataSize, 0, FF_INPUT_BUFFER_PADDING_SIZE);
		}

		// setup defaults
		pPacket->dts = DVD_NOPTS_VALUE;
		pPacket->pts = DVD_NOPTS_VALUE;
		pPacket->iStreamId = -1;
		pPacket->dispTime = 0;
	} catch (...) {
//		CLog::Log(LOGERROR, "%s - Exception thrown", __FUNCTION__);
		Free(pPacket);
		pPacket = nullptr;
	}
	return pPacket;
}

NS_KRMOVIE_END
