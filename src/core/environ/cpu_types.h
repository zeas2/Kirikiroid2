// ; this is a part of TVP (KIRIKIRI) software source.
// ; see other sources for license.
// ; (C)2001-2003 W.Dee <dee@kikyou.info> and contributors
// 
// ;;[emit_c_h]/*[*/
// ;;[emit_c_h]//---------------------------------------------------------------------------
// ;;[emit_c_h]// CPU Types
// ;;[emit_c_h]//---------------------------------------------------------------------------
// ;;[emit_c_h]/*]*/
// 
// ;;[emit_c_h_equ_begin]
#pragma once

#define TVP_CPU_HAS_FPU			0x00010000
#define TVP_CPU_HAS_MMX			0x00020000
#define TVP_CPU_HAS_3DN			0x00040000
#define TVP_CPU_HAS_SSE			0x00080000
#define TVP_CPU_HAS_CMOV		0x00100000
#define TVP_CPU_HAS_E3DN		0x00200000
#define TVP_CPU_HAS_EMMX		0x00400000
#define TVP_CPU_HAS_SSE2		0x00800000
#define TVP_CPU_HAS_TSC			0x01000000
#define TVP_CPU_HAS_NEON		0x02000000

#define TVP_CPU_FEATURE_MASK	0xffff0000
    
#define TVP_CPU_IS_INTEL		0x00000010
#define TVP_CPU_IS_AMD			0x00000020
#define TVP_CPU_IS_IDT			0x00000030
#define TVP_CPU_IS_CYRIX		0x00000040
#define TVP_CPU_IS_NEXGEN		0x00000050
#define TVP_CPU_IS_RISE			0x00000060
#define TVP_CPU_IS_UMC			0x00000070
#define TVP_CPU_IS_TRANSMETA	0x00000080
#define TVP_CPU_IS_UNKNOWN		0x00000000

#define TVP_CPU_VENDOR_MASK		0x00000ff0
    
#define TVP_CPU_FAMILY_X86      0x00000001
#define TVP_CPU_FAMILY_X64      0x00000002
#define TVP_CPU_FAMILY_ARM      0x00000003
#define TVP_CPU_FAMILY_MIPS     0x00000003

#define TVP_CPU_FAMILY_MASK		0x0000000f

#ifdef __cplusplus
extern "C" unsigned int TVPCPUFeatures;
#else
extern unsigned int TVPCPUFeatures;
#endif

// ;;[emit_c_h_equ_end]
// 
// ; note: EMMX is refered to as MMX2
