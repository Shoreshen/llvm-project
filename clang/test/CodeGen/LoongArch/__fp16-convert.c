// NOTE: Assertions have been autogenerated by utils/update_cc_test_checks.py UTC_ARGS: --version 5
// RUN: %clang_cc1 -triple loongarch64 -emit-llvm %s -o - \
// RUN:   | FileCheck %s

__fp16 y;
short z;
// CHECK-LABEL: define dso_local void @bar1(
// CHECK-SAME: ) #[[ATTR0:[0-9]+]] {
// CHECK-NEXT:  [[ENTRY:.*:]]
// CHECK-NEXT:    [[TMP0:%.*]] = load half, ptr @y, align 2
// CHECK-NEXT:    [[CONV:%.*]] = fpext half [[TMP0]] to float
// CHECK-NEXT:    [[CONV1:%.*]] = fptosi float [[CONV]] to i16
// CHECK-NEXT:    store i16 [[CONV1]], ptr @z, align 2
// CHECK-NEXT:    ret void
//
void bar1(){
    z = y;
}
// CHECK-LABEL: define dso_local void @bar2(
// CHECK-SAME: ) #[[ATTR0]] {
// CHECK-NEXT:  [[ENTRY:.*:]]
// CHECK-NEXT:    [[TMP0:%.*]] = load i16, ptr @z, align 2
// CHECK-NEXT:    [[CONV:%.*]] = sitofp i16 [[TMP0]] to float
// CHECK-NEXT:    [[CONV1:%.*]] = fptrunc float [[CONV]] to half
// CHECK-NEXT:    store half [[CONV1]], ptr @y, align 2
// CHECK-NEXT:    ret void
//
void bar2(){
    y = z;
}
