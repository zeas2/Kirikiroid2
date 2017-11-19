#include "etcpak.h"
#include <array>
#include <assert.h>
#include <cstdint>
#include <algorithm>
#include <condition_variable>
#include <mutex>
#include <future>
#include <cmath>
#include "ThreadIntf.h"
#include "tvpgl.h"

#define _bswap(x) ((x&0xFF)<<24)|((x&0xFF00)<<8)|((x&0xFF0000)>>8)|(x>>24)
namespace std {
	float _fma_(float x, float y, float z) { return fmaf(x, y, z); }
}
#define fma _fma_
float _log2_(float n) {
	return log(n) / M_LN2;
}
#define log2 _log2_

namespace ETCPacker {
typedef int8_t      int8;
typedef uint8_t     uint8;
typedef int16_t     int16;
typedef uint16_t    uint16;
typedef int32_t     int32;
typedef uint32_t    uint32;
typedef int64_t     int64;
typedef uint64_t    uint64;

typedef unsigned int uint;

// takes as input a value, returns the value clamped to the interval [0,255].
// NO WARRANTY --- SEE STATEMENT IN TOP OF FILE (C) Ericsson AB 2013. All Rights Reserved.
static int clamp(int val)
{
	if (val < 0)
		val = 0;
	if (val > 255)
		val = 255;
	return val;
}
static int alphaTableInitialized = 0;
static int alphaTable[256][8];
static int alphaBase[16][4] = {
	{ -15,-9,-6,-3 },
	{ -13,-10,-7,-3 },
	{ -13,-8,-5,-2 },
	{ -13,-6,-4,-2 },
	{ -12,-8,-6,-3 },
	{ -11,-9,-7,-3 },
	{ -11,-8,-7,-4 },
	{ -11,-8,-5,-3 },
	{ -10,-8,-6,-2 },
	{ -10,-8,-5,-2 },
	{ -10,-8,-4,-2 },
	{ -10,-7,-5,-2 },
	{ -10,-7,-4,-3 },
	{ -10,-3,-2, -1 },
	{ -9,-8,-6,-4 },
	{ -9,-7,-5,-3 }
};

// Table for fast implementationi of clamping to the interval [0,255]
static const int clamp_table[768] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 };

static void setupAlphaTable()
{
	if (alphaTableInitialized)
		return;
	alphaTableInitialized = 1;

	//read table used for alpha compression
	int buf;
	for (int i = 16; i < 32; i++)
	{
		for (int j = 0; j < 8; j++)
		{
			buf = alphaBase[i - 16][3 - j % 4];
			if (j < 4)
				alphaTable[i][j] = buf;
			else
				alphaTable[i][j] = (-buf - 1);
		}
	}

	//beyond the first 16 values, the rest of the table is implicit.. so calculate that!
	for (int i = 0; i < 256; i++)
	{
		//fill remaining slots in table with multiples of the first ones.
		int mul = i / 16;
		int old = 16 + i % 16;
		for (int j = 0; j < 8; j++)
		{
			alphaTable[i][j] = alphaTable[old][j] * mul;
			//note: we don't do clamping here, though we could, because we'll be clamped afterwards anyway.
		}
	}
}

static uint8 getbit(uint8 input, int frompos, int topos)
{
	uint8 output = 0;
	if (frompos > topos)
		return ((1 << frompos)&input) >> (frompos - topos);
	return ((1 << frompos)&input) << (topos - frompos);
}

// Compresses the alpha part of a GL_COMPRESSED_RGBA8_ETC2_EAC block.
// NO WARRANTY --- SEE STATEMENT IN TOP OF FILE (C) Ericsson AB 2013. All Rights Reserved.
void compressBlockAlphaFast(uint8 * data, uint8* returnData)
{
	int alphasum = 0;
	int maxdist = -2;
	bool solid = true;
	for (int x = 0; x < 16; x++) {
		alphasum += data[x];
	}
	int alpha = (int)(((float)alphasum) / 16.0f + 0.5f); //average pixel value, used as guess for base value.
	for (int x = 0; x < 16; x++) {
		if (abs(alpha - data[x])>maxdist)
			maxdist = abs(alpha - data[x]); //maximum distance from average
	}
	if (maxdist == 0) {
		// fast route for solid block
		memset(returnData, 0, 8);
		returnData[0] = alpha;
		return;
	}
	int approxPos = (maxdist * 255) / 160 - 4;  //experimentally derived formula for calculating approximate table position given a max distance from average
	if (approxPos > 255)
		approxPos = 255;
	int startTable = approxPos - 15; //first table to be tested
	if (startTable < 0)
		startTable = 0;
	int endTable = clamp(approxPos + 15);  //last table to be tested

	int bestsum = 1000000000;
	int besttable = -3;
	int bestalpha = 128;
	int prevalpha = alpha;

	//main loop: determine best base alpha value and offset table to use for compression
	//try some different alpha tables.
	for (int table = startTable; table < endTable&&bestsum>0; table++)
	{
		int tablealpha = prevalpha;
		int tablebestsum = 1000000000;
		//test some different alpha values, trying to find the best one for the given table.	
		for (int alphascale = 16; alphascale > 0; alphascale /= 4)
		{
			int startalpha;
			int endalpha;
			if (alphascale == 16)
			{
				startalpha = clamp(tablealpha - alphascale * 4);
				endalpha = clamp(tablealpha + alphascale * 4);
			} else
			{
				startalpha = clamp(tablealpha - alphascale * 2);
				endalpha = clamp(tablealpha + alphascale * 2);
			}
			for (alpha = startalpha; alpha <= endalpha; alpha += alphascale)
			{
				int sum = 0;
				int val, diff, bestdiff = 10000000, index;
				for (int x = 0; x < 16; x++)
				{
				//	for (int y = 0; y < 4; y++)
					{
						//compute best offset here, add square difference to sum..
						val = data[x];
						bestdiff = 1000000000;
						//the values are always ordered from small to large, with the first 4 being negative and the last 4 positive
						//search is therefore made in the order 0-1-2-3 or 7-6-5-4, stopping when error increases compared to the previous entry tested.
						if (val > alpha)
						{
							for (index = 7; index > 3; index--)
							{
								diff = clamp_table[alpha + (int)(alphaTable[table][index]) + 255] - val;
								diff *= diff;
								if (diff <= bestdiff)
								{
									bestdiff = diff;
								} else
									break;
							}
						} else
						{
							for (index = 0; index < 4; index++)
							{
								diff = clamp_table[alpha + (int)(alphaTable[table][index]) + 255] - val;
								diff *= diff;
								if (diff < bestdiff)
								{
									bestdiff = diff;
								} else
									break;
							}
						}

						//best diff here is bestdiff, add it to sum!
						sum += bestdiff;
						//if the sum here is worse than previously best already, there's no use in continuing the count..
						//note that tablebestsum could be used for more precise estimation, but the speedup gained here is deemed more important.
						if (sum > bestsum)
						{
							x = 9999; //just to make it large and get out of the x<4 loop
							break;
						}
					}
				}
				if (sum < tablebestsum)
				{
					tablebestsum = sum;
					tablealpha = alpha;
				}
				if (sum < bestsum)
				{
					bestsum = sum;
					besttable = table;
					bestalpha = alpha;
				}
			}
			if (alphascale <= 2)
				alphascale = 0;
		}
	}

	alpha = bestalpha;

	//"good" alpha value and table are known!
	//store them, then loop through the pixels again and print indices.

	returnData[0] = alpha;
	returnData[1] = besttable;
	for (int pos = 2; pos < 8; pos++)
	{
		returnData[pos] = 0;
	}
	int byte = 2;
	int bit = 0;
	for (int x = 0; x < 16; x++)
	{
	//	for (int y = 0; y < 4; y++)
		{
			//find correct index
			int besterror = 1000000;
			int bestindex = 99;
			for (int index = 0; index < 8; index++) //no clever ordering this time, as this loop is only run once per block anyway
			{
				int error = (clamp(alpha + (int)(alphaTable[besttable][index])) - data[x])*(clamp(alpha + (int)(alphaTable[besttable][index])) - data[x]);
				if (error < besterror)
				{
					besterror = error;
					bestindex = index;
				}
			}
			//best table index has been determined.
			//pack 3-bit index into compressed data, one bit at a time
			for (int numbit = 0; numbit < 3; numbit++)
			{
				returnData[byte] |= getbit(bestindex, 2 - numbit, 7 - bit);

				bit++;
				if (bit > 7)
				{
					bit = 0;
					byte++;
				}
			}
		}
	}
}

template<typename T>
inline T AlignPOT(T val)
{
	if (val == 0) return 1;
	val--;
	for (unsigned int i = 1; i < sizeof(T) * 8; i <<= 1)
	{
		val |= val >> i;
	}
	return val + 1;
}

inline int CountSetBits(uint32 val)
{
	val -= (val >> 1) & 0x55555555;
	val = ((val >> 2) & 0x33333333) + (val & 0x33333333);
	val = ((val >> 4) + val) & 0x0f0f0f0f;
	val += val >> 8;
	val += val >> 16;
	return val & 0x0000003f;
}

inline int CountLeadingZeros(uint32 val)
{
	val |= val >> 1;
	val |= val >> 2;
	val |= val >> 4;
	val |= val >> 8;
	val |= val >> 16;
	return 32 - CountSetBits(val);
}

inline float sRGB2linear(float v)
{
	const float a = 0.055f;
	if (v <= 0.04045f)
	{
		return v / 12.92f;
	} else
	{
		return std::pow((v + a) / (1 + a), 2.4f);
	}
}

inline float linear2sRGB(float v)
{
	const float a = 0.055f;
	if (v <= 0.0031308f)
	{
		return 12.92f * v;
	} else
	{
		return (1 + a) * std::pow(v, 1 / 2.4f) - a;
	}
}

template<class T>
inline T SmoothStep(T x)
{
	return x*x*(3 - 2 * x);
}

inline uint8 clampu8(int32 val)
{
	return std::min(std::max(0, val), 255);
}

template<class T>
inline T sq(T val)
{
	return val * val;
}

static inline int mul8bit(int a, int b)
{
	int t = a*b + 128;
	return (t + (t >> 8)) >> 8;
}

template<class T>
struct Vector2
{
	Vector2() : x(0), y(0) {}
	Vector2(T v) : x(v), y(v) {}
	Vector2(T _x, T _y) : x(_x), y(_y) {}

	bool operator==(const Vector2<T>& rhs) const { return x == rhs.x && y == rhs.y; }
	bool operator!=(const Vector2<T>& rhs) const { return !(*this == rhs); }

	Vector2<T>& operator+=(const Vector2<T>& rhs)
	{
		x += rhs.x;
		y += rhs.y;
		return *this;
	}
	Vector2<T>& operator-=(const Vector2<T>& rhs)
	{
		x -= rhs.x;
		y -= rhs.y;
		return *this;
	}
	Vector2<T>& operator*=(const Vector2<T>& rhs)
	{
		x *= rhs.x;
		y *= rhs.y;
		return *this;
	}

