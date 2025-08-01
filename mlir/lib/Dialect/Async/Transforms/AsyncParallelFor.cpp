//===- AsyncParallelFor.cpp - Implementation of Async Parallel For --------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements scf.parallel to scf.for + async.execute conversion pass.
//
//===----------------------------------------------------------------------===//

#include "mlir/Dialect/Async/Passes.h"

#include "PassDetail.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/Async/IR/Async.h"
#include "mlir/Dialect/Async/Transforms.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/IR/IRMapping.h"
#include "mlir/IR/Matchers.h"
#include "mlir/IR/PatternMatch.h"
#include "mlir/Support/LLVM.h"
#include "mlir/Transforms/GreedyPatternRewriteDriver.h"
#include "mlir/Transforms/RegionUtils.h"
#include <utility>

namespace mlir {
#define GEN_PASS_DEF_ASYNCPARALLELFORPASS
#include "mlir/Dialect/Async/Passes.h.inc"
} // namespace mlir

using namespace mlir;
using namespace mlir::async;

#define DEBUG_TYPE "async-parallel-for"

namespace {

// Rewrite scf.parallel operation into multiple concurrent async.execute
// operations over non overlapping subranges of the original loop.
//
// Example:
//
//   scf.parallel (%i, %j) = (%lbi, %lbj) to (%ubi, %ubj) step (%si, %sj) {
//     "do_some_compute"(%i, %j): () -> ()
//   }
//
// Converted to:
//
//   // Parallel compute function that executes the parallel body region for
//   // a subset of the parallel iteration space defined by the one-dimensional
//   // compute block index.
//   func parallel_compute_function(%block_index : index, %block_size : index,
//                                  <parallel operation properties>, ...) {
//     // Compute multi-dimensional loop bounds for %block_index.
//     %block_lbi, %block_lbj = ...
//     %block_ubi, %block_ubj = ...
//
//     // Clone parallel operation body into the scf.for loop nest.
//     scf.for %i = %blockLbi to %blockUbi {
//       scf.for %j = block_lbj to %block_ubj {
//         "do_some_compute"(%i, %j): () -> ()
//       }
//     }
//   }
//
// And a dispatch function depending on the `asyncDispatch` option.
//
// When async dispatch is on: (pseudocode)
//
//   %block_size = ... compute parallel compute block size
//   %block_count = ... compute the number of compute blocks
//
//   func @async_dispatch(%block_start : index, %block_end : index, ...) {
//     // Keep splitting block range until we reached a range of size 1.
//     while (%block_end - %block_start > 1) {
//       %mid_index = block_start + (block_end - block_start) / 2;
//       async.execute { call @async_dispatch(%mid_index, %block_end); }
//       %block_end = %mid_index
//     }
//
//     // Call parallel compute function for a single block.
//     call @parallel_compute_fn(%block_start, %block_size, ...);
//   }
//
//   // Launch async dispatch for [0, block_count) range.
//   call @async_dispatch(%c0, %block_count);
//
// When async dispatch is off:
//
//   %block_size = ... compute parallel compute block size
//   %block_count = ... compute the number of compute blocks
//
//   scf.for %block_index = %c0 to %block_count {
//      call @parallel_compute_fn(%block_index, %block_size, ...)
//   }
//
struct AsyncParallelForPass
    : public impl::AsyncParallelForPassBase<AsyncParallelForPass> {
  using Base::Base;

  void runOnOperation() override;
};

struct AsyncParallelForRewrite : public OpRewritePattern<scf::ParallelOp> {
public:
  AsyncParallelForRewrite(
      MLIRContext *ctx, bool asyncDispatch, int32_t numWorkerThreads,
      AsyncMinTaskSizeComputationFunction computeMinTaskSize)
      : OpRewritePattern(ctx), asyncDispatch(asyncDispatch),
        numWorkerThreads(numWorkerThreads),
        computeMinTaskSize(std::move(computeMinTaskSize)) {}

  LogicalResult matchAndRewrite(scf::ParallelOp op,
                                PatternRewriter &rewriter) const override;

private:
  bool asyncDispatch;
  int32_t numWorkerThreads;
  AsyncMinTaskSizeComputationFunction computeMinTaskSize;
};

struct ParallelComputeFunctionType {
  FunctionType type;
  SmallVector<Value> captures;
};

// Helper struct to parse parallel compute function argument list.
struct ParallelComputeFunctionArgs {
  BlockArgument blockIndex();
  BlockArgument blockSize();
  ArrayRef<BlockArgument> tripCounts();
  ArrayRef<BlockArgument> lowerBounds();
  ArrayRef<BlockArgument> steps();
  ArrayRef<BlockArgument> captures();

  unsigned numLoops;
  ArrayRef<BlockArgument> args;
};

struct ParallelComputeFunctionBounds {
  SmallVector<IntegerAttr> tripCounts;
  SmallVector<IntegerAttr> lowerBounds;
  SmallVector<IntegerAttr> upperBounds;
  SmallVector<IntegerAttr> steps;
};

struct ParallelComputeFunction {
  unsigned numLoops;
  func::FuncOp func;
  llvm::SmallVector<Value> captures;
};

} // namespace

