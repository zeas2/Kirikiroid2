#include "pvrtc.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>
#include <vector>

//============================================================================
//
// Modulation data specifies weightings of colorA to colorB for each pixel
//
// For mode = 0
//	00: 0/8
//  01: 3/8
//  10: 5/8
//  11: 8/8
//
// For mode = 1
//  00: 0/8
//  01: 4/8
//  10: 4/8 with alpha punchthrough
//  11: 8/8
//
// For colorIsOpaque=0
//  3 bits A
//  4 bits R
//  4 bits G
//  3/4 bits B
//
// For colorIsOpaque=1
//  5 bits R
//  5 bits G
//  4/5 bits B
//
//============================================================================


namespace PvrTcEncoder {
static const unsigned char MODULATION_LUT[16] = {
	0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 3, 3, 3
};
constexpr unsigned short MORTON_TABLE[256] =
{
	0x0000, 0x0001, 0x0004, 0x0005, 0x0010, 0x0011, 0x0014, 0x0015,
	0x0040, 0x0041, 0x0044, 0x0045, 0x0050, 0x0051, 0x0054, 0x0055,
	0x0100, 0x0101, 0x0104, 0x0105, 0x0110, 0x0111, 0x0114, 0x0115,
	0x0140, 0x0141, 0x0144, 0x0145, 0x0150, 0x0151, 0x0154, 0x0155,
	0x0400, 0x0401, 0x0404, 0x0405, 0x0410, 0x0411, 0x0414, 0x0415,
	0x0440, 0x0441, 0x0444, 0x0445, 0x0450, 0x0451, 0x0454, 0x0455,
	0x0500, 0x0501, 0x0504, 0x0505, 0x0510, 0x0511, 0x0514, 0x0515,
	0x0540, 0x0541, 0x0544, 0x0545, 0x0550, 0x0551, 0x0554, 0x0555,
	0x1000, 0x1001, 0x1004, 0x1005, 0x1010, 0x1011, 0x1014, 0x1015,
	0x1040, 0x1041, 0x1044, 0x1045, 0x1050, 0x1051, 0x1054, 0x1055,
	0x1100, 0x1101, 0x1104, 0x1105, 0x1110, 0x1111, 0x1114, 0x1115,
	0x1140, 0x1141, 0x1144, 0x1145, 0x1150, 0x1151, 0x1154, 0x1155,
	0x1400, 0x1401, 0x1404, 0x1405, 0x1410, 0x1411, 0x1414, 0x1415,
	0x1440, 0x1441, 0x1444, 0x1445, 0x1450, 0x1451, 0x1454, 0x1455,
	0x1500, 0x1501, 0x1504, 0x1505, 0x1510, 0x1511, 0x1514, 0x1515,
	0x1540, 0x1541, 0x1544, 0x1545, 0x1550, 0x1551, 0x1554, 0x1555,
	0x4000, 0x4001, 0x4004, 0x4005, 0x4010, 0x4011, 0x4014, 0x4015,
	0x4040, 0x4041, 0x4044, 0x4045, 0x4050, 0x4051, 0x4054, 0x4055,
	0x4100, 0x4101, 0x4104, 0x4105, 0x4110, 0x4111, 0x4114, 0x4115,
	0x4140, 0x4141, 0x4144, 0x4145, 0x4150, 0x4151, 0x4154, 0x4155,
	0x4400, 0x4401, 0x4404, 0x4405, 0x4410, 0x4411, 0x4414, 0x4415,
	0x4440, 0x4441, 0x4444, 0x4445, 0x4450, 0x4451, 0x4454, 0x4455,
	0x4500, 0x4501, 0x4504, 0x4505, 0x4510, 0x4511, 0x4514, 0x4515,
	0x4540, 0x4541, 0x4544, 0x4545, 0x4550, 0x4551, 0x4554, 0x4555,
	0x5000, 0x5001, 0x5004, 0x5005, 0x5010, 0x5011, 0x5014, 0x5015,
	0x5040, 0x5041, 0x5044, 0x5045, 0x5050, 0x5051, 0x5054, 0x5055,
	0x5100, 0x5101, 0x5104, 0x5105, 0x5110, 0x5111, 0x5114, 0x5115,
	0x5140, 0x5141, 0x5144, 0x5145, 0x5150, 0x5151, 0x5154, 0x5155,
	0x5400, 0x5401, 0x5404, 0x5405, 0x5410, 0x5411, 0x5414, 0x5415,
	0x5440, 0x5441, 0x5444, 0x5445, 0x5450, 0x5451, 0x5454, 0x5455,
	0x5500, 0x5501, 0x5504, 0x5505, 0x5510, 0x5511, 0x5514, 0x5515,
	0x5540, 0x5541, 0x5544, 0x5545, 0x5550, 0x5551, 0x5554, 0x5555
};
namespace Data {
constexpr uint8_t BITSCALE_5_TO_8[32] = {
	0, 8, 16, 24, 32, 41, 49, 57, 65, 74,
	82, 90, 98, 106, 115, 123, 131, 139, 148, 156,
	164, 172, 180, 189, 197, 205, 213, 222, 230, 238,
	246, 255 };

constexpr uint8_t BITSCALE_4_TO_8[16] = {
	0, 17, 34, 51, 68, 85, 102, 119, 136, 153,
	170, 187, 204, 221, 238, 255 };

constexpr uint8_t BITSCALE_3_TO_8[8] = {
	0, 36, 72, 109, 145, 182, 218, 255 };

constexpr uint8_t BITSCALE_8_TO_5_FLOOR[256] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	1, 1, 1, 1, 1, 1, 1, 2, 2, 2,
	2, 2, 2, 2, 2, 3, 3, 3, 3, 3,
	3, 3, 3, 4, 4, 4, 4, 4, 4, 4,
	4, 4, 5, 5, 5, 5, 5, 5, 5, 5,
	6, 6, 6, 6, 6, 6, 6, 6, 7, 7,
	7, 7, 7, 7, 7, 7, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 9, 9, 9, 9, 9,
	9, 9, 9, 10, 10, 10, 10, 10, 10, 10,
	10, 11, 11, 11, 11, 11, 11, 11, 11, 12,
	12, 12, 12, 12, 12, 12, 12, 13, 13, 13,
	13, 13, 13, 13, 13, 13, 14, 14, 14, 14,
	14, 14, 14, 14, 15, 15, 15, 15, 15, 15,
	15, 15, 16, 16, 16, 16, 16, 16, 16, 16,
	17, 17, 17, 17, 17, 17, 17, 17, 17, 18,
	18, 18, 18, 18, 18, 18, 18, 19, 19, 19,
	19, 19, 19, 19, 19, 20, 20, 20, 20, 20,
	20, 20, 20, 21, 21, 21, 21, 21, 21, 21,
	21, 22, 22, 22, 22, 22, 22, 22, 22, 22,
	23, 23, 23, 23, 23, 23, 23, 23, 24, 24,
	24, 24, 24, 24, 24, 24, 25, 25, 25, 25,
	25, 25, 25, 25, 26, 26, 26, 26, 26, 26,
	26, 26, 26, 27, 27, 27, 27, 27, 27, 27,
	27, 28, 28, 28, 28, 28, 28, 28, 28, 29,
	29, 29, 29, 29, 29, 29, 29, 30, 30, 30,
	30, 30, 30, 30, 30, 31 };

constexpr uint8_t BITSCALE_8_TO_4_FLOOR[256] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 3, 3, 3, 3, 3, 3, 3, 3, 3,
	3, 3, 3, 3, 3, 3, 3, 3, 4, 4,
	4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
	4, 4, 4, 4, 4, 5, 5, 5, 5, 5,
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	5, 5, 6, 6, 6, 6, 6, 6, 6, 6,
	6, 6, 6, 6, 6, 6, 6, 6, 6, 7,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 9, 9, 9, 9, 9, 9, 9,
	9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
	10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
	10, 10, 10, 10, 10, 10, 10, 11, 11, 11,
	11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
	11, 11, 11, 11, 12, 12, 12, 12, 12, 12,
	12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
	12, 13, 13, 13, 13, 13, 13, 13, 13, 13,
	13, 13, 13, 13, 13, 13, 13, 13, 14, 14,
	14, 14, 14, 14, 14, 14, 14, 14, 14, 14,
	14, 14, 14, 14, 14, 15 };

