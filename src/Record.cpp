#include "Record.hpp"

#include "CRC32.hpp"
#include "File.hpp"
#include "bitcask/Logger.hpp"
#include "bitcask/Define.hpp"
namespace bitcask {

// Record structure:
//
// | crc32 | keySize | valueSize | key | value |

struct RecordHeader {
  uint32_t crc32;
  uint32_t keySize;
  uint32_t valueSize;
};

namespace {
namespace internal {
uint32_t getCRC32(RecordHeader& header, const Key& key, const Value& value);
size_t fillHeader(RecordHeader& header, const Key& key, const Value& value);
}  // namespace internal
}  // namespace

RecordInf WriteRecord(file::FileHandler& file, const Key& key,
                      const Value& value) {
  RecordHeader header;
  size_t size = internal::fillHeader(header, key, value);

  file::WriteFile(file, &header, sizeof(header));

  RecordInf recordInf;
  recordInf.keyOffset = file->tellp();
  file::WriteFile(file, key.c_str(), key.size());
  recordInf.valueOffset = file->tellp();
  file::WriteFile(file, value.c_str(), value.size());
  recordInf.size = size;
  BITCASK_LOGGER_DEBUG("Write record: key={}, value={}, offset after write: {}",
                       key, value, file->tellp());
  return recordInf;
}

bool ReadRecord(file::FileHandler& file, Key& key, Value& value,
                RecordInf& record) {
  RecordHeader header;
  if (!file::ReadFile(file, &header, sizeof(header))) {
    BITCASK_LOGGER_ERROR("Failed to read record header");
    return false;
  }
  record.keyOffset = file->tellp();
  key = file::ReadFile(file, record.keyOffset, header.keySize);
  record.valueOffset = record.keyOffset + header.keySize;
  value = file::ReadFile(file, record.valueOffset, header.valueSize);
  record.size = sizeof(header) + header.keySize + header.valueSize;

  if (header.crc32 != internal::getCRC32(header, key, value)) {
    BITCASK_LOGGER_ERROR("CRC32 not match");
    return false;
  }
  return true;
}

void ReadAllRecordFromFile(file::FileHandler& file,
                           const RecordFoundCallback& callback) {
  RecordInf info;
  uint32_t count = 0;
  Key key;
  Value value;

  file->seekg(0, std::ios::beg);
  BITCASK_LOGGER_INFO("Starting to read all records from file, position: {}",
                      file->tellg());

  while (ReadRecord(file, key, value, info)) {
    callback(key, value, info);
    count++;
    BITCASK_LOGGER_INFO("Record {}: key={}, value={}, size={}", count, key,
                        value, info.size);
  }

  if (!file->eof()) {
    BITCASK_LOGGER_ERROR(
        "Read file error, file state: is good {}, is bad {}, "
        "is_open {}, position {}",
        file->good(), file->bad(), file->is_open(), file->tellg());
  } else {
    BITCASK_LOGGER_INFO("Finished reading all records, total count: {}", count);
  }
}

namespace {
namespace internal {
uint32_t getCRC32(RecordHeader& header, const Key& key, const Value& value) {
  uint32_t crc = 0;
  crc = crc32(crc, reinterpret_cast<const uint8_t*>(&header.keySize),
              sizeof(header.keySize));
  crc = crc32(crc, reinterpret_cast<const uint8_t*>(&header.valueSize),
              sizeof(header.valueSize));
  crc = crc32(crc, key);
  crc = crc32(crc, value);
  return crc;
}
size_t fillHeader(RecordHeader& header, const Key& key, const Value& value) {
  header.keySize = key.size();
  header.valueSize = value.size();
  header.crc32 = getCRC32(header, key, value);
  return sizeof(header) + header.keySize + header.valueSize;
  ;
}
}  // namespace internal
}  // namespace
}  // namespace bitcask
