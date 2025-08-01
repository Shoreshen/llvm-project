; NOTE: Assertions have been autogenerated by utils/update_llc_test_checks.py UTC_ARGS: --version 2
; RUN: llc -amdgpu-scalarize-global-loads=false -mtriple=amdgcn -mcpu=tahiti -enable-unsafe-fp-math < %s | FileCheck -check-prefixes=SI %s
; RUN: llc -amdgpu-scalarize-global-loads=false -mtriple=amdgcn -mcpu=fiji -mattr=-flat-for-global -enable-unsafe-fp-math < %s | FileCheck -check-prefixes=VI %s
; RUN: llc -amdgpu-scalarize-global-loads=false -mtriple=amdgcn -mcpu=gfx1100 -mattr=-flat-for-global,+real-true16 -enable-unsafe-fp-math < %s | FileCheck -check-prefixes=GFX11-TRUE16 %s
; RUN: llc -amdgpu-scalarize-global-loads=false -mtriple=amdgcn -mcpu=gfx1100 -mattr=-flat-for-global,-real-true16 -enable-unsafe-fp-math < %s | FileCheck -check-prefixes=GFX11-FAKE16 %s

define amdgpu_kernel void @sitofp_i16_to_f16(
; SI-LABEL: sitofp_i16_to_f16:
; SI:       ; %bb.0: ; %entry
; SI-NEXT:    s_load_dwordx4 s[0:3], s[4:5], 0x9
; SI-NEXT:    s_mov_b32 s7, 0xf000
; SI-NEXT:    s_mov_b32 s6, -1
; SI-NEXT:    s_mov_b32 s10, s6
; SI-NEXT:    s_mov_b32 s11, s7
; SI-NEXT:    s_waitcnt lgkmcnt(0)
; SI-NEXT:    s_mov_b32 s8, s2
; SI-NEXT:    s_mov_b32 s9, s3
; SI-NEXT:    buffer_load_sshort v0, off, s[8:11], 0
; SI-NEXT:    s_mov_b32 s4, s0
; SI-NEXT:    s_mov_b32 s5, s1
; SI-NEXT:    s_waitcnt vmcnt(0)
; SI-NEXT:    v_cvt_f32_i32_e32 v0, v0
; SI-NEXT:    v_cvt_f16_f32_e32 v0, v0
; SI-NEXT:    buffer_store_short v0, off, s[4:7], 0
; SI-NEXT:    s_endpgm
;
; VI-LABEL: sitofp_i16_to_f16:
; VI:       ; %bb.0: ; %entry
; VI-NEXT:    s_load_dwordx4 s[0:3], s[4:5], 0x24
; VI-NEXT:    s_mov_b32 s7, 0xf000
; VI-NEXT:    s_mov_b32 s6, -1
; VI-NEXT:    s_mov_b32 s10, s6
; VI-NEXT:    s_mov_b32 s11, s7
; VI-NEXT:    s_waitcnt lgkmcnt(0)
; VI-NEXT:    s_mov_b32 s8, s2
; VI-NEXT:    s_mov_b32 s9, s3
; VI-NEXT:    buffer_load_ushort v0, off, s[8:11], 0
; VI-NEXT:    s_mov_b32 s4, s0
; VI-NEXT:    s_mov_b32 s5, s1
; VI-NEXT:    s_waitcnt vmcnt(0)
; VI-NEXT:    v_cvt_f16_i16_e32 v0, v0
; VI-NEXT:    buffer_store_short v0, off, s[4:7], 0
; VI-NEXT:    s_endpgm
;
; GFX11-TRUE16-LABEL: sitofp_i16_to_f16:
; GFX11-TRUE16:       ; %bb.0: ; %entry
; GFX11-TRUE16-NEXT:    s_load_b128 s[0:3], s[4:5], 0x24
; GFX11-TRUE16-NEXT:    s_mov_b32 s6, -1
; GFX11-TRUE16-NEXT:    s_mov_b32 s7, 0x31016000
; GFX11-TRUE16-NEXT:    s_mov_b32 s10, s6
; GFX11-TRUE16-NEXT:    s_mov_b32 s11, s7
; GFX11-TRUE16-NEXT:    s_waitcnt lgkmcnt(0)
; GFX11-TRUE16-NEXT:    s_mov_b32 s8, s2
; GFX11-TRUE16-NEXT:    s_mov_b32 s9, s3
; GFX11-TRUE16-NEXT:    s_mov_b32 s4, s0
; GFX11-TRUE16-NEXT:    buffer_load_u16 v0, off, s[8:11], 0
; GFX11-TRUE16-NEXT:    s_mov_b32 s5, s1
; GFX11-TRUE16-NEXT:    s_waitcnt vmcnt(0)
; GFX11-TRUE16-NEXT:    v_cvt_f16_i16_e32 v0.l, v0.l
; GFX11-TRUE16-NEXT:    buffer_store_b16 v0, off, s[4:7], 0
; GFX11-TRUE16-NEXT:    s_endpgm
;
; GFX11-FAKE16-LABEL: sitofp_i16_to_f16:
; GFX11-FAKE16:       ; %bb.0: ; %entry
; GFX11-FAKE16-NEXT:    s_load_b128 s[0:3], s[4:5], 0x24
; GFX11-FAKE16-NEXT:    s_mov_b32 s6, -1
; GFX11-FAKE16-NEXT:    s_mov_b32 s7, 0x31016000
; GFX11-FAKE16-NEXT:    s_mov_b32 s10, s6
; GFX11-FAKE16-NEXT:    s_mov_b32 s11, s7
; GFX11-FAKE16-NEXT:    s_waitcnt lgkmcnt(0)
; GFX11-FAKE16-NEXT:    s_mov_b32 s8, s2
; GFX11-FAKE16-NEXT:    s_mov_b32 s9, s3
; GFX11-FAKE16-NEXT:    s_mov_b32 s4, s0
; GFX11-FAKE16-NEXT:    buffer_load_u16 v0, off, s[8:11], 0
; GFX11-FAKE16-NEXT:    s_mov_b32 s5, s1
; GFX11-FAKE16-NEXT:    s_waitcnt vmcnt(0)
; GFX11-FAKE16-NEXT:    v_cvt_f16_i16_e32 v0, v0
; GFX11-FAKE16-NEXT:    buffer_store_b16 v0, off, s[4:7], 0
; GFX11-FAKE16-NEXT:    s_endpgm
    ptr addrspace(1) %r,
    ptr addrspace(1) %a) {
entry:
  %a.val = load i16, ptr addrspace(1) %a
  %r.val = sitofp i16 %a.val to half
  store half %r.val, ptr addrspace(1) %r
  ret void
}

