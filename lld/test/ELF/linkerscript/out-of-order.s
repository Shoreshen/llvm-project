# REQUIRES: x86
# RUN: llvm-mc -filetype=obj -triple=x86_64-linux %s -o %t.o
# RUN: echo "SECTIONS { .data 0x4000 : {*(.data)} .dynsym 0x2000 : {*(.dynsym)} .dynstr : {*(.dynstr)} }" > %t.script
# RUN: ld.lld --hash-style=sysv -o %t.so --script %t.script %t.o -shared
# RUN: llvm-objdump --section-headers %t.so | FileCheck %s

# Note: how the layout is done:
#  we need to layout 2 segments, each contains sections:
#    seg1: .data .dynamic
#    seg2: .dynsym .dynstr .text .hash
# for each segment, we start from the first section, regardless
# whether it is an orphan or not (sections that are not listed in the
# linkerscript are orphans):
#   for seg1, we assign address: .data(0x4000), .dynamic(0x4008)
#   for seg2, we assign address: .dynsym(0x2000), .dynstr(0x2018) ...
# .dynsym is not an orphan, so we take address from script, we assign
# .dynstr current address cursor, which is the end # of .dynsym and so
# on for later sections.

# Also note, it is absolutely *illegal* to have section addresses of
# the same segment in none-increasing order, authors of linker scripts
# must take responsibility to make sure this does not happen.

# CHECK:      Sections:
# CHECK-NEXT: Idx Name          Size     VMA              Type
# CHECK-NEXT:   0               00000000 0000000000000000
# CHECK-NEXT:   1 .dynamic      00000060 0000000000000000
# CHECK-NEXT:   2 .data         00000008 0000000000004000
# CHECK-NEXT:   3 .dynsym       00000018 0000000000002000
# CHECK-NEXT:   4 .dynstr       00000001 0000000000002018
# CHECK-NEXT:   5 .hash         00000010 000000000000201c
# CHECK-NEXT:   6 .text         00000008 000000000000202c

# RUN: ld.lld -e 0 -o %t --script %t.script %t.o --fatal-warnings
# RUN: llvm-readelf -Sl %t | FileCheck %s --check-prefix=CHECK1

# CHECK1:       Name              Type            Address          Off    Size   ES Flg Lk Inf Al
# CHECK1-NEXT:                    NULL            0000000000000000 000000 000000 00      0   0  0
# CHECK1-NEXT:  .text             PROGBITS        0000000000000000 001000 000008 00  AX  0   0  4
# CHECK1-NEXT:  .data             PROGBITS        0000000000004000 002000 000008 00  WA  0   0  1
# CHECK1:     Program Headers:
# CHECK1-NEXT:  Type           Offset   VirtAddr           PhysAddr           FileSiz  MemSiz   Flg Align
# CHECK1-NEXT:  LOAD           0x001000 0x0000000000000000 0x0000000000000000 0x000008 0x000008 R E 0x1000
# CHECK1-NEXT:  LOAD           0x002000 0x0000000000004000 0x0000000000004000 0x000008 0x000008 RW  0x1000

.quad 0
.data
.quad 0