BlockArgument ParallelComputeFunctionArgs::blockIndex() { return args[0]; }
BlockArgument ParallelComputeFunctionArgs::blockSize() { return args[1]; }

ArrayRef<BlockArgument> ParallelComputeFunctionArgs::tripCounts() {
  return args.drop_front(2).take_front(numLoops);
}

ArrayRef<BlockArgument> ParallelComputeFunctionArgs::lowerBounds() {
  return args.drop_front(2 + 1 * numLoops).take_front(numLoops);
}

ArrayRef<BlockArgument> ParallelComputeFunctionArgs::steps() {
  return args.drop_front(2 + 3 * numLoops).take_front(numLoops);
}

ArrayRef<BlockArgument> ParallelComputeFunctionArgs::captures() {
  return args.drop_front(2 + 4 * numLoops);
}

template <typename ValueRange>
static SmallVector<IntegerAttr> integerConstants(ValueRange values) {
  SmallVector<IntegerAttr> attrs(values.size());
  for (unsigned i = 0; i < values.size(); ++i)
    matchPattern(values[i], m_Constant(&attrs[i]));
  return attrs;
}

// Converts one-dimensional iteration index in the [0, tripCount) interval
// into multidimensional iteration coordinate.
static SmallVector<Value> delinearize(ImplicitLocOpBuilder &b, Value index,
                                      ArrayRef<Value> tripCounts) {
  SmallVector<Value> coords(tripCounts.size());
  assert(!tripCounts.empty() && "tripCounts must be not empty");

  for (ssize_t i = tripCounts.size() - 1; i >= 0; --i) {
    coords[i] = arith::RemSIOp::create(b, index, tripCounts[i]);
    index = arith::DivSIOp::create(b, index, tripCounts[i]);
  }

  return coords;
}

// Returns a function type and implicit captures for a parallel compute
// function. We'll need a list of implicit captures to setup block and value
// mapping when we'll clone the body of the parallel operation.
static ParallelComputeFunctionType
getParallelComputeFunctionType(scf::ParallelOp op, PatternRewriter &rewriter) {
  // Values implicitly captured by the parallel operation.
  llvm::SetVector<Value> captures;
  getUsedValuesDefinedAbove(op.getRegion(), op.getRegion(), captures);

  SmallVector<Type> inputs;
  inputs.reserve(2 + 4 * op.getNumLoops() + captures.size());

  Type indexTy = rewriter.getIndexType();

  // One-dimensional iteration space defined by the block index and size.
  inputs.push_back(indexTy); // blockIndex
  inputs.push_back(indexTy); // blockSize

  // Multi-dimensional parallel iteration space defined by the loop trip counts.
  for (unsigned i = 0; i < op.getNumLoops(); ++i)
    inputs.push_back(indexTy); // loop tripCount

  // Parallel operation lower bound, upper bound and step. Lower bound, upper
  // bound and step passed as contiguous arguments:
  //   call @compute(%lb0, %lb1, ..., %ub0, %ub1, ..., %step0, %step1, ...)
  for (unsigned i = 0; i < op.getNumLoops(); ++i) {
    inputs.push_back(indexTy); // lower bound
    inputs.push_back(indexTy); // upper bound
    inputs.push_back(indexTy); // step
  }

  // Types of the implicit captures.
  for (Value capture : captures)
    inputs.push_back(capture.getType());

  // Convert captures to vector for later convenience.
  SmallVector<Value> capturesVector(captures.begin(), captures.end());
  return {rewriter.getFunctionType(inputs, TypeRange()), capturesVector};
}