constexpr uint8_t BITSCALE_8_TO_3_FLOOR[256] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
	3, 3, 3, 3, 3, 3, 4, 4, 4, 4,
	4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
	4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
	4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
	4, 4, 4, 5, 5, 5, 5, 5, 5, 5,
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	5, 5, 5, 5, 5, 5, 5, 5, 5, 6,
	6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
	6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
	6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
	6, 6, 6, 6, 6, 7 };

constexpr uint8_t BITSCALE_8_TO_5_CEIL[256] = {
	0, 1, 1, 1, 1, 1, 1, 1, 1, 2,
	2, 2, 2, 2, 2, 2, 2, 3, 3, 3,
	3, 3, 3, 3, 3, 4, 4, 4, 4, 4,
	4, 4, 4, 5, 5, 5, 5, 5, 5, 5,
	5, 5, 6, 6, 6, 6, 6, 6, 6, 6,
	7, 7, 7, 7, 7, 7, 7, 7, 8, 8,
	8, 8, 8, 8, 8, 8, 9, 9, 9, 9,
	9, 9, 9, 9, 9, 10, 10, 10, 10, 10,
	10, 10, 10, 11, 11, 11, 11, 11, 11, 11,
	11, 12, 12, 12, 12, 12, 12, 12, 12, 13,
	13, 13, 13, 13, 13, 13, 13, 14, 14, 14,
	14, 14, 14, 14, 14, 14, 15, 15, 15, 15,
	15, 15, 15, 15, 16, 16, 16, 16, 16, 16,
	16, 16, 17, 17, 17, 17, 17, 17, 17, 17,
	18, 18, 18, 18, 18, 18, 18, 18, 18, 19,
	19, 19, 19, 19, 19, 19, 19, 20, 20, 20,
	20, 20, 20, 20, 20, 21, 21, 21, 21, 21,
	21, 21, 21, 22, 22, 22, 22, 22, 22, 22,
	22, 23, 23, 23, 23, 23, 23, 23, 23, 23,
	24, 24, 24, 24, 24, 24, 24, 24, 25, 25,
	25, 25, 25, 25, 25, 25, 26, 26, 26, 26,
	26, 26, 26, 26, 27, 27, 27, 27, 27, 27,
	27, 27, 27, 28, 28, 28, 28, 28, 28, 28,
	28, 29, 29, 29, 29, 29, 29, 29, 29, 30,
	30, 30, 30, 30, 30, 30, 30, 31, 31, 31,
	31, 31, 31, 31, 31, 31 };

