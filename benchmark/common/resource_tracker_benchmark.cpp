#include <emmintrin.h>

#include <thread>

#include "benchmark/benchmark.h"
#include "common/resource_tracker.h"
#include "common/scoped_timer.h"
#include "folly/tracing/StaticTracepoint.h"

namespace noisepage {

FOLLY_SDT_DEFINE_SEMAPHORE(, tracker__done);

#define MAX_FEATURES 8

struct features {
  const uint32_t query_id;
  const uint32_t pipeline_id;
  const uint8_t num_features;
  uint8_t features[MAX_FEATURES];
  const uint8_t execution_mode;
  const uint64_t memory_bytes;
  uint32_t num_rows[MAX_FEATURES];
  uint16_t key_sizes[MAX_FEATURES];
  uint8_t num_keys[MAX_FEATURES];
  uint32_t est_cardinalities[MAX_FEATURES];
  uint8_t mem_factor[MAX_FEATURES];
  uint8_t num_loops[MAX_FEATURES];
  uint8_t num_concurrent[MAX_FEATURES];
};

static void PauseFor(const uint8_t duration) {
  for (uint8_t i = 0; i < duration; i++) _mm_pause();
}

static void WaitForBPF() {
  uint8_t pause_duration = 1;
  while (!FOLLY_SDT_IS_ENABLED(, tracker__done)) {
    if (pause_duration <= 16) {
      PauseFor(pause_duration);
      pause_duration *= 2;
    } else {
      std::this_thread::yield();
    }
  }
}

/**
 * These benchmarks exist to verify the performance difference between grouped and ungrouped perf counters. We do not
 * include them in our CI regression checks since their behavior is determined more by the OS than our wrapper.
 */
class ResourceTrackerBenchmark : public benchmark::Fixture {};

// NOLINTNEXTLINE
BENCHMARK_DEFINE_F(ResourceTrackerBenchmark, ResourceTrackerStart)(benchmark::State &state) {
  common::ResourceTracker tracker;
  uint64_t elapsed_us;
  // NOLINTNEXTLINE
  for (auto _ : state) {
    {
      common::ScopedTimer<std::chrono::microseconds> timer(&elapsed_us);
      tracker.Start();
    }
    tracker.Stop();
    state.SetIterationTime(static_cast<double>(elapsed_us) / 1e6);
  }
  state.SetItemsProcessed(state.iterations());
}

// NOLINTNEXTLINE
BENCHMARK_DEFINE_F(ResourceTrackerBenchmark, ResourceTrackerStop)(benchmark::State &state) {
  common::ResourceTracker tracker;
  uint64_t elapsed_us;
  // NOLINTNEXTLINE
  for (auto _ : state) {
    tracker.Start();
    {
      common::ScopedTimer<std::chrono::microseconds> timer(&elapsed_us);
      tracker.Stop();
    }
    state.SetIterationTime(static_cast<double>(elapsed_us) / 1e6);
  }
  state.SetItemsProcessed(state.iterations());
}

// NOLINTNEXTLINE
BENCHMARK_DEFINE_F(ResourceTrackerBenchmark, BPFStart)(benchmark::State &state) {
  struct features feats = {};
  uint64_t elapsed_us;

  WaitForBPF();

  std::this_thread::sleep_for(std::chrono::seconds(1));

  // NOLINTNEXTLINE
  for (auto _ : state) {
    {
      common::ScopedTimer<std::chrono::microseconds> timer(&elapsed_us);
      FOLLY_SDT(, tracker__start);
    }
    FOLLY_SDT_WITH_SEMAPHORE(, tracker__done, &feats);
    state.SetIterationTime(static_cast<double>(elapsed_us) / 1e6);
  }
  state.SetItemsProcessed(state.iterations());
}

// NOLINTNEXTLINE
BENCHMARK_DEFINE_F(ResourceTrackerBenchmark, BPFStop)(benchmark::State &state) {
  struct features feats = {};
  uint64_t elapsed_us;

  WaitForBPF();

  std::this_thread::sleep_for(std::chrono::seconds(1));

  // NOLINTNEXTLINE
  for (auto _ : state) {
    FOLLY_SDT(, tracker__start);
    {
      common::ScopedTimer<std::chrono::microseconds> timer(&elapsed_us);
      FOLLY_SDT_WITH_SEMAPHORE(, tracker__done, &feats);
    }
    state.SetIterationTime(static_cast<double>(elapsed_us) / 1e6);
  }
  state.SetItemsProcessed(state.iterations());
}

BENCHMARK_REGISTER_F(ResourceTrackerBenchmark, ResourceTrackerStart)->Repetitions(10);

BENCHMARK_REGISTER_F(ResourceTrackerBenchmark, ResourceTrackerStop)->Repetitions(10);

BENCHMARK_REGISTER_F(ResourceTrackerBenchmark, BPFStart)->Repetitions(10);

BENCHMARK_REGISTER_F(ResourceTrackerBenchmark, BPFStop)->Repetitions(10);
}  // namespace noisepage
