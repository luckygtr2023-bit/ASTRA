// ===========================================================================
// Project Astra Cosmos — src/memory/slab_allocator.h
// Phase 1 (MP1): zero-runtime-allocation slab allocator.
//
// Design (decision log: TDD T-015)
// --------------------------------
// * ONE contiguous 8 GB virtual address reservation at startup
//   (VirtualAlloc MEM_RESERVE on Windows, mmap PROT_NONE elsewhere).
//   Committing the pool costs no physical RAM — pages are faulted in on
//   first touch, so the 16 GB laptop sees exactly what the game touches.
//
// * The pool is carved into 7 fixed size classes:
//       64 B | 256 B | 1 KB | 4 KB | 16 KB | 64 KB | 1 MB
//   Each class owns a static region of the pool (splits below sum to 8 GB).
//   Fixed-size blocks are the anti-fragmentation argument: there is no
//   splitting, no coalescing, no free-list bookkeeping by address — a freed
//   block always fits back into the exact block it came from.
//
// * Each class region is divided into slabs (64 KB of blocks per slab;
//   1 MB blocks are 1 block per slab). Every slab owns its own free list —
//   a lock-free Treiber stack whose head is a 128-bit tagged pointer
//   (block pointer + monotonically increasing tag) updated with a 128-bit
//   compare-exchange (cmpxchg16b). The tag makes ABA impossible: a node that
//   is popped and re-pushed between two of our operations cannot make our
//   stale head pointer look current, because the tag has advanced.
//   No mutexes anywhere in the allocation path.
//
// * Allocation is O(1): pick a slab (per-thread round-robin, so 8 threads
//   contend on 8 different slab heads, not one), CAS-pop its free list.
//   Deallocation is O(1): CAS-push. The only way allocate() returns null is
//   total pool exhaustion — a design failure that is counted and reported,
//   never silently worked around with a fallback malloc.
//
// * Block header (16 bytes, in-block): next-pointer (8) + size-class index
//   (4) + allocation category (4). deallocate() therefore needs only the
//   pointer, and the memory budget tracker sees every alloc/free event.
//   User pointers are 16-byte aligned (blocks are 64 B aligned; +16 keeps
//   16 B alignment in every class). Callers needing wider alignment size up
//   to the next class — standard slab semantics.
//
// * Per-category accounting (ENGINE, CHUNKS, ASSETS, AUDIO, UI) feeds
//   MemoryBudget, which enforces the 8 GB / per-category RAM budget (TDD).
//
// Thread-safety model: each slab head is touched only via DWCAS (safe from
// any number of threads); per-thread round-robin counters are relaxed
// atomics; budget counters are relaxed atomics. No lock is ever taken.
//
// Platform: MinGW-w64 x86_64 is the shipping toolchain (TDD T-001); the
// 128-bit CAS uses the GCC/Clang builtin, so any GCC targeting x86_64
// (Windows or Linux) is supported.
// ===========================================================================
#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>

namespace astra {

// ---------------------------------------------------------------------------
// Allocation categories — one per RAM-budget line (TDD T-006 pools).
// ---------------------------------------------------------------------------
enum class MemCategory : uint32_t {
  ENGINE = 0,
  CHUNKS = 1,
  ASSETS = 2,
  AUDIO = 3,
  UI = 4,
  COUNT = 5,
};

// ---------------------------------------------------------------------------
// Pool constants (the 8 GB layout; the only tunables of the allocator).
// Splits are deliberately coarse — MP1 benchmarks will tune them against the
// real chunk/asset workload without touching any code path.
// ---------------------------------------------------------------------------
struct SlabConfig {
  static constexpr int kClassCount = 7;

  // Class sizes (bytes). Powers of two so class_for() is a 2-comparison loop.
  static constexpr uint64_t kSizes[kClassCount] = {
      64, 256, 1024, 4096, 16384, 65536, 1ull << 20};

  // Static region per class (bytes). Sum = exactly 8 GB.
  //   small blocks (many of them) get a quarter; 4K gets a sixth; the big
  //   classes (chunk tiles, star tables) get half of the pool.
  static constexpr uint64_t kRegions[kClassCount] = {
      512ull << 20,  // 64B   : 512 MB  (8,192 slabs  x 1024 blocks)
      512ull << 20,  // 256B  : 512 MB  (8,192 slabs  x 256 blocks)
      512ull << 20,  // 1KB   : 512 MB  (8,192 slabs  x 64 blocks)
      1ull << 30,    // 4KB   : 1 GB    (16,384 slabs x 16 blocks)
      1536ull << 20, // 16KB  : 1.5 GB  (24,576 slabs x 4 blocks)
      2ull << 30,    // 64KB  : 2 GB    (32,768 slabs x 1 block)
      2ull << 30,    // 1MB   : 2 GB    (2,048 slabs  x 1 block)
  };