constexpr uint8_t BITSCALE_8_TO_4_CEIL[256] = {
	0, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 3, 3, 3, 3, 3,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
	3, 3, 4, 4, 4, 4, 4, 4, 4, 4,
	4, 4, 4, 4, 4, 4, 4, 4, 4, 5,
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	5, 5, 5, 5, 5, 5, 6, 6, 6, 6,
	6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
	6, 6, 6, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 9, 9, 9,
	9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
	9, 9, 9, 9, 10, 10, 10, 10, 10, 10,
	10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
	10, 11, 11, 11, 11, 11, 11, 11, 11, 11,
	11, 11, 11, 11, 11, 11, 11, 11, 12, 12,
	12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
	12, 12, 12, 12, 12, 13, 13, 13, 13, 13,
	13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
	13, 13, 14, 14, 14, 14, 14, 14, 14, 14,
	14, 14, 14, 14, 14, 14, 14, 14, 14, 15,
	15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15 };

constexpr uint8_t BITSCALE_8_TO_3_CEIL[256] = {
	0, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 3, 3, 3, 3, 3, 3, 3,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
	4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
	4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
	4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
	4, 4, 4, 4, 4, 4, 5, 5, 5, 5,
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	5, 5, 5, 6, 6, 6, 6, 6, 6, 6,
	6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
	6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
	6, 6, 6, 6, 6, 6, 6, 6, 6, 7,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7 };
}
template<typename T>
class Point2 {
public:
	T x;
	T y;

	Point2(int a, int b)
		: x(a)
		, y(b) {
	}
};

class BitUtility {
public:
	static bool IsPowerOf2(unsigned int x) {
		return (x & (x - 1)) == 0;
	}

	static unsigned int RotateRight(unsigned int value, unsigned int shift) {
		if ((shift &= sizeof(value) * 8 - 1) == 0) {
			return value;
		}
		return (value >> shift) | (value << (sizeof(value) * 8 - shift));
	}
};

template<typename T>
class ColorRgb {
public:
	T r;
	T g;
	T b;


	ColorRgb()
		: r(0)
		, g(0)
		, b(0) {
	}

	ColorRgb(T red, T green, T blue)
		: r(red)
		, g(green)
		, b(blue) {
	}

	ColorRgb(const ColorRgb<T> &x)
		: r(x.r)
		, g(x.g)
		, b(x.b) {
	}

	ColorRgb<int> operator *(int x) {
		return ColorRgb<int>(r * x, g * x, b * x);
	}

	ColorRgb<int> operator +(const ColorRgb<T> &x) const {
		return ColorRgb<int>(r + (int)x.r, g + (int)x.g, b + (int)x.b);
	}

