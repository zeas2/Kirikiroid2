#include "astcrt.h"
#include <cmath>
#include <algorithm>
#include <assert.h>
#define DCHECK(x) assert(x)

namespace ASTCRealTimeCodec {

const size_t BLOCK_WIDTH = 4;
const size_t BLOCK_HEIGHT = 4;
const size_t BLOCK_TEXEL_COUNT = BLOCK_WIDTH * BLOCK_HEIGHT;
const size_t BLOCK_BYTES = 16;

const size_t MAXIMUM_ENCODED_WEIGHT_BITS = 96;
const size_t MAXIMUM_ENCODED_WEIGHT_BYTES = 12;

const size_t MAXIMUM_ENCODED_COLOR_ENDPOINT_BYTES = 12;

const size_t MAX_ENDPOINT_VALUE_COUNT = 18;

const int APPROX_COLOR_EPSILON = 50;

template <typename T>
T min(T a, T b) {
	return a <= b ? a : b;
}
template <typename T>
T max(T a, T b) {
	return a >= b ? a : b;
}

template <typename T>
T clamp(T a, T b, T x) {
	if (x < a) {
		return a;
	}

	if (x > b) {
		return b;
	}

	return x;
}

inline bool approx_equal(float x, float y, float epsilon) {
	return fabs(x - y) < epsilon;
}

template <typename T>
union vec3_t {
public:
	vec3_t() {}
	vec3_t(T x_, T y_, T z_) : x(x_), y(y_), z(z_) {}