  static constexpr uint64_t kPoolBytes = 8ull << 30;  // 8 GB
  static constexpr uint64_t kSlabBytes = 64ull << 10; // 64 KB of blocks per slab

  static constexpr uint64_t SlabSize(int c) {
    // 64 KB of blocks, unless the class itself is bigger (1 MB).
    return kSizes[c] > kSlabBytes ? kSizes[c] : kSlabBytes;
  }
  static constexpr uint32_t BlocksPerSlab(int c) {
    return static_cast<uint32_t>(SlabSize(c) / kSizes[c]);
  }
  static constexpr uint32_t SlabCount(int c) {
    return static_cast<uint32_t>(kRegions[c] / SlabSize(c));
  }
};

// Block header: 16 bytes, lives at the start of every block. The free-list
// "next" pointer doubles as the header while the block is free.
struct alignas(16) SlabBlockHeader {
  uint64_t next;       // free list: next block; live: unused
  uint32_t class_idx;  // which size class stamped this block
  uint32_t category;   // MemCategory it was allocated for (budget tracking)
};
static_assert(sizeof(SlabBlockHeader) == 16, "header must be 16 bytes");
static_assert(alignof(SlabBlockHeader) == 16, "header must be 16-aligned");

// ---------------------------------------------------------------------------
// 128-bit tagged CAS (ABA-proof Treiber stack).
// Target is MinGW-w64 / GCC on x86_64, where cmpxchg16b exists; GCC exposes
// it through the generic __atomic builtins for 16-byte aligned objects.
// ---------------------------------------------------------------------------
// The 128-bit CAS value is a canonical unsigned __int128: the LOW 64 bits
// hold the block pointer, the HIGH 64 bits hold the ABA tag. x86_64 user
// pointers are 48 bits, so the pointer fits the low half with room to spare.
// GCC/Clang accept __atomic_compare_exchange_n only for native integer types,
// hence __int128 rather than a struct (the struct form is rejected at the
// frontend). The tag increments monotonically; a wrap colliding with a live
// (ptr, tag) pair is astronomically unlikely and still safe (a CAS retry).
//
// Implementation note: GCC routes 128-bit __atomic builtins through the
// libatomic helper __atomic_compare_exchange_16 (a cmpxchg16b loop when the
// target has cx16 — see -mcx16 in CMakeLists). libatomic is part of the GCC
// toolchain, so this stays within the "no external libraries" constraint;
// the build links it explicitly (-latomic / MinGW's static libatomic.a).
// Measured in the build sandbox: ~60M CAS/s uncontended single-thread,
// comfortably inside the 10M alloc/s budget (a pop is one such CAS).
using Tagged128 = unsigned __int128;

inline Tagged128 make_tagged(uint64_t ptr, uint64_t tag) {
  return Tagged128(ptr) | (Tagged128(tag) << 64);
}
inline uint64_t tagged_ptr(Tagged128 v) { return static_cast<uint64_t>(v); }
inline uint64_t tagged_tag(Tagged128 v) {
  return static_cast<uint64_t>(v >> 64);
}

inline bool tagged_cas(Tagged128* dst, Tagged128* expected, Tagged128 desired) {
#if defined(__GNUC__) || defined(__clang__)
  return __atomic_compare_exchange_n(dst, expected, desired,
                                     /*weak=*/false,
                                     __ATOMIC_ACQ_REL,
                                     __ATOMIC_RELAXED);
#else
#error "slab_allocator requires a 128-bit CAS (GCC/Clang x86_64; MinGW-w64 per TDD T-001)"
#endif
}

// ---------------------------------------------------------------------------
// Slab: one 64 KB (or 1 MB) region of fixed-size blocks + its free list.
// 64-byte aligned so the head never shares a cache line with a neighbour.
// ---------------------------------------------------------------------------
struct alignas(64) Slab {
  Tagged128 head;  // free list (DWCAS Treiber stack)

  void init(uint8_t* base, uint32_t blocks_per_slab, uint64_t block_size) {
    // Every block starts empty on the free list: block i's next = i+1,
    // last block's next = 0 (null). Single-threaded, at startup only.
    uint8_t* b = base;
    for (uint32_t i = 0; i < blocks_per_slab; ++i) {
      auto* h = reinterpret_cast<SlabBlockHeader*>(b);
      h->next = (i + 1 < blocks_per_slab)
                    ? reinterpret_cast<uint64_t>(b + block_size)
                    : 0;
      b += block_size;
    }
    head = make_tagged(reinterpret_cast<uint64_t>(base), 1);
  }

