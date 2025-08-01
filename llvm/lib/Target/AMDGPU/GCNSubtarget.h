//=====-- GCNSubtarget.h - Define GCN Subtarget for AMDGPU ------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//==-----------------------------------------------------------------------===//
//
/// \file
/// AMD GCN specific subclass of TargetSubtarget.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_AMDGPU_GCNSUBTARGET_H
#define LLVM_LIB_TARGET_AMDGPU_GCNSUBTARGET_H

#include "AMDGPUCallLowering.h"
#include "AMDGPURegisterBankInfo.h"
#include "AMDGPUSubtarget.h"
#include "SIFrameLowering.h"
#include "SIISelLowering.h"
#include "SIInstrInfo.h"
#include "Utils/AMDGPUBaseInfo.h"
#include "llvm/Support/ErrorHandling.h"

#define GET_SUBTARGETINFO_HEADER
#include "AMDGPUGenSubtargetInfo.inc"

namespace llvm {

class GCNTargetMachine;

class GCNSubtarget final : public AMDGPUGenSubtargetInfo,
                           public AMDGPUSubtarget {
public:
  using AMDGPUSubtarget::getMaxWavesPerEU;

  // Following 2 enums are documented at:
  //   - https://llvm.org/docs/AMDGPUUsage.html#trap-handler-abi
  enum class TrapHandlerAbi {
    NONE   = 0x00,
    AMDHSA = 0x01,
  };

  enum class TrapID {
    LLVMAMDHSATrap      = 0x02,
    LLVMAMDHSADebugTrap = 0x03,
  };

private:
  /// SelectionDAGISel related APIs.
  std::unique_ptr<const SelectionDAGTargetInfo> TSInfo;

  /// GlobalISel related APIs.
  std::unique_ptr<AMDGPUCallLowering> CallLoweringInfo;
  std::unique_ptr<InlineAsmLowering> InlineAsmLoweringInfo;
  std::unique_ptr<InstructionSelector> InstSelector;
  std::unique_ptr<LegalizerInfo> Legalizer;
  std::unique_ptr<AMDGPURegisterBankInfo> RegBankInfo;

protected:
  // Basic subtarget description.
  Triple TargetTriple;
  AMDGPU::IsaInfo::AMDGPUTargetID TargetID;
  unsigned Gen = INVALID;
  InstrItineraryData InstrItins;
  int LDSBankCount = 0;
  unsigned MaxPrivateElementSize = 0;

  // Possibly statically set by tablegen, but may want to be overridden.
  bool FastDenormalF32 = false;
  bool HalfRate64Ops = false;
  bool FullRate64Ops = false;

  // Dynamically set bits that enable features.
  bool FlatForGlobal = false;
  bool AutoWaitcntBeforeBarrier = false;
  bool BackOffBarrier = false;
  bool UnalignedScratchAccess = false;
  bool UnalignedAccessMode = false;
  bool RelaxedBufferOOBMode = false;
  bool HasApertureRegs = false;
  bool SupportsXNACK = false;
  bool KernargPreload = false;

  // This should not be used directly. 'TargetID' tracks the dynamic settings
  // for XNACK.
  bool EnableXNACK = false;

  bool EnableTgSplit = false;
  bool EnableCuMode = false;
  bool TrapHandler = false;
  bool EnablePreciseMemory = false;

  // Used as options.
  bool EnableLoadStoreOpt = false;
  bool EnableUnsafeDSOffsetFolding = false;
  bool EnableSIScheduler = false;
  bool EnableDS128 = false;
  bool EnablePRTStrictNull = false;
  bool DumpCode = false;

  // Subtarget statically properties set by tablegen
  bool FP64 = false;
  bool FMA = false;
  bool MIMG_R128 = false;
  bool CIInsts = false;
  bool GFX8Insts = false;
  bool GFX9Insts = false;
  bool GFX90AInsts = false;
  bool GFX940Insts = false;
  bool GFX950Insts = false;
  bool GFX10Insts = false;
  bool GFX11Insts = false;
  bool GFX12Insts = false;
  bool GFX1250Insts = false;
  bool GFX10_3Insts = false;
  bool GFX7GFX8GFX9Insts = false;
  bool SGPRInitBug = false;
  bool UserSGPRInit16Bug = false;
  bool NegativeScratchOffsetBug = false;
  bool NegativeUnalignedScratchOffsetBug = false;
  bool HasSMemRealTime = false;
  bool HasIntClamp = false;
  bool HasFmaMixInsts = false;
  bool HasFmaMixBF16Insts = false;
  bool HasMovrel = false;
  bool HasVGPRIndexMode = false;
  bool HasScalarDwordx3Loads = false;
  bool HasScalarStores = false;
  bool HasScalarAtomics = false;
  bool HasSDWAOmod = false;
  bool HasSDWAScalar = false;
  bool HasSDWASdst = false;
  bool HasSDWAMac = false;
  bool HasSDWAOutModsVOPC = false;
  bool HasDPP = false;
  bool HasDPP8 = false;
  bool HasDPALU_DPP = false;
  bool HasDPPSrc1SGPR = false;
  bool HasPackedFP32Ops = false;
  bool HasImageInsts = false;
  bool HasExtendedImageInsts = false;
  bool HasR128A16 = false;
  bool HasA16 = false;
  bool HasG16 = false;
  bool HasNSAEncoding = false;
  bool HasPartialNSAEncoding = false;
  bool GFX10_AEncoding = false;
  bool GFX10_BEncoding = false;
  bool HasDLInsts = false;
  bool HasFmacF64Inst = false;
  bool HasDot1Insts = false;
  bool HasDot2Insts = false;
  bool HasDot3Insts = false;
  bool HasDot4Insts = false;
  bool HasDot5Insts = false;
  bool HasDot6Insts = false;
  bool HasDot7Insts = false;
  bool HasDot8Insts = false;
  bool HasDot9Insts = false;
  bool HasDot10Insts = false;
  bool HasDot11Insts = false;
  bool HasDot12Insts = false;
  bool HasDot13Insts = false;
  bool HasMAIInsts = false;
  bool HasFP8Insts = false;
  bool HasFP8ConversionInsts = false;
  bool HasFP8E5M3Insts = false;
  bool HasCvtFP8Vop1Bug = false;
  bool HasPkFmacF16Inst = false;
  bool HasAtomicFMinFMaxF32GlobalInsts = false;
  bool HasAtomicFMinFMaxF64GlobalInsts = false;
  bool HasAtomicFMinFMaxF32FlatInsts = false;
  bool HasAtomicFMinFMaxF64FlatInsts = false;
  bool HasAtomicDsPkAdd16Insts = false;
  bool HasAtomicFlatPkAdd16Insts = false;
  bool HasAtomicFaddRtnInsts = false;
  bool HasAtomicFaddNoRtnInsts = false;
  bool HasMemoryAtomicFaddF32DenormalSupport = false;
  bool HasAtomicBufferGlobalPkAddF16NoRtnInsts = false;
  bool HasAtomicBufferGlobalPkAddF16Insts = false;
  bool HasAtomicCSubNoRtnInsts = false;
  bool HasAtomicGlobalPkAddBF16Inst = false;
  bool HasAtomicBufferPkAddBF16Inst = false;
  bool HasFlatAtomicFaddF32Inst = false;
  bool HasFlatBufferGlobalAtomicFaddF64Inst = false;
  bool HasDefaultComponentZero = false;
  bool HasAgentScopeFineGrainedRemoteMemoryAtomics = false;
  bool HasDefaultComponentBroadcast = false;
  bool HasXF32Insts = false;
  /// The maximum number of instructions that may be placed within an S_CLAUSE,
  /// which is one greater than the maximum argument to S_CLAUSE. A value of 0
  /// indicates a lack of S_CLAUSE support.
  unsigned MaxHardClauseLength = 0;
  bool SupportsSRAMECC = false;
  bool DynamicVGPR = false;
  bool DynamicVGPRBlockSize32 = false;
  bool HasVMemToLDSLoad = false;

  // This should not be used directly. 'TargetID' tracks the dynamic settings
  // for SRAMECC.
  bool EnableSRAMECC = false;

  bool HasNoSdstCMPX = false;
  bool HasVscnt = false;
  bool HasWaitXcnt = false;
  bool HasGetWaveIdInst = false;
  bool HasSMemTimeInst = false;
  bool HasShaderCyclesRegister = false;
  bool HasShaderCyclesHiLoRegisters = false;
  bool HasVOP3Literal = false;
  bool HasNoDataDepHazard = false;
  bool FlatAddressSpace = false;
  bool FlatInstOffsets = false;
  bool FlatGlobalInsts = false;
  bool FlatScratchInsts = false;
  bool FlatGVSMode = false;
  bool ScalarFlatScratchInsts = false;
  bool HasArchitectedFlatScratch = false;
  bool EnableFlatScratch = false;
  bool HasArchitectedSGPRs = false;
  bool HasGDS = false;
  bool HasGWS = false;
  bool AddNoCarryInsts = false;
  bool HasUnpackedD16VMem = false;
  bool LDSMisalignedBug = false;
  bool HasMFMAInlineLiteralBug = false;
  bool UnalignedBufferAccess = false;
  bool UnalignedDSAccess = false;
  bool HasPackedTID = false;
  bool ScalarizeGlobal = false;
  bool HasSALUFloatInsts = false;
  bool HasPseudoScalarTrans = false;
  bool HasRestrictedSOffset = false;
  bool Has64BitLiterals = false;
  bool HasBitOp3Insts = false;
  bool HasTanhInsts = false;
  bool HasTransposeLoadF4F6Insts = false;
  bool HasPrngInst = false;
  bool HasBVHDualAndBVH8Insts = false;
  bool HasPermlane16Swap = false;
  bool HasPermlane32Swap = false;
  bool HasVcmpxPermlaneHazard = false;
  bool HasVMEMtoScalarWriteHazard = false;
  bool HasSMEMtoVectorWriteHazard = false;
  bool HasInstFwdPrefetchBug = false;
  bool HasVmemPrefInsts = false;
  bool HasSafeSmemPrefetch = false;
  bool HasSafeCUPrefetch = false;
  bool HasCUStores = false;
  bool HasVcmpxExecWARHazard = false;
  bool HasLdsBranchVmemWARHazard = false;
  bool HasNSAtoVMEMBug = false;
  bool HasNSAClauseBug = false;
  bool HasOffset3fBug = false;
  bool HasFlatSegmentOffsetBug = false;
  bool HasImageStoreD16Bug = false;
  bool HasImageGather4D16Bug = false;
  bool HasMSAALoadDstSelBug = false;
  bool HasPrivEnabledTrap2NopBug = false;
  bool Has1_5xVGPRs = false;
  bool HasMADIntraFwdBug = false;
  bool HasVOPDInsts = false;
  bool HasVALUTransUseHazard = false;
  bool HasRequiredExportPriority = false;
  bool HasVmemWriteVgprInOrder = false;
  bool HasAshrPkInsts = false;
  bool HasIEEEMinimumMaximumInsts = false;
  bool HasMinimum3Maximum3F32 = false;
  bool HasMinimum3Maximum3F16 = false;
  bool HasMin3Max3PKF16 = false;
  bool HasMinimum3Maximum3PKF16 = false;
  bool HasLshlAddU64Inst = false;
  bool HasAddSubU64Insts = false;
  bool HasMadU32Inst = false;
  bool HasPointSampleAccel = false;
  bool HasLdsBarrierArriveAtomic = false;
  bool HasSetPrioIncWgInst = false;

  bool RequiresCOV6 = false;
  bool UseBlockVGPROpsForCSR = false;

  // Dummy feature to use for assembler in tablegen.
  bool FeatureDisable = false;

private:
  SIInstrInfo InstrInfo;
  SITargetLowering TLInfo;
  SIFrameLowering FrameLowering;

public:
  GCNSubtarget(const Triple &TT, StringRef GPU, StringRef FS,
               const GCNTargetMachine &TM);
  ~GCNSubtarget() override;

  GCNSubtarget &initializeSubtargetDependencies(const Triple &TT,
                                                   StringRef GPU, StringRef FS);

  /// Diagnose inconsistent subtarget features before attempting to codegen
  /// function \p F.
  void checkSubtargetFeatures(const Function &F) const;

  const SIInstrInfo *getInstrInfo() const override {
    return &InstrInfo;
  }

  const SIFrameLowering *getFrameLowering() const override {
    return &FrameLowering;
  }

  const SITargetLowering *getTargetLowering() const override {
    return &TLInfo;
  }

  const SIRegisterInfo *getRegisterInfo() const override {
    return &InstrInfo.getRegisterInfo();
  }

  const SelectionDAGTargetInfo *getSelectionDAGInfo() const override;

  const CallLowering *getCallLowering() const override {
    return CallLoweringInfo.get();
  }

  const InlineAsmLowering *getInlineAsmLowering() const override {
    return InlineAsmLoweringInfo.get();
  }

  InstructionSelector *getInstructionSelector() const override {
    return InstSelector.get();
  }

  const LegalizerInfo *getLegalizerInfo() const override {
    return Legalizer.get();
  }

  const AMDGPURegisterBankInfo *getRegBankInfo() const override {
    return RegBankInfo.get();
  }

  const AMDGPU::IsaInfo::AMDGPUTargetID &getTargetID() const {
    return TargetID;
  }

  const InstrItineraryData *getInstrItineraryData() const override {
    return &InstrItins;
  }

  void ParseSubtargetFeatures(StringRef CPU, StringRef TuneCPU, StringRef FS);

  Generation getGeneration() const {
    return (Generation)Gen;
  }

  unsigned getMaxWaveScratchSize() const {
    // See COMPUTE_TMPRING_SIZE.WAVESIZE.
    if (getGeneration() >= GFX12) {
      // 18-bit field in units of 64-dword.
      return (64 * 4) * ((1 << 18) - 1);
    }
    if (getGeneration() == GFX11) {
      // 15-bit field in units of 64-dword.
      return (64 * 4) * ((1 << 15) - 1);
    }
    // 13-bit field in units of 256-dword.
    return (256 * 4) * ((1 << 13) - 1);
  }

  /// Return the number of high bits known to be zero for a frame index.
  unsigned getKnownHighZeroBitsForFrameIndex() const {
    return llvm::countl_zero(getMaxWaveScratchSize()) + getWavefrontSizeLog2();
  }

  int getLDSBankCount() const {
    return LDSBankCount;
  }

  unsigned getMaxPrivateElementSize(bool ForBufferRSrc = false) const {
    return (ForBufferRSrc || !enableFlatScratch()) ? MaxPrivateElementSize : 16;
  }

  unsigned getConstantBusLimit(unsigned Opcode) const;

  /// Returns if the result of this instruction with a 16-bit result returned in
  /// a 32-bit register implicitly zeroes the high 16-bits, rather than preserve
  /// the original value.
  bool zeroesHigh16BitsOfDest(unsigned Opcode) const;

  bool supportsWGP() const { return getGeneration() >= GFX10; }

  bool hasIntClamp() const {
    return HasIntClamp;
  }

  bool hasFP64() const {
    return FP64;
  }

  bool hasMIMG_R128() const {
    return MIMG_R128;
  }

  bool hasHWFP64() const {
    return FP64;
  }

  bool hasHalfRate64Ops() const {
    return HalfRate64Ops;
  }

  bool hasFullRate64Ops() const {
    return FullRate64Ops;
  }

  bool hasAddr64() const {
    return (getGeneration() < AMDGPUSubtarget::VOLCANIC_ISLANDS);
  }

  bool hasFlat() const {
    return (getGeneration() > AMDGPUSubtarget::SOUTHERN_ISLANDS);
  }

  // Return true if the target only has the reverse operand versions of VALU
  // shift instructions (e.g. v_lshrrev_b32, and no v_lshr_b32).
  bool hasOnlyRevVALUShifts() const {
    return getGeneration() >= VOLCANIC_ISLANDS;
  }

  bool hasFractBug() const {
    return getGeneration() == SOUTHERN_ISLANDS;
  }

  bool hasBFE() const {
    return true;
  }

  bool hasBFI() const {
    return true;
  }

  bool hasBFM() const {
    return hasBFE();
  }

  bool hasBCNT(unsigned Size) const {
    return true;
  }

  bool hasFFBL() const {
    return true;
  }

  bool hasFFBH() const {
    return true;
  }

  bool hasMed3_16() const {
    return getGeneration() >= AMDGPUSubtarget::GFX9;
  }

  bool hasMin3Max3_16() const {
    return getGeneration() >= AMDGPUSubtarget::GFX9;
  }

  bool hasFmaMixInsts() const {
    return HasFmaMixInsts;
  }

  bool hasFmaMixBF16Insts() const { return HasFmaMixBF16Insts; }

  bool hasCARRY() const {
    return true;
  }

  bool hasFMA() const {
    return FMA;
  }

  bool hasSwap() const {
    return GFX9Insts;
  }

  bool hasScalarPackInsts() const {
    return GFX9Insts;
  }

  bool hasScalarMulHiInsts() const {
    return GFX9Insts;
  }

  bool hasScalarSubwordLoads() const { return getGeneration() >= GFX12; }

  TrapHandlerAbi getTrapHandlerAbi() const {
    return isAmdHsaOS() ? TrapHandlerAbi::AMDHSA : TrapHandlerAbi::NONE;
  }

  bool supportsGetDoorbellID() const {
    // The S_GETREG DOORBELL_ID is supported by all GFX9 onward targets.
    return getGeneration() >= GFX9;
  }

  /// True if the offset field of DS instructions works as expected. On SI, the
  /// offset uses a 16-bit adder and does not always wrap properly.
  bool hasUsableDSOffset() const {
    return getGeneration() >= SEA_ISLANDS;
  }

  bool unsafeDSOffsetFoldingEnabled() const {
    return EnableUnsafeDSOffsetFolding;
  }

  /// Condition output from div_scale is usable.
  bool hasUsableDivScaleConditionOutput() const {
    return getGeneration() != SOUTHERN_ISLANDS;
  }

  /// Extra wait hazard is needed in some cases before
  /// s_cbranch_vccnz/s_cbranch_vccz.
  bool hasReadVCCZBug() const {
    return getGeneration() <= SEA_ISLANDS;
  }

  /// Writes to VCC_LO/VCC_HI update the VCCZ flag.
  bool partialVCCWritesUpdateVCCZ() const {
    return getGeneration() >= GFX10;
  }

  /// A read of an SGPR by SMRD instruction requires 4 wait states when the SGPR
  /// was written by a VALU instruction.
  bool hasSMRDReadVALUDefHazard() const {
    return getGeneration() == SOUTHERN_ISLANDS;
  }

  /// A read of an SGPR by a VMEM instruction requires 5 wait states when the
  /// SGPR was written by a VALU Instruction.
  bool hasVMEMReadSGPRVALUDefHazard() const {
    return getGeneration() >= VOLCANIC_ISLANDS;
  }

  bool hasRFEHazards() const {
    return getGeneration() >= VOLCANIC_ISLANDS;
  }

  /// Number of hazard wait states for s_setreg_b32/s_setreg_imm32_b32.
  unsigned getSetRegWaitStates() const {
    return getGeneration() <= SEA_ISLANDS ? 1 : 2;
  }

  bool dumpCode() const {
    return DumpCode;
  }

  /// Return the amount of LDS that can be used that will not restrict the
  /// occupancy lower than WaveCount.
  unsigned getMaxLocalMemSizeWithWaveCount(unsigned WaveCount,
                                           const Function &) const;

  bool supportsMinMaxDenormModes() const {
    return getGeneration() >= AMDGPUSubtarget::GFX9;
  }

  /// \returns If target supports S_DENORM_MODE.
  bool hasDenormModeInst() const {
    return getGeneration() >= AMDGPUSubtarget::GFX10;
  }

  bool useFlatForGlobal() const {
    return FlatForGlobal;
  }

  /// \returns If target supports ds_read/write_b128 and user enables generation
  /// of ds_read/write_b128.
  bool useDS128() const {
    return CIInsts && EnableDS128;
  }

  /// \return If target supports ds_read/write_b96/128.
  bool hasDS96AndDS128() const {
    return CIInsts;
  }

  /// Have v_trunc_f64, v_ceil_f64, v_rndne_f64
  bool haveRoundOpsF64() const {
    return CIInsts;
  }

  /// \returns If MUBUF instructions always perform range checking, even for
  /// buffer resources used for private memory access.
  bool privateMemoryResourceIsRangeChecked() const {
    return getGeneration() < AMDGPUSubtarget::GFX9;
  }

  /// \returns If target requires PRT Struct NULL support (zero result registers
  /// for sparse texture support).
  bool usePRTStrictNull() const {
    return EnablePRTStrictNull;
  }

  bool hasAutoWaitcntBeforeBarrier() const {
    return AutoWaitcntBeforeBarrier;
  }

  /// \returns true if the target supports backing off of s_barrier instructions
  /// when an exception is raised.
  bool supportsBackOffBarrier() const {
    return BackOffBarrier;
  }

  bool hasUnalignedBufferAccess() const {
    return UnalignedBufferAccess;
  }

  bool hasUnalignedBufferAccessEnabled() const {
    return UnalignedBufferAccess && UnalignedAccessMode;
  }

  bool hasUnalignedDSAccess() const {
    return UnalignedDSAccess;
  }

  bool hasUnalignedDSAccessEnabled() const {
    return UnalignedDSAccess && UnalignedAccessMode;
  }

  bool hasUnalignedScratchAccess() const {
    return UnalignedScratchAccess;
  }

  bool hasUnalignedScratchAccessEnabled() const {
    return UnalignedScratchAccess && UnalignedAccessMode;
  }

  bool hasUnalignedAccessMode() const {
    return UnalignedAccessMode;
  }

  bool hasRelaxedBufferOOBMode() const { return RelaxedBufferOOBMode; }

  bool hasApertureRegs() const {
    return HasApertureRegs;
  }

  bool isTrapHandlerEnabled() const {
    return TrapHandler;
  }

  bool isXNACKEnabled() const {
    return TargetID.isXnackOnOrAny();
  }

  bool isTgSplitEnabled() const {
    return EnableTgSplit;
  }

  bool isCuModeEnabled() const {
    return EnableCuMode;
  }

  bool isPreciseMemoryEnabled() const { return EnablePreciseMemory; }

  bool hasFlatAddressSpace() const {
    return FlatAddressSpace;
  }

  bool hasFlatScrRegister() const {
    return hasFlatAddressSpace();
  }

  bool hasFlatInstOffsets() const {
    return FlatInstOffsets;
  }

  bool hasFlatGlobalInsts() const {
    return FlatGlobalInsts;
  }

  bool hasFlatScratchInsts() const {
    return FlatScratchInsts;
  }

  // Check if target supports ST addressing mode with FLAT scratch instructions.
  // The ST addressing mode means no registers are used, either VGPR or SGPR,
  // but only immediate offset is swizzled and added to the FLAT scratch base.
  bool hasFlatScratchSTMode() const {
    return hasFlatScratchInsts() && (hasGFX10_3Insts() || hasGFX940Insts());
  }

  bool hasFlatScratchSVSMode() const { return GFX940Insts || GFX11Insts; }

  bool hasScalarFlatScratchInsts() const {
    return ScalarFlatScratchInsts;
  }

  bool enableFlatScratch() const {
    return flatScratchIsArchitected() ||
           (EnableFlatScratch && hasFlatScratchInsts());
  }

  bool hasGlobalAddTidInsts() const {
    return GFX10_BEncoding;
  }

  bool hasAtomicCSub() const {
    return GFX10_BEncoding;
  }

  bool hasMTBUFInsts() const { return !hasGFX1250Insts(); }

  bool hasFormattedMUBUFInsts() const { return !hasGFX1250Insts(); }

  bool hasExportInsts() const {
    return !hasGFX940Insts() && !hasGFX1250Insts();
  }

  bool hasVINTERPEncoding() const { return GFX11Insts && !hasGFX1250Insts(); }

  // DS_ADD_F64/DS_ADD_RTN_F64
  bool hasLdsAtomicAddF64() const {
    return hasGFX90AInsts() || hasGFX1250Insts();
  }

  bool hasMultiDwordFlatScratchAddressing() const {
    return getGeneration() >= GFX9;
  }

  bool hasFlatSegmentOffsetBug() const {
    return HasFlatSegmentOffsetBug;
  }

  bool hasFlatLgkmVMemCountInOrder() const {
    return getGeneration() > GFX9;
  }

  bool hasD16LoadStore() const {
    return getGeneration() >= GFX9;
  }

  bool d16PreservesUnusedBits() const {
    return hasD16LoadStore() && !TargetID.isSramEccOnOrAny();
  }

  bool hasD16Images() const {
    return getGeneration() >= VOLCANIC_ISLANDS;
  }

  /// Return if most LDS instructions have an m0 use that require m0 to be
  /// initialized.
  bool ldsRequiresM0Init() const {
    return getGeneration() < GFX9;
  }

  // True if the hardware rewinds and replays GWS operations if a wave is
  // preempted.
  //
  // If this is false, a GWS operation requires testing if a nack set the
  // MEM_VIOL bit, and repeating if so.
  bool hasGWSAutoReplay() const {
    return getGeneration() >= GFX9;
  }

  /// \returns if target has ds_gws_sema_release_all instruction.
  bool hasGWSSemaReleaseAll() const {
    return CIInsts;
  }

  /// \returns true if the target has integer add/sub instructions that do not
  /// produce a carry-out. This includes v_add_[iu]32, v_sub_[iu]32,
  /// v_add_[iu]16, and v_sub_[iu]16, all of which support the clamp modifier
  /// for saturation.
  bool hasAddNoCarry() const {
    return AddNoCarryInsts;
  }

  bool hasScalarAddSub64() const { return getGeneration() >= GFX12; }

  bool hasScalarSMulU64() const { return getGeneration() >= GFX12; }

  bool hasUnpackedD16VMem() const {
    return HasUnpackedD16VMem;
  }

  // Covers VS/PS/CS graphics shaders
  bool isMesaGfxShader(const Function &F) const {
    return isMesa3DOS() && AMDGPU::isShader(F.getCallingConv());
  }

  bool hasMad64_32() const {
    return getGeneration() >= SEA_ISLANDS;
  }

  bool hasSDWAOmod() const {
    return HasSDWAOmod;
  }

  bool hasSDWAScalar() const {
    return HasSDWAScalar;
  }

  bool hasSDWASdst() const {
    return HasSDWASdst;
  }

  bool hasSDWAMac() const {
    return HasSDWAMac;
  }

  bool hasSDWAOutModsVOPC() const {
    return HasSDWAOutModsVOPC;
  }

  bool hasDLInsts() const {
    return HasDLInsts;
  }

  bool hasFmacF64Inst() const { return HasFmacF64Inst; }

  bool hasDot1Insts() const {
    return HasDot1Insts;
  }

  bool hasDot2Insts() const {
    return HasDot2Insts;
  }

  bool hasDot3Insts() const {
    return HasDot3Insts;
  }

  bool hasDot4Insts() const {
    return HasDot4Insts;
  }

  bool hasDot5Insts() const {
    return HasDot5Insts;
  }

  bool hasDot6Insts() const {
    return HasDot6Insts;
  }

  bool hasDot7Insts() const {
    return HasDot7Insts;
  }

  bool hasDot8Insts() const {
    return HasDot8Insts;
  }

  bool hasDot9Insts() const {
    return HasDot9Insts;
  }

  bool hasDot10Insts() const {
    return HasDot10Insts;
  }

  bool hasDot11Insts() const {
    return HasDot11Insts;
  }

  bool hasDot12Insts() const {
    return HasDot12Insts;
  }

  bool hasDot13Insts() const {
    return HasDot13Insts;
  }

  bool hasMAIInsts() const {
    return HasMAIInsts;
  }

  bool hasFP8Insts() const {
    return HasFP8Insts;
  }

  bool hasFP8ConversionInsts() const { return HasFP8ConversionInsts; }

  bool hasFP8E5M3Insts() const { return HasFP8E5M3Insts; }

  bool hasPkFmacF16Inst() const {
    return HasPkFmacF16Inst;
  }

  bool hasAtomicFMinFMaxF32GlobalInsts() const {
    return HasAtomicFMinFMaxF32GlobalInsts;
  }

  bool hasAtomicFMinFMaxF64GlobalInsts() const {
    return HasAtomicFMinFMaxF64GlobalInsts;
  }

  bool hasAtomicFMinFMaxF32FlatInsts() const {
    return HasAtomicFMinFMaxF32FlatInsts;
  }

  bool hasAtomicFMinFMaxF64FlatInsts() const {
    return HasAtomicFMinFMaxF64FlatInsts;
  }

  bool hasAtomicDsPkAdd16Insts() const { return HasAtomicDsPkAdd16Insts; }

  bool hasAtomicFlatPkAdd16Insts() const { return HasAtomicFlatPkAdd16Insts; }

  bool hasAtomicFaddInsts() const {
    return HasAtomicFaddRtnInsts || HasAtomicFaddNoRtnInsts;
  }

  bool hasAtomicFaddRtnInsts() const { return HasAtomicFaddRtnInsts; }

  bool hasAtomicFaddNoRtnInsts() const { return HasAtomicFaddNoRtnInsts; }

  bool hasAtomicBufferGlobalPkAddF16NoRtnInsts() const {
    return HasAtomicBufferGlobalPkAddF16NoRtnInsts;
  }

  bool hasAtomicBufferGlobalPkAddF16Insts() const {
    return HasAtomicBufferGlobalPkAddF16Insts;
  }

  bool hasAtomicGlobalPkAddBF16Inst() const {
    return HasAtomicGlobalPkAddBF16Inst;
  }

  bool hasAtomicBufferPkAddBF16Inst() const {
    return HasAtomicBufferPkAddBF16Inst;
  }

  bool hasFlatAtomicFaddF32Inst() const { return HasFlatAtomicFaddF32Inst; }

  /// \return true if the target has flat, global, and buffer atomic fadd for
  /// double.
  bool hasFlatBufferGlobalAtomicFaddF64Inst() const {
    return HasFlatBufferGlobalAtomicFaddF64Inst;
  }

  /// \return true if the target's flat, global, and buffer atomic fadd for
  /// float supports denormal handling.
  bool hasMemoryAtomicFaddF32DenormalSupport() const {
    return HasMemoryAtomicFaddF32DenormalSupport;
  }

  /// \return true if atomic operations targeting fine-grained memory work
  /// correctly at device scope, in allocations in host or peer PCIe device
  /// memory.
  bool supportsAgentScopeFineGrainedRemoteMemoryAtomics() const {
    return HasAgentScopeFineGrainedRemoteMemoryAtomics;
  }

  bool hasDefaultComponentZero() const { return HasDefaultComponentZero; }

  bool hasDefaultComponentBroadcast() const {
    return HasDefaultComponentBroadcast;
  }

  bool hasNoSdstCMPX() const {
    return HasNoSdstCMPX;
  }

  bool hasVscnt() const {
    return HasVscnt;
  }

  bool hasGetWaveIdInst() const {
    return HasGetWaveIdInst;
  }

  bool hasSMemTimeInst() const {
    return HasSMemTimeInst;
  }

  bool hasShaderCyclesRegister() const {
    return HasShaderCyclesRegister;
  }

  bool hasShaderCyclesHiLoRegisters() const {
    return HasShaderCyclesHiLoRegisters;
  }

  bool hasVOP3Literal() const {
    return HasVOP3Literal;
  }

  bool hasNoDataDepHazard() const {
    return HasNoDataDepHazard;
  }

  bool vmemWriteNeedsExpWaitcnt() const {
    return getGeneration() < SEA_ISLANDS;
  }

  bool hasInstPrefetch() const {
    return getGeneration() == GFX10 || getGeneration() == GFX11;
  }

  bool hasPrefetch() const { return GFX12Insts; }

  bool hasVmemPrefInsts() const { return HasVmemPrefInsts; }

  bool hasSafeSmemPrefetch() const { return HasSafeSmemPrefetch; }

  bool hasSafeCUPrefetch() const { return HasSafeCUPrefetch; }

  bool hasCUStores() const { return HasCUStores; }

  // Has s_cmpk_* instructions.
  bool hasSCmpK() const { return getGeneration() < GFX12; }

  // Scratch is allocated in 256 dword per wave blocks for the entire
  // wavefront. When viewed from the perspective of an arbitrary workitem, this
  // is 4-byte aligned.
  //
  // Only 4-byte alignment is really needed to access anything. Transformations
  // on the pointer value itself may rely on the alignment / known low bits of
  // the pointer. Set this to something above the minimum to avoid needing
  // dynamic realignment in common cases.
  Align getStackAlignment() const { return Align(16); }

  bool enableMachineScheduler() const override {
    return true;
  }

  bool useAA() const override;

  bool enableSubRegLiveness() const override {
    return true;
  }

  void setScalarizeGlobalBehavior(bool b) { ScalarizeGlobal = b; }
  bool getScalarizeGlobalBehavior() const { return ScalarizeGlobal; }

  // static wrappers
  static bool hasHalfRate64Ops(const TargetSubtargetInfo &STI);

  // XXX - Why is this here if it isn't in the default pass set?
  bool enableEarlyIfConversion() const override {
    return true;
  }

  void overrideSchedPolicy(MachineSchedPolicy &Policy,
                           const SchedRegion &Region) const override;

  void mirFileLoaded(MachineFunction &MF) const override;

  unsigned getMaxNumUserSGPRs() const {
    return AMDGPU::getMaxNumUserSGPRs(*this);
  }

  bool hasSMemRealTime() const {
    return HasSMemRealTime;
  }

  bool hasMovrel() const {
    return HasMovrel;
  }

  bool hasVGPRIndexMode() const {
    return HasVGPRIndexMode;
  }

  bool useVGPRIndexMode() const;

  bool hasScalarCompareEq64() const {
    return getGeneration() >= VOLCANIC_ISLANDS;
  }

  bool hasScalarDwordx3Loads() const { return HasScalarDwordx3Loads; }

  bool hasScalarStores() const {
    return HasScalarStores;
  }

  bool hasScalarAtomics() const {
    return HasScalarAtomics;
  }

  bool hasLDSFPAtomicAddF32() const { return GFX8Insts; }
  bool hasLDSFPAtomicAddF64() const { return GFX90AInsts; }

  /// \returns true if the subtarget has the v_permlanex16_b32 instruction.
  bool hasPermLaneX16() const { return getGeneration() >= GFX10; }

  /// \returns true if the subtarget has the v_permlane64_b32 instruction.
  bool hasPermLane64() const { return getGeneration() >= GFX11; }

  bool hasDPP() const {
    return HasDPP;
  }

  bool hasDPPBroadcasts() const {
    return HasDPP && getGeneration() < GFX10;
  }

  bool hasDPPWavefrontShifts() const {
    return HasDPP && getGeneration() < GFX10;
  }

  bool hasDPP8() const {
    return HasDPP8;
  }

  bool hasDPALU_DPP() const {
    return HasDPALU_DPP;
  }

  bool hasDPPSrc1SGPR() const { return HasDPPSrc1SGPR; }

  bool hasPackedFP32Ops() const {
    return HasPackedFP32Ops;
  }

  // Has V_PK_MOV_B32 opcode
  bool hasPkMovB32() const {
    return GFX90AInsts;
  }

  bool hasFmaakFmamkF32Insts() const {
    return getGeneration() >= GFX10 || hasGFX940Insts();
  }

  bool hasFmaakFmamkF64Insts() const { return hasGFX1250Insts(); }

  bool hasImageInsts() const {
    return HasImageInsts;
  }

  bool hasExtendedImageInsts() const {
    return HasExtendedImageInsts;
  }

  bool hasR128A16() const {
    return HasR128A16;
  }

  bool hasA16() const { return HasA16; }

  bool hasG16() const { return HasG16; }

  bool hasOffset3fBug() const {
    return HasOffset3fBug;
  }

  bool hasImageStoreD16Bug() const { return HasImageStoreD16Bug; }

  bool hasImageGather4D16Bug() const { return HasImageGather4D16Bug; }

  bool hasMADIntraFwdBug() const { return HasMADIntraFwdBug; }

  bool hasMSAALoadDstSelBug() const { return HasMSAALoadDstSelBug; }

  bool hasPrivEnabledTrap2NopBug() const { return HasPrivEnabledTrap2NopBug; }

  bool hasNSAEncoding() const { return HasNSAEncoding; }

  bool hasNonNSAEncoding() const { return getGeneration() < GFX12; }

  bool hasPartialNSAEncoding() const { return HasPartialNSAEncoding; }

  unsigned getNSAMaxSize(bool HasSampler = false) const {
    return AMDGPU::getNSAMaxSize(*this, HasSampler);
  }

  bool hasGFX10_AEncoding() const {
    return GFX10_AEncoding;
  }

  bool hasGFX10_BEncoding() const {
    return GFX10_BEncoding;
  }

  bool hasGFX10_3Insts() const {
    return GFX10_3Insts;
  }

  bool hasMadF16() const;

  bool hasMovB64() const { return GFX940Insts || GFX1250Insts; }

  bool hasLshlAddU64Inst() const { return HasLshlAddU64Inst; }

  // Scalar and global loads support scale_offset bit.
  bool hasScaleOffset() const { return GFX1250Insts; }

  bool hasFlatGVSMode() const { return FlatGVSMode; }

  // FLAT GLOBAL VOffset is signed
  bool hasSignedGVSOffset() const { return GFX1250Insts; }

  bool enableSIScheduler() const {
    return EnableSIScheduler;
  }

  bool loadStoreOptEnabled() const {
    return EnableLoadStoreOpt;
  }

  bool hasSGPRInitBug() const {
    return SGPRInitBug;
  }

  bool hasUserSGPRInit16Bug() const {
    return UserSGPRInit16Bug && isWave32();
  }

  bool hasNegativeScratchOffsetBug() const { return NegativeScratchOffsetBug; }

  bool hasNegativeUnalignedScratchOffsetBug() const {
    return NegativeUnalignedScratchOffsetBug;
  }

  bool hasMFMAInlineLiteralBug() const {
    return HasMFMAInlineLiteralBug;
  }

  bool has12DWordStoreHazard() const {
    return getGeneration() != AMDGPUSubtarget::SOUTHERN_ISLANDS;
  }

  // \returns true if the subtarget supports DWORDX3 load/store instructions.
  bool hasDwordx3LoadStores() const {
    return CIInsts;
  }

  bool hasReadM0MovRelInterpHazard() const {
    return getGeneration() == AMDGPUSubtarget::GFX9;
  }

  bool hasReadM0SendMsgHazard() const {
    return getGeneration() >= AMDGPUSubtarget::VOLCANIC_ISLANDS &&
           getGeneration() <= AMDGPUSubtarget::GFX9;
  }

  bool hasReadM0LdsDmaHazard() const {
    return getGeneration() == AMDGPUSubtarget::GFX9;
  }

  bool hasReadM0LdsDirectHazard() const {
    return getGeneration() == AMDGPUSubtarget::GFX9;
  }

  bool hasVcmpxPermlaneHazard() const {
    return HasVcmpxPermlaneHazard;
  }

  bool hasVMEMtoScalarWriteHazard() const {
    return HasVMEMtoScalarWriteHazard;
  }

  bool hasSMEMtoVectorWriteHazard() const {
    return HasSMEMtoVectorWriteHazard;
  }

  bool hasLDSMisalignedBug() const {
    return LDSMisalignedBug && !EnableCuMode;
  }

  bool hasInstFwdPrefetchBug() const {
    return HasInstFwdPrefetchBug;
  }

  bool hasVcmpxExecWARHazard() const {
    return HasVcmpxExecWARHazard;
  }

  bool hasLdsBranchVmemWARHazard() const {
    return HasLdsBranchVmemWARHazard;
  }

  // Shift amount of a 64 bit shift cannot be a highest allocated register
  // if also at the end of the allocation block.
  bool hasShift64HighRegBug() const {
    return GFX90AInsts && !GFX940Insts;
  }

  // Has one cycle hazard on transcendental instruction feeding a
  // non transcendental VALU.
  bool hasTransForwardingHazard() const { return GFX940Insts; }

  // Has one cycle hazard on a VALU instruction partially writing dst with
  // a shift of result bits feeding another VALU instruction.
  bool hasDstSelForwardingHazard() const { return GFX940Insts; }

  // Cannot use op_sel with v_dot instructions.
  bool hasDOTOpSelHazard() const { return GFX940Insts || GFX11Insts; }

  // Does not have HW interlocs for VALU writing and then reading SGPRs.
  bool hasVDecCoExecHazard() const {
    return GFX940Insts;
  }

  bool hasNSAtoVMEMBug() const {
    return HasNSAtoVMEMBug;
  }

  bool hasNSAClauseBug() const { return HasNSAClauseBug; }

  bool hasHardClauses() const { return MaxHardClauseLength > 0; }

  bool hasGFX90AInsts() const { return GFX90AInsts; }

  bool hasFPAtomicToDenormModeHazard() const {
    return getGeneration() == GFX10;
  }

  bool hasVOP3DPP() const { return getGeneration() >= GFX11; }

  bool hasLdsDirect() const { return getGeneration() >= GFX11; }

  bool hasLdsWaitVMSRC() const { return getGeneration() >= GFX12; }

  bool hasVALUPartialForwardingHazard() const {
    return getGeneration() == GFX11;
  }

  bool hasVALUTransUseHazard() const { return HasVALUTransUseHazard; }

  bool hasCvtScaleForwardingHazard() const { return GFX950Insts; }

  bool requiresCodeObjectV6() const { return RequiresCOV6; }

  bool useVGPRBlockOpsForCSR() const { return UseBlockVGPROpsForCSR; }

  bool hasVALUMaskWriteHazard() const { return getGeneration() == GFX11; }

  bool hasVALUReadSGPRHazard() const { return GFX12Insts && !GFX1250Insts; }

  /// Return if operations acting on VGPR tuples require even alignment.
  bool needsAlignedVGPRs() const { return GFX90AInsts || GFX1250Insts; }

  /// Return true if the target has the S_PACK_HL_B32_B16 instruction.
  bool hasSPackHL() const { return GFX11Insts; }

  /// Return true if the target's EXP instruction has the COMPR flag, which
  /// affects the meaning of the EN (enable) bits.
  bool hasCompressedExport() const { return !GFX11Insts; }

  /// Return true if the target's EXP instruction supports the NULL export
  /// target.
  bool hasNullExportTarget() const { return !GFX11Insts; }

  bool has1_5xVGPRs() const { return Has1_5xVGPRs; }

  bool hasVOPDInsts() const { return HasVOPDInsts; }

  bool hasFlatScratchSVSSwizzleBug() const { return getGeneration() == GFX11; }

  /// Return true if the target has the S_DELAY_ALU instruction.
  bool hasDelayAlu() const { return GFX11Insts; }

  bool hasPackedTID() const { return HasPackedTID; }

  // GFX94* is a derivation to GFX90A. hasGFX940Insts() being true implies that
  // hasGFX90AInsts is also true.
  bool hasGFX940Insts() const { return GFX940Insts; }

  // GFX950 is a derivation to GFX94*. hasGFX950Insts() implies that
  // hasGFX940Insts and hasGFX90AInsts are also true.
  bool hasGFX950Insts() const { return GFX950Insts; }

  /// Returns true if the target supports
  /// global_load_lds_dwordx3/global_load_lds_dwordx4 or
  /// buffer_load_dwordx3/buffer_load_dwordx4 with the lds bit.
  bool hasLDSLoadB96_B128() const {
    return hasGFX950Insts();
  }

  bool hasVMemToLDSLoad() const { return HasVMemToLDSLoad; }

  bool hasSALUFloatInsts() const { return HasSALUFloatInsts; }

  bool hasPseudoScalarTrans() const { return HasPseudoScalarTrans; }

  bool hasRestrictedSOffset() const { return HasRestrictedSOffset; }

  bool hasRequiredExportPriority() const { return HasRequiredExportPriority; }

  bool hasVmemWriteVgprInOrder() const { return HasVmemWriteVgprInOrder; }

  /// \returns true if the target uses LOADcnt/SAMPLEcnt/BVHcnt, DScnt/KMcnt
  /// and STOREcnt rather than VMcnt, LGKMcnt and VScnt respectively.
  bool hasExtendedWaitCounts() const { return getGeneration() >= GFX12; }

  /// \returns true if inline constants are not supported for F16 pseudo
  /// scalar transcendentals.
  bool hasNoF16PseudoScalarTransInlineConstants() const {
    return getGeneration() == GFX12;
  }

  /// \returns true if the target has instructions with xf32 format support.
  bool hasXF32Insts() const { return HasXF32Insts; }

  bool hasBitOp3Insts() const { return HasBitOp3Insts; }

  bool hasPermlane16Swap() const { return HasPermlane16Swap; }
  bool hasPermlane32Swap() const { return HasPermlane32Swap; }
  bool hasAshrPkInsts() const { return HasAshrPkInsts; }

  bool hasMinimum3Maximum3F32() const {
    return HasMinimum3Maximum3F32;
  }

  bool hasMinimum3Maximum3F16() const {
    return HasMinimum3Maximum3F16;
  }

  bool hasMin3Max3PKF16() const { return HasMin3Max3PKF16; }

  bool hasTanhInsts() const { return HasTanhInsts; }

  bool hasAddPC64Inst() const { return GFX1250Insts; }

  bool hasMinimum3Maximum3PKF16() const {
    return HasMinimum3Maximum3PKF16;
  }

  bool hasTransposeLoadF4F6Insts() const { return HasTransposeLoadF4F6Insts; }

  /// \returns true if the target has s_wait_xcnt insertion. Supported for
  /// GFX1250.
  bool hasWaitXCnt() const { return HasWaitXcnt; }

  // A single DWORD instructions can use a 64-bit literal.
  bool has64BitLiterals() const { return Has64BitLiterals; }

  bool hasPointSampleAccel() const { return HasPointSampleAccel; }

  bool hasLdsBarrierArriveAtomic() const { return HasLdsBarrierArriveAtomic; }

  /// \returns The maximum number of instructions that can be enclosed in an
  /// S_CLAUSE on the given subtarget, or 0 for targets that do not support that
  /// instruction.
  unsigned maxHardClauseLength() const { return MaxHardClauseLength; }

  bool hasPrngInst() const { return HasPrngInst; }

  bool hasBVHDualAndBVH8Insts() const { return HasBVHDualAndBVH8Insts; }

  /// Return the maximum number of waves per SIMD for kernels using \p SGPRs
  /// SGPRs
  unsigned getOccupancyWithNumSGPRs(unsigned SGPRs) const;

  /// Return the maximum number of waves per SIMD for kernels using \p VGPRs
  /// VGPRs
  unsigned getOccupancyWithNumVGPRs(unsigned VGPRs,
                                    unsigned DynamicVGPRBlockSize) const;

  /// Subtarget's minimum/maximum occupancy, in number of waves per EU, that can
  /// be achieved when the only function running on a CU is \p F, each workgroup
  /// uses \p LDSSize bytes of LDS, and each wave uses \p NumSGPRs SGPRs and \p
  /// NumVGPRs VGPRs. The flat workgroup sizes associated to the function are a
  /// range, so this returns a range as well.
  ///
  /// Note that occupancy can be affected by the scratch allocation as well, but
  /// we do not have enough information to compute it.
  std::pair<unsigned, unsigned> computeOccupancy(const Function &F,
                                                 unsigned LDSSize = 0,
                                                 unsigned NumSGPRs = 0,
                                                 unsigned NumVGPRs = 0) const;

  /// \returns true if the flat_scratch register should be initialized with the
  /// pointer to the wave's scratch memory rather than a size and offset.
  bool flatScratchIsPointer() const {
    return getGeneration() >= AMDGPUSubtarget::GFX9;
  }

  /// \returns true if the flat_scratch register is initialized by the HW.
  /// In this case it is readonly.
  bool flatScratchIsArchitected() const { return HasArchitectedFlatScratch; }

  /// \returns true if the architected SGPRs are enabled.
  bool hasArchitectedSGPRs() const { return HasArchitectedSGPRs; }

  /// \returns true if Global Data Share is supported.
  bool hasGDS() const { return HasGDS; }

  /// \returns true if Global Wave Sync is supported.
  bool hasGWS() const { return HasGWS; }

  /// \returns true if the machine has merged shaders in which s0-s7 are
  /// reserved by the hardware and user SGPRs start at s8
  bool hasMergedShaders() const {
    return getGeneration() >= GFX9;
  }

  // \returns true if the target supports the pre-NGG legacy geometry path.
  bool hasLegacyGeometry() const { return getGeneration() < GFX11; }

  // \returns true if preloading kernel arguments is supported.
  bool hasKernargPreload() const { return KernargPreload; }

  // \returns true if the target has split barriers feature
  bool hasSplitBarriers() const { return getGeneration() >= GFX12; }

  // \returns true if FP8/BF8 VOP1 form of conversion to F32 is unreliable.
  bool hasCvtFP8VOP1Bug() const { return HasCvtFP8Vop1Bug; }

  // \returns true if CSUB (a.k.a. SUB_CLAMP on GFX12) atomics support a
  // no-return form.
  bool hasAtomicCSubNoRtnInsts() const { return HasAtomicCSubNoRtnInsts; }

  // \returns true if the target has DX10_CLAMP kernel descriptor mode bit
  bool hasDX10ClampMode() const { return getGeneration() < GFX12; }

  // \returns true if the target has IEEE kernel descriptor mode bit
  bool hasIEEEMode() const { return getGeneration() < GFX12; }

  // \returns true if the target has IEEE fminimum/fmaximum instructions
  bool hasIEEEMinimumMaximumInsts() const { return HasIEEEMinimumMaximumInsts; }

  // \returns true if the target has WG_RR_MODE kernel descriptor mode bit
  bool hasRrWGMode() const { return getGeneration() >= GFX12; }

  /// \returns true if VADDR and SADDR fields in VSCRATCH can use negative
  /// values.
  bool hasSignedScratchOffsets() const { return getGeneration() >= GFX12; }

  bool hasGFX1250Insts() const { return GFX1250Insts; }

  bool hasVOPD3() const { return GFX1250Insts; }

  // \returns true if the target has V_ADD_U64/V_SUB_U64 instructions.
  bool hasAddSubU64Insts() const { return HasAddSubU64Insts; }

  // \returns true if the target has V_MAD_U32 instruction.
  bool hasMadU32Inst() const { return HasMadU32Inst; }

  // \returns true if the target has V_MUL_U64/V_MUL_I64 instructions.
  bool hasVectorMulU64() const { return GFX1250Insts; }

  // \returns true if the target has V_MAD_NC_U64_U32/V_MAD_NC_I64_I32
  // instructions.
  bool hasMadU64U32NoCarry() const { return GFX1250Insts; }

  // \returns true if the target has V_PK_ADD_{MIN|MAX}_{I|U}16 instructions.
  bool hasPkAddMinMaxInsts() const { return GFX1250Insts; }

  // \returns true if the target has V_PK_{MIN|MAX}3_{I|U}16 instructions.
  bool hasPkMinMax3Insts() const { return GFX1250Insts; }

  // \returns true if target has S_SETPRIO_INC_WG instruction.
  bool hasSetPrioIncWgInst() const { return HasSetPrioIncWgInst; }

  // \returns true if S_GETPC_B64 zero-extends the result from 48 bits instead
  // of sign-extending.
  bool hasGetPCZeroExtension() const { return GFX12Insts; }

  /// \returns SGPR allocation granularity supported by the subtarget.
  unsigned getSGPRAllocGranule() const {
    return AMDGPU::IsaInfo::getSGPRAllocGranule(this);
  }

  /// \returns SGPR encoding granularity supported by the subtarget.
  unsigned getSGPREncodingGranule() const {
    return AMDGPU::IsaInfo::getSGPREncodingGranule(this);
  }

  /// \returns Total number of SGPRs supported by the subtarget.
  unsigned getTotalNumSGPRs() const {
    return AMDGPU::IsaInfo::getTotalNumSGPRs(this);
  }

  /// \returns Addressable number of SGPRs supported by the subtarget.
  unsigned getAddressableNumSGPRs() const {
    return AMDGPU::IsaInfo::getAddressableNumSGPRs(this);
  }

  /// \returns Minimum number of SGPRs that meets the given number of waves per
  /// execution unit requirement supported by the subtarget.
  unsigned getMinNumSGPRs(unsigned WavesPerEU) const {
    return AMDGPU::IsaInfo::getMinNumSGPRs(this, WavesPerEU);
  }

  /// \returns Maximum number of SGPRs that meets the given number of waves per
  /// execution unit requirement supported by the subtarget.
  unsigned getMaxNumSGPRs(unsigned WavesPerEU, bool Addressable) const {
    return AMDGPU::IsaInfo::getMaxNumSGPRs(this, WavesPerEU, Addressable);
  }

  /// \returns Reserved number of SGPRs. This is common
  /// utility function called by MachineFunction and
  /// Function variants of getReservedNumSGPRs.
  unsigned getBaseReservedNumSGPRs(const bool HasFlatScratch) const;
  /// \returns Reserved number of SGPRs for given machine function \p MF.
  unsigned getReservedNumSGPRs(const MachineFunction &MF) const;

  /// \returns Reserved number of SGPRs for given function \p F.
  unsigned getReservedNumSGPRs(const Function &F) const;

  /// \returns Maximum number of preloaded SGPRs for the subtarget.
  unsigned getMaxNumPreloadedSGPRs() const;

  /// \returns max num SGPRs. This is the common utility
  /// function called by MachineFunction and Function
  /// variants of getMaxNumSGPRs.
  unsigned getBaseMaxNumSGPRs(const Function &F,
                              std::pair<unsigned, unsigned> WavesPerEU,
                              unsigned PreloadedSGPRs,
                              unsigned ReservedNumSGPRs) const;

  /// \returns Maximum number of SGPRs that meets number of waves per execution
  /// unit requirement for function \p MF, or number of SGPRs explicitly
  /// requested using "amdgpu-num-sgpr" attribute attached to function \p MF.
  ///
  /// \returns Value that meets number of waves per execution unit requirement
  /// if explicitly requested value cannot be converted to integer, violates
  /// subtarget's specifications, or does not meet number of waves per execution
  /// unit requirement.
  unsigned getMaxNumSGPRs(const MachineFunction &MF) const;

  /// \returns Maximum number of SGPRs that meets number of waves per execution
  /// unit requirement for function \p F, or number of SGPRs explicitly
  /// requested using "amdgpu-num-sgpr" attribute attached to function \p F.
  ///
  /// \returns Value that meets number of waves per execution unit requirement
  /// if explicitly requested value cannot be converted to integer, violates
  /// subtarget's specifications, or does not meet number of waves per execution
  /// unit requirement.
  unsigned getMaxNumSGPRs(const Function &F) const;

  /// \returns VGPR allocation granularity supported by the subtarget.
  unsigned getVGPRAllocGranule(unsigned DynamicVGPRBlockSize) const {
    return AMDGPU::IsaInfo::getVGPRAllocGranule(this, DynamicVGPRBlockSize);
  }

  /// \returns VGPR encoding granularity supported by the subtarget.
  unsigned getVGPREncodingGranule() const {
    return AMDGPU::IsaInfo::getVGPREncodingGranule(this);
  }

  /// \returns Total number of VGPRs supported by the subtarget.
  unsigned getTotalNumVGPRs() const {
    return AMDGPU::IsaInfo::getTotalNumVGPRs(this);
  }

  /// \returns Addressable number of architectural VGPRs supported by the
  /// subtarget.
  unsigned getAddressableNumArchVGPRs() const {
    return AMDGPU::IsaInfo::getAddressableNumArchVGPRs(this);
  }

  /// \returns Addressable number of VGPRs supported by the subtarget.
  unsigned getAddressableNumVGPRs(unsigned DynamicVGPRBlockSize) const {
    return AMDGPU::IsaInfo::getAddressableNumVGPRs(this, DynamicVGPRBlockSize);
  }

  /// \returns the minimum number of VGPRs that will prevent achieving more than
  /// the specified number of waves \p WavesPerEU.
  unsigned getMinNumVGPRs(unsigned WavesPerEU,
                          unsigned DynamicVGPRBlockSize) const {
    return AMDGPU::IsaInfo::getMinNumVGPRs(this, WavesPerEU,
                                           DynamicVGPRBlockSize);
  }

  /// \returns the maximum number of VGPRs that can be used and still achieved
  /// at least the specified number of waves \p WavesPerEU.
  unsigned getMaxNumVGPRs(unsigned WavesPerEU,
                          unsigned DynamicVGPRBlockSize) const {
    return AMDGPU::IsaInfo::getMaxNumVGPRs(this, WavesPerEU,
                                           DynamicVGPRBlockSize);
  }

  /// \returns max num VGPRs. This is the common utility function
  /// called by MachineFunction and Function variants of getMaxNumVGPRs.
  unsigned
  getBaseMaxNumVGPRs(const Function &F,
                     std::pair<unsigned, unsigned> NumVGPRBounds) const;

  /// \returns Maximum number of VGPRs that meets number of waves per execution
  /// unit requirement for function \p F, or number of VGPRs explicitly
  /// requested using "amdgpu-num-vgpr" attribute attached to function \p F.
  ///
  /// \returns Value that meets number of waves per execution unit requirement
  /// if explicitly requested value cannot be converted to integer, violates
  /// subtarget's specifications, or does not meet number of waves per execution
  /// unit requirement.
  unsigned getMaxNumVGPRs(const Function &F) const;

  unsigned getMaxNumAGPRs(const Function &F) const {
    return getMaxNumVGPRs(F);
  }

  /// Return a pair of maximum numbers of VGPRs and AGPRs that meet the number
  /// of waves per execution unit required for the function \p MF.
  std::pair<unsigned, unsigned> getMaxNumVectorRegs(const Function &F) const;

  /// \returns Maximum number of VGPRs that meets number of waves per execution
  /// unit requirement for function \p MF, or number of VGPRs explicitly
  /// requested using "amdgpu-num-vgpr" attribute attached to function \p MF.
  ///
  /// \returns Value that meets number of waves per execution unit requirement
  /// if explicitly requested value cannot be converted to integer, violates
  /// subtarget's specifications, or does not meet number of waves per execution
  /// unit requirement.
  unsigned getMaxNumVGPRs(const MachineFunction &MF) const;

  bool isWave32() const {
    return getWavefrontSize() == 32;
  }

  bool isWave64() const {
    return getWavefrontSize() == 64;
  }

  /// Returns if the wavesize of this subtarget is known reliable. This is false
  /// only for the a default target-cpu that does not have an explicit
  /// +wavefrontsize target feature.
  bool isWaveSizeKnown() const {
    return hasFeature(AMDGPU::FeatureWavefrontSize32) ||
           hasFeature(AMDGPU::FeatureWavefrontSize64);
  }

  const TargetRegisterClass *getBoolRC() const {
    return getRegisterInfo()->getBoolRC();
  }

  /// \returns Maximum number of work groups per compute unit supported by the
  /// subtarget and limited by given \p FlatWorkGroupSize.
  unsigned getMaxWorkGroupsPerCU(unsigned FlatWorkGroupSize) const override {
    return AMDGPU::IsaInfo::getMaxWorkGroupsPerCU(this, FlatWorkGroupSize);
  }

  /// \returns Minimum flat work group size supported by the subtarget.
  unsigned getMinFlatWorkGroupSize() const override {
    return AMDGPU::IsaInfo::getMinFlatWorkGroupSize(this);
  }

  /// \returns Maximum flat work group size supported by the subtarget.
  unsigned getMaxFlatWorkGroupSize() const override {
    return AMDGPU::IsaInfo::getMaxFlatWorkGroupSize(this);
  }

  /// \returns Number of waves per execution unit required to support the given
  /// \p FlatWorkGroupSize.
  unsigned
  getWavesPerEUForWorkGroup(unsigned FlatWorkGroupSize) const override {
    return AMDGPU::IsaInfo::getWavesPerEUForWorkGroup(this, FlatWorkGroupSize);
  }

  /// \returns Minimum number of waves per execution unit supported by the
  /// subtarget.
  unsigned getMinWavesPerEU() const override {
    return AMDGPU::IsaInfo::getMinWavesPerEU(this);
  }

  void adjustSchedDependency(SUnit *Def, int DefOpIdx, SUnit *Use, int UseOpIdx,
                             SDep &Dep,
                             const TargetSchedModel *SchedModel) const override;

  // \returns true if it's beneficial on this subtarget for the scheduler to
  // cluster stores as well as loads.
  bool shouldClusterStores() const { return getGeneration() >= GFX11; }

  // \returns the number of address arguments from which to enable MIMG NSA
  // on supported architectures.
  unsigned getNSAThreshold(const MachineFunction &MF) const;

  // \returns true if the subtarget has a hazard requiring an "s_nop 0"
  // instruction before "s_sendmsg sendmsg(MSG_DEALLOC_VGPRS)".
  bool requiresNopBeforeDeallocVGPRs() const {
    // Currently all targets that support the dealloc VGPRs message also require
    // the nop.
    return true;
  }

  bool isDynamicVGPREnabled() const { return DynamicVGPR; }
  unsigned getDynamicVGPRBlockSize() const {
    return DynamicVGPRBlockSize32 ? 32 : 16;
  }

  bool requiresDisjointEarlyClobberAndUndef() const override {
    // AMDGPU doesn't care if early-clobber and undef operands are allocated
    // to the same register.
    return false;
  }
};

class GCNUserSGPRUsageInfo {
public:
  bool hasImplicitBufferPtr() const { return ImplicitBufferPtr; }

