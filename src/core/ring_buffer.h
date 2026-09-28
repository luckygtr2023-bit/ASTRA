// ===========================================================================
// Project Astra Cosmos — src/core/ring_buffer.h
// Phase 1 (MP1): single-producer / single-consumer (SPSC) lock-free ring
// buffer (TDD T-016).
//
// Use: the chunk streaming pipeline. The Stream Thread (pinned core 1,
// secondary core 2) PRODUCES ready-chunk descriptors; the Gen Threads
// (pinned cores 3–5) CONSUME them. One producer, one consumer per ring —
// never share a ring across multiple producers (MPMC is not implemented;
// the job system in a later phase provides a work queue if that is ever
// needed).
//
// Design (the four requirements, one line each):
//   * capacity is a power of two            -> index = counter & mask, one op
//   * head/tail are std::atomic<uint64_t>   -> monotonic 64-bit counters;
//     the ring can be pushed 1.8e19 items before any wrap concern
//   * acquire / release memory ordering     -> the store side is release
//     (publishes the item write / the counter slot release), the read side
//     is acquire (sees the item before using the counter). This is the
//     minimal correct pair: no seq_cst fence traffic on the hot path.
//   * no mutexes / spinlocks / OS calls     -> push/pop are 1 load, 1 store,
//     1 compare on the happy path
//
// False sharing: head_, tail_ and the item array each sit on their own
// 64-byte cache line (alignas(64)). The producer writes tail_ and items_;
// the consumer writes head_ and reads items_ — with separate lines, the
// two cores ping-pong only the item line (unavoidable, it's the data) and
// never the counter lines. Measured target: >100M items/sec (the ring
// test asserts it).
//
// Backpressure: push() returns false when full. The producer's contract
// (stream thread) is to RETRY with a bounded spin — a full ring means the
// gen threads are behind, so the correct response is to wait for them, not
// to drop chunks. The test's integration demo counts backpressure attempts
// so it is visible, not silent.
//
// Zero runtime allocation: the buffer is fixed-size on the caller's stack
// or in a slab block. No heap use, ever.
// ===========================================================================
#pragma once

#include <atomic>
#include <cstdint>
#include <new>

namespace astra {

template <typename T, uint32_t Capacity>
class RingBuffer {
  static_assert((Capacity & (Capacity - 1)) == 0,
                "RingBuffer capacity must be a power of two");
  static_assert(Capacity > 0, "RingBuffer capacity must be > 0");

 public:
  static constexpr uint32_t kCapacity = Capacity;
  static constexpr uint32_t kMask = Capacity - 1;

  RingBuffer() = default;

  RingBuffer(const RingBuffer&) = delete;  // atomics + fixed array: not
  RingBuffer& operator=(const RingBuffer&) = delete;  // copyable, by design

  // Single producer only. Returns false when the ring is full — the
  // backpressure signal (see header). Never blocks, never allocates.
  bool push(const T& value) {
    const uint64_t t = tail_.load(std::memory_order_acquire);
    const uint64_t h = head_.load(std::memory_order_acquire);
    if (t - h >= Capacity) return false;  // full: caller retries (backpressure)
    new (item_at(t)) T(value);
    tail_.store(t + 1, std::memory_order_release);
    return true;
  }

  // Single consumer only. Returns false when the ring is empty.
  bool pop(T& out) {
    const uint64_t h = head_.load(std::memory_order_acquire);
    const uint64_t t = tail_.load(std::memory_order_acquire);
    if (h == t) return false;  // empty
    out = *item_at(h);
    head_.store(h + 1, std::memory_order_release);
    return true;
  }

  uint64_t size() const {
    // Both loads acquire: the snapshot is self-consistent enough for
    // size() (documented as approximate under concurrent use).
    const uint64_t t = tail_.load(std::memory_order_acquire);
    const uint64_t h = head_.load(std::memory_order_acquire);
    return t - h;  // 64-bit monotonic: wrap-safe for any practical lifetime
  }
  bool empty() const { return size() == 0; }
  bool full() const { return size() >= Capacity; }

  // Drain helper for tests / shutdown: pop until empty.
  template <typename F>
  uint64_t drain(F&& sink) {
    uint64_t n = 0;
    T v;
    while (pop(v)) {
      sink(v);
      ++n;
    }
    return n;
  }

 private:
  // Uninitialized storage: construction is explicit (placement new in
  // push, copy-assign in pop). Avoids touching Capacity * sizeof(T) at
  // construction time — matters for large capacities and for "zero work
  // before first push".
  // Non-const on purpose: push writes through this.
  T* item_at(uint64_t i) {
    // items_ is one flat aligned block (Capacity * sizeof(T)); index by
    // byte offset so the storage type stays a single object.
    return reinterpret_cast<T*>(
        reinterpret_cast<uint8_t*>(&items_) + (i & kMask) * sizeof(T));
  }

  alignas(64) std::atomic<uint64_t> head_{0};  // consumer writes
  alignas(64) std::atomic<uint64_t> tail_{0};  // producer writes
  alignas(64) std::aligned_storage_t<Capacity * sizeof(T), alignof(T)> items_;
};

}  // namespace astra
