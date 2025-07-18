//===-- primary64.h ---------------------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef SCUDO_PRIMARY64_H_
#define SCUDO_PRIMARY64_H_

#include "allocator_common.h"
#include "bytemap.h"
#include "common.h"
#include "condition_variable.h"
#include "list.h"
#include "mem_map.h"
#include "memtag.h"
#include "options.h"
#include "release.h"
#include "size_class_allocator.h"
#include "stats.h"
#include "string_utils.h"
#include "thread_annotations.h"

namespace scudo {

// SizeClassAllocator64 is an allocator tuned for 64-bit address space.
//
// It starts by reserving NumClasses * 2^RegionSizeLog bytes, equally divided in
// Regions, specific to each size class. Note that the base of that mapping is
// random (based to the platform specific map() capabilities). If
// PrimaryEnableRandomOffset is set, each Region actually starts at a random
// offset from its base.
//
// Regions are mapped incrementally on demand to fulfill allocation requests,
// those mappings being split into equally sized Blocks based on the size class
// they belong to. The Blocks created are shuffled to prevent predictable
// address patterns (the predictability increases with the size of the Blocks).
//
// The 1st Region (for size class 0) holds the Batches. This is a
// structure used to transfer arrays of available pointers from the class size
// freelist to the thread specific freelist, and back.
//
// The memory used by this allocator is never unmapped, but can be partially
// released if the platform allows for it.

template <typename Config> class SizeClassAllocator64 {
public:
  typedef typename Config::CompactPtrT CompactPtrT;
  typedef typename Config::SizeClassMap SizeClassMap;
  typedef typename Config::ConditionVariableT ConditionVariableT;
  static const uptr CompactPtrScale = Config::getCompactPtrScale();
  static const uptr RegionSizeLog = Config::getRegionSizeLog();
  static const uptr GroupSizeLog = Config::getGroupSizeLog();
  static_assert(RegionSizeLog >= GroupSizeLog,
                "Group size shouldn't be greater than the region size");
  static const uptr GroupScale = GroupSizeLog - CompactPtrScale;
  typedef SizeClassAllocator64<Config> ThisT;
  typedef Batch<ThisT> BatchT;
  typedef BatchGroup<ThisT> BatchGroupT;
  using SizeClassAllocatorT =
      typename Conditional<Config::getEnableBlockCache(),
                           SizeClassAllocatorLocalCache<ThisT>,
                           SizeClassAllocatorNoCache<ThisT>>::type;
  static const u16 MaxNumBlocksInBatch = SizeClassMap::MaxNumCachedHint;

  static constexpr uptr getSizeOfBatchClass() {
    const uptr HeaderSize = sizeof(BatchT);
    return roundUp(HeaderSize + sizeof(CompactPtrT) * MaxNumBlocksInBatch,
                   1 << CompactPtrScale);
  }

  static_assert(sizeof(BatchGroupT) <= getSizeOfBatchClass(),
                "BatchGroupT also uses BatchClass");

  // BachClass is used to store internal metadata so it needs to be at least as
  // large as the largest data structure.
  static uptr getSizeByClassId(uptr ClassId) {
    return (ClassId == SizeClassMap::BatchClassId)
               ? getSizeOfBatchClass()
               : SizeClassMap::getSizeByClassId(ClassId);
  }

  static bool canAllocate(uptr Size) { return Size <= SizeClassMap::MaxSize; }
  static bool conditionVariableEnabled() {
    return Config::hasConditionVariableT();
  }
  static uptr getRegionInfoArraySize() { return sizeof(RegionInfoArray); }
  static BlockInfo findNearestBlock(const char *RegionInfoData,
                                    uptr Ptr) NO_THREAD_SAFETY_ANALYSIS;

  void init(s32 ReleaseToOsInterval) NO_THREAD_SAFETY_ANALYSIS;

  void unmapTestOnly();

  // When all blocks are freed, it has to be the same size as `AllocatedUser`.
  void verifyAllBlocksAreReleasedTestOnly();

  u16 popBlocks(SizeClassAllocatorT *SizeClassAllocator, uptr ClassId,
                CompactPtrT *ToArray, const u16 MaxBlockCount);

  // Push the array of free blocks to the designated batch group.
  void pushBlocks(SizeClassAllocatorT *SizeClassAllocator, uptr ClassId,
                  CompactPtrT *Array, u32 Size);

  void disable() NO_THREAD_SAFETY_ANALYSIS;
  void enable() NO_THREAD_SAFETY_ANALYSIS;

  template <typename F> void iterateOverBlocks(F Callback);

  void getStats(ScopedString *Str);
  void getFragmentationInfo(ScopedString *Str);
  void getMemoryGroupFragmentationInfo(ScopedString *Str);

  bool setOption(Option O, sptr Value);

  // These are used for returning unused pages. Note that it doesn't unmap the
  // pages, it only suggests that the physical pages can be released.
  uptr tryReleaseToOS(uptr ClassId, ReleaseToOS ReleaseType);
  uptr releaseToOS(ReleaseToOS ReleaseType);

  const char *getRegionInfoArrayAddress() const {
    return reinterpret_cast<const char *>(RegionInfoArray);
  }

  uptr getCompactPtrBaseByClassId(uptr ClassId) {
    return getRegionInfo(ClassId)->RegionBeg;
  }
  CompactPtrT compactPtr(uptr ClassId, uptr Ptr) {
    DCHECK_LE(ClassId, SizeClassMap::LargestClassId);
    return compactPtrInternal(getCompactPtrBaseByClassId(ClassId), Ptr);
  }
  void *decompactPtr(uptr ClassId, CompactPtrT CompactPtr) {
    DCHECK_LE(ClassId, SizeClassMap::LargestClassId);
    return reinterpret_cast<void *>(
        decompactPtrInternal(getCompactPtrBaseByClassId(ClassId), CompactPtr));
  }

  AtomicOptions Options;

private:
  static const uptr RegionSize = 1UL << RegionSizeLog;
  static const uptr NumClasses = SizeClassMap::NumClasses;
  static const uptr MapSizeIncrement = Config::getMapSizeIncrement();
  // Fill at most this number of batches from the newly map'd memory.
  static const u32 MaxNumBatches = SCUDO_ANDROID ? 4U : 8U;

  struct ReleaseToOsInfo {
    uptr BytesInFreeListAtLastCheckpoint;
    uptr NumReleasesAttempted;
    uptr LastReleasedBytes;
    // The minimum size of pushed blocks to trigger page release.
    uptr TryReleaseThreshold;
    // The number of bytes not triggering `releaseToOSMaybe()` because of
    // the length of release interval.
    uptr PendingPushedBytesDelta;
    u64 LastReleaseAtNs;
  };

  struct BlocksInfo {
    SinglyLinkedList<BatchGroupT> BlockList = {};
    uptr PoppedBlocks = 0;
    uptr PushedBlocks = 0;
  };

  struct PagesInfo {
    MemMapT MemMap = {};
    // Bytes mapped for user memory.
    uptr MappedUser = 0;
    // Bytes allocated for user memory.
    uptr AllocatedUser = 0;
  };

  struct UnpaddedRegionInfo {
    // Mutex for operations on freelist
    HybridMutex FLLock;
    ConditionVariableT FLLockCV GUARDED_BY(FLLock);
    // Mutex for memmap operations
    HybridMutex MMLock ACQUIRED_BEFORE(FLLock);
    // `RegionBeg` is initialized before thread creation and won't be changed.
    uptr RegionBeg = 0;
    u32 RandState GUARDED_BY(MMLock) = 0;
    BlocksInfo FreeListInfo GUARDED_BY(FLLock);
    PagesInfo MemMapInfo GUARDED_BY(MMLock);
    ReleaseToOsInfo ReleaseInfo GUARDED_BY(MMLock) = {};
    bool Exhausted GUARDED_BY(MMLock) = false;
    bool isPopulatingFreeList GUARDED_BY(FLLock) = false;
  };
  struct RegionInfo : UnpaddedRegionInfo {
    char Padding[SCUDO_CACHE_LINE_SIZE -
                 (sizeof(UnpaddedRegionInfo) % SCUDO_CACHE_LINE_SIZE)] = {};
  };
  static_assert(sizeof(RegionInfo) % SCUDO_CACHE_LINE_SIZE == 0, "");

  RegionInfo *getRegionInfo(uptr ClassId) {
    DCHECK_LT(ClassId, NumClasses);
    return &RegionInfoArray[ClassId];
  }

  uptr getRegionBaseByClassId(uptr ClassId) {
    RegionInfo *Region = getRegionInfo(ClassId);
    Region->MMLock.assertHeld();

    if (!Config::getEnableContiguousRegions() &&
        !Region->MemMapInfo.MemMap.isAllocated()) {
      return 0U;
    }
    return Region->MemMapInfo.MemMap.getBase();
  }

  CompactPtrT compactPtrInternal(uptr Base, uptr Ptr) const {
    return static_cast<CompactPtrT>((Ptr - Base) >> CompactPtrScale);
  }
  uptr decompactPtrInternal(uptr Base, CompactPtrT CompactPtr) const {
    return Base + (static_cast<uptr>(CompactPtr) << CompactPtrScale);
  }
  uptr compactPtrGroup(CompactPtrT CompactPtr) const {
    const uptr Mask = (static_cast<uptr>(1) << GroupScale) - 1;
    return static_cast<uptr>(CompactPtr) & ~Mask;
  }
  uptr decompactGroupBase(uptr Base, uptr CompactPtrGroupBase) const {
    DCHECK_EQ(CompactPtrGroupBase % (static_cast<uptr>(1) << (GroupScale)), 0U);
    return Base + (CompactPtrGroupBase << CompactPtrScale);
  }
  ALWAYS_INLINE bool isSmallBlock(uptr BlockSize) const {
    const uptr PageSize = getPageSizeCached();
    return BlockSize < PageSize / 16U;
  }
  ALWAYS_INLINE uptr getMinReleaseAttemptSize(uptr BlockSize) {
    return roundUp(BlockSize, getPageSizeCached());
  }

  ALWAYS_INLINE void initRegion(RegionInfo *Region, uptr ClassId,
                                MemMapT MemMap, bool EnableRandomOffset)
      REQUIRES(Region->MMLock);

  void pushBlocksImpl(SizeClassAllocatorT *SizeClassAllocator, uptr ClassId,
                      RegionInfo *Region, CompactPtrT *Array, u32 Size,
                      bool SameGroup = false) REQUIRES(Region->FLLock);

  // Similar to `pushBlocksImpl` but has some logics specific to BatchClass.
  void pushBatchClassBlocks(RegionInfo *Region, CompactPtrT *Array, u32 Size)
      REQUIRES(Region->FLLock);

  // Pop at most `MaxBlockCount` from the freelist of the given region.
  u16 popBlocksImpl(SizeClassAllocatorT *SizeClassAllocator, uptr ClassId,
                    RegionInfo *Region, CompactPtrT *ToArray,
                    const u16 MaxBlockCount) REQUIRES(Region->FLLock);
  // Same as `popBlocksImpl` but is used when conditional variable is enabled.
  u16 popBlocksWithCV(SizeClassAllocatorT *SizeClassAllocator, uptr ClassId,
                      RegionInfo *Region, CompactPtrT *ToArray,
                      const u16 MaxBlockCount, bool &ReportRegionExhausted);

  // When there's no blocks available in the freelist, it tries to prepare more
  // blocks by mapping more pages.
  NOINLINE u16 populateFreeListAndPopBlocks(
      SizeClassAllocatorT *SizeClassAllocator, uptr ClassId, RegionInfo *Region,
      CompactPtrT *ToArray, const u16 MaxBlockCount) REQUIRES(Region->MMLock)
      EXCLUDES(Region->FLLock);

  void getStats(ScopedString *Str, uptr ClassId, RegionInfo *Region)
      REQUIRES(Region->MMLock, Region->FLLock);
  void getRegionFragmentationInfo(RegionInfo *Region, uptr ClassId,
                                  ScopedString *Str) REQUIRES(Region->MMLock);
  void getMemoryGroupFragmentationInfoInRegion(RegionInfo *Region, uptr ClassId,
                                               ScopedString *Str)
      REQUIRES(Region->MMLock) EXCLUDES(Region->FLLock);

  NOINLINE uptr releaseToOSMaybe(RegionInfo *Region, uptr ClassId,
                                 ReleaseToOS ReleaseType = ReleaseToOS::Normal)
      REQUIRES(Region->MMLock) EXCLUDES(Region->FLLock);
  bool hasChanceToReleasePages(RegionInfo *Region, uptr BlockSize,
                               uptr BytesInFreeList, ReleaseToOS ReleaseType)
      REQUIRES(Region->MMLock, Region->FLLock);
  SinglyLinkedList<BatchGroupT>
  collectGroupsToRelease(RegionInfo *Region, const uptr BlockSize,
                         const uptr AllocatedUserEnd, const uptr CompactPtrBase)
      REQUIRES(Region->MMLock, Region->FLLock);
  PageReleaseContext
  markFreeBlocks(RegionInfo *Region, const uptr BlockSize,
                 const uptr AllocatedUserEnd, const uptr CompactPtrBase,
                 SinglyLinkedList<BatchGroupT> &GroupsToRelease)
      REQUIRES(Region->MMLock) EXCLUDES(Region->FLLock);

  void mergeGroupsToReleaseBack(RegionInfo *Region,
                                SinglyLinkedList<BatchGroupT> &GroupsToRelease)
      REQUIRES(Region->MMLock) EXCLUDES(Region->FLLock);

  // The minimum size of pushed blocks that we will try to release the pages in
  // that size class.
  uptr SmallerBlockReleasePageDelta = 0;
  atomic_s32 ReleaseToOsIntervalMs = {};
  alignas(SCUDO_CACHE_LINE_SIZE) RegionInfo RegionInfoArray[NumClasses];
};

template <typename Config>
void SizeClassAllocator64<Config>::init(s32 ReleaseToOsInterval)
    NO_THREAD_SAFETY_ANALYSIS {
  DCHECK(isAligned(reinterpret_cast<uptr>(this), alignof(ThisT)));

  const uptr PageSize = getPageSizeCached();
  const uptr GroupSize = (1UL << GroupSizeLog);
  const uptr PagesInGroup = GroupSize / PageSize;
  const uptr MinSizeClass = getSizeByClassId(1);
  // When trying to release pages back to memory, visiting smaller size
  // classes is expensive. Therefore, we only try to release smaller size
  // classes when the amount of free blocks goes over a certain threshold (See
  // the comment in releaseToOSMaybe() for more details). For example, for
  // size class 32, we only do the release when the size of free blocks is
  // greater than 97% of pages in a group. However, this may introduce another
  // issue that if the number of free blocks is bouncing between 97% ~ 100%.
  // Which means we may try many page releases but only release very few of
  // them (less than 3% in a group). Even though we have
  // `&ReleaseToOsIntervalMs` which slightly reduce the frequency of these
  // calls but it will be better to have another guard to mitigate this issue.
  //
  // Here we add another constraint on the minimum size requirement. The
  // constraint is determined by the size of in-use blocks in the minimal size
  // class. Take size class 32 as an example,
  //
  //   +-     one memory group      -+
  //   +----------------------+------+
  //   |  97% of free blocks  |      |
  //   +----------------------+------+
  //                           \    /
  //                      3% in-use blocks
  //
  //   * The release size threshold is 97%.
  //
  // The 3% size in a group is about 7 pages. For two consecutive
  // releaseToOSMaybe(), we require the difference between `PushedBlocks`
  // should be greater than 7 pages. This mitigates the page releasing
  // thrashing which is caused by memory usage bouncing around the threshold.
  // The smallest size class takes longest time to do the page release so we
  // use its size of in-use blocks as a heuristic.
  SmallerBlockReleasePageDelta = PagesInGroup * (1 + MinSizeClass / 16U) / 100;

  u32 Seed;
  const u64 Time = getMonotonicTimeFast();
  if (!getRandom(reinterpret_cast<void *>(&Seed), sizeof(Seed)))
    Seed = static_cast<u32>(Time ^ (reinterpret_cast<uptr>(&Seed) >> 12));

  for (uptr I = 0; I < NumClasses; I++)
    getRegionInfo(I)->RandState = getRandomU32(&Seed);

  if (Config::getEnableContiguousRegions()) {
    ReservedMemoryT ReservedMemory = {};
    // Reserve the space required for the Primary.
    CHECK(ReservedMemory.create(/*Addr=*/0U, RegionSize * NumClasses,
                                "scudo:primary_reserve"));
    const uptr PrimaryBase = ReservedMemory.getBase();

    for (uptr I = 0; I < NumClasses; I++) {
      MemMapT RegionMemMap = ReservedMemory.dispatch(
          PrimaryBase + (I << RegionSizeLog), RegionSize);
      RegionInfo *Region = getRegionInfo(I);

      initRegion(Region, I, RegionMemMap, Config::getEnableRandomOffset());
    }
    shuffle(RegionInfoArray, NumClasses, &Seed);
  }

  // The binding should be done after region shuffling so that it won't bind
  // the FLLock from the wrong region.
  for (uptr I = 0; I < NumClasses; I++)
    getRegionInfo(I)->FLLockCV.bindTestOnly(getRegionInfo(I)->FLLock);

  // The default value in the primary config has the higher priority.
  if (Config::getDefaultReleaseToOsIntervalMs() != INT32_MIN)
    ReleaseToOsInterval = Config::getDefaultReleaseToOsIntervalMs();
  setOption(Option::ReleaseInterval, static_cast<sptr>(ReleaseToOsInterval));
}

template <typename Config>
void SizeClassAllocator64<Config>::initRegion(RegionInfo *Region, uptr ClassId,
                                              MemMapT MemMap,
                                              bool EnableRandomOffset)
    REQUIRES(Region->MMLock) {
  DCHECK(!Region->MemMapInfo.MemMap.isAllocated());
  DCHECK(MemMap.isAllocated());

  const uptr PageSize = getPageSizeCached();

  Region->MemMapInfo.MemMap = MemMap;

  Region->RegionBeg = MemMap.getBase();
  if (EnableRandomOffset) {
    Region->RegionBeg += (getRandomModN(&Region->RandState, 16) + 1) * PageSize;
  }

  const uptr BlockSize = getSizeByClassId(ClassId);
  // Releasing small blocks is expensive, set a higher threshold to avoid
  // frequent page releases.
  if (isSmallBlock(BlockSize)) {
    Region->ReleaseInfo.TryReleaseThreshold =
        PageSize * SmallerBlockReleasePageDelta;
  } else {
    Region->ReleaseInfo.TryReleaseThreshold =
        getMinReleaseAttemptSize(BlockSize);
  }
}

template <typename Config> void SizeClassAllocator64<Config>::unmapTestOnly() {
  for (uptr I = 0; I < NumClasses; I++) {
    RegionInfo *Region = getRegionInfo(I);
    {
      ScopedLock ML(Region->MMLock);
      MemMapT MemMap = Region->MemMapInfo.MemMap;
      if (MemMap.isAllocated())
        MemMap.unmap();
    }
    *Region = {};
  }
}

template <typename Config>
void SizeClassAllocator64<Config>::verifyAllBlocksAreReleasedTestOnly() {
  // `BatchGroup` and `Batch` also use the blocks from BatchClass.
  uptr BatchClassUsedInFreeLists = 0;
  for (uptr I = 0; I < NumClasses; I++) {
    // We have to count BatchClassUsedInFreeLists in other regions first.
    if (I == SizeClassMap::BatchClassId)
      continue;
    RegionInfo *Region = getRegionInfo(I);
    ScopedLock ML(Region->MMLock);
    ScopedLock FL(Region->FLLock);
    const uptr BlockSize = getSizeByClassId(I);
    uptr TotalBlocks = 0;
    for (BatchGroupT &BG : Region->FreeListInfo.BlockList) {
      // `BG::Batches` are `Batches`. +1 for `BatchGroup`.
      BatchClassUsedInFreeLists += BG.Batches.size() + 1;
      for (const auto &It : BG.Batches)
        TotalBlocks += It.getCount();
    }

    DCHECK_EQ(TotalBlocks, Region->MemMapInfo.AllocatedUser / BlockSize);
    DCHECK_EQ(Region->FreeListInfo.PushedBlocks,
              Region->FreeListInfo.PoppedBlocks);
  }

  RegionInfo *Region = getRegionInfo(SizeClassMap::BatchClassId);
  ScopedLock ML(Region->MMLock);
  ScopedLock FL(Region->FLLock);
  const uptr BlockSize = getSizeByClassId(SizeClassMap::BatchClassId);
  uptr TotalBlocks = 0;
  for (BatchGroupT &BG : Region->FreeListInfo.BlockList) {
    if (LIKELY(!BG.Batches.empty())) {
      for (const auto &It : BG.Batches)
        TotalBlocks += It.getCount();
    } else {
      // `BatchGroup` with empty freelist doesn't have `Batch` record
      // itself.
      ++TotalBlocks;
    }
  }
  DCHECK_EQ(TotalBlocks + BatchClassUsedInFreeLists,
            Region->MemMapInfo.AllocatedUser / BlockSize);
  DCHECK_GE(Region->FreeListInfo.PoppedBlocks,
            Region->FreeListInfo.PushedBlocks);
  const uptr BlocksInUse =
      Region->FreeListInfo.PoppedBlocks - Region->FreeListInfo.PushedBlocks;
  DCHECK_EQ(BlocksInUse, BatchClassUsedInFreeLists);
}

template <typename Config>
u16 SizeClassAllocator64<Config>::popBlocks(
    SizeClassAllocatorT *SizeClassAllocator, uptr ClassId, CompactPtrT *ToArray,
    const u16 MaxBlockCount) {
  DCHECK_LT(ClassId, NumClasses);
  RegionInfo *Region = getRegionInfo(ClassId);
  u16 PopCount = 0;

  {
    ScopedLock L(Region->FLLock);
    PopCount = popBlocksImpl(SizeClassAllocator, ClassId, Region, ToArray,
                             MaxBlockCount);
    if (PopCount != 0U)
      return PopCount;
  }

  bool ReportRegionExhausted = false;

  if (conditionVariableEnabled()) {
    PopCount = popBlocksWithCV(SizeClassAllocator, ClassId, Region, ToArray,
                               MaxBlockCount, ReportRegionExhausted);
  } else {
    while (true) {
      // When two threads compete for `Region->MMLock`, we only want one of
      // them to call populateFreeListAndPopBlocks(). To avoid both of them
      // doing that, always check the freelist before mapping new pages.
      ScopedLock ML(Region->MMLock);
      {
        ScopedLock FL(Region->FLLock);
        PopCount = popBlocksImpl(SizeClassAllocator, ClassId, Region, ToArray,
                                 MaxBlockCount);
        if (PopCount != 0U)
          return PopCount;
      }

      const bool RegionIsExhausted = Region->Exhausted;
      if (!RegionIsExhausted) {
        PopCount = populateFreeListAndPopBlocks(SizeClassAllocator, ClassId,
                                                Region, ToArray, MaxBlockCount);
      }
      ReportRegionExhausted = !RegionIsExhausted && Region->Exhausted;
      break;
    }
  }

  if (UNLIKELY(ReportRegionExhausted)) {
    Printf("Can't populate more pages for size class %zu.\n",
           getSizeByClassId(ClassId));

    // Theoretically, BatchClass shouldn't be used up. Abort immediately  when
    // it happens.
    if (ClassId == SizeClassMap::BatchClassId)
      reportOutOfBatchClass();
  }

  return PopCount;
}

template <typename Config>
u16 SizeClassAllocator64<Config>::popBlocksWithCV(
    SizeClassAllocatorT *SizeClassAllocator, uptr ClassId, RegionInfo *Region,
    CompactPtrT *ToArray, const u16 MaxBlockCount,
    bool &ReportRegionExhausted) {
  u16 PopCount = 0;

  while (true) {
    // We only expect one thread doing the freelist refillment and other
    // threads will be waiting for either the completion of the
    // `populateFreeListAndPopBlocks()` or `pushBlocks()` called by other
    // threads.
    bool PopulateFreeList = false;
    {
      ScopedLock FL(Region->FLLock);
      if (!Region->isPopulatingFreeList) {
        Region->isPopulatingFreeList = true;
        PopulateFreeList = true;
      }
    }

    if (PopulateFreeList) {
      ScopedLock ML(Region->MMLock);

      const bool RegionIsExhausted = Region->Exhausted;
      if (!RegionIsExhausted) {
        PopCount = populateFreeListAndPopBlocks(SizeClassAllocator, ClassId,
                                                Region, ToArray, MaxBlockCount);
      }
      ReportRegionExhausted = !RegionIsExhausted && Region->Exhausted;

      {
        // Before reacquiring the `FLLock`, the freelist may be used up again
        // and some threads are waiting for the freelist refillment by the
        // current thread. It's important to set
        // `Region->isPopulatingFreeList` to false so the threads about to
        // sleep will notice the status change.
        ScopedLock FL(Region->FLLock);
        Region->isPopulatingFreeList = false;
        Region->FLLockCV.notifyAll(Region->FLLock);
      }

      break;
    }

    // At here, there are two preconditions to be met before waiting,
    //   1. The freelist is empty.
    //   2. Region->isPopulatingFreeList == true, i.e, someone is still doing
    //   `populateFreeListAndPopBlocks()`.
    //
    // Note that it has the chance that freelist is empty but
    // Region->isPopulatingFreeList == false because all the new populated
    // blocks were used up right after the refillment. Therefore, we have to
    // check if someone is still populating the freelist.
    ScopedLock FL(Region->FLLock);
    PopCount = popBlocksImpl(SizeClassAllocator, ClassId, Region, ToArray,
                             MaxBlockCount);
    if (PopCount != 0U)
      break;

    if (!Region->isPopulatingFreeList)
      continue;

    // Now the freelist is empty and someone's doing the refillment. We will
    // wait until anyone refills the freelist or someone finishes doing
    // `populateFreeListAndPopBlocks()`. The refillment can be done by
    // `populateFreeListAndPopBlocks()`, `pushBlocks()`,
    // `pushBatchClassBlocks()` and `mergeGroupsToReleaseBack()`.
    Region->FLLockCV.wait(Region->FLLock);

    PopCount = popBlocksImpl(SizeClassAllocator, ClassId, Region, ToArray,
                             MaxBlockCount);
    if (PopCount != 0U)
      break;
  }

  return PopCount;
}

template <typename Config>
u16 SizeClassAllocator64<Config>::popBlocksImpl(
    SizeClassAllocatorT *SizeClassAllocator, uptr ClassId, RegionInfo *Region,
    CompactPtrT *ToArray, const u16 MaxBlockCount) REQUIRES(Region->FLLock) {
  if (Region->FreeListInfo.BlockList.empty())
    return 0U;

  SinglyLinkedList<BatchT> &Batches =
      Region->FreeListInfo.BlockList.front()->Batches;

  if (Batches.empty()) {
    DCHECK_EQ(ClassId, SizeClassMap::BatchClassId);
    BatchGroupT *BG = Region->FreeListInfo.BlockList.front();
    Region->FreeListInfo.BlockList.pop_front();

    // Block used by `BatchGroup` is from BatchClassId. Turn the block into
    // `Batch` with single block.
    BatchT *TB = reinterpret_cast<BatchT *>(BG);
    ToArray[0] =
        compactPtr(SizeClassMap::BatchClassId, reinterpret_cast<uptr>(TB));
    Region->FreeListInfo.PoppedBlocks += 1;
    return 1U;
  }

  // So far, instead of always filling blocks to `MaxBlockCount`, we only
  // examine single `Batch` to minimize the time spent in the primary
  // allocator. Besides, the sizes of `Batch` and
  // `SizeClassAllocatorT::getMaxCached()` may also impact the time spent on
  // accessing the primary allocator.
  // TODO(chiahungduan): Evaluate if we want to always prepare `MaxBlockCount`
  // blocks and/or adjust the size of `Batch` according to
  // `SizeClassAllocatorT::getMaxCached()`.
  BatchT *B = Batches.front();
  DCHECK_NE(B, nullptr);
  DCHECK_GT(B->getCount(), 0U);

  // BachClassId should always take all blocks in the Batch. Read the
  // comment in `pushBatchClassBlocks()` for more details.
  const u16 PopCount = ClassId == SizeClassMap::BatchClassId
                           ? B->getCount()
                           : Min(MaxBlockCount, B->getCount());
  B->moveNToArray(ToArray, PopCount);

  // TODO(chiahungduan): The deallocation of unused BatchClassId blocks can be
  // done without holding `FLLock`.
  if (B->empty()) {
    Batches.pop_front();
    // `Batch` of BatchClassId is self-contained, no need to
    // deallocate. Read the comment in `pushBatchClassBlocks()` for more
    // details.
    if (ClassId != SizeClassMap::BatchClassId)
      SizeClassAllocator->deallocate(SizeClassMap::BatchClassId, B);

    if (Batches.empty()) {
      BatchGroupT *BG = Region->FreeListInfo.BlockList.front();
      Region->FreeListInfo.BlockList.pop_front();

      // We don't keep BatchGroup with zero blocks to avoid empty-checking
      // while allocating. Note that block used for constructing BatchGroup is
      // recorded as free blocks in the last element of BatchGroup::Batches.
      // Which means, once we pop the last Batch, the block is
      // implicitly deallocated.
      if (ClassId != SizeClassMap::BatchClassId)
        SizeClassAllocator->deallocate(SizeClassMap::BatchClassId, BG);
    }
  }

  Region->FreeListInfo.PoppedBlocks += PopCount;

  return PopCount;
}

template <typename Config>
u16 SizeClassAllocator64<Config>::populateFreeListAndPopBlocks(
    SizeClassAllocatorT *SizeClassAllocator, uptr ClassId, RegionInfo *Region,
    CompactPtrT *ToArray, const u16 MaxBlockCount) REQUIRES(Region->MMLock)
    EXCLUDES(Region->FLLock) {
  if (!Config::getEnableContiguousRegions() &&
      !Region->MemMapInfo.MemMap.isAllocated()) {
    ReservedMemoryT ReservedMemory;
    if (UNLIKELY(!ReservedMemory.create(/*Addr=*/0U, RegionSize,
                                        "scudo:primary_reserve",
                                        MAP_ALLOWNOMEM))) {
      Printf("Can't reserve pages for size class %zu.\n",
             getSizeByClassId(ClassId));
      return 0U;
    }
    initRegion(Region, ClassId,
               ReservedMemory.dispatch(ReservedMemory.getBase(),
                                       ReservedMemory.getCapacity()),
               /*EnableRandomOffset=*/false);
  }

  DCHECK(Region->MemMapInfo.MemMap.isAllocated());
  const uptr Size = getSizeByClassId(ClassId);
  const u16 MaxCount = SizeClassAllocatorT::getMaxCached(Size);
  const uptr RegionBeg = Region->RegionBeg;
  const uptr MappedUser = Region->MemMapInfo.MappedUser;
  const uptr TotalUserBytes =
      Region->MemMapInfo.AllocatedUser + MaxCount * Size;
  // Map more space for blocks, if necessary.
  if (TotalUserBytes > MappedUser) {
    // Do the mmap for the user memory.
    const uptr MapSize = roundUp(TotalUserBytes - MappedUser, MapSizeIncrement);
    const uptr RegionBase = RegionBeg - getRegionBaseByClassId(ClassId);
    if (UNLIKELY(RegionBase + MappedUser + MapSize > RegionSize)) {
      Region->Exhausted = true;
      return 0U;
    }

    if (UNLIKELY(!Region->MemMapInfo.MemMap.remap(
            RegionBeg + MappedUser, MapSize, "scudo:primary",
            MAP_ALLOWNOMEM | MAP_RESIZABLE |
                (useMemoryTagging<Config>(Options.load()) ? MAP_MEMTAG : 0)))) {
      return 0U;
    }
    Region->MemMapInfo.MappedUser += MapSize;
    SizeClassAllocator->getStats().add(StatMapped, MapSize);
  }

  const u32 NumberOfBlocks =
      Min(MaxNumBatches * MaxCount,
          static_cast<u32>((Region->MemMapInfo.MappedUser -
                            Region->MemMapInfo.AllocatedUser) /
                           Size));
  DCHECK_GT(NumberOfBlocks, 0);

  constexpr u32 ShuffleArraySize = MaxNumBatches * MaxNumBlocksInBatch;
  CompactPtrT ShuffleArray[ShuffleArraySize];
  DCHECK_LE(NumberOfBlocks, ShuffleArraySize);

  const uptr CompactPtrBase = getCompactPtrBaseByClassId(ClassId);
  uptr P = RegionBeg + Region->MemMapInfo.AllocatedUser;
  for (u32 I = 0; I < NumberOfBlocks; I++, P += Size)
    ShuffleArray[I] = compactPtrInternal(CompactPtrBase, P);

  ScopedLock L(Region->FLLock);

  if (ClassId != SizeClassMap::BatchClassId) {
    u32 N = 1;
    uptr CurGroup = compactPtrGroup(ShuffleArray[0]);
    for (u32 I = 1; I < NumberOfBlocks; I++) {
      if (UNLIKELY(compactPtrGroup(ShuffleArray[I]) != CurGroup)) {
        shuffle(ShuffleArray + I - N, N, &Region->RandState);
        pushBlocksImpl(SizeClassAllocator, ClassId, Region,
                       ShuffleArray + I - N, N,
                       /*SameGroup=*/true);
        N = 1;
        CurGroup = compactPtrGroup(ShuffleArray[I]);
      } else {
        ++N;
      }
    }

    shuffle(ShuffleArray + NumberOfBlocks - N, N, &Region->RandState);
    pushBlocksImpl(SizeClassAllocator, ClassId, Region,
                   &ShuffleArray[NumberOfBlocks - N], N,
                   /*SameGroup=*/true);
  } else {
    pushBatchClassBlocks(Region, ShuffleArray, NumberOfBlocks);
  }

  const u16 PopCount = popBlocksImpl(SizeClassAllocator, ClassId, Region,
                                     ToArray, MaxBlockCount);
  DCHECK_NE(PopCount, 0U);

  // Note that `PushedBlocks` and `PoppedBlocks` are supposed to only record
  // the requests from `PushBlocks` and `PopBatch` which are external
  // interfaces. `populateFreeListAndPopBlocks` is the internal interface so
  // we should set the values back to avoid incorrectly setting the stats.
  Region->FreeListInfo.PushedBlocks -= NumberOfBlocks;

  const uptr AllocatedUser = Size * NumberOfBlocks;
  SizeClassAllocator->getStats().add(StatFree, AllocatedUser);
  Region->MemMapInfo.AllocatedUser += AllocatedUser;

  return PopCount;
}

template <typename Config>
void SizeClassAllocator64<Config>::pushBlocks(
    SizeClassAllocatorT *SizeClassAllocator, uptr ClassId, CompactPtrT *Array,
    u32 Size) {
  DCHECK_LT(ClassId, NumClasses);
  DCHECK_GT(Size, 0);

  RegionInfo *Region = getRegionInfo(ClassId);
  if (ClassId == SizeClassMap::BatchClassId) {
    ScopedLock L(Region->FLLock);
    pushBatchClassBlocks(Region, Array, Size);
    if (conditionVariableEnabled())
      Region->FLLockCV.notifyAll(Region->FLLock);
    return;
  }

  // TODO(chiahungduan): Consider not doing grouping if the group size is not
  // greater than the block size with a certain scale.

  bool SameGroup = true;
  if (GroupSizeLog < RegionSizeLog) {
    // Sort the blocks so that blocks belonging to the same group can be
    // pushed together.
    for (u32 I = 1; I < Size; ++I) {
      if (compactPtrGroup(Array[I - 1]) != compactPtrGroup(Array[I]))
        SameGroup = false;
      CompactPtrT Cur = Array[I];
      u32 J = I;
      while (J > 0 && compactPtrGroup(Cur) < compactPtrGroup(Array[J - 1])) {
        Array[J] = Array[J - 1];
        --J;
      }
      Array[J] = Cur;
    }
  }

  {
    ScopedLock L(Region->FLLock);
    pushBlocksImpl(SizeClassAllocator, ClassId, Region, Array, Size, SameGroup);
    if (conditionVariableEnabled())
      Region->FLLockCV.notifyAll(Region->FLLock);
  }
}

// Push the blocks to their batch group. The layout will be like,
//
// FreeListInfo.BlockList - > BG -> BG -> BG
//                            |     |     |
//                            v     v     v
//                            TB    TB    TB
//                            |
//                            v
//                            TB
//
// Each BlockGroup(BG) will associate with unique group id and the free blocks
// are managed by a list of Batch(TB). To reduce the time of inserting blocks,
// BGs are sorted and the input `Array` are supposed to be sorted so that we can
// get better performance of maintaining sorted property. Use `SameGroup=true`
// to indicate that all blocks in the array are from the same group then we will
// skip checking the group id of each block.
template <typename Config>
void SizeClassAllocator64<Config>::pushBlocksImpl(
    SizeClassAllocatorT *SizeClassAllocator, uptr ClassId, RegionInfo *Region,
    CompactPtrT *Array, u32 Size, bool SameGroup) REQUIRES(Region->FLLock) {
  DCHECK_NE(ClassId, SizeClassMap::BatchClassId);
  DCHECK_GT(Size, 0U);

  auto CreateGroup = [&](uptr CompactPtrGroupBase) {
    BatchGroupT *BG = reinterpret_cast<BatchGroupT *>(
        SizeClassAllocator->getBatchClassBlock());
    BG->Batches.clear();
    BatchT *TB =
        reinterpret_cast<BatchT *>(SizeClassAllocator->getBatchClassBlock());
    TB->clear();

    BG->CompactPtrGroupBase = CompactPtrGroupBase;
    BG->Batches.push_front(TB);
    BG->BytesInBGAtLastCheckpoint = 0;
    BG->MaxCachedPerBatch = MaxNumBlocksInBatch;

    return BG;
  };

  auto InsertBlocks = [&](BatchGroupT *BG, CompactPtrT *Array, u32 Size) {
    SinglyLinkedList<BatchT> &Batches = BG->Batches;
    BatchT *CurBatch = Batches.front();
    DCHECK_NE(CurBatch, nullptr);

    for (u32 I = 0; I < Size;) {
      DCHECK_GE(BG->MaxCachedPerBatch, CurBatch->getCount());
      u16 UnusedSlots =
          static_cast<u16>(BG->MaxCachedPerBatch - CurBatch->getCount());
      if (UnusedSlots == 0) {
        CurBatch = reinterpret_cast<BatchT *>(
            SizeClassAllocator->getBatchClassBlock());
        CurBatch->clear();
        Batches.push_front(CurBatch);
        UnusedSlots = BG->MaxCachedPerBatch;
      }
      // `UnusedSlots` is u16 so the result will be also fit in u16.
      u16 AppendSize = static_cast<u16>(Min<u32>(UnusedSlots, Size - I));
      CurBatch->appendFromArray(&Array[I], AppendSize);
      I += AppendSize;
    }
  };

  Region->FreeListInfo.PushedBlocks += Size;
  BatchGroupT *Cur = Region->FreeListInfo.BlockList.front();

  // In the following, `Cur` always points to the BatchGroup for blocks that
  // will be pushed next. `Prev` is the element right before `Cur`.
  BatchGroupT *Prev = nullptr;

  while (Cur != nullptr &&
         compactPtrGroup(Array[0]) > Cur->CompactPtrGroupBase) {
    Prev = Cur;
    Cur = Cur->Next;
  }

  if (Cur == nullptr || compactPtrGroup(Array[0]) != Cur->CompactPtrGroupBase) {
    Cur = CreateGroup(compactPtrGroup(Array[0]));
    if (Prev == nullptr)
      Region->FreeListInfo.BlockList.push_front(Cur);
    else
      Region->FreeListInfo.BlockList.insert(Prev, Cur);
  }

  // All the blocks are from the same group, just push without checking group
  // id.
  if (SameGroup) {
    for (u32 I = 0; I < Size; ++I)
      DCHECK_EQ(compactPtrGroup(Array[I]), Cur->CompactPtrGroupBase);

    InsertBlocks(Cur, Array, Size);
    return;
  }

  // The blocks are sorted by group id. Determine the segment of group and
  // push them to their group together.
  u32 Count = 1;
  for (u32 I = 1; I < Size; ++I) {
    if (compactPtrGroup(Array[I - 1]) != compactPtrGroup(Array[I])) {
      DCHECK_EQ(compactPtrGroup(Array[I - 1]), Cur->CompactPtrGroupBase);
      InsertBlocks(Cur, Array + I - Count, Count);

      while (Cur != nullptr &&
             compactPtrGroup(Array[I]) > Cur->CompactPtrGroupBase) {
        Prev = Cur;
        Cur = Cur->Next;
      }

      if (Cur == nullptr ||
          compactPtrGroup(Array[I]) != Cur->CompactPtrGroupBase) {
        Cur = CreateGroup(compactPtrGroup(Array[I]));
        DCHECK_NE(Prev, nullptr);
        Region->FreeListInfo.BlockList.insert(Prev, Cur);
      }

      Count = 1;
    } else {
      ++Count;
    }
  }

  InsertBlocks(Cur, Array + Size - Count, Count);
}

template <typename Config>
void SizeClassAllocator64<Config>::pushBatchClassBlocks(RegionInfo *Region,
                                                        CompactPtrT *Array,
                                                        u32 Size)
    REQUIRES(Region->FLLock) {
  DCHECK_EQ(Region, getRegionInfo(SizeClassMap::BatchClassId));

  // Free blocks are recorded by Batch in freelist for all
  // size-classes. In addition, Batch is allocated from BatchClassId.
  // In order not to use additional block to record the free blocks in
  // BatchClassId, they are self-contained. I.e., A Batch records the
  // block address of itself. See the figure below:
  //
  // Batch at 0xABCD
  // +----------------------------+
  // | Free blocks' addr          |
  // | +------+------+------+     |
  // | |0xABCD|...   |...   |     |
  // | +------+------+------+     |
  // +----------------------------+
  //
  // When we allocate all the free blocks in the Batch, the block used
  // by Batch is also free for use. We don't need to recycle the
  // Batch. Note that the correctness is maintained by the invariant,
  //
  //   Each popBlocks() request returns the entire Batch. Returning
  //   part of the blocks in a Batch is invalid.
  //
  // This ensures that Batch won't leak the address itself while it's
  // still holding other valid data.
  //
  // Besides, BatchGroup is also allocated from BatchClassId and has its
  // address recorded in the Batch too. To maintain the correctness,
  //
  //   The address of BatchGroup is always recorded in the last Batch
  //   in the freelist (also imply that the freelist should only be
  //   updated with push_front). Once the last Batch is popped,
  //   the block used by BatchGroup is also free for use.
  //
  // With this approach, the blocks used by BatchGroup and Batch are
  // reusable and don't need additional space for them.

  Region->FreeListInfo.PushedBlocks += Size;
  BatchGroupT *BG = Region->FreeListInfo.BlockList.front();

  if (BG == nullptr) {
    // Construct `BatchGroup` on the last element.
    BG = reinterpret_cast<BatchGroupT *>(
        decompactPtr(SizeClassMap::BatchClassId, Array[Size - 1]));
    --Size;
    BG->Batches.clear();
    // BatchClass hasn't enabled memory group. Use `0` to indicate there's no
    // memory group here.
    BG->CompactPtrGroupBase = 0;
    BG->BytesInBGAtLastCheckpoint = 0;
    BG->MaxCachedPerBatch = SizeClassAllocatorT::getMaxCached(
        getSizeByClassId(SizeClassMap::BatchClassId));

    Region->FreeListInfo.BlockList.push_front(BG);
  }

  if (UNLIKELY(Size == 0))
    return;

  // This happens under 2 cases.
  //   1. just allocated a new `BatchGroup`.
  //   2. Only 1 block is pushed when the freelist is empty.
  if (BG->Batches.empty()) {
    // Construct the `Batch` on the last element.
    BatchT *TB = reinterpret_cast<BatchT *>(
        decompactPtr(SizeClassMap::BatchClassId, Array[Size - 1]));
    TB->clear();
    // As mentioned above, addresses of `Batch` and `BatchGroup` are
    // recorded in the Batch.
    TB->add(Array[Size - 1]);
    TB->add(compactPtr(SizeClassMap::BatchClassId, reinterpret_cast<uptr>(BG)));
    --Size;
    BG->Batches.push_front(TB);
  }

  BatchT *CurBatch = BG->Batches.front();
  DCHECK_NE(CurBatch, nullptr);

  for (u32 I = 0; I < Size;) {
    u16 UnusedSlots =
        static_cast<u16>(BG->MaxCachedPerBatch - CurBatch->getCount());
    if (UnusedSlots == 0) {
      CurBatch = reinterpret_cast<BatchT *>(
          decompactPtr(SizeClassMap::BatchClassId, Array[I]));
      CurBatch->clear();
      // Self-contained
      CurBatch->add(Array[I]);
      ++I;
      // TODO(chiahungduan): Avoid the use of push_back() in `Batches` of
      // BatchClassId.
      BG->Batches.push_front(CurBatch);
      UnusedSlots = static_cast<u16>(BG->MaxCachedPerBatch - 1);
    }
    // `UnusedSlots` is u16 so the result will be also fit in u16.
    const u16 AppendSize = static_cast<u16>(Min<u32>(UnusedSlots, Size - I));
    CurBatch->appendFromArray(&Array[I], AppendSize);
    I += AppendSize;
  }
}

template <typename Config>
void SizeClassAllocator64<Config>::disable() NO_THREAD_SAFETY_ANALYSIS {
  // The BatchClassId must be locked last since other classes can use it.
  for (sptr I = static_cast<sptr>(NumClasses) - 1; I >= 0; I--) {
    if (static_cast<uptr>(I) == SizeClassMap::BatchClassId)
      continue;
    getRegionInfo(static_cast<uptr>(I))->MMLock.lock();
    getRegionInfo(static_cast<uptr>(I))->FLLock.lock();
  }
  getRegionInfo(SizeClassMap::BatchClassId)->MMLock.lock();
  getRegionInfo(SizeClassMap::BatchClassId)->FLLock.lock();
}

template <typename Config>
void SizeClassAllocator64<Config>::enable() NO_THREAD_SAFETY_ANALYSIS {
  getRegionInfo(SizeClassMap::BatchClassId)->FLLock.unlock();
  getRegionInfo(SizeClassMap::BatchClassId)->MMLock.unlock();
  for (uptr I = 0; I < NumClasses; I++) {
    if (I == SizeClassMap::BatchClassId)
      continue;
    getRegionInfo(I)->FLLock.unlock();
    getRegionInfo(I)->MMLock.unlock();
  }
}

template <typename Config>
template <typename F>
void SizeClassAllocator64<Config>::iterateOverBlocks(F Callback) {
  for (uptr I = 0; I < NumClasses; I++) {
    if (I == SizeClassMap::BatchClassId)
      continue;
    RegionInfo *Region = getRegionInfo(I);
    // TODO: The call of `iterateOverBlocks` requires disabling
    // SizeClassAllocator64. We may consider locking each region on demand
    // only.
    Region->FLLock.assertHeld();
    Region->MMLock.assertHeld();
    const uptr BlockSize = getSizeByClassId(I);
    const uptr From = Region->RegionBeg;
    const uptr To = From + Region->MemMapInfo.AllocatedUser;
    for (uptr Block = From; Block < To; Block += BlockSize)
      Callback(Block);
  }
}

template <typename Config>
void SizeClassAllocator64<Config>::getStats(ScopedString *Str) {
  // TODO(kostyak): get the RSS per region.
  uptr TotalMapped = 0;
  uptr PoppedBlocks = 0;
  uptr PushedBlocks = 0;
  for (uptr I = 0; I < NumClasses; I++) {
    RegionInfo *Region = getRegionInfo(I);
    {
      ScopedLock L(Region->MMLock);
      TotalMapped += Region->MemMapInfo.MappedUser;
    }
    {
      ScopedLock L(Region->FLLock);
      PoppedBlocks += Region->FreeListInfo.PoppedBlocks;
      PushedBlocks += Region->FreeListInfo.PushedBlocks;
    }
  }
  const s32 IntervalMs = atomic_load_relaxed(&ReleaseToOsIntervalMs);
  Str->append("Stats: SizeClassAllocator64: %zuM mapped (%uM rss) in %zu "
              "allocations; remains %zu; ReleaseToOsIntervalMs = %d\n",
              TotalMapped >> 20, 0U, PoppedBlocks, PoppedBlocks - PushedBlocks,
              IntervalMs >= 0 ? IntervalMs : -1);

  for (uptr I = 0; I < NumClasses; I++) {
    RegionInfo *Region = getRegionInfo(I);
    ScopedLock L1(Region->MMLock);
    ScopedLock L2(Region->FLLock);
    getStats(Str, I, Region);
  }
}

template <typename Config>
void SizeClassAllocator64<Config>::getStats(ScopedString *Str, uptr ClassId,
                                            RegionInfo *Region)
    REQUIRES(Region->MMLock, Region->FLLock) {
  if (Region->MemMapInfo.MappedUser == 0)
    return;
  const uptr BlockSize = getSizeByClassId(ClassId);
  const uptr InUseBlocks =
      Region->FreeListInfo.PoppedBlocks - Region->FreeListInfo.PushedBlocks;
  const uptr BytesInFreeList =
      Region->MemMapInfo.AllocatedUser - InUseBlocks * BlockSize;
  uptr RegionPushedBytesDelta = 0;
  if (BytesInFreeList >= Region->ReleaseInfo.BytesInFreeListAtLastCheckpoint) {
    RegionPushedBytesDelta =
        BytesInFreeList - Region->ReleaseInfo.BytesInFreeListAtLastCheckpoint;
  }
  const uptr TotalChunks = Region->MemMapInfo.AllocatedUser / BlockSize;
  Str->append(
      "%s %02zu (%6zu): mapped: %6zuK popped: %7zu pushed: %7zu "
      "inuse: %6zu total: %6zu releases attempted: %6zu last "
      "released: %6zuK latest pushed bytes: %6zuK region: 0x%zx "
      "(0x%zx)\n",
      Region->Exhausted ? "E" : " ", ClassId, getSizeByClassId(ClassId),
      Region->MemMapInfo.MappedUser >> 10, Region->FreeListInfo.PoppedBlocks,
      Region->FreeListInfo.PushedBlocks, InUseBlocks, TotalChunks,
      Region->ReleaseInfo.NumReleasesAttempted,
      Region->ReleaseInfo.LastReleasedBytes >> 10, RegionPushedBytesDelta >> 10,
      Region->RegionBeg, getRegionBaseByClassId(ClassId));
}

template <typename Config>
void SizeClassAllocator64<Config>::getFragmentationInfo(ScopedString *Str) {
  Str->append(
      "Fragmentation Stats: SizeClassAllocator64: page size = %zu bytes\n",
      getPageSizeCached());

  for (uptr I = 1; I < NumClasses; I++) {
    RegionInfo *Region = getRegionInfo(I);
    ScopedLock L(Region->MMLock);
    getRegionFragmentationInfo(Region, I, Str);
  }
}

template <typename Config>
void SizeClassAllocator64<Config>::getRegionFragmentationInfo(
    RegionInfo *Region, uptr ClassId, ScopedString *Str)
    REQUIRES(Region->MMLock) {
  const uptr BlockSize = getSizeByClassId(ClassId);
  const uptr AllocatedUserEnd =
      Region->MemMapInfo.AllocatedUser + Region->RegionBeg;

  SinglyLinkedList<BatchGroupT> GroupsToRelease;
  {
    ScopedLock L(Region->FLLock);
    GroupsToRelease = Region->FreeListInfo.BlockList;
    Region->FreeListInfo.BlockList.clear();
  }

  FragmentationRecorder Recorder;
  if (!GroupsToRelease.empty()) {
    PageReleaseContext Context =
        markFreeBlocks(Region, BlockSize, AllocatedUserEnd,
                       getCompactPtrBaseByClassId(ClassId), GroupsToRelease);
    auto SkipRegion = [](UNUSED uptr RegionIndex) { return false; };
    releaseFreeMemoryToOS(Context, Recorder, SkipRegion);

    mergeGroupsToReleaseBack(Region, GroupsToRelease);
  }

  ScopedLock L(Region->FLLock);
  const uptr PageSize = getPageSizeCached();
  const uptr TotalBlocks = Region->MemMapInfo.AllocatedUser / BlockSize;
  const uptr InUseBlocks =
      Region->FreeListInfo.PoppedBlocks - Region->FreeListInfo.PushedBlocks;
  const uptr AllocatedPagesCount =
      roundUp(Region->MemMapInfo.AllocatedUser, PageSize) / PageSize;
  DCHECK_GE(AllocatedPagesCount, Recorder.getReleasedPagesCount());
  const uptr InUsePages =
      AllocatedPagesCount - Recorder.getReleasedPagesCount();
  const uptr InUseBytes = InUsePages * PageSize;

  uptr Integral;
  uptr Fractional;
  computePercentage(BlockSize * InUseBlocks, InUseBytes, &Integral,
                    &Fractional);
  Str->append("  %02zu (%6zu): inuse/total blocks: %6zu/%6zu inuse/total "
              "pages: %6zu/%6zu inuse bytes: %6zuK util: %3zu.%02zu%%\n",
              ClassId, BlockSize, InUseBlocks, TotalBlocks, InUsePages,
              AllocatedPagesCount, InUseBytes >> 10, Integral, Fractional);
}

template <typename Config>
void SizeClassAllocator64<Config>::getMemoryGroupFragmentationInfoInRegion(
    RegionInfo *Region, uptr ClassId, ScopedString *Str)
    REQUIRES(Region->MMLock) EXCLUDES(Region->FLLock) {
  const uptr BlockSize = getSizeByClassId(ClassId);
  const uptr AllocatedUserEnd =
      Region->MemMapInfo.AllocatedUser + Region->RegionBeg;

  SinglyLinkedList<BatchGroupT> GroupsToRelease;
  {
    ScopedLock L(Region->FLLock);
    GroupsToRelease = Region->FreeListInfo.BlockList;
    Region->FreeListInfo.BlockList.clear();
  }

  constexpr uptr GroupSize = (1UL << GroupSizeLog);
  constexpr uptr MaxNumGroups = RegionSize / GroupSize;

  MemoryGroupFragmentationRecorder<GroupSize, MaxNumGroups> Recorder;
  if (!GroupsToRelease.empty()) {
    PageReleaseContext Context =
        markFreeBlocks(Region, BlockSize, AllocatedUserEnd,
                       getCompactPtrBaseByClassId(ClassId), GroupsToRelease);
    auto SkipRegion = [](UNUSED uptr RegionIndex) { return false; };
    releaseFreeMemoryToOS(Context, Recorder, SkipRegion);

    mergeGroupsToReleaseBack(Region, GroupsToRelease);
  }

  Str->append("MemoryGroupFragmentationInfo in Region %zu (%zu)\n", ClassId,
              BlockSize);

  const uptr MaxNumGroupsInUse =
      roundUp(Region->MemMapInfo.AllocatedUser, GroupSize) / GroupSize;
  for (uptr I = 0; I < MaxNumGroupsInUse; ++I) {
    uptr Integral;
    uptr Fractional;
    computePercentage(Recorder.NumPagesInOneGroup - Recorder.getNumFreePages(I),
                      Recorder.NumPagesInOneGroup, &Integral, &Fractional);
    Str->append("MemoryGroup #%zu (0x%zx): util: %3zu.%02zu%%\n", I,
                Region->RegionBeg + I * GroupSize, Integral, Fractional);
  }
}

template <typename Config>
void SizeClassAllocator64<Config>::getMemoryGroupFragmentationInfo(
    ScopedString *Str) {
  Str->append(
      "Fragmentation Stats: SizeClassAllocator64: page size = %zu bytes\n",
      getPageSizeCached());

  for (uptr I = 1; I < NumClasses; I++) {
    RegionInfo *Region = getRegionInfo(I);
    ScopedLock L(Region->MMLock);
    getMemoryGroupFragmentationInfoInRegion(Region, I, Str);
  }
}

template <typename Config>
bool SizeClassAllocator64<Config>::setOption(Option O, sptr Value) {
  if (O == Option::ReleaseInterval) {
    const s32 Interval =
        Max(Min(static_cast<s32>(Value), Config::getMaxReleaseToOsIntervalMs()),
            Config::getMinReleaseToOsIntervalMs());
    atomic_store_relaxed(&ReleaseToOsIntervalMs, Interval);
    return true;
  }
  // Not supported by the Primary, but not an error either.
  return true;
}

template <typename Config>
uptr SizeClassAllocator64<Config>::tryReleaseToOS(uptr ClassId,
                                                  ReleaseToOS ReleaseType) {
  RegionInfo *Region = getRegionInfo(ClassId);
  // Note that the tryLock() may fail spuriously, given that it should rarely
  // happen and page releasing is fine to skip, we don't take certain
  // approaches to ensure one page release is done.
  if (Region->MMLock.tryLock()) {
    uptr BytesReleased = releaseToOSMaybe(Region, ClassId, ReleaseType);
    Region->MMLock.unlock();
    return BytesReleased;
  }
  return 0;
}

template <typename Config>
uptr SizeClassAllocator64<Config>::releaseToOS(ReleaseToOS ReleaseType) {
  uptr TotalReleasedBytes = 0;
  for (uptr I = 0; I < NumClasses; I++) {
    if (I == SizeClassMap::BatchClassId)
      continue;
    RegionInfo *Region = getRegionInfo(I);
    ScopedLock L(Region->MMLock);
    TotalReleasedBytes += releaseToOSMaybe(Region, I, ReleaseType);
  }
  return TotalReleasedBytes;
}

template <typename Config>
/* static */ BlockInfo SizeClassAllocator64<Config>::findNearestBlock(
    const char *RegionInfoData, uptr Ptr) NO_THREAD_SAFETY_ANALYSIS {
  const RegionInfo *RegionInfoArray =
      reinterpret_cast<const RegionInfo *>(RegionInfoData);

  uptr ClassId;
  uptr MinDistance = -1UL;
  for (uptr I = 0; I != NumClasses; ++I) {
    if (I == SizeClassMap::BatchClassId)
      continue;
    uptr Begin = RegionInfoArray[I].RegionBeg;
    // TODO(chiahungduan): In fact, We need to lock the RegionInfo::MMLock.
    // However, the RegionInfoData is passed with const qualifier and lock the
    // mutex requires modifying RegionInfoData, which means we need to remove
    // the const qualifier. This may lead to another undefined behavior (The
    // first one is accessing `AllocatedUser` without locking. It's better to
    // pass `RegionInfoData` as `void *` then we can lock the mutex properly.
    uptr End = Begin + RegionInfoArray[I].MemMapInfo.AllocatedUser;
    if (Begin > End || End - Begin < SizeClassMap::getSizeByClassId(I))
      continue;
    uptr RegionDistance;
    if (Begin <= Ptr) {
      if (Ptr < End)
        RegionDistance = 0;
      else
        RegionDistance = Ptr - End;
    } else {
      RegionDistance = Begin - Ptr;
    }

    if (RegionDistance < MinDistance) {
      MinDistance = RegionDistance;
      ClassId = I;
    }
  }

  BlockInfo B = {};
  if (MinDistance <= 8192) {
    B.RegionBegin = RegionInfoArray[ClassId].RegionBeg;
    B.RegionEnd =
        B.RegionBegin + RegionInfoArray[ClassId].MemMapInfo.AllocatedUser;
    B.BlockSize = SizeClassMap::getSizeByClassId(ClassId);
    B.BlockBegin = B.RegionBegin + uptr(sptr(Ptr - B.RegionBegin) /
                                        sptr(B.BlockSize) * sptr(B.BlockSize));
    while (B.BlockBegin < B.RegionBegin)
      B.BlockBegin += B.BlockSize;
    while (B.RegionEnd < B.BlockBegin + B.BlockSize)
      B.BlockBegin -= B.BlockSize;
  }
  return B;
}

template <typename Config>
uptr SizeClassAllocator64<Config>::releaseToOSMaybe(RegionInfo *Region,
                                                    uptr ClassId,
                                                    ReleaseToOS ReleaseType)
    REQUIRES(Region->MMLock) EXCLUDES(Region->FLLock) {
  const uptr BlockSize = getSizeByClassId(ClassId);
  uptr BytesInFreeList;
  const uptr AllocatedUserEnd =
      Region->MemMapInfo.AllocatedUser + Region->RegionBeg;
  uptr RegionPushedBytesDelta = 0;
  SinglyLinkedList<BatchGroupT> GroupsToRelease;

  {
    ScopedLock L(Region->FLLock);

    BytesInFreeList =
        Region->MemMapInfo.AllocatedUser - (Region->FreeListInfo.PoppedBlocks -
                                            Region->FreeListInfo.PushedBlocks) *
                                               BlockSize;
    if (UNLIKELY(BytesInFreeList == 0))
      return false;

    // ==================================================================== //
    // 1. Check if we have enough free blocks and if it's worth doing a page
    //    release.
    // ==================================================================== //
    if (ReleaseType != ReleaseToOS::ForceAll &&
        !hasChanceToReleasePages(Region, BlockSize, BytesInFreeList,
                                 ReleaseType)) {
      return 0;
    }

    // Given that we will unlock the freelist for block operations, cache the
    // value here so that when we are adapting the `TryReleaseThreshold`
    // later, we are using the right metric.
    RegionPushedBytesDelta =
        BytesInFreeList - Region->ReleaseInfo.BytesInFreeListAtLastCheckpoint;

    // ==================================================================== //
    // 2. Determine which groups can release the pages. Use a heuristic to
    //    gather groups that are candidates for doing a release.
    // ==================================================================== //
    if (ReleaseType == ReleaseToOS::ForceAll) {
      GroupsToRelease = Region->FreeListInfo.BlockList;
      Region->FreeListInfo.BlockList.clear();
    } else {
      GroupsToRelease =
          collectGroupsToRelease(Region, BlockSize, AllocatedUserEnd,
                                 getCompactPtrBaseByClassId(ClassId));
    }
    if (GroupsToRelease.empty())
      return 0;
  }

  // The following steps contribute to the majority time spent in page
  // releasing thus we increment the counter here.
  ++Region->ReleaseInfo.NumReleasesAttempted;

  // Note that we have extracted the `GroupsToRelease` from region freelist.
  // It's safe to let pushBlocks()/popBlocks() access the remaining region
  // freelist. In the steps 3 and 4, we will temporarily release the FLLock
  // and lock it again before step 5.

  // ==================================================================== //
  // 3. Mark the free blocks in `GroupsToRelease` in the `PageReleaseContext`.
  //    Then we can tell which pages are in-use by querying
  //    `PageReleaseContext`.
  // ==================================================================== //
  PageReleaseContext Context =
      markFreeBlocks(Region, BlockSize, AllocatedUserEnd,
                     getCompactPtrBaseByClassId(ClassId), GroupsToRelease);
  if (UNLIKELY(!Context.hasBlockMarked())) {
    mergeGroupsToReleaseBack(Region, GroupsToRelease);
    return 0;
  }

  // ==================================================================== //
  // 4. Release the unused physical pages back to the OS.
  // ==================================================================== //
  RegionReleaseRecorder<MemMapT> Recorder(&Region->MemMapInfo.MemMap,
                                          Region->RegionBeg,
                                          Context.getReleaseOffset());
  auto SkipRegion = [](UNUSED uptr RegionIndex) { return false; };
  releaseFreeMemoryToOS(Context, Recorder, SkipRegion);
  if (Recorder.getReleasedBytes() > 0) {
    // This is the case that we didn't hit the release threshold but it has
    // been past a certain period of time. Thus we try to release some pages
    // and if it does release some additional pages, it's hint that we are
    // able to lower the threshold. Currently, this case happens when the
    // `RegionPushedBytesDelta` is over half of the `TryReleaseThreshold`. As
    // a result, we shrink the threshold to half accordingly.
    // TODO(chiahungduan): Apply the same adjustment strategy to small blocks.
    if (!isSmallBlock(BlockSize)) {
      if (RegionPushedBytesDelta < Region->ReleaseInfo.TryReleaseThreshold &&
          Recorder.getReleasedBytes() >
              Region->ReleaseInfo.LastReleasedBytes +
                  getMinReleaseAttemptSize(BlockSize)) {
        Region->ReleaseInfo.TryReleaseThreshold =
            Max(Region->ReleaseInfo.TryReleaseThreshold / 2,
                getMinReleaseAttemptSize(BlockSize));
      }
    }

    Region->ReleaseInfo.BytesInFreeListAtLastCheckpoint = BytesInFreeList;
    Region->ReleaseInfo.LastReleasedBytes = Recorder.getReleasedBytes();
  }
  Region->ReleaseInfo.LastReleaseAtNs = getMonotonicTimeFast();

  if (Region->ReleaseInfo.PendingPushedBytesDelta > 0) {
    // Instead of increasing the threshold by the amount of
    // `PendingPushedBytesDelta`, we only increase half of the amount so that
    // it won't be a leap (which may lead to higher memory pressure) because
    // of certain memory usage bursts which don't happen frequently.
    Region->ReleaseInfo.TryReleaseThreshold +=
        Region->ReleaseInfo.PendingPushedBytesDelta / 2;
    // This is another guard of avoiding the growth of threshold indefinitely.
    // Note that we may consider to make this configurable if we have a better
    // way to model this.
    Region->ReleaseInfo.TryReleaseThreshold = Min<uptr>(
        Region->ReleaseInfo.TryReleaseThreshold, (1UL << GroupSizeLog) / 2);
    Region->ReleaseInfo.PendingPushedBytesDelta = 0;
  }

  // ====================================================================== //
  // 5. Merge the `GroupsToRelease` back to the freelist.
  // ====================================================================== //
  mergeGroupsToReleaseBack(Region, GroupsToRelease);

  return Recorder.getReleasedBytes();
}

template <typename Config>
bool SizeClassAllocator64<Config>::hasChanceToReleasePages(
    RegionInfo *Region, uptr BlockSize, uptr BytesInFreeList,
    ReleaseToOS ReleaseType) REQUIRES(Region->MMLock, Region->FLLock) {
  DCHECK_GE(Region->FreeListInfo.PoppedBlocks,
            Region->FreeListInfo.PushedBlocks);
  // Always update `BytesInFreeListAtLastCheckpoint` with the smallest value
  // so that we won't underestimate the releasable pages. For example, the
  // following is the region usage,
  //
  //  BytesInFreeListAtLastCheckpoint   AllocatedUser
  //                v                         v
  //  |--------------------------------------->
  //         ^                   ^
  //  BytesInFreeList     ReleaseThreshold
  //
  // In general, if we have collected enough bytes and the amount of free
  // bytes meets the ReleaseThreshold, we will try to do page release. If we
  // don't update `BytesInFreeListAtLastCheckpoint` when the current
  // `BytesInFreeList` is smaller, we may take longer time to wait for enough
  // freed blocks because we miss the bytes between
  // (BytesInFreeListAtLastCheckpoint - BytesInFreeList).
  if (BytesInFreeList <= Region->ReleaseInfo.BytesInFreeListAtLastCheckpoint) {
    Region->ReleaseInfo.BytesInFreeListAtLastCheckpoint = BytesInFreeList;
  }

  const uptr RegionPushedBytesDelta =
      BytesInFreeList - Region->ReleaseInfo.BytesInFreeListAtLastCheckpoint;

  if (ReleaseType == ReleaseToOS::Normal) {
    if (RegionPushedBytesDelta < Region->ReleaseInfo.TryReleaseThreshold / 2)
      return false;

    const s64 IntervalMs = atomic_load_relaxed(&ReleaseToOsIntervalMs);
    if (IntervalMs < 0)
      return false;

    const u64 IntervalNs = static_cast<u64>(IntervalMs) * 1000000;
    const u64 CurTimeNs = getMonotonicTimeFast();
    const u64 DiffSinceLastReleaseNs =
        CurTimeNs - Region->ReleaseInfo.LastReleaseAtNs;

    // At here, `RegionPushedBytesDelta` is more than half of
    // `TryReleaseThreshold`. If the last release happened 2 release interval
    // before, we will still try to see if there's any chance to release some
    // memory even it doesn't exceed the threshold.
    if (RegionPushedBytesDelta < Region->ReleaseInfo.TryReleaseThreshold) {
      // We want the threshold to have a shorter response time to the variant
      // memory usage patterns. According to data collected during experiments
      // (which were done with 1, 2, 4, 8 intervals), `2` strikes the better
      // balance between the memory usage and number of page release attempts.
      if (DiffSinceLastReleaseNs < 2 * IntervalNs)
        return false;
    } else if (DiffSinceLastReleaseNs < IntervalNs) {
      // In this case, we are over the threshold but we just did some page
      // release in the same release interval. This is a hint that we may want
      // a higher threshold so that we can release more memory at once.
      // `TryReleaseThreshold` will be adjusted according to how many bytes
      // are not released, i.e., the `PendingPushedBytesdelta` here.
      // TODO(chiahungduan): Apply the same adjustment strategy to small
      // blocks.
      if (!isSmallBlock(BlockSize))
        Region->ReleaseInfo.PendingPushedBytesDelta = RegionPushedBytesDelta;

      // Memory was returned recently.
      return false;
    }
  } // if (ReleaseType == ReleaseToOS::Normal)

  return true;
}

template <typename Config>
SinglyLinkedList<typename SizeClassAllocator64<Config>::BatchGroupT>
SizeClassAllocator64<Config>::collectGroupsToRelease(
    RegionInfo *Region, const uptr BlockSize, const uptr AllocatedUserEnd,
    const uptr CompactPtrBase) REQUIRES(Region->MMLock, Region->FLLock) {
  const uptr GroupSize = (1UL << GroupSizeLog);
  const uptr PageSize = getPageSizeCached();
  SinglyLinkedList<BatchGroupT> GroupsToRelease;

  // We are examining each group and will take the minimum distance to the
  // release threshold as the next `TryReleaseThreshold`. Note that if the
  // size of free blocks has reached the release threshold, the distance to
  // the next release will be PageSize * SmallerBlockReleasePageDelta. See the
  // comment on `SmallerBlockReleasePageDelta` for more details.
  uptr MinDistToThreshold = GroupSize;

  for (BatchGroupT *BG = Region->FreeListInfo.BlockList.front(),
                   *Prev = nullptr;
       BG != nullptr;) {
    // Group boundary is always GroupSize-aligned from CompactPtr base. The
    // layout of memory groups is like,
    //
    //     (CompactPtrBase)
    // #1 CompactPtrGroupBase   #2 CompactPtrGroupBase            ...
    //           |                       |                       |
    //           v                       v                       v
    //           +-----------------------+-----------------------+
    //            \                     / \                     /
    //             ---   GroupSize   ---   ---   GroupSize   ---
    //
    // After decompacting the CompactPtrGroupBase, we expect the alignment
    // property is held as well.
    const uptr BatchGroupBase =
        decompactGroupBase(CompactPtrBase, BG->CompactPtrGroupBase);
    DCHECK_LE(Region->RegionBeg, BatchGroupBase);
    DCHECK_GE(AllocatedUserEnd, BatchGroupBase);
    DCHECK_EQ((Region->RegionBeg - BatchGroupBase) % GroupSize, 0U);
    // Batches are pushed in front of BG.Batches. The first one may
    // not have all caches used.
    const uptr NumBlocks = (BG->Batches.size() - 1) * BG->MaxCachedPerBatch +
                           BG->Batches.front()->getCount();
    const uptr BytesInBG = NumBlocks * BlockSize;

    if (BytesInBG <= BG->BytesInBGAtLastCheckpoint) {
      BG->BytesInBGAtLastCheckpoint = BytesInBG;
      Prev = BG;
      BG = BG->Next;
      continue;
    }

    const uptr PushedBytesDelta = BytesInBG - BG->BytesInBGAtLastCheckpoint;
    if (PushedBytesDelta < getMinReleaseAttemptSize(BlockSize)) {
      Prev = BG;
      BG = BG->Next;
      continue;
    }

    // Given the randomness property, we try to release the pages only if the
    // bytes used by free blocks exceed certain proportion of group size. Note
    // that this heuristic only applies when all the spaces in a BatchGroup
    // are allocated.
    if (isSmallBlock(BlockSize)) {
      const uptr BatchGroupEnd = BatchGroupBase + GroupSize;
      const uptr AllocatedGroupSize = AllocatedUserEnd >= BatchGroupEnd
                                          ? GroupSize
                                          : AllocatedUserEnd - BatchGroupBase;
      const uptr ReleaseThreshold =
          (AllocatedGroupSize * (100 - 1U - BlockSize / 16U)) / 100U;
      const bool HighDensity = BytesInBG >= ReleaseThreshold;
      const bool MayHaveReleasedAll = NumBlocks >= (GroupSize / BlockSize);
      // If all blocks in the group are released, we will do range marking
      // which is fast. Otherwise, we will wait until we have accumulated
      // a certain amount of free memory.
      const bool ReachReleaseDelta =
          MayHaveReleasedAll
              ? true
              : PushedBytesDelta >= PageSize * SmallerBlockReleasePageDelta;

      if (!HighDensity) {
        DCHECK_LE(BytesInBG, ReleaseThreshold);
        // The following is the usage of a memroy group,
        //
        //     BytesInBG             ReleaseThreshold
        //  /             \                 v
        //  +---+---------------------------+-----+
        //  |   |         |                 |     |
        //  +---+---------------------------+-----+
        //       \        /                       ^
        //    PushedBytesDelta                 GroupEnd
        MinDistToThreshold =
            Min(MinDistToThreshold,
                ReleaseThreshold - BytesInBG + PushedBytesDelta);
      } else {
        // If it reaches high density at this round, the next time we will try
        // to release is based on SmallerBlockReleasePageDelta
        MinDistToThreshold =
            Min(MinDistToThreshold, PageSize * SmallerBlockReleasePageDelta);
      }

      if (!HighDensity || !ReachReleaseDelta) {
        Prev = BG;
        BG = BG->Next;
        continue;
      }
    }

    // If `BG` is the first BatchGroupT in the list, we only need to advance
    // `BG` and call FreeListInfo.BlockList::pop_front(). No update is needed
    // for `Prev`.
    //
    //         (BG)   (BG->Next)
    // Prev     Cur      BG
    //   |       |       |
    //   v       v       v
    //  nil     +--+    +--+
    //          |X | -> |  | -> ...
    //          +--+    +--+
    //
    // Otherwise, `Prev` will be used to extract the `Cur` from the
    // `FreeListInfo.BlockList`.
    //
    //         (BG)   (BG->Next)
    // Prev     Cur      BG
    //   |       |       |
    //   v       v       v
    //  +--+    +--+    +--+
    //  |  | -> |X | -> |  | -> ...
    //  +--+    +--+    +--+
    //
    // After FreeListInfo.BlockList::extract(),
    //
    // Prev     Cur       BG
    //   |       |        |
    //   v       v        v
    //  +--+    +--+     +--+
    //  |  |-+  |X |  +->|  | -> ...
    //  +--+ |  +--+  |  +--+
    //       +--------+
    //
    // Note that we need to advance before pushing this BatchGroup to
    // GroupsToRelease because it's a destructive operation.

    BatchGroupT *Cur = BG;
    BG = BG->Next;

    // Ideally, we may want to update this only after successful release.
    // However, for smaller blocks, each block marking is a costly operation.
    // Therefore, we update it earlier.
    // TODO: Consider updating this after releasing pages if `ReleaseRecorder`
    // can tell the released bytes in each group.
    Cur->BytesInBGAtLastCheckpoint = BytesInBG;

    if (Prev != nullptr)
      Region->FreeListInfo.BlockList.extract(Prev, Cur);
    else
      Region->FreeListInfo.BlockList.pop_front();
    GroupsToRelease.push_back(Cur);
  }

  // Only small blocks have the adaptive `TryReleaseThreshold`.
  if (isSmallBlock(BlockSize)) {
    // If the MinDistToThreshold is not updated, that means each memory group
    // may have only pushed less than a page size. In that case, just set it
    // back to normal.
    if (MinDistToThreshold == GroupSize)
      MinDistToThreshold = PageSize * SmallerBlockReleasePageDelta;
    Region->ReleaseInfo.TryReleaseThreshold = MinDistToThreshold;
  }

  return GroupsToRelease;
}

template <typename Config>
PageReleaseContext SizeClassAllocator64<Config>::markFreeBlocks(
    RegionInfo *Region, const uptr BlockSize, const uptr AllocatedUserEnd,
    const uptr CompactPtrBase, SinglyLinkedList<BatchGroupT> &GroupsToRelease)
    REQUIRES(Region->MMLock) EXCLUDES(Region->FLLock) {
  const uptr GroupSize = (1UL << GroupSizeLog);
  auto DecompactPtr = [CompactPtrBase, this](CompactPtrT CompactPtr) {
    return decompactPtrInternal(CompactPtrBase, CompactPtr);
  };

  const uptr ReleaseBase = decompactGroupBase(
      CompactPtrBase, GroupsToRelease.front()->CompactPtrGroupBase);
  const uptr LastGroupEnd =
      Min(decompactGroupBase(CompactPtrBase,
                             GroupsToRelease.back()->CompactPtrGroupBase) +
              GroupSize,
          AllocatedUserEnd);
  // The last block may straddle the group boundary. Rounding up to BlockSize
  // to get the exact range.
  const uptr ReleaseEnd =
      roundUpSlow(LastGroupEnd - Region->RegionBeg, BlockSize) +
      Region->RegionBeg;
  const uptr ReleaseRangeSize = ReleaseEnd - ReleaseBase;
  const uptr ReleaseOffset = ReleaseBase - Region->RegionBeg;

  PageReleaseContext Context(BlockSize, /*NumberOfRegions=*/1U,
                             ReleaseRangeSize, ReleaseOffset);
  // We may not be able to do the page release in a rare case that we may
  // fail on PageMap allocation.
  if (UNLIKELY(!Context.ensurePageMapAllocated()))
    return Context;

  for (BatchGroupT &BG : GroupsToRelease) {
    const uptr BatchGroupBase =
        decompactGroupBase(CompactPtrBase, BG.CompactPtrGroupBase);
    const uptr BatchGroupEnd = BatchGroupBase + GroupSize;
    const uptr AllocatedGroupSize = AllocatedUserEnd >= BatchGroupEnd
                                        ? GroupSize
                                        : AllocatedUserEnd - BatchGroupBase;
    const uptr BatchGroupUsedEnd = BatchGroupBase + AllocatedGroupSize;
    const bool MayContainLastBlockInRegion =
        BatchGroupUsedEnd == AllocatedUserEnd;
    const bool BlockAlignedWithUsedEnd =
        (BatchGroupUsedEnd - Region->RegionBeg) % BlockSize == 0;

    uptr MaxContainedBlocks = AllocatedGroupSize / BlockSize;
    if (!BlockAlignedWithUsedEnd)
      ++MaxContainedBlocks;

    const uptr NumBlocks = (BG.Batches.size() - 1) * BG.MaxCachedPerBatch +
                           BG.Batches.front()->getCount();

    if (NumBlocks == MaxContainedBlocks) {
      for (const auto &It : BG.Batches) {
        if (&It != BG.Batches.front())
          DCHECK_EQ(It.getCount(), BG.MaxCachedPerBatch);
        for (u16 I = 0; I < It.getCount(); ++I)
          DCHECK_EQ(compactPtrGroup(It.get(I)), BG.CompactPtrGroupBase);
      }

      Context.markRangeAsAllCounted(BatchGroupBase, BatchGroupUsedEnd,
                                    Region->RegionBeg, /*RegionIndex=*/0,
                                    Region->MemMapInfo.AllocatedUser);
    } else {
      DCHECK_LT(NumBlocks, MaxContainedBlocks);
      // Note that we don't always visit blocks in each BatchGroup so that we
      // may miss the chance of releasing certain pages that cross
      // BatchGroups.
      Context.markFreeBlocksInRegion(
          BG.Batches, DecompactPtr, Region->RegionBeg, /*RegionIndex=*/0,
          Region->MemMapInfo.AllocatedUser, MayContainLastBlockInRegion);
    }
  }

  DCHECK(Context.hasBlockMarked());

  return Context;
}

template <typename Config>
void SizeClassAllocator64<Config>::mergeGroupsToReleaseBack(
    RegionInfo *Region, SinglyLinkedList<BatchGroupT> &GroupsToRelease)
    REQUIRES(Region->MMLock) EXCLUDES(Region->FLLock) {
  ScopedLock L(Region->FLLock);

  // After merging two freelists, we may have redundant `BatchGroup`s that
  // need to be recycled. The number of unused `BatchGroup`s is expected to be
  // small. Pick a constant which is inferred from real programs.
  constexpr uptr MaxUnusedSize = 8;
  CompactPtrT Blocks[MaxUnusedSize];
  u32 Idx = 0;
  RegionInfo *BatchClassRegion = getRegionInfo(SizeClassMap::BatchClassId);
  // We can't call pushBatchClassBlocks() to recycle the unused `BatchGroup`s
  // when we are manipulating the freelist of `BatchClassRegion`. Instead, we
  // should just push it back to the freelist when we merge two `BatchGroup`s.
  // This logic hasn't been implemented because we haven't supported releasing
  // pages in `BatchClassRegion`.
  DCHECK_NE(BatchClassRegion, Region);

  // Merge GroupsToRelease back to the Region::FreeListInfo.BlockList. Note
  // that both `Region->FreeListInfo.BlockList` and `GroupsToRelease` are
  // sorted.
  for (BatchGroupT *BG = Region->FreeListInfo.BlockList.front(),
                   *Prev = nullptr;
       ;) {
    if (BG == nullptr || GroupsToRelease.empty()) {
      if (!GroupsToRelease.empty())
        Region->FreeListInfo.BlockList.append_back(&GroupsToRelease);
      break;
    }

    DCHECK(!BG->Batches.empty());

    if (BG->CompactPtrGroupBase <
        GroupsToRelease.front()->CompactPtrGroupBase) {
      Prev = BG;
      BG = BG->Next;
      continue;
    }

    BatchGroupT *Cur = GroupsToRelease.front();
    BatchT *UnusedBatch = nullptr;
    GroupsToRelease.pop_front();

    if (BG->CompactPtrGroupBase == Cur->CompactPtrGroupBase) {
      // We have updated `BatchGroup::BytesInBGAtLastCheckpoint` while
      // collecting the `GroupsToRelease`.
      BG->BytesInBGAtLastCheckpoint = Cur->BytesInBGAtLastCheckpoint;
      const uptr MaxCachedPerBatch = BG->MaxCachedPerBatch;

      // Note that the first Batches in both `Batches` may not be
      // full and only the first Batch can have non-full blocks. Thus
      // we have to merge them before appending one to another.
      if (Cur->Batches.front()->getCount() == MaxCachedPerBatch) {
        BG->Batches.append_back(&Cur->Batches);
      } else {
        BatchT *NonFullBatch = Cur->Batches.front();
        Cur->Batches.pop_front();
        const u16 NonFullBatchCount = NonFullBatch->getCount();
        // The remaining Batches in `Cur` are full.
        BG->Batches.append_back(&Cur->Batches);

        if (BG->Batches.front()->getCount() == MaxCachedPerBatch) {
          // Only 1 non-full Batch, push it to the front.
          BG->Batches.push_front(NonFullBatch);
        } else {
          const u16 NumBlocksToMove = static_cast<u16>(
              Min(static_cast<u16>(MaxCachedPerBatch -
                                   BG->Batches.front()->getCount()),
                  NonFullBatchCount));
          BG->Batches.front()->appendFromBatch(NonFullBatch, NumBlocksToMove);
          if (NonFullBatch->isEmpty())
            UnusedBatch = NonFullBatch;
          else
            BG->Batches.push_front(NonFullBatch);
        }
      }

      const u32 NeededSlots = UnusedBatch == nullptr ? 1U : 2U;
      if (UNLIKELY(Idx + NeededSlots > MaxUnusedSize)) {
        ScopedLock L(BatchClassRegion->FLLock);
        pushBatchClassBlocks(BatchClassRegion, Blocks, Idx);
        if (conditionVariableEnabled())
          BatchClassRegion->FLLockCV.notifyAll(BatchClassRegion->FLLock);
        Idx = 0;
      }
      Blocks[Idx++] =
          compactPtr(SizeClassMap::BatchClassId, reinterpret_cast<uptr>(Cur));
      if (UnusedBatch) {
        Blocks[Idx++] = compactPtr(SizeClassMap::BatchClassId,
                                   reinterpret_cast<uptr>(UnusedBatch));
      }
      Prev = BG;
      BG = BG->Next;
      continue;
    }

    // At here, the `BG` is the first BatchGroup with CompactPtrGroupBase
    // larger than the first element in `GroupsToRelease`. We need to insert
    // `GroupsToRelease::front()` (which is `Cur` below)  before `BG`.
    //
    //   1. If `Prev` is nullptr, we simply push `Cur` to the front of
    //      FreeListInfo.BlockList.
    //   2. Otherwise, use `insert()` which inserts an element next to `Prev`.
    //
    // Afterwards, we don't need to advance `BG` because the order between
    // `BG` and the new `GroupsToRelease::front()` hasn't been checked.
    if (Prev == nullptr)
      Region->FreeListInfo.BlockList.push_front(Cur);
    else
      Region->FreeListInfo.BlockList.insert(Prev, Cur);
    DCHECK_EQ(Cur->Next, BG);
    Prev = Cur;
  }

  if (Idx != 0) {
    ScopedLock L(BatchClassRegion->FLLock);
    pushBatchClassBlocks(BatchClassRegion, Blocks, Idx);
    if (conditionVariableEnabled())
      BatchClassRegion->FLLockCV.notifyAll(BatchClassRegion->FLLock);
  }

  if (SCUDO_DEBUG) {
    BatchGroupT *Prev = Region->FreeListInfo.BlockList.front();
    for (BatchGroupT *Cur = Prev->Next; Cur != nullptr;
         Prev = Cur, Cur = Cur->Next) {
      CHECK_LT(Prev->CompactPtrGroupBase, Cur->CompactPtrGroupBase);
    }
  }

  if (conditionVariableEnabled())
    Region->FLLockCV.notifyAll(Region->FLLock);
}
} // namespace scudo

#endif // SCUDO_PRIMARY64_H_
