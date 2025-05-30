// RUN: mlir-opt  --allow-unregistered-dialect \
// RUN:   --test-gpu-subgroup-reduce-lowering %s \
// RUN:   | FileCheck %s --check-prefix=CHECK-SUB

// RUN: mlir-opt --allow-unregistered-dialect \
// RUN:   --test-gpu-subgroup-reduce-lowering="expand-to-shuffles" %s \
// RUN:   | FileCheck %s --check-prefix=CHECK-SHFL

// RUN: mlir-opt --allow-unregistered-dialect \
// RUN:   --test-gpu-subgroup-reduce-lowering="expand-to-shuffles target=gfx942" %s \
// RUN:   | FileCheck %s --check-prefix=CHECK-GFX9

// RUN: mlir-opt --allow-unregistered-dialect \
// RUN:   --test-gpu-subgroup-reduce-lowering="expand-to-shuffles target=gfx1030" %s \
// RUN:   | FileCheck %s --check-prefix=CHECK-GFX10

// CHECK-SUB:  gpu.module @kernels {
// CHECK-SHFL: gpu.module @kernels {
// CHECK-GFX9: gpu.module @kernels {
// CHECK-GFX10: gpu.module @kernels {
gpu.module @kernels {

  // CHECK-SUB-LABEL:  gpu.func @kernel0(
  // CHECK-SUB-SAME:     %[[ARG0:.+]]: vector<5xf16>)
  //
  // CHECK-SHFL-LABEL: gpu.func @kernel0(
  // CHECK-GFX9-LABEL: gpu.func @kernel0(
  // CHECK-GFX10-LABEL: gpu.func @kernel0(
  gpu.func @kernel0(%arg0: vector<5xf16>) kernel {
    // CHECK-SUB: %[[VZ:.+]] = arith.constant dense<0.0{{.*}}> : vector<5xf16>
    // CHECK-SUB: %[[E0:.+]] = vector.extract_strided_slice %[[ARG0]] {offsets = [0], sizes = [2], strides = [1]} : vector<5xf16> to vector<2xf16>
    // CHECK-SUB: %[[R0:.+]] = gpu.subgroup_reduce add %[[E0]] : (vector<2xf16>) -> vector<2xf16>
    // CHECK-SUB: %[[V0:.+]] = vector.insert_strided_slice %[[R0]], %[[VZ]] {offsets = [0], strides = [1]} : vector<2xf16> into vector<5xf16>
    // CHECK-SUB: %[[E1:.+]] = vector.extract_strided_slice %[[ARG0]] {offsets = [2], sizes = [2], strides = [1]} : vector<5xf16> to vector<2xf16>
    // CHECK-SUB: %[[R1:.+]] = gpu.subgroup_reduce add %[[E1]] : (vector<2xf16>) -> vector<2xf16>
    // CHECK-SUB: %[[V1:.+]] = vector.insert_strided_slice %[[R1]], %[[V0]] {offsets = [2], strides = [1]} : vector<2xf16> into vector<5xf16>
    // CHECK-SUB: %[[E2:.+]] = vector.extract %[[ARG0]][4] : f16 from vector<5xf16>
    // CHECK-SUB: %[[R2:.+]] = gpu.subgroup_reduce add %[[E2]] : (f16) -> f16
    // CHECK-SUB: %[[V2:.+]] = vector.insert %[[R2]], %[[V1]] [4] : f16 into vector<5xf16>
    // CHECK-SUB: "test.consume"(%[[V2]]) : (vector<5xf16>) -> ()
    // CHECK-GFX9-COUNT-6: amdgpu.dpp
    // CHECK-GFX10-COUNT-4: amdgpu.dpp
    // CHECK-GFX10: rocdl.permlanex16
    // CHECK-GFX10-COUNT-2: rocdl.readlane
    %sum0 = gpu.subgroup_reduce add %arg0 : (vector<5xf16>) -> (vector<5xf16>)
    "test.consume"(%sum0) : (vector<5xf16>) -> ()

    // CHECK-SUB-COUNT-3: gpu.subgroup_reduce mul {{.+}} uniform
    // CHECK-SUB: "test.consume"
    // CHECK-GFX9-COUNT-6: amdgpu.dpp
    // CHECK-GFX10-COUNT-4: amdgpu.dpp
    // CHECK-GFX10: rocdl.permlanex16
    // CHECK-GFX10-COUNT-2: rocdl.readlane
    %sum1 = gpu.subgroup_reduce mul %arg0 uniform : (vector<5xf16>) -> (vector<5xf16>)
    "test.consume"(%sum1) : (vector<5xf16>) -> ()

    // CHECK-SUB-COUNT-3: gpu.subgroup_reduce mul {{.+}} cluster(size = 4)
    // CHECK-SUB: "test.consume"
    // CHECK-GFX9-COUNT-2: amdgpu.dpp {{.+}}
    // CHECK-GFX10-COUNT-2: amdgpu.dpp {{.+}}
    %sum2 = gpu.subgroup_reduce mul %arg0 cluster(size = 4) : (vector<5xf16>) -> (vector<5xf16>)
    "test.consume"(%sum2) : (vector<5xf16>) -> ()

    // CHECK-SUB-COUNT-3: gpu.subgroup_reduce mul {{.+}} uniform cluster(size = 4, stride = 2)
    // CHECK-SUB: "test.consume"
    %sum3 = gpu.subgroup_reduce mul %arg0 uniform cluster(size = 4, stride = 2) : (vector<5xf16>) -> (vector<5xf16>)
    "test.consume"(%sum3) : (vector<5xf16>) -> ()

    // CHECK-SUB: gpu.return
    gpu.return
  }

  // CHECK-SUB-LABEL:  gpu.func @kernel1(
  // CHECK-SUB-SAME:     %[[ARG0:.+]]: vector<1xf32>)
  //
  // CHECK-SHFL-LABEL: gpu.func @kernel1(
  // CHECK-GFX9-LABEL: gpu.func @kernel1(
  // CHECK-GFX10-LABEL: gpu.func @kernel1(
  gpu.func @kernel1(%arg0: vector<1xf32>) kernel {
    // CHECK-SUB: %[[E0:.+]] = vector.extract %[[ARG0]][0] : f32 from vector<1xf32>
    // CHECK-SUB: %[[R0:.+]] = gpu.subgroup_reduce add %[[E0]] : (f32) -> f32
    // CHECK-SUB: %[[V0:.+]] = vector.broadcast %[[R0]] : f32 to vector<1xf32>
    // CHECK-SUB: "test.consume"(%[[V0]]) : (vector<1xf32>) -> ()
    // CHECK-GFX9-COUNT-6: amdgpu.dpp
    // CHECK-GFX10-COUNT-4: amdgpu.dpp
    // CHECK-GFX10: rocdl.permlanex16
    // CHECK-GFX10-COUNT-2: rocdl.readlane
    %sum0 = gpu.subgroup_reduce add %arg0 : (vector<1xf32>) -> (vector<1xf32>)
    "test.consume"(%sum0) : (vector<1xf32>) -> ()

    // CHECK-SUB: gpu.subgroup_reduce add {{.+}} uniform : (f32) -> f32
    // CHECK-SUB: "test.consume"
    // CHECK-GFX9-COUNT-6: amdgpu.dpp
    // CHECK-GFX10-COUNT-4: amdgpu.dpp
    // CHECK-GFX10: rocdl.permlanex16
    // CHECK-GFX10-COUNT-2: rocdl.readlane
    %sum1 = gpu.subgroup_reduce add %arg0 uniform : (vector<1xf32>) -> (vector<1xf32>)
    "test.consume"(%sum1) : (vector<1xf32>) -> ()

    // Note stride is dropped because it is == 1.
    // CHECK-SUB: gpu.subgroup_reduce add {{.+}} cluster(size = 8) : (f32) -> f32
    // CHECK-SUB: "test.consume"
    // CHECK-GFX9-COUNT-2: amdgpu.dpp {{.+}} quad_perm
    // CHECK-GFX9: amdgpu.dpp {{.+}} row_half_mirror
    // CHECK-GFX10-COUNT-2: amdgpu.dpp {{.+}} quad_perm
    // CHECK-GFX10: amdgpu.dpp {{.+}} row_half_mirror
    %sum2 = gpu.subgroup_reduce add %arg0 cluster(size = 8, stride = 1) : (vector<1xf32>) -> (vector<1xf32>)
    "test.consume"(%sum2) : (vector<1xf32>) -> ()

    // CHECK-SUB: gpu.subgroup_reduce add {{.+}} uniform cluster(size = 8, stride = 4) : (f32) -> f32
    // CHECK-SUB: "test.consume"
    // CHECK-GFX9-NOT: amdgpu.dpp
    // CHECK-GFX10-NOT: amdgpu.dpp
    // CHECK-GFX10-NOT: rocdl.permlanex16
    %sum3 = gpu.subgroup_reduce add %arg0 uniform cluster(size = 8, stride = 4) : (vector<1xf32>) -> (vector<1xf32>)
    "test.consume"(%sum3) : (vector<1xf32>) -> ()

    // CHECK-SUB: gpu.return
    gpu.return
  }

  // These vectors fit the native shuffle size and should not be broken down.
  //
  // CHECK-SUB-LABEL:  gpu.func @kernel2(
  // CHECK-SUB-SAME:     %[[ARG0:.+]]: vector<3xi8>, %[[ARG1:.+]]: vector<4xi8>)
  //
  // CHECK-SHFL-LABEL: gpu.func @kernel2(
  //
  // CHECK-GFX9-LABEL: gpu.func @kernel2(
  // CHECK-GFX9-NOT: amdgpu.dpp
  //
  // CHECK-GFX10-LABEL: gpu.func @kernel2(
  // CHECK-GFX10-NOT: amdgpu.dpp
  gpu.func @kernel2(%arg0: vector<3xi8>, %arg1: vector<4xi8>) kernel {
    // CHECK-SUB: %[[R0:.+]] = gpu.subgroup_reduce add %[[ARG0]] : (vector<3xi8>) -> vector<3xi8>
    // CHECK-SUB: "test.consume"(%[[R0]]) : (vector<3xi8>) -> ()
    %sum0 = gpu.subgroup_reduce add %arg0 : (vector<3xi8>) -> (vector<3xi8>)
    "test.consume"(%sum0) : (vector<3xi8>) -> ()

    // CHECK-SUB: %[[R1:.+]] = gpu.subgroup_reduce add %[[ARG1]] : (vector<4xi8>) -> vector<4xi8>
    // CHECK-SUB: "test.consume"(%[[R1]]) : (vector<4xi8>) -> ()
    %sum1 = gpu.subgroup_reduce add %arg1 : (vector<4xi8>) -> (vector<4xi8>)
    "test.consume"(%sum1) : (vector<4xi8>) -> ()

    // CHECK-SUB: gpu.return
    gpu.return
  }

  // CHECK-SHFL-LABEL: gpu.func @kernel3(
  // CHECK-SHFL-SAME:    %[[ARG0:.+]]: i32)
  // CHECK-GFX9-LABEL: gpu.func @kernel3(
  // CHECK-GFX10-LABEL: gpu.func @kernel3(
  gpu.func @kernel3(%arg0: i32) kernel {
    // CHECK-SHFL-DAG: %[[C1:.+]] = arith.constant 1 : i32
    // CHECK-SHFL-DAG: %[[C2:.+]] = arith.constant 2 : i32
    // CHECK-SHFL-DAG: %[[C4:.+]] = arith.constant 4 : i32
    // CHECK-SHFL-DAG: %[[C8:.+]] = arith.constant 8 : i32
    // CHECK-SHFL-DAG: %[[C16:.+]] = arith.constant 16 : i32
    // CHECK-SHFL-DAG: %[[C32:.+]] = arith.constant 32 : i32

    // CHECK-SHFL: %[[S0:.+]], %{{.+}} = gpu.shuffle xor %[[ARG0]], %[[C1]], %[[C32]] : i32
    // CHECK-SHFL: %[[A0:.+]] = arith.addi %[[ARG0]], %[[S0]] : i32
    // CHECK-SHFL: %[[S1:.+]], %{{.+}} = gpu.shuffle xor %[[A0]], %[[C2]], %[[C32]] : i32
    // CHECK-SHFL: %[[A1:.+]] = arith.addi %[[A0]], %[[S1]] : i32
    // CHECK-SHFL: %[[S2:.+]], %{{.+}} = gpu.shuffle xor %[[A1]], %[[C4]], %[[C32]] : i32
    // CHECK-SHFL: %[[A2:.+]] = arith.addi %[[A1]], %[[S2]] : i32
    // CHECK-SHFL: %[[S3:.+]], %{{.+}} = gpu.shuffle xor %[[A2]], %[[C8]], %[[C32]] : i32
    // CHECK-SHFL: %[[A3:.+]] = arith.addi %[[A2]], %[[S3]] : i32
    // CHECK-SHFL: %[[S4:.+]], %{{.+}} = gpu.shuffle xor %[[A3]], %[[C16]], %[[C32]] : i32
    // CHECK-SHFL: %[[A4:.+]] = arith.addi %[[A3]], %[[S4]] : i32
    // CHECK-SHFL: "test.consume"(%[[A4]]) : (i32) -> ()
    
    // CHECK-GFX9-COUNT-6: amdgpu.dpp
    
    // CHECK-GFX10-COUNT-4: amdgpu.dpp
    // CHECK-GFX10: rocdl.permlanex16
    // CHECK-GFX10-COUNT-2: rocdl.readlane
    %sum0 = gpu.subgroup_reduce add %arg0 : (i32) -> i32
    "test.consume"(%sum0) : (i32) -> ()

    // CHECK-SHFL: gpu.return
    gpu.return
  }

  // CHECK-SHFL-LABEL: gpu.func @kernel3_clustered(
  // CHECK-SHFL-SAME:    %[[ARG0:.+]]: i32)
  //
  // CHECK-GFX9-LABEL: gpu.func @kernel3_clustered(
  // CHECK-GFX9-SAME:    %[[ARG0:.+]]: i32)
  //
  // CHECK-GFX10-LABEL: gpu.func @kernel3_clustered(
  // CHECK-GFX10-SAME:    %[[ARG0:.+]]: i32)
  gpu.func @kernel3_clustered(%arg0: i32) kernel {
    // CHECK-SHFL-DAG: %[[C1:.+]] = arith.constant 1 : i32
    // CHECK-SHFL-DAG: %[[C2:.+]] = arith.constant 2 : i32
    // CHECK-SHFL-DAG: %[[C4:.+]] = arith.constant 4 : i32
    // CHECK-SHFL-DAG: %[[C32:.+]] = arith.constant 32 : i32

    // CHECK-SHFL: %[[S0:.+]], %{{.+}} = gpu.shuffle xor %[[ARG0]], %[[C1]], %[[C32]] : i32
    // CHECK-SHFL: %[[A0:.+]] = arith.addi %[[ARG0]], %[[S0]] : i32
    // CHECK-SHFL: %[[S1:.+]], %{{.+}} = gpu.shuffle xor %[[A0]], %[[C2]], %[[C32]] : i32
    // CHECK-SHFL: %[[A1:.+]] = arith.addi %[[A0]], %[[S1]] : i32
    // CHECK-SHFL: %[[S2:.+]], %{{.+}} = gpu.shuffle xor %[[A1]], %[[C4]], %[[C32]] : i32
    // CHECK-SHFL: %[[A2:.+]] = arith.addi %[[A1]], %[[S2]] : i32
    // CHECK-SHFL: "test.consume"(%[[A2]]) : (i32) -> ()

    // CHECK-GFX9: %[[D0:.+]] = amdgpu.dpp %[[ARG0]] %[[ARG0]]  quad_perm([1 : i32, 0 : i32, 3 : i32, 2 : i32]) {bound_ctrl = true} : i32
    // CHECK-GFX9: %[[A0:.+]] = arith.addi %[[ARG0]], %[[D0]] : i32
    // CHECK-GFX9: %[[D1:.+]] = amdgpu.dpp %[[A0]] %[[A0]]  quad_perm([2 : i32, 3 : i32, 0 : i32, 1 : i32]) {bound_ctrl = true} : i32
    // CHECK-GFX9: %[[A1:.+]] = arith.addi %[[A0]], %[[D1]] : i32
    // CHECK-GFX9: %[[D2:.+]] = amdgpu.dpp %[[A1]] %[[A1]]  row_half_mirror(unit) {bound_ctrl = true} : i32
    // CHECK-GFX9: %[[A2:.+]] = arith.addi %[[A1]], %[[D2]] : i32

    // CHECK-GFX10: %[[D0:.+]] = amdgpu.dpp %[[ARG0]] %[[ARG0]]  quad_perm([1 : i32, 0 : i32, 3 : i32, 2 : i32]) {bound_ctrl = true} : i32
    // CHECK-GFX10: %[[A0:.+]] = arith.addi %[[ARG0]], %[[D0]] : i32
    // CHECK-GFX10: %[[D1:.+]] = amdgpu.dpp %[[A0]] %[[A0]]  quad_perm([2 : i32, 3 : i32, 0 : i32, 1 : i32]) {bound_ctrl = true} : i32
    // CHECK-GFX10: %[[A1:.+]] = arith.addi %[[A0]], %[[D1]] : i32
    // CHECK-GFX10: %[[D2:.+]] = amdgpu.dpp %[[A1]] %[[A1]]  row_half_mirror(unit) {bound_ctrl = true} : i32
    // CHECK-GFX10: %[[A2:.+]] = arith.addi %[[A1]], %[[D2]] : i32
    // CHECK-GFX10: "test.consume"(%[[A2]]) : (i32) -> ()
    %sum0 = gpu.subgroup_reduce add %arg0 cluster(size = 8) : (i32) -> i32
    "test.consume"(%sum0) : (i32) -> ()

    // CHECK-SHFL: gpu.return
    gpu.return
  }

  // CHECK-SHFL-LABEL: gpu.func @kernel3_clustered_strided(
  // CHECK-SHFL-SAME:    %[[ARG0:.+]]: i32)
  //
  // CHECK-GFX9-LABEL: gpu.func @kernel3_clustered_strided(
  // CHECK-GFX9-NOT: amdgpu.dpp
  //
  // CHECK-GFX10-LABEL: gpu.func @kernel3_clustered_strided(
  // CHECK-GFX10-NOT: amdgpu.dpp
  gpu.func @kernel3_clustered_strided(%arg0: i32) kernel {
    // CHECK-SHFL-DAG: %[[C1:.+]] = arith.constant 4 : i32
    // CHECK-SHFL-DAG: %[[C2:.+]] = arith.constant 8 : i32
    // CHECK-SHFL-DAG: %[[C4:.+]] = arith.constant 16 : i32
    // CHECK-SHFL-DAG: %[[C32:.+]] = arith.constant 32 : i32

    // CHECK-SHFL: %[[S0:.+]], %{{.+}} = gpu.shuffle xor %[[ARG0]], %[[C1]], %[[C32]] : i32
    // CHECK-SHFL: %[[A0:.+]] = arith.addi %[[ARG0]], %[[S0]] : i32
    // CHECK-SHFL: %[[S1:.+]], %{{.+}} = gpu.shuffle xor %[[A0]], %[[C2]], %[[C32]] : i32
    // CHECK-SHFL: %[[A1:.+]] = arith.addi %[[A0]], %[[S1]] : i32
    // CHECK-SHFL: %[[S2:.+]], %{{.+}} = gpu.shuffle xor %[[A1]], %[[C4]], %[[C32]] : i32
    // CHECK-SHFL: %[[A2:.+]] = arith.addi %[[A1]], %[[S2]] : i32
    // CHECK-SHFL: "test.consume"(%[[A2]]) : (i32) -> ()
    %sum0 = gpu.subgroup_reduce add %arg0 cluster(size = 8, stride = 4) : (i32) -> i32
    "test.consume"(%sum0) : (i32) -> ()

    // CHECK-SHFL: gpu.return
    gpu.return
  }

  // CHECK-SHFL-LABEL: gpu.func @kernel4(
  // CHECK-SHFL-SAME:    %[[ARG0:.+]]: vector<2xf16>)
  //
  // CHECK-GFX9-LABEL: gpu.func @kernel4(
  // CHECK-GFX9-NOT: amdgpu.dpp
  //
  // CHECK-GFX10-LABEL: gpu.func @kernel4(
  // CHECK-GFX10-NOT: amdgpu.dpp
  gpu.func @kernel4(%arg0: vector<2xf16>) kernel {
    // CHECK-SHFL-DAG: %[[C1:.+]] = arith.constant 1 : i32
    // CHECK-SHFL-DAG: %[[C2:.+]] = arith.constant 2 : i32
    // CHECK-SHFL-DAG: %[[C4:.+]] = arith.constant 4 : i32
    // CHECK-SHFL-DAG: %[[C8:.+]] = arith.constant 8 : i32
    // CHECK-SHFL-DAG: %[[C16:.+]] = arith.constant 16 : i32
    // CHECK-SHFL-DAG: %[[C32:.+]] = arith.constant 32 : i32

    // CHECK-SHFL: %[[V0:.+]] = vector.bitcast %[[ARG0]] : vector<2xf16> to vector<1xi32>
    // CHECK-SHFL: %[[I0:.+]] = vector.extract %[[V0]][0] : i32 from vector<1xi32>
    // CHECK-SHFL: %[[S0:.+]], %{{.+}} = gpu.shuffle xor %[[I0]], %[[C1]], %[[C32]] : i32
    // CHECK-SHFL: %[[BR0:.+]] = vector.broadcast %[[S0]] : i32 to vector<1xi32>
    // CHECK-SHFL: %[[BC0:.+]] = vector.bitcast %[[BR0]] : vector<1xi32> to vector<2xf16>
    // CHECK-SHFL: %[[ADD0:.+]] = arith.addf %[[ARG0]], %[[BC0]] : vector<2xf16>
    // CHECK-SHFL: %[[BC1:.+]] = vector.bitcast %[[ADD0]] : vector<2xf16> to vector<1xi32>
    // CHECK-SHFL: %[[I1:.+]] = vector.extract %[[BC1]][0] : i32 from vector<1xi32>
    // CHECK-SHFL: gpu.shuffle xor %[[I1]], %[[C2]], %[[C32]] : i32
    // CHECK-SHFL: arith.addf {{.+}} : vector<2xf16>
    // CHECK-SHFL: gpu.shuffle xor %{{.+}}, %[[C4]], %[[C32]] : i32
    // CHECK-SHFL: arith.addf {{.+}} : vector<2xf16>
    // CHECK-SHFL: gpu.shuffle xor %{{.+}}, %[[C8]], %[[C32]] : i32
    // CHECK-SHFL: arith.addf {{.+}} : vector<2xf16>
    // CHECK-SHFL: %[[SL:.+]], %{{.+}} = gpu.shuffle xor %{{.+}}, %[[C16]], %[[C32]] : i32
    // CHECK-SHFL: %[[BRL:.+]] = vector.broadcast %[[SL]] : i32 to vector<1xi32>
    // CHECK-SHFL: %[[BCL:.+]] = vector.bitcast %[[BRL]] : vector<1xi32> to vector<2xf16>
    // CHECK-SHFL: %[[ADDL:.+]] = arith.addf %{{.+}}, %[[BCL]] : vector<2xf16>
    // CHECK-SHFL: "test.consume"(%[[ADDL]]) : (vector<2xf16>) -> ()
    %sum0 = gpu.subgroup_reduce add %arg0 : (vector<2xf16>) -> (vector<2xf16>)
    "test.consume"(%sum0) : (vector<2xf16>) -> ()

    // CHECK-SHFL: gpu.return
    gpu.return
  }

  // CHECK-SHFL-LABEL: gpu.func @kernel4_clustered(
  // CHECK-SHFL-SAME:    %[[ARG0:.+]]: vector<2xf16>)
  //
  // CHECK-GFX9-LABEL: gpu.func @kernel4_clustered(
  // CHECK-GFX9-NOT: amdgpu.dpp
  //
  // CHECK-GFX10-LABEL: gpu.func @kernel4_clustered(
  // CHECK-GFX10-NOT: amdgpu.dpp
  gpu.func @kernel4_clustered(%arg0: vector<2xf16>) kernel {
    // CHECK-SHFL-DAG: %[[C1:.+]] = arith.constant 1 : i32
    // CHECK-SHFL-DAG: %[[C2:.+]] = arith.constant 2 : i32
    // CHECK-SHFL-DAG: %[[C32:.+]] = arith.constant 32 : i32

    // CHECK-SHFL-COUNT-2: gpu.shuffle xor
    %sum0 = gpu.subgroup_reduce add %arg0 cluster(size = 4) : (vector<2xf16>) -> (vector<2xf16>)
    "test.consume"(%sum0) : (vector<2xf16>) -> ()

    // CHECK-SHFL: gpu.return
    gpu.return
  }

  // CHECK-SHFL-LABEL: gpu.func @kernel5(
  // CHECK-SHFL-SAME:    %[[ARG0:.+]]: i16)
  //
  // CHECK-GFX9-LABEL: gpu.func @kernel5(
  //
  // CHECK-GFX10-LABEL: gpu.func @kernel5(
  // CHECK-GFX10-SAME:    %[[ARG0:.+]]: i16)
  gpu.func @kernel5(%arg0: i16) kernel {
    // CHECK-SHFL: %[[E0:.+]] = arith.extui %[[ARG0]] : i16 to i32
    // CHECK-SHFL: %[[S0:.+]], %{{.+}} = gpu.shuffle xor %[[E0]], {{.+}} : i32
    // CHECK-SHFL: %[[T0:.+]] = arith.trunci %[[S0]] : i32 to i16
    // CHECK-SHFL: %[[A0:.+]] = arith.addi %[[ARG0]], %[[T0]] : i16
    // CHECK-SHFL: %[[E1:.+]] = arith.extui %[[A0]] : i16 to i32
    // CHECK-SHFL: %{{.+}}, %{{.+}} = gpu.shuffle xor %[[E1]], {{.+}} : i32
    // CHECK-SHFL-COUNT-3: gpu.shuffle xor
    // CHECK-SHFL: arith.trunci {{.+}} : i32 to i16
    // CHECK-SHFL: %[[AL:.+]] = arith.addi {{.+}} : i16
    // CHECK-SHFL: "test.consume"(%[[AL]]) : (i16) -> ()
    
    // CHECK-GFX9-COUNT-6: amdgpu.dpp

    // CHECK-GFX10: %[[D0:.+]] = amdgpu.dpp %[[ARG0]] %[[ARG0]]  quad_perm([1 : i32, 0 : i32, 3 : i32, 2 : i32]) {bound_ctrl = true} : i16
    // CHECK-GFX10: %[[A0:.+]] = arith.addi %[[ARG0]], %[[D0]] : i16
    // CHECK-GFX10: %[[D1:.+]] = amdgpu.dpp %[[A0]] %[[A0]]  quad_perm([2 : i32, 3 : i32, 0 : i32, 1 : i32]) {bound_ctrl = true} : i16
    // CHECK-GFX10: %[[A1:.+]] = arith.addi %[[A0]], %[[D1]] : i16
    // CHECK-GFX10: %[[D2:.+]] = amdgpu.dpp %[[A1]] %[[A1]]  row_half_mirror(unit) {bound_ctrl = true} : i16
    // CHECK-GFX10: %[[A2:.+]] = arith.addi %[[A1]], %[[D2]] : i16
    // CHECK-GFX10: %[[D3:.+]] = amdgpu.dpp %[[A2]] %[[A2]]  row_mirror(unit) {bound_ctrl = true} : i16
    // CHECK-GFX10: %[[A3:.+]] = arith.addi %[[A2]], %[[D3]] : i16
    // CHECK-GFX10: %[[P0:.+]] = rocdl.permlanex16 %[[A3]], %[[A3]], %c-1_i32, %c-1_i32, true, false : i16, i32
    // CHECK-GFX10: %[[A4:.+]] = arith.addi %[[A3]], %[[P0]] : i16
    // CHECK-GFX10: %[[R0:.+]] = rocdl.readlane %[[A4]], %{{.+}} : (i16, i32) -> i16
    // CHECK-GFX10: %[[R1:.+]] = rocdl.readlane %[[A4]], %{{.+}} : (i16, i32) -> i16
    // CHECK-GFX10: %[[A5:.+]] = arith.addi %[[R0]], %[[R1]] : i16
    // CHECK-GFX10: "test.consume"(%[[A5]]) : (i16) -> ()
    %sum0 = gpu.subgroup_reduce add %arg0 : (i16) -> i16
    "test.consume"(%sum0) : (i16) -> ()

    // CHECK-SHFL: gpu.return
    gpu.return
  }

  // CHECK-SHFL-LABEL: gpu.func @kernel5_clustered(
  // CHECK-SHFL-SAME:    %[[ARG0:.+]]: i16)
  //
  // CHECK-GFX9-LABEL: gpu.func @kernel5_clustered
  // CHECK-GFX9-SAME:    %[[ARG0:.+]]: i16)
  //
  // CHECK-GFX10-LABEL: gpu.func @kernel5_clustered
  // CHECK-GFX10-SAME:    %[[ARG0:.+]]: i16)
  gpu.func @kernel5_clustered(%arg0: i16) kernel {
    // CHECK-SHFL: %[[E0:.+]] = arith.extui %[[ARG0]] : i16 to i32
    // CHECK-SHFL: %[[S0:.+]], %{{.+}} = gpu.shuffle xor %[[E0]], {{.+}} : i32
    // CHECK-SHFL: %[[T0:.+]] = arith.trunci %[[S0]] : i32 to i16
    // CHECK-SHFL: %[[A0:.+]] = arith.addi %[[ARG0]], %[[T0]] : i16
    // CHECK-SHFL: %[[E1:.+]] = arith.extui %[[A0]] : i16 to i32
    // CHECK-SHFL: %{{.+}}, %{{.+}} = gpu.shuffle xor %[[E1]], {{.+}} : i32
    // CHECK-SHFL-COUNT-2: gpu.shuffle xor
    // CHECK-SHFL: arith.trunci {{.+}} : i32 to i16
    // CHECK-SHFL: %[[AL:.+]] = arith.addi {{.+}} : i16
    // CHECK-SHFL: "test.consume"(%[[AL]]) : (i16) -> ()

    // CHECK-GFX9: %[[VAR0:.+]] = amdgpu.dpp %[[ARG0]] %[[ARG0]]  quad_perm([1 : i32, 0 : i32, 3 : i32, 2 : i32]) {bound_ctrl = true} : i16
    // CHECK-GFX9: %[[VAR1:.+]] = arith.addi %[[ARG0]], %[[VAR0]] : i16
    // CHECK-GFX9: %[[VAR2:.+]] = amdgpu.dpp %[[VAR1]] %[[VAR1]]  quad_perm([2 : i32, 3 : i32, 0 : i32, 1 : i32]) {bound_ctrl = true} : i16
    // CHECK-GFX9: %[[VAR3:.+]] = arith.addi %[[VAR1]], %[[VAR2]] : i16
    // CHECK-GFX9: %[[VAR4:.+]] = amdgpu.dpp %[[VAR3]] %[[VAR3]]  row_half_mirror(unit) {bound_ctrl = true} : i16
    // CHECK-GFX9: %[[VAR5:.+]] = arith.addi %[[VAR3]], %[[VAR4]] : i16
    // CHECK-GFX9: %[[VAR6:.+]] = amdgpu.dpp %[[VAR5]] %[[VAR5]]  row_mirror(unit) {bound_ctrl = true} : i16
    // CHECK-GFX9: %[[VAR7:.+]] = arith.addi %[[VAR5]], %[[VAR6]] : i16
    // CHECK-GFX9: "test.consume"(%[[VAR7]]) : (i16) -> ()

    // CHECK-GFX10: %[[VAR0:.+]] = amdgpu.dpp %[[ARG0]] %[[ARG0]]  quad_perm([1 : i32, 0 : i32, 3 : i32, 2 : i32]) {bound_ctrl = true} : i16
    // CHECK-GFX10: %[[VAR1:.+]] = arith.addi %[[ARG0]], %[[VAR0]] : i16
    // CHECK-GFX10: %[[VAR2:.+]] = amdgpu.dpp %[[VAR1]] %[[VAR1]]  quad_perm([2 : i32, 3 : i32, 0 : i32, 1 : i32]) {bound_ctrl = true} : i16
    // CHECK-GFX10: %[[VAR3:.+]] = arith.addi %[[VAR1]], %[[VAR2]] : i16
    // CHECK-GFX10: %[[VAR4:.+]] = amdgpu.dpp %[[VAR3]] %[[VAR3]]  row_half_mirror(unit) {bound_ctrl = true} : i16
    // CHECK-GFX10: %[[VAR5:.+]] = arith.addi %[[VAR3]], %[[VAR4]] : i16
    // CHECK-GFX10: %[[VAR6:.+]] = amdgpu.dpp %[[VAR5]] %[[VAR5]]  row_mirror(unit) {bound_ctrl = true} : i16
    // CHECK-GFX10: %[[VAR7:.+]] = arith.addi %[[VAR5]], %[[VAR6]] : i16
    // CHECK-GFX10: "test.consume"(%[[VAR7]]) : (i16) -> ()
    %sum0 = gpu.subgroup_reduce add %arg0 cluster(size = 16) : (i16) -> i16
    "test.consume"(%sum0) : (i16) -> ()

    // CHECK-SHFL: gpu.return
    gpu.return
  }

  // CHECK-SHFL-LABEL: gpu.func @kernel6(
  // CHECK-SHFL-SAME:    %[[ARG0:.+]]: vector<3xi8>)
  //
  // CHECK-GFX9-LABEL: gpu.func @kernel6(
  // CHECK-GFX9-NOT: amdgpu.dpp
  //
  // CHECK-GFX10-LABEL: gpu.func @kernel6(
  // CHECK-GFX10-NOT: amdgpu.dpp
  gpu.func @kernel6(%arg0: vector<3xi8>) kernel {
    // CHECK-SHFL: %[[CZ:.+]] = arith.constant dense<0> : vector<4xi8>
    // CHECK-SHFL: %[[V0:.+]] = vector.insert_strided_slice %[[ARG0]], %[[CZ]] {offsets = [0], strides = [1]} : vector<3xi8> into vector<4xi8>
    // CHECK-SHFL: %[[BC0:.+]] = vector.bitcast %[[V0]] : vector<4xi8> to vector<1xi32>
    // CHECK-SHFL: %[[I0:.+]] = vector.extract %[[BC0]][0] : i32 from vector<1xi32>
    // CHECK-SHFL: %[[S0:.+]], %{{.+}} = gpu.shuffle xor %[[I0]], {{.+}} : i32
    // CHECK-SHFL: %[[BR0:.+]] = vector.broadcast %[[S0]] : i32 to vector<1xi32>
    // CHECK-SHFL: %[[BC1:.+]] = vector.bitcast %[[BR0]] : vector<1xi32> to vector<4xi8>
    // CHECK-SHFL: %[[ADD0:.+]] = arith.addi %[[V0]], %[[BC1]] : vector<4xi8>
    // CHECK-SHFL: %[[BC2:.+]] = vector.bitcast %[[ADD0]] : vector<4xi8> to vector<1xi32>
    // CHECK-SHFL: %[[I1:.+]] = vector.extract %[[BC2]][0] : i32 from vector<1xi32>
    // CHECK-SHFL-COUNT-4: gpu.shuffle xor
    // CHECK-SHFL: %[[ESS:.+]] = vector.extract_strided_slice %{{.+}} {offsets = [0], sizes = [3], strides = [1]} : vector<4xi8> to vector<3xi8>
    // CHECK-SHFL: "test.consume"(%[[ESS]]) : (vector<3xi8>) -> ()
    %sum0 = gpu.subgroup_reduce add %arg0 : (vector<3xi8>) -> (vector<3xi8>)
    "test.consume"(%sum0) : (vector<3xi8>) -> ()

    // CHECK-SHFL: gpu.return
    gpu.return
  }

  // CHECK-SHFL-LABEL: gpu.func @kernel_cluster_size_is_subgroup_size(
  // CHECK-SHFL-SAME:    %[[ARG0:.+]]: vector<3xi8>)
  //
  // CHECK-GFX9-LABEL: gpu.func @kernel_cluster_size_is_subgroup_size(
  // CHECK-GFX9-NOT: amdgpu.dpp
  //
  // CHECK-GFX10-LABEL: gpu.func @kernel_cluster_size_is_subgroup_size(
  // CHECK-GFX10-NOT: amdgpu.dpp
  gpu.func @kernel_cluster_size_is_subgroup_size(%arg0: vector<3xi8>) kernel {
    // CHECK-SHFL-COUNT-5: gpu.shuffle xor
    %sum0 = gpu.subgroup_reduce add %arg0 cluster(size = 32) : (vector<3xi8>) -> (vector<3xi8>)
    "test.consume"(%sum0) : (vector<3xi8>) -> ()

    // CHECK-SHFL: gpu.return
    gpu.return
  }
}