  // O(1) lock-free pop. Returns nullptr when the slab is empty.
  uint8_t* pop() {
    for (;;) {
      Tagged128 expected = head;
      const uint64_t ptr = tagged_ptr(expected);
      if (ptr == 0) return nullptr;  // empty
      auto* node = reinterpret_cast<SlabBlockHeader*>(ptr);
      const Tagged128 desired =
          make_tagged(node->next, tagged_tag(expected) + 1);
      if (tagged_cas(&head, &expected, desired))
        return reinterpret_cast<uint8_t*>(ptr);
      // CAS failed: expected was refreshed by tagged_cas — retry.
    }
  }

  // O(1) lock-free push. `block` is not on any free list yet (the caller
  // just freed it), so stamping node->next here races with no one.
  void push(uint8_t* block) {
    for (;;) {
      Tagged128 expected = head;
      auto* node = reinterpret_cast<SlabBlockHeader*>(block);
      node->next = tagged_ptr(expected);
      const Tagged128 desired = make_tagged(
          reinterpret_cast<uint64_t>(block), tagged_tag(expected) + 1);
      if (tagged_cas(&head, &expected, desired)) return;
    }
  }
};
static_assert(alignof(Slab) == 64, "slab heads must be cache-line aligned");

// ---------------------------------------------------------------------------
// SlabAllocator — process-wide singleton. init()/shutdown() bracket the
// whole application; everything between must go through allocate/free.
// ---------------------------------------------------------------------------
class SlabAllocator {
 public:
  static inline SlabAllocator& get() {
    static SlabAllocator instance;  // static init runs before main() — safe
    return instance;
  }

  // Reserves + commits the pool and links every slab's free list.
  // pool_bytes may be reduced for dev/test profiles (regions scale
  // proportionally); the shipping app uses the full 8 GB default.
  // Returns false (and leaves the allocator unusable) if the reservation
  // fails — at startup there is no fallback, only a clean abort upstream.
  bool init(uint64_t pool_bytes = SlabConfig::kPoolBytes);
  void shutdown();

  // O(1). Returns a 16 B-aligned pointer holding >= bytes, or nullptr on
  // pool exhaustion (counted, reported — never a hidden malloc).
  void* allocate(std::size_t bytes, MemCategory category);

  // O(1). The pointer must have come from allocate(). nullptr is ignored.
  void free(void* ptr);

  // -- statistics (for the budget tracker / debug HUD / tests) -------------
  uint64_t total_pool_bytes() const { return pool_bytes_; }
  uint64_t class_bytes_in_use(int c) const;
  uint64_t class_live_blocks(int c) const;
  uint64_t class_total_blocks(int c) const;
  uint64_t class_free_blocks(int c) const;   // total - live
  uint64_t class_oom_count(int c) const;
  uint64_t alloc_count(MemCategory c) const;
  uint64_t free_count(MemCategory c) const;
  uint64_t oom_total() const { return oom_total_.load(std::memory_order_relaxed); }

 private:
  SlabAllocator() = default;
  static int class_for(std::size_t bytes);   // smallest class index >= bytes

  struct ClassState {
    uint8_t* base = nullptr;
    Slab* slabs = nullptr;      // slab_count entries (BSS, startup-linked)
    uint32_t slab_count = 0;
    uint64_t region_bytes = 0;
    std::atomic<uint64_t> live_blocks{0};    // category-agnostic
    std::atomic<uint64_t> oom{0};
    // Per-thread round-robin slab cursors: relaxed atomics (a wrapped
    // cursor is only a locality hint; two threads sharing a slot is safe).
    std::atomic<uint32_t> rr[32];
  };

  uint8_t* pool_base_ = nullptr;
  uint64_t pool_bytes_ = 0;
  bool ready_ = false;
  ClassState classes_[SlabConfig::kClassCount];
  std::atomic<uint64_t> alloc_counts_[static_cast<int>(MemCategory::COUNT)];
  std::atomic<uint64_t> free_counts_[static_cast<int>(MemCategory::COUNT)];
  std::atomic<uint64_t> oom_total_{0};
};

// The single place where the rest of the engine allocates. Kept tiny and
// inlined so call sites read like the C++ they replace.
inline void* slab_alloc(std::size_t bytes, MemCategory c) {
  return SlabAllocator::get().allocate(bytes, c);
}
inline void slab_free(void* p) { SlabAllocator::get().free(p); }

}  // namespace astra