	ColorRgb<int> operator -(const ColorRgb<T> &x) const {
		return ColorRgb<int>(r - (int)x.r, g - (int)x.g, b - (int)x.b);
	}

	int operator %(const ColorRgb<T> &x) const {
		return r * (int)x.r + g * (int)x.g + b * (int)x.b;
	}

	bool operator ==(const ColorRgb<T> &x) const {
		return r == x.r && g == x.g && b == x.b;
	}

	bool operator !=(const ColorRgb<T> &x) const {
		return r != x.r || g != x.g || b != x.b;
	}

	void SetMin(const ColorRgb<T> &x) {
		if (x.r < r) {
			r = x.r;
		}
		if (x.g < g) {
			g = x.g;
		}
		if (x.b < b) {
			b = x.b;
		}
	}

	void SetMax(const ColorRgb<T> &x) {
		if (x.r > r) {
			r = x.r;
		}
		if (x.g > g) {
			g = x.g;
		}
		if (x.b > b) {
			b = x.b;
		}
	}
};

template<typename T>
class ColorRgba : public ColorRgb<T> {
public:
	T a;

	ColorRgba() :
		a(0) {
	}

	ColorRgba(T red, T green, T blue, T alpha)
		: ColorRgb<T>(red, green, blue)
		, a(alpha) {
	}

	ColorRgba(const ColorRgba<T> &x)
		: ColorRgb<T>(x.r, x.g, x.b)
		, a(x.a) {
	}

	ColorRgba<int> operator *(int x) {
		return ColorRgba<T>(ColorRgb<T>::r * x,
			ColorRgb<T>::g * x,
			ColorRgb<T>::b * x,
			a * x);
	}

	ColorRgba<int> operator +(const ColorRgba<T> &x) {
		return ColorRgba<T>(ColorRgb<T>::r + (int)x.r,
			ColorRgb<T>::g + (int)x.g,
			ColorRgb<T>::b + (int)x.b,
			a + (int)x.a);
	}

	ColorRgba<int> operator -(const ColorRgba<T> &x) {
		return ColorRgba<T>(ColorRgb<T>::r - (int)x.r,
			ColorRgb<T>::g - (int)x.g,
			ColorRgb<T>::b - (int)x.b,
			a - (int)x.a);
	}

	int operator %(const ColorRgba<T> &x) {
		return ColorRgb<T>::r * (int)x.r +
			ColorRgb<T>::g * (int)x.g +
			ColorRgb<T>::b * (int)x.b +
			a * (int)x.a;
	}

	bool operator ==(const ColorRgba<T> &x) {
		return ColorRgb<T>::r == x.r && ColorRgb<T>::g == x.g &&
			ColorRgb<T>::b == x.b && a == x.a;
	}

	bool operator !=(const ColorRgba<T> &x) {
		return ColorRgb<T>::r != x.r || ColorRgb<T>::g != x.g ||
			ColorRgb<T>::b != x.b || a != x.a;
	}

	void SetMin(const ColorRgba<T> &x) {
		ColorRgb<T>::SetMin(x);
		if (x.a < a) {
			a = x.a;
		}
	}

	void SetMax(const ColorRgba<T> &x) {
		ColorRgb<T>::SetMax(x);
		if (x.a > a) {
			a = x.a;
		}
	}
};
//============================================================================

struct PvrTcPacket
{
	unsigned int    modulationData;
	unsigned        usePunchthroughAlpha : 1;
	unsigned        colorA : 14;
	unsigned        colorAIsOpaque : 1;
	unsigned        colorB : 15;
	unsigned        colorBIsOpaque : 1;

	ColorRgb<int> GetColorRgbA() const;
	ColorRgb<int> GetColorRgbB() const;
	ColorRgba<int> GetColorRgbaA() const;
	ColorRgba<int> GetColorRgbaB() const;

	void SetColorA(const ColorRgb<unsigned char>& c);
	void SetColorB(const ColorRgb<unsigned char>& c);

	void SetColorA(const ColorRgba<unsigned char>& c);
	void SetColorB(const ColorRgba<unsigned char>& c);

	static const unsigned char BILINEAR_FACTORS[16][4];
	static const unsigned char WEIGHTS[8][4];
};

const unsigned char PvrTcPacket::BILINEAR_FACTORS[16][4] =
{
	{ 4, 4, 4, 4 },
	{ 2, 6, 2, 6 },
	{ 8, 0, 8, 0 },
	{ 6, 2, 6, 2 },

	{ 2, 2, 6, 6 },
	{ 1, 3, 3, 9 },
	{ 4, 0, 12, 0 },
	{ 3, 1, 9, 3 },

	{ 8, 8, 0, 0 },
	{ 4, 12, 0, 0 },
	{ 16, 0, 0, 0 },
	{ 12, 4, 0, 0 },

	{ 6, 6, 2, 2 },
	{ 3, 9, 1, 3 },
	{ 12, 0, 4, 0 },
	{ 9, 3, 3, 1 },
};