	struct {
		T x, y, z;
	};
	struct {
		T r, g, b;
	};
	T components[3];
};

typedef vec3_t<float> vec3f_t;
typedef vec3_t<int> vec3i_t;

template <typename T>
vec3_t<T> operator+(vec3_t<T> a, vec3_t<T> b) {
	vec3_t<T> result;
	result.x = a.x + b.x;
	result.y = a.y + b.y;
	result.z = a.z + b.z;
	return result;
}

template <typename T>
vec3_t<T> operator-(vec3_t<T> a, vec3_t<T> b) {
	vec3_t<T> result;
	result.x = a.x - b.x;
	result.y = a.y - b.y;
	result.z = a.z - b.z;
	return result;
}

template <typename T>
vec3_t<T> operator*(vec3_t<T> a, vec3_t<T> b) {
	vec3_t<T> result;
	result.x = a.x * b.x;
	result.y = a.y * b.y;
	result.z = a.z * b.z;
	return result;
}

template <typename T>
vec3_t<T> operator*(vec3_t<T> a, T b) {
	vec3_t<T> result;
	result.x = a.x * b;
	result.y = a.y * b;
	result.z = a.z * b;
	return result;
}

template <typename T>
vec3_t<T> operator/(vec3_t<T> a, T b) {
	vec3_t<T> result;
	result.x = a.x / b;
	result.y = a.y / b;
	result.z = a.z / b;
	return result;
}

template <typename T>
vec3_t<T> operator/(vec3_t<T> a, vec3_t<T> b) {
	vec3_t<T> result;
	result.x = a.x / b.x;
	result.y = a.y / b.y;
	result.z = a.z / b.z;
	return result;
}

template <typename T>
bool operator==(vec3_t<T> a, vec3_t<T> b) {
	return a.x == b.x && a.y == b.y && a.z == b.z;
}

template <typename T>
bool operator!=(vec3_t<T> a, vec3_t<T> b) {
	return a.x != b.x || a.y != b.y || a.z != b.z;
}

template <typename T>
T dot(vec3_t<T> a, vec3_t<T> b) {
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

template <typename T>
T quadrance(vec3_t<T> a) {
	return dot(a, a);
}

template <typename T>
T norm(vec3_t<T> a) {
	return static_cast<T>(sqrt(quadrance(a)));
}

template <typename T>
T distance(vec3_t<T> a, vec3_t<T> b) {
	return norm(a - b);
}

template <typename T>
T qd(vec3_t<T> a, vec3_t<T> b) {
	return quadrance(a - b);
}

template <typename T>
vec3_t<T> signorm(vec3_t<T> a) {
	T x = norm(a);
	DCHECK(x != 0.0);
	return a / x;
}

template <typename T>
vec3_t<T> min(vec3_t<T> a, vec3_t<T> b) {
	vec3_t<T> result;
	result.x = min(a.x, b.x);
	result.y = min(a.y, b.y);
	result.z = min(a.z, b.z);
	return result;
}

template <typename T>
vec3_t<T> max(vec3_t<T> a, vec3_t<T> b) {
	vec3_t<T> result;
	result.x = max(a.x, b.x);
	result.y = max(a.y, b.y);
	result.z = max(a.z, b.z);
	return result;
}

template <typename T>
T qd_to_line(vec3_t<T> m, vec3_t<T> k, T kk, vec3_t<T> p) {
	T t = dot(p - m, k) / kk;
	vec3_t<T> q = k * t + m;
	return qd(p, q);
}

union unorm8_t {
	struct RgbaColorType {
		uint8_t b, g, r, a;
	} channels;
	uint8_t components[4];
	uint32_t bits;
};

union unorm16_t {
	struct RgbaColorType {
		uint16_t b, g, r, a;
	} channels;
	uint16_t components[4];
	uint64_t bits;
};

inline bool is_greyscale(vec3i_t color) {
	// integer equality is transitive
	return color.r == color.g && color.g == color.b;
}

inline int luminance(vec3i_t color) {
	return (color.r + color.g + color.b) / 3;
}

inline bool approx_equal(vec3i_t a, vec3i_t b) {
	return quadrance(a - b) <= APPROX_COLOR_EPSILON;
}

inline vec3i_t clamp_rgb(vec3i_t color) {
	vec3i_t result;
	result.r = clamp(0, 255, color.r);
	result.g = clamp(0, 255, color.g);
	result.b = clamp(0, 255, color.b);
	return result;
}

inline vec3f_t clamp_rgb(vec3f_t color) {
	vec3f_t result;
	result.r = clamp(0.0f, 255.0f, color.r);
	result.g = clamp(0.0f, 255.0f, color.g);
	result.b = clamp(0.0f, 255.0f, color.b);
	return result;
}

inline bool is_rgb(float color) {
	return color >= 0.0f && color <= 255.0f;
}

inline bool is_rgb(vec3f_t color) {
	return is_rgb(color.r) && is_rgb(color.g) && is_rgb(color.b);
}

inline vec3i_t floor(vec3f_t color) {
	vec3i_t result;
	result.r = static_cast<int>(std::floor(color.r));
	result.g = static_cast<int>(std::floor(color.g));
	result.b = static_cast<int>(std::floor(color.b));
	return result;
}

inline vec3i_t round(vec3f_t color) {
	vec3i_t result;
	result.r = static_cast<int>(roundf(color.r));
	result.g = static_cast<int>(roundf(color.g));
	result.b = static_cast<int>(roundf(color.b));
	return result;
}

inline vec3i_t to_vec3i(unorm8_t color) {
	vec3i_t result;
	result.r = color.channels.r;
	result.g = color.channels.g;
	result.b = color.channels.b;
	return result;
}

inline vec3i_t to_vec3i(vec3f_t color) {
	vec3i_t result;
	result.r = static_cast<int>(color.r);
	result.g = static_cast<int>(color.g);
	result.b = static_cast<int>(color.b);
	return result;
}

inline vec3f_t to_vec3f(unorm8_t color) {
	vec3f_t result;
	result.r = color.channels.r;
	result.g = color.channels.g;
	result.b = color.channels.b;
	return result;
}

inline vec3f_t to_vec3f(vec3i_t color) {
	vec3f_t result;
	result.r = static_cast<float>(color.r);
	result.g = static_cast<float>(color.g);
	result.b = static_cast<float>(color.b);
	return result;
}

inline unorm8_t to_unorm8(vec3i_t color) {
	unorm8_t result;
	result.channels.r = static_cast<uint8_t>(color.r);
	result.channels.g = static_cast<uint8_t>(color.g);
	result.channels.b = static_cast<uint8_t>(color.b);
	result.channels.a = 255;
	return result;
}

inline unorm16_t unorm8_to_unorm16(unorm8_t c8) {
	// (x / 255) * (2^16-1) = x * 65535 / 255 = x * 257
	unorm16_t result;
	result.channels.r = static_cast<uint16_t>(c8.channels.r * 257);
	result.channels.g = static_cast<uint16_t>(c8.channels.g * 257);
	result.channels.b = static_cast<uint16_t>(c8.channels.b * 257);
	result.channels.a = static_cast<uint16_t>(c8.channels.a * 257);
	return result;
}

inline bool getbit(size_t number, size_t n) {
	return (number >> n) & 1;
}

inline uint8_t getbits(uint8_t number, uint8_t msb, uint8_t lsb) {
	int count = msb - lsb + 1;
	return static_cast<uint8_t>((number >> lsb) & ((1 << count) - 1));
}

inline size_t getbits(size_t number, size_t msb, size_t lsb) {
	size_t count = msb - lsb + 1;
	return (number >> lsb) & (static_cast<size_t>(1 << count) - 1);
}

inline void orbits8_ptr(uint8_t* ptr,
	size_t bitoffset,
	size_t number,
	size_t bitcount) {
	DCHECK(bitcount <= 8);
	DCHECK((number >> bitcount) == 0);

	size_t index = bitoffset / 8;
	size_t shift = bitoffset % 8;

	// Depending on the offset we might have to consider two bytes when
	// writing, for instance if we are writing 8 bits and the offset is 4,
	// then we have to write 4 bits to the first byte (ptr[index]) and 4 bits
	// to the second byte (ptr[index+1]).
	//
	// FIXME: Writing to the last byte when the number of bytes is a multiple of 2
	// will write past the allocated memory.

	uint8_t* p = ptr + index;
	size_t mask = number << shift;

	DCHECK((p[0] & mask) == 0);
	DCHECK((p[1] & (mask >> 8)) == 0);

	p[0] |= static_cast<uint8_t>(mask & 0xFF);
	p[1] |= static_cast<uint8_t>((mask >> 8) & 0xFF);
}

inline void orbits16_ptr(uint8_t* ptr,
	size_t bitoffset,
	size_t number,
	size_t bitcount) {
	DCHECK(bitcount > 8 && bitcount <= 16);

	size_t index = bitoffset / 8;
	size_t shift = bitoffset % 8;

	uint8_t* p = ptr + index;
	size_t mask = number << shift;

	p[0] |= static_cast<uint8_t>(mask & 0xFF);
	p[1] |= static_cast<uint8_t>((mask >> 8) & 0xFF);
	p[2] |= static_cast<uint8_t>((mask >> 16) & 0xFF);
	p[3] |= static_cast<uint8_t>((mask >> 24) & 0xFF);
}

inline uint16_t getbytes2(const uint8_t* ptr, size_t byteoffset) {
	const uint8_t* p = ptr + byteoffset;
	return static_cast<uint16_t>((p[1] << 8) | p[0]);
}

inline void setbytes2(uint8_t* ptr, size_t byteoffset, uint16_t bytes) {
	ptr[byteoffset + 0] = static_cast<uint8_t>(bytes & 0xFF);
	ptr[byteoffset + 1] = static_cast<uint8_t>((bytes >> 8) & 0xFF);
}

inline void split_high_low(uint8_t n, size_t i, uint8_t& high, uint8_t& low) {
	DCHECK(i < 8);

	uint8_t low_mask = static_cast<uint8_t>((1 << i) - 1);

	low = n & low_mask;
	high = static_cast<uint8_t>(n >> i);
}

class bitwriter {
public:
	explicit bitwriter(uint8_t* ptr) : ptr_(ptr), bitoffset_(0) {
		// assumption that all bits in ptr are zero after the offset

		// writing beyound the bounds of the allocated memory is undefined
		// behaviour
	}

	// Specialized function that can't write more than 8 bits.
	void write8(uint8_t number, size_t bitcount) {
		orbits8_ptr(ptr_, bitoffset_, number, bitcount);

		bitoffset_ += bitcount;
	}

	size_t offset() const { return bitoffset_; }

private:
	uint8_t* ptr_;
	size_t bitoffset_;  // in bits
};

const uint8_t bit_reverse_table[256] = {
	0x00, 0x80, 0x40, 0xC0, 0x20, 0xA0, 0x60, 0xE0, 0x10, 0x90, 0x50, 0xD0,
	0x30, 0xB0, 0x70, 0xF0, 0x08, 0x88, 0x48, 0xC8, 0x28, 0xA8, 0x68, 0xE8,
	0x18, 0x98, 0x58, 0xD8, 0x38, 0xB8, 0x78, 0xF8, 0x04, 0x84, 0x44, 0xC4,
	0x24, 0xA4, 0x64, 0xE4, 0x14, 0x94, 0x54, 0xD4, 0x34, 0xB4, 0x74, 0xF4,
	0x0C, 0x8C, 0x4C, 0xCC, 0x2C, 0xAC, 0x6C, 0xEC, 0x1C, 0x9C, 0x5C, 0xDC,
	0x3C, 0xBC, 0x7C, 0xFC, 0x02, 0x82, 0x42, 0xC2, 0x22, 0xA2, 0x62, 0xE2,
	0x12, 0x92, 0x52, 0xD2, 0x32, 0xB2, 0x72, 0xF2, 0x0A, 0x8A, 0x4A, 0xCA,
	0x2A, 0xAA, 0x6A, 0xEA, 0x1A, 0x9A, 0x5A, 0xDA, 0x3A, 0xBA, 0x7A, 0xFA,
	0x06, 0x86, 0x46, 0xC6, 0x26, 0xA6, 0x66, 0xE6, 0x16, 0x96, 0x56, 0xD6,
	0x36, 0xB6, 0x76, 0xF6, 0x0E, 0x8E, 0x4E, 0xCE, 0x2E, 0xAE, 0x6E, 0xEE,
	0x1E, 0x9E, 0x5E, 0xDE, 0x3E, 0xBE, 0x7E, 0xFE, 0x01, 0x81, 0x41, 0xC1,
	0x21, 0xA1, 0x61, 0xE1, 0x11, 0x91, 0x51, 0xD1, 0x31, 0xB1, 0x71, 0xF1,
	0x09, 0x89, 0x49, 0xC9, 0x29, 0xA9, 0x69, 0xE9, 0x19, 0x99, 0x59, 0xD9,
	0x39, 0xB9, 0x79, 0xF9, 0x05, 0x85, 0x45, 0xC5, 0x25, 0xA5, 0x65, 0xE5,
	0x15, 0x95, 0x55, 0xD5, 0x35, 0xB5, 0x75, 0xF5, 0x0D, 0x8D, 0x4D, 0xCD,
	0x2D, 0xAD, 0x6D, 0xED, 0x1D, 0x9D, 0x5D, 0xDD, 0x3D, 0xBD, 0x7D, 0xFD,
	0x03, 0x83, 0x43, 0xC3, 0x23, 0xA3, 0x63, 0xE3, 0x13, 0x93, 0x53, 0xD3,
	0x33, 0xB3, 0x73, 0xF3, 0x0B, 0x8B, 0x4B, 0xCB, 0x2B, 0xAB, 0x6B, 0xEB,
	0x1B, 0x9B, 0x5B, 0xDB, 0x3B, 0xBB, 0x7B, 0xFB, 0x07, 0x87, 0x47, 0xC7,
	0x27, 0xA7, 0x67, 0xE7, 0x17, 0x97, 0x57, 0xD7, 0x37, 0xB7, 0x77, 0xF7,
	0x0F, 0x8F, 0x4F, 0xCF, 0x2F, 0xAF, 0x6F, 0xEF, 0x1F, 0x9F, 0x5F, 0xDF,
	0x3F, 0xBF, 0x7F, 0xFF };

/**
* Reverse a byte, total function.
*/
inline uint8_t reverse_byte(uint8_t number) {
	return bit_reverse_table[number];
}

/**
* Reverse a sequence of bytes.
*
* Assumes that the bits written to (using bitwise or) are zero and that they
* will not clash with bits already written to target sequence. That is it is
* possible to write to a non-zero byte as long as the bits that are actually
* written to are zero.
*/
inline void reverse_bytes(const uint8_t* source,
	size_t bytecount,
	uint8_t* target) {
	for (int i = 0; i < static_cast<int>(bytecount); ++i) {
		DCHECK((reverse_byte(source[i]) & target[-i]) == 0);
		target[-i] = target[-i] | reverse_byte(source[i]);
	}
}

inline void copy_bytes(const uint8_t* source,
	size_t bytecount,
	uint8_t* target,
	size_t bitoffset) {
	for (size_t i = 0; i < bytecount; ++i) {
		orbits8_ptr(target, bitoffset + i * 8, source[i], 8);
	}
}

struct PhysicalBlock {
	uint8_t data[BLOCK_BYTES];
};

inline void void_extent_to_physical(unorm16_t color, PhysicalBlock* pb) {
	pb->data[0] = 0xFC;
	pb->data[1] = 0xFD;
	pb->data[2] = 0xFF;
	pb->data[3] = 0xFF;
	pb->data[4] = 0xFF;
	pb->data[5] = 0xFF;
	pb->data[6] = 0xFF;
	pb->data[7] = 0xFF;

	setbytes2(pb->data, 8, color.channels.r);
	setbytes2(pb->data, 10, color.channels.g);
	setbytes2(pb->data, 12, color.channels.b);
	setbytes2(pb->data, 14, color.channels.a);
}

enum color_endpoint_mode_t {
	CEM_LDR_LUMINANCE_DIRECT = 0,
	CEM_LDR_LUMINANCE_BASE_OFFSET = 1,
	CEM_HDR_LUMINANCE_LARGE_RANGE = 2,
	CEM_HDR_LUMINANCE_SMALL_RANGE = 3,
	CEM_LDR_LUMINANCE_ALPHA_DIRECT = 4,
	CEM_LDR_LUMINANCE_ALPHA_BASE_OFFSET = 5,
	CEM_LDR_RGB_BASE_SCALE = 6,
	CEM_HDR_RGB_BASE_SCALE = 7,
	CEM_LDR_RGB_DIRECT = 8,
	CEM_LDR_RGB_BASE_OFFSET = 9,
	CEM_LDR_RGB_BASE_SCALE_PLUS_TWO_ALPHA = 10,
	CEM_HDR_RGB = 11,
	CEM_LDR_RGBA_DIRECT = 12,
	CEM_LDR_RGBA_BASE_OFFSET = 13,
	CEM_HDR_RGB_LDR_ALPHA = 14,
	CEM_HDR_RGB_HDR_ALPHA = 15,
	CEM_MAX = 16
};

/**
* Define normalized (starting at zero) numeric ranges that can be represented
* with 8 bits or less.
*/
enum range_t {
	RANGE_2,
	RANGE_3,
	RANGE_4,
	RANGE_5,
	RANGE_6,
	RANGE_8,
	RANGE_10,
	RANGE_12,
	RANGE_16,
	RANGE_20,
	RANGE_24,
	RANGE_32,
	RANGE_40,
	RANGE_48,
	RANGE_64,
	RANGE_80,
	RANGE_96,
	RANGE_128,
	RANGE_160,
	RANGE_192,
	RANGE_256,
	RANGE_MAX
};

/**
* Table of maximum value for each range, minimum is always zero.
*/
const uint8_t range_max_table[RANGE_MAX] = {
	1,  2,  3,  4,   5,   7,   9,
	11, 15, 19, 23,  31,  39,  47,
	63, 79, 95, 127, 159, 191, 255 };


/**
* Table that describes the number of trits or quints along with bits required
* for storing each range.
*/
const uint8_t bits_trits_quints_table[RANGE_MAX][3] = {
	{ 1, 0, 0 },  // RANGE_2
	{ 0, 1, 0 },  // RANGE_3
	{ 2, 0, 0 },  // RANGE_4
	{ 0, 0, 1 },  // RANGE_5
	{ 1, 1, 0 },  // RANGE_6
	{ 3, 0, 0 },  // RANGE_8
	{ 1, 0, 1 },  // RANGE_10
	{ 2, 1, 0 },  // RANGE_12
	{ 4, 0, 0 },  // RANGE_16
	{ 2, 0, 1 },  // RANGE_20
	{ 3, 1, 0 },  // RANGE_24
	{ 5, 0, 0 },  // RANGE_32
	{ 3, 0, 1 },  // RANGE_40
	{ 4, 1, 0 },  // RANGE_48
	{ 6, 0, 0 },  // RANGE_64
	{ 4, 0, 1 },  // RANGE_80
	{ 5, 1, 0 },  // RANGE_96
	{ 7, 0, 0 },  // RANGE_128
	{ 5, 0, 1 },  // RANGE_160
	{ 6, 1, 0 },  // RANGE_192
	{ 8, 0, 0 }   // RANGE_256
};

const uint8_t integer_from_trits[3][3][3][3][3] = {
	{ { { { 0, 1, 2 },{ 4, 5, 6 },{ 8, 9, 10 } },
	{ { 16, 17, 18 },{ 20, 21, 22 },{ 24, 25, 26 } },
	{ { 3, 7, 15 },{ 19, 23, 27 },{ 12, 13, 14 } } },
	{ { { 32, 33, 34 },{ 36, 37, 38 },{ 40, 41, 42 } },
	{ { 48, 49, 50 },{ 52, 53, 54 },{ 56, 57, 58 } },
	{ { 35, 39, 47 },{ 51, 55, 59 },{ 44, 45, 46 } } },
	{ { { 64, 65, 66 },{ 68, 69, 70 },{ 72, 73, 74 } },
	{ { 80, 81, 82 },{ 84, 85, 86 },{ 88, 89, 90 } },
	{ { 67, 71, 79 },{ 83, 87, 91 },{ 76, 77, 78 } } } },
	{ { { { 128, 129, 130 },{ 132, 133, 134 },{ 136, 137, 138 } },
	{ { 144, 145, 146 },{ 148, 149, 150 },{ 152, 153, 154 } },
	{ { 131, 135, 143 },{ 147, 151, 155 },{ 140, 141, 142 } } },
	{ { { 160, 161, 162 },{ 164, 165, 166 },{ 168, 169, 170 } },
	{ { 176, 177, 178 },{ 180, 181, 182 },{ 184, 185, 186 } },
	{ { 163, 167, 175 },{ 179, 183, 187 },{ 172, 173, 174 } } },
	{ { { 192, 193, 194 },{ 196, 197, 198 },{ 200, 201, 202 } },
	{ { 208, 209, 210 },{ 212, 213, 214 },{ 216, 217, 218 } },
	{ { 195, 199, 207 },{ 211, 215, 219 },{ 204, 205, 206 } } } },
	{ { { { 96, 97, 98 },{ 100, 101, 102 },{ 104, 105, 106 } },
	{ { 112, 113, 114 },{ 116, 117, 118 },{ 120, 121, 122 } },
	{ { 99, 103, 111 },{ 115, 119, 123 },{ 108, 109, 110 } } },
	{ { { 224, 225, 226 },{ 228, 229, 230 },{ 232, 233, 234 } },
	{ { 240, 241, 242 },{ 244, 245, 246 },{ 248, 249, 250 } },
	{ { 227, 231, 239 },{ 243, 247, 251 },{ 236, 237, 238 } } },
	{ { { 28, 29, 30 },{ 60, 61, 62 },{ 92, 93, 94 } },
	{ { 156, 157, 158 },{ 188, 189, 190 },{ 220, 221, 222 } },
	{ { 31, 63, 127 },{ 159, 191, 255 },{ 252, 253, 254 } } } } };

/**
* Encode a group of 5 numbers using trits and bits.
*/
inline void encode_trits(size_t bits,
	uint8_t b0,
	uint8_t b1,
	uint8_t b2,
	uint8_t b3,
	uint8_t b4,
	bitwriter& writer) {
	uint8_t t0, t1, t2, t3, t4;
	uint8_t m0, m1, m2, m3, m4;

	split_high_low(b0, bits, t0, m0);
	split_high_low(b1, bits, t1, m1);
	split_high_low(b2, bits, t2, m2);
	split_high_low(b3, bits, t3, m3);
	split_high_low(b4, bits, t4, m4);

	DCHECK(t0 < 3);
	DCHECK(t1 < 3);
	DCHECK(t2 < 3);
	DCHECK(t3 < 3);
	DCHECK(t4 < 3);

	uint8_t packed = integer_from_trits[t4][t3][t2][t1][t0];

	writer.write8(m0, bits);
	writer.write8(getbits(packed, 1, 0), 2);
	writer.write8(m1, bits);
	writer.write8(getbits(packed, 3, 2), 2);
	writer.write8(m2, bits);
	writer.write8(getbits(packed, 4, 4), 1);
	writer.write8(m3, bits);
	writer.write8(getbits(packed, 6, 5), 2);
	writer.write8(m4, bits);
	writer.write8(getbits(packed, 7, 7), 1);
}

const uint8_t integer_from_quints[5][5][5] = 
{ { { 0, 1, 2, 3, 4 },
	{ 8, 9, 10, 11, 12 },
	{ 16, 17, 18, 19, 20 },
	{ 24, 25, 26, 27, 28 },
	{ 5, 13, 21, 29, 6 } },
	{ { 32, 33, 34, 35, 36 },
	{ 40, 41, 42, 43, 44 },
	{ 48, 49, 50, 51, 52 },
	{ 56, 57, 58, 59, 60 },
	{ 37, 45, 53, 61, 14 } },
	{ { 64, 65, 66, 67, 68 },
	{ 72, 73, 74, 75, 76 },
	{ 80, 81, 82, 83, 84 },
	{ 88, 89, 90, 91, 92 },
	{ 69, 77, 85, 93, 22 } },
	{ { 96, 97, 98, 99, 100 },
	{ 104, 105, 106, 107, 108 },
	{ 112, 113, 114, 115, 116 },
	{ 120, 121, 122, 123, 124 },
	{ 101, 109, 117, 125, 30 } },
	{ { 102, 103, 70, 71, 38 },
	{ 110, 111, 78, 79, 46 },
	{ 118, 119, 86, 87, 54 },
	{ 126, 127, 94, 95, 62 },
	{ 39, 47, 55, 63, 31 } } };

/**
* Encode a group of 3 numbers using quints and bits.
*/
inline void encode_quints(size_t bits,
	uint8_t b0,
	uint8_t b1,
	uint8_t b2,
	bitwriter& writer) {
	uint8_t q0, q1, q2;
	uint8_t m0, m1, m2;

	split_high_low(b0, bits, q0, m0);
	split_high_low(b1, bits, q1, m1);
	split_high_low(b2, bits, q2, m2);

	DCHECK(q0 < 5);
	DCHECK(q1 < 5);
	DCHECK(q2 < 5);

	uint8_t packed = integer_from_quints[q2][q1][q0];

	writer.write8(m0, bits);
	writer.write8(getbits(packed, 2, 0), 3);
	writer.write8(m1, bits);
	writer.write8(getbits(packed, 4, 3), 2);
	writer.write8(m2, bits);
	writer.write8(getbits(packed, 6, 5), 2);
}

/**
* Encode a sequence of numbers using using one trit and a custom number of
* bits per number.
*/
inline void encode_trits(const uint8_t* numbers,
	size_t count,
	bitwriter& writer,
	size_t bits) {
	for (size_t i = 0; i < count; i += 5) {
		uint8_t b0 = numbers[i + 0];
		uint8_t b1 = i + 1 >= count ? 0 : numbers[i + 1];
		uint8_t b2 = i + 2 >= count ? 0 : numbers[i + 2];
		uint8_t b3 = i + 3 >= count ? 0 : numbers[i + 3];
		uint8_t b4 = i + 4 >= count ? 0 : numbers[i + 4];

		encode_trits(bits, b0, b1, b2, b3, b4, writer);
	}
}

/**
* Encode a sequence of numbers using one quint and the custom number of bits
* per number.
*/
inline void encode_quints(const uint8_t* numbers,
	size_t count,
	bitwriter& writer,
	size_t bits) {
	for (size_t i = 0; i < count; i += 3) {
		uint8_t b0 = numbers[i + 0];
		uint8_t b1 = i + 1 >= count ? 0 : numbers[i + 1];
		uint8_t b2 = i + 2 >= count ? 0 : numbers[i + 2];
		encode_quints(bits, b0, b1, b2, writer);
	}
}

/**
* Encode a sequence of numbers using binary representation with the selected
* bit count.
*/
inline void encode_binary(const uint8_t* numbers,
	size_t count,
	bitwriter& writer,
	size_t bits) {
	DCHECK(count > 0);
	for (size_t i = 0; i < count; ++i) {
		writer.write8(numbers[i], bits);
	}
}

/**
* Encode a sequence of numbers in a specific range using the binary integer
* sequence encoding. The numbers are assumed to be in the correct range and
* the memory we are writing to is assumed to be zero-initialized.
*/
inline void integer_sequence_encode(const uint8_t* numbers,
	size_t count,
	range_t range,
	bitwriter writer) {
#ifndef NDEBUG
	for (size_t i = 0; i < count; ++i) {
		DCHECK(numbers[i] <= range_max_table[range]);
	}
#endif

	size_t bits = bits_trits_quints_table[range][0];
	size_t trits = bits_trits_quints_table[range][1];
	size_t quints = bits_trits_quints_table[range][2];

	if (trits == 1) {
		encode_trits(numbers, count, writer, bits);
	} else if (quints == 1) {
		encode_quints(numbers, count, writer, bits);
	} else {
		encode_binary(numbers, count, writer, bits);
	}
}

inline void integer_sequence_encode(const uint8_t* numbers,
	size_t count,
	range_t range,
	uint8_t* output) {
	integer_sequence_encode(numbers, count, range, bitwriter(output));
}

/**
* Compute the number of bits required to store a number of items in a specific
* range using the binary integer sequence encoding.
*/
inline size_t compute_ise_bitcount(size_t items, range_t range) {
	size_t bits = bits_trits_quints_table[range][0];
	size_t trits = bits_trits_quints_table[range][1];
	size_t quints = bits_trits_quints_table[range][2];

	if (trits) {
		return ((8 + 5 * bits) * items + 4) / 5;
	}

	if (quints) {
		return ((7 + 3 * bits) * items + 2) / 3;
	}

	return items * bits;
}

inline void symbolic_to_physical(
	color_endpoint_mode_t color_endpoint_mode,
	range_t endpoint_quant,
	range_t weight_quant,

	size_t partition_count,
	size_t partition_index,

	const uint8_t endpoint_ise[MAXIMUM_ENCODED_COLOR_ENDPOINT_BYTES],

	// FIXME: +1 needed here because orbits_8ptr breaks when the offset reaches
	// the last byte which always happens if the weight mode is RANGE_32.
	const uint8_t weights_ise[MAXIMUM_ENCODED_WEIGHT_BYTES + 1],

	PhysicalBlock* pb) {
	DCHECK(weight_quant <= RANGE_32);
	DCHECK(endpoint_quant < RANGE_MAX);
	DCHECK(color_endpoint_mode < CEM_MAX);
	DCHECK(partition_count == 1 || partition_index < 1024);
	DCHECK(partition_count >= 1 && partition_count <= 4);
	DCHECK(compute_ise_bitcount(BLOCK_TEXEL_COUNT, weight_quant) <
		MAXIMUM_ENCODED_WEIGHT_BITS);

	size_t n = BLOCK_WIDTH;
	size_t m = BLOCK_HEIGHT;

	static const bool h_table[RANGE_32 + 1] = {
		0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1 };

	static const uint8_t r_table[RANGE_32 + 1] = {
		0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7 };

	bool h = h_table[weight_quant];
	size_t r = r_table[weight_quant];

	// Use the first row of Table 11 in the ASTC specification. Beware that
	// this has to be changed if another block-size is used.
	size_t a = m - 2;
	size_t b = n - 4;

	bool d = 0;  // TODO: dual plane

	bool multi_part = partition_count > 1;

	size_t part_value = partition_count - 1;
	size_t part_index = multi_part ? partition_index : 0;

	size_t cem_offset = multi_part ? 23 : 13;
	size_t ced_offset = multi_part ? 29 : 17;

	size_t cem_bits = multi_part ? 6 : 4;
	size_t cem = color_endpoint_mode;
	cem = multi_part ? cem << 2 : cem;

	// Block mode
	orbits8_ptr(pb->data, 0, getbit(r, 1), 1);
	orbits8_ptr(pb->data, 1, getbit(r, 2), 1);
	orbits8_ptr(pb->data, 2, 0, 1);
	orbits8_ptr(pb->data, 3, 0, 1);
	orbits8_ptr(pb->data, 4, getbit(r, 0), 1);
	orbits8_ptr(pb->data, 5, a, 2);
	orbits8_ptr(pb->data, 7, b, 2);
	orbits8_ptr(pb->data, 9, h, 1);
	orbits8_ptr(pb->data, 10, d, 1);

	// Partitions
	orbits8_ptr(pb->data, 11, part_value, 2);
	orbits16_ptr(pb->data, 13, part_index, 10);

	// CEM
	orbits8_ptr(pb->data, cem_offset, cem, cem_bits);

	copy_bytes(
		endpoint_ise, MAXIMUM_ENCODED_COLOR_ENDPOINT_BYTES, pb->data, ced_offset);

	reverse_bytes(weights_ise, MAXIMUM_ENCODED_WEIGHT_BYTES, pb->data + 15);
}

const int8_t color_endpoint_range_table[2][12][16] = {
	{ { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	{ 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20 },
	{ 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20 },
	{ 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20 },
	{ 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20 },
	{ 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 19, 19, 19, 19 },
	{ 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 17, 17, 17, 17 },
	{ 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 16, 16, 16, 16 },
	{ 20, 20, 20, 20, 20, 20, 20, 20, 19, 19, 19, 19, 13, 13, 13, 13 },
	{ 20, 20, 20, 20, 20, 20, 20, 20, 16, 16, 16, 16, 11, 11, 11, 11 },
	{ 20, 20, 20, 20, 20, 20, 20, 20, 14, 14, 14, 14, 10, 10, 10, 10 },
	{ 20, 20, 20, 20, 19, 19, 19, 19, 11, 11, 11, 11, 7, 7, 7, 7 } },
	{ { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	{ 20, 20, 20, 20, 20, 20, 20, 20, 14, 14, 14, 14, 9, 9, 9, 9 },
	{ 20, 20, 20, 20, 20, 20, 20, 20, 12, 12, 12, 12, 8, 8, 8, 8 },
	{ 20, 20, 20, 20, 19, 19, 19, 19, 11, 11, 11, 11, 7, 7, 7, 7 },
	{ 20, 20, 20, 20, 17, 17, 17, 17, 10, 10, 10, 10, 6, 6, 6, 6 },
	{ 20, 20, 20, 20, 15, 15, 15, 15, 8, 8, 8, 8, 5, 5, 5, 5 },
	{ 20, 20, 20, 20, 13, 13, 13, 13, 7, 7, 7, 7, 4, 4, 4, 4 },
	{ 20, 20, 20, 20, 11, 11, 11, 11, 6, 6, 6, 6, 3, 3, 3, 3 },
	{ 20, 20, 20, 20, 9, 9, 9, 9, 4, 4, 4, 4, 2, 2, 2, 2 },
	{ 17, 17, 17, 17, 7, 7, 7, 7, 3, 3, 3, 3, 1, 1, 1, 1 },
	{ 14, 14, 14, 14, 5, 5, 5, 5, 2, 2, 2, 2, 0, 0, 0, 0 },
	{ 10, 10, 10, 10, 3, 3, 3, 3, 0, 0, 0, 0, 0, 0, 0, 0 } },
};

// FIXME: This is copied from ARM-code
const uint8_t color_unquantize_table[21][256] = {
	{ 0, 255 },
	{ 0, 128, 255 },
	{ 0, 85, 170, 255 },
	{ 0, 64, 128, 192, 255 },
	{ 0, 255, 51, 204, 102, 153 },
	{ 0, 36, 73, 109, 146, 182, 219, 255 },
	{ 0, 255, 28, 227, 56, 199, 84, 171, 113, 142 },
	{ 0, 255, 69, 186, 23, 232, 92, 163, 46, 209, 116, 139 },
	{ 0, 17, 34, 51, 68, 85, 102, 119, 136, 153, 170, 187, 204, 221, 238, 255 },
	{ 0,  255, 67, 188, 13,  242, 80, 175, 27,  228,
	94, 161, 40, 215, 107, 148, 54, 201, 121, 134 },
	{ 0,  255, 33,  222, 66, 189, 99, 156, 11, 244, 44,  211,
	77, 178, 110, 145, 22, 233, 55, 200, 88, 167, 121, 134 },
	{ 0,   8,   16,  24,  33,  41,  49,  57,  66,  74,  82,
	90,  99,  107, 115, 123, 132, 140, 148, 156, 165, 173,
	181, 189, 198, 206, 214, 222, 231, 239, 247, 255 },
	{ 0,   255, 32,  223, 65, 190, 97, 158, 6,   249, 39,  216, 71, 184,
	104, 151, 13,  242, 45, 210, 78, 177, 110, 145, 19,  236, 52, 203,
	84,  171, 117, 138, 26, 229, 58, 197, 91,  164, 123, 132 },
	{ 0,  255, 16, 239, 32, 223, 48, 207, 65, 190, 81, 174, 97,  158, 113, 142,
	5,  250, 21, 234, 38, 217, 54, 201, 70, 185, 86, 169, 103, 152, 119, 136,
	11, 244, 27, 228, 43, 212, 59, 196, 76, 179, 92, 163, 108, 147, 124, 131 },
	{ 0,   4,   8,   12,  16,  20,  24,  28,  32,  36,  40,  44,  48,
	52,  56,  60,  65,  69,  73,  77,  81,  85,  89,  93,  97,  101,
	105, 109, 113, 117, 121, 125, 130, 134, 138, 142, 146, 150, 154,
	158, 162, 166, 170, 174, 178, 182, 186, 190, 195, 199, 203, 207,
	211, 215, 219, 223, 227, 231, 235, 239, 243, 247, 251, 255 },
	{ 0,  255, 16, 239, 32, 223, 48, 207, 64, 191, 80, 175, 96,  159, 112, 143,
	3,  252, 19, 236, 35, 220, 51, 204, 67, 188, 83, 172, 100, 155, 116, 139,
	6,  249, 22, 233, 38, 217, 54, 201, 71, 184, 87, 168, 103, 152, 119, 136,
	9,  246, 25, 230, 42, 213, 58, 197, 74, 181, 90, 165, 106, 149, 122, 133,
	13, 242, 29, 226, 45, 210, 61, 194, 77, 178, 93, 162, 109, 146, 125, 130 },
	{ 0,   255, 8,   247, 16,  239, 24,  231, 32,  223, 40,  215, 48,  207,
	56,  199, 64,  191, 72,  183, 80,  175, 88,  167, 96,  159, 104, 151,
	112, 143, 120, 135, 2,   253, 10,  245, 18,  237, 26,  229, 35,  220,
	43,  212, 51,  204, 59,  196, 67,  188, 75,  180, 83,  172, 91,  164,
	99,  156, 107, 148, 115, 140, 123, 132, 5,   250, 13,  242, 21,  234,
	29,  226, 37,  218, 45,  210, 53,  202, 61,  194, 70,  185, 78,  177,
	86,  169, 94,  161, 102, 153, 110, 145, 118, 137, 126, 129 },
	{ 0,   2,   4,   6,   8,   10,  12,  14,  16,  18,  20,  22,  24,  26,  28,
	30,  32,  34,  36,  38,  40,  42,  44,  46,  48,  50,  52,  54,  56,  58,
	60,  62,  64,  66,  68,  70,  72,  74,  76,  78,  80,  82,  84,  86,  88,
	90,  92,  94,  96,  98,  100, 102, 104, 106, 108, 110, 112, 114, 116, 118,
	120, 122, 124, 126, 129, 131, 133, 135, 137, 139, 141, 143, 145, 147, 149,
	151, 153, 155, 157, 159, 161, 163, 165, 167, 169, 171, 173, 175, 177, 179,
	181, 183, 185, 187, 189, 191, 193, 195, 197, 199, 201, 203, 205, 207, 209,
	211, 213, 215, 217, 219, 221, 223, 225, 227, 229, 231, 233, 235, 237, 239,
	241, 243, 245, 247, 249, 251, 253, 255 },
	{ 0,   255, 8,   247, 16,  239, 24,  231, 32,  223, 40,  215, 48,  207, 56,
	199, 64,  191, 72,  183, 80,  175, 88,  167, 96,  159, 104, 151, 112, 143,
	120, 135, 1,   254, 9,   246, 17,  238, 25,  230, 33,  222, 41,  214, 49,
	206, 57,  198, 65,  190, 73,  182, 81,  174, 89,  166, 97,  158, 105, 150,
	113, 142, 121, 134, 3,   252, 11,  244, 19,  236, 27,  228, 35,  220, 43,
	212, 51,  204, 59,  196, 67,  188, 75,  180, 83,  172, 91,  164, 99,  156,
	107, 148, 115, 140, 123, 132, 4,   251, 12,  243, 20,  235, 28,  227, 36,
	219, 44,  211, 52,  203, 60,  195, 68,  187, 76,  179, 84,  171, 92,  163,
	100, 155, 108, 147, 116, 139, 124, 131, 6,   249, 14,  241, 22,  233, 30,
	225, 38,  217, 46,  209, 54,  201, 62,  193, 70,  185, 78,  177, 86,  169,
	94,  161, 102, 153, 110, 145, 118, 137, 126, 129 },
	{ 0,   255, 4,   251, 8,   247, 12,  243, 16,  239, 20,  235, 24,  231, 28,
	227, 32,  223, 36,  219, 40,  215, 44,  211, 48,  207, 52,  203, 56,  199,
	60,  195, 64,  191, 68,  187, 72,  183, 76,  179, 80,  175, 84,  171, 88,
	167, 92,  163, 96,  159, 100, 155, 104, 151, 108, 147, 112, 143, 116, 139,
	120, 135, 124, 131, 1,   254, 5,   250, 9,   246, 13,  242, 17,  238, 21,
	234, 25,  230, 29,  226, 33,  222, 37,  218, 41,  214, 45,  210, 49,  206,
	53,  202, 57,  198, 61,  194, 65,  190, 69,  186, 73,  182, 77,  178, 81,
	174, 85,  170, 89,  166, 93,  162, 97,  158, 101, 154, 105, 150, 109, 146,
	113, 142, 117, 138, 121, 134, 125, 130, 2,   253, 6,   249, 10,  245, 14,
	241, 18,  237, 22,  233, 26,  229, 30,  225, 34,  221, 38,  217, 42,  213,
	46,  209, 50,  205, 54,  201, 58,  197, 62,  193, 66,  189, 70,  185, 74,
	181, 78,  177, 82,  173, 86,  169, 90,  165, 94,  161, 98,  157, 102, 153,
	106, 149, 110, 145, 114, 141, 118, 137, 122, 133, 126, 129 },
	{ 0,   1,   2,   3,   4,   5,   6,   7,   8,   9,   10,  11,  12,  13,  14,
	15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,
	30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,
	45,  46,  47,  48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,
	60,  61,  62,  63,  64,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,
	75,  76,  77,  78,  79,  80,  81,  82,  83,  84,  85,  86,  87,  88,  89,
	90,  91,  92,  93,  94,  95,  96,  97,  98,  99,  100, 101, 102, 103, 104,
	105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119,
	120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134,
	135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149,
	150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164,
	165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179,
	180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194,
	195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209,
	210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 224,
	225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239,
	240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254,
	255 } };

// FIXME: This is copied from ARM-code
const uint8_t color_quantize_table[21][256] = {
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 },
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3 },
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
	3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
	4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4 },
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
	4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
	4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
	3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
	4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5,
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
	6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7 },
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 4, 4, 4, 4, 4,
	4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 6,
	6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
	6, 6, 6, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
	9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 5, 5, 5, 5, 5, 5, 5,
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 3, 3, 3,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
	3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
	{ 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  4,  4,  4,  4,  4,  4,  4,
	4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  8,  8,  8,
	8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,
	8,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
	2,  2,  2,  2,  2,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,
	6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  10, 10, 10, 10, 10, 10, 10, 10, 10,
	10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 11, 11, 11, 11, 11,
	11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 7,
	7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,
	7,  7,  7,  7,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
	3,  3,  3,  3,  3,  3,  3,  3,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,
	9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  5,  5,  5,  5,  5,  5,  5,
	5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  1,  1,  1,
	1,  1,  1,  1,  1,  1,  1,  1,  1 },
	{ 0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
	1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
	2,  2,  2,  2,  2,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
	3,  3,  3,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,
	4,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  6,
	6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  7,  7,  7,
	7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  8,  8,  8,  8,  8,
	8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  9,  9,  9,  9,  9,  9,  9,
	9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  10, 10, 10, 10, 10, 10, 10, 10, 10,
	10, 10, 10, 10, 10, 10, 10, 10, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
	11, 11, 11, 11, 11, 11, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
	12, 12, 12, 12, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
	13, 13, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14,
	15, 15, 15, 15, 15, 15, 15, 15, 15 },
	{ 0,  0,  0,  0,  0,  0,  0,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,
	4,  4,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  12, 12, 12, 12,
	12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 16, 16, 16, 16, 16, 16, 16, 16, 16,
	16, 16, 16, 16, 2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  6,  6,
	6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  10, 10, 10, 10, 10, 10, 10,
	10, 10, 10, 10, 10, 10, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14,
	14, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 19, 19, 19, 19, 19,
	19, 19, 19, 19, 19, 19, 19, 19, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 7,  7,  7,
	7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  3,  3,  3,  3,  3,  3,  3,  3,
	3,  3,  3,  3,  3,  17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 13,
	13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 9,  9,  9,  9,  9,  9,
	9,  9,  9,  9,  9,  9,  9,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,
	5,  5,  1,  1,  1,  1,  1,  1,  1 },
	{ 0,  0,  0,  0,  0,  0,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  16, 16,
	16, 16, 16, 16, 16, 16, 16, 16, 16, 2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
	2,  10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 18, 18, 18, 18, 18, 18, 18,
	18, 18, 18, 18, 4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  12, 12, 12, 12,
	12, 12, 12, 12, 12, 12, 12, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 6,
	6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  14, 14, 14, 14, 14, 14, 14, 14, 14,
	14, 14, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 23, 23, 23, 23, 23,
	23, 23, 23, 23, 23, 23, 23, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 7,
	7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  21, 21, 21, 21, 21, 21, 21, 21, 21,
	21, 21, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 5,  5,  5,  5,  5,  5,
	5,  5,  5,  5,  5,  19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 11, 11, 11,
	11, 11, 11, 11, 11, 11, 11, 11, 3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
	17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 9,  9,  9,  9,  9,  9,  9,  9,
	9,  9,  9,  1,  1,  1,  1,  1,  1 },
	{ 0,  0,  0,  0,  0,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,
	2,  2,  3,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  4,  4,  4,  4,
	5,  5,  5,  5,  5,  5,  5,  5,  6,  6,  6,  6,  6,  6,  6,  6,  7,  7,  7,
	7,  7,  7,  7,  7,  8,  8,  8,  8,  8,  8,  8,  8,  8,  9,  9,  9,  9,  9,
	9,  9,  9,  10, 10, 10, 10, 10, 10, 10, 10, 11, 11, 11, 11, 11, 11, 11, 11,
	12, 12, 12, 12, 12, 12, 12, 12, 12, 13, 13, 13, 13, 13, 13, 13, 13, 14, 14,
	14, 14, 14, 14, 14, 14, 15, 15, 15, 15, 15, 15, 15, 15, 16, 16, 16, 16, 16,
	16, 16, 16, 17, 17, 17, 17, 17, 17, 17, 17, 18, 18, 18, 18, 18, 18, 18, 18,
	19, 19, 19, 19, 19, 19, 19, 19, 19, 20, 20, 20, 20, 20, 20, 20, 20, 21, 21,
	21, 21, 21, 21, 21, 21, 22, 22, 22, 22, 22, 22, 22, 22, 23, 23, 23, 23, 23,
	23, 23, 23, 23, 24, 24, 24, 24, 24, 24, 24, 24, 25, 25, 25, 25, 25, 25, 25,
	25, 26, 26, 26, 26, 26, 26, 26, 26, 27, 27, 27, 27, 27, 27, 27, 27, 27, 28,
	28, 28, 28, 28, 28, 28, 28, 29, 29, 29, 29, 29, 29, 29, 29, 30, 30, 30, 30,
	30, 30, 30, 30, 31, 31, 31, 31, 31 },
	{ 0,  0,  0,  0,  8,  8,  8,  8,  8,  8,  16, 16, 16, 16, 16, 16, 16, 24, 24,
	24, 24, 24, 24, 32, 32, 32, 32, 32, 32, 32, 2,  2,  2,  2,  2,  2,  10, 10,
	10, 10, 10, 10, 10, 18, 18, 18, 18, 18, 18, 26, 26, 26, 26, 26, 26, 26, 34,
	34, 34, 34, 34, 34, 4,  4,  4,  4,  4,  4,  4,  12, 12, 12, 12, 12, 12, 20,
	20, 20, 20, 20, 20, 20, 28, 28, 28, 28, 28, 28, 36, 36, 36, 36, 36, 36, 36,
	6,  6,  6,  6,  6,  6,  14, 14, 14, 14, 14, 14, 14, 22, 22, 22, 22, 22, 22,
	30, 30, 30, 30, 30, 30, 30, 38, 38, 38, 38, 38, 38, 38, 39, 39, 39, 39, 39,
	39, 39, 31, 31, 31, 31, 31, 31, 31, 23, 23, 23, 23, 23, 23, 15, 15, 15, 15,
	15, 15, 15, 7,  7,  7,  7,  7,  7,  37, 37, 37, 37, 37, 37, 37, 29, 29, 29,
	29, 29, 29, 21, 21, 21, 21, 21, 21, 21, 13, 13, 13, 13, 13, 13, 5,  5,  5,
	5,  5,  5,  5,  35, 35, 35, 35, 35, 35, 27, 27, 27, 27, 27, 27, 27, 19, 19,
	19, 19, 19, 19, 11, 11, 11, 11, 11, 11, 11, 3,  3,  3,  3,  3,  3,  33, 33,
	33, 33, 33, 33, 33, 25, 25, 25, 25, 25, 25, 17, 17, 17, 17, 17, 17, 17, 9,
	9,  9,  9,  9,  9,  1,  1,  1,  1 },
	{ 0,  0,  0,  16, 16, 16, 16, 16, 16, 32, 32, 32, 32, 32, 2,  2,  2,  2,  2,
	18, 18, 18, 18, 18, 18, 34, 34, 34, 34, 34, 4,  4,  4,  4,  4,  4,  20, 20,
	20, 20, 20, 36, 36, 36, 36, 36, 6,  6,  6,  6,  6,  6,  22, 22, 22, 22, 22,
	38, 38, 38, 38, 38, 38, 8,  8,  8,  8,  8,  24, 24, 24, 24, 24, 24, 40, 40,
	40, 40, 40, 10, 10, 10, 10, 10, 26, 26, 26, 26, 26, 26, 42, 42, 42, 42, 42,
	12, 12, 12, 12, 12, 12, 28, 28, 28, 28, 28, 44, 44, 44, 44, 44, 14, 14, 14,
	14, 14, 14, 30, 30, 30, 30, 30, 46, 46, 46, 46, 46, 46, 47, 47, 47, 47, 47,
	47, 31, 31, 31, 31, 31, 15, 15, 15, 15, 15, 15, 45, 45, 45, 45, 45, 29, 29,
	29, 29, 29, 13, 13, 13, 13, 13, 13, 43, 43, 43, 43, 43, 27, 27, 27, 27, 27,
	27, 11, 11, 11, 11, 11, 41, 41, 41, 41, 41, 25, 25, 25, 25, 25, 25, 9,  9,
	9,  9,  9,  39, 39, 39, 39, 39, 39, 23, 23, 23, 23, 23, 7,  7,  7,  7,  7,
	7,  37, 37, 37, 37, 37, 21, 21, 21, 21, 21, 5,  5,  5,  5,  5,  5,  35, 35,
	35, 35, 35, 19, 19, 19, 19, 19, 19, 3,  3,  3,  3,  3,  33, 33, 33, 33, 33,
	17, 17, 17, 17, 17, 17, 1,  1,  1 },
	{ 0,  0,  0,  1,  1,  1,  1,  2,  2,  2,  2,  3,  3,  3,  3,  4,  4,  4,  4,
	5,  5,  5,  5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  8,  9,  9,  9,
	9,  10, 10, 10, 10, 11, 11, 11, 11, 12, 12, 12, 12, 13, 13, 13, 13, 14, 14,
	14, 14, 15, 15, 15, 15, 16, 16, 16, 16, 16, 17, 17, 17, 17, 18, 18, 18, 18,
	19, 19, 19, 19, 20, 20, 20, 20, 21, 21, 21, 21, 22, 22, 22, 22, 23, 23, 23,
	23, 24, 24, 24, 24, 25, 25, 25, 25, 26, 26, 26, 26, 27, 27, 27, 27, 28, 28,
	28, 28, 29, 29, 29, 29, 30, 30, 30, 30, 31, 31, 31, 31, 32, 32, 32, 32, 33,
	33, 33, 33, 34, 34, 34, 34, 35, 35, 35, 35, 36, 36, 36, 36, 37, 37, 37, 37,
	38, 38, 38, 38, 39, 39, 39, 39, 40, 40, 40, 40, 41, 41, 41, 41, 42, 42, 42,
	42, 43, 43, 43, 43, 44, 44, 44, 44, 45, 45, 45, 45, 46, 46, 46, 46, 47, 47,
	47, 47, 47, 48, 48, 48, 48, 49, 49, 49, 49, 50, 50, 50, 50, 51, 51, 51, 51,
	52, 52, 52, 52, 53, 53, 53, 53, 54, 54, 54, 54, 55, 55, 55, 55, 56, 56, 56,
	56, 57, 57, 57, 57, 58, 58, 58, 58, 59, 59, 59, 59, 60, 60, 60, 60, 61, 61,
	61, 61, 62, 62, 62, 62, 63, 63, 63 },
	{ 0,  0,  16, 16, 16, 32, 32, 32, 48, 48, 48, 48, 64, 64, 64, 2,  2,  2,  18,
	18, 18, 34, 34, 34, 50, 50, 50, 50, 66, 66, 66, 4,  4,  4,  20, 20, 20, 36,
	36, 36, 36, 52, 52, 52, 68, 68, 68, 6,  6,  6,  22, 22, 22, 38, 38, 38, 38,
	54, 54, 54, 70, 70, 70, 8,  8,  8,  24, 24, 24, 24, 40, 40, 40, 56, 56, 56,
	72, 72, 72, 10, 10, 10, 26, 26, 26, 26, 42, 42, 42, 58, 58, 58, 74, 74, 74,
	12, 12, 12, 12, 28, 28, 28, 44, 44, 44, 60, 60, 60, 76, 76, 76, 14, 14, 14,
	14, 30, 30, 30, 46, 46, 46, 62, 62, 62, 78, 78, 78, 78, 79, 79, 79, 79, 63,
	63, 63, 47, 47, 47, 31, 31, 31, 15, 15, 15, 15, 77, 77, 77, 61, 61, 61, 45,
	45, 45, 29, 29, 29, 13, 13, 13, 13, 75, 75, 75, 59, 59, 59, 43, 43, 43, 27,
	27, 27, 27, 11, 11, 11, 73, 73, 73, 57, 57, 57, 41, 41, 41, 25, 25, 25, 25,
	9,  9,  9,  71, 71, 71, 55, 55, 55, 39, 39, 39, 39, 23, 23, 23, 7,  7,  7,
	69, 69, 69, 53, 53, 53, 37, 37, 37, 37, 21, 21, 21, 5,  5,  5,  67, 67, 67,
	51, 51, 51, 51, 35, 35, 35, 19, 19, 19, 3,  3,  3,  65, 65, 65, 49, 49, 49,
	49, 33, 33, 33, 17, 17, 17, 1,  1 },
	{ 0,  0,  32, 32, 64, 64, 64, 2,  2,  2,  34, 34, 66, 66, 66, 4,  4,  4,  36,
	36, 68, 68, 68, 6,  6,  6,  38, 38, 70, 70, 70, 8,  8,  8,  40, 40, 40, 72,
	72, 10, 10, 10, 42, 42, 42, 74, 74, 12, 12, 12, 44, 44, 44, 76, 76, 14, 14,
	14, 46, 46, 46, 78, 78, 16, 16, 16, 48, 48, 48, 80, 80, 80, 18, 18, 50, 50,
	50, 82, 82, 82, 20, 20, 52, 52, 52, 84, 84, 84, 22, 22, 54, 54, 54, 86, 86,
	86, 24, 24, 56, 56, 56, 88, 88, 88, 26, 26, 58, 58, 58, 90, 90, 90, 28, 28,
	60, 60, 60, 92, 92, 92, 30, 30, 62, 62, 62, 94, 94, 94, 95, 95, 95, 63, 63,
	63, 31, 31, 93, 93, 93, 61, 61, 61, 29, 29, 91, 91, 91, 59, 59, 59, 27, 27,
	89, 89, 89, 57, 57, 57, 25, 25, 87, 87, 87, 55, 55, 55, 23, 23, 85, 85, 85,
	53, 53, 53, 21, 21, 83, 83, 83, 51, 51, 51, 19, 19, 81, 81, 81, 49, 49, 49,
	17, 17, 17, 79, 79, 47, 47, 47, 15, 15, 15, 77, 77, 45, 45, 45, 13, 13, 13,
	75, 75, 43, 43, 43, 11, 11, 11, 73, 73, 41, 41, 41, 9,  9,  9,  71, 71, 71,
	39, 39, 7,  7,  7,  69, 69, 69, 37, 37, 5,  5,  5,  67, 67, 67, 35, 35, 3,
	3,  3,  65, 65, 65, 33, 33, 1,  1 },
	{ 0,   0,   1,   1,   2,   2,   3,   3,   4,   4,   5,   5,   6,   6,   7,
	7,   8,   8,   9,   9,   10,  10,  11,  11,  12,  12,  13,  13,  14,  14,
	15,  15,  16,  16,  17,  17,  18,  18,  19,  19,  20,  20,  21,  21,  22,
	22,  23,  23,  24,  24,  25,  25,  26,  26,  27,  27,  28,  28,  29,  29,
	30,  30,  31,  31,  32,  32,  33,  33,  34,  34,  35,  35,  36,  36,  37,
	37,  38,  38,  39,  39,  40,  40,  41,  41,  42,  42,  43,  43,  44,  44,
	45,  45,  46,  46,  47,  47,  48,  48,  49,  49,  50,  50,  51,  51,  52,
	52,  53,  53,  54,  54,  55,  55,  56,  56,  57,  57,  58,  58,  59,  59,
	60,  60,  61,  61,  62,  62,  63,  63,  64,  64,  65,  65,  66,  66,  67,
	67,  68,  68,  69,  69,  70,  70,  71,  71,  72,  72,  73,  73,  74,  74,
	75,  75,  76,  76,  77,  77,  78,  78,  79,  79,  80,  80,  81,  81,  82,
	82,  83,  83,  84,  84,  85,  85,  86,  86,  87,  87,  88,  88,  89,  89,
	90,  90,  91,  91,  92,  92,  93,  93,  94,  94,  95,  95,  96,  96,  97,
	97,  98,  98,  99,  99,  100, 100, 101, 101, 102, 102, 103, 103, 104, 104,
	105, 105, 106, 106, 107, 107, 108, 108, 109, 109, 110, 110, 111, 111, 112,
	112, 113, 113, 114, 114, 115, 115, 116, 116, 117, 117, 118, 118, 119, 119,
	120, 120, 121, 121, 122, 122, 123, 123, 124, 124, 125, 125, 126, 126, 127,
	127 },
	{ 0,   32,  32,  64,  96,  96,  128, 128, 2,   34,  34,  66,  98,  98,  130,
	130, 4,   36,  36,  68,  100, 100, 132, 132, 6,   38,  38,  70,  102, 102,
	134, 134, 8,   40,  40,  72,  104, 104, 136, 136, 10,  42,  42,  74,  106,
	106, 138, 138, 12,  44,  44,  76,  108, 108, 140, 140, 14,  46,  46,  78,
	110, 110, 142, 142, 16,  48,  48,  80,  112, 112, 144, 144, 18,  50,  50,
	82,  114, 114, 146, 146, 20,  52,  52,  84,  116, 116, 148, 148, 22,  54,
	54,  86,  118, 118, 150, 150, 24,  56,  56,  88,  120, 120, 152, 152, 26,
	58,  58,  90,  122, 122, 154, 154, 28,  60,  60,  92,  124, 124, 156, 156,
	30,  62,  62,  94,  126, 126, 158, 158, 159, 159, 127, 127, 95,  63,  63,
	31,  157, 157, 125, 125, 93,  61,  61,  29,  155, 155, 123, 123, 91,  59,
	59,  27,  153, 153, 121, 121, 89,  57,  57,  25,  151, 151, 119, 119, 87,
	55,  55,  23,  149, 149, 117, 117, 85,  53,  53,  21,  147, 147, 115, 115,
	83,  51,  51,  19,  145, 145, 113, 113, 81,  49,  49,  17,  143, 143, 111,
	111, 79,  47,  47,  15,  141, 141, 109, 109, 77,  45,  45,  13,  139, 139,
	107, 107, 75,  43,  43,  11,  137, 137, 105, 105, 73,  41,  41,  9,   135,
	135, 103, 103, 71,  39,  39,  7,   133, 133, 101, 101, 69,  37,  37,  5,
	131, 131, 99,  99,  67,  35,  35,  3,   129, 129, 97,  97,  65,  33,  33,
	1 },
	{ 0,   64,  128, 128, 2,   66,  130, 130, 4,   68,  132, 132, 6,   70,  134,
	134, 8,   72,  136, 136, 10,  74,  138, 138, 12,  76,  140, 140, 14,  78,
	142, 142, 16,  80,  144, 144, 18,  82,  146, 146, 20,  84,  148, 148, 22,
	86,  150, 150, 24,  88,  152, 152, 26,  90,  154, 154, 28,  92,  156, 156,
	30,  94,  158, 158, 32,  96,  160, 160, 34,  98,  162, 162, 36,  100, 164,
	164, 38,  102, 166, 166, 40,  104, 168, 168, 42,  106, 170, 170, 44,  108,
	172, 172, 46,  110, 174, 174, 48,  112, 176, 176, 50,  114, 178, 178, 52,
	116, 180, 180, 54,  118, 182, 182, 56,  120, 184, 184, 58,  122, 186, 186,
	60,  124, 188, 188, 62,  126, 190, 190, 191, 191, 127, 63,  189, 189, 125,
	61,  187, 187, 123, 59,  185, 185, 121, 57,  183, 183, 119, 55,  181, 181,
	117, 53,  179, 179, 115, 51,  177, 177, 113, 49,  175, 175, 111, 47,  173,
	173, 109, 45,  171, 171, 107, 43,  169, 169, 105, 41,  167, 167, 103, 39,
	165, 165, 101, 37,  163, 163, 99,  35,  161, 161, 97,  33,  159, 159, 95,
	31,  157, 157, 93,  29,  155, 155, 91,  27,  153, 153, 89,  25,  151, 151,
	87,  23,  149, 149, 85,  21,  147, 147, 83,  19,  145, 145, 81,  17,  143,
	143, 79,  15,  141, 141, 77,  13,  139, 139, 75,  11,  137, 137, 73,  9,
	135, 135, 71,  7,   133, 133, 69,  5,   131, 131, 67,  3,   129, 129, 65,
	1 },
	{ 0,   1,   2,   3,   4,   5,   6,   7,   8,   9,   10,  11,  12,  13,  14,
	15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,
	30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,
	45,  46,  47,  48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,
	60,  61,  62,  63,  64,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,
	75,  76,  77,  78,  79,  80,  81,  82,  83,  84,  85,  86,  87,  88,  89,
	90,  91,  92,  93,  94,  95,  96,  97,  98,  99,  100, 101, 102, 103, 104,
	105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119,
	120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134,
	135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149,
	150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164,
	165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179,
	180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194,
	195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209,
	210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 224,
	225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239,
	240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254,
	255 } };

