#include <benchmark/benchmark.h>

#include <bitcask/Bitcask.hpp>
#include <bitcask/Setting.hpp>
#include <cstdlib>
#include <filesystem>
#include <string>

namespace fs = std::filesystem;

// ─── helpers ────────────────────────────────────────────────────────────────

static std::string make_temp_dir() {
  std::string dir =
      "/tmp/bitcask_bench_" + std::to_string(std::rand() % 1000000);
  fs::create_directories(dir);
  return dir;
}

static std::string make_key(int i, int width = 8) {
  std::string k = std::to_string(i);
  return std::string(width - std::min((int)k.size(), width), '0') + k;
}

static std::string make_value(size_t size, char fill = 'v') {
  return std::string(size, fill);
}

// ─── BM_Put ─────────────────────────────────────────────────────────────────
// Measures synchronous Put() throughput for various value sizes.

template <size_t ValueSize>
static void BM_Put(benchmark::State& state) {
  std::string dir = make_temp_dir();
  auto* db = bitcask::Bitcask::Create(dir);

  int i = 0;
  for (auto _ : state) {
    db->Put(make_key(i++), make_value(ValueSize));
  }

  delete db;
  fs::remove_all(dir);
  state.SetItemsProcessed(state.iterations());
  state.SetBytesProcessed((long)state.iterations() *
                          (8 + (long)ValueSize));  // 8-byte keys
}
BENCHMARK_TEMPLATE(BM_Put, 16)->Iterations(50000);
BENCHMARK_TEMPLATE(BM_Put, 128)->Iterations(50000);
BENCHMARK_TEMPLATE(BM_Put, 1024)->Iterations(10000);

// ─── BM_PutAsync ────────────────────────────────────────────────────────────
// Measures async Put() throughput. Uses .get() to drain futures in batches
// so we don't accumulate unbounded memory.

template <size_t ValueSize>
static void BM_PutAsync(benchmark::State& state) {
  std::string dir = make_temp_dir();
  auto* db = bitcask::Bitcask::Create(dir);

  int i = 0;
  constexpr int kDrainEvery = 256;
  std::vector<std::future<void>> pending;
  pending.reserve(kDrainEvery);

  for (auto _ : state) {
    pending.push_back(db->PutAsync(make_key(i++), make_value(ValueSize)));
    if ((int)pending.size() >= kDrainEvery) {
      for (auto& f : pending) f.get();
      pending.clear();
    }
  }
  for (auto& f : pending) f.get();

  delete db;
  fs::remove_all(dir);
  state.SetItemsProcessed(state.iterations());
  state.SetBytesProcessed((long)state.iterations() * (8 + (long)ValueSize));
}
BENCHMARK_TEMPLATE(BM_PutAsync, 16)->Iterations(50000);
BENCHMARK_TEMPLATE(BM_PutAsync, 128)->Iterations(50000);
BENCHMARK_TEMPLATE(BM_PutAsync, 1024)->Iterations(10000);

// ─── BM_Get_HotCache ────────────────────────────────────────────────────────
// Measures Get() from the active file (served from the in-memory ActiveMap).

template <size_t ValueSize>
static void BM_Get_HotCache(benchmark::State& state) {
  std::string dir = make_temp_dir();
  auto* db = bitcask::Bitcask::Create(dir);

  const int N = 10000;
  for (int i = 0; i < N; ++i) db->Put(make_key(i), make_value(ValueSize));

  int i = 0;
  for (auto _ : state) {
    benchmark::DoNotOptimize(db->Get(make_key(i++ % N)));
  }

  delete db;
  fs::remove_all(dir);
  state.SetItemsProcessed(state.iterations());
}
BENCHMARK_TEMPLATE(BM_Get_HotCache, 16)->Iterations(200000);
BENCHMARK_TEMPLATE(BM_Get_HotCache, 128)->Iterations(200000);
BENCHMARK_TEMPLATE(BM_Get_HotCache, 1024)->Iterations(100000);

// ─── BM_Get_StableFile ──────────────────────────────────────────────────────
// Measures Get() served from a stable (on-disk) file by forcing rotation via
// a tiny maxFileSize (4 KB). After pre-population the active file is rotated,
// so all keys live in stable files and reads go through the disk path.

template <size_t ValueSize>
static void BM_Get_StableFile(benchmark::State& state) {
  std::string dir = make_temp_dir();
  // Small file size to force rotation after every few records
  bitcask::Setting setting;
  setting.maxFileSize = 4 * 1024;
  auto* db = bitcask::Bitcask::Create(dir, setting);

  // Write enough records so all data is in stable files
  const int N = 200;
  for (int i = 0; i < N; ++i) db->Put(make_key(i), make_value(ValueSize));

  // Force one more Put to rotate the last file
  db->Put("__rotate__", "x");

  int i = 0;
  for (auto _ : state) {
    benchmark::DoNotOptimize(db->Get(make_key(i++ % N)));
  }

  delete db;
  fs::remove_all(dir);
  state.SetItemsProcessed(state.iterations());
}
BENCHMARK_TEMPLATE(BM_Get_StableFile, 16)->Iterations(50000);
BENCHMARK_TEMPLATE(BM_Get_StableFile, 128)->Iterations(50000);
BENCHMARK_TEMPLATE(BM_Get_StableFile, 1024)->Iterations(20000);

// ─── BM_PutGet_Mixed ────────────────────────────────────────────────────────
// 80 % reads / 20 % writes — representative of a read-heavy workload.

static void BM_PutGet_Mixed(benchmark::State& state) {
  std::string dir = make_temp_dir();
  auto* db = bitcask::Bitcask::Create(dir);

  const int N = 5000;
  for (int i = 0; i < N; ++i) db->Put(make_key(i), make_value(64));

  int i = 0;
  for (auto _ : state) {
    if (i % 5 == 0)
      db->Put(make_key(i % N), make_value(64));
    else
      benchmark::DoNotOptimize(db->Get(make_key(i % N)));
    ++i;
  }

  delete db;
  fs::remove_all(dir);
  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_PutGet_Mixed)->Iterations(100000);

BENCHMARK_MAIN();