// Weights are { colorA, colorB, alphaA, alphaB }
const unsigned char PvrTcPacket::WEIGHTS[8][4] =
{
	// Weights for Mode=0
	{ 8, 0, 8, 0 },
	{ 5, 3, 5, 3 },
	{ 3, 5, 3, 5 },
	{ 0, 8, 0, 8 },

	// Weights for Mode=1
	{ 8, 0, 8, 0 },
	{ 4, 4, 4, 4 },
	{ 4, 4, 0, 0 },
	{ 0, 8, 0, 8 },
};

//============================================================================

ColorRgb<int> PvrTcPacket::GetColorRgbA() const
{
	if (colorAIsOpaque)
	{
		unsigned char r = colorA >> 9;
		unsigned char g = colorA >> 4 & 0x1f;
		unsigned char b = colorA & 0xf;
		return ColorRgb<int>(Data::BITSCALE_5_TO_8[r],
			Data::BITSCALE_5_TO_8[g],
			Data::BITSCALE_4_TO_8[b]);
	} else
	{
		unsigned char r = (colorA >> 7) & 0xf;
		unsigned char g = (colorA >> 3) & 0xf;
		unsigned char b = colorA & 7;
		return ColorRgb<int>(Data::BITSCALE_4_TO_8[r],
			Data::BITSCALE_4_TO_8[g],
			Data::BITSCALE_3_TO_8[b]);
	}
}

ColorRgb<int> PvrTcPacket::GetColorRgbB() const
{
	if (colorBIsOpaque)
	{
		unsigned char r = colorB >> 10;
		unsigned char g = colorB >> 5 & 0x1f;
		unsigned char b = colorB & 0x1f;
		return ColorRgb<int>(Data::BITSCALE_5_TO_8[r],
			Data::BITSCALE_5_TO_8[g],
			Data::BITSCALE_5_TO_8[b]);
	} else
	{
		unsigned char r = colorB >> 8 & 0xf;
		unsigned char g = colorB >> 4 & 0xf;
		unsigned char b = colorB & 0xf;
		return ColorRgb<int>(Data::BITSCALE_4_TO_8[r],
			Data::BITSCALE_4_TO_8[g],
			Data::BITSCALE_4_TO_8[b]);
	}
}

ColorRgba<int> PvrTcPacket::GetColorRgbaA() const
{
	if (colorAIsOpaque)
	{
		unsigned char r = colorA >> 9;
		unsigned char g = colorA >> 4 & 0x1f;
		unsigned char b = colorA & 0xf;
		return ColorRgba<int>(Data::BITSCALE_5_TO_8[r],
			Data::BITSCALE_5_TO_8[g],
			Data::BITSCALE_4_TO_8[b],
			255);
	} else
	{
		unsigned char a = colorA >> 11 & 7;
		unsigned char r = colorA >> 7 & 0xf;
		unsigned char g = colorA >> 3 & 0xf;
		unsigned char b = colorA & 7;
		return ColorRgba<int>(Data::BITSCALE_4_TO_8[r],
			Data::BITSCALE_4_TO_8[g],
			Data::BITSCALE_3_TO_8[b],
			Data::BITSCALE_3_TO_8[a]);
	}
}

ColorRgba<int> PvrTcPacket::GetColorRgbaB() const
{
	if (colorBIsOpaque)
	{
		unsigned char r = colorB >> 10;
		unsigned char g = colorB >> 5 & 0x1f;
		unsigned char b = colorB & 0x1f;
		return ColorRgba<int>(Data::BITSCALE_5_TO_8[r],
			Data::BITSCALE_5_TO_8[g],
			Data::BITSCALE_5_TO_8[b],
			255);
	} else
	{
		unsigned char a = colorB >> 12 & 7;
		unsigned char r = colorB >> 8 & 0xf;
		unsigned char g = colorB >> 4 & 0xf;
		unsigned char b = colorB & 0xf;
		return ColorRgba<int>(Data::BITSCALE_4_TO_8[r],
			Data::BITSCALE_4_TO_8[g],
			Data::BITSCALE_4_TO_8[b],
			Data::BITSCALE_3_TO_8[a]);
	}
}

//============================================================================