// Create a parallel compute fuction from the parallel operation.
static ParallelComputeFunction createParallelComputeFunction(
    scf::ParallelOp op, const ParallelComputeFunctionBounds &bounds,
    unsigned numBlockAlignedInnerLoops, PatternRewriter &rewriter) {
  OpBuilder::InsertionGuard guard(rewriter);
  ImplicitLocOpBuilder b(op.getLoc(), rewriter);

  ModuleOp module = op->getParentOfType<ModuleOp>();

  ParallelComputeFunctionType computeFuncType =
      getParallelComputeFunctionType(op, rewriter);

  FunctionType type = computeFuncType.type;
  func::FuncOp func = func::FuncOp::create(
      op.getLoc(),
      numBlockAlignedInnerLoops > 0 ? "parallel_compute_fn_with_aligned_loops"
                                    : "parallel_compute_fn",
      type);
  func.setPrivate();

  // Insert function into the module symbol table and assign it unique name.
  SymbolTable symbolTable(module);
  symbolTable.insert(func);
  rewriter.getListener()->notifyOperationInserted(func, /*previous=*/{});

  // Create function entry block.
  Block *block =
      b.createBlock(&func.getBody(), func.begin(), type.getInputs(),
                    SmallVector<Location>(type.getNumInputs(), op.getLoc()));
  b.setInsertionPointToEnd(block);

  ParallelComputeFunctionArgs args = {op.getNumLoops(), func.getArguments()};

  // Block iteration position defined by the block index and size.
  BlockArgument blockIndex = args.blockIndex();
  BlockArgument blockSize = args.blockSize();

  // Constants used below.
  Value c0 = arith::ConstantIndexOp::create(b, 0);
  Value c1 = arith::ConstantIndexOp::create(b, 1);

  // Materialize known constants as constant operation in the function body.
  auto values = [&](ArrayRef<BlockArgument> args, ArrayRef<IntegerAttr> attrs) {
    return llvm::to_vector(
        llvm::map_range(llvm::zip(args, attrs), [&](auto tuple) -> Value {
          if (IntegerAttr attr = std::get<1>(tuple))
            return arith::ConstantOp::create(b, attr);
          return std::get<0>(tuple);
        }));
  };

  // Multi-dimensional parallel iteration space defined by the loop trip counts.
  auto tripCounts = values(args.tripCounts(), bounds.tripCounts);

  // Parallel operation lower bound and step.
  auto lowerBounds = values(args.lowerBounds(), bounds.lowerBounds);
  auto steps = values(args.steps(), bounds.steps);

  // Remaining arguments are implicit captures of the parallel operation.
  ArrayRef<BlockArgument> captures = args.captures();

  // Compute a product of trip counts to get the size of the flattened
  // one-dimensional iteration space.
  Value tripCount = tripCounts[0];
  for (unsigned i = 1; i < tripCounts.size(); ++i)
    tripCount = arith::MulIOp::create(b, tripCount, tripCounts[i]);

  // Find one-dimensional iteration bounds: [blockFirstIndex, blockLastIndex]:
  //   blockFirstIndex = blockIndex * blockSize
  Value blockFirstIndex = arith::MulIOp::create(b, blockIndex, blockSize);

  // The last one-dimensional index in the block defined by the `blockIndex`:
  //   blockLastIndex = min(blockFirstIndex + blockSize, tripCount) - 1
  Value blockEnd0 = arith::AddIOp::create(b, blockFirstIndex, blockSize);
  Value blockEnd1 = arith::MinSIOp::create(b, blockEnd0, tripCount);
  Value blockLastIndex = arith::SubIOp::create(b, blockEnd1, c1);

  // Convert one-dimensional indices to multi-dimensional coordinates.
  auto blockFirstCoord = delinearize(b, blockFirstIndex, tripCounts);
  auto blockLastCoord = delinearize(b, blockLastIndex, tripCounts);

  // Compute loops upper bounds derived from the block last coordinates:
  //   blockEndCoord[i] = blockLastCoord[i] + 1
  //
  // Block first and last coordinates can be the same along the outer compute
  // dimension when inner compute dimension contains multiple blocks.
  SmallVector<Value> blockEndCoord(op.getNumLoops());
  for (size_t i = 0; i < blockLastCoord.size(); ++i)
    blockEndCoord[i] = arith::AddIOp::create(b, blockLastCoord[i], c1);

  // Construct a loop nest out of scf.for operations that will iterate over
  // all coordinates in [blockFirstCoord, blockLastCoord] range.
  using LoopBodyBuilder =
      std::function<void(OpBuilder &, Location, Value, ValueRange)>;
  using LoopNestBuilder = std::function<LoopBodyBuilder(size_t loopIdx)>;

  // Parallel region induction variables computed from the multi-dimensional
  // iteration coordinate using parallel operation bounds and step:
  //
  //   computeBlockInductionVars[loopIdx] =
  //       lowerBound[loopIdx] + blockCoord[loopIdx] * step[loopIdx]
  SmallVector<Value> computeBlockInductionVars(op.getNumLoops());

  // We need to know if we are in the first or last iteration of the
  // multi-dimensional loop for each loop in the nest, so we can decide what
  // loop bounds should we use for the nested loops: bounds defined by compute
  // block interval, or bounds defined by the parallel operation.
  //
  // Example: 2d parallel operation
  //                   i   j
  //   loop sizes:   [50, 50]
  //   first coord:  [25, 25]
  //   last coord:   [30, 30]
  //
  // If `i` is equal to 25 then iteration over `j` should start at 25, when `i`
  // is between 25 and 30 it should start at 0. The upper bound for `j` should
  // be 50, except when `i` is equal to 30, then it should also be 30.
  //
  // Value at ith position specifies if all loops in [0, i) range of the loop
  // nest are in the first/last iteration.
  SmallVector<Value> isBlockFirstCoord(op.getNumLoops());
  SmallVector<Value> isBlockLastCoord(op.getNumLoops());

  // Builds inner loop nest inside async.execute operation that does all the
  // work concurrently.
  LoopNestBuilder workLoopBuilder = [&](size_t loopIdx) -> LoopBodyBuilder {
    return [&, loopIdx](OpBuilder &nestedBuilder, Location loc, Value iv,
                        ValueRange args) {
      ImplicitLocOpBuilder b(loc, nestedBuilder);

      // Compute induction variable for `loopIdx`.
      computeBlockInductionVars[loopIdx] =
          arith::AddIOp::create(b, lowerBounds[loopIdx],
                                arith::MulIOp::create(b, iv, steps[loopIdx]));

      // Check if we are inside first or last iteration of the loop.
      isBlockFirstCoord[loopIdx] = arith::CmpIOp::create(
          b, arith::CmpIPredicate::eq, iv, blockFirstCoord[loopIdx]);
      isBlockLastCoord[loopIdx] = arith::CmpIOp::create(
          b, arith::CmpIPredicate::eq, iv, blockLastCoord[loopIdx]);

      // Check if the previous loop is in its first or last iteration.
      if (loopIdx > 0) {
        isBlockFirstCoord[loopIdx] = arith::AndIOp::create(
            b, isBlockFirstCoord[loopIdx], isBlockFirstCoord[loopIdx - 1]);
        isBlockLastCoord[loopIdx] = arith::AndIOp::create(
            b, isBlockLastCoord[loopIdx], isBlockLastCoord[loopIdx - 1]);
      }

      // Keep building loop nest.
      if (loopIdx < op.getNumLoops() - 1) {
        if (loopIdx + 1 >= op.getNumLoops() - numBlockAlignedInnerLoops) {
          // For block aligned loops we always iterate starting from 0 up to
          // the loop trip counts.
          scf::ForOp::create(b, c0, tripCounts[loopIdx + 1], c1, ValueRange(),
                             workLoopBuilder(loopIdx + 1));

        } else {
          // Select nested loop lower/upper bounds depending on our position in
          // the multi-dimensional iteration space.
          auto lb = arith::SelectOp::create(b, isBlockFirstCoord[loopIdx],
                                            blockFirstCoord[loopIdx + 1], c0);

          auto ub = arith::SelectOp::create(b, isBlockLastCoord[loopIdx],
                                            blockEndCoord[loopIdx + 1],
                                            tripCounts[loopIdx + 1]);

          scf::ForOp::create(b, lb, ub, c1, ValueRange(),
                             workLoopBuilder(loopIdx + 1));
        }

        scf::YieldOp::create(b, loc);
        return;
      }

      // Copy the body of the parallel op into the inner-most loop.
      IRMapping mapping;
      mapping.map(op.getInductionVars(), computeBlockInductionVars);
      mapping.map(computeFuncType.captures, captures);

      for (auto &bodyOp : op.getRegion().front().without_terminator())
        b.clone(bodyOp, mapping);
      scf::YieldOp::create(b, loc);
    };
  };

  scf::ForOp::create(b, blockFirstCoord[0], blockEndCoord[0], c1, ValueRange(),
                     workLoopBuilder(0));
  func::ReturnOp::create(b, ValueRange());

  return {op.getNumLoops(), func, std::move(computeFuncType.captures)};
}

