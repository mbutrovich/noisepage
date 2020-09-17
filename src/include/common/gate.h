#pragma once

#include <emmintrin.h>

#include <atomic>
#include <thread>

#include "common/macros.h"

namespace terrier::common {

/**
 * A cheap synchronization primitive to prevent execution from proceeding past
 * a given point.  The advantage of this primitive over other synchronization
 * primitives is that the blocked invocation is read-only and should be more
 * performant when it is frequently used.
 *
 * Gates can be recursively locked.
 *
 * @warning A lock holder cannot traverse the gate.  This will cause deadlock.
 */
class Gate {
 public:
  /**
   * Adds a lock to the gate.
   */
  void Lock() { count_++; }

  /**
   * Removes a lock from the gate.
   */
  void Unlock() { count_--; }

  void pause_for(const uint8_t delay) {
    for (uint8_t i = 0; i < delay; i++) _mm_pause();
  }

  /**
   * Traverses the gate unless there are currently locks emplaced.  If there
   * are locks on the gate, spin until its free.
   */
  void Traverse() {
    uint8_t i = 1;
    while (count_.load() > 0) {
      if (i <= 16) {
        pause_for(i);
        i *= 2;
      } else {
        std::this_thread::yield();
      }
    }
  }

  /**
   * Scoped locking of the gate that guarantees unlocking on destruction
   */
  class ScopedLock {
   public:
    /**
     * Add a lock to the gate
     * @param gate pointer to Gate to lock
     */
    explicit ScopedLock(Gate *const gate) : gate_(gate) { gate_->Lock(); }

    /**
     * Undo the lock that was added in the constructor
     */
    ~ScopedLock() { gate_->Unlock(); }
    DISALLOW_COPY_AND_MOVE(ScopedLock)
   private:
    Gate *const gate_;
  };

  /**
   * Scoped traversal of the lock that guarantees traversal of gate when destructed
   */
  class ScopedExit {
   public:
    /**
     * Add requirement to traverse gate on destruction
     * @param gate pointer to Gate that will be traversed
     */
    explicit ScopedExit(Gate *const gate) : gate_(gate) {}

    /**
     * Traverse the gate while being destructed
     */
    ~ScopedExit() { gate_->Traverse(); }
    DISALLOW_COPY_AND_MOVE(ScopedExit)
   private:
    Gate *const gate_;
  };

 private:
  std::atomic<int64_t> count_ = 0;
};

}  // namespace terrier::common