define amdgpu_kernel void @sitofp_i32_to_f16(
; SI-LABEL: sitofp_i32_to_f16:
; SI:       ; %bb.0: ; %entry
; SI-NEXT:    s_load_dwordx4 s[0:3], s[4:5], 0x9
; SI-NEXT:    s_mov_b32 s7, 0xf000
; SI-NEXT:    s_mov_b32 s6, -1
; SI-NEXT:    s_mov_b32 s10, s6
; SI-NEXT:    s_mov_b32 s11, s7
; SI-NEXT:    s_waitcnt lgkmcnt(0)
; SI-NEXT:    s_mov_b32 s8, s2
; SI-NEXT:    s_mov_b32 s9, s3
; SI-NEXT:    buffer_load_dword v0, off, s[8:11], 0
; SI-NEXT:    s_mov_b32 s4, s0
; SI-NEXT:    s_mov_b32 s5, s1
; SI-NEXT:    s_waitcnt vmcnt(0)
; SI-NEXT:    v_cvt_f32_i32_e32 v0, v0
; SI-NEXT:    v_cvt_f16_f32_e32 v0, v0
; SI-NEXT:    buffer_store_short v0, off, s[4:7], 0
; SI-NEXT:    s_endpgm
;
; VI-LABEL: sitofp_i32_to_f16:
; VI:       ; %bb.0: ; %entry
; VI-NEXT:    s_load_dwordx4 s[0:3], s[4:5], 0x24
; VI-NEXT:    s_mov_b32 s7, 0xf000
; VI-NEXT:    s_mov_b32 s6, -1
; VI-NEXT:    s_mov_b32 s10, s6
; VI-NEXT:    s_mov_b32 s11, s7
; VI-NEXT:    s_waitcnt lgkmcnt(0)
; VI-NEXT:    s_mov_b32 s8, s2
; VI-NEXT:    s_mov_b32 s9, s3
; VI-NEXT:    buffer_load_dword v0, off, s[8:11], 0
; VI-NEXT:    s_mov_b32 s4, s0
; VI-NEXT:    s_mov_b32 s5, s1
; VI-NEXT:    s_waitcnt vmcnt(0)
; VI-NEXT:    v_cvt_f32_i32_e32 v0, v0
; VI-NEXT:    v_cvt_f16_f32_e32 v0, v0
; VI-NEXT:    buffer_store_short v0, off, s[4:7], 0
; VI-NEXT:    s_endpgm
;
; GFX11-TRUE16-LABEL: sitofp_i32_to_f16:
; GFX11-TRUE16:       ; %bb.0: ; %entry
; GFX11-TRUE16-NEXT:    s_load_b128 s[0:3], s[4:5], 0x24
; GFX11-TRUE16-NEXT:    s_mov_b32 s6, -1
; GFX11-TRUE16-NEXT:    s_mov_b32 s7, 0x31016000
; GFX11-TRUE16-NEXT:    s_mov_b32 s10, s6
; GFX11-TRUE16-NEXT:    s_mov_b32 s11, s7
; GFX11-TRUE16-NEXT:    s_waitcnt lgkmcnt(0)
; GFX11-TRUE16-NEXT:    s_mov_b32 s8, s2
; GFX11-TRUE16-NEXT:    s_mov_b32 s9, s3
; GFX11-TRUE16-NEXT:    s_mov_b32 s4, s0
; GFX11-TRUE16-NEXT:    buffer_load_b32 v0, off, s[8:11], 0
; GFX11-TRUE16-NEXT:    s_mov_b32 s5, s1
; GFX11-TRUE16-NEXT:    s_waitcnt vmcnt(0)
; GFX11-TRUE16-NEXT:    v_cvt_f32_i32_e32 v0, v0
; GFX11-TRUE16-NEXT:    s_delay_alu instid0(VALU_DEP_1)
; GFX11-TRUE16-NEXT:    v_cvt_f16_f32_e32 v0.l, v0
; GFX11-TRUE16-NEXT:    buffer_store_b16 v0, off, s[4:7], 0
; GFX11-TRUE16-NEXT:    s_endpgm
;
; GFX11-FAKE16-LABEL: sitofp_i32_to_f16:
; GFX11-FAKE16:       ; %bb.0: ; %entry
; GFX11-FAKE16-NEXT:    s_load_b128 s[0:3], s[4:5], 0x24
; GFX11-FAKE16-NEXT:    s_mov_b32 s6, -1
; GFX11-FAKE16-NEXT:    s_mov_b32 s7, 0x31016000
; GFX11-FAKE16-NEXT:    s_mov_b32 s10, s6
; GFX11-FAKE16-NEXT:    s_mov_b32 s11, s7
; GFX11-FAKE16-NEXT:    s_waitcnt lgkmcnt(0)
; GFX11-FAKE16-NEXT:    s_mov_b32 s8, s2
; GFX11-FAKE16-NEXT:    s_mov_b32 s9, s3
; GFX11-FAKE16-NEXT:    s_mov_b32 s4, s0
; GFX11-FAKE16-NEXT:    buffer_load_b32 v0, off, s[8:11], 0
; GFX11-FAKE16-NEXT:    s_mov_b32 s5, s1
; GFX11-FAKE16-NEXT:    s_waitcnt vmcnt(0)
; GFX11-FAKE16-NEXT:    v_cvt_f32_i32_e32 v0, v0
; GFX11-FAKE16-NEXT:    s_delay_alu instid0(VALU_DEP_1)
; GFX11-FAKE16-NEXT:    v_cvt_f16_f32_e32 v0, v0
; GFX11-FAKE16-NEXT:    buffer_store_b16 v0, off, s[4:7], 0
; GFX11-FAKE16-NEXT:    s_endpgm
    ptr addrspace(1) %r,
    ptr addrspace(1) %a) {
entry:
  %a.val = load i32, ptr addrspace(1) %a
  %r.val = sitofp i32 %a.val to half
  store half %r.val, ptr addrspace(1) %r
  ret void
}