// Creates recursive async dispatch function for the given parallel compute
// function. Dispatch function keeps splitting block range into halves until it
// reaches a single block, and then excecutes it inline.
//
// Function pseudocode (mix of C++ and MLIR):
//
//   func @async_dispatch(%block_start : index, %block_end : index, ...) {
//
//     // Keep splitting block range until we reached a range of size 1.
//     while (%block_end - %block_start > 1) {
//       %mid_index = block_start + (block_end - block_start) / 2;
//       async.execute { call @async_dispatch(%mid_index, %block_end); }
//       %block_end = %mid_index
//     }
//
//     // Call parallel compute function for a single block.
//     call @parallel_compute_fn(%block_start, %block_size, ...);
//   }
//
static func::FuncOp
createAsyncDispatchFunction(ParallelComputeFunction &computeFunc,
                            PatternRewriter &rewriter) {
  OpBuilder::InsertionGuard guard(rewriter);
  Location loc = computeFunc.func.getLoc();
  ImplicitLocOpBuilder b(loc, rewriter);

  ModuleOp module = computeFunc.func->getParentOfType<ModuleOp>();

  ArrayRef<Type> computeFuncInputTypes =
      computeFunc.func.getFunctionType().getInputs();

  // Compared to the parallel compute function async dispatch function takes
  // additional !async.group argument. Also instead of a single `blockIndex` it
  // takes `blockStart` and `blockEnd` arguments to define the range of
  // dispatched blocks.
  SmallVector<Type> inputTypes;
  inputTypes.push_back(async::GroupType::get(rewriter.getContext()));
  inputTypes.push_back(rewriter.getIndexType()); // add blockStart argument
  inputTypes.append(computeFuncInputTypes.begin(), computeFuncInputTypes.end());

  FunctionType type = rewriter.getFunctionType(inputTypes, TypeRange());
  func::FuncOp func = func::FuncOp::create(loc, "async_dispatch_fn", type);
  func.setPrivate();

  // Insert function into the module symbol table and assign it unique name.
  SymbolTable symbolTable(module);
  symbolTable.insert(func);
  rewriter.getListener()->notifyOperationInserted(func, /*previous=*/{});

  // Create function entry block.
  Block *block = b.createBlock(&func.getBody(), func.begin(), type.getInputs(),
                               SmallVector<Location>(type.getNumInputs(), loc));
  b.setInsertionPointToEnd(block);

  Type indexTy = b.getIndexType();
  Value c1 = arith::ConstantIndexOp::create(b, 1);
  Value c2 = arith::ConstantIndexOp::create(b, 2);

  // Get the async group that will track async dispatch completion.
  Value group = block->getArgument(0);

  // Get the block iteration range: [blockStart, blockEnd)
  Value blockStart = block->getArgument(1);
  Value blockEnd = block->getArgument(2);

  // Create a work splitting while loop for the [blockStart, blockEnd) range.
  SmallVector<Type> types = {indexTy, indexTy};
  SmallVector<Value> operands = {blockStart, blockEnd};
  SmallVector<Location> locations = {loc, loc};

  // Create a recursive dispatch loop.
  scf::WhileOp whileOp = scf::WhileOp::create(b, types, operands);
  Block *before = b.createBlock(&whileOp.getBefore(), {}, types, locations);
  Block *after = b.createBlock(&whileOp.getAfter(), {}, types, locations);

  // Setup dispatch loop condition block: decide if we need to go into the
  // `after` block and launch one more async dispatch.
  {
    b.setInsertionPointToEnd(before);
    Value start = before->getArgument(0);
    Value end = before->getArgument(1);
    Value distance = arith::SubIOp::create(b, end, start);
    Value dispatch =
        arith::CmpIOp::create(b, arith::CmpIPredicate::sgt, distance, c1);
    scf::ConditionOp::create(b, dispatch, before->getArguments());
  }

  // Setup the async dispatch loop body: recursively call dispatch function
  // for the seconds half of the original range and go to the next iteration.
  {
    b.setInsertionPointToEnd(after);
    Value start = after->getArgument(0);
    Value end = after->getArgument(1);
    Value distance = arith::SubIOp::create(b, end, start);
    Value halfDistance = arith::DivSIOp::create(b, distance, c2);
    Value midIndex = arith::AddIOp::create(b, start, halfDistance);

    // Call parallel compute function inside the async.execute region.
    auto executeBodyBuilder = [&](OpBuilder &executeBuilder,
                                  Location executeLoc, ValueRange executeArgs) {
      // Update the original `blockStart` and `blockEnd` with new range.
      SmallVector<Value> operands{block->getArguments().begin(),
                                  block->getArguments().end()};
      operands[1] = midIndex;
      operands[2] = end;

      func::CallOp::create(executeBuilder, executeLoc, func.getSymName(),
                           func.getResultTypes(), operands);
      async::YieldOp::create(executeBuilder, executeLoc, ValueRange());
    };

    // Create async.execute operation to dispatch half of the block range.
    auto execute = ExecuteOp::create(b, TypeRange(), ValueRange(), ValueRange(),
                                     executeBodyBuilder);
    AddToGroupOp::create(b, indexTy, execute.getToken(), group);
    scf::YieldOp::create(b, ValueRange({start, midIndex}));
  }

  // After dispatching async operations to process the tail of the block range
  // call the parallel compute function for the first block of the range.
  b.setInsertionPointAfter(whileOp);

  // Drop async dispatch specific arguments: async group, block start and end.
  auto forwardedInputs = block->getArguments().drop_front(3);
  SmallVector<Value> computeFuncOperands = {blockStart};
  computeFuncOperands.append(forwardedInputs.begin(), forwardedInputs.end());

  func::CallOp::create(b, computeFunc.func.getSymName(),
                       computeFunc.func.getResultTypes(), computeFuncOperands);
  func::ReturnOp::create(b, ValueRange());

  return func;
}

