// RUN: mlir-opt -split-input-file -verify-diagnostics %s

func.func @not_enough_sizes(%sz : index) {
  // expected-error@+1 {{expected 6 or more operands, but found 5}}
  "gpu.launch"(%sz, %sz, %sz, %sz, %sz) ({
    gpu.return
  }) {operandSegmentSizes = array<i32: 0, 1, 1, 1, 1, 1, 1, 0>} : (index, index, index, index, index) -> ()
  return
}

// -----

func.func @no_region_attrs(%sz : index) {
  // expected-error@+1 {{unexpected number of region arguments}}
  "gpu.launch"(%sz, %sz, %sz, %sz, %sz, %sz) ({
  ^bb1(%bx: index, %by: index, %bz: index,
       %tx: index, %ty: index, %tz: index):
    gpu.terminator
  }) {operandSegmentSizes = array<i32: 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0>} : (index, index, index, index, index, index) -> ()
  return
}

// -----

func.func @launch_requires_gpu_return(%sz : index) {
  // @expected-note@+1 {{in 'gpu.launch' body region}}
  gpu.launch blocks(%bx, %by, %bz) in (%sbx = %sz, %sby = %sz, %sbz = %sz)
             threads(%tx, %ty, %tz) in (%stx = %sz, %sty = %sz, %stz = %sz) {
    // @expected-error@+2 {{expected 'gpu.terminator' or a terminator with successors}}
    %one = arith.constant 1 : i32
    "gpu.yield"(%one) : (i32) -> ()
  }
  return
}

// -----

func.func @launch_func_too_few_operands(%sz : index) {
  // expected-error@+1 {{expected 6 or more operands}}
  "gpu.launch_func"(%sz, %sz, %sz, %sz, %sz)
      {operandSegmentSizes = array<i32: 0, 1, 1, 1, 1, 1, 0, 0>}
      : (index, index, index, index, index) -> ()
  return
}

// -----

func.func @launch_func_missing_parent_module_attribute(%sz : index) {
  // expected-error@+1 {{expected the closest surrounding module to have the 'gpu.container_module' attribute}}
  gpu.launch_func @foo::@bar blocks in (%sz, %sz, %sz) threads in (%sz, %sz, %sz)
  return
}

// -----

module attributes {gpu.container_module} {
  func.func @launch_func_missing_callee_attribute(%sz : index) {
    // expected-error@+1 {{'gpu.launch_func' op requires attribute 'kernel'}}
    "gpu.launch_func"(%sz, %sz, %sz, %sz, %sz, %sz)
        {operandSegmentSizes = array<i32: 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0>}
        : (index, index, index, index, index, index) -> ()
    return
  }
}

// -----

module attributes {gpu.container_module} {
  func.func @launch_func_no_function_attribute(%sz : index) {
    // expected-error@+1 {{custom op 'gpu.launch_func' invalid kind of attribute specified}}
    gpu.launch_func "foo" blocks in (%sz, %sz, %sz) threads in (%sz, %sz, %sz)
    return
  }
}

// -----

module attributes {gpu.container_module} {
  func.func @launch_func_undefined_module(%sz : index) {
    // expected-error@+1 {{kernel container 'kernels' is undefined}}
    gpu.launch_func @kernels::@kernel_1 blocks in (%sz, %sz, %sz) threads in (%sz, %sz, %sz)
    return
  }
}

// -----

module attributes {gpu.container_module} {
  module @kernels {
    // expected-error@+1 {{'gpu.func' op expects parent op 'gpu.module'}}
    gpu.func @kernel_1(%arg1 : !llvm.ptr) {
      gpu.return
    }
  }
}

// -----

module attributes {gpu.container_module} {
  module @kernels {
  }

  func.func @launch_func_missing_module_attribute(%sz : index) {
    // expected-error@+1 {{kernel module 'kernels' is undefined}}
    gpu.launch_func @kernels::@kernel_1 blocks in (%sz, %sz, %sz) threads in (%sz, %sz, %sz)
    return
  }
}

// -----

module attributes {gpu.container_module} {
  gpu.module @kernels { }

  func.func @launch_func_undefined_function(%sz : index) {
    // expected-error@+1 {{kernel function '@kernels::@kernel_1' is undefined}}
    gpu.launch_func @kernels::@kernel_1 blocks in (%sz, %sz, %sz) threads in (%sz, %sz, %sz)
    return
  }
}

// -----

module attributes {gpu.container_module} {
  gpu.module @kernels {
    // expected-note@+1 {{see the kernel definition here}}
    memref.global "private" @kernel_1 : memref<4xi32>
  }

  func.func @launch_func_undefined_function(%sz : index) {
    // expected-error@+1 {{referenced kernel '@kernels::@kernel_1' is not a function}}
    gpu.launch_func @kernels::@kernel_1 blocks in (%sz, %sz, %sz) threads in (%sz, %sz, %sz)
    return
  }
}

// -----