	T x, y;
};

template<class T>
Vector2<T> operator+(const Vector2<T>& lhs, const Vector2<T>& rhs)
{
	return Vector2<T>(lhs.x + rhs.x, lhs.y + rhs.y);
}

template<class T>
Vector2<T> operator-(const Vector2<T>& lhs, const Vector2<T>& rhs)
{
	return Vector2<T>(lhs.x - rhs.x, lhs.y - rhs.y);
}

template<class T>
Vector2<T> operator*(const Vector2<T>& lhs, const float& rhs)
{
	return Vector2<T>(lhs.x * rhs, lhs.y * rhs);
}

template<class T>
Vector2<T> operator/(const Vector2<T>& lhs, const T& rhs)
{
	return Vector2<T>(lhs.x / rhs, lhs.y / rhs);
}


typedef Vector2<int32> v2i;
typedef Vector2<float> v2f;


template<class T>
struct Vector3
{
	Vector3() : x(0), y(0), z(0) {}
	Vector3(T v) : x(v), y(v), z(v) {}
	Vector3(T _x, T _y, T _z) : x(_x), y(_y), z(_z) {}
	template<class Y>
	Vector3(const Vector3<Y>& v) : x(T(v.x)), y(T(v.y)), z(T(v.z)) {}

	T Luminance() const { return T(x * 0.3f + y * 0.59f + z * 0.11f); }
	void Clamp()
	{
		x = std::min(T(1), std::max(T(0), x));
		y = std::min(T(1), std::max(T(0), y));
		z = std::min(T(1), std::max(T(0), z));
	}

	bool operator==(const Vector3<T>& rhs) const { return x == rhs.x && y == rhs.y && z == rhs.z; }
	bool operator!=(const Vector2<T>& rhs) const { return !(*this == rhs); }

	T& operator[](uint idx) { assert(idx < 3); return ((T*)this)[idx]; }
	const T& operator[](uint idx) const { assert(idx < 3); return ((T*)this)[idx]; }

	Vector3<T> operator+=(const Vector3<T>& rhs)
	{
		x += rhs.x;
		y += rhs.y;
		z += rhs.z;
		return *this;
	}

	Vector3<T> operator*=(const Vector3<T>& rhs)
	{
		x *= rhs.x;
		y *= rhs.y;
		z *= rhs.z;
		return *this;
	}

	Vector3<T> operator*=(const float& rhs)
	{
		x *= rhs;
		y *= rhs;
		z *= rhs;
		return *this;
	}

	T x, y, z;
	T padding;
};

template<class T>
Vector3<T> operator+(const Vector3<T>& lhs, const Vector3<T>& rhs)
{
	return Vector3<T>(lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z);
}

template<class T>
Vector3<T> operator-(const Vector3<T>& lhs, const Vector3<T>& rhs)
{
	return Vector3<T>(lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z);
}

template<class T>
Vector3<T> operator*(const Vector3<T>& lhs, const Vector3<T>& rhs)
{
	return Vector3<T>(lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z);
}

template<class T>
Vector3<T> operator*(const Vector3<T>& lhs, const float& rhs)
{
	return Vector3<T>(T(lhs.x * rhs), T(lhs.y * rhs), T(lhs.z * rhs));
}

template<class T>
Vector3<T> operator/(const Vector3<T>& lhs, const T& rhs)
{
	return Vector3<T>(lhs.x / rhs, lhs.y / rhs, lhs.z / rhs);
}

template<class T>
bool operator<(const Vector3<T>& lhs, const Vector3<T>& rhs)
{
	return lhs.Luminance() < rhs.Luminance();
}

typedef Vector3<int32> v3i;
typedef Vector3<float> v3f;
typedef Vector3<uint8> v3b;


static inline v3b v3f_to_v3b(const v3f& v)
{
	return v3b(uint8(std::min(1.f, v.x) * 255), uint8(std::min(1.f, v.y) * 255), uint8(std::min(1.f, v.z) * 255));
}

template<class T>
Vector3<T> Mix(const Vector3<T>& v1, const Vector3<T>& v2, float amount)
{
	return v1 + (v2 - v1) * amount;
}

template<>
inline v3b Mix(const v3b& v1, const v3b& v2, float amount)
{
	return v3b(v3f(v1) + (v3f(v2) - v3f(v1)) * amount);
}

template<class T>
Vector3<T> Desaturate(const Vector3<T>& v)
{
	T l = v.Luminance();
	return Vector3<T>(l, l, l);
}

template<class T>
Vector3<T> Desaturate(const Vector3<T>& v, float mul)
{
	T l = T(v.Luminance() * mul);
	return Vector3<T>(l, l, l);
}

template<class T>
Vector3<T> pow(const Vector3<T>& base, float exponent)
{
	return Vector3<T>(
		std::pow(base.x, exponent),
		std::pow(base.y, exponent),
		std::pow(base.z, exponent));
}

template<class T>
Vector3<T> sRGB2linear(const Vector3<T>& v)
{
	return Vector3<T>(
		sRGB2linear(v.x),
		sRGB2linear(v.y),
		sRGB2linear(v.z));
}

template<class T>
Vector3<T> linear2sRGB(const Vector3<T>& v)
{
	return Vector3<T>(
		linear2sRGB(v.x),
		linear2sRGB(v.y),
		linear2sRGB(v.z));
}

enum class Channels
{
	RGB,
	Alpha
};

class Bitmap
{
public:
	Bitmap(const v2i& size);
	Bitmap(const v2i& size, const void *pixelData, int pitch, uint lines);
	virtual ~Bitmap();

	uint32* Data() { if (m_load.valid()) m_load.wait(); return m_data; }
	const uint32* Data() const { if (m_load.valid()) m_load.wait(); return m_data; }
	const v2i& Size() const { return m_size; }
	bool Alpha() const { return m_alpha; }

	const uint32* NextBlock(uint& lines, bool& done);

protected:
	Bitmap(const Bitmap& src, uint lines);

	uint32* m_data;
	uint32* m_block;
	uint m_lines;
	uint m_linesLeft;
	v2i m_size;
	bool m_alpha;
//	Semaphore m_sema;
//	std::mutex m_lock;
	std::future<void> m_load;
};

typedef std::shared_ptr<Bitmap> BitmapPtr;

struct DataPart
{
	const uint32* src;
	uint width;
	uint lines;
	uint offset;
};

const int32 g_table[8][4] = {
    {  2,  8,   -2,   -8 },
    {  5, 17,   -5,  -17 },
    {  9, 29,   -9,  -29 },
    { 13, 42,  -13,  -42 },
    { 18, 60,  -18,  -60 },
    { 24, 80,  -24,  -80 },
    { 33, 106, -33, -106 },
    { 47, 183, -47, -183 }
};

const int64 g_table256[8][4] = {
    {  2*256,  8*256,   -2*256,   -8*256 },
    {  5*256, 17*256,   -5*256,  -17*256 },
    {  9*256, 29*256,   -9*256,  -29*256 },
    { 13*256, 42*256,  -13*256,  -42*256 },
    { 18*256, 60*256,  -18*256,  -60*256 },
    { 24*256, 80*256,  -24*256,  -80*256 },
    { 33*256, 106*256, -33*256, -106*256 },
    { 47*256, 183*256, -47*256, -183*256 }
};

const uint32 g_id[4][16] = {
    { 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 3, 3, 2, 2, 3, 3, 2, 2, 3, 3, 2, 2, 3, 3, 2, 2 },
    { 5, 5, 5, 5, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4 },
    { 7, 7, 6, 6, 7, 7, 6, 6, 7, 7, 6, 6, 7, 7, 6, 6 }
};

const uint32 g_avg2[16] = {
    0x00,
    0x11,
    0x22,
    0x33,
    0x44,
    0x55,
    0x66,
    0x77,
    0x88,
    0x99,
    0xAA,
    0xBB,
    0xCC,
    0xDD,
    0xEE,
    0xFF
};

const uint32 g_flags[64] = {
    0x80800402, 0x80800402, 0x80800402, 0x80800402,
    0x80800402, 0x80800402, 0x80800402, 0x8080E002,
    0x80800402, 0x80800402, 0x8080E002, 0x8080E002,
    0x80800402, 0x8080E002, 0x8080E002, 0x8080E002,
    0x80000402, 0x80000402, 0x80000402, 0x80000402,
    0x80000402, 0x80000402, 0x80000402, 0x8000E002,
    0x80000402, 0x80000402, 0x8000E002, 0x8000E002,
    0x80000402, 0x8000E002, 0x8000E002, 0x8000E002,
    0x00800402, 0x00800402, 0x00800402, 0x00800402,
    0x00800402, 0x00800402, 0x00800402, 0x0080E002,
    0x00800402, 0x00800402, 0x0080E002, 0x0080E002,
    0x00800402, 0x0080E002, 0x0080E002, 0x0080E002,
    0x00000402, 0x00000402, 0x00000402, 0x00000402,
    0x00000402, 0x00000402, 0x00000402, 0x0000E002,
    0x00000402, 0x00000402, 0x0000E002, 0x0000E002,
    0x00000402, 0x0000E002, 0x0000E002, 0x0000E002
};

template<class T>
static size_t GetLeastError(const T* err, size_t num)
{
	size_t idx = 0;
	for (size_t i = 1; i < num; i++)
	{
		if (err[i] < err[idx])
		{
			idx = i;
		}
	}
	return idx;
}

static uint64 FixByteOrder(uint64 d)
{
	return ((d & 0x00000000FFFFFFFF)) |
		((d & 0xFF00000000000000) >> 24) |
		((d & 0x000000FF00000000) << 24) |
		((d & 0x00FF000000000000) >> 8) |
		((d & 0x0000FF0000000000) << 8);
}

template<class T, class S>
static uint64 EncodeSelectors(uint64 d, const T terr[2][8], const S tsel[16][8], const uint32* id)
{
	size_t tidx[2];
	tidx[0] = GetLeastError(terr[0], 8);
	tidx[1] = GetLeastError(terr[1], 8);

	d |= tidx[0] << 26;
	d |= tidx[1] << 29;
	for (int i = 0; i < 16; i++)
	{
		uint64 t = tsel[i][tidx[id[i] % 2]];
		d |= (t & 0x1) << (i + 32);
		d |= (t & 0x2) << (i + 47);
	}

	return d;
}

