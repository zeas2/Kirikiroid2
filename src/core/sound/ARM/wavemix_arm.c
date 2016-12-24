#include <arm_neon.h>
#include "cpu_types.h"
#include <assert.h>

#ifdef DEBUG_ARM_NEON
extern "C" {
#endif
extern const int MAX_VOLUME;

void TVPWaveMixer_ASM_Init()
{
	if ((TVPCPUFeatures & TVP_CPU_FAMILY_MASK) == TVP_CPU_FAMILY_ARM && (TVPCPUFeatures & TVP_CPU_HAS_NEON)) {
#if defined( DEBUG_ARM_NEON) && 0
		static const signed short testsrc_s16[12] = {
			-1,0,0x7FFF,-0x8000,
			0x7000, 0x7000, -0x7000, -0x7000,
		};
		static const signed short testdest_s16_c[12] = {
			-1,0,0x7FFF,-0x8000,
			0x7000, -0x7000, -0x7000, 0x7000,
		};

		signed short testdest_s16_1[12];
		memcpy(testdest_s16_1, testdest_s16_c, sizeof(testdest_s16_c));
		VolumeMix_c_s16(testdest_s16_1, testsrc_s16, 12, 16384);
		signed short testdest_s16_2[12];
		memcpy(testdest_s16_2, testdest_s16_c, sizeof(testdest_s16_c));
		VolumeMix_NEON_s16(testdest_s16_2, testsrc_s16, 12, 16384);
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
		VolumeMix_c_f32(testdest_f32_1, testsrc_f32, 12, 0.9);
		float testdest_f32_2[12];
		memcpy(testdest_f32_2, testdest_f32_c, sizeof(testdest_f32_c));
		VolumeMix_NEON_f32(testdest_f32_2, testsrc_f32, 12, 0.9);
		assert(!memcmp(testdest_f32_1, testdest_f32_2, sizeof(testdest_f32_1)));
#endif
		//VolumeMix = VolumeMix_NEON;
	}
}
#ifdef DEBUG_ARM_NEON
}
#endif