; f16 = sitofp i64 is in sint_to_fp.i64.ll

define amdgpu_kernel void @sitofp_v2i16_to_v2f16(
; SI-LABEL: sitofp_v2i16_to_v2f16:
; SI:       ; %bb.0: ; %entry
; SI-NEXT:    s_load_dwordx4 s[0:3], s[4:5], 0x9
; SI-NEXT:    s_mov_b32 s7, 0xf000
; SI-NEXT:    s_mov_b32 s6, -1
; SI-NEXT:    s_mov_b32 s10, s6
; SI-NEXT:    s_mov_b32 s11, s7
; SI-NEXT:    s_waitcnt lgkmcnt(0)
; SI-NEXT:    s_mov_b32 s8, s2
; SI-NEXT:    s_mov_b32 s9, s3
; SI-NEXT:    buffer_load_dword v0, off, s[8:11], 0
; SI-NEXT:    s_mov_b32 s4, s0
; SI-NEXT:    s_mov_b32 s5, s1
; SI-NEXT:    s_waitcnt vmcnt(0)
; SI-NEXT:    v_bfe_i32 v1, v0, 0, 16
; SI-NEXT:    v_ashrrev_i32_e32 v0, 16, v0
; SI-NEXT:    v_cvt_f32_i32_e32 v0, v0
; SI-NEXT:    v_cvt_f32_i32_e32 v1, v1
; SI-NEXT:    v_cvt_f16_f32_e32 v0, v0
; SI-NEXT:    v_cvt_f16_f32_e32 v1, v1
; SI-NEXT:    v_lshlrev_b32_e32 v0, 16, v0
; SI-NEXT:    v_or_b32_e32 v0, v1, v0
; SI-NEXT:    buffer_store_dword v0, off, s[4:7], 0
; SI-NEXT:    s_endpgm
;
; VI-LABEL: sitofp_v2i16_to_v2f16:
; VI:       ; %bb.0: ; %entry
; VI-NEXT:    s_load_dwordx4 s[0:3], s[4:5], 0x24
; VI-NEXT:    s_mov_b32 s7, 0xf000
; VI-NEXT:    s_mov_b32 s6, -1
; VI-NEXT:    s_mov_b32 s10, s6
; VI-NEXT:    s_mov_b32 s11, s7
; VI-NEXT:    s_waitcnt lgkmcnt(0)
; VI-NEXT:    s_mov_b32 s8, s2
; VI-NEXT:    s_mov_b32 s9, s3
; VI-NEXT:    buffer_load_dword v0, off, s[8:11], 0
; VI-NEXT:    s_mov_b32 s4, s0
; VI-NEXT:    s_mov_b32 s5, s1
; VI-NEXT:    s_waitcnt vmcnt(0)
; VI-NEXT:    v_cvt_f16_i16_sdwa v1, v0 dst_sel:WORD_1 dst_unused:UNUSED_PAD src0_sel:WORD_1
; VI-NEXT:    v_cvt_f16_i16_e32 v0, v0
; VI-NEXT:    v_or_b32_e32 v0, v0, v1
; VI-NEXT:    buffer_store_dword v0, off, s[4:7], 0
; VI-NEXT:    s_endpgm
;
; GFX11-TRUE16-LABEL: sitofp_v2i16_to_v2f16:
; GFX11-TRUE16:       ; %bb.0: ; %entry
; GFX11-TRUE16-NEXT:    s_load_b128 s[0:3], s[4:5], 0x24
; GFX11-TRUE16-NEXT:    s_mov_b32 s6, -1
; GFX11-TRUE16-NEXT:    s_mov_b32 s7, 0x31016000
; GFX11-TRUE16-NEXT:    s_mov_b32 s10, s6
; GFX11-TRUE16-NEXT:    s_mov_b32 s11, s7
; GFX11-TRUE16-NEXT:    s_waitcnt lgkmcnt(0)
; GFX11-TRUE16-NEXT:    s_mov_b32 s8, s2
; GFX11-TRUE16-NEXT:    s_mov_b32 s9, s3
; GFX11-TRUE16-NEXT:    s_mov_b32 s4, s0
; GFX11-TRUE16-NEXT:    buffer_load_b32 v0, off, s[8:11], 0
; GFX11-TRUE16-NEXT:    s_mov_b32 s5, s1
; GFX11-TRUE16-NEXT:    s_waitcnt vmcnt(0)
; GFX11-TRUE16-NEXT:    v_lshrrev_b32_e32 v1, 16, v0
; GFX11-TRUE16-NEXT:    v_cvt_f16_i16_e32 v0.l, v0.l
; GFX11-TRUE16-NEXT:    s_delay_alu instid0(VALU_DEP_2) | instskip(NEXT) | instid1(VALU_DEP_1)
; GFX11-TRUE16-NEXT:    v_cvt_f16_i16_e32 v0.h, v1.l
; GFX11-TRUE16-NEXT:    v_pack_b32_f16 v0, v0.l, v0.h
; GFX11-TRUE16-NEXT:    buffer_store_b32 v0, off, s[4:7], 0
; GFX11-TRUE16-NEXT:    s_endpgm
;
; GFX11-FAKE16-LABEL: sitofp_v2i16_to_v2f16:
; GFX11-FAKE16:       ; %bb.0: ; %entry
; GFX11-FAKE16-NEXT:    s_load_b128 s[0:3], s[4:5], 0x24
; GFX11-FAKE16-NEXT:    s_mov_b32 s6, -1
; GFX11-FAKE16-NEXT:    s_mov_b32 s7, 0x31016000
; GFX11-FAKE16-NEXT:    s_mov_b32 s10, s6
; GFX11-FAKE16-NEXT:    s_mov_b32 s11, s7
; GFX11-FAKE16-NEXT:    s_waitcnt lgkmcnt(0)
; GFX11-FAKE16-NEXT:    s_mov_b32 s8, s2
; GFX11-FAKE16-NEXT:    s_mov_b32 s9, s3
; GFX11-FAKE16-NEXT:    s_mov_b32 s4, s0
; GFX11-FAKE16-NEXT:    buffer_load_b32 v0, off, s[8:11], 0
; GFX11-FAKE16-NEXT:    s_mov_b32 s5, s1
; GFX11-FAKE16-NEXT:    s_waitcnt vmcnt(0)
; GFX11-FAKE16-NEXT:    v_lshrrev_b32_e32 v1, 16, v0
; GFX11-FAKE16-NEXT:    v_cvt_f16_i16_e32 v0, v0
; GFX11-FAKE16-NEXT:    s_delay_alu instid0(VALU_DEP_2) | instskip(NEXT) | instid1(VALU_DEP_1)
; GFX11-FAKE16-NEXT:    v_cvt_f16_i16_e32 v1, v1
; GFX11-FAKE16-NEXT:    v_pack_b32_f16 v0, v0, v1
; GFX11-FAKE16-NEXT:    buffer_store_b32 v0, off, s[4:7], 0
; GFX11-FAKE16-NEXT:    s_endpgm
    ptr addrspace(1) %r,
    ptr addrspace(1) %a) {
entry:
  %a.val = load <2 x i16>, ptr addrspace(1) %a
  %r.val = sitofp <2 x i16> %a.val to <2 x half>
  store <2 x half> %r.val, ptr addrspace(1) %r
  ret void
}

