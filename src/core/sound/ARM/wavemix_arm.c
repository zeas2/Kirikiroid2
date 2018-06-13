#include <arm_neon.h>
#include "cpu_types.h"
#include <assert.h>
#include "Protect.h"

#ifdef __cplusplus
extern "C" {
#endif
extern const int MAX_VOLUME;
typedef void(FAudioMix)(void *dst, const void *src, int samples, int16_t *volume);
static FAudioMix* _VolumeMix_CPP_S16_CH2;

static void VolumeMix_NEON_S16_CH2(void *dst, const void *src, int len, int16_t *vol) {
	int16_t *dst16 = (int16_t *)dst;
	const int16_t *src16 = (const int16_t *)src;

	const int addr_mask = ~(8 - 1);
	signed short* pEndDst = dst16 + len * 2;
	// Pre Frag
	{
		int PreFragLen = ((int16_t*)((((intptr_t)dst16) + 7)&~7) - dst16) / 4;
		if (PreFragLen > len) PreFragLen = len;
		if (PreFragLen) {
			_VolumeMix_CPP_S16_CH2(dst16, src16, PreFragLen, vol);
			dst16 += PreFragLen * 2;
			src16 += PreFragLen * 2;
		}
	}

	int16_t* pVecEndDst = (int16_t*)(((intptr_t)pEndDst)&addr_mask) - 7;
	int16x4_t volume = vld1_s16(vol);
	int16x4_t sign_mask = vdup_n_s16(0x8000);

	while (dst16 < pVecEndDst) {
		int16x4_t src_sample = vld1_s16(src16);
		int16x4_t dest_sample = vld1_s16(dst16);
		int16x4_t src_sign = vreinterpret_s16_u16(vtst_s16(src_sample, sign_mask));
		int16x4_t dst_sign = vreinterpret_s16_u16(vtst_s16(dest_sample, sign_mask));
		src_sample = vshrn_n_s32(vmull_s16(src_sample, volume), 14);
		int16x4_t error = vshrn_n_s32(vmull_s16(src_sample, dest_sample), 15);
		int16x4_t same_sign = veor_s16(src_sign, dst_sign); // -1 = same sign
		error = vand_s16(error, same_sign);
		int16x4_t errneg = vneg_s16(error);
		errneg = vand_s16(errneg, src_sign);
		error = vand_s16(error, vmvn_s16(src_sign));
		int16x4_t result = vadd_s16(src_sample, dest_sample);
		result = vadd_s16(result, error);
		result = vadd_s16(result, errneg);
		vst1_s16(dst16, result);
		src16 += 4;
		dst16 += 4;
	}

	if (dst16 < pEndDst) {
		_VolumeMix_CPP_S16_CH2(dst16, src16, (pEndDst - dst16) / 2, vol);
	}
}

static FAudioMix* _VolumeMix_CPP_F32_CH2;
static void VolumeMix_NEON_F32_CH2(void *dst, const void *src, int len, int16_t *vol) {
	float *dst32 = (float *)dst;
	const float *src32 = (const float *)src;
	const int addr_mask = ~(16 - 1);
	float* pEndDst = dst32 + len;
	// Pre Frag
	{
		int PreFragLen = ((float*)((((intptr_t)dst32) + 15)&~15) - dst32) / 8;
		if (PreFragLen > len) PreFragLen = len / 2;
		if (PreFragLen) {
			_VolumeMix_CPP_S16_CH2(dst32, src32, PreFragLen, vol);
			dst32 += PreFragLen * 2;
			src32 += PreFragLen * 2;
		}
	}

	float* pVecEndDst = (float*)(((intptr_t)pEndDst)&addr_mask) - 15;
	float32x4_t volume = vcvtq_f32_s32(vmovl_s16(vld1_s16(vol)));
	int32x4_t sign_mask = vdupq_n_s32((int32_t)0x80000000);

	while (dst32 < pVecEndDst) {
		float32x4_t src_sample = vmulq_f32(vld1q_f32(src32), volume);
		float32x4_t dest_sample = vld1q_f32(dst32);
		int32x4_t src_sign = vreinterpretq_s32_u32(vtstq_s32(vreinterpretq_s32_f32(src_sample), sign_mask));
		int32x4_t dst_sign = vreinterpretq_s32_u32(vtstq_s32(vreinterpretq_s32_f32(dest_sample), sign_mask));
		float32x4_t error = vmulq_f32(src_sample, dest_sample);
		float32x4_t result = vaddq_f32(src_sample, dest_sample);
		result = vsubq_f32(result, error);
		vst1q_f32(dst32, result);
		src32 += 4;
		dst32 += 4;
	}

	if (dst32 < pEndDst) {
		_VolumeMix_CPP_F32_CH2(dst32, src32, (pEndDst - dst32) / 2, vol);
	}
}

void TVPWaveMixer_ASM_Init(FAudioMix **func16, FAudioMix **func32)
{
	return; // currently not used
	if ((TVPCPUFeatures & TVP_CPU_FAMILY_MASK) == TVP_CPU_FAMILY_ARM && (TVPCPUFeatures & TVP_CPU_HAS_NEON)) {
		_VolumeMix_CPP_S16_CH2 = func16[1];
		_VolumeMix_CPP_F32_CH2 = func32[1];
		func16[1] = VolumeMix_NEON_S16_CH2;
#if defined( DEBUG_ARM_NEON) && 1
		static const signed short testsrc_s16[12] = {
			-1,0,0x7FFF,-0x8000,
			0x7000, 0x7000, -0x7000, -0x7000,
		};
		static const signed short testdest_s16_c[12] = {
			-1,0,0x7FFF,-0x8000,
			0x7000, -0x7000, -0x7000, 0x7000,
		};

		signed short testdest_s16_1[12];
		short vol16[] = { 16384 , 16384 , 16384 , 16384 };
		memcpy(testdest_s16_1, testdest_s16_c, sizeof(testdest_s16_c));
		_VolumeMix_CPP_S16_CH2(testdest_s16_1, testsrc_s16, 4, vol16);
		signed short testdest_s16_2[12];
		memcpy(testdest_s16_2, testdest_s16_c, sizeof(testdest_s16_c));
		VolumeMix_NEON_S16_CH2(testdest_s16_2, testsrc_s16, 4, vol16);
		assert(!memcmp(testdest_s16_1, testdest_s16_2, sizeof(testdest_s16_1)));

		static const float testsrc_f32[12] = {
			0,0,0,0,
			0.99, 0.99, -0.99, -0.99,
		};
		static const float testdest_f32_c[12] = {
			0,0,0,0,
			0.99, -0.99, -0.99, 0.99,
		};

		float testdest_f32_1[12];
		memcpy(testdest_f32_1, testdest_f32_c, sizeof(testdest_f32_c));
		_VolumeMix_CPP_F32_CH2(testdest_f32_1, testsrc_f32, 4, vol16);
		float testdest_f32_2[12];
		memcpy(testdest_f32_2, testdest_f32_c, sizeof(testdest_f32_c));
		VolumeMix_NEON_F32_CH2(testdest_f32_2, testsrc_f32, 4, vol16);
		assert(!memcmp(testdest_f32_1, testdest_f32_2, sizeof(testdest_f32_1)));
#endif
		//VolumeMix = VolumeMix_NEON;
	}
}
#ifdef __cplusplus
}
#endif