// Launch async dispatch of the parallel compute function.
static void doAsyncDispatch(ImplicitLocOpBuilder &b, PatternRewriter &rewriter,
                            ParallelComputeFunction &parallelComputeFunction,
                            scf::ParallelOp op, Value blockSize,
                            Value blockCount,
                            const SmallVector<Value> &tripCounts) {
  MLIRContext *ctx = op->getContext();

  // Add one more level of indirection to dispatch parallel compute functions
  // using async operations and recursive work splitting.
  func::FuncOp asyncDispatchFunction =
      createAsyncDispatchFunction(parallelComputeFunction, rewriter);

  Value c0 = arith::ConstantIndexOp::create(b, 0);
  Value c1 = arith::ConstantIndexOp::create(b, 1);

  // Appends operands shared by async dispatch and parallel compute functions to
  // the given operands vector.
  auto appendBlockComputeOperands = [&](SmallVector<Value> &operands) {
    operands.append(tripCounts);
    operands.append(op.getLowerBound().begin(), op.getLowerBound().end());
    operands.append(op.getUpperBound().begin(), op.getUpperBound().end());
    operands.append(op.getStep().begin(), op.getStep().end());
    operands.append(parallelComputeFunction.captures);
  };

  // Check if the block size is one, in this case we can skip the async dispatch
  // completely. If this will be known statically, then canonicalization will
  // erase async group operations.
  Value isSingleBlock =
      arith::CmpIOp::create(b, arith::CmpIPredicate::eq, blockCount, c1);

  auto syncDispatch = [&](OpBuilder &nestedBuilder, Location loc) {
    ImplicitLocOpBuilder b(loc, nestedBuilder);

    // Call parallel compute function for the single block.
    SmallVector<Value> operands = {c0, blockSize};
    appendBlockComputeOperands(operands);

    func::CallOp::create(b, parallelComputeFunction.func.getSymName(),
                         parallelComputeFunction.func.getResultTypes(),
                         operands);
    scf::YieldOp::create(b);
  };

  auto asyncDispatch = [&](OpBuilder &nestedBuilder, Location loc) {
    ImplicitLocOpBuilder b(loc, nestedBuilder);

    // Create an async.group to wait on all async tokens from the concurrent
    // execution of multiple parallel compute function. First block will be
    // executed synchronously in the caller thread.
    Value groupSize = arith::SubIOp::create(b, blockCount, c1);
    Value group = CreateGroupOp::create(b, GroupType::get(ctx), groupSize);

    // Launch async dispatch function for [0, blockCount) range.
    SmallVector<Value> operands = {group, c0, blockCount, blockSize};
    appendBlockComputeOperands(operands);

    func::CallOp::create(b, asyncDispatchFunction.getSymName(),
                         asyncDispatchFunction.getResultTypes(), operands);

    // Wait for the completion of all parallel compute operations.
    AwaitAllOp::create(b, group);

    scf::YieldOp::create(b);
  };

  // Dispatch either single block compute function, or launch async dispatch.
  scf::IfOp::create(b, isSingleBlock, syncDispatch, asyncDispatch);
}

