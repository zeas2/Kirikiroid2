#pragma once
#include "stdint.h"

namespace ASTCRealTimeCodec {
	// from https://github.com/daoo/astcrt

	/**
	* Compress an texture with the ASTC format.
	*
	* @param src The source data, width*height*4 bytes with BGRA ordering.
	* @param dst The output, width*height bytes.
	* @param width The width of the input texture.
	* @param height The height of the input texture.
	*/
	void compress_texture_4x4(const uint8_t* src, uint8_t* dst, int width, int height);
}