module attributes {gpu.container_module} {
  module @kernels {
    gpu.func @kernel_1(%arg1 : !llvm.ptr) kernel {
      gpu.return
    }
  }

  func.func @launch_func_missing_kernel_attr(%sz : index, %arg : !llvm.ptr) {
    // expected-error@+1 {{kernel module 'kernels' is undefined}}
    gpu.launch_func @kernels::@kernel_1 blocks in (%sz, %sz, %sz) threads in (%sz, %sz, %sz) args(%arg : !llvm.ptr)
    return
  }
}

// -----

module attributes {gpu.container_module} {
  gpu.module @kernels {
    gpu.func @kernel_1(%arg1 : !llvm.ptr) {
      gpu.return
    }
  }

  func.func @launch_func_missing_kernel_attr(%sz : index, %arg : !llvm.ptr) {
    // expected-error@+1 {{kernel function is missing the 'gpu.kernel' attribute}}
    gpu.launch_func @kernels::@kernel_1 blocks in (%sz, %sz, %sz) threads in (%sz, %sz, %sz) args(%arg : !llvm.ptr)
    return
  }
}

// -----

module attributes {gpu.container_module} {
  gpu.module @kernels {
    gpu.func @kernel_1(%arg1 : !llvm.ptr) kernel {
      gpu.return
    }
  }

  func.func @launch_func_kernel_operand_size(%sz : index, %arg : !llvm.ptr) {
    // expected-error@+1 {{got 2 kernel operands but expected 1}}
    gpu.launch_func @kernels::@kernel_1 blocks in (%sz, %sz, %sz) threads in (%sz, %sz, %sz) args(%arg : !llvm.ptr, %arg : !llvm.ptr)
    return
  }
}

// -----

module attributes {gpu.container_module} {
  gpu.module @kernels {
    gpu.func @kernel_1(%arg1 : f32) kernel {
      gpu.return
    }
  }

  func.func @launch_func_kernel_operand_types(%sz : index, %arg : f32) {
    // expected-err@+1 {{type of function argument 0 does not match}}
    gpu.launch_func @kernels::@kernel_1 blocks in (%sz, %sz, %sz) threads in (%sz, %sz, %sz) args(%arg : f32)
    return
  }
}

// -----

module attributes {gpu.container_module} {
  func.func @launch_func_kernel_operand_attr(%sz : index) {
    // expected-error@+1 {{expected ')' in argument list}}
    gpu.launch_func @foo::@bar blocks in (%sz, %sz, %sz) threads in (%sz, %sz, %sz) args(%sz : index {foo})
    return
  }
}

// -----

func.func @reduce_bad_type(%arg0 : vector<4xf32>) {
  // expected-error@+1 {{'gpu.all_reduce' op operand #0 must be Integer or Float}}
  %res = gpu.all_reduce add %arg0 {} : (vector<4xf32>) -> vector<4xf32>
  return
}

// -----

func.func @reduce_no_op_no_body(%arg0 : f32) {
  // expected-error@+1 {{expected either an op attribute or a non-empty body}}
  %res = "gpu.all_reduce"(%arg0) ({}) : (f32) -> (f32)
  return
}

// -----

