#pragma once
#include <stdint.h>

#pragma pack(push, 1)
struct PVRv3Header
{
	uint32_t version;
	uint32_t flags;
	uint64_t pixelFormat;
	uint32_t colorSpace;
	uint32_t channelType;
	uint32_t height;
	uint32_t width;
	uint32_t depth;
	uint32_t numberOfSurfaces;
	uint32_t numberOfFaces;
	uint32_t numberOfMipmaps;
	uint32_t metadataLength;
};
enum class PVR3TexturePixelFormat : uint64_t
{
	PVRTC2BPP_RGB = 0ULL,
	PVRTC2BPP_RGBA = 1ULL,
	PVRTC4BPP_RGB = 2ULL,
	PVRTC4BPP_RGBA = 3ULL,
	PVRTC2_2BPP_RGBA = 4ULL,
	PVRTC2_4BPP_RGBA = 5ULL,
	ETC1 = 6ULL,
	DXT1 = 7ULL,
	DXT2 = 8ULL,
	DXT3 = 9ULL,
	DXT4 = 10ULL,
	DXT5 = 11ULL,
	BC1 = 7ULL,
	BC2 = 9ULL,
	BC3 = 11ULL,
	BC4 = 12ULL,
	BC5 = 13ULL,
	BC6 = 14ULL,
	BC7 = 15ULL,
	UYVY = 16ULL,
	YUY2 = 17ULL,
	BW1bpp = 18ULL,
	R9G9B9E5 = 19ULL,
	RGBG8888 = 20ULL,
	GRGB8888 = 21ULL,
	ETC2_RGB = 22ULL,
	ETC2_RGBA = 23ULL,
	ETC2_RGBA1 = 24ULL,
	EAC_R11_Unsigned = 25ULL,
	EAC_R11_Signed = 26ULL,
	EAC_RG11_Unsigned = 27ULL,
	EAC_RG11_Signed = 28ULL,

	BGRA8888 = 0x0808080861726762ULL,
	RGBA8888 = 0x0808080861626772ULL,
	RGBA4444 = 0x0404040461626772ULL,
	RGBA5551 = 0x0105050561626772ULL,
	RGB565 = 0x0005060500626772ULL,
	RGB888 = 0x0008080800626772ULL,
	A8 = 0x0000000800000061ULL,
	L8 = 0x000000080000006cULL,
	LA88 = 0x000008080000616cULL,
};
#pragma pack(pop)