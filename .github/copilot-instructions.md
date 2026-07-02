# Copilot Instructions

## Build & Test

```bash
# Configure and build
cmake -B build
cmake --build build

# Run all tests
cd build && ctest

# Run a single test executable
./build/test/record
./build/test/bitcaskimpl
./build/test/bitcask
./build/test/init_from_existed_db
./build/test/bitcask_benchmark
```

Build defaults to **Debug** mode with `-g -O0`. Dependencies (crc32c, googletest, libcuckoo) are fetched automatically via CMake `FetchContent`.

## Architecture

This is a C++17 implementation of the [Bitcask](https://riak.com/assets/bitcask-intro.pdf) log-structured key-value storage engine, built as a shared library.

### Layer overview

```
include/bitcask/   — Public API headers (Bitcask.hpp, Type.hpp, Setting.hpp, Logger.hpp)
src/               — All implementation (.cpp + internal .hpp)
test/              — One test file per component, each compiled as its own executable
```

### Key data flow

**Write path** (`Put`):
1. `Bitcask::Put` → `BitcaskImpl::Put` enqueues a `Write` struct onto `_writes` and returns a `std::future<void>`.
2. `CommitWorker` (background thread) drains `_writes`, calls `CommitWrite`, which appends each record to the **active file** and updates both `_activeMap` (in-memory value cache) and `_recordMap` (key → `Hint` index).
3. When the active file exceeds `Setting::maxFileSize`, it is **rotated**: the current file becomes a `StableFile` and a new active file is created. File IDs are monotonically incrementing integers; the **highest-numbered `.db` file is always the active file**.

**Read path** (`Get`):
1. Look up key in `_recordMap` to get a `Hint` (fd, offset, size).
2. If `hint.fd == getActiveFD()` (i.e., the key is in the current active file), serve from `_activeMap` (in-memory).
3. Otherwise, seek into the corresponding `StableFile` and read the value at the stored offset.

**Concurrency**: `std::shared_mutex` — `Put`/`CommitWrite` take exclusive locks; `Get` takes a shared lock. `ActiveMap` and `RecordMap` both use `libcuckoo::cuckoohash_map` for concurrent access.

### Record on-disk format

```
| crc32 (4B) | keySize (4B) | valueSize (4B) | key (keySize B) | value (valueSize B) |
```

CRC32C is computed over `keySize`, `valueSize`, `key`, and `value` (not the crc field itself).

### Pimpl pattern

`Bitcask` (public, `include/bitcask/Bitcask.hpp`) holds a `shared_ptr<BitcaskImpl>` and delegates all work to it. Consumer code only sees `include/bitcask/`.

### `Delete` is not implemented

`BitcaskImpl::Delete` currently returns `false` (marked TBD).

## Conventions

### Namespaces
- `bitcask` — all public and internal types
- `bitcask::file` — raw file I/O utilities (`File.hpp` / `File.cpp`)
- `bitcask::logger` — logging system

### Logging macros
Use `BITCASK_LOGGER_*` macros (defined in `include/bitcask/Logger.hpp`), not `logger::*` functions directly. Format strings use `{}` placeholders:
```cpp
BITCASK_LOGGER_INFO("Key: {}, offset: {}", key, offset);
BITCASK_LOGGER_DEBUG("...");   // also appends file:line in debug builds
BITCASK_LOGGER_ERROR("...");
```
Logger must be initialized by the caller via `bitcask::logger::init(levels, outFunc, errFunc)` before any logging occurs.

### Test naming
Test files follow `<name>_test.cpp` and the CMake macro `add_test_executable(<name>)` compiles each as a standalone executable named `<name>` under `build/test/`.

### Export macro
`BITCASK_EXPORT` (defined in `include/LibExport.hpp`) must be applied to any public symbol in the shared library. It expands to `__attribute__((visibility("default")))` on non-Windows platforms.