range_t endpoint_quantization(size_t partitions,
	range_t weight_quant,
	color_endpoint_mode_t endpoint_mode) {
	int8_t ce_range =
		color_endpoint_range_table[partitions - 1][weight_quant][endpoint_mode];
	DCHECK(ce_range >= 0 && ce_range <= RANGE_MAX);
	return static_cast<range_t>(ce_range);
}

int color_channel_sum(vec3i_t color) {
	return color.r + color.g + color.b;
}

uint8_t quantize_color(range_t quant, int c) {
	DCHECK(c >= 0 && c <= 255);
	return color_quantize_table[quant][c];
}

vec3i_t quantize_color(range_t quant, vec3i_t c) {
	vec3i_t result;
	result.r = color_quantize_table[quant][c.r];
	result.g = color_quantize_table[quant][c.g];
	result.b = color_quantize_table[quant][c.b];
	return result;
}

uint8_t unquantize_color(range_t quant, int c) {
	DCHECK(c >= 0 && c <= 255);
	return color_unquantize_table[quant][c];
}

vec3i_t unquantize_color(range_t quant, vec3i_t c) {
	vec3i_t result;
	result.r = color_unquantize_table[quant][c.r];
	result.g = color_unquantize_table[quant][c.g];
	result.b = color_unquantize_table[quant][c.b];
	return result;
}