void PvrTcPacket::SetColorA(const ColorRgb<unsigned char>& c)
{
	int r = Data::BITSCALE_8_TO_5_FLOOR[c.r];
	int g = Data::BITSCALE_8_TO_5_FLOOR[c.g];
	int b = Data::BITSCALE_8_TO_4_FLOOR[c.b];
	colorA = r << 9 | g << 4 | b;
	colorAIsOpaque = true;
}

void PvrTcPacket::SetColorB(const ColorRgb<unsigned char>& c)
{
	int r = Data::BITSCALE_8_TO_5_CEIL[c.r];
	int g = Data::BITSCALE_8_TO_5_CEIL[c.g];
	int b = Data::BITSCALE_8_TO_5_CEIL[c.b];
	colorB = r << 10 | g << 5 | b;
	colorBIsOpaque = true;
}

void PvrTcPacket::SetColorA(const ColorRgba<unsigned char>& c)
{
	int a = Data::BITSCALE_8_TO_3_FLOOR[c.a];
	if (a == 7)
	{
		int r = Data::BITSCALE_8_TO_5_FLOOR[c.r];
		int g = Data::BITSCALE_8_TO_5_FLOOR[c.g];
		int b = Data::BITSCALE_8_TO_4_FLOOR[c.b];
		colorA = r << 9 | g << 4 | b;
		colorAIsOpaque = true;
	} else
	{
		int r = Data::BITSCALE_8_TO_4_FLOOR[c.r];
		int g = Data::BITSCALE_8_TO_4_FLOOR[c.g];
		int b = Data::BITSCALE_8_TO_3_FLOOR[c.b];
		colorA = a << 11 | r << 7 | g << 3 | b;
		colorAIsOpaque = false;
	}
}

void PvrTcPacket::SetColorB(const ColorRgba<unsigned char>& c)
{
	int a = Data::BITSCALE_8_TO_3_CEIL[c.a];
	if (a == 7)
	{
		int r = Data::BITSCALE_8_TO_5_CEIL[c.r];
		int g = Data::BITSCALE_8_TO_5_CEIL[c.g];
		int b = Data::BITSCALE_8_TO_5_CEIL[c.b];
		colorB = r << 10 | g << 5 | b;
		colorBIsOpaque = true;
	} else
	{
		int r = Data::BITSCALE_8_TO_4_CEIL[c.r];
		int g = Data::BITSCALE_8_TO_4_CEIL[c.g];
		int b = Data::BITSCALE_8_TO_4_CEIL[c.b];
		colorB = a << 12 | r << 8 | g << 4 | b;
		colorBIsOpaque = false;
	}
}

//============================================================================

class Bitmap {
public:
	int width;
	int height;
	unsigned char *data;

	Bitmap(int w, int h, int bytesPerPixel, unsigned char * pix)
		: width(w)
		, height(h)
		, data(pix) {
	}

	virtual ~Bitmap() {
	//	delete[] data;
	}

	Point2<int> GetSize() const { return Point2<int>(width, height); }

	int GetArea() const { return width * height; }

	int GetBitmapWidth() const { return width; }

	int GetBitmapHeight() const { return height; }

	const unsigned char *GetRawData() const { return data; }
};

class RgbaBitmap : public Bitmap {
public:
	RgbaBitmap(int w, int h, unsigned char * pix)
		: Bitmap(w, h, 4, pix) {
	}

	const ColorRgba<unsigned char> *GetData() const {
		return reinterpret_cast<ColorRgba<unsigned char> *>(data);
	}

	ColorRgba<unsigned char> *GetData() {
		return reinterpret_cast<ColorRgba<unsigned char> *>(data);
	}
};

static unsigned GetMortonNumber(int x, int y)
{
	return MORTON_TABLE[x >> 8] << 17 | MORTON_TABLE[y >> 8] << 16 | MORTON_TABLE[x & 0xFF] << 1 | MORTON_TABLE[y & 0xFF];
}
//============================================================================

template<typename T>
class Interval {
public:
	T min;
	T max;

	Interval() {
	}

	Interval<T> &operator|=(const T &x) {
		min.SetMin(x);
		max.SetMax(x);
		return *this;
	}
};
typedef Interval<ColorRgb<unsigned char>> ColorRgbBoundingBox;

//============================================================================

