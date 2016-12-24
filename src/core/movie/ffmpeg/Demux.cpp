#include "Demux.h"

NS_KRMOVIE_BEGIN
int64_t IDemux::NewGuid()
{
	static int64_t guid = 0;
	return guid++;
}

std::string CDemuxStreamAudio::GetStreamType()
{
	switch (iChannels) {
	case 1: return "Mono";
	case 2: return "Stereo";
	case 6: return "5.1";
	case 8: return "7.1";
	default: return "";
	}
}

NS_KRMOVIE_END