void encode_luminance_direct(range_t endpoint_quant,
	int v0,
	int v1,
	uint8_t endpoint_unquantized[2],
	uint8_t endpoint_quantized[2]) {
	endpoint_quantized[0] = quantize_color(endpoint_quant, v0);
	endpoint_quantized[1] = quantize_color(endpoint_quant, v1);
	endpoint_unquantized[0] =
		unquantize_color(endpoint_quant, endpoint_quantized[0]);
	endpoint_unquantized[1] =
		unquantize_color(endpoint_quant, endpoint_quantized[1]);
}

void encode_rgb_direct(range_t endpoint_quant,
	vec3i_t e0,
	vec3i_t e1,
	uint8_t endpoint_quantized[6],
	vec3i_t endpoint_unquantized[2]) {
	vec3i_t e0q = quantize_color(endpoint_quant, e0);
	vec3i_t e1q = quantize_color(endpoint_quant, e1);
	vec3i_t e0u = unquantize_color(endpoint_quant, e0q);
	vec3i_t e1u = unquantize_color(endpoint_quant, e1q);

	// ASTC uses a different blue contraction encoding when the sum of values for
	// the first endpoint is larger than the sum of values in the second
	// endpoint. Sort the endpoints to ensure that the normal encoding is used.
	if (color_channel_sum(e0u) > color_channel_sum(e1u)) {
		endpoint_quantized[0] = static_cast<uint8_t>(e1q.r);
		endpoint_quantized[1] = static_cast<uint8_t>(e0q.r);
		endpoint_quantized[2] = static_cast<uint8_t>(e1q.g);
		endpoint_quantized[3] = static_cast<uint8_t>(e0q.g);
		endpoint_quantized[4] = static_cast<uint8_t>(e1q.b);
		endpoint_quantized[5] = static_cast<uint8_t>(e0q.b);

		endpoint_unquantized[0] = e1u;
		endpoint_unquantized[1] = e0u;
	} else {
		endpoint_quantized[0] = static_cast<uint8_t>(e0q.r);
		endpoint_quantized[1] = static_cast<uint8_t>(e1q.r);
		endpoint_quantized[2] = static_cast<uint8_t>(e0q.g);
		endpoint_quantized[3] = static_cast<uint8_t>(e1q.g);
		endpoint_quantized[4] = static_cast<uint8_t>(e0q.b);
		endpoint_quantized[5] = static_cast<uint8_t>(e1q.b);

		endpoint_unquantized[0] = e0u;
		endpoint_unquantized[1] = e1u;
	}
}