  bool hasPrivateSegmentBuffer() const { return PrivateSegmentBuffer; }

  bool hasDispatchPtr() const { return DispatchPtr; }

  bool hasQueuePtr() const { return QueuePtr; }

  bool hasKernargSegmentPtr() const { return KernargSegmentPtr; }

  bool hasDispatchID() const { return DispatchID; }

  bool hasFlatScratchInit() const { return FlatScratchInit; }

  bool hasPrivateSegmentSize() const { return PrivateSegmentSize; }

  unsigned getNumKernargPreloadSGPRs() const { return NumKernargPreloadSGPRs; }

  unsigned getNumUsedUserSGPRs() const { return NumUsedUserSGPRs; }

  unsigned getNumFreeUserSGPRs();

  void allocKernargPreloadSGPRs(unsigned NumSGPRs);

  enum UserSGPRID : unsigned {
    ImplicitBufferPtrID = 0,
    PrivateSegmentBufferID = 1,
    DispatchPtrID = 2,
    QueuePtrID = 3,
    KernargSegmentPtrID = 4,
    DispatchIdID = 5,
    FlatScratchInitID = 6,
    PrivateSegmentSizeID = 7
  };

  // Returns the size in number of SGPRs for preload user SGPR field.
  static unsigned getNumUserSGPRForField(UserSGPRID ID) {
    switch (ID) {
    case ImplicitBufferPtrID:
      return 2;
    case PrivateSegmentBufferID:
      return 4;
    case DispatchPtrID:
      return 2;
    case QueuePtrID:
      return 2;
    case KernargSegmentPtrID:
      return 2;
    case DispatchIdID:
      return 2;
    case FlatScratchInitID:
      return 2;
    case PrivateSegmentSizeID:
      return 1;
    }
    llvm_unreachable("Unknown UserSGPRID.");
  }

  GCNUserSGPRUsageInfo(const Function &F, const GCNSubtarget &ST);

private:
  const GCNSubtarget &ST;

  // Private memory buffer
  // Compute directly in sgpr[0:1]
  // Other shaders indirect 64-bits at sgpr[0:1]
  bool ImplicitBufferPtr = false;

  bool PrivateSegmentBuffer = false;

  bool DispatchPtr = false;

  bool QueuePtr = false;

  bool KernargSegmentPtr = false;

  bool DispatchID = false;

  bool FlatScratchInit = false;

  bool PrivateSegmentSize = false;

  unsigned NumKernargPreloadSGPRs = 0;

  unsigned NumUsedUserSGPRs = 0;
};

} // end namespace llvm

#endif // LLVM_LIB_TARGET_AMDGPU_GCNSUBTARGET_H