// Dispatch parallel compute functions by submitting all async compute tasks
// from a simple for loop in the caller thread.
static void
doSequentialDispatch(ImplicitLocOpBuilder &b, PatternRewriter &rewriter,
                     ParallelComputeFunction &parallelComputeFunction,
                     scf::ParallelOp op, Value blockSize, Value blockCount,
                     const SmallVector<Value> &tripCounts) {
  MLIRContext *ctx = op->getContext();

  func::FuncOp compute = parallelComputeFunction.func;

  Value c0 = arith::ConstantIndexOp::create(b, 0);
  Value c1 = arith::ConstantIndexOp::create(b, 1);

  // Create an async.group to wait on all async tokens from the concurrent
  // execution of multiple parallel compute function. First block will be
  // executed synchronously in the caller thread.
  Value groupSize = arith::SubIOp::create(b, blockCount, c1);
  Value group = CreateGroupOp::create(b, GroupType::get(ctx), groupSize);

  // Call parallel compute function for all blocks.
  using LoopBodyBuilder =
      std::function<void(OpBuilder &, Location, Value, ValueRange)>;

  // Returns parallel compute function operands to process the given block.
  auto computeFuncOperands = [&](Value blockIndex) -> SmallVector<Value> {
    SmallVector<Value> computeFuncOperands = {blockIndex, blockSize};
    computeFuncOperands.append(tripCounts);
    computeFuncOperands.append(op.getLowerBound().begin(),
                               op.getLowerBound().end());
    computeFuncOperands.append(op.getUpperBound().begin(),
                               op.getUpperBound().end());
    computeFuncOperands.append(op.getStep().begin(), op.getStep().end());
    computeFuncOperands.append(parallelComputeFunction.captures);
    return computeFuncOperands;
  };

  // Induction variable is the index of the block: [0, blockCount).
  LoopBodyBuilder loopBuilder = [&](OpBuilder &loopBuilder, Location loc,
                                    Value iv, ValueRange args) {
    ImplicitLocOpBuilder b(loc, loopBuilder);

    // Call parallel compute function inside the async.execute region.
    auto executeBodyBuilder = [&](OpBuilder &executeBuilder,
                                  Location executeLoc, ValueRange executeArgs) {
      func::CallOp::create(executeBuilder, executeLoc, compute.getSymName(),
                           compute.getResultTypes(), computeFuncOperands(iv));
      async::YieldOp::create(executeBuilder, executeLoc, ValueRange());
    };

    // Create async.execute operation to launch parallel computate function.
    auto execute = ExecuteOp::create(b, TypeRange(), ValueRange(), ValueRange(),
                                     executeBodyBuilder);
    AddToGroupOp::create(b, rewriter.getIndexType(), execute.getToken(), group);
    scf::YieldOp::create(b);
  };

  // Iterate over all compute blocks and launch parallel compute operations.
  scf::ForOp::create(b, c1, blockCount, c1, ValueRange(), loopBuilder);

  // Call parallel compute function for the first block in the caller thread.
  func::CallOp::create(b, compute.getSymName(), compute.getResultTypes(),
                       computeFuncOperands(c0));

  // Wait for the completion of all async compute operations.
  AwaitAllOp::create(b, group);
}