namespace
{

typedef std::array<uint16, 4> v4i;

void Average( const uint8* data, v4i* a )
{
#ifdef __SSE4_1__
    __m128i d0 = _mm_loadu_si128(((__m128i*)data) + 0);
    __m128i d1 = _mm_loadu_si128(((__m128i*)data) + 1);
    __m128i d2 = _mm_loadu_si128(((__m128i*)data) + 2);
    __m128i d3 = _mm_loadu_si128(((__m128i*)data) + 3);

    __m128i d0l = _mm_unpacklo_epi8(d0, _mm_setzero_si128());
    __m128i d0h = _mm_unpackhi_epi8(d0, _mm_setzero_si128());
    __m128i d1l = _mm_unpacklo_epi8(d1, _mm_setzero_si128());
    __m128i d1h = _mm_unpackhi_epi8(d1, _mm_setzero_si128());
    __m128i d2l = _mm_unpacklo_epi8(d2, _mm_setzero_si128());
    __m128i d2h = _mm_unpackhi_epi8(d2, _mm_setzero_si128());
    __m128i d3l = _mm_unpacklo_epi8(d3, _mm_setzero_si128());
    __m128i d3h = _mm_unpackhi_epi8(d3, _mm_setzero_si128());

    __m128i sum0 = _mm_add_epi16(d0l, d1l);
    __m128i sum1 = _mm_add_epi16(d0h, d1h);
    __m128i sum2 = _mm_add_epi16(d2l, d3l);
    __m128i sum3 = _mm_add_epi16(d2h, d3h);

    __m128i sum0l = _mm_unpacklo_epi16(sum0, _mm_setzero_si128());
    __m128i sum0h = _mm_unpackhi_epi16(sum0, _mm_setzero_si128());
    __m128i sum1l = _mm_unpacklo_epi16(sum1, _mm_setzero_si128());
    __m128i sum1h = _mm_unpackhi_epi16(sum1, _mm_setzero_si128());
    __m128i sum2l = _mm_unpacklo_epi16(sum2, _mm_setzero_si128());
    __m128i sum2h = _mm_unpackhi_epi16(sum2, _mm_setzero_si128());
    __m128i sum3l = _mm_unpacklo_epi16(sum3, _mm_setzero_si128());
    __m128i sum3h = _mm_unpackhi_epi16(sum3, _mm_setzero_si128());

    __m128i b0 = _mm_add_epi32(sum0l, sum0h);
    __m128i b1 = _mm_add_epi32(sum1l, sum1h);
    __m128i b2 = _mm_add_epi32(sum2l, sum2h);
    __m128i b3 = _mm_add_epi32(sum3l, sum3h);

    __m128i a0 = _mm_srli_epi32(_mm_add_epi32(_mm_add_epi32(b2, b3), _mm_set1_epi32(4)), 3);
    __m128i a1 = _mm_srli_epi32(_mm_add_epi32(_mm_add_epi32(b0, b1), _mm_set1_epi32(4)), 3);
    __m128i a2 = _mm_srli_epi32(_mm_add_epi32(_mm_add_epi32(b1, b3), _mm_set1_epi32(4)), 3);
    __m128i a3 = _mm_srli_epi32(_mm_add_epi32(_mm_add_epi32(b0, b2), _mm_set1_epi32(4)), 3);

    _mm_storeu_si128((__m128i*)&a[0], _mm_packus_epi32(_mm_shuffle_epi32(a0, _MM_SHUFFLE(3, 0, 1, 2)), _mm_shuffle_epi32(a1, _MM_SHUFFLE(3, 0, 1, 2))));
    _mm_storeu_si128((__m128i*)&a[2], _mm_packus_epi32(_mm_shuffle_epi32(a2, _MM_SHUFFLE(3, 0, 1, 2)), _mm_shuffle_epi32(a3, _MM_SHUFFLE(3, 0, 1, 2))));
#else
    uint32 r[4];
    uint32 g[4];
    uint32 b[4];

    memset(r, 0, sizeof(r));
    memset(g, 0, sizeof(g));
    memset(b, 0, sizeof(b));

    for( int j=0; j<4; j++ )
    {
        for( int i=0; i<4; i++ )
        {
            int index = (j & 2) + (i >> 1);
            b[index] += *data++;
            g[index] += *data++;
            r[index] += *data++;
            data++;
        }
    }

    a[0] = v4i{ uint16( (r[2] + r[3] + 4) / 8 ), uint16( (g[2] + g[3] + 4) / 8 ), uint16( (b[2] + b[3] + 4) / 8 ), 0};
    a[1] = v4i{ uint16( (r[0] + r[1] + 4) / 8 ), uint16( (g[0] + g[1] + 4) / 8 ), uint16( (b[0] + b[1] + 4) / 8 ), 0};
    a[2] = v4i{ uint16( (r[1] + r[3] + 4) / 8 ), uint16( (g[1] + g[3] + 4) / 8 ), uint16( (b[1] + b[3] + 4) / 8 ), 0};
    a[3] = v4i{ uint16( (r[0] + r[2] + 4) / 8 ), uint16( (g[0] + g[2] + 4) / 8 ), uint16( (b[0] + b[2] + 4) / 8 ), 0};
#endif
}

void CalcErrorBlock( const uint8* data, uint err[4][4] )
{
#ifdef __SSE4_1__
    __m128i d0 = _mm_loadu_si128(((__m128i*)data) + 0);
    __m128i d1 = _mm_loadu_si128(((__m128i*)data) + 1);
    __m128i d2 = _mm_loadu_si128(((__m128i*)data) + 2);
    __m128i d3 = _mm_loadu_si128(((__m128i*)data) + 3);

    __m128i dm0 = _mm_and_si128(d0, _mm_set1_epi32(0x00FFFFFF));
    __m128i dm1 = _mm_and_si128(d1, _mm_set1_epi32(0x00FFFFFF));
    __m128i dm2 = _mm_and_si128(d2, _mm_set1_epi32(0x00FFFFFF));
    __m128i dm3 = _mm_and_si128(d3, _mm_set1_epi32(0x00FFFFFF));

    __m128i d0l = _mm_unpacklo_epi8(dm0, _mm_setzero_si128());
    __m128i d0h = _mm_unpackhi_epi8(dm0, _mm_setzero_si128());
    __m128i d1l = _mm_unpacklo_epi8(dm1, _mm_setzero_si128());
    __m128i d1h = _mm_unpackhi_epi8(dm1, _mm_setzero_si128());
    __m128i d2l = _mm_unpacklo_epi8(dm2, _mm_setzero_si128());
    __m128i d2h = _mm_unpackhi_epi8(dm2, _mm_setzero_si128());
    __m128i d3l = _mm_unpacklo_epi8(dm3, _mm_setzero_si128());
    __m128i d3h = _mm_unpackhi_epi8(dm3, _mm_setzero_si128());

    __m128i sum0 = _mm_add_epi16(d0l, d1l);
    __m128i sum1 = _mm_add_epi16(d0h, d1h);
    __m128i sum2 = _mm_add_epi16(d2l, d3l);
    __m128i sum3 = _mm_add_epi16(d2h, d3h);

    __m128i sum0l = _mm_unpacklo_epi16(sum0, _mm_setzero_si128());
    __m128i sum0h = _mm_unpackhi_epi16(sum0, _mm_setzero_si128());
    __m128i sum1l = _mm_unpacklo_epi16(sum1, _mm_setzero_si128());
    __m128i sum1h = _mm_unpackhi_epi16(sum1, _mm_setzero_si128());
    __m128i sum2l = _mm_unpacklo_epi16(sum2, _mm_setzero_si128());
    __m128i sum2h = _mm_unpackhi_epi16(sum2, _mm_setzero_si128());
    __m128i sum3l = _mm_unpacklo_epi16(sum3, _mm_setzero_si128());
    __m128i sum3h = _mm_unpackhi_epi16(sum3, _mm_setzero_si128());

    __m128i b0 = _mm_add_epi32(sum0l, sum0h);
    __m128i b1 = _mm_add_epi32(sum1l, sum1h);
    __m128i b2 = _mm_add_epi32(sum2l, sum2h);
    __m128i b3 = _mm_add_epi32(sum3l, sum3h);

    __m128i a0 = _mm_add_epi32(b2, b3);
    __m128i a1 = _mm_add_epi32(b0, b1);
    __m128i a2 = _mm_add_epi32(b1, b3);
    __m128i a3 = _mm_add_epi32(b0, b2);

    _mm_storeu_si128((__m128i*)&err[0], a0);
    _mm_storeu_si128((__m128i*)&err[1], a1);
    _mm_storeu_si128((__m128i*)&err[2], a2);
    _mm_storeu_si128((__m128i*)&err[3], a3);
#else
    uint terr[4][4];

    memset(terr, 0, 16 * sizeof(uint));

    for( int j=0; j<4; j++ )
    {
        for( int i=0; i<4; i++ )
        {
            int index = (j & 2) + (i >> 1);
            uint d = *data++;
            terr[index][0] += d;
            d = *data++;
            terr[index][1] += d;
            d = *data++;
            terr[index][2] += d;
            data++;
        }
    }

    for( int i=0; i<3; i++ )
    {
        err[0][i] = terr[2][i] + terr[3][i];
        err[1][i] = terr[0][i] + terr[1][i];
        err[2][i] = terr[1][i] + terr[3][i];
        err[3][i] = terr[0][i] + terr[2][i];
    }
    for( int i=0; i<4; i++ )
    {
        err[i][3] = 0;
    }
#endif
}

uint CalcError( const uint block[4], const v4i& average )
{
    uint err = 0x3FFFFFFF; // Big value to prevent negative values, but small enough to prevent overflow
    err -= block[0] * 2 * average[2];
    err -= block[1] * 2 * average[1];
    err -= block[2] * 2 * average[0];
    err += 8 * ( sq( average[0] ) + sq( average[1] ) + sq( average[2] ) );
    return err;
}

void ProcessAverages( v4i* a )
{
#ifdef __SSE4_1__
    for( int i=0; i<2; i++ )
    {
        __m128i d = _mm_loadu_si128((__m128i*)a[i*2].data());

        __m128i t = _mm_add_epi16(_mm_mullo_epi16(d, _mm_set1_epi16(31)), _mm_set1_epi16(128));

        __m128i c = _mm_srli_epi16(_mm_add_epi16(t, _mm_srli_epi16(t, 8)), 8);

        __m128i c1 = _mm_shuffle_epi32(c, _MM_SHUFFLE(3, 2, 3, 2));
        __m128i diff = _mm_sub_epi16(c, c1);
        diff = _mm_max_epi16(diff, _mm_set1_epi16(-4));
        diff = _mm_min_epi16(diff, _mm_set1_epi16(3));

        __m128i co = _mm_add_epi16(c1, diff);

        c = _mm_blend_epi16(co, c, 0xF0);

        __m128i a0 = _mm_or_si128(_mm_slli_epi16(c, 3), _mm_srli_epi16(c, 2));

        _mm_storeu_si128((__m128i*)a[4+i*2].data(), a0);
    }

    for( int i=0; i<2; i++ )
    {
        __m128i d = _mm_loadu_si128((__m128i*)a[i*2].data());

        __m128i t0 = _mm_add_epi16(_mm_mullo_epi16(d, _mm_set1_epi16(15)), _mm_set1_epi16(128));
        __m128i t1 = _mm_srli_epi16(_mm_add_epi16(t0, _mm_srli_epi16(t0, 8)), 8);

        __m128i t2 = _mm_or_si128(t1, _mm_slli_epi16(t1, 4));

        _mm_storeu_si128((__m128i*)a[i*2].data(), t2);
    }
#else
    for( int i=0; i<2; i++ )
    {
        for( int j=0; j<3; j++ )
        {
            int32 c1 = mul8bit( a[i*2+1][j], 31 );
            int32 c2 = mul8bit( a[i*2][j], 31 );

            int32 diff = c2 - c1;
            if( diff > 3 ) diff = 3;
            else if( diff < -4 ) diff = -4;

            int32 co = c1 + diff;

            a[5+i*2][j] = ( c1 << 3 ) | ( c1 >> 2 );
            a[4+i*2][j] = ( co << 3 ) | ( co >> 2 );
        }
    }

    for( int i=0; i<4; i++ )
    {
        a[i][0] = g_avg2[mul8bit( a[i][0], 15 )];
        a[i][1] = g_avg2[mul8bit( a[i][1], 15 )];
        a[i][2] = g_avg2[mul8bit( a[i][2], 15 )];
    }
#endif
}

void EncodeAverages( uint64& _d, const v4i* a, size_t idx )
{
    auto d = _d;
    d |= ( idx << 24 );
    size_t base = idx << 1;

    if( ( idx & 0x2 ) == 0 )
    {
        for( int i=0; i<3; i++ )
        {
            d |= uint64( a[base+0][i] >> 4 ) << ( i*8 );
            d |= uint64( a[base+1][i] >> 4 ) << ( i*8 + 4 );
        }
    }
    else
    {
        for( int i=0; i<3; i++ )
        {
            d |= uint64( a[base+1][i] & 0xF8 ) << ( i*8 );
            int32 c = ( ( a[base+0][i] & 0xF8 ) - ( a[base+1][i] & 0xF8 ) ) >> 3;
            c &= ~0xFFFFFFF8;
            d |= ((uint64)c) << ( i*8 );
        }
    }
    _d = d;
}

uint64 CheckSolid( const uint8* src )
{
#ifdef __SSE4_1__
    __m128i d0 = _mm_loadu_si128(((__m128i*)src) + 0);
    __m128i d1 = _mm_loadu_si128(((__m128i*)src) + 1);
    __m128i d2 = _mm_loadu_si128(((__m128i*)src) + 2);
    __m128i d3 = _mm_loadu_si128(((__m128i*)src) + 3);

    __m128i c = _mm_shuffle_epi32(d0, _MM_SHUFFLE(0, 0, 0, 0));

    __m128i c0 = _mm_cmpeq_epi8(d0, c);
    __m128i c1 = _mm_cmpeq_epi8(d1, c);
    __m128i c2 = _mm_cmpeq_epi8(d2, c);
    __m128i c3 = _mm_cmpeq_epi8(d3, c);

    __m128i m0 = _mm_and_si128(c0, c1);
    __m128i m1 = _mm_and_si128(c2, c3);
    __m128i m = _mm_and_si128(m0, m1);

    if (!_mm_testc_si128(m, _mm_set1_epi32(-1)))
    {
        return 0;
    }
#else
    const uint8* ptr = src + 4;
    for( int i=1; i<16; i++ )
    {
        if( memcmp( src, ptr, 4 ) != 0 )
        {
            return 0;
        }
        ptr += 4;
    }
#endif
    return 0x02000000 |
        ( uint( src[0] & 0xF8 ) << 16 ) |
        ( uint( src[1] & 0xF8 ) << 8 ) |
        ( uint( src[2] & 0xF8 ) );
}

void PrepareAverages( v4i a[8], const uint8* src, uint err[4] )
{
    Average( src, a );
    ProcessAverages( a );

    uint errblock[4][4];
    CalcErrorBlock( src, errblock );

    for( int i=0; i<4; i++ )
    {
        err[i/2] += CalcError( errblock[i], a[i] );
        err[2+i/2] += CalcError( errblock[i], a[i+4] );
    }
}

void FindBestFit( uint64 terr[2][8], uint16 tsel[16][8], v4i a[8], const uint32* id, const uint8* data )
{
    for( size_t i=0; i<16; i++ )
    {
        uint16* sel = tsel[i];
        uint bid = id[i];
        uint64* ter = terr[bid%2];

        uint8 b = *data++;
        uint8 g = *data++;
        uint8 r = *data++;
        data++;

        int dr = a[bid][0] - r;
        int dg = a[bid][1] - g;
        int db = a[bid][2] - b;

#ifdef __SSE4_1__
        // Reference implementation

        __m128i pix = _mm_set1_epi32(dr * 77 + dg * 151 + db * 28);
        // Taking the absolute value is way faster. The values are only used to sort, so the result will be the same.
        __m128i error0 = _mm_abs_epi32(_mm_add_epi32(pix, g_table256_SIMD[0]));
        __m128i error1 = _mm_abs_epi32(_mm_add_epi32(pix, g_table256_SIMD[1]));
        __m128i error2 = _mm_abs_epi32(_mm_sub_epi32(pix, g_table256_SIMD[0]));
        __m128i error3 = _mm_abs_epi32(_mm_sub_epi32(pix, g_table256_SIMD[1]));

        __m128i index0 = _mm_and_si128(_mm_cmplt_epi32(error1, error0), _mm_set1_epi32(1));
        __m128i minError0 = _mm_min_epi32(error0, error1);

        __m128i index1 = _mm_sub_epi32(_mm_set1_epi32(2), _mm_cmplt_epi32(error3, error2));
        __m128i minError1 = _mm_min_epi32(error2, error3);

        __m128i minIndex0 = _mm_blendv_epi8(index0, index1, _mm_cmplt_epi32(minError1, minError0));
        __m128i minError = _mm_min_epi32(minError0, minError1);

        // Squaring the minimum error to produce correct values when adding
        __m128i minErrorLow = _mm_shuffle_epi32(minError, _MM_SHUFFLE(1, 1, 0, 0));
        __m128i squareErrorLow = _mm_mul_epi32(minErrorLow, minErrorLow);
        squareErrorLow = _mm_add_epi64(squareErrorLow, _mm_loadu_si128(((__m128i*)ter) + 0));
        _mm_storeu_si128(((__m128i*)ter) + 0, squareErrorLow);
        __m128i minErrorHigh = _mm_shuffle_epi32(minError, _MM_SHUFFLE(3, 3, 2, 2));
        __m128i squareErrorHigh = _mm_mul_epi32(minErrorHigh, minErrorHigh);
        squareErrorHigh = _mm_add_epi64(squareErrorHigh, _mm_loadu_si128(((__m128i*)ter) + 1));
        _mm_storeu_si128(((__m128i*)ter) + 1, squareErrorHigh);

        // Taking the absolute value is way faster. The values are only used to sort, so the result will be the same.
        error0 = _mm_abs_epi32(_mm_add_epi32(pix, g_table256_SIMD[2]));
        error1 = _mm_abs_epi32(_mm_add_epi32(pix, g_table256_SIMD[3]));
        error2 = _mm_abs_epi32(_mm_sub_epi32(pix, g_table256_SIMD[2]));
        error3 = _mm_abs_epi32(_mm_sub_epi32(pix, g_table256_SIMD[3]));

        index0 = _mm_and_si128(_mm_cmplt_epi32(error1, error0), _mm_set1_epi32(1));
        minError0 = _mm_min_epi32(error0, error1);

        index1 = _mm_sub_epi32(_mm_set1_epi32(2), _mm_cmplt_epi32(error3, error2));
        minError1 = _mm_min_epi32(error2, error3);

        __m128i minIndex1 = _mm_blendv_epi8(index0, index1, _mm_cmplt_epi32(minError1, minError0));
        minError = _mm_min_epi32(minError0, minError1);

        // Squaring the minimum error to produce correct values when adding
        minErrorLow = _mm_shuffle_epi32(minError, _MM_SHUFFLE(1, 1, 0, 0));
        squareErrorLow = _mm_mul_epi32(minErrorLow, minErrorLow);
        squareErrorLow = _mm_add_epi64(squareErrorLow, _mm_loadu_si128(((__m128i*)ter) + 2));
        _mm_storeu_si128(((__m128i*)ter) + 2, squareErrorLow);
        minErrorHigh = _mm_shuffle_epi32(minError, _MM_SHUFFLE(3, 3, 2, 2));
        squareErrorHigh = _mm_mul_epi32(minErrorHigh, minErrorHigh);
        squareErrorHigh = _mm_add_epi64(squareErrorHigh, _mm_loadu_si128(((__m128i*)ter) + 3));
        _mm_storeu_si128(((__m128i*)ter) + 3, squareErrorHigh);
        __m128i minIndex = _mm_packs_epi32(minIndex0, minIndex1);
        _mm_storeu_si128((__m128i*)sel, minIndex);
#else
        int pix = dr * 77 + dg * 151 + db * 28;

        for( int t=0; t<8; t++ )
        {
            const int64* tab = g_table256[t];
            uint idx = 0;
            uint64 err = sq( tab[0] + pix );
            for( int j=1; j<4; j++ )
            {
                uint64 local = sq( tab[j] + pix );
                if( local < err )
                {
                    err = local;
                    idx = j;
                }
            }
            *sel++ = idx;
            *ter++ += err;
        }
#endif
    }
}

#ifdef __SSE4_1__
// Non-reference implementation, but faster. Produces same results as the AVX2 version
void FindBestFit( uint32 terr[2][8], uint16 tsel[16][8], v4i a[8], const uint32* id, const uint8* data )
{
    for( size_t i=0; i<16; i++ )
    {
        uint16* sel = tsel[i];
        uint bid = id[i];
        uint32* ter = terr[bid%2];

        uint8 b = *data++;
        uint8 g = *data++;
        uint8 r = *data++;
        data++;

        int dr = a[bid][0] - r;
        int dg = a[bid][1] - g;
        int db = a[bid][2] - b;

        // The scaling values are divided by two and rounded, to allow the differences to be in the range of signed int16
        // This produces slightly different results, but is significant faster
        __m128i pixel = _mm_set1_epi16(dr * 38 + dg * 76 + db * 14);
        __m128i pix = _mm_abs_epi16(pixel);

        // Taking the absolute value is way faster. The values are only used to sort, so the result will be the same.
        // Since the selector table is symmetrical, we need to calculate the difference only for half of the entries.
        __m128i error0 = _mm_abs_epi16(_mm_sub_epi16(pix, g_table128_SIMD[0]));
        __m128i error1 = _mm_abs_epi16(_mm_sub_epi16(pix, g_table128_SIMD[1]));

        __m128i index = _mm_and_si128(_mm_cmplt_epi16(error1, error0), _mm_set1_epi16(1));
        __m128i minError = _mm_min_epi16(error0, error1);

        // Exploiting symmetry of the selector table and use the sign bit
        // This produces slightly different results, but is needed to produce same results as AVX2 implementation
        __m128i indexBit = _mm_andnot_si128(_mm_srli_epi16(pixel, 15), _mm_set1_epi8(-1));
        __m128i minIndex = _mm_or_si128(index, _mm_add_epi16(indexBit, indexBit));

        // Squaring the minimum error to produce correct values when adding
        __m128i squareErrorLo = _mm_mullo_epi16(minError, minError);
        __m128i squareErrorHi = _mm_mulhi_epi16(minError, minError);

        __m128i squareErrorLow = _mm_unpacklo_epi16(squareErrorLo, squareErrorHi);
        __m128i squareErrorHigh = _mm_unpackhi_epi16(squareErrorLo, squareErrorHi);

        squareErrorLow = _mm_add_epi32(squareErrorLow, _mm_loadu_si128(((__m128i*)ter) + 0));
        _mm_storeu_si128(((__m128i*)ter) + 0, squareErrorLow);
        squareErrorHigh = _mm_add_epi32(squareErrorHigh, _mm_loadu_si128(((__m128i*)ter) + 1));
        _mm_storeu_si128(((__m128i*)ter) + 1, squareErrorHigh);

        _mm_storeu_si128((__m128i*)sel, minIndex);
    }
}
#endif

uint8_t convert6(float f)
{
    int i = (std::min(std::max(static_cast<int>(f), 0), 1023) - 15) >> 1;
    return (i + 11 - ((i + 11) >> 7) - ((i + 4) >> 7)) >> 3;
}

uint8_t convert7(float f)
{
    int i = (std::min(std::max(static_cast<int>(f), 0), 1023) - 15) >> 1;
    return (i + 9 - ((i + 9) >> 8) - ((i + 6) >> 8)) >> 2;
}

std::pair<uint64, uint64> Planar(const uint8* src)
{
    int32 r = 0;
    int32 g = 0;
    int32 b = 0;

    for (int i = 0; i < 16; ++i)
    {
        b += src[i * 4 + 0];
        g += src[i * 4 + 1];
        r += src[i * 4 + 2];
    }

    int32 difRyz = 0;
    int32 difGyz = 0;
    int32 difByz = 0;
    int32 difRxz = 0;
    int32 difGxz = 0;
    int32 difBxz = 0;

    const int32 scaling[] = { -255, -85, 85, 255 };

    for (int i = 0; i < 16; ++i)
    {
        int32 difB = (static_cast<int>(src[i * 4 + 0]) << 4) - b;
        int32 difG = (static_cast<int>(src[i * 4 + 1]) << 4) - g;
        int32 difR = (static_cast<int>(src[i * 4 + 2]) << 4) - r;

        difRyz += difR * scaling[i % 4];
        difGyz += difG * scaling[i % 4];
        difByz += difB * scaling[i % 4];

        difRxz += difR * scaling[i / 4];
        difGxz += difG * scaling[i / 4];
        difBxz += difB * scaling[i / 4];
    }

    const float scale = -4.0f / ((255 * 255 * 8.0f + 85 * 85 * 8.0f) * 16.0f);

    float aR = difRxz * scale;
    float aG = difGxz * scale;
    float aB = difBxz * scale;

    float bR = difRyz * scale;
    float bG = difGyz * scale;
    float bB = difByz * scale;

    float dR = r * (4.0f / 16.0f);
    float dG = g * (4.0f / 16.0f);
    float dB = b * (4.0f / 16.0f);

    // calculating the three colors RGBO, RGBH, and RGBV.  RGB = df - af * x - bf * y;
    float cofR = std::fma(aR,  255.0f, std::fma(bR,  255.0f, dR));
    float cofG = std::fma(aG,  255.0f, std::fma(bG,  255.0f, dG));
    float cofB = std::fma(aB,  255.0f, std::fma(bB,  255.0f, dB));
    float chfR = std::fma(aR, -425.0f, std::fma(bR,  255.0f, dR));
    float chfG = std::fma(aG, -425.0f, std::fma(bG,  255.0f, dG));
    float chfB = std::fma(aB, -425.0f, std::fma(bB,  255.0f, dB));
    float cvfR = std::fma(aR,  255.0f, std::fma(bR, -425.0f, dR));
    float cvfG = std::fma(aG,  255.0f, std::fma(bG, -425.0f, dG));
    float cvfB = std::fma(aB,  255.0f, std::fma(bB, -425.0f, dB));

    // convert to r6g7b6
    int32 coR = convert6(cofR);
    int32 coG = convert7(cofG);
    int32 coB = convert6(cofB);
    int32 chR = convert6(chfR);
    int32 chG = convert7(chfG);
    int32 chB = convert6(chfB);
    int32 cvR = convert6(cvfR);
    int32 cvG = convert7(cvfG);
    int32 cvB = convert6(cvfB);

    // Error calculation
    auto ro0 = coR;
    auto go0 = coG;
    auto bo0 = coB;
    auto ro1 = (ro0 >> 4) | (ro0 << 2);
    auto go1 = (go0 >> 6) | (go0 << 1);
    auto bo1 = (bo0 >> 4) | (bo0 << 2);
    auto ro2 = (ro1 << 2) + 2;
    auto go2 = (go1 << 2) + 2;
    auto bo2 = (bo1 << 2) + 2;

    auto rh0 = chR;
    auto gh0 = chG;
    auto bh0 = chB;
    auto rh1 = (rh0 >> 4) | (rh0 << 2);
    auto gh1 = (gh0 >> 6) | (gh0 << 1);
    auto bh1 = (bh0 >> 4) | (bh0 << 2);

    auto rh2 = rh1 - ro1;
    auto gh2 = gh1 - go1;
    auto bh2 = bh1 - bo1;

    auto rv0 = cvR;
    auto gv0 = cvG;
    auto bv0 = cvB;
    auto rv1 = (rv0 >> 4) | (rv0 << 2);
    auto gv1 = (gv0 >> 6) | (gv0 << 1);
    auto bv1 = (bv0 >> 4) | (bv0 << 2);

    auto rv2 = rv1 - ro1;
    auto gv2 = gv1 - go1;
    auto bv2 = bv1 - bo1;

    uint64 error = 0;

    for (int i = 0; i < 16; ++i)
    {
        int32 cR = clampu8((rh2 * (i / 4) + rv2 * (i % 4) + ro2) >> 2);
        int32 cG = clampu8((gh2 * (i / 4) + gv2 * (i % 4) + go2) >> 2);
        int32 cB = clampu8((bh2 * (i / 4) + bv2 * (i % 4) + bo2) >> 2);

        int32 difB = static_cast<int>(src[i * 4 + 0]) - cB;
        int32 difG = static_cast<int>(src[i * 4 + 1]) - cG;
        int32 difR = static_cast<int>(src[i * 4 + 2]) - cR;

        int32 dif = difR * 38 + difG * 76 + difB * 14;

        error += dif * dif;
    }

    /**/
    uint32 rgbv = cvB | (cvG << 6) | (cvR << 13);
    uint32 rgbh = chB | (chG << 6) | (chR << 13);
    uint32 hi = rgbv | ((rgbh & 0x1FFF) << 19);
    uint32 lo = (chR & 0x1) | 0x2 | ((chR << 1) & 0x7C);
    lo |= ((coB & 0x07) <<  7) | ((coB & 0x18) <<  8) | ((coB & 0x20) << 11);
    lo |= ((coG & 0x3F) << 17) | ((coG & 0x40) << 18);
    lo |= coR << 25;

    const auto idx = (coR & 0x20) | ((coG & 0x20) >> 1) | ((coB & 0x1E) >> 1);

    lo |= g_flags[idx];

    uint64 result = static_cast<uint32>(_bswap(lo));
    result |= static_cast<uint64>(static_cast<uint32>(_bswap(hi))) << 32;

    return std::make_pair(result, error);
}

template<class T, class S>
uint64 EncodeSelectors( uint64 d, const T terr[2][8], const S tsel[16][8], const uint32* id, const uint64 value, const uint64 error)
{
    size_t tidx[2];
    tidx[0] = GetLeastError( terr[0], 8 );
    tidx[1] = GetLeastError( terr[1], 8 );

    if ((terr[0][tidx[0]] + terr[1][tidx[1]]) >= error)
    {
        return value;
    }

    d |= tidx[0] << 26;
    d |= tidx[1] << 29;
    for( int i=0; i<16; i++ )
    {
        uint64 t = tsel[i][tidx[id[i]%2]];
        d |= ( t & 0x1 ) << ( i + 32 );
        d |= ( t & 0x2 ) << ( i + 47 );
    }

    return FixByteOrder(d);
}
}

