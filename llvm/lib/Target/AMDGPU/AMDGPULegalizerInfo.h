//===- AMDGPULegalizerInfo ---------------------------------------*- C++ -*-==//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file
/// This file declares the targeting of the Machinelegalizer class for
/// AMDGPU.
/// \todo This should be generated by TableGen.
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_AMDGPU_AMDGPUMACHINELEGALIZER_H
#define LLVM_LIB_TARGET_AMDGPU_AMDGPUMACHINELEGALIZER_H

#include "llvm/CodeGen/GlobalISel/LegalizerInfo.h"
#include "AMDGPUArgumentUsageInfo.h"
#include "SIInstrInfo.h"

namespace llvm {

class GCNTargetMachine;
class GCNSubtarget;
class MachineIRBuilder;

namespace AMDGPU {
struct ImageDimIntrinsicInfo;
}
class AMDGPULegalizerInfo final : public LegalizerInfo {
  const GCNSubtarget &ST;

public:
  AMDGPULegalizerInfo(const GCNSubtarget &ST,
                      const GCNTargetMachine &TM);

  bool legalizeCustom(LegalizerHelper &Helper, MachineInstr &MI,
                      LostDebugLocObserver &LocObserver) const override;

  Register getSegmentAperture(unsigned AddrSpace,
                              MachineRegisterInfo &MRI,
                              MachineIRBuilder &B) const;

  bool legalizeAddrSpaceCast(MachineInstr &MI, MachineRegisterInfo &MRI,
                             MachineIRBuilder &B) const;
  bool legalizeFroundeven(MachineInstr &MI, MachineRegisterInfo &MRI,
                          MachineIRBuilder &B) const;
  bool legalizeFceil(MachineInstr &MI, MachineRegisterInfo &MRI,
                     MachineIRBuilder &B) const;
  bool legalizeFrem(MachineInstr &MI, MachineRegisterInfo &MRI,
                    MachineIRBuilder &B) const;
  bool legalizeIntrinsicTrunc(MachineInstr &MI, MachineRegisterInfo &MRI,
                              MachineIRBuilder &B) const;
  bool legalizeITOFP(MachineInstr &MI, MachineRegisterInfo &MRI,
                     MachineIRBuilder &B, bool Signed) const;
  bool legalizeFPTOI(MachineInstr &MI, MachineRegisterInfo &MRI,
                     MachineIRBuilder &B, bool Signed) const;
  bool legalizeMinNumMaxNum(LegalizerHelper &Helper, MachineInstr &MI) const;
  bool legalizeExtractVectorElt(MachineInstr &MI, MachineRegisterInfo &MRI,
                                MachineIRBuilder &B) const;
  bool legalizeInsertVectorElt(MachineInstr &MI, MachineRegisterInfo &MRI,
                               MachineIRBuilder &B) const;

  bool legalizeSinCos(MachineInstr &MI, MachineRegisterInfo &MRI,
                      MachineIRBuilder &B) const;

  bool buildPCRelGlobalAddress(Register DstReg, LLT PtrTy, MachineIRBuilder &B,
                               const GlobalValue *GV, int64_t Offset,
                               unsigned GAFlags = SIInstrInfo::MO_NONE) const;

  void buildAbsGlobalAddress(Register DstReg, LLT PtrTy, MachineIRBuilder &B,
                             const GlobalValue *GV,
                             MachineRegisterInfo &MRI) const;

  bool legalizeGlobalValue(MachineInstr &MI, MachineRegisterInfo &MRI,
                           MachineIRBuilder &B) const;
  bool legalizeLoad(LegalizerHelper &Helper, MachineInstr &MI) const;
  bool legalizeStore(LegalizerHelper &Helper, MachineInstr &MI) const;

  bool legalizeFMad(MachineInstr &MI, MachineRegisterInfo &MRI,
                    MachineIRBuilder &B) const;

  bool legalizeAtomicCmpXChg(MachineInstr &MI, MachineRegisterInfo &MRI,
                             MachineIRBuilder &B) const;

  std::pair<Register, Register>
  getScaledLogInput(MachineIRBuilder &B, Register Src, unsigned Flags) const;

