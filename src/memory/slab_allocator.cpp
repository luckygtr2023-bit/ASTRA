// ===========================================================================
// Project Astra Cosmos — src/memory/slab_allocator.cpp
// Implementation: pool reservation (Windows VirtualAlloc / POSIX mmap),
// slab linking, O(1) lock-free allocate/free, per-category stats.
// See slab_allocator.h for the full design rationale (TDD T-015).
// ===========================================================================
#include "memory/slab_allocator.h"

#include <cstdio>
#include <new>

#include "memory/memory_budget.h"

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#else
#include <sys/mman.h>
#endif

namespace astra {

// ---------------------------------------------------------------------------
// class_for: smallest size class that fits `bytes`.
// Seven classes: the loop is at most 7 iterations — cheap against a
// >10M alloc/sec budget, and branch-predictable in every real workload.
// ---------------------------------------------------------------------------
int SlabAllocator::class_for(std::size_t bytes) {
  for (int c = 0; c < SlabConfig::kClassCount; ++c) {
    if (bytes <= SlabConfig::kSizes[c]) return c;
  }
  return -1;  // larger than 1 MB: outside the slab pool (design error)
}

// ---------------------------------------------------------------------------
// Platform pool reservation. One contiguous region; the OS guarantees the
// base is at least page-aligned (4 KB on Linux, 64 KB on Windows), which
// already implies 16 B-aligned block starts throughout, because every class
// size is a multiple of 16.
// ---------------------------------------------------------------------------
namespace {

bool reserve_pool(uint8_t** base, uint64_t bytes) {
#if defined(_WIN32)
  // RESERVE + COMMIT the whole pool in one call. Committing is a
  // virtual-memory operation: physical pages fault in on first touch, so at
  // startup this costs zero physical RAM on the 16 GB laptop. The 8 GB
  // figure in the TDD RAM budget is an address-space reservation — exactly
  // what it means there.
  void* p = VirtualAlloc(nullptr, static_cast<SIZE_T>(bytes),
                         MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
  if (!p) return false;
  *base = static_cast<uint8_t*>(p);
  return true;
#else
  void* p = mmap(nullptr, bytes, PROT_READ | PROT_WRITE,
                 MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (p == MAP_FAILED) return false;
  *base = static_cast<uint8_t*>(p);
  return true;
#endif
}

void release_pool(uint8_t* base, uint64_t bytes) {
#if defined(_WIN32)
  VirtualFree(base, 0, MEM_RELEASE);
#else
  munmap(base, bytes);
#endif
}

}  // namespace

// ---------------------------------------------------------------------------
// init: reserve the pool, carve it into class regions, link every slab's
// free list. Single-threaded, startup only.
// ---------------------------------------------------------------------------
bool SlabAllocator::init(uint64_t pool_bytes) {
  if (ready_) return true;
  if (pool_bytes < 64ull << 20) {  // dev floor: 64 MB keeps tests sane
    std::fprintf(stderr, "[slab] pool_bytes below 64 MB floor — refusing\n");
    return false;
  }
  // Region splits scale proportionally to the requested pool size so dev
  // profiles keep the same shape as the shipping 8 GB pool.
  const uint64_t scale_num = pool_bytes;
  const uint64_t scale_den = SlabConfig::kPoolBytes;

  if (!reserve_pool(&pool_base_, pool_bytes)) {
    std::fprintf(stderr, "[slab] failed to reserve %llu MB pool\n",
                 static_cast<unsigned long long>(pool_bytes >> 20));
    return false;
  }
  pool_bytes_ = pool_bytes;

  uint64_t offset = 0;
  for (int c = 0; c < SlabConfig::kClassCount; ++c) {
    // Scaled region, rounded down to whole slabs.
    const uint64_t region = (SlabConfig::kRegions[c] * scale_num) / scale_den;
    const uint64_t slab_size = SlabConfig::SlabSize(c);
    const uint32_t slabs = static_cast<uint32_t>(region / slab_size);
    const uint64_t used = static_cast<uint64_t>(slabs) * slab_size;

    ClassState& cs = classes_[c];
    cs.base = pool_base_ + offset;
    cs.region_bytes = used;
    cs.slab_count = slabs;
    for (auto& r : cs.rr) r.store(0, std::memory_order_relaxed);
    offset += used;

    // Slab header array: 64 B per slab. ~97k slabs * 64 B ≈ 6 MB of
    // metadata total, deliberately OUTSIDE the 8 GB pool (the pool is the
    // engine's data; the index is the engine's bookkeeping). This is the
    // only allocation inside init() — startup is explicitly allowed to
    // allocate (TDD T-015: zero allocations during gameplay).
    void* meta = ::operator new(static_cast<std::size_t>(slabs) * sizeof(Slab),
                                std::align_val_t(64));
    cs.slabs = static_cast<Slab*>(meta);

    uint8_t* b = cs.base;
    for (uint32_t s = 0; s < slabs; ++s) {
      cs.slabs[s].init(b, SlabConfig::BlocksPerSlab(c), SlabConfig::kSizes[c]);
      b += slab_size;
    }
    cs.live_blocks.store(0, std::memory_order_relaxed);
    cs.oom.store(0, std::memory_order_relaxed);
  }
  for (int k = 0; k < static_cast<int>(MemCategory::COUNT); ++k) {
    alloc_counts_[k].store(0, std::memory_order_relaxed);
    free_counts_[k].store(0, std::memory_order_relaxed);
  }
  oom_total_.store(0, std::memory_order_relaxed);
  ready_ = true;

  MemoryBudget::get().register_pool(pool_bytes);
  std::printf("[slab] pool ready: %llu MB, %d classes\n",
              static_cast<unsigned long long>(pool_bytes >> 20),
              SlabConfig::kClassCount);
  return true;
}

void SlabAllocator::shutdown() {
  if (!ready_) return;
  // The caller (main) prints the budget report before this call; any
  // residue is a leak and shows up there.
  for (int c = 0; c < SlabConfig::kClassCount; ++c) {
    if (classes_[c].slabs) {
      ::operator delete(reinterpret_cast<void*>(classes_[c].slabs),
                         std::align_val_t(64));
      classes_[c].slabs = nullptr;
    }
  }
  release_pool(pool_base_, pool_bytes_);
  pool_base_ = nullptr;
  ready_ = false;
}

// ---------------------------------------------------------------------------
// allocate: O(1).
//   1. Pick the size class.
//   2. Pick a slab via this thread's round-robin cursor — with 8 threads
//      and up to 32k slabs per class, concurrent allocates almost never
//      touch the same Treiber head, which is what keeps the DWCAS
//      contention-free (that is the point of slabbing).
//   3. CAS-pop; on a drained slab roll to the next one (bounded), after
//      which it is genuine pool exhaustion: counted, logged, nullptr.
// ---------------------------------------------------------------------------
void* SlabAllocator::allocate(std::size_t bytes, MemCategory category) {
  if (!ready_ || bytes == 0) return nullptr;
  const int c = class_for(bytes);
  if (c < 0) {
    oom_total_.fetch_add(1, std::memory_order_relaxed);
    std::fprintf(stderr,
                 "[slab] OOM: request of %zu bytes exceeds the 1 MB class — "
                 "outside the pool by design (use the .astroct pool)\n",
                 bytes);
    return nullptr;
  }

  ClassState& cs = classes_[c];
  // Thread slot: stable per thread for its lifetime; the low 5 bits keep
  // the cursor array bounded at 32 (two threads sharing a slot after a
  // wrap is harmless — the cursor is a locality hint, not ownership).
  static thread_local const uint32_t slot = [] {
    static std::atomic<uint32_t> counter{0};
    return counter.fetch_add(1, std::memory_order_relaxed) & 31u;
  }();

  const uint32_t start = cs.rr[slot].fetch_add(1, std::memory_order_relaxed);
  for (uint32_t tries = 0; tries < cs.slab_count; ++tries) {
    Slab* slab = &cs.slabs[(start + tries) % cs.slab_count];
    uint8_t* block = slab->pop();
    if (!block) continue;  // this slab is drained — try the next

    auto* h = reinterpret_cast<SlabBlockHeader*>(block);
    h->class_idx = static_cast<uint32_t>(c);
    h->category = static_cast<uint32_t>(category);
    cs.live_blocks.fetch_add(1, std::memory_order_relaxed);
    alloc_counts_[static_cast<int>(category)].fetch_add(1,
                                                        std::memory_order_relaxed);
    MemoryBudget::get().charge(SlabConfig::kSizes[c], category);
    return block + sizeof(SlabBlockHeader);  // 16 B-aligned user pointer
  }

  // Every slab in the class is empty: pool exhaustion for this class.
  cs.oom.fetch_add(1, std::memory_order_relaxed);
  oom_total_.fetch_add(1, std::memory_order_relaxed);
  std::fprintf(stderr,
               "[slab] OOM: %llu B class exhausted (%llu bytes in use of %llu)\n",
               static_cast<unsigned long long>(SlabConfig::kSizes[c]),
               static_cast<unsigned long long>(class_bytes_in_use(c)),
               static_cast<unsigned long long>(cs.region_bytes));
  return nullptr;
}

// ---------------------------------------------------------------------------
// free: O(1). The 16-byte header records exactly where the block came from,
// so no size argument, no lookup table, no lock.
// ---------------------------------------------------------------------------
void SlabAllocator::free(void* ptr) {
  if (!ready_ || !ptr) return;
  auto* h = reinterpret_cast<SlabBlockHeader*>(
      static_cast<uint8_t*>(ptr) - sizeof(SlabBlockHeader));
  const int c = h->class_idx;
  if (c < 0 || c >= SlabConfig::kClassCount) {
    std::fprintf(stderr, "[slab] free(): corrupt header (class %d)\n", c);
    return;
  }
  const MemCategory cat = static_cast<MemCategory>(h->category);
  ClassState& cs = classes_[c];
  cs.live_blocks.fetch_sub(1, std::memory_order_relaxed);
  free_counts_[static_cast<int>(cat)].fetch_add(1, std::memory_order_relaxed);
  MemoryBudget::get().release(SlabConfig::kSizes[c], cat);
  // Slab ownership is positional: offset into the class region / slab size.
  uint8_t* block = reinterpret_cast<uint8_t*>(h);
  const uint32_t slab_idx = static_cast<uint32_t>(
      static_cast<uint64_t>(block - cs.base) / SlabConfig::SlabSize(c));
  cs.slabs[slab_idx].push(block);
}

// ---------------------------------------------------------------------------
// Statistics (budget tracker, debug HUD, tests).
// ---------------------------------------------------------------------------
uint64_t SlabAllocator::class_bytes_in_use(int c) const {
  return class_live_blocks(c) * SlabConfig::kSizes[c];
}
uint64_t SlabAllocator::class_live_blocks(int c) const {
  return classes_[c].live_blocks.load(std::memory_order_relaxed);
}
uint64_t SlabAllocator::class_total_blocks(int c) const {
  return static_cast<uint64_t>(classes_[c].slab_count) *
         SlabConfig::BlocksPerSlab(c);
}
uint64_t SlabAllocator::class_free_blocks(int c) const {
  return class_total_blocks(c) - class_live_blocks(c);
}
uint64_t SlabAllocator::class_oom_count(int c) const {
  return classes_[c].oom.load(std::memory_order_relaxed);
}
uint64_t SlabAllocator::alloc_count(MemCategory c) const {
  return alloc_counts_[static_cast<int>(c)].load(std::memory_order_relaxed);
}
uint64_t SlabAllocator::free_count(MemCategory c) const {
  return free_counts_[static_cast<int>(c)].load(std::memory_order_relaxed);
}

}  // namespace astra