static void CalculateBoundingBox(ColorRgbBoundingBox& cbb, const RgbaBitmap& bitmap, int blockX, int blockY)
{
	int size = bitmap.GetBitmapWidth();
	const ColorRgba<unsigned char>* data = bitmap.GetData() + blockY * 4 * size + blockX * 4;
	
	cbb.min = data[0];
	cbb.max = data[0];
	
	cbb |= data[1];
	cbb |= data[2];
	cbb |= data[3];
	
	cbb |= data[size];
	cbb |= data[size+1];
	cbb |= data[size+2];
	cbb |= data[size+3];
	
	cbb |= data[2*size];
	cbb |= data[2*size+1];
	cbb |= data[2*size+2];
	cbb |= data[2*size+3];
	
	cbb |= data[3*size];
	cbb |= data[3*size+1];
	cbb |= data[3*size+2];
	cbb |= data[3*size+3];
}

void EncodeRgb4Bpp(void* result, const RgbaBitmap& bitmap)
{
	assert(bitmap.GetBitmapWidth() == bitmap.GetBitmapHeight());
	assert(BitUtility::IsPowerOf2(bitmap.GetBitmapWidth()));
	const int size = bitmap.GetBitmapWidth();
	const int blocks = size / 4;
	const int blockMask = blocks-1;
	
	PvrTcPacket* packets = static_cast<PvrTcPacket*>(result);
	
	for(int y = 0; y < blocks; ++y)
	{
		for(int x = 0; x < blocks; ++x)
		{
			ColorRgbBoundingBox cbb;
			CalculateBoundingBox(cbb, bitmap, x, y);
			PvrTcPacket* packet = packets + GetMortonNumber(x, y);
			packet->usePunchthroughAlpha = 0;
			packet->SetColorA(cbb.min);
			packet->SetColorB(cbb.max);
		}
	}
	
	for(int y = 0; y < blocks; ++y)
	{
		for(int x = 0; x < blocks; ++x)
		{
			const unsigned char (*factor)[4] = PvrTcPacket::BILINEAR_FACTORS;
			const ColorRgba<unsigned char>* data = bitmap.GetData() + y * 4 * size + x * 4;
			
			uint32_t modulationData = 0;
			
			for(int py = 0; py < 4; ++py)
			{
				const int yOffset = (py < 2) ? -1 : 0;
				const int y0 = (y + yOffset) & blockMask;
				const int y1 = (y0+1) & blockMask;

				for(int px = 0; px < 4; ++px)
				{
					const int xOffset = (px < 2) ? -1 : 0;
					const int x0 = (x + xOffset) & blockMask;
					const int x1 = (x0+1) & blockMask;
					
					const PvrTcPacket* p0 = packets + GetMortonNumber(x0, y0);
					const PvrTcPacket* p1 = packets + GetMortonNumber(x1, y0);
					const PvrTcPacket* p2 = packets + GetMortonNumber(x0, y1);
					const PvrTcPacket* p3 = packets + GetMortonNumber(x1, y1);
					
					ColorRgb<int> ca = p0->GetColorRgbA() * (*factor)[0] +
									   p1->GetColorRgbA() * (*factor)[1] +
									   p2->GetColorRgbA() * (*factor)[2] +
									   p3->GetColorRgbA() * (*factor)[3];
					
					ColorRgb<int> cb = p0->GetColorRgbB() * (*factor)[0] +
									   p1->GetColorRgbB() * (*factor)[1] +
									   p2->GetColorRgbB() * (*factor)[2] +
									   p3->GetColorRgbB() * (*factor)[3];
					
					const ColorRgb<unsigned char>& pixel = data[py*size + px];
					ColorRgb<int> d = cb - ca;
					ColorRgb<int> p{pixel.r*16, pixel.g*16, pixel.b*16};
					ColorRgb<int> v = p - ca;
					
					// PVRTC uses weightings of 0, 3/8, 5/8 and 1
					// The boundaries for these are 3/16, 1/2 (=8/16), 13/16
					int projection = (v % d) * 16;
					int lengthSquared = d % d;
					if(projection > 3*lengthSquared) modulationData++;
					if(projection > 8*lengthSquared) modulationData++;
					if(projection > 13*lengthSquared) modulationData++;
					
					modulationData = BitUtility::RotateRight(modulationData, 2);
					
					factor++;
				}
			}

			PvrTcPacket* packet = packets + GetMortonNumber(x, y);
			packet->modulationData = modulationData;
		}
	}
}

//============================================================================

typedef Interval<ColorRgba<unsigned char>> ColorRgbaBoundingBox;

