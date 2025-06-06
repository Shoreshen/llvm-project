// RUN: llvm-tblgen -gen-disassembler -I %p/../../include %s | FileCheck %s
// RUN: llvm-tblgen -gen-disassembler --large-decoder-table -I %p/../../include %s | FileCheck %s --check-prefix=CHECK-LARGE

// Check that if decoding of an instruction fails and the instruction does not
// have a complete decoder method that can determine if the bitpattern is valid
// or not then the decoder tries to find a more general instruction that
// matches the bitpattern too.

include "llvm/Target/Target.td"

def archInstrInfo : InstrInfo { }

def arch : Target {
  let InstructionSet = archInstrInfo;
}

class TestInstruction : Instruction {
  let Size = 1;
  let OutOperandList = (outs);
  let InOperandList = (ins);
  field bits<8> Inst;
  field bits<8> SoftFail = 0;
}

def InstA : TestInstruction {
  let Inst = {0,0,0,0,?,?,?,?};
  let AsmString = "InstA";
}

def InstB : TestInstruction {
  let Inst = {0,0,0,0,0,0,?,?};
  let AsmString = "InstB";
  let DecoderMethod = "DecodeInstB";
  let hasCompleteDecoder = 0;
}

// CHECK:      /* 0 */       MCD::OPC_ExtractField, 4, 4,  // Inst{7-4} ...
// CHECK-NEXT: /* 3 */       MCD::OPC_FilterValueOrFail, 0,
// CHECK-NEXT: /* 5 */       MCD::OPC_CheckField, 2, 2, 0, 6, 0, // Skip to: 17
// CHECK-NEXT: /* 11 */      MCD::OPC_TryDecode, {{[0-9]+}}, {{[0-9]+}}, 0, 0, 0, // Opcode: InstB, Skip to: 17
// CHECK-NEXT: /* 17 */      MCD::OPC_Decode, {{[0-9]+}}, {{[0-9]+}}, 1, // Opcode: InstA
// CHECK-NEXT: /* 21 */      MCD::OPC_Fail,

// CHECK: if (!Check(S, DecodeInstB(MI, insn, Address, Decoder))) { DecodeComplete = false; return MCDisassembler::Fail; }

// CHECK:       unsigned NumToSkip = *Ptr++;
// CHECK-NEXT:  NumToSkip |= (*Ptr++) << 8;
// CHECK-NEXT:  return NumToSkip;

// CHECK-LARGE:      /* 0 */       MCD::OPC_ExtractField, 4, 4,  // Inst{7-4} ...
// CHECK-LARGE-NEXT: /* 3 */       MCD::OPC_FilterValueOrFail, 0,
// CHECK-LARGE-NEXT: /* 5 */       MCD::OPC_CheckField, 2, 2, 0, 7, 0, 0, // Skip to: 19
// CHECK-LARGE-NEXT: /* 12 */      MCD::OPC_TryDecode, {{[0-9]+}}, {{[0-9]+}}, 0, 0, 0, 0, // Opcode: InstB, Skip to: 19
// CHECK-LARGE-NEXT: /* 19 */      MCD::OPC_Decode, {{[0-9]+}}, {{[0-9]+}}, 1, // Opcode: InstA
// CHECK-LARGE-NEXT: /* 23 */      MCD::OPC_Fail,

// CHECK-LARGE: if (!Check(S, DecodeInstB(MI, insn, Address, Decoder))) { DecodeComplete = false; return MCDisassembler::Fail; }

// CHECK-LARGE:       unsigned NumToSkip = *Ptr++;
// CHECK-LARGE-NEXT:  NumToSkip |= (*Ptr++) << 8;
// CHECK-LARGE-NEXT:  NumToSkip |= (*Ptr++) << 16;
// CHECK-LARGE-NEXT:  return NumToSkip;