// FIXME: This is copied from ARM-code
const uint8_t weight_quantize_table[12][1025] = {
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 },
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3 },
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4,
	4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
	4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
	4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
	4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
	4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4 },
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 4, 4, 4,
	4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
	4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
	4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
	4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
	4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
	4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
	4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
	4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
	4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	5, 5, 5, 5, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
	3, 3, 3, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
	4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
	4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
	4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
	4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
	4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
	4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
	6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
	6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
	6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
	6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
	6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
	6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7 },
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 4, 4, 4, 4, 4, 4,
	4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
	4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
	4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
	4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
	4, 4, 4, 4, 4, 4, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
	6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
	6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
	6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
	6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
	9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
	9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
	9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
	9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
	9, 9, 9, 9, 9, 9, 9, 9, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 5, 5, 5, 5, 5,
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	5, 5, 5, 5, 5, 5, 5, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
	{ 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,
	4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,
	4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,
	4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,
	4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  8,  8,  8,  8,
	8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,
	8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,
	8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,
	8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,
	8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  2,  2,  2,
	2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
	2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
	2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
	2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
	2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  6,  6,
	6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,
	6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,
	6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,
	6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,
	6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  10, 10, 10, 10, 10, 10, 10, 10, 10,
	10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
	10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
	10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
	10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
	10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
	11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
	11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
	11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
	11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
	11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
	11, 11, 11, 11, 11, 11, 11, 11, 11, 7,  7,  7,  7,  7,  7,  7,  7,  7,  7,
	7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,
	7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,
	7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,
	7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,
	7,  7,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
	3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
	3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
	3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
	3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
	3,  3,  3,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,
	9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,
	9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,
	9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,
	9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,
	9,  9,  9,  9,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,
	5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,
	5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,
	5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,
	5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  1,  1,  1,
	1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
	1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1 },
	{ 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  1,
	1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
	1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
	1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
	1,  1,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
	2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
	2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
	2,  2,  2,  2,  2,  2,  2,  2,  2,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
	3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
	3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
	3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
	3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,
	4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,
	4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,
	4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,
	4,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,
	5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,
	5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,
	5,  5,  5,  5,  5,  5,  5,  5,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,
	6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,
	6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,
	6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  7,  7,  7,  7,
	7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,
	7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,
	7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,
	7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,
	8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,
	8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,
	8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,
	8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,
	8,  8,  8,  8,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,
	9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,
	9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,
	9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  10, 10, 10, 10, 10, 10, 10, 10,
	10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
	10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
	10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 11,
	11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
	11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
	11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
	11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 12, 12, 12, 12, 12,
	12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
	12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
	12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
	12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 13, 13, 13, 13, 13, 13, 13, 13, 13,
	13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
	13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
	13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 14, 14,
	14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14,
	14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14,
	14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14,
	14, 14, 14, 14, 14, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15 },
	{ 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,
	4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,
	4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  8,  8,  8,
	8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,
	8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,
	8,  8,  8,  8,  8,  8,  8,  12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
	12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
	12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
	12, 12, 12, 12, 12, 12, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
	16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
	16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
	16, 16, 16, 16, 16, 2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
	2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
	2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  6,  6,  6,  6,
	6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,
	6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,
	6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  10, 10, 10, 10, 10,
	10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
	10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
	10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 14, 14, 14, 14, 14, 14,
	14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14,
	14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14,
	14, 14, 14, 14, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
	18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
	18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
	18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
	19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
	19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
	19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
	19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
	11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
	11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
	11, 11, 11, 11, 11, 7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,
	7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,
	7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,
	7,  7,  7,  7,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
	3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
	3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  17, 17, 17, 17, 17,
	17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
	17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
	17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 13, 13, 13, 13, 13, 13,
	13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
	13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
	13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 9,  9,  9,  9,  9,  9,  9,
	9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,
	9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,
	9,  9,  9,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,
	5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,
	5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  1,  1,  1,  1,  1,  1,
	1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1 },
	{ 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  8,  8,
	8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,
	8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,
	16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
	16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
	16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 2,  2,  2,  2,  2,  2,  2,  2,  2,
	2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
	2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
	2,  10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
	10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
	10, 10, 10, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
	18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
	18, 18, 18, 18, 18, 4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,
	4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,
	4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  12, 12, 12, 12,
	12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
	12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
	12, 12, 12, 12, 12, 12, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
	20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
	20, 20, 20, 20, 20, 20, 20, 20, 6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,
	6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,
	6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  14, 14, 14, 14, 14, 14, 14, 14, 14,
	14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14,
	14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14,
	14, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
	22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
	22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
	23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
	23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
	23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 15,
	15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15, 15, 7,  7,  7,  7,  7,  7,  7,  7,  7,  7,
	7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,
	7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  21, 21, 21, 21, 21, 21, 21, 21,
	21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21,
	21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 13, 13, 13, 13, 13, 13,
	13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
	13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
	13, 13, 13, 13, 5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,
	5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,
	5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  19, 19, 19, 19, 19,
	19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
	19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 11, 11, 11,
	11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
	11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 3,
	3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
	3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
	3,  3,  3,  3,  3,  3,  3,  3,  3,  17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
	17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
	17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
	9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,
	9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,
	9,  9,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1 },
	{ 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,
	1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
	1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,  2,
	2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
	2,  2,  2,  2,  2,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
	3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  4,
	4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,
	4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  5,  5,  5,  5,  5,  5,  5,
	5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,
	5,  5,  5,  5,  5,  5,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,
	6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,
	7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,
	7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  8,  8,  8,  8,  8,  8,
	8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,
	8,  8,  8,  8,  8,  8,  8,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,
	9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,
	9,  10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
	10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 11, 11, 11, 11, 11,
	11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
	11, 11, 11, 11, 11, 11, 11, 11, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
	12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
	12, 12, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
	13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 14, 14, 14, 14,
	14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14,
	14, 14, 14, 14, 14, 14, 14, 14, 14, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
	16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
	16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
	16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 17, 17, 17, 17, 17, 17, 17, 17, 17,
	17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
	17, 17, 17, 17, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
	18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 19, 19,
	19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
	19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 20, 20, 20, 20, 20, 20, 20, 20,
	20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
	20, 20, 20, 20, 20, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21,
	21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 22,
	22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
	22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 23, 23, 23, 23, 23, 23, 23,
	23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
	23, 23, 23, 23, 23, 23, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
	24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
	25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25,
	25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 26, 26, 26, 26, 26, 26,
	26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26,
	26, 26, 26, 26, 26, 26, 26, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
	27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
	27, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
	28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 29, 29, 29, 29, 29,
	29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
	29, 29, 29, 29, 29, 29, 29, 29, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
	30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
	30, 30, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31 } };

