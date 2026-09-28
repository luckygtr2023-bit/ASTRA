// ===========================================================================
// Project Astra Cosmos — src/memory/memory_budget.cpp
// Implementation. See memory_budget.h for the budget rationale.
//
// Note on enforcement model: the per-category budgets are ACCOUNTING lines
// (sum of charged bytes per category). The 8 GB slab pool itself is shared
// across categories by size class — a category within its line is not
// physically ring-fenced. Physical exhaustion surfaces as the allocator's
// OOM counter (never a fallback malloc); budget breach surfaces here.
// The two together give both guarantees the TDD asks for: the game can
// neither silently over-draw the RAM budget nor silently fall back to the
// system heap.
// ===========================================================================
#include "memory/memory_budget.h"

#include <cstdio>
#include <cstdlib>  // std::abort (debug breach enforcement)

namespace astra {

void MemoryBudget::register_pool(uint64_t pool_bytes) {
  pool_bytes_.store(pool_bytes, std::memory_order_relaxed);
  for (auto& c : cats_) {
    c.in_use.store(0, std::memory_order_relaxed);
    c.peak.store(0, std::memory_order_relaxed);
    c.violations.store(0, std::memory_order_relaxed);
  }
  total_peak_.store(0, std::memory_order_relaxed);
}

void MemoryBudget::on_breach(int cat, uint64_t in_use) {
  cats_[cat].violations.fetch_add(1, std::memory_order_relaxed);
  const char* names[static_cast<int>(MemCategory::COUNT)] = {
      "ENGINE", "CHUNKS", "ASSETS", "AUDIO", "UI"};
  std::fprintf(stderr,
               "[budget] BREACH: %s at %llu MB over its %llu MB line\n",
               names[cat],
               static_cast<unsigned long long>(in_use >> 20),
               static_cast<unsigned long long>(kBudget[cat] >> 20));
#ifndef NDEBUG
  // Debug builds: a breach is a stop-the-world design bug.
  std::abort();
#endif
}

MemoryBudget::Stats MemoryBudget::snapshot() {
  // Cold path (HUD / tests / reports). The total is aggregated here rather
  // than maintained on the hot path — see charge() in the header.
  Stats s{};
  uint64_t total = 0;
  uint64_t total_violations = 0;
  for (int k = 0; k < static_cast<int>(MemCategory::COUNT); ++k) {
    s.category[k].in_use = cats_[k].in_use.load(std::memory_order_relaxed);
    s.category[k].peak = cats_[k].peak.load(std::memory_order_relaxed);
    s.category[k].budget = kBudget[k];
    s.category[k].alloc_count =
        SlabAllocator::get().alloc_count(static_cast<MemCategory>(k));
    s.category[k].violations =
        cats_[k].violations.load(std::memory_order_relaxed);
    total += s.category[k].in_use;
    total_violations += s.category[k].violations;
    if (s.category[k].in_use > kBudget[k]) s.breach_now = true;
  }
  s.total_in_use = total;
  // Low-frequency total-peak observation: atomic-max against the current
  // total (HUD-rate resolution is plenty for a peak meter).
  uint64_t tp = total_peak_.load(std::memory_order_relaxed);
  if (total > tp)
    total_peak_.compare_exchange_weak(tp, total, std::memory_order_relaxed,
                                      std::memory_order_relaxed);
  s.total_peak = total_peak_.load(std::memory_order_relaxed);
  s.total_budget = kTotalBudget;
  s.pool_bytes = pool_bytes_.load(std::memory_order_relaxed);
  s.total_violations = total_violations;
  // Safety net (logically subsumed by the per-category lines, which sum
  // to exactly kTotalBudget): keep the flag honest if arithmetic ever
  // drifts.
  if (total > kTotalBudget) s.breach_now = true;
  return s;
}

void MemoryBudget::reset_peaks() {
  for (auto& c : cats_) c.peak.store(0, std::memory_order_relaxed);
  total_peak_.store(0, std::memory_order_relaxed);
}

}  // namespace astra
