// ===========================================================================
// Project Astra Cosmos — src/memory/memory_budget.h
// Phase 1 (MP1): global RAM-budget tracker (TDD T-006 pools, TDD T-015).
//
// The 8 GB budget (target: Ryzen 7 170 / 16 GB laptop, RAM cap 8 GB):
//
//   category   budget     purpose
//   --------   --------   -------------------------------------------
//   CHUNKS     4 GB       procedural chunk streaming pool
//   ASSETS     2 GB       pre-cached .astroct asset pool (textures, tables)
//   ENGINE     1 GB       renderer state, job system, nav, audio-graph host
//   AUDIO      500 MB     audio pool (its own <50 MB active cap, TDD T-013)
//   UI         500 MB     HUD, atlas cache, wayfinding, misc
//
// Every slab alloc/free flows through this tracker (charge/release), so
// the numbers below are the ground truth for the debug HUD overlay and for
// the startup/shutdown leak report.
//
// Enforcement: a budget breach is a design bug. Debug builds assert (hard
// fail — a breach in development must stop the session, not degrade it).
// Release builds count and log the breach and keep running, because a
// shipping crash is worse than a logged over-budget frame; the HUD shows
// violations so it stays visible in QA.
//
// Thread-safety: relaxed atomics throughout — these counters are
// observation points on the hot path and must not contend.
// ===========================================================================
#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>

#include "memory/slab_allocator.h"  // MemCategory

namespace astra {

class MemoryBudget {
 public:
  static inline MemoryBudget& get() {
    static MemoryBudget instance;
    return instance;
  }

  // The 8 GB per-category split (bytes). Index: MemCategory.
  static constexpr uint64_t kBudget[static_cast<int>(MemCategory::COUNT)] = {
      1ull << 30,       // ENGINE : 1 GB
      4ull << 30,       // CHUNKS : 4 GB
      2ull << 30,       // ASSETS : 2 GB
      512ull << 20,     // AUDIO  : 500 MB
      512ull << 20,     // UI     : 500 MB
  };
  static constexpr uint64_t kTotalBudget = 8ull << 30;  // 8 GB

  // Called by the slab allocator's init() with the real pool size.
  void register_pool(uint64_t pool_bytes);

  // Hot path: one alloc / one free. Relaxed atomics, no locks, no logs on
  // the happy path. Inlined in the header: allocate/free call these every
  // time, and the >10M alloc/s budget leaves no room for call overhead.
  //
  // Deliberately minimal per call:
  //  * only the per-category in_use atomic is touched (one RMW each
  //    way). The total is NOT maintained as a separate hot atomic — it is
  //    the sum of the five category lines, which is computed in snapshot()
  //    (HUD frequency). A shared total counter would be a single cache
  //    line contended by every thread on every alloc/free.
  //  * the total 8 GB check is subsumed by the per-category checks: the
  //    category lines sum to exactly kTotalBudget, so the total can only
  //    be exceeded if some category breaches its line (which asserts).
  //  * the per-category allocation count lives in the slab allocator (the
  //    component that performs allocations); snapshot() reads it from
  //    there, so charge() does not duplicate the RMW.
  inline void charge(std::size_t bytes, MemCategory cat) {
    const int k = static_cast<int>(cat);
    const uint64_t now = cats_[k].in_use.fetch_add(bytes,
                                                   std::memory_order_relaxed) +
                        bytes;
    if (now > cats_[k].peak.load(std::memory_order_relaxed))
      update_peak(cats_[k]);
    if (now > kBudget[k]) on_breach(k, now);
  }
  inline void release(std::size_t bytes, MemCategory cat) {
    const int k = static_cast<int>(cat);
    cats_[k].in_use.fetch_sub(bytes, std::memory_order_relaxed);
  }

  // Snapshot for the debug HUD overlay / tests / startup-shutdown report.
  struct CategoryStats {
    uint64_t in_use;
    uint64_t peak;
    uint64_t budget;
    uint64_t alloc_count;
    uint64_t violations;
  };
  struct Stats {
    CategoryStats category[static_cast<int>(MemCategory::COUNT)];
    uint64_t total_in_use;
    uint64_t total_peak;
    uint64_t total_budget;
    uint64_t pool_bytes;
    uint64_t total_violations;
    bool breach_now;  // any category (or the total) over budget now
  };
  // Non-const on purpose: this is also where the low-frequency (HUD-rate)
  // total-peak observation is updated. Never called from the hot path.
  Stats snapshot();

  void reset_peaks();  // for per-act / per-scene peak tracking

 private:
  MemoryBudget() = default;

  struct Cat {
    std::atomic<uint64_t> in_use{0};
    std::atomic<uint64_t> peak{0};
    std::atomic<uint64_t> violations{0};
    // Note: no per-category alloc_count here — the slab allocator is the
    // component that performs allocations and already maintains
    // alloc_counts_[cat] on the hot path (it is also what the leak check
    // verifies). snapshot() exposes that count so the budget stats remain
    // complete without a second atomic RMW per alloc.
  };

  inline void update_peak(Cat& c) {
    // CAS max — relaxed, contention-tolerant "atomic max".
    for (;;) {
      uint64_t cur = c.peak.load(std::memory_order_relaxed);
      uint64_t now = c.in_use.load(std::memory_order_relaxed);
      if (now <= cur) return;
      if (c.peak.compare_exchange_weak(cur, now, std::memory_order_relaxed,
                                       std::memory_order_relaxed))
        return;
    }
  }
  void on_breach(int cat, uint64_t in_use);
  // No total-in-use counter: it is the sum of the category lines (see
  // charge()), computed in snapshot(). A shared total would be one cache
  // line contended by every thread on every alloc/free.

  Cat cats_[static_cast<int>(MemCategory::COUNT)];
  std::atomic<uint64_t> total_peak_{0};  // HUD-rate, updated in snapshot()
  std::atomic<uint64_t> pool_bytes_{0};
};

}  // namespace astra