uint8_t quantize_weight(range_t weight_quant, size_t weight) {
	DCHECK(weight_quant <= RANGE_32);
	DCHECK(weight <= 1024);
	return weight_quantize_table[weight_quant][weight];
}

/**
* Project a texel to a line and quantize the result in 1 dimension.
*
* The line is defined by t=k*x + m. This function calculates and quantizes x
* by projecting n=t-m onto k, x=|n|/|k|. Since k and m is derived from the
* minimum and maximum of all texel values the result will be in the range [0,
* 1].
*
* To quantize the result using the weight_quantize_table the value needs to
* be extended to the range [0, 1024].
*
* @param k the derivative of the line
* @param m the minimum endpoint
* @param t the texel value
*/
size_t project(size_t k, size_t m, size_t t) {
	DCHECK(k > 0);
	return size_t((t - m) * 1024) / k;
}

/**
* Project a texel to a line and quantize the result in 3 dimensions.
*/
size_t project(vec3i_t k, int kk, vec3i_t m, vec3i_t t) {
	DCHECK(kk > 0);

	return static_cast<size_t>(clamp(0, 1024, dot(t - m, k) * 1024 / kk));
}

void calculate_quantized_weights_luminance(
	const uint8_t texels[BLOCK_TEXEL_COUNT],
	range_t quant,
	uint8_t l0,
	uint8_t l1,
	uint8_t weights[BLOCK_TEXEL_COUNT]) {
	DCHECK(l0 < l1);

	size_t k = l1 - l0;
	size_t m = l0;

	for (size_t i = 0; i < BLOCK_TEXEL_COUNT; ++i) {
		size_t t = static_cast<size_t>(texels[i]);
		weights[i] = quantize_weight(quant, project(k, m, t));
	}
}