define amdgpu_kernel void @sitofp_v2i32_to_v2f16(
; SI-LABEL: sitofp_v2i32_to_v2f16:
; SI:       ; %bb.0: ; %entry
; SI-NEXT:    s_load_dwordx4 s[0:3], s[4:5], 0x9
; SI-NEXT:    s_mov_b32 s7, 0xf000
; SI-NEXT:    s_mov_b32 s6, -1
; SI-NEXT:    s_mov_b32 s10, s6
; SI-NEXT:    s_mov_b32 s11, s7
; SI-NEXT:    s_waitcnt lgkmcnt(0)
; SI-NEXT:    s_mov_b32 s8, s2
; SI-NEXT:    s_mov_b32 s9, s3
; SI-NEXT:    buffer_load_dwordx2 v[0:1], off, s[8:11], 0
; SI-NEXT:    s_mov_b32 s4, s0
; SI-NEXT:    s_mov_b32 s5, s1
; SI-NEXT:    s_waitcnt vmcnt(0)
; SI-NEXT:    v_cvt_f32_i32_e32 v1, v1
; SI-NEXT:    v_cvt_f32_i32_e32 v0, v0
; SI-NEXT:    v_cvt_f16_f32_e32 v1, v1
; SI-NEXT:    v_cvt_f16_f32_e32 v0, v0
; SI-NEXT:    v_lshlrev_b32_e32 v1, 16, v1
; SI-NEXT:    v_or_b32_e32 v0, v0, v1
; SI-NEXT:    buffer_store_dword v0, off, s[4:7], 0
; SI-NEXT:    s_endpgm
;
; VI-LABEL: sitofp_v2i32_to_v2f16:
; VI:       ; %bb.0: ; %entry
; VI-NEXT:    s_load_dwordx4 s[0:3], s[4:5], 0x24
; VI-NEXT:    s_mov_b32 s7, 0xf000
; VI-NEXT:    s_mov_b32 s6, -1
; VI-NEXT:    s_mov_b32 s10, s6
; VI-NEXT:    s_mov_b32 s11, s7
; VI-NEXT:    s_waitcnt lgkmcnt(0)
; VI-NEXT:    s_mov_b32 s8, s2
; VI-NEXT:    s_mov_b32 s9, s3
; VI-NEXT:    buffer_load_dwordx2 v[0:1], off, s[8:11], 0
; VI-NEXT:    s_mov_b32 s4, s0
; VI-NEXT:    s_mov_b32 s5, s1
; VI-NEXT:    s_waitcnt vmcnt(0)
; VI-NEXT:    v_cvt_f32_i32_e32 v1, v1
; VI-NEXT:    v_cvt_f32_i32_e32 v0, v0
; VI-NEXT:    v_cvt_f16_f32_sdwa v1, v1 dst_sel:WORD_1 dst_unused:UNUSED_PAD src0_sel:DWORD
; VI-NEXT:    v_cvt_f16_f32_e32 v0, v0
; VI-NEXT:    v_or_b32_e32 v0, v0, v1
; VI-NEXT:    buffer_store_dword v0, off, s[4:7], 0
; VI-NEXT:    s_endpgm
;
; GFX11-TRUE16-LABEL: sitofp_v2i32_to_v2f16:
; GFX11-TRUE16:       ; %bb.0: ; %entry
; GFX11-TRUE16-NEXT:    s_load_b128 s[0:3], s[4:5], 0x24
; GFX11-TRUE16-NEXT:    s_mov_b32 s6, -1
; GFX11-TRUE16-NEXT:    s_mov_b32 s7, 0x31016000
; GFX11-TRUE16-NEXT:    s_mov_b32 s10, s6
; GFX11-TRUE16-NEXT:    s_mov_b32 s11, s7
; GFX11-TRUE16-NEXT:    s_waitcnt lgkmcnt(0)
; GFX11-TRUE16-NEXT:    s_mov_b32 s8, s2
; GFX11-TRUE16-NEXT:    s_mov_b32 s9, s3
; GFX11-TRUE16-NEXT:    s_mov_b32 s4, s0
; GFX11-TRUE16-NEXT:    buffer_load_b64 v[0:1], off, s[8:11], 0
; GFX11-TRUE16-NEXT:    s_mov_b32 s5, s1
; GFX11-TRUE16-NEXT:    s_waitcnt vmcnt(0)
; GFX11-TRUE16-NEXT:    v_cvt_f32_i32_e32 v1, v1
; GFX11-TRUE16-NEXT:    v_cvt_f32_i32_e32 v2, v0
; GFX11-TRUE16-NEXT:    s_delay_alu instid0(VALU_DEP_2) | instskip(NEXT) | instid1(VALU_DEP_2)
; GFX11-TRUE16-NEXT:    v_cvt_f16_f32_e32 v0.l, v1
; GFX11-TRUE16-NEXT:    v_cvt_f16_f32_e32 v0.h, v2
; GFX11-TRUE16-NEXT:    s_delay_alu instid0(VALU_DEP_1)
; GFX11-TRUE16-NEXT:    v_pack_b32_f16 v0, v0.h, v0.l
; GFX11-TRUE16-NEXT:    buffer_store_b32 v0, off, s[4:7], 0
; GFX11-TRUE16-NEXT:    s_endpgm
;
; GFX11-FAKE16-LABEL: sitofp_v2i32_to_v2f16:
; GFX11-FAKE16:       ; %bb.0: ; %entry
; GFX11-FAKE16-NEXT:    s_load_b128 s[0:3], s[4:5], 0x24
; GFX11-FAKE16-NEXT:    s_mov_b32 s6, -1
; GFX11-FAKE16-NEXT:    s_mov_b32 s7, 0x31016000
; GFX11-FAKE16-NEXT:    s_mov_b32 s10, s6
; GFX11-FAKE16-NEXT:    s_mov_b32 s11, s7
; GFX11-FAKE16-NEXT:    s_waitcnt lgkmcnt(0)
; GFX11-FAKE16-NEXT:    s_mov_b32 s8, s2
; GFX11-FAKE16-NEXT:    s_mov_b32 s9, s3
; GFX11-FAKE16-NEXT:    s_mov_b32 s4, s0
; GFX11-FAKE16-NEXT:    buffer_load_b64 v[0:1], off, s[8:11], 0
; GFX11-FAKE16-NEXT:    s_mov_b32 s5, s1
; GFX11-FAKE16-NEXT:    s_waitcnt vmcnt(0)
; GFX11-FAKE16-NEXT:    v_cvt_f32_i32_e32 v1, v1
; GFX11-FAKE16-NEXT:    v_cvt_f32_i32_e32 v0, v0
; GFX11-FAKE16-NEXT:    s_delay_alu instid0(VALU_DEP_2) | instskip(NEXT) | instid1(VALU_DEP_2)
; GFX11-FAKE16-NEXT:    v_cvt_f16_f32_e32 v1, v1
; GFX11-FAKE16-NEXT:    v_cvt_f16_f32_e32 v0, v0
; GFX11-FAKE16-NEXT:    s_delay_alu instid0(VALU_DEP_1)
; GFX11-FAKE16-NEXT:    v_pack_b32_f16 v0, v0, v1
; GFX11-FAKE16-NEXT:    buffer_store_b32 v0, off, s[4:7], 0
; GFX11-FAKE16-NEXT:    s_endpgm
    ptr addrspace(1) %r,
    ptr addrspace(1) %a) {
entry:
  %a.val = load <2 x i32>, ptr addrspace(1) %a
  %r.val = sitofp <2 x i32> %a.val to <2 x half>
  store <2 x half> %r.val, ptr addrspace(1) %r
  ret void
}

define amdgpu_kernel void @s_sint_to_fp_i1_to_f16(ptr addrspace(1) %out, ptr addrspace(1) %in0, ptr addrspace(1) %in1) {
; SI-LABEL: s_sint_to_fp_i1_to_f16:
; SI:       ; %bb.0:
; SI-NEXT:    s_load_dwordx4 s[8:11], s[4:5], 0x9
; SI-NEXT:    s_load_dwordx2 s[4:5], s[4:5], 0xd
; SI-NEXT:    s_mov_b32 s3, 0xf000
; SI-NEXT:    s_mov_b32 s2, -1
; SI-NEXT:    s_mov_b32 s6, s2
; SI-NEXT:    s_mov_b32 s7, s3
; SI-NEXT:    s_waitcnt lgkmcnt(0)
; SI-NEXT:    s_mov_b32 s12, s10
; SI-NEXT:    s_mov_b32 s13, s11
; SI-NEXT:    s_mov_b32 s14, s2
; SI-NEXT:    s_mov_b32 s15, s3
; SI-NEXT:    buffer_load_dword v0, off, s[4:7], 0
; SI-NEXT:    buffer_load_dword v1, off, s[12:15], 0
; SI-NEXT:    s_waitcnt vmcnt(1)
; SI-NEXT:    v_cmp_le_f32_e32 vcc, 1.0, v0
; SI-NEXT:    s_waitcnt vmcnt(0)
; SI-NEXT:    v_cmp_le_f32_e64 s[0:1], 0, v1
; SI-NEXT:    s_xor_b64 s[0:1], s[0:1], vcc
; SI-NEXT:    v_cndmask_b32_e64 v0, 0, -1.0, s[0:1]
; SI-NEXT:    v_cvt_f16_f32_e32 v0, v0
; SI-NEXT:    s_mov_b32 s0, s8
; SI-NEXT:    s_mov_b32 s1, s9
; SI-NEXT:    buffer_store_short v0, off, s[0:3], 0
; SI-NEXT:    s_endpgm
;
; VI-LABEL: s_sint_to_fp_i1_to_f16:
; VI:       ; %bb.0:
; VI-NEXT:    s_load_dwordx4 s[8:11], s[4:5], 0x24
; VI-NEXT:    s_load_dwordx2 s[4:5], s[4:5], 0x34
; VI-NEXT:    s_mov_b32 s3, 0xf000
; VI-NEXT:    s_mov_b32 s2, -1
; VI-NEXT:    s_mov_b32 s6, s2
; VI-NEXT:    s_mov_b32 s7, s3
; VI-NEXT:    s_waitcnt lgkmcnt(0)
; VI-NEXT:    s_mov_b32 s12, s10
; VI-NEXT:    s_mov_b32 s13, s11
; VI-NEXT:    s_mov_b32 s14, s2
; VI-NEXT:    s_mov_b32 s15, s3
; VI-NEXT:    buffer_load_dword v0, off, s[4:7], 0
; VI-NEXT:    buffer_load_dword v1, off, s[12:15], 0
; VI-NEXT:    s_waitcnt vmcnt(1)
; VI-NEXT:    v_cmp_le_f32_e32 vcc, 1.0, v0
; VI-NEXT:    s_waitcnt vmcnt(0)
; VI-NEXT:    v_cmp_le_f32_e64 s[0:1], 0, v1
; VI-NEXT:    s_xor_b64 s[0:1], s[0:1], vcc
; VI-NEXT:    v_cndmask_b32_e64 v0, 0, -1.0, s[0:1]
; VI-NEXT:    v_cvt_f16_f32_e32 v0, v0
; VI-NEXT:    s_mov_b32 s0, s8
; VI-NEXT:    s_mov_b32 s1, s9
; VI-NEXT:    buffer_store_short v0, off, s[0:3], 0
; VI-NEXT:    s_endpgm
;
; GFX11-TRUE16-LABEL: s_sint_to_fp_i1_to_f16:
; GFX11-TRUE16:       ; %bb.0:
; GFX11-TRUE16-NEXT:    s_clause 0x1
; GFX11-TRUE16-NEXT:    s_load_b128 s[8:11], s[4:5], 0x24
; GFX11-TRUE16-NEXT:    s_load_b64 s[0:1], s[4:5], 0x34
; GFX11-TRUE16-NEXT:    s_mov_b32 s6, -1
; GFX11-TRUE16-NEXT:    s_mov_b32 s7, 0x31016000
; GFX11-TRUE16-NEXT:    s_mov_b32 s2, s6
; GFX11-TRUE16-NEXT:    s_mov_b32 s3, s7
; GFX11-TRUE16-NEXT:    s_mov_b32 s14, s6
; GFX11-TRUE16-NEXT:    s_mov_b32 s15, s7
; GFX11-TRUE16-NEXT:    s_waitcnt lgkmcnt(0)
; GFX11-TRUE16-NEXT:    s_mov_b32 s12, s10
; GFX11-TRUE16-NEXT:    s_mov_b32 s13, s11
; GFX11-TRUE16-NEXT:    buffer_load_b32 v0, off, s[0:3], 0
; GFX11-TRUE16-NEXT:    buffer_load_b32 v1, off, s[12:15], 0
; GFX11-TRUE16-NEXT:    s_mov_b32 s4, s8
; GFX11-TRUE16-NEXT:    s_mov_b32 s5, s9
; GFX11-TRUE16-NEXT:    s_waitcnt vmcnt(1)
; GFX11-TRUE16-NEXT:    v_cmp_le_f32_e32 vcc_lo, 1.0, v0
; GFX11-TRUE16-NEXT:    s_waitcnt vmcnt(0)
; GFX11-TRUE16-NEXT:    v_cmp_le_f32_e64 s0, 0, v1
; GFX11-TRUE16-NEXT:    s_xor_b32 s0, s0, vcc_lo
; GFX11-TRUE16-NEXT:    s_delay_alu instid0(SALU_CYCLE_1) | instskip(NEXT) | instid1(VALU_DEP_1)
; GFX11-TRUE16-NEXT:    v_cndmask_b32_e64 v0, 0, -1.0, s0
; GFX11-TRUE16-NEXT:    v_cvt_f16_f32_e32 v0.l, v0
; GFX11-TRUE16-NEXT:    buffer_store_b16 v0, off, s[4:7], 0
; GFX11-TRUE16-NEXT:    s_endpgm
;
; GFX11-FAKE16-LABEL: s_sint_to_fp_i1_to_f16:
; GFX11-FAKE16:       ; %bb.0:
; GFX11-FAKE16-NEXT:    s_clause 0x1
; GFX11-FAKE16-NEXT:    s_load_b128 s[8:11], s[4:5], 0x24
; GFX11-FAKE16-NEXT:    s_load_b64 s[0:1], s[4:5], 0x34
; GFX11-FAKE16-NEXT:    s_mov_b32 s6, -1
; GFX11-FAKE16-NEXT:    s_mov_b32 s7, 0x31016000
; GFX11-FAKE16-NEXT:    s_mov_b32 s2, s6
; GFX11-FAKE16-NEXT:    s_mov_b32 s3, s7
; GFX11-FAKE16-NEXT:    s_mov_b32 s14, s6
; GFX11-FAKE16-NEXT:    s_mov_b32 s15, s7
; GFX11-FAKE16-NEXT:    s_waitcnt lgkmcnt(0)
; GFX11-FAKE16-NEXT:    s_mov_b32 s12, s10
; GFX11-FAKE16-NEXT:    s_mov_b32 s13, s11
; GFX11-FAKE16-NEXT:    buffer_load_b32 v0, off, s[0:3], 0
; GFX11-FAKE16-NEXT:    buffer_load_b32 v1, off, s[12:15], 0
; GFX11-FAKE16-NEXT:    s_mov_b32 s4, s8
; GFX11-FAKE16-NEXT:    s_mov_b32 s5, s9
; GFX11-FAKE16-NEXT:    s_waitcnt vmcnt(1)
; GFX11-FAKE16-NEXT:    v_cmp_le_f32_e32 vcc_lo, 1.0, v0
; GFX11-FAKE16-NEXT:    s_waitcnt vmcnt(0)
; GFX11-FAKE16-NEXT:    v_cmp_le_f32_e64 s0, 0, v1
; GFX11-FAKE16-NEXT:    s_xor_b32 s0, s0, vcc_lo
; GFX11-FAKE16-NEXT:    s_delay_alu instid0(SALU_CYCLE_1) | instskip(NEXT) | instid1(VALU_DEP_1)
; GFX11-FAKE16-NEXT:    v_cndmask_b32_e64 v0, 0, -1.0, s0
; GFX11-FAKE16-NEXT:    v_cvt_f16_f32_e32 v0, v0
; GFX11-FAKE16-NEXT:    buffer_store_b16 v0, off, s[4:7], 0
; GFX11-FAKE16-NEXT:    s_endpgm
  %a = load float, ptr addrspace(1) %in0
  %b = load float, ptr addrspace(1) %in1
  %acmp = fcmp oge float %a, 0.000000e+00
  %bcmp = fcmp oge float %b, 1.000000e+00
  %result = xor i1 %acmp, %bcmp
  %fp = sitofp i1 %result to half
  store half %fp, ptr addrspace(1) %out
  ret void
}

; v2f16 = sitofp v2i64 is in sint_to_fp.i64.ll
