// ===========================================================================
// Project Astra Cosmos — src/core/ring_buffer_test.cpp
// Phase 1 (MP1) exit-criteria tests for the SPSC ring buffer (TDD T-016).
//
//   R1  fifo      : 1,000,000 items single-threaded — strict order check.
//   R2  zero-loss : 10,000,000 items, producer + consumer threads, strict
//                   in-order sequence verification (FIFO + no drops).
//   R3  bench     : throughput, must exceed 100,000,000 items/second
//                   (one "item op" = one push or one pop).
//   R4  stress    : 60 s, both sides randomized (delays, bursty work),
//                   zero loss, order preserved.
//   R5  demo      : the Phase 1 integration example — a Stream "thread"
//                   (core 1-2 role) produces, a Gen "thread" (core 3-5
//                   role) consumes slower; the ring fills, push() returns
//                   false, the producer backpressures (bounded spin retry)
//                   instead of dropping data. Stats printed at the end.
//
// Exit code 0 only if every check passes.
// ===========================================================================
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <thread>

#include "core/ring_buffer.h"

namespace {

using namespace astra;

int g_failures = 0;

#define CHECK(cond, msg, ...)                                     \
  do {                                                            \
    if (!(cond)) {                                                \
      std::printf("FAIL(line %d): %s | " msg "\n", __LINE__,     \
                  #cond, ##__VA_ARGS__);                          \
      ++g_failures;                                               \
    }                                                             \
  } while (0)

// 16-byte item: the shape a chunk descriptor will have (id + payload word).
struct Item {
  uint64_t seq;
  uint64_t payload;
};

// Tiny deterministic RNG (splitmix64) — reproducible stress runs.
struct Rng {
  uint64_t s;
  explicit Rng(uint64_t seed) : s(seed ? seed : 1) {}
  uint64_t next() {
    s += 0x9E3779B97F4A7C15ull;
    uint64_t z = s;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
  }
  uint64_t below(uint64_t n) { return next() % n; }
};

// ---------------------------------------------------------------------------
// R1: single-threaded FIFO, 1M items through a 1024-slot ring.
// ---------------------------------------------------------------------------
void test_fifo() {
  std::printf("R1: 1M items, strict FIFO order\n");
  RingBuffer<uint64_t, 1024> rb;
  constexpr uint64_t kN = 1000000;
  uint64_t produced = 0, consumed = 0;
  for (uint64_t i = 0; i < kN; ++i) {
    while (!rb.push(i)) {  // can't happen here (single thread), but be safe
      consumed++;
      uint64_t v;
      if (!rb.pop(v)) break;
      CHECK(v == consumed, "fifo: expected %llu got %llu",
            static_cast<unsigned long long>(consumed),
            static_cast<unsigned long long>(v));
    }
    produced++;
    // Drain a bit to keep the ring cycling through all slots.
    while (!rb.empty()) {
      uint64_t v;
      if (!rb.pop(v)) break;
      CHECK(v == consumed, "fifo: expected %llu got %llu",
            static_cast<unsigned long long>(consumed),
            static_cast<unsigned long long>(v));
      consumed++;
    }
  }
  while (!rb.empty()) {
    uint64_t v;
    if (!rb.pop(v)) break;
    CHECK(v == consumed, "fifo drain: expected %llu got %llu",
          static_cast<unsigned long long>(consumed),
          static_cast<unsigned long long>(v));
    consumed++;
  }
  CHECK(produced == consumed && consumed == kN, "fifo: %llu/%llu of %llu",
        static_cast<unsigned long long>(consumed),
        static_cast<unsigned long long>(produced),
        static_cast<unsigned long long>(kN));
}

// ---------------------------------------------------------------------------
// R2: 10M items across two threads, zero loss, strict order.
// The producer stops at kN; the consumer exits on the stop flag after the
// ring drains.
// ---------------------------------------------------------------------------
void test_zero_loss() {
  std::printf("R2: 10M items, 2 threads, zero loss + order\n");
  constexpr uint64_t kN = 10000000;
  RingBuffer<Item, 4096> rb;
  std::atomic<bool> done{false};
  std::atomic<uint64_t> popped{0};

  std::thread producer([&] {
    for (uint64_t i = 0; i < kN; ++i) {
      Item it{
          i,
          (i * 0x9E3779B97F4A7C15ull) ^ (i >> 8)};
      while (!rb.push(it)) {  // backpressure: retry until there is room
      }
    }
    done.store(true, std::memory_order_release);
  });

  uint64_t expect = 0;
  bool seq_ok = true;
  for (;;) {
    Item it{};
    if (rb.pop(it)) {
      const uint64_t expect_payload =
          (it.seq * 0x9E3779B97F4A7C15ull) ^ (it.seq >> 8);
      if (it.seq != expect || it.payload != expect_payload) {
        if (seq_ok) {
          std::printf("FAIL: R2 order broken at item %llu (seq %llu)\n",
                      static_cast<unsigned long long>(expect),
                      static_cast<unsigned long long>(it.seq));
        }
        seq_ok = false;
      }
      expect++;
      popped.fetch_add(1, std::memory_order_relaxed);
    } else if (done.load(std::memory_order_acquire) && rb.empty()) {
      break;
    }
  }
  producer.join();
  CHECK(popped.load() == kN, "zero-loss: %llu of %llu delivered",
        static_cast<unsigned long long>(popped.load()),
        static_cast<unsigned long long>(kN));
  CHECK(seq_ok, "zero-loss: sequence violations (see above)");
}

// ---------------------------------------------------------------------------
// R3: throughput bench. Push+pop of 8-byte items on an 8K ring (L2-resident
// counters + data). "Item ops" counts one push or one pop.
// ---------------------------------------------------------------------------
void test_bench() {
  std::printf("R3: throughput bench\n");
  RingBuffer<uint64_t, 8192> rb;
  // Pre-warm (page faults must not count).
  for (int i = 0; i < 1 << 20; ++i) {
    rb.push(static_cast<uint64_t>(i));
    uint64_t v;
    rb.pop(v);
  }
  auto t0 = std::chrono::steady_clock::now();
  const int n = 50000000;  // => 100M item ops
  uint64_t sink = 0;
  for (int i = 0; i < n; ++i) {
    rb.push(static_cast<uint64_t>(i));
    uint64_t v;
    rb.pop(v);
    sink ^= v;  // use the value so the round-trip can't be optimized away
  }
  auto t1 = std::chrono::steady_clock::now();
  (void)sink;
  const double secs = std::chrono::duration<double>(t1 - t0).count();
  const double ops_per_sec = static_cast<double>(2 * n) / secs;
  std::printf("  %.1f M item ops/s (%d iters in %.2f s)\n", ops_per_sec / 1e6,
              n, secs);
  CHECK(ops_per_sec > 100e6, "throughput %.1f M/s below the 100 M/s bar",
        ops_per_sec / 1e6);
}

// ---------------------------------------------------------------------------
// R4: 60 s stress — randomized producer/consumer with delays and bursts.
// ---------------------------------------------------------------------------
void test_stress() {
  std::printf("R4: 60 s randomized stress (hold on)...\n");
  RingBuffer<Item, 2048> rb;
  std::atomic<bool> stop{false};
  std::atomic<uint64_t> pushed{0};
  std::atomic<uint64_t> popped{0};
  std::atomic<bool> seq_bad{false};

  auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(60);

  std::thread producer([&] {
    Rng rng(0x5EED);
    uint64_t seq = 0;
    while (std::chrono::steady_clock::now() < deadline) {
      // Random delay: 0-63 ns-ish spins.
      for (uint64_t d = rng.below(64); d; --d) {}
      Item it{
          seq,
          seq ^ 0xA5A5A5A5A5A5A5A5ull};
      int tries = 0;
      while (!rb.push(it)) {
        // Backpressure: a short spin, then yield so the consumer thread
        // actually gets CPU (matters on the 2-core sandbox; on the target
        // the core pinning does this for us). The give-up threshold is a
        // liveness guard, not a performance one: a healthy consumer pops
        // in between yields, so this only trips if the consumer truly died.
        for (volatile int s = 0; s < 16; ++s) {}
        std::this_thread::yield();
        if (++tries > 2000000) {
          std::printf("FAIL: R4 producer starved (consumer stalled after "
                      "%d yields)\n",
                      tries);
          ++g_failures;
          stop.store(true, std::memory_order_release);
          return;
        }
      }
      pushed.fetch_add(1, std::memory_order_relaxed);
      ++seq;
    }
    stop.store(true, std::memory_order_release);
  });

  uint64_t expect = 0;
  Rng rng(0xC0DE);
  for (;;) {
    if (stop.load(std::memory_order_acquire) && rb.empty()) break;
    Item it{};
    if (rb.pop(it)) {
      if (it.seq != expect || it.payload != (it.seq ^ 0xA5A5A5A5A5A5A5A5ull)) {
        std::printf("FAIL: R4 order broken at %llu (got seq %llu)\n",
                    static_cast<unsigned long long>(expect),
                    static_cast<unsigned long long>(it.seq));
        seq_bad.store(true, std::memory_order_relaxed);
      }
      expect++;
      popped.fetch_add(1, std::memory_order_relaxed);
      // Random work: 0-255 spin iterations, bursty.
      for (uint64_t w = rng.below(256); w; --w) {}
    }
  }
  producer.join();
  const uint64_t p = pushed.load();
  const uint64_t q = popped.load();
  std::printf("  pushed=%llu popped=%llu (%.0f K items/s)\n",
              static_cast<unsigned long long>(p),
              static_cast<unsigned long long>(q), q / 60000.0);
  CHECK(q == p, "stress loss: %llu pushed vs %llu popped",
        static_cast<unsigned long long>(p), static_cast<unsigned long long>(q));
  CHECK(!seq_bad.load(), "stress: sequence violations");
}

// ---------------------------------------------------------------------------
// R5: integration demo — stream (producer) vs gen (consumer) with
// backpressure. The gen side consumes at ~1/2 the stream rate, so the ring
// fills and the producer must backpressure; nothing may be lost.
// ---------------------------------------------------------------------------
struct BackpressureStats {
  uint64_t produced = 0;
  uint64_t consumed = 0;
  uint64_t backpressure_events = 0;   // push() calls that returned false
  uint64_t max_backpressure_runs = 0; // longest consecutive retry streak
};

void test_backpressure_demo() {
  std::printf("R5: stream->gen backpressure demo\n");
  constexpr uint64_t kTotal = 2000000;
  RingBuffer<Item, 256> rb;  // deliberately small: fills fast
  std::atomic<bool> done{false};
  BackpressureStats st{};
  std::atomic<uint64_t> gen_work{0};

  // Stream thread: pushes as fast as it can; on a full ring it counts the
  // backpressure and retries — the contract is "wait for gen, never drop".
  std::thread stream([&] {
    uint64_t streak = 0;
    for (uint64_t i = 0; i < kTotal; ++i) {
      Item it{i, i * 2};
      while (!rb.push(it)) {
        ++st.backpressure_events;
        ++streak;
        for (volatile int s = 0; s < 8; ++s) {}  // brief spin before retry
      }
      if (streak > st.max_backpressure_runs) st.max_backpressure_runs = streak;
      streak = 0;
      st.produced++;
    }
    done.store(true, std::memory_order_release);
  });

  // Gen thread: consumes, but does ~2x the work per item (simulated shader
  // job), so it is the slow side.
  uint64_t expect = 0;
  for (;;) {
    Item it{};
    if (rb.pop(it)) {
      CHECK(it.seq == expect, "demo: order broken at %llu",
            static_cast<unsigned long long>(expect));
      expect++;
      st.consumed++;
      // Simulated generation work: ~2x the push cost.
      for (volatile int w = 0; w < 4; ++w) gen_work.fetch_add(1,
                                                              std::memory_order_relaxed);
    } else if (done.load(std::memory_order_acquire) && rb.empty()) {
      break;
    }
  }
  stream.join();
  std::printf("  produced=%llu consumed=%llu backpressure_events=%llu "
              "max_consecutive_retries=%llu\n",
              static_cast<unsigned long long>(st.produced),
              static_cast<unsigned long long>(st.consumed),
              static_cast<unsigned long long>(st.backpressure_events),
              static_cast<unsigned long long>(st.max_backpressure_runs));
  CHECK(st.consumed == kTotal, "demo: %llu consumed of %llu (loss!)",
        static_cast<unsigned long long>(st.consumed),
        static_cast<unsigned long long>(kTotal));
  CHECK(st.backpressure_events > 0,
        "demo: expected backpressure to trigger on a 256-slot ring");
}

}  // namespace

int main() {
  std::printf("== Astra SPSC ring buffer test ==\n");
  test_fifo();
  test_zero_loss();
  test_bench();
  test_stress();
  test_backpressure_demo();
  if (g_failures) {
    std::printf("== RING TEST: %d FAILURES ==\n", g_failures);
    return 1;
  }
  std::printf("== RING TEST: ALL PASS ==\n");
  return 0;
}