uint64 ProcessRGB( const uint8* src )
{
    uint64 d = CheckSolid( src );
    if( d != 0 ) return d;

    v4i a[8];
    uint err[4] = {};
    PrepareAverages( a, src, err );
    size_t idx = GetLeastError( err, 4 );
    EncodeAverages( d, a, idx );

#if defined __SSE4_1__ && !defined REFERENCE_IMPLEMENTATION
    uint32 terr[2][8] = {};
#else
    uint64 terr[2][8] = {};
#endif
    uint16 tsel[16][8];
    auto id = g_id[idx];
    FindBestFit( terr, tsel, a, id, src );

    return FixByteOrder( EncodeSelectors( d, terr, tsel, id ) );
}

uint64 ProcessRGB_ETC2( const uint8* src )
{
    auto result = Planar( src );

    uint64 d = 0;

    v4i a[8];
    uint err[4] = {};
    PrepareAverages( a, src, err );
    size_t idx = GetLeastError( err, 4 );
    EncodeAverages( d, a, idx );

#if defined __SSE4_1__ && !defined REFERENCE_IMPLEMENTATION
    uint32 terr[2][8] = {};
#else
    uint64 terr[2][8] = {};
#endif
    uint16 tsel[16][8];
    auto id = g_id[idx];
    FindBestFit( terr, tsel, a, id, src );

    return EncodeSelectors( d, terr, tsel, id, result.first, result.second );
}