  bool legalizeFlog2(MachineInstr &MI, MachineIRBuilder &B) const;
  bool legalizeFlogCommon(MachineInstr &MI, MachineIRBuilder &B) const;
  bool legalizeFlogUnsafe(MachineIRBuilder &B, Register Dst, Register Src,
                          bool IsLog10, unsigned Flags) const;
  bool legalizeFExp2(MachineInstr &MI, MachineIRBuilder &B) const;
  bool legalizeFExpUnsafe(MachineIRBuilder &B, Register Dst, Register Src,
                          unsigned Flags) const;
  bool legalizeFExp(MachineInstr &MI, MachineIRBuilder &B) const;
  bool legalizeFPow(MachineInstr &MI, MachineIRBuilder &B) const;
  bool legalizeFFloor(MachineInstr &MI, MachineRegisterInfo &MRI,
                      MachineIRBuilder &B) const;

  bool legalizeBuildVector(MachineInstr &MI, MachineRegisterInfo &MRI,
                           MachineIRBuilder &B) const;

  void buildMultiply(LegalizerHelper &Helper, MutableArrayRef<Register> Accum,
                     ArrayRef<Register> Src0, ArrayRef<Register> Src1,
                     bool UsePartialMad64_32,
                     bool SeparateOddAlignedProducts) const;
  bool legalizeMul(LegalizerHelper &Helper, MachineInstr &MI) const;
  bool legalizeCTLZ_CTTZ(MachineInstr &MI, MachineRegisterInfo &MRI,
                         MachineIRBuilder &B) const;
  bool legalizeCTLZ_ZERO_UNDEF(MachineInstr &MI, MachineRegisterInfo &MRI,
                               MachineIRBuilder &B) const;

  void buildLoadInputValue(Register DstReg, MachineIRBuilder &B,
                           const ArgDescriptor *Arg,
                           const TargetRegisterClass *ArgRC, LLT ArgTy) const;
  bool loadInputValue(Register DstReg, MachineIRBuilder &B,
                      AMDGPUFunctionArgInfo::PreloadedValue ArgType) const;

  bool legalizePointerAsRsrcIntrin(MachineInstr &MI, MachineRegisterInfo &MRI,
                                   MachineIRBuilder &B) const;

  bool legalizePreloadedArgIntrin(
    MachineInstr &MI, MachineRegisterInfo &MRI, MachineIRBuilder &B,
    AMDGPUFunctionArgInfo::PreloadedValue ArgType) const;
  bool legalizeWorkitemIDIntrinsic(
      MachineInstr &MI, MachineRegisterInfo &MRI, MachineIRBuilder &B,
      unsigned Dim, AMDGPUFunctionArgInfo::PreloadedValue ArgType) const;

  Register getKernargParameterPtr(MachineIRBuilder &B, int64_t Offset) const;
  bool legalizeKernargMemParameter(MachineInstr &MI, MachineIRBuilder &B,
                                   uint64_t Offset,
                                   Align Alignment = Align(4)) const;

  bool legalizeUnsignedDIV_REM(MachineInstr &MI, MachineRegisterInfo &MRI,
                               MachineIRBuilder &B) const;

  void legalizeUnsignedDIV_REM32Impl(MachineIRBuilder &B, Register DstDivReg,
                                     Register DstRemReg, Register Num,
                                     Register Den) const;

  void legalizeUnsignedDIV_REM64Impl(MachineIRBuilder &B, Register DstDivReg,
                                     Register DstRemReg, Register Num,
                                     Register Den) const;

  bool legalizeSignedDIV_REM(MachineInstr &MI, MachineRegisterInfo &MRI,
                             MachineIRBuilder &B) const;

  bool legalizeFDIV(MachineInstr &MI, MachineRegisterInfo &MRI,
                    MachineIRBuilder &B) const;
  bool legalizeFDIV16(MachineInstr &MI, MachineRegisterInfo &MRI,
                      MachineIRBuilder &B) const;
  bool legalizeFDIV32(MachineInstr &MI, MachineRegisterInfo &MRI,
                      MachineIRBuilder &B) const;
  bool legalizeFDIV64(MachineInstr &MI, MachineRegisterInfo &MRI,
                      MachineIRBuilder &B) const;
  bool legalizeFFREXP(MachineInstr &MI, MachineRegisterInfo &MRI,
                      MachineIRBuilder &B) const;
  bool legalizeFastUnsafeFDIV(MachineInstr &MI, MachineRegisterInfo &MRI,
                              MachineIRBuilder &B) const;
  bool legalizeFastUnsafeFDIV64(MachineInstr &MI, MachineRegisterInfo &MRI,
                                MachineIRBuilder &B) const;
  bool legalizeFDIVFastIntrin(MachineInstr &MI, MachineRegisterInfo &MRI,
                              MachineIRBuilder &B) const;