static void CalculateBoundingBox(ColorRgbaBoundingBox& cbb, const RgbaBitmap& bitmap, int blockX, int blockY)
{
	int size = bitmap.GetBitmapWidth();
	const ColorRgba<unsigned char>* data = bitmap.GetData() + blockY * 4 * size + blockX * 4;
	
	cbb.min = data[0];
	cbb.max = data[0];
	
	cbb |= data[1];
	cbb |= data[2];
	cbb |= data[3];
	
	cbb |= data[size];
	cbb |= data[size+1];
	cbb |= data[size+2];
	cbb |= data[size+3];
	
	cbb |= data[2*size];
	cbb |= data[2*size+1];
	cbb |= data[2*size+2];
	cbb |= data[2*size+3];
	
	cbb |= data[3*size];
	cbb |= data[3*size+1];
	cbb |= data[3*size+2];
	cbb |= data[3*size+3];
}

void EncodeRgba4Bpp(void* result, const RgbaBitmap& bitmap)
{
	assert(bitmap.GetBitmapWidth() == bitmap.GetBitmapHeight());
	assert(BitUtility::IsPowerOf2(bitmap.GetBitmapWidth()));
	const int size = bitmap.GetBitmapWidth();
	const int blocks = size / 4;
	const int blockMask = blocks-1;
	
	PvrTcPacket* packets = static_cast<PvrTcPacket*>(result);
	
	for(int y = 0; y < blocks; ++y)
	{
		for(int x = 0; x < blocks; ++x)
		{
			ColorRgbaBoundingBox cbb;
			CalculateBoundingBox(cbb, bitmap, x, y);
			PvrTcPacket* packet = packets + GetMortonNumber(x, y);
			packet->usePunchthroughAlpha = 0;
			packet->SetColorA(cbb.min);
			packet->SetColorB(cbb.max);
		}
	}
	
	for(int y = 0; y < blocks; ++y)
	{
		for(int x = 0; x < blocks; ++x)
		{
			const unsigned char (*factor)[4] = PvrTcPacket::BILINEAR_FACTORS;
			const ColorRgba<unsigned char>* data = bitmap.GetData() + y * 4 * size + x * 4;
			
			uint32_t modulationData = 0;
			
			for(int py = 0; py < 4; ++py)
			{
				const int yOffset = (py < 2) ? -1 : 0;
				const int y0 = (y + yOffset) & blockMask;
				const int y1 = (y0+1) & blockMask;
				
				for(int px = 0; px < 4; ++px)
				{
					const int xOffset = (px < 2) ? -1 : 0;
					const int x0 = (x + xOffset) & blockMask;
					const int x1 = (x0+1) & blockMask;
					
					const PvrTcPacket* p0 = packets + GetMortonNumber(x0, y0);
					const PvrTcPacket* p1 = packets + GetMortonNumber(x1, y0);
					const PvrTcPacket* p2 = packets + GetMortonNumber(x0, y1);
					const PvrTcPacket* p3 = packets + GetMortonNumber(x1, y1);
					
					ColorRgba<int> ca = p0->GetColorRgbaA() * (*factor)[0] +
										p1->GetColorRgbaA() * (*factor)[1] +
										p2->GetColorRgbaA() * (*factor)[2] +
										p3->GetColorRgbaA() * (*factor)[3];
					
					ColorRgba<int> cb = p0->GetColorRgbaB() * (*factor)[0] +
										p1->GetColorRgbaB() * (*factor)[1] +
										p2->GetColorRgbaB() * (*factor)[2] +
										p3->GetColorRgbaB() * (*factor)[3];
					
					const ColorRgba<unsigned char>& pixel = data[py*size + px];
					ColorRgba<int> d = cb - ca;
					ColorRgba<int> p{pixel.r*16, pixel.g*16, pixel.b*16, pixel.a*16};
					ColorRgba<int> v = p - ca;
					
					// PVRTC uses weightings of 0, 3/8, 5/8 and 1
					// The boundaries for these are 3/16, 1/2 (=8/16), 13/16
					int projection = (v % d) * 16;
					int lengthSquared = d % d;
					if(projection > 3*lengthSquared) modulationData++;
					if(projection > 8*lengthSquared) modulationData++;
					if(projection > 13*lengthSquared) modulationData++;
					
					modulationData = BitUtility::RotateRight(modulationData, 2);
					
					factor++;
				}
			}
			
			PvrTcPacket* packet = packets + GetMortonNumber(x, y);
			packet->modulationData = modulationData;
		}
	}
}

void EncodeRgba4Bpp(const void *inBuf, void *outBuffer, unsigned int width, unsigned int height, bool isOpaque) {
	RgbaBitmap bitmap(width, height, (unsigned char *)inBuf);

	if (isOpaque) {
		EncodeRgb4Bpp(outBuffer, bitmap);
	} else {
		EncodeRgba4Bpp(outBuffer, bitmap);
	}
}

}