inline int NumberOfMipLevels(const v2i& size)
{
	return (int)floor(log2(std::max(size.x, size.y))) + 1;
}

Bitmap::Bitmap(const v2i& size)
	: m_data(new uint32[size.x*size.y])
	, m_block(nullptr)
	, m_lines(1)
	, m_linesLeft(size.y / 4)
	, m_size(size)
//	, m_sema(0)
{
}

Bitmap::Bitmap(const Bitmap& src, uint lines)
	: m_lines(lines)
	, m_alpha(src.Alpha())
//	, m_sema(0)
{
}

Bitmap::Bitmap(const v2i& size, const void *pixelData, int pitch, uint lines)
	: m_data(nullptr)
	, m_block((uint32*)pixelData)
	, m_lines(lines)
	, m_linesLeft(size.y / 4)
	, m_size(size)
//	, m_sema(0)
{

}

Bitmap::~Bitmap()
{
	delete[] m_data;
}

const uint32* Bitmap::NextBlock(uint& lines, bool& done)
{
//	std::lock_guard<std::mutex> lock(m_lock);
	lines = std::min(m_lines, m_linesLeft);
	auto ret = m_block;
//	m_sema.lock();
	m_block += m_size.x * 4 * lines;
	m_linesLeft -= lines;
	done = m_linesLeft == 0;
	return ret;
}

