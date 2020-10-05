#include <vector>

#include "benchmark/benchmark.h"
#include "benchmark_util/benchmark_config.h"
#include "common/scoped_timer.h"
#include "main/db_main.h"
#include "test_util/multithread_test_util.h"
#include "transaction/transaction_manager.h"

namespace terrier {

class TransactionManagerBenchmark : public benchmark::Fixture {
 public:
  void SetUp(const benchmark::State &state) final {
    db_main_ = DBMain::Builder().SetUseGC(true).SetUseGCThread(true).Build();
    txn_manager_ = db_main_->GetTransactionLayer()->GetTransactionManager();
  }

  void TearDown(const benchmark::State &state) final { db_main_.reset(); }

  common::ManagedPointer<transaction::TransactionManager> txn_manager_;

  std::unique_ptr<DBMain> db_main_;

  static constexpr uint32_t num_txns_ = 1000000;
};

// NOLINTNEXTLINE
BENCHMARK_DEFINE_F(TransactionManagerBenchmark, NoOp)(benchmark::State &state) {
  common::WorkerPool thread_pool(BenchmarkConfig::num_threads, {});
  thread_pool.Startup();

  auto workload = [&](uint32_t id) {
    for (uint32_t i = 0; i < num_txns_ / BenchmarkConfig::num_threads; i++) {
      auto *txn = txn_manager_->BeginTransaction();
      txn_manager_->Commit(txn, transaction::TransactionUtil::EmptyCallback, nullptr);
    }
  };

  // NOLINTNEXTLINE
  for (auto _ : state) {
    uint64_t elapsed_ms;
    {
      common::ScopedTimer<std::chrono::milliseconds> timer(&elapsed_ms);
      MultiThreadTestUtil::RunThreadsUntilFinish(&thread_pool, 48, workload);
    }
    state.SetIterationTime(static_cast<double>(elapsed_ms) / 1000.0);
  }

  state.SetItemsProcessed(state.iterations() * num_txns_);
}

// ----------------------------------------------------------------------------
// BENCHMARK REGISTRATION
// ----------------------------------------------------------------------------
// clang-format off
BENCHMARK_REGISTER_F(TransactionManagerBenchmark, NoOp)
  ->Unit(benchmark::kMillisecond)
  ->UseManualTime()
  ->MinTime(3);
// clang-format on

}  // namespace terrier
