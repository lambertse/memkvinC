# Bitcask

A C++17 implementation of the [Bitcask](https://riak.com/assets/bitcask-intro.pdf) log-structured key-value storage engine, built as a shared library.

Bitcask provides low-latency reads and writes by keeping all keys in memory (via a hash index) and appending every write to an append-only log file. When a log file exceeds a configurable size threshold it is rotated into an immutable "stable" file and a new active file is created.

## Features

- **O(1) reads** — key lookup goes through an in-memory index directly to the on-disk value offset
- **Append-only writes** — no in-place updates; every write is a sequential append
- **Synchronous and asynchronous Put** — `Put` blocks until committed; `PutAsync` returns a `std::future<void>`
- **Automatic file rotation** — active file rotates when it exceeds `Setting::maxFileSize`
- **Crash recovery** — on restart, all existing `.db` files are scanned and the in-memory index is rebuilt
- **CRC32C integrity** — every record is checksummed on write and verified on read
- **Thread-safe** — concurrent reads and writes via `std::shared_mutex` and `libcuckoo::cuckoohash_map`

## Architecture

```
┌──────────────────────────────────────────────────────────┐
│  Public API  (include/bitcask/)                          │
│  Bitcask ──► BitcaskImpl  (pimpl)                        │
└────────────────────────┬─────────────────────────────────┘
                         │
           ┌─────────────▼─────────────┐
           │  In-memory index          │
           │  RecordMap  key → Hint    │
           │  ActiveMap  key → Value   │
           └─────────────┬─────────────┘
                         │
           ┌─────────────▼─────────────┐
           │  Storage layer            │
           │  ActiveFile  (write/read) │
           │  StableFile  (read-only)  │
           └───────────────────────────┘
```

**Write path:** `Put` enqueues a write → `CommitWorker` (background thread) drains the queue → appends a record to the active file → updates `RecordMap` and `ActiveMap` → rotates active file if it exceeds `maxFileSize`.

**Read path:** look up key in `RecordMap` to get a `Hint` (file id, offset, size) → if the hint points to the active file, serve from `ActiveMap` (in-memory) → otherwise seek into the corresponding `StableFile` and read the bytes at the stored offset.

**On-disk record format:**
```
| crc32c (4B) | keySize (4B) | valueSize (4B) | key (keySize B) | value (valueSize B) |
```

## Requirements

| Tool | Version |
|------|---------|
| C++ compiler | C++17 or later |
| CMake | 3.16 or later |

Dependencies are fetched automatically via CMake `FetchContent`:

| Library | Purpose |
|---------|---------|
| [google/crc32c](https://github.com/google/crc32c) | CRC32C checksum |
| [google/googletest](https://github.com/google/googletest) v1.17.0 | Unit testing |
| [efficient/libcuckoo](https://github.com/efficient/libcuckoo) | Concurrent hash maps |

## Build

```bash
# Configure (fetches dependencies automatically)
cmake -B build

# Build
cmake --build build

# Or build with parallel jobs
cmake --build build -- -j$(nproc)
```

The output shared library is `build/libbitcask_cpp.dylib` (macOS) or `build/libbitcask_cpp.so` (Linux).

## Testing

```bash
# Run all tests
cd build && ctest

# Run a specific test suite
./build/test/record
./build/test/active_map
./build/test/record_map
./build/test/init_from_existed_db
./build/test/bitcask
./build/test/bitcask_benchmark

# Run a single test case
./build/test/bitcask --gtest_filter="BitCaskTest.OverwriteKeyReturnsLatestValue"
```

| Suite | What it covers |
|-------|---------------|
| `record` | On-disk record serialisation and CRC32C verification |
| `active_map` | In-memory key-value cache including concurrent access |
| `record_map` | In-memory key→Hint index including concurrent access |
| `init_from_existed_db` | DB recovery and data persistence across restarts |
| `bitcask` | Public API (Put, PutAsync, Get, Delete) and file rotation |
| `bitcask_benchmark` | Throughput with 100 000 keys and file rotation enabled |

## Usage

```cpp
#include <bitcask/Bitcask.hpp>
#include <bitcask/Logger.hpp>
#include <iostream>

int main() {
    // Optional: enable logging
    bitcask::logger::init(
        bitcask::logger::LOG_LEVEL_FROM_INFO,
        [](const std::string &msg) { std::cout << msg << "\n"; });

    // Open (or create) a database
    bitcask::Bitcask *db = bitcask::Bitcask::Create("/tmp/mydb/");

    // Synchronous write — blocks until committed to disk
    db->Put("hello", "world");

    // Asynchronous write — returns immediately with a future
    auto f = db->PutAsync("key2", "value2");
    f.get(); // wait for commit if needed

    // Read
    auto val = db->Get("hello");
    if (val) {
        std::cout << *val << "\n"; // "world"
    }

    delete db;
}
```

### Custom settings

```cpp
bitcask::Setting setting;
setting.maxFileSize = 64 * 1024 * 1024; // rotate every 64 MB (default: 1 GB)

bitcask::Bitcask *db = bitcask::Bitcask::Create("/tmp/mydb/", setting);
```

### Linking (CMake consumer)

```cmake
target_link_libraries(my_app PRIVATE bitcask_cpp)
target_include_directories(my_app PRIVATE path/to/bitcask-cpp/include)
```

## Limitations

- **`Delete` is not yet implemented** — `Delete()` returns `false` (TBD).
- **No compaction** — stable files are never merged; disk usage grows monotonically until compaction is implemented.
- All keys must fit in memory (standard Bitcask constraint).

## License

This project is open source. See the repository for licence details.