class BitmapDownsampled : public Bitmap
{
public:
	BitmapDownsampled(const Bitmap& bmp, uint lines);
	~BitmapDownsampled();
};

BitmapDownsampled::BitmapDownsampled(const Bitmap& bmp, uint lines)
	: Bitmap(bmp, lines)
{
	m_size.x = std::max(1, bmp.Size().x / 2);
	m_size.y = std::max(1, bmp.Size().y / 2);

	int w = std::max(m_size.x, 4);
	int h = std::max(m_size.y, 4);

//	DBGPRINT("Subbitmap " << m_size.x << "x" << m_size.y);

	m_block = m_data = new uint32[w*h];

	if (m_size.x < w || m_size.y < h)
	{
		memset(m_data, 0, w*h*sizeof(uint32));
		m_linesLeft = h / 4;
		uint lines = 0;
		for (int i = 0; i < h / 4; i++)
		{
			for (int j = 0; j < 4; j++)
			{
				lines++;
				if (lines > m_lines)
				{
					lines = 0;
				//	m_sema.unlock();
				}
			}
		}
		if (lines != 0)
		{
		//	m_sema.unlock();
		}
	} else
	{
		m_linesLeft = h / 4;
		m_load = std::async(std::launch::async, [this, &bmp, w, h]() mutable
		{
			auto ptr = m_data;
			auto src1 = bmp.Data();
			auto src2 = src1 + bmp.Size().x;
			uint lines = 0;
			for (int i = 0; i < h / 4; i++)
			{
				for (int j = 0; j < 4; j++)
				{
					for (int k = 0; k < m_size.x; k++)
					{
						int r = ((*src1 & 0x000000FF) + (*(src1 + 1) & 0x000000FF) + (*src2 & 0x000000FF) + (*(src2 + 1) & 0x000000FF)) / 4;
						int g = (((*src1 & 0x0000FF00) + (*(src1 + 1) & 0x0000FF00) + (*src2 & 0x0000FF00) + (*(src2 + 1) & 0x0000FF00)) / 4) & 0x0000FF00;
						int b = (((*src1 & 0x00FF0000) + (*(src1 + 1) & 0x00FF0000) + (*src2 & 0x00FF0000) + (*(src2 + 1) & 0x00FF0000)) / 4) & 0x00FF0000;
						int a = (((((*src1 & 0xFF000000) >> 8) + ((*(src1 + 1) & 0xFF000000) >> 8) + ((*src2 & 0xFF000000) >> 8) + ((*(src2 + 1) & 0xFF000000) >> 8)) / 4) & 0x00FF0000) << 8;
						*ptr++ = r | g | b | a;
						src1 += 2;
						src2 += 2;
					}
					src1 += m_size.x * 2;
					src2 += m_size.x * 2;
				}
				lines++;
				if (lines >= m_lines)
				{
					lines = 0;
				//	m_sema.unlock();
				}
			}

			if (lines != 0)
			{
			//	m_sema.unlock();
			}
		});
	}
}

BitmapDownsampled::~BitmapDownsampled()
{
}

static uint8 e5[32];
static uint8 e6[64];
static uint8 qrb[256 + 16];
static uint8 qg[256 + 16];

void InitDither()
{
	for (int i = 0; i < 32; i++)
	{
		e5[i] = (i << 3) | (i >> 2);
	}
	for (int i = 0; i < 64; i++)
	{
		e6[i] = (i << 2) | (i >> 4);
	}
	for (int i = 0; i < 256 + 16; i++)
	{
		int v = std::min(std::max(0, i - 8), 255);
		qrb[i] = e5[mul8bit(v, 31)];
		qg[i] = e6[mul8bit(v, 63)];
	}
}

void Dither(uint8* data)
{
	int err[8];
	int* ep1 = err;
	int* ep2 = err + 4;

	for (int ch = 0; ch < 3; ch++)
	{
		uint8* ptr = data + ch;
		uint8* quant = (ch == 1) ? qg + 8 : qrb + 8;
		memset(err, 0, sizeof(err));

		for (int y = 0; y < 4; y++)
		{
			uint8 tmp;
			tmp = quant[ptr[0] + ((3 * ep2[1] + 5 * ep2[0]) >> 4)];
			ep1[0] = ptr[0] - tmp;
			ptr[0] = tmp;
			tmp = quant[ptr[4] + ((7 * ep1[0] + 3 * ep2[2] + 5 * ep2[1] + ep2[0]) >> 4)];
			ep1[1] = ptr[4] - tmp;
			ptr[4] = tmp;
			tmp = quant[ptr[8] + ((7 * ep1[1] + 3 * ep2[3] + 5 * ep2[2] + ep2[1]) >> 4)];
			ep1[2] = ptr[8] - tmp;
			ptr[8] = tmp;
			tmp = quant[ptr[12] + ((7 * ep1[2] + 5 * ep2[3] + ep2[2]) >> 4)];
			ep1[3] = ptr[12] - tmp;
			ptr[12] = tmp;
			ptr += 16;
			std::swap(ep1, ep2);
		}
	}
}

void Swizzle(const uint8* data, const ptrdiff_t pitch, uint8* output)
{
	for (int i = 0; i < 4; ++i)
	{
		uint64 d0 = *(const uint64*)(data + i * pitch + 0);
		uint64 d1 = *(const uint64*)(data + i * pitch + 8);

		*(uint64*)(output + i * 16 + 0) = d0;
		*(uint64*)(output + i * 16 + 8) = d1;
	}
}

static int AdjustSizeForMipmaps(const v2i& size, int levels)
{
	int len = 0;
	v2i current = size;
	for (int i = 1; i < levels; i++)
	{
		assert(current.x != 1 || current.y != 1);
		current.x = std::max(1, current.x / 2);
		current.y = std::max(1, current.y / 2);
		len += std::max(4, current.x) * std::max(4, current.y) / 2;
	}
	assert(current.x == 1 && current.y == 1);
	return len;
}

static uint8* OpenForWriting(/*const char* fn,*/ size_t len, const v2i& size, /*FILE** f,*/ int levels)
{
	auto ret = new uint8[len];
	auto dst = (uint32*)ret;

	*dst++ = 0x03525650;  // version
	*dst++ = 0;           // flags
	*dst++ = 6;           // pixelformat[0], value 22 is needed for etc2
	*dst++ = 0;           // pixelformat[1]
	*dst++ = 0;           // colourspace
	*dst++ = 0;           // channel type
	*dst++ = size.y;      // height
	*dst++ = size.x;      // width
	*dst++ = 1;           // depth
	*dst++ = 1;           // num surfs
	*dst++ = 1;           // num faces
	*dst++ = levels;      // mipmap count
	*dst++ = 0;           // metadata size

	return ret;
}

static uint64 _f_rgb(uint8* ptr)
{
	return ProcessRGB(ptr);
}

#ifdef __SSE4_1__
static uint64 _f_rgb_avx2(uint8* ptr)
{
	return ProcessRGB_AVX2(ptr);
}
#endif

static uint64 _f_rgb_dither(uint8* ptr)
{
	Dither(ptr);
	return ProcessRGB(ptr);
}

#ifdef __SSE4_1__
static uint64 _f_rgb_dither_avx2(uint8* ptr)
{
	Dither(ptr);
	return ProcessRGB_AVX2(ptr);
}
#endif

static uint64 _f_rgb_etc2(uint8* ptr)
{
	return ProcessRGB_ETC2(ptr);
}

#ifdef __SSE4_1__
static uint64 _f_rgb_etc2_avx2(uint8* ptr)
{
	return ProcessRGB_ETC2_AVX2(ptr);
}
#endif

static uint64 _f_rgb_etc2_dither(uint8* ptr)
{
	Dither(ptr);
	return ProcessRGB_ETC2(ptr);
}

#ifdef __SSE4_1__
static uint64 _f_rgb_etc2_dither_avx2(uint8* ptr)
{
	Dither(ptr);
	return ProcessRGB_ETC2_AVX2(ptr);
}
#endif

namespace
{
	struct BlockColor
	{
		uint32 r1, g1, b1;
		uint32 r2, g2, b2;
	};

	enum class Etc2Mode
	{
		none,
		t,
		h,
		planar
	};

	Etc2Mode DecodeBlockColor(uint64 d, BlockColor& c)
	{
		if (d & 0x2)
		{
			int32 dr, dg, db;

			c.r1 = (d & 0xF8000000) >> 27;
			c.g1 = (d & 0x00F80000) >> 19;
			c.b1 = (d & 0x0000F800) >> 11;

			dr = (d & 0x07000000) >> 24;
			dg = (d & 0x00070000) >> 16;
			db = (d & 0x00000700) >> 8;

			if (dr & 0x4)
			{
				dr |= 0xFFFFFFF8;
			}
			if (dg & 0x4)
			{
				dg |= 0xFFFFFFF8;
			}
			if (db & 0x4)
			{
				db |= 0xFFFFFFF8;
			}

			int32 r = static_cast<int32_t>(c.r1) + dr;
			int32 g = static_cast<int32_t>(c.g1) + dg;
			int32 b = static_cast<int32_t>(c.b1) + db;

			if ((r < 0) || (r > 31))
			{
				return Etc2Mode::t;
			}

			if ((g < 0) || (g > 31))
			{
				return Etc2Mode::h;
			}

			if ((b < 0) || (b > 31))
			{
				return Etc2Mode::planar;
			}

			c.r2 = c.r1 + dr;
			c.g2 = c.g1 + dg;
			c.b2 = c.b1 + db;

			c.r1 = (c.r1 << 3) | (c.r1 >> 2);
			c.g1 = (c.g1 << 3) | (c.g1 >> 2);
			c.b1 = (c.b1 << 3) | (c.b1 >> 2);
			c.r2 = (c.r2 << 3) | (c.r2 >> 2);
			c.g2 = (c.g2 << 3) | (c.g2 >> 2);
			c.b2 = (c.b2 << 3) | (c.b2 >> 2);
		} else
		{
			c.r1 = ((d & 0xF0000000) >> 24) | ((d & 0xF0000000) >> 28);
			c.r2 = ((d & 0x0F000000) >> 20) | ((d & 0x0F000000) >> 24);
			c.g1 = ((d & 0x00F00000) >> 16) | ((d & 0x00F00000) >> 20);
			c.g2 = ((d & 0x000F0000) >> 12) | ((d & 0x000F0000) >> 16);
			c.b1 = ((d & 0x0000F000) >> 8) | ((d & 0x0000F000) >> 12);
			c.b2 = ((d & 0x00000F00) >> 4) | ((d & 0x00000F00) >> 8);
		}
		return Etc2Mode::none;
	}

	inline int32 expand6(uint32 value)
	{
		return (value << 2) | (value >> 4);
	}

	inline int32 expand7(uint32 value)
	{
		return (value << 1) | (value >> 6);
	}

	void DecodePlanar(uint64 block, uint32** l)
	{
		const auto bv = expand6((block >> (0 + 32)) & 0x3F);
		const auto gv = expand7((block >> (6 + 32)) & 0x7F);
		const auto rv = expand6((block >> (13 + 32)) & 0x3F);

		const auto bh = expand6((block >> (19 + 32)) & 0x3F);
		const auto gh = expand7((block >> (25 + 32)) & 0x7F);

		const auto rh0 = (block >> (32 - 32)) & 0x01;
		const auto rh1 = ((block >> (34 - 32)) & 0x1F) << 1;
		const auto rh = expand6(rh0 | rh1);

		const auto bo0 = (block >> (39 - 32)) & 0x07;
		const auto bo1 = ((block >> (43 - 32)) & 0x3) << 3;
		const auto bo2 = ((block >> (48 - 32)) & 0x1) << 5;
		const auto bo = expand6(bo0 | bo1 | bo2);
		const auto go0 = (block >> (49 - 32)) & 0x3F;
		const auto go1 = ((block >> (56 - 32)) & 0x01) << 6;
		const auto go = expand7(go0 | go1);
		const auto ro = expand6((block >> (57 - 32)) & 0x3F);

		for (auto j = 0; j < 4; j++)
		{
			for (auto i = 0; i < 4; i++)
			{
				uint32 r = clampu8((i * (rh - ro) + j * (rv - ro) + 4 * ro + 2) >> 2);
				uint32 g = clampu8((i * (gh - go) + j * (gv - go) + 4 * go + 2) >> 2);
				uint32 b = clampu8((i * (bh - bo) + j * (bv - bo) + 4 * bo + 2) >> 2);

				*l[j]++ = r | (g << 8) | (b << 16) | 0xFF000000;
			}
		}
	}

}

static void DecodeBlock(uint64 d, uint32** l) {
	d = ((d & 0xFF000000FF000000) >> 24) |
		((d & 0x000000FF000000FF) << 24) |
		((d & 0x00FF000000FF0000) >> 8) |
		((d & 0x0000FF000000FF00) << 8);

	BlockColor c;
	const auto mode = DecodeBlockColor(d, c);

	if (mode == Etc2Mode::planar)
	{
		DecodePlanar(d, l);
		return;
	}

	uint tcw[2];
	tcw[0] = (d & 0xE0) >> 5;
	tcw[1] = (d & 0x1C) >> 2;

	uint ra, ga, ba;
	uint rb, gb, bb;
	uint rc, gc, bc;
	uint rd, gd, bd;

	if (d & 0x1) {
		int o = 0;
		for (int i = 0; i < 4; i++)
		{
			ra = clampu8(c.r1 + g_table[tcw[0]][((d & (1ll << (o + 32))) >> (o + 32)) | ((d & (1ll << (o + 48))) >> (o + 47))]);
			ga = clampu8(c.g1 + g_table[tcw[0]][((d & (1ll << (o + 32))) >> (o + 32)) | ((d & (1ll << (o + 48))) >> (o + 47))]);
			ba = clampu8(c.b1 + g_table[tcw[0]][((d & (1ll << (o + 32))) >> (o + 32)) | ((d & (1ll << (o + 48))) >> (o + 47))]);

			rb = clampu8(c.r1 + g_table[tcw[0]][((d & (1ll << (o + 33))) >> (o + 33)) | ((d & (1ll << (o + 49))) >> (o + 48))]);
			gb = clampu8(c.g1 + g_table[tcw[0]][((d & (1ll << (o + 33))) >> (o + 33)) | ((d & (1ll << (o + 49))) >> (o + 48))]);
			bb = clampu8(c.b1 + g_table[tcw[0]][((d & (1ll << (o + 33))) >> (o + 33)) | ((d & (1ll << (o + 49))) >> (o + 48))]);

			rc = clampu8(c.r2 + g_table[tcw[1]][((d & (1ll << (o + 34))) >> (o + 34)) | ((d & (1ll << (o + 50))) >> (o + 49))]);
			gc = clampu8(c.g2 + g_table[tcw[1]][((d & (1ll << (o + 34))) >> (o + 34)) | ((d & (1ll << (o + 50))) >> (o + 49))]);
			bc = clampu8(c.b2 + g_table[tcw[1]][((d & (1ll << (o + 34))) >> (o + 34)) | ((d & (1ll << (o + 50))) >> (o + 49))]);

			rd = clampu8(c.r2 + g_table[tcw[1]][((d & (1ll << (o + 35))) >> (o + 35)) | ((d & (1ll << (o + 51))) >> (o + 50))]);
			gd = clampu8(c.g2 + g_table[tcw[1]][((d & (1ll << (o + 35))) >> (o + 35)) | ((d & (1ll << (o + 51))) >> (o + 50))]);
			bd = clampu8(c.b2 + g_table[tcw[1]][((d & (1ll << (o + 35))) >> (o + 35)) | ((d & (1ll << (o + 51))) >> (o + 50))]);

			*l[0]++ = ra | (ga << 8) | (ba << 16) | 0xFF000000;
			*l[1]++ = rb | (gb << 8) | (bb << 16) | 0xFF000000;
			*l[2]++ = rc | (gc << 8) | (bc << 16) | 0xFF000000;
			*l[3]++ = rd | (gd << 8) | (bd << 16) | 0xFF000000;

			o += 4;
		}
	} else {
		int o = 0;
		for (int i = 0; i < 2; i++)
		{
			ra = clampu8(c.r1 + g_table[tcw[0]][((d & (1ll << (o + 32))) >> (o + 32)) | ((d & (1ll << (o + 48))) >> (o + 47))]);
			ga = clampu8(c.g1 + g_table[tcw[0]][((d & (1ll << (o + 32))) >> (o + 32)) | ((d & (1ll << (o + 48))) >> (o + 47))]);
			ba = clampu8(c.b1 + g_table[tcw[0]][((d & (1ll << (o + 32))) >> (o + 32)) | ((d & (1ll << (o + 48))) >> (o + 47))]);

			rb = clampu8(c.r1 + g_table[tcw[0]][((d & (1ll << (o + 33))) >> (o + 33)) | ((d & (1ll << (o + 49))) >> (o + 48))]);
			gb = clampu8(c.g1 + g_table[tcw[0]][((d & (1ll << (o + 33))) >> (o + 33)) | ((d & (1ll << (o + 49))) >> (o + 48))]);
			bb = clampu8(c.b1 + g_table[tcw[0]][((d & (1ll << (o + 33))) >> (o + 33)) | ((d & (1ll << (o + 49))) >> (o + 48))]);

			rc = clampu8(c.r1 + g_table[tcw[0]][((d & (1ll << (o + 34))) >> (o + 34)) | ((d & (1ll << (o + 50))) >> (o + 49))]);
			gc = clampu8(c.g1 + g_table[tcw[0]][((d & (1ll << (o + 34))) >> (o + 34)) | ((d & (1ll << (o + 50))) >> (o + 49))]);
			bc = clampu8(c.b1 + g_table[tcw[0]][((d & (1ll << (o + 34))) >> (o + 34)) | ((d & (1ll << (o + 50))) >> (o + 49))]);

			rd = clampu8(c.r1 + g_table[tcw[0]][((d & (1ll << (o + 35))) >> (o + 35)) | ((d & (1ll << (o + 51))) >> (o + 50))]);
			gd = clampu8(c.g1 + g_table[tcw[0]][((d & (1ll << (o + 35))) >> (o + 35)) | ((d & (1ll << (o + 51))) >> (o + 50))]);
			bd = clampu8(c.b1 + g_table[tcw[0]][((d & (1ll << (o + 35))) >> (o + 35)) | ((d & (1ll << (o + 51))) >> (o + 50))]);

			*l[0]++ = ra | (ga << 8) | (ba << 16) | 0xFF000000;
			*l[1]++ = rb | (gb << 8) | (bb << 16) | 0xFF000000;
			*l[2]++ = rc | (gc << 8) | (bc << 16) | 0xFF000000;
			*l[3]++ = rd | (gd << 8) | (bd << 16) | 0xFF000000;

			o += 4;
		}
		for (int i = 0; i < 2; i++)
		{
			ra = clampu8(c.r2 + g_table[tcw[1]][((d & (1ll << (o + 32))) >> (o + 32)) | ((d & (1ll << (o + 48))) >> (o + 47))]);
			ga = clampu8(c.g2 + g_table[tcw[1]][((d & (1ll << (o + 32))) >> (o + 32)) | ((d & (1ll << (o + 48))) >> (o + 47))]);
			ba = clampu8(c.b2 + g_table[tcw[1]][((d & (1ll << (o + 32))) >> (o + 32)) | ((d & (1ll << (o + 48))) >> (o + 47))]);

			rb = clampu8(c.r2 + g_table[tcw[1]][((d & (1ll << (o + 33))) >> (o + 33)) | ((d & (1ll << (o + 49))) >> (o + 48))]);
			gb = clampu8(c.g2 + g_table[tcw[1]][((d & (1ll << (o + 33))) >> (o + 33)) | ((d & (1ll << (o + 49))) >> (o + 48))]);
			bb = clampu8(c.b2 + g_table[tcw[1]][((d & (1ll << (o + 33))) >> (o + 33)) | ((d & (1ll << (o + 49))) >> (o + 48))]);

			rc = clampu8(c.r2 + g_table[tcw[1]][((d & (1ll << (o + 34))) >> (o + 34)) | ((d & (1ll << (o + 50))) >> (o + 49))]);
			gc = clampu8(c.g2 + g_table[tcw[1]][((d & (1ll << (o + 34))) >> (o + 34)) | ((d & (1ll << (o + 50))) >> (o + 49))]);
			bc = clampu8(c.b2 + g_table[tcw[1]][((d & (1ll << (o + 34))) >> (o + 34)) | ((d & (1ll << (o + 50))) >> (o + 49))]);

			rd = clampu8(c.r2 + g_table[tcw[1]][((d & (1ll << (o + 35))) >> (o + 35)) | ((d & (1ll << (o + 51))) >> (o + 50))]);
			gd = clampu8(c.g2 + g_table[tcw[1]][((d & (1ll << (o + 35))) >> (o + 35)) | ((d & (1ll << (o + 51))) >> (o + 50))]);
			bd = clampu8(c.b2 + g_table[tcw[1]][((d & (1ll << (o + 35))) >> (o + 35)) | ((d & (1ll << (o + 51))) >> (o + 50))]);

			*l[0]++ = ra | (ga << 8) | (ba << 16) | 0xFF000000;
			*l[1]++ = rb | (gb << 8) | (bb << 16) | 0xFF000000;
			*l[2]++ = rc | (gc << 8) | (bc << 16) | 0xFF000000;
			*l[3]++ = rd | (gd << 8) | (bd << 16) | 0xFF000000;

			o += 4;
		}
	}
}

static void DecodeAlphaBlock(const unsigned char* data, uint8** l) {
	int alpha = data[0];
	int table = data[1];

	int bit = 0;
	int byte = 2;
	//extract an alpha value for each pixel.
	for (int x = 0; x < 4; x++) {
		for (int y = 0; y < 4; y++) {
			//Extract table index
			int index = 0;
			for (int bitpos = 0; bitpos < 3; bitpos++)
			{
				index |= getbit(data[byte], 7 - bit, 2 - bitpos);
				bit++;
				if (bit > 7) {
					bit = 0;
					byte++;
				}
			}
			l[y][x * 4] = clampu8(alpha + alphaTable[table][index]);
		}
	}
	l[0] += 16;
	l[1] += 16;
	l[2] += 16;
	l[3] += 16;
}

void decode(const void *data, void* pixel, int pitch, int h, int blkw, int blkh)
{
	int dpitch = pitch;
	uint32 *ret = (uint32 *)pixel;
	uint32* l[4];
	l[0] = ret;
	l[1] = l[0] + blkw * 4;
	l[2] = l[1] + blkw * 4;
	l[3] = l[2] + blkw * 4;

	const uint64* src = (const uint64*)data;
	if (h & 3) --blkh;
	for (int y = 0; y < blkh; y++)
	{
		for (int x = 0; x < blkw; x++)
		{
			DecodeBlock(*src++, l);
		}

		l[0] += dpitch * 3;
		l[1] += dpitch * 3;
		l[2] += dpitch * 3;
		l[3] += dpitch * 3;
	}
	if (h & 3)
	{
		h &= 3;
		std::vector<uint32> dummy; dummy.resize(blkw * 4);
		for (int y = 0; y < h; ++y) {
			l[3 - y] = &dummy.front();
		}
		for (int x = 0; x < blkw; x++)
		{
			DecodeBlock(*src++, l);
		}
	}
}

void decodeWithAlpha(const void *data, void* pixel, int pitch, int h, int blkw, int blkh)
{
	setupAlphaTable();
	int dpitch = pitch;
	uint32 *ret = (uint32 *)pixel;
	uint32* l[4]; uint8* la[4];
	l[0] = ret;
	l[1] = l[0] + blkw * 4;
	l[2] = l[1] + blkw * 4;
	l[3] = l[2] + blkw * 4;
	la[0] = ((uint8*)ret) + 3;
	la[1] = la[0] + blkw * 4 * 4;
	la[2] = la[1] + blkw * 4 * 4;
	la[3] = la[2] + blkw * 4 * 4;

	const uint64* src = (const uint64*)data;
	if (h & 3) --blkh;
	for (int y = 0; y < blkh; y++)
	{
		for (int x = 0; x < blkw; x++)
		{
			const unsigned char *a = (const unsigned char *)src++;
			DecodeBlock(*src++, l);
			DecodeAlphaBlock(a, la);
		}

		l[0] += dpitch * 3;
		l[1] += dpitch * 3;
		l[2] += dpitch * 3;
		l[3] += dpitch * 3;
		la[0] += dpitch * 3;
		la[1] += dpitch * 3;
		la[2] += dpitch * 3;
		la[3] += dpitch * 3;
	}
	if (h & 3)
	{
		h &= 3;
		std::vector<uint32> dummy; dummy.resize(blkw * 4);
		for (int y = 0; y < h; ++y) {
			l[3 - y] = &dummy.front();
			la[3 - y] = (uint8*)&dummy.front();
		}
		for (int x = 0; x < blkw; x++)
		{
			const unsigned char *a = (const unsigned char *)src++;
			DecodeBlock(*src++, l);
			DecodeAlphaBlock(a, la);
		}
	}
}

void* convert(const void *_pixel, int w, int h, int pitch, bool etc2, size_t &datalen)
{
	tjs_uint32 *pixel = (tjs_uint32 *)TJSAlignedAlloc(pitch * h, 4);
	TVPReverseRGB(pixel, (const tjs_uint32 *)_pixel, pitch * h / 4);
	int blkw = w / 4, blkh = h / 4;
	int edgew = w % 4, edgeh = h % 4;
	int dpitch = (blkw + !!edgew) * 8;
	datalen = dpitch * (blkh + !!edgeh);
	uint8* data = new uint8[datalen];
	uint8 *src = (uint8 *)pixel, *dst = data;
	for (int blky = 0; blky < blkh; ++blky) {
		uint8* sline = src, *dline = dst;
		for (int blkx = 0; blkx < blkw; ++blkx) {
			uint32 buf[4 * 4];
			uint8* line = sline;
			for (int y = 0; y < 4; ++y) {
				for (int x = 0; x < 4; ++x) {
					buf[x * 4 + y] = *(uint32*)(line + x * 4);
				}
				line += pitch;
			}
			*(uint64*)dline = _f_rgb_etc2((uint8*)buf);
			sline += 16;
			dline += 8;
		}
		src += pitch * 4;
		dst += dpitch;
	}
	if (edgew) {
		uint8* sline = ((uint8 *)pixel) + blkw * 16, *dline = data + blkw * 8;
		for (int blky = 0; blky < blkh; ++blky) {
			uint32 buf[4 * 4] = { 0 };
			uint8* line = sline;
			for (int y = 0; y < 4; ++y) {
				for (int x = 0; x < edgew; ++x) {
					buf[x * 4 + y] = *(uint32*)(line + x * 4);
				}
				line += pitch;
			}
			*(uint64*)(dline) = _f_rgb_etc2((uint8*)buf);
			sline += pitch * 4;
			dline += dpitch;
		}
	}
	if (edgeh) {
		uint8* sline = ((uint8 *)pixel) + blkh * pitch * 4, *dline = data + blkh * dpitch;
		for (int blkx = 0; blkx < blkw; ++blkx) {
			uint32 buf[4 * 4] = { 0 };
			uint8* line = sline;
			for (int y = 0; y < edgeh; ++y) {
				for (int x = 0; x < 4; ++x) {
					buf[x * 4 + y] = *(uint32*)(line + x * 4);
				}
				line += pitch;
			}
			*(uint64*)dline = _f_rgb_etc2((uint8*)buf);
			sline += 16;
			dline += 8;
		}
		if (edgew) {
			uint32 buf[4 * 4] = { 0 };
			uint8* line = sline;
			for (int y = 0; y < edgeh; ++y) {
				for (int x = 0; x < edgew; ++x) {
					buf[x * 4 + y] = *(uint32*)(line + x * 4);
				}
				line += pitch;
			}
			*(uint64*)dline = _f_rgb_etc2((uint8*)buf);
		}
	}
	TJSAlignedDealloc(pixel);
	return data;
#if 0
	int nThread = TVPGetThreadNum() - 1; // don't use main thread if possible
	if (nThread < 1) nThread = 1;
	else if (nThread > num) nThread = num;
	TVPExecThreadTask(nThread, [&](int n) {
		for (int i = n; i < data.size(); i += nThread) {
			DataPart& part = data[i];
			bool dither = false;
			bd->Process(part.src, part.width / 4 * part.lines, part.offset, part.width, Channels::RGB, dither, etc2);
		}
	});
	void* pixeldata = bd->Detach(datalen);
//	v2i size = dp.ImageData().Size();

	bd.reset();
	return pixeldata;
#endif
}

void* convertWithAlpha(const void *_pixel, int w, int h, int pitch, size_t &datalen)
{
	setupAlphaTable();
	tjs_uint32 *pixel = (tjs_uint32 *)TJSAlignedAlloc(pitch * h, 4);
	TVPReverseRGB(pixel, (const tjs_uint32 *)_pixel, pitch * h / 4);
	int blkw = w / 4, blkh = h / 4;
	int edgew = w % 4, edgeh = h % 4;
	int dpitch = (blkw + !!edgew) * 16;
	datalen = dpitch * (blkh + !!edgeh);
	uint8* data = new uint8[datalen];
	uint8 *src = (uint8 *)pixel, *dst = data;
	for (int blky = 0; blky < blkh; ++blky) {
		uint8* sline = src, *dline = dst;
		for (int blkx = 0; blkx < blkw; ++blkx) {
			uint8 abuf[4 * 4];
			uint32 buf[4 * 4];
			uint8* line = sline;
			for (int y = 0; y < 4; ++y) {
				for (int x = 0; x < 4; ++x) {
					abuf[x * 4 + y] = line[x * 4 + 3];
					buf[x * 4 + y] = *(uint32*)(line + x * 4);
				}
				line += pitch;
			}
			compressBlockAlphaFast(abuf, dline);
			dline += 8;
			*(uint64*)dline = _f_rgb_etc2((uint8*)buf);
			dline += 8;
			sline += 16;
		}
		src += pitch * 4;
		dst += dpitch;
	}
	if (edgew) {
		uint8* sline = ((uint8 *)pixel) + blkw * 16, *dline = data + blkw * 16;
		for (int blky = 0; blky < blkh; ++blky) {
			uint8 abuf[4 * 4] = { 0 };
			uint32 buf[4 * 4] = { 0 };
			uint8* line = sline;
			for (int y = 0; y < 4; ++y) {
				for (int x = 0; x < edgew; ++x) {
					abuf[x * 4 + y] = line[x * 4 + 3];
					buf[x * 4 + y] = *(uint32*)(line + x * 4);
				}
				line += pitch;
			}
			compressBlockAlphaFast(abuf, dline);
			*(uint64*)(dline + 8) = _f_rgb_etc2((uint8*)buf);
			dline += dpitch;
			sline += pitch * 4;
		}
	}
	if (edgeh) {
		uint8* sline = ((uint8 *)pixel) + blkh * pitch * 4, *dline = data + blkh * dpitch;
		for (int blkx = 0; blkx < blkw; ++blkx) {
			uint8 abuf[4 * 4] = { 0 };
			uint32 buf[4 * 4] = { 0 };
			uint8* line = sline;
			for (int y = 0; y < edgeh; ++y) {
				for (int x = 0; x < 4; ++x) {
					abuf[x * 4 + y] = line[x * 4 + 3];
					buf[x * 4 + y] = *(uint32*)(line + x * 4);
				}
				line += pitch;
			}
			compressBlockAlphaFast(abuf, dline);
			dline += 8;
			*(uint64*)dline = _f_rgb_etc2((uint8*)buf);
			dline += 8;
			sline += 16;
		}
		if (edgew) {
			uint8 abuf[4 * 4] = { 0 };
			uint32 buf[4 * 4] = { 0 };
			uint8* line = sline;
			for (int y = 0; y < edgeh; ++y) {
				for (int x = 0; x < edgew; ++x) {
					abuf[x * 4 + y] = line[x * 4 + 3];
					buf[x * 4 + y] = *(uint32*)(line + x * 4);
				}
				line += pitch;
			}
			compressBlockAlphaFast(abuf, dline);
			dline += 8;
			*(uint64*)dline = _f_rgb_etc2((uint8*)buf);
		}
	}
	
	TJSAlignedDealloc(pixel);
	return data;
}

}