  bool legalizeFSQRTF16(MachineInstr &MI, MachineRegisterInfo &MRI,
                        MachineIRBuilder &B) const;
  bool legalizeFSQRTF32(MachineInstr &MI, MachineRegisterInfo &MRI,
                        MachineIRBuilder &B) const;
  bool legalizeFSQRTF64(MachineInstr &MI, MachineRegisterInfo &MRI,
                        MachineIRBuilder &B) const;
  bool legalizeFSQRT(MachineInstr &MI, MachineRegisterInfo &MRI,
                     MachineIRBuilder &B) const;

  bool legalizeRsqClampIntrinsic(MachineInstr &MI, MachineRegisterInfo &MRI,
                                 MachineIRBuilder &B) const;

  bool getImplicitArgPtr(Register DstReg, MachineRegisterInfo &MRI,
                         MachineIRBuilder &B) const;

  bool legalizeImplicitArgPtr(MachineInstr &MI, MachineRegisterInfo &MRI,
                              MachineIRBuilder &B) const;

  bool getLDSKernelId(Register DstReg, MachineRegisterInfo &MRI,
                      MachineIRBuilder &B) const;

  bool legalizeLDSKernelId(MachineInstr &MI, MachineRegisterInfo &MRI,
                           MachineIRBuilder &B) const;

  bool legalizeIsAddrSpace(MachineInstr &MI, MachineRegisterInfo &MRI,
                           MachineIRBuilder &B, unsigned AddrSpace) const;

  std::pair<Register, unsigned> splitBufferOffsets(MachineIRBuilder &B,
                                                   Register OrigOffset) const;

  Register handleD16VData(MachineIRBuilder &B, MachineRegisterInfo &MRI,
                          Register Reg, bool ImageStore = false) const;
  Register fixStoreSourceType(MachineIRBuilder &B, Register VData, LLT MemTy,
                              bool IsFormat) const;

  bool legalizeBufferStore(MachineInstr &MI, LegalizerHelper &Helper,
                           bool IsTyped, bool IsFormat) const;
  bool legalizeBufferLoad(MachineInstr &MI, LegalizerHelper &Helper,
                          bool IsFormat, bool IsTyped) const;
  bool legalizeBufferAtomic(MachineInstr &MI, MachineIRBuilder &B,
                            Intrinsic::ID IID) const;

  bool legalizeBVHIntersectRayIntrinsic(MachineInstr &MI,
                                        MachineIRBuilder &B) const;

  bool legalizeBVHDualOrBVH8IntersectRayIntrinsic(MachineInstr &MI,
                                                  MachineIRBuilder &B) const;

  bool legalizeLaneOp(LegalizerHelper &Helper, MachineInstr &MI,
                      Intrinsic::ID IID) const;

  bool legalizeBVHIntrinsic(MachineInstr &MI, MachineIRBuilder &B) const;

  bool legalizeStackSave(MachineInstr &MI, MachineIRBuilder &B) const;
  bool legalizeWaveID(MachineInstr &MI, MachineIRBuilder &B) const;

  bool legalizeGetFPEnv(MachineInstr &MI, MachineRegisterInfo &MRI,
                        MachineIRBuilder &B) const;
  bool legalizeSetFPEnv(MachineInstr &MI, MachineRegisterInfo &MRI,
                        MachineIRBuilder &B) const;

  bool legalizeImageIntrinsic(
      MachineInstr &MI, MachineIRBuilder &B,
      GISelChangeObserver &Observer,
      const AMDGPU::ImageDimIntrinsicInfo *ImageDimIntr) const;

  bool legalizeSBufferLoad(LegalizerHelper &Helper, MachineInstr &MI) const;

  bool legalizeSBufferPrefetch(LegalizerHelper &Helper, MachineInstr &MI) const;

  bool legalizeTrap(MachineInstr &MI, MachineRegisterInfo &MRI,
                    MachineIRBuilder &B) const;
  bool legalizeTrapEndpgm(MachineInstr &MI, MachineRegisterInfo &MRI,
                          MachineIRBuilder &B) const;
  bool legalizeTrapHsaQueuePtr(MachineInstr &MI, MachineRegisterInfo &MRI,
                               MachineIRBuilder &B) const;
  bool legalizeTrapHsa(MachineInstr &MI, MachineRegisterInfo &MRI,
                       MachineIRBuilder &B) const;
  bool legalizeDebugTrap(MachineInstr &MI, MachineRegisterInfo &MRI,
                         MachineIRBuilder &B) const;

  bool legalizeIntrinsic(LegalizerHelper &Helper,
                         MachineInstr &MI) const override;
};
} // End llvm namespace.
#endif