func.func @reduce_op_and_body(%arg0 : f32) {
  // expected-error@+1 {{expected either an op attribute or a non-empty body}}
  %res = "gpu.all_reduce"(%arg0) ({
  ^bb(%lhs : f32, %rhs : f32):
    "gpu.yield"(%lhs) : (f32) -> ()
  }) {op = #gpu<all_reduce_op add>} : (f32) -> (f32)
  return
}

// -----

func.func @reduce_invalid_op(%arg0 : f32) {
  // expected-error@+1 {{invalid op kind}}
  %res = gpu.all_reduce foo %arg0 {} : (f32) -> (f32)
  return
}

// -----

func.func @reduce_invalid_op_type_minsi(%arg0 : f32) {
  // expected-error@+1 {{`minsi` reduction operation is not compatible with type 'f32'}}
  %res = gpu.all_reduce minsi %arg0 {} : (f32) -> (f32)
  return
}

// -----

func.func @reduce_invalid_op_type_minui(%arg0 : f32) {
  // expected-error@+1 {{`minui` reduction operation is not compatible with type 'f32'}}
  %res = gpu.all_reduce minui %arg0 {} : (f32) -> (f32)
  return
}

// -----

func.func @reduce_invalid_op_type_maxsi(%arg0 : f32) {
  // expected-error@+1 {{`maxsi` reduction operation is not compatible with type 'f32'}}
  %res = gpu.all_reduce maxsi %arg0 {} : (f32) -> (f32)
  return
}

// -----

func.func @reduce_invalid_op_type_maxui(%arg0 : f32) {
  // expected-error@+1 {{`maxui` reduction operation is not compatible with type 'f32'}}
  %res = gpu.all_reduce maxui %arg0 {} : (f32) -> (f32)
  return
}

// -----

func.func @reduce_invalid_op_type_and(%arg0 : f32) {
  // expected-error@+1 {{`and` reduction operation is not compatible with type 'f32'}}
  %res = gpu.all_reduce and %arg0 {} : (f32) -> (f32)
  return
}

// -----

func.func @reduce_invalid_op_type_or(%arg0 : f32) {
  // expected-error@+1 {{`or` reduction operation is not compatible with type 'f32'}}
  %res = gpu.all_reduce or %arg0 {} : (f32) -> (f32)
  return
}

// -----

func.func @reduce_invalid_op_type_xor(%arg0 : f32) {
  // expected-error@+1 {{`xor` reduction operation is not compatible with type 'f32'}}
  %res = gpu.all_reduce xor %arg0 {} : (f32) -> (f32)
  return
}

// -----

func.func @reduce_invalid_op_type_minnumf(%arg0 : i32) {
  // expected-error@+1 {{`minnumf` reduction operation is not compatible with type 'i32'}}
  %res = gpu.all_reduce minnumf %arg0 {} : (i32) -> (i32)
  return
}

// -----

func.func @reduce_invalid_op_type_maxnumf(%arg0 : i32) {
  // expected-error@+1 {{`maxnumf` reduction operation is not compatible with type 'i32'}}
  %res = gpu.all_reduce maxnumf %arg0 {} : (i32) -> (i32)
  return
}

// -----

func.func @reduce_invalid_op_type_minimumf(%arg0 : i32) {
  // expected-error@+1 {{`minimumf` reduction operation is not compatible with type 'i32'}}
  %res = gpu.all_reduce minimumf %arg0 {} : (i32) -> (i32)
  return
}

// -----

func.func @reduce_invalid_op_type_maximumf(%arg0 : i32) {
  // expected-error@+1 {{`maximumf` reduction operation is not compatible with type 'i32'}}
  %res = gpu.all_reduce maximumf %arg0 {} : (i32) -> (i32)
  return
}

// -----

func.func @subgroup_reduce_zero_cluster_size(%arg0 : vector<4xf32>) {
  // expected-error@+1 {{cluster size 0 is not a power of two}}
  %res = gpu.subgroup_reduce add %arg0 cluster(size = 0) : (vector<4xf32>) -> vector<4xf32>
  return
}

// -----

func.func @subgroup_reduce_npot_cluster_size(%arg0 : vector<4xf32>) {
  // expected-error@+1 {{cluster size 3 is not a power of two}}
  %res = gpu.subgroup_reduce add %arg0 cluster(size = 3) : (vector<4xf32>) -> vector<4xf32>
  return
}

// -----

func.func @subgroup_reduce_zero_cluster_stride(%arg0 : vector<4xf32>) {
  // expected-error@+1 {{cluster stride 0 is not a power of two}}
  %res = gpu.subgroup_reduce add %arg0 cluster(size = 4, stride = 0) : (vector<4xf32>) -> vector<4xf32>
  return
}

// -----

func.func @subgroup_reduce_cluster_stride_without_size(%arg0 : vector<4xf32>) {
  // expected-error@+1 {{cluster stride can only be specified if cluster size is specified}}
  %res = gpu.subgroup_reduce add %arg0 { cluster_stride = 2 : i32 } : (vector<4xf32>) -> vector<4xf32>
  return
}


// -----

func.func @subgroup_reduce_bad_type(%arg0 : vector<2x2xf32>) {
  // expected-error@+1 {{'gpu.subgroup_reduce' op operand #0 must be Integer or Float or fixed-length vector of}}
  %res = gpu.subgroup_reduce add %arg0 : (vector<2x2xf32>) -> vector<2x2xf32>
  return
}

// -----

func.func @subgroup_reduce_bad_type_scalable(%arg0 : vector<[2]xf32>) {
  // expected-error@+1 {{'gpu.subgroup_reduce' op operand #0 must be Integer or Float or fixed-length vector of}}
  %res = gpu.subgroup_reduce add %arg0 : (vector<[2]xf32>) -> vector<[2]xf32>
  return
}

// -----

func.func @subgroup_reduce_invalid_op_type_and(%arg0 : f32) {
  // expected-error@+1 {{`and` reduction operation is not compatible with type 'f32'}}
  %res = gpu.subgroup_reduce and %arg0 : (f32) -> (f32)
  return
}

// -----

func.func @subgroup_reduce_invalid_op_type_maxnumf(%arg0 : i32) {
  // expected-error@+1 {{`maxnumf` reduction operation is not compatible with type 'i32'}}
  %res = gpu.subgroup_reduce maxnumf %arg0 : (i32) -> (i32)
  return
}

// -----

func.func @reduce_incorrect_region_arguments(%arg0 : f32) {
  // expected-error@+1 {{expected two region arguments}}
  %res = gpu.all_reduce %arg0 {
  ^bb(%lhs : f32):
    "gpu.yield"(%lhs) : (f32) -> ()
  } : (f32) -> (f32)
  return
}

// -----

func.func @reduce_incorrect_region_arguments(%arg0 : f32) {
  // expected-error@+1 {{incorrect region argument type}}
  %res = gpu.all_reduce %arg0 {
  ^bb(%lhs : f32, %rhs : i32):
    "gpu.yield"(%lhs) : (f32) -> ()
  } : (f32) -> (f32)
  return
}

// -----

func.func @reduce_incorrect_yield(%arg0 : f32) {
  // expected-error@+1 {{expected one gpu.yield operand}}
  %res = gpu.all_reduce %arg0 {
  ^bb(%lhs : f32, %rhs : f32):
    "gpu.yield"(%lhs, %rhs) : (f32, f32) -> ()
  } : (f32) -> (f32)
  return
}

// -----

func.func @reduce_incorrect_yield(%arg0 : f32) {
  // expected-error@+1 {{incorrect gpu.yield type}}
  %res = gpu.all_reduce %arg0 {
  ^bb(%lhs : f32, %rhs : f32):
    %one = arith.constant 1 : i32
    "gpu.yield"(%one) : (i32) -> ()
  } : (f32) -> (f32)
  return
}

// -----

func.func @reduce_incorrect_yield(%arg0 : f32) {
  // expected-error@+1 {{expected gpu.yield op in region}}
  %res = gpu.all_reduce %arg0 {
  ^bb(%lhs : f32, %rhs : f32):
    "test.finish" () : () -> ()
  } : (f32) -> (f32)
  return
}

// -----

func.func @shuffle_mismatching_type(%arg0 : f32, %arg1 : i32, %arg2 : i32) {
  // expected-error@+1 {{op failed to verify that all of {value, shuffleResult} have same type}}
  %shfl, %pred = "gpu.shuffle"(%arg0, %arg1, %arg2) { mode = #gpu<shuffle_mode xor> } : (f32, i32, i32) -> (i32, i1)
  return
}

// -----

func.func @shuffle_unsupported_type(%arg0 : index, %arg1 : i32, %arg2 : i32) {
  // expected-error@+1 {{op operand #0 must be Integer or Float or fixed-length vector of Integer or Float values of ranks 1, but got 'index'}}
  %shfl, %pred = gpu.shuffle xor %arg0, %arg1, %arg2 : index
  return
}

// -----

func.func @shuffle_unsupported_type_vec(%arg0 : vector<[4]xf32>, %arg1 : i32, %arg2 : i32) {
  // expected-error@+1 {{op operand #0 must be Integer or Float or fixed-length vector of Integer or Float values of ranks 1, but got 'vector<[4]xf32>'}}
  %shfl, %pred = gpu.shuffle xor %arg0, %arg1, %arg2 : vector<[4]xf32>
  return
}

// -----

func.func @rotate_mismatching_type(%arg0 : f32) {
  // expected-error@+1 {{op failed to verify that all of {value, rotateResult} have same type}}
  %rotate, %valid = "gpu.rotate"(%arg0) { offset = 4 : i32, width = 16 : i32 } : (f32) -> (i32, i1)
  return
}

// -----

func.func @rotate_unsupported_type(%arg0 : index) {
  // expected-error@+1 {{op operand #0 must be Integer or Float or fixed-length vector of Integer or Float values of ranks 1, but got 'index'}}
  %rotate, %valid = gpu.rotate %arg0, 4, 16 : index
  return
}

// -----

func.func @rotate_unsupported_type_vec(%arg0 : vector<[4]xf32>) {
  %offset = arith.constant 4 : i32
  %width = arith.constant 16 : i32
  // expected-error@+1 {{op operand #0 must be Integer or Float or fixed-length vector of Integer or Float values of ranks 1, but got 'vector<[4]xf32>'}}
  %rotate, %valid = gpu.rotate %arg0, 4, 16 : vector<[4]xf32>
  return
}

// -----

func.func @rotate_unsupported_width(%arg0 : f32) {
  // expected-error@+1 {{'gpu.rotate' op attribute 'width' failed to satisfy constraint: 32-bit signless integer attribute whose value is a power of two > 0}}
  %rotate, %valid = "gpu.rotate"(%arg0) { offset = 4 : i32, width = 15 : i32 } : (f32) -> (f32, i1)
  return
}

// -----

func.func @rotate_unsupported_offset(%arg0 : f32) {
  // expected-error@+1 {{op offset must be in the range [0, 16)}}
  %rotate, %valid = "gpu.rotate"(%arg0) { offset = 16 : i32, width = 16 : i32 }: (f32) -> (f32, i1)
  return
}

// -----

func.func @rotate_unsupported_offset_minus(%arg0 : f32) {
  // expected-error@+1 {{'gpu.rotate' op attribute 'offset' failed to satisfy constraint: 32-bit signless integer attribute whose minimum value is 0}}
  %rotate, %valid = "gpu.rotate"(%arg0) { offset = -1 : i32, width = 16 : i32 } : (f32) -> (f32, i1)
  return
}

// -----

module {
  gpu.module @gpu_funcs {
    // expected-error @+1 {{custom op 'gpu.func' gpu.func requires named arguments}}
    gpu.func @kernel_1(f32, f32) {
    ^bb0(%arg0: f32):
      gpu.return
    }
  }
}

// -----

module {
  gpu.module @gpu_funcs {
    // expected-error @+1 {{attribute 'function_type' failed to satisfy constraint: type attribute of function type}}
    "gpu.func"() ({
      gpu.return
    }) {sym_name="kernel_1", function_type=f32} : () -> ()
  }
}

// -----

module {
  gpu.module @gpu_funcs {
    // expected-error @below {{'gpu.func' op expected memref type in attribution}}
    gpu.func @kernel() workgroup(%0: i32) {
      gpu.return
    }
  }
}

// -----

module {
  gpu.module @gpu_funcs {
    // expected-error @below {{'gpu.func' op expected memory space workgroup in attribution}}
    gpu.func @kernel() workgroup(%0: memref<4xf32, #gpu.address_space<private>>) {
      gpu.return
    }
  }
}

// -----

module {
  gpu.module @gpu_funcs {
    // expected-error @below {{'gpu.func' op expected memory space private in attribution}}
    gpu.func @kernel() private(%0: memref<4xf32, #gpu.address_space<workgroup>>) {
      gpu.return
    }
  }
}

// -----

module {
  gpu.module @gpu_funcs {
    // expected-note @+1 {{return type declared here}}
    gpu.func @kernel() {
      %0 = arith.constant 0 : index
      // expected-error @+1 {{'gpu.return' op expected 0 result operands}}
      gpu.return %0 : index
    }
  }
}

// -----

module {
  gpu.module @gpu_funcs {
    // expected-error @+1 {{'gpu.func' op expected void return type for kernel function}}
    gpu.func @kernel() -> index kernel {
      %0 = arith.constant 0 : index
      gpu.return
    }
  }
}

// -----

module {
  gpu.module @gpu_funcs {
    // expected-error @+1 {{'gpu.func' op expected at least 5 arguments to body region}}
    "gpu.func"() ({
    ^bb0(%arg0: f32, %arg1: memref<?xf32>, %arg2: memref<5xf32, 3>, %arg3: memref<5xf32, 5>):
      "gpu.return"() : () -> ()
    } ) {function_type = (f32, memref<?xf32>) -> (), gpu.kernel, sym_name = "kernel_1", workgroup_attributions = 3: i64} : () -> ()
  }
}

// -----

module {
  gpu.module @gpu_funcs {
    // expected-error @+1 {{expected body with at least one block}}
    "gpu.func"() ({}) {function_type = () -> (), gpu.kernel, sym_name = "kernel"} : () -> ()
  }
}

// -----

func.func @sync_wait_with_result() {
  // expected-error @+1 {{cannot name an operation with no results}}
  %t = gpu.wait
}

// -----

func.func @async_wait_without_result() {
  // expected-error @+1 {{custom op 'gpu.wait' needs to be named when marked 'async'}}
  gpu.wait async
}

// -----

func.func @memcpy_incompatible_type(%dst : memref<?xf32>, %src : memref<?xi32>) {
  // expected-error @+1 {{'gpu.memcpy' op arguments have incompatible element type}}
  gpu.memcpy %dst, %src  : memref<?xf32>, memref<?xi32>
}

// -----

func.func @memcpy_incompatible_shape(%dst : memref<7xf32>, %src : memref<9xf32>) {
  // expected-error @+1 {{'gpu.memcpy' op arguments have incompatible shape}}
  gpu.memcpy %dst, %src  : memref<7xf32>, memref<9xf32>
}

// -----

func.func @memset_incompatible_shape(%dst : memref<?xf32>, %value : i32) {
  // expected-error @+1 {{'gpu.memset' op failed to verify that all of {dst, value} have same element type}}
  gpu.memset %dst, %value  : memref<?xf32>, i32
}

// -----

func.func @mmamatrix_invalid_shape(){
    %wg = memref.alloca() {alignment = 32} : memref<32x32xf16, 3>
    %i = arith.constant 16 : index
    // expected-error @+1 {{MMAMatrixType must have exactly two dimensions}}
    %0 = gpu.subgroup_mma_load_matrix %wg[%i, %i] {leadDimension = 32 : index} : memref<32x32xf16, 3> -> !gpu.mma_matrix<16x16x16xf16, "AOp">
    return
}

// -----

func.func @mmamatrix_operand_type(){
    %wg = memref.alloca() {alignment = 32} : memref<32x32xf16, 3>
    %i = arith.constant 16 : index
    // expected-error @+1 {{operand expected to be one of AOp, BOp or COp}}
    %0 = gpu.subgroup_mma_load_matrix %wg[%i, %i] {leadDimension = 32 : index} : memref<32x32xf16, 3> -> !gpu.mma_matrix<16x16xf16, "EOp">
    return
}

// -----

func.func @mmamatrix_invalid_element_type(){
    %wg = memref.alloca() {alignment = 32} : memref<32x32xf16, 3>
    %i = arith.constant 16 : index
    // expected-error @+1 {{MMAMatrixType elements must be SI8, UI8, I32, F16, or F32}}
    %0 = gpu.subgroup_mma_load_matrix %wg[%i, %i] {leadDimension = 32 : index} : memref<32x32xf16, 3> -> !gpu.mma_matrix<16x16xbf16, "AOp">
    return
}

// -----

#layout_map_col_major = affine_map<(i, j) -> (j, i)>

func.func @mmaLoadOp_identity_layout(){
    %wg = memref.alloca() {alignment = 32} : memref<32x32xf16, #layout_map_col_major, 3>
    %i = arith.constant 16 : index
    // expected-error @+1 {{expected source memref most minor dim must have unit stride}}
    %0 = gpu.subgroup_mma_load_matrix %wg[%i, %i] {leadDimension = 32 : index} : memref<32x32xf16, #layout_map_col_major, 3> -> !gpu.mma_matrix<16x16xf16, "AOp">
    return
}

// -----

func.func @mma_invalid_memref_type(%src: memref<32x4xvector<4x8xf32>>, %i: index) {
    // expected-error @+1 {{operand #0 must be memref of 8-bit signless integer or 32-bit signless integer or 16-bit float or 32-bit float or vector of 8-bit signless integer or 32-bit signless integer or 16-bit float or 32-bit float values of ranks 1 values}}
    %0 = gpu.subgroup_mma_load_matrix %src[%i, %i] {leadDimension = 4 : index} : memref<32x4xvector<4x8xf32>> -> !gpu.mma_matrix<16x16xf16, "AOp">
    return
}

// -----

#layout_map_col_major = affine_map<(i, j) -> (j, i)>

func.func @wmmaStoreOp_invalid_map(%arg0 : !gpu.mma_matrix<16x16xf16, "COp">) -> () {
    %sg = memref.alloca(){alignment = 32} : memref<32x32xf16, #layout_map_col_major, 3>
    %i = arith.constant 16 : index
    %j = arith.constant 16 : index
    // expected-error @+1 {{expected destination memref most minor dim must have unit stride}}
    gpu.subgroup_mma_store_matrix %arg0, %sg[%i,%j] {leadDimension= 32 : index} : !gpu.mma_matrix<16x16xf16, "COp">, memref<32x32xf16,#layout_map_col_major, 3>
    return
}

// -----

func.func @wmmaStoreOp_invalid_store_operand(%arg0 : !gpu.mma_matrix<16x16xf16, "AOp">) -> () {
    %sg = memref.alloca(){alignment = 32} : memref<32x32xf16, 3>
    %i = arith.constant 16 : index
    %j = arith.constant 16 : index
    // expected-error @+1 {{expected the operand matrix being stored to have 'COp' operand type}}
    gpu.subgroup_mma_store_matrix %arg0, %sg[%i,%j] {leadDimension= 32 : index} : !gpu.mma_matrix<16x16xf16, "AOp">, memref<32x32xf16, 3>
    return
}

// -----

func.func @wmmaMmaOp_invalid_operand_order(%A : !gpu.mma_matrix<16x16xf16, "AOp">, %B : !gpu.mma_matrix<16x16xf16, "BOp">, %C : !gpu.mma_matrix<16x16xf16, "COp">) -> () {
    // expected-error @+1 {{operands must be in the order AOp, BOp, COp}}
    %D = gpu.subgroup_mma_compute %B, %A, %C : !gpu.mma_matrix<16x16xf16, "BOp">, !gpu.mma_matrix<16x16xf16, "AOp"> -> !gpu.mma_matrix<16x16xf16, "COp">
    return
}

// -----

func.func @wmmaMmaOp_invalid_operand_shapes(%A : !gpu.mma_matrix<16x32xf16, "AOp">, %B : !gpu.mma_matrix<16x16xf16, "BOp">, %C : !gpu.mma_matrix<16x16xf16, "COp">) -> () {
    // expected-error @+1 {{operand shapes do not satisfy matmul constraints}}
    %D = gpu.subgroup_mma_compute %A, %B, %C : !gpu.mma_matrix<16x32xf16, "AOp">, !gpu.mma_matrix<16x16xf16, "BOp"> -> !gpu.mma_matrix<16x16xf16, "COp">
    return
}

// -----

// Number of symbol operand count less than memref symbol count.
func.func @alloc() {
   // expected-error@+1 {{symbol operand count does not equal memref symbol count}}
   %1 = gpu.alloc() : memref<2x4xf32, affine_map<(d0, d1)[s0] -> ((d0 + s0), d1)>, 1>
   return
}

// -----

// Number of symbol operand count greater than memref symbol count.
func.func @alloc() {
   %0 = arith.constant 7 : index
   // expected-error@+1 {{symbol operand count does not equal memref symbol count}}
   %1 = gpu.alloc()[%0] : memref<2x4xf32, 1>
   return
}

// -----

// Number of dynamic dimension operand count greater than memref dynamic dimension count.
func.func @alloc() {
   %0 = arith.constant 7 : index
   // expected-error@+1 {{dimension operand count does not equal memref dynamic dimension count}}
   %1 = gpu.alloc(%0, %0) : memref<2x?xf32, 1>
   return
}

// -----

// Number of dynamic dimension operand count less than memref dynamic dimension count.
func.func @alloc() {
   %0 = arith.constant 7 : index
   // expected-error@+1 {{dimension operand count does not equal memref dynamic dimension count}}
   %1 = gpu.alloc(%0) : memref<2x?x?xf32, 1>
   return
}

// -----

module attributes {gpu.container_module} {
  // expected-error@+1 {{'func.func' op gpu.known_block_size must be a dense i32 array}}
  func.func @kernel() attributes {gpu.known_block_size = 32 : i32} {
    func.return
  }
}

// -----

module attributes {gpu.container_module} {
  gpu.module @kernel {
    // expected-error@+1 {{'gpu.func' op attribute 'known_block_size' failed to satisfy constraint: i32 dense array attribute with 3 elements (if present)}}
    gpu.func @kernel() kernel attributes {known_block_size = array<i32: 2, 1>} {
      gpu.return
    }
  }
}

// -----

module {
  // expected-error@+1 {{'func.func' op gpu.known_block_size must contain exactly 3 elements}}
  func.func @kernel() attributes {gpu.known_block_size = array<i32: 2, 1>} {
    func.return
  }
}

// -----

module {
  // expected-error @+1 {{'gpu.module' op attribute 'targets' failed to satisfy constraint: array of GPU target attributes with at least 1 elements}}
  gpu.module @gpu_funcs [] {
  }
}

// -----

module {
  // expected-error @+1 {{'gpu.module' op attribute 'targets' failed to satisfy constraint: array of GPU target attributes with at least 1 elements}}
  gpu.module @gpu_funcs [1] {
  }
}

// -----

module {
  // expected-error @+1 {{'gpu.binary' op attribute 'objects' failed to satisfy constraint: an array of GPU object attributes with at least 1 elements}}
  gpu.binary @binary []
}

// -----

module {
  // expected-error @+1 {{'gpu.binary' op attribute 'offloadingHandler' failed to satisfy constraint: any attribute with the `OffloadingTranslationAttrTrait` trait.}}
  gpu.binary @binary <1> [#gpu.object<#nvvm.target, "">]
}

// -----

func.func @main() {
  %shmemSize = arith.constant 10000 : i32
  %c1 = arith.constant 1 : index
  gpu.launch blocks(%bx, %by, %bz) in (%sbx = %c1, %sby = %c1, %sbz = %c1)
             threads(%tx, %ty, %tz) in (%stx = %c1, %sty = %c1, %stz = %c1)
             dynamic_shared_memory_size %shmemSize
  {
    // expected-error @below {{'gpu.dynamic_shared_memory' op address space must be address_space<workgroup>}}
    %0 = gpu.dynamic_shared_memory : memref<?xi8>
    gpu.terminator
  }
  return
}


// -----

func.func @main() {
  %shmemSize = arith.constant 8192 : i32
  %c1 = arith.constant 1 : index
  gpu.launch blocks(%bx, %by, %bz) in (%sbx = %c1, %sby = %c1, %sbz = %c1)
             threads(%tx, %ty, %tz) in (%stx = %c1, %sty = %c1, %stz = %c1)
             dynamic_shared_memory_size %shmemSize
  {
    // expected-error @below {{'gpu.dynamic_shared_memory' op result memref type must be memref<?xi8, #gpu.address_space<workgroup>>}}
    %0 = gpu.dynamic_shared_memory : memref<1xi8, #gpu.address_space<workgroup>>
    gpu.terminator
  }
  return
}

// -----

func.func @main(%arg0 : index) {
  %shmemSize = arith.constant 8192 : i32
  %c1 = arith.constant 1 : index
  gpu.launch blocks(%bx, %by, %bz) in (%sbx = %c1, %sby = %c1, %sbz = %c1)
             threads(%tx, %ty, %tz) in (%stx = %c1, %sty = %c1, %stz = %c1)
             dynamic_shared_memory_size %shmemSize
  {
    // expected-error @below {{'gpu.dynamic_shared_memory' op address space must be address_space<workgroup>}}
    %0 = gpu.dynamic_shared_memory : memref<?xi8, #gpu.address_space<private>>
    gpu.terminator
  }
  return
}

// -----

func.func @main(%arg0 : index) {
  %shmemSize = arith.constant 8192 : i32
  %c1 = arith.constant 1 : index
  gpu.launch blocks(%bx, %by, %bz) in (%sbx = %c1, %sby = %c1, %sbz = %c1)
             threads(%tx, %ty, %tz) in (%stx = %c1, %sty = %c1, %stz = %c1)
             dynamic_shared_memory_size %shmemSize
  {
    // expected-error @below {{'gpu.dynamic_shared_memory' op result #0 must be 1D memref of 8-bit signless integer values, but got 'memref<?xf32, #gpu.address_space<workgroup>}}
    %0 = gpu.dynamic_shared_memory : memref<?xf32, #gpu.address_space<workgroup>>
    gpu.terminator
  }
  return
}

// -----

module attributes {gpu.container_module} {
  // expected-error@+1 {{expected attribute value}}
  gpu.module @kernel <> {
  }
}

// -----

gpu.binary @binary [#gpu.object<#rocdl.target<chip = "gfx900">,
  // expected-error@+1{{expected all kernels to be uniquely named}}
    kernels = #gpu.kernel_table<[
      #gpu.kernel_metadata<"kernel", (i32) -> ()>,
      #gpu.kernel_metadata<"kernel", (i32, f32) -> (), metadata = {sgpr_count = 255}>
  // expected-error@below{{failed to parse GPU_ObjectAttr parameter 'kernels' which is to be a `KernelTableAttr`}}
    ]>,
    bin = "BLOB">
  ]

// -----

func.func @warp_wrong_num_outputs(%laneid: index) {
  // expected-error@+1 {{'gpu.warp_execute_on_lane_0' op expected same number of yield operands and return values.}}
  %2 = gpu.warp_execute_on_lane_0(%laneid)[64] -> (vector<4xi32>) {
  }
  return
}

// -----

func.func @warp_wrong_num_inputs(%laneid: index) {
  // expected-error@+1 {{'gpu.warp_execute_on_lane_0' op expected same number op arguments and block arguments.}}
  gpu.warp_execute_on_lane_0(%laneid)[64] {
  ^bb0(%arg0 : vector<128xi32>) :
  }
  return
}

// -----

func.func @warp_wrong_return_distribution(%laneid: index) {
  // expected-error@+1 {{'gpu.warp_execute_on_lane_0' op incompatible distribution dimensions from 'vector<128xi32>' to 'vector<4xi32>'}}
  %2 = gpu.warp_execute_on_lane_0(%laneid)[64] -> (vector<4xi32>) {
    %0 = arith.constant dense<2>: vector<128xi32>
    gpu.yield %0 : vector<128xi32>
  }
  return
}


// -----

func.func @warp_wrong_arg_distribution(%laneid: index, %v0 : vector<4xi32>) {
  // expected-error@+1 {{'gpu.warp_execute_on_lane_0' op incompatible distribution dimensions from 'vector<128xi32>' to 'vector<4xi32>'}}
  gpu.warp_execute_on_lane_0(%laneid)[64]
  args(%v0 : vector<4xi32>) {
   ^bb0(%arg0 : vector<128xi32>) :
  }
  return
}

// -----

func.func @warp_2_distributed_dims(%laneid: index) {
  // expected-error@+1 {{incompatible distribution dimensions from 'vector<128x128xi32>' to 'vector<4x4xi32>' with warp size = 32}}
  %2 = gpu.warp_execute_on_lane_0(%laneid)[32] -> (vector<4x4xi32>) {
    %0 = arith.constant dense<2>: vector<128x128xi32>
    gpu.yield %0 : vector<128x128xi32>
  }
  return
}

// -----

func.func @warp_2_distributed_dims(%laneid: index) {
  // expected-error@+1 {{expected expanded vector dimension #1 (8) to be a multipler of the distributed vector dimension (3)}}
  %2 = gpu.warp_execute_on_lane_0(%laneid)[32] -> (vector<1x3xi32>) {
    %0 = arith.constant dense<2>: vector<4x8xi32>
    gpu.yield %0 : vector<4x8xi32>
  }
  return
}

// -----

func.func @warp_mismatch_rank(%laneid: index) {
  // expected-error@+1 {{'gpu.warp_execute_on_lane_0' op expected distributed vectors to have same rank and element type.}}
  %2 = gpu.warp_execute_on_lane_0(%laneid)[32] -> (vector<4x4xi32>) {
    %0 = arith.constant dense<2>: vector<128xi32>
    gpu.yield %0 : vector<128xi32>
  }
  return
}

// -----

func.func @warp_mismatch_rank(%laneid: index) {
  // expected-error@+1 {{'gpu.warp_execute_on_lane_0' op expected vector type for distributed operands.}}
  %2 = gpu.warp_execute_on_lane_0(%laneid)[32] -> (i32) {
    %0 = arith.constant dense<2>: vector<128xi32>
    gpu.yield %0 : vector<128xi32>
  }
  return
}
