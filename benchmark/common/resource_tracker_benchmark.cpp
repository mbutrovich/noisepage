#include <emmintrin.h>

#include <thread>

#include "benchmark/benchmark.h"
#include "common/resource_tracker.h"
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

/**
 * These benchmarks exist to verify the performance difference between grouped and ungrouped perf counters. We do not
 * include them in our CI regression checks since their behavior is determined more by the OS than our wrapper.
 */
class ResourceTrackerBenchmark : public benchmark::Fixture {};

// NOLINTNEXTLINE
BENCHMARK_DEFINE_F(ResourceTrackerBenchmark, ResourceTracker)(benchmark::State &state) {
  common::ResourceTracker tracker;
  // NOLINTNEXTLINE
  for (auto _ : state) {
    tracker.Start();
    tracker.Stop();
  }
  state.SetItemsProcessed(state.iterations());
}

// NOLINTNEXTLINE
BENCHMARK_DEFINE_F(ResourceTrackerBenchmark, BPF)(benchmark::State &state) {
  struct features feats = {};

  while (!FOLLY_SDT_IS_ENABLED(, tracker__done)) {
    _mm_pause();
  }

  std::this_thread::sleep_for(std::chrono::seconds(1));

  // NOLINTNEXTLINE
  for (auto _ : state) {
    FOLLY_SDT(, tracker__start);
    FOLLY_SDT_WITH_SEMAPHORE(, tracker__done, &feats);
  }
  state.SetItemsProcessed(state.iterations());
}

BENCHMARK_REGISTER_F(ResourceTrackerBenchmark, ResourceTracker);

BENCHMARK_REGISTER_F(ResourceTrackerBenchmark, BPF);
}  // namespace noisepage