void calculate_quantized_weights_rgb(const unorm8_t texels[BLOCK_TEXEL_COUNT],
	range_t quant,
	vec3i_t e0,
	vec3i_t e1,
	uint8_t weights[BLOCK_TEXEL_COUNT]) {
	if (e0 == e1) {
		for (size_t i = 0; i < BLOCK_TEXEL_COUNT; ++i) {
			weights[i] = 0;  // quantize_weight(quant, 0) is always 0
		}
	} else {
		vec3i_t k = e1 - e0;
		vec3i_t m = e0;

		int kk = dot(k, k);
		for (size_t i = 0; i < BLOCK_TEXEL_COUNT; ++i) {
			weights[i] =
				quantize_weight(quant, project(k, kk, m, to_vec3i(texels[i])));
		}
	}
}

/**
* Write void extent block bits for LDR mode and unused extent coordinates.
*/
void encode_void_extent(vec3i_t color, PhysicalBlock* physical_block) {
	void_extent_to_physical(unorm8_to_unorm16(to_unorm8(color)), physical_block);
}

void encode_luminance(const uint8_t texels[BLOCK_TEXEL_COUNT],
	PhysicalBlock* physical_block) {
	size_t partition_count = 1;
	size_t partition_index = 0;

	color_endpoint_mode_t color_endpoint_mode = CEM_LDR_LUMINANCE_DIRECT;
	range_t weight_quant = RANGE_32;
	range_t endpoint_quant =
		endpoint_quantization(partition_count, weight_quant, color_endpoint_mode);

	uint8_t l0 = 255;
	uint8_t l1 = 0;
	for (size_t i = 0; i < BLOCK_TEXEL_COUNT; ++i) {
		l0 = min(l0, texels[i]);
		l1 = max(l1, texels[i]);
	}

	uint8_t endpoint_unquantized[2];
	uint8_t endpoint_quantized[2];
	encode_luminance_direct(
		endpoint_quant, l0, l1, endpoint_quantized, endpoint_unquantized);

	uint8_t weights_quantized[BLOCK_TEXEL_COUNT];
	calculate_quantized_weights_luminance(texels,
		weight_quant,
		endpoint_unquantized[0],
		endpoint_unquantized[1],
		weights_quantized);

	uint8_t endpoint_ise[MAXIMUM_ENCODED_COLOR_ENDPOINT_BYTES] = { 0 };
	integer_sequence_encode(endpoint_quantized, 2, RANGE_256, endpoint_ise);

	uint8_t weights_ise[MAXIMUM_ENCODED_WEIGHT_BYTES + 1] = { 0 };
	integer_sequence_encode(
		weights_quantized, BLOCK_TEXEL_COUNT, RANGE_32, weights_ise);

	symbolic_to_physical(color_endpoint_mode,
		endpoint_quant,
		weight_quant,
		partition_count,
		partition_index,
		endpoint_ise,
		weights_ise,
		physical_block);
}