LogicalResult
AsyncParallelForRewrite::matchAndRewrite(scf::ParallelOp op,
                                         PatternRewriter &rewriter) const {
  // We do not currently support rewrite for parallel op with reductions.
  if (op.getNumReductions() != 0)
    return failure();

  ImplicitLocOpBuilder b(op.getLoc(), rewriter);

  // Computing minTaskSize emits IR and can be implemented as executing a cost
  // model on the body of the scf.parallel. Thus it needs to be computed before
  // the body of the scf.parallel has been manipulated.
  Value minTaskSize = computeMinTaskSize(b, op);

  // Make sure that all constants will be inside the parallel operation body to
  // reduce the number of parallel compute function arguments.
  cloneConstantsIntoTheRegion(op.getRegion(), rewriter);

  // Compute trip count for each loop induction variable:
  //   tripCount = ceil_div(upperBound - lowerBound, step);
  SmallVector<Value> tripCounts(op.getNumLoops());
  for (size_t i = 0; i < op.getNumLoops(); ++i) {
    auto lb = op.getLowerBound()[i];
    auto ub = op.getUpperBound()[i];
    auto step = op.getStep()[i];
    auto range = b.createOrFold<arith::SubIOp>(ub, lb);
    tripCounts[i] = b.createOrFold<arith::CeilDivSIOp>(range, step);
  }

  // Compute a product of trip counts to get the 1-dimensional iteration space
  // for the scf.parallel operation.
  Value tripCount = tripCounts[0];
  for (size_t i = 1; i < tripCounts.size(); ++i)
    tripCount = arith::MulIOp::create(b, tripCount, tripCounts[i]);

  // Short circuit no-op parallel loops (zero iterations) that can arise from
  // the memrefs with dynamic dimension(s) equal to zero.
  Value c0 = arith::ConstantIndexOp::create(b, 0);
  Value isZeroIterations =
      arith::CmpIOp::create(b, arith::CmpIPredicate::eq, tripCount, c0);

  // Do absolutely nothing if the trip count is zero.
  auto noOp = [&](OpBuilder &nestedBuilder, Location loc) {
    scf::YieldOp::create(nestedBuilder, loc);
  };

  // Compute the parallel block size and dispatch concurrent tasks computing
  // results for each block.
  auto dispatch = [&](OpBuilder &nestedBuilder, Location loc) {
    ImplicitLocOpBuilder b(loc, nestedBuilder);

    // Collect statically known constants defining the loop nest in the parallel
    // compute function. LLVM can't always push constants across the non-trivial
    // async dispatch call graph, by providing these values explicitly we can
    // choose to build more efficient loop nest, and rely on a better constant
    // folding, loop unrolling and vectorization.
    ParallelComputeFunctionBounds staticBounds = {
        integerConstants(tripCounts),
        integerConstants(op.getLowerBound()),
        integerConstants(op.getUpperBound()),
        integerConstants(op.getStep()),
    };

    // Find how many inner iteration dimensions are statically known, and their
    // product is smaller than the `512`. We align the parallel compute block
    // size by the product of statically known dimensions, so that we can
    // guarantee that the inner loops executes from 0 to the loop trip counts
    // and we can elide dynamic loop boundaries, and give LLVM an opportunity to
    // unroll the loops. The constant `512` is arbitrary, it should depend on
    // how many iterations LLVM will typically decide to unroll.
    static constexpr int64_t maxUnrollableIterations = 512;

    // The number of inner loops with statically known number of iterations less
    // than the `maxUnrollableIterations` value.
    int numUnrollableLoops = 0;

    auto getInt = [](IntegerAttr attr) { return attr ? attr.getInt() : 0; };

    SmallVector<int64_t> numIterations(op.getNumLoops());
    numIterations.back() = getInt(staticBounds.tripCounts.back());

    for (int i = op.getNumLoops() - 2; i >= 0; --i) {
      int64_t tripCount = getInt(staticBounds.tripCounts[i]);
      int64_t innerIterations = numIterations[i + 1];
      numIterations[i] = tripCount * innerIterations;

      // Update the number of inner loops that we can potentially unroll.
      if (innerIterations > 0 && innerIterations <= maxUnrollableIterations)
        numUnrollableLoops++;
    }

    Value numWorkerThreadsVal;
    if (numWorkerThreads >= 0)
      numWorkerThreadsVal = arith::ConstantIndexOp::create(b, numWorkerThreads);
    else
      numWorkerThreadsVal = async::RuntimeNumWorkerThreadsOp::create(b);

    // With large number of threads the value of creating many compute blocks
    // is reduced because the problem typically becomes memory bound. For this
    // reason we scale the number of workers using an equivalent to the
    // following logic:
    //   float overshardingFactor = numWorkerThreads <= 4    ? 8.0
    //                              : numWorkerThreads <= 8  ? 4.0
    //                              : numWorkerThreads <= 16 ? 2.0
    //                              : numWorkerThreads <= 32 ? 1.0
    //                              : numWorkerThreads <= 64 ? 0.8
    //                                                       : 0.6;

    // Pairs of non-inclusive lower end of the bracket and factor that the
    // number of workers needs to be scaled with if it falls in that bucket.
    const SmallVector<std::pair<int, float>> overshardingBrackets = {
        {4, 4.0f}, {8, 2.0f}, {16, 1.0f}, {32, 0.8f}, {64, 0.6f}};
    const float initialOvershardingFactor = 8.0f;

    Value scalingFactor = arith::ConstantFloatOp::create(
        b, b.getF32Type(), llvm::APFloat(initialOvershardingFactor));
    for (const std::pair<int, float> &p : overshardingBrackets) {
      Value bracketBegin = arith::ConstantIndexOp::create(b, p.first);
      Value inBracket = arith::CmpIOp::create(
          b, arith::CmpIPredicate::sgt, numWorkerThreadsVal, bracketBegin);
      Value bracketScalingFactor = arith::ConstantFloatOp::create(
          b, b.getF32Type(), llvm::APFloat(p.second));
      scalingFactor = arith::SelectOp::create(
          b, inBracket, bracketScalingFactor, scalingFactor);
    }
    Value numWorkersIndex =
        arith::IndexCastOp::create(b, b.getI32Type(), numWorkerThreadsVal);
    Value numWorkersFloat =
        arith::SIToFPOp::create(b, b.getF32Type(), numWorkersIndex);
    Value scaledNumWorkers =
        arith::MulFOp::create(b, scalingFactor, numWorkersFloat);
    Value scaledNumInt =
        arith::FPToSIOp::create(b, b.getI32Type(), scaledNumWorkers);
    Value scaledWorkers =
        arith::IndexCastOp::create(b, b.getIndexType(), scaledNumInt);

    Value maxComputeBlocks = arith::MaxSIOp::create(
        b, arith::ConstantIndexOp::create(b, 1), scaledWorkers);

    // Compute parallel block size from the parallel problem size:
    //   blockSize = min(tripCount,
    //                   max(ceil_div(tripCount, maxComputeBlocks),
    //                       minTaskSize))
    Value bs0 = arith::CeilDivSIOp::create(b, tripCount, maxComputeBlocks);
    Value bs1 = arith::MaxSIOp::create(b, bs0, minTaskSize);
    Value blockSize = arith::MinSIOp::create(b, tripCount, bs1);

    // Dispatch parallel compute function using async recursive work splitting,
    // or by submitting compute task sequentially from a caller thread.
    auto doDispatch = asyncDispatch ? doAsyncDispatch : doSequentialDispatch;

    // Create a parallel compute function that takes a block id and computes
    // the parallel operation body for a subset of iteration space.

    // Compute the number of parallel compute blocks.
    Value blockCount = arith::CeilDivSIOp::create(b, tripCount, blockSize);

    // Dispatch parallel compute function without hints to unroll inner loops.
    auto dispatchDefault = [&](OpBuilder &nestedBuilder, Location loc) {
      ParallelComputeFunction compute =
          createParallelComputeFunction(op, staticBounds, 0, rewriter);

      ImplicitLocOpBuilder b(loc, nestedBuilder);
      doDispatch(b, rewriter, compute, op, blockSize, blockCount, tripCounts);
      scf::YieldOp::create(b);
    };

    // Dispatch parallel compute function with hints for unrolling inner loops.
    auto dispatchBlockAligned = [&](OpBuilder &nestedBuilder, Location loc) {
      ParallelComputeFunction compute = createParallelComputeFunction(
          op, staticBounds, numUnrollableLoops, rewriter);

      ImplicitLocOpBuilder b(loc, nestedBuilder);
      // Align the block size to be a multiple of the statically known
      // number of iterations in the inner loops.
      Value numIters = arith::ConstantIndexOp::create(
          b, numIterations[op.getNumLoops() - numUnrollableLoops]);
      Value alignedBlockSize = arith::MulIOp::create(
          b, arith::CeilDivSIOp::create(b, blockSize, numIters), numIters);
      doDispatch(b, rewriter, compute, op, alignedBlockSize, blockCount,
                 tripCounts);
      scf::YieldOp::create(b);
    };

    // Dispatch to block aligned compute function only if the computed block
    // size is larger than the number of iterations in the unrollable inner
    // loops, because otherwise it can reduce the available parallelism.
    if (numUnrollableLoops > 0) {
      Value numIters = arith::ConstantIndexOp::create(
          b, numIterations[op.getNumLoops() - numUnrollableLoops]);
      Value useBlockAlignedComputeFn = arith::CmpIOp::create(
          b, arith::CmpIPredicate::sge, blockSize, numIters);

      scf::IfOp::create(b, useBlockAlignedComputeFn, dispatchBlockAligned,
                        dispatchDefault);
      scf::YieldOp::create(b);
    } else {
      dispatchDefault(b, loc);
    }
  };

  // Replace the `scf.parallel` operation with the parallel compute function.
  scf::IfOp::create(b, isZeroIterations, noOp, dispatch);

  // Parallel operation was replaced with a block iteration loop.
  rewriter.eraseOp(op);

  return success();
}

void AsyncParallelForPass::runOnOperation() {
  MLIRContext *ctx = &getContext();

  RewritePatternSet patterns(ctx);
  populateAsyncParallelForPatterns(
      patterns, asyncDispatch, numWorkerThreads,
      [&](ImplicitLocOpBuilder builder, scf::ParallelOp op) {
        return arith::ConstantIndexOp::create(builder, minTaskSize);
      });
  if (failed(applyPatternsGreedily(getOperation(), std::move(patterns))))
    signalPassFailure();
}

void mlir::async::populateAsyncParallelForPatterns(
    RewritePatternSet &patterns, bool asyncDispatch, int32_t numWorkerThreads,
    const AsyncMinTaskSizeComputationFunction &computeMinTaskSize) {
  MLIRContext *ctx = patterns.getContext();
  patterns.add<AsyncParallelForRewrite>(ctx, asyncDispatch, numWorkerThreads,
                                        computeMinTaskSize);
}
