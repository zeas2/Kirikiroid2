#pragma once
#include <functional>

// from https://bitbucket.org/wolfpld/etcpak.git

namespace ETCPacker {
	void* convert(const void *pixel, int w, int h, int pitch, bool etc2, size_t &datalen);
	void* convertWithAlpha(const void *pixel, int w, int h, int pitch, size_t &datalen);
	void decode(const void *data, void* pixel, int pitch, int h, int blkw, int blkh);
	void decodeWithAlpha(const void *data, void* pixel, int pitch, int h, int blkw, int blkh);
}