void encode_rgb_single_partition(const unorm8_t texels[BLOCK_TEXEL_COUNT],
	vec3f_t e0,
	vec3f_t e1,
	PhysicalBlock* physical_block) {
	size_t partition_index = 0;
	size_t partition_count = 1;

	color_endpoint_mode_t color_endpoint_mode = CEM_LDR_RGB_DIRECT;
	range_t weight_quant = RANGE_12;
	range_t endpoint_quant =
		endpoint_quantization(partition_count, weight_quant, color_endpoint_mode);

	vec3i_t endpoint_unquantized[2];
	uint8_t endpoint_quantized[6];
	encode_rgb_direct(endpoint_quant,
		round(e0),
		round(e1),
		endpoint_quantized,
		endpoint_unquantized);

	uint8_t weights_quantized[BLOCK_TEXEL_COUNT];
	calculate_quantized_weights_rgb(texels,
		weight_quant,
		endpoint_unquantized[0],
		endpoint_unquantized[1],
		weights_quantized);

	uint8_t endpoint_ise[MAXIMUM_ENCODED_COLOR_ENDPOINT_BYTES] = { 0 };
	integer_sequence_encode(endpoint_quantized, 6, endpoint_quant, endpoint_ise);

	uint8_t weights_ise[MAXIMUM_ENCODED_WEIGHT_BYTES + 1] = { 0 };
	integer_sequence_encode(
		weights_quantized, BLOCK_TEXEL_COUNT, weight_quant, weights_ise);

	symbolic_to_physical(color_endpoint_mode,
		endpoint_quant,
		weight_quant,
		partition_count,
		partition_index,
		endpoint_ise,
		weights_ise,
		physical_block);
}

bool is_solid(const unorm8_t texels[BLOCK_TEXEL_COUNT],
	size_t count,
	unorm8_t& color) {
	for (size_t i = 0; i < count; ++i) {
		if (!approx_equal(to_vec3i(texels[i]), to_vec3i(texels[0]))) {
			return false;
		}
	}

	// TODO: Calculate average color?
	color = texels[0];
	return true;
}

bool is_greyscale(const unorm8_t texels[BLOCK_TEXEL_COUNT],
	size_t count,
	uint8_t luminances[BLOCK_TEXEL_COUNT]) {
	for (size_t i = 0; i < count; ++i) {
		vec3i_t color = to_vec3i(texels[i]);
		luminances[i] = static_cast<uint8_t>(luminance(color));
		vec3i_t lum(luminances[i], luminances[i], luminances[i]);
		if (!approx_equal(color, lum)) {
			return false;
		}
	}

	return true;
}

vec3f_t mean(const unorm8_t texels[BLOCK_TEXEL_COUNT], size_t count) {
	vec3i_t sum(0, 0, 0);
	for (size_t i = 0; i < count; ++i) {
		sum = sum + to_vec3i(texels[i]);
	}

	return to_vec3f(sum) / static_cast<float>(count);
}

void subtract(const unorm8_t texels[BLOCK_TEXEL_COUNT],
	size_t count,
	vec3f_t v,
	vec3f_t output[BLOCK_TEXEL_COUNT]) {
	for (size_t i = 0; i < count; ++i) {
		output[i] = to_vec3f(texels[i]) - v;
	}
}

struct mat3x3f_t {
public:
	mat3x3f_t() {}

	mat3x3f_t(float m00,
		float m01,
		float m02,
		float m10,
		float m11,
		float m12,
		float m20,
		float m21,
		float m22) {
		m[0] = vec3f_t(m00, m01, m02);
		m[1] = vec3f_t(m10, m11, m12);
		m[2] = vec3f_t(m20, m21, m22);
	}

	const vec3f_t& row(size_t i) const { return m[i]; }

	float& at(size_t i, size_t j) { return m[i].components[j]; }
	const float& at(size_t i, size_t j) const { return m[i].components[j]; }

private:
	vec3f_t m[3];
};

inline vec3f_t operator*(const mat3x3f_t& a, vec3f_t b) {
	vec3f_t tmp;
	tmp.x = dot(a.row(0), b);
	tmp.y = dot(a.row(1), b);
	tmp.z = dot(a.row(2), b);
	return tmp;
}

void eigen_vector(const mat3x3f_t& a, vec3f_t& eig) {
	vec3f_t b = signorm(vec3f_t(1, 3, 2));  // FIXME: Magic number
	for (size_t i = 0; i < 8; ++i) {
		b = signorm(a * b);
	}

	eig = b;
}

mat3x3f_t covariance(const vec3f_t m[BLOCK_TEXEL_COUNT], size_t count) {
	mat3x3f_t cov;
	for (size_t i = 0; i < 3; ++i) {
		for (size_t j = 0; j < 3; ++j) {
			float s = 0;
			for (size_t k = 0; k < count; ++k) {
				s += m[k].components[i] * m[k].components[j];
			}
			cov.at(i, j) = s / static_cast<float>(count - 1);
		}
	}

	return cov;
}

void principal_component_analysis(const unorm8_t texels[BLOCK_TEXEL_COUNT],
	size_t count,
	vec3f_t& line_k,
	vec3f_t& line_m) {
	// Since we are working with fixed sized blocks count we can cap count. This
	// avoids dynamic allocation.
	DCHECK(count <= BLOCK_TEXEL_COUNT);

	line_m = mean(texels, count);

	vec3f_t n[BLOCK_TEXEL_COUNT];
	subtract(texels, count, line_m, n);

	mat3x3f_t w = covariance(n, count);

	eigen_vector(w, line_k);
}

inline void principal_component_analysis_block(
	const unorm8_t texels[BLOCK_TEXEL_COUNT],
	vec3f_t& line_k,
	vec3f_t& line_m) {
	principal_component_analysis(texels, BLOCK_TEXEL_COUNT, line_k, line_m);
}

void find_min_max(const unorm8_t texels[BLOCK_TEXEL_COUNT],
	size_t count,
	vec3f_t line_k,
	vec3f_t line_m,
	vec3f_t& e0,
	vec3f_t& e1) {
	DCHECK(count <= BLOCK_TEXEL_COUNT);
	DCHECK(approx_equal(quadrance(line_k), 1.0, 0.0001f));

	float a, b;
	{
		float t = dot(to_vec3f(texels[0]) - line_m, line_k);
		a = t;
		b = t;
	}

	for (size_t i = 1; i < count; ++i) {
		float t = dot(to_vec3f(texels[i]) - line_m, line_k);
		a = min(a, t);
		b = max(b, t);
	}

	e0 = clamp_rgb(line_k * a + line_m);
	e1 = clamp_rgb(line_k * b + line_m);
}

void find_min_max_block(const unorm8_t texels[BLOCK_TEXEL_COUNT],
	vec3f_t line_k,
	vec3f_t line_m,
	vec3f_t& e0,
	vec3f_t& e1) {
	find_min_max(texels, BLOCK_TEXEL_COUNT, line_k, line_m, e0, e1);
}

void compress_block(const unorm8_t texels[BLOCK_TEXEL_COUNT],
	PhysicalBlock* physical_block) {
		{
			unorm8_t color;
			if (is_solid(texels, BLOCK_TEXEL_COUNT, color)) {
				encode_void_extent(to_vec3i(color), physical_block);
				/* encode_void_extent(vec3i_t(0, 0, 0), physical_block); */
				return;
			}
		}

		{
			uint8_t luminances[BLOCK_TEXEL_COUNT];
			if (is_greyscale(texels, BLOCK_TEXEL_COUNT, luminances)) {
				encode_luminance(luminances, physical_block);
				/* encode_void_extent(vec3i_t(255, 0, 0), physical_block); */
				return;
			}
		}

		vec3f_t k, m;
		principal_component_analysis_block(texels, k, m);
		vec3f_t e0, e1;
		find_min_max_block(texels, k, m, e0, e1);
		encode_rgb_single_partition(texels, e0, e1, physical_block);
		/* encode_void_extent(vec3i_t(0, 255, 0), physical_block); */
}

void fetch_image_block(const unorm8_t* source,
	size_t image_width,
	size_t xpos,
	size_t ypos,
	unorm8_t texels[BLOCK_TEXEL_COUNT]) {
	size_t topleft_index = ypos * image_width + xpos;

	const unorm8_t* row0 = source + topleft_index;
	const unorm8_t* row1 = row0 + image_width;
	const unorm8_t* row2 = row0 + 2 * image_width;
	const unorm8_t* row3 = row0 + 3 * image_width;

	texels[0] = row0[0];
	texels[1] = row0[1];
	texels[2] = row0[2];
	texels[3] = row0[3];

	texels[4] = row1[0];
	texels[5] = row1[1];
	texels[6] = row1[2];
	texels[7] = row1[3];

	texels[8] = row2[0];
	texels[9] = row2[1];
	texels[10] = row2[2];
	texels[11] = row2[3];

	texels[12] = row3[0];
	texels[13] = row3[1];
	texels[14] = row3[2];
	texels[15] = row3[3];
}

PhysicalBlock physical_block_zero = { 0 };

void compress_texture_4x4(const uint8_t* src,
	uint8_t* dst,
	int width_int,
	int height_int) {
	const unorm8_t* data = reinterpret_cast<const unorm8_t*>(src);

	size_t width = static_cast<size_t>(width_int);
	size_t height = static_cast<size_t>(height_int);

	PhysicalBlock* dst_re = reinterpret_cast<PhysicalBlock*>(dst);

	for (size_t ypos = 0; ypos < height; ypos += BLOCK_WIDTH) {
		for (size_t xpos = 0; xpos < width; xpos += BLOCK_HEIGHT) {
			unorm8_t texels[BLOCK_TEXEL_COUNT];
			fetch_image_block(data, width, xpos, ypos, texels);

			*dst_re = physical_block_zero;
			compress_block(texels, dst_re);

			++dst_re;
		}
	}
}

}