#pragma once
// from https://bitbucket.org/jthlim/pvrtccompressor
namespace PvrTcEncoder {
	void EncodeRgba4Bpp(const void *inBuf, void *outBuffer, unsigned int width, unsigned int height, bool isOpaque);
}