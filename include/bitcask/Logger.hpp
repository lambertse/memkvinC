#pragma once

#include <cstdint>
#include <functional>
#include <sstream>
#include <string>

#include "LibExport.hpp"

namespace bitcask::logger {

using LogLevels = uint8_t;
using LogLevel = uint8_t;

inline constexpr LogLevel LOG_LEVEL_SILENCE = 0;
inline constexpr LogLevel LOG_LEVEL_INFO = 1;
inline constexpr LogLevel LOG_LEVEL_WARN = 2;
inline constexpr LogLevel LOG_LEVEL_ERROR = 4;
inline constexpr LogLevel LOG_LEVEL_FATAL = 8;
inline constexpr LogLevel LOG_LEVEL_DEBUG = 16;
inline constexpr LogLevel LOG_LEVEL_VERBOSE = 32;
inline constexpr LogLevel LOG_LEVEL_FROM_ERROR =
    LOG_LEVEL_ERROR | LOG_LEVEL_FATAL;
inline constexpr LogLevel LOG_LEVEL_FROM_WARN =
    LOG_LEVEL_WARN | LOG_LEVEL_FROM_ERROR;
inline constexpr LogLevel LOG_LEVEL_FROM_INFO =
    LOG_LEVEL_INFO | LOG_LEVEL_FROM_WARN;
inline constexpr LogLevel LOG_LEVEL_FROM_DEBUG =
    LOG_LEVEL_DEBUG | LOG_LEVEL_FROM_INFO;

using LoggingFunctionType = std::function<void(const std::string& msg)>;

BITCASK_EXPORT void init(LogLevels allowedLevels = LOG_LEVEL_SILENCE,
                         LoggingFunctionType outLogFunc = {},
                         LoggingFunctionType errLogFunc = {});

BITCASK_EXPORT void stopLogging();
BITCASK_EXPORT void changeLogLevels(
    LogLevels allowedLevels = LOG_LEVEL_SILENCE);
BITCASK_EXPORT bool allowed(LogLevel level);
BITCASK_EXPORT void enable(LogLevel level);
BITCASK_EXPORT void disable(LogLevel level);
BITCASK_EXPORT void logImpl(LogLevel filteredLevel, const std::string& msg);

template <typename... Msg>
void debug(Msg&&... msg);
template <typename... Msg>
void info(Msg&&... msg);
template <typename... Msg>
void warn(Msg&&... msg);
template <typename... Msg>
void error(Msg&&... msg);
template <typename... Msg>
void fatal(Msg&&... msg);
template <typename... Msg>
void verbose(Msg&&... msg);
template <typename... Msg>
void log(LogLevel level, Msg&&... msg);

//--------------------------- Parser declaration----------------------------
std::string to_string(const std::wstring& arg);
template <typename T>
std::string to_string(const T& arg);
template <typename... Msg>
std::string formatMsg(Msg&&... args);

//------------------------------Implementation---------------------------------
template <typename... Msg>
void log(LogLevel level, Msg&&... msg) {
  if (allowed(level)) {
    std::string formatedStr = formatMsg(std::forward<Msg>(msg)...);
    logImpl(level, formatedStr);
  }
}
template <typename... Msg>
void debug(Msg&&... msg) {
  log(LOG_LEVEL_DEBUG, "DEBUG   :    ", std::forward<Msg>(msg)...);
}
template <typename... Msg>
void info(Msg&&... msg) {
  log(LOG_LEVEL_INFO, "INFO    :    ", std::forward<Msg>(msg)...);
}
template <typename... Msg>
void warn(Msg&&... msg) {
  log(LOG_LEVEL_WARN, "WARN    :    ", std::forward<Msg>(msg)...);
}
template <typename... Msg>
void error(Msg&&... msg) {
  log(LOG_LEVEL_ERROR, "ERROR   :    ", std::forward<Msg>(msg)...);
}
template <typename... Msg>
void fatal(Msg&&... msg) {
  log(LOG_LEVEL_FATAL, "FATAL   :    ", std::forward<Msg>(msg)...);
}
template <typename... Msg>
void verbose(Msg&&... msg) {
  log(LOG_LEVEL_VERBOSE, "VERBOSE :    ", std::forward<Msg>(msg)...);
}

template <typename T>
std::string to_string(const T& arg) {
  try {
    std::stringstream ss;
    ss << arg;
    return ss.str();
  } catch (...) {
    return "{}";
  }
}

template <typename... Msg>
std::string formatMsg(Msg&&... args) {
  std::vector<std::string> arg_strings;
  ((arg_strings.push_back(to_string(args))), ...);
  if (arg_strings.size() < 2) return "";  // Not in right format

  std::string format_string = arg_strings[1];
  std::string brace = "{}";
  size_t arg_idx = 2;

  size_t idx = 0;
  while (idx < format_string.size()) {
    int32_t found_idx = format_string.find(brace, idx);
    if (found_idx == std::string::npos) break;
    std::string replace_str = "";
    if (arg_idx < arg_strings.size()) {
      replace_str = arg_strings[arg_idx++];
    } else {
      replace_str = brace;
    }
    format_string.replace(found_idx, brace.size(), replace_str);
    idx = found_idx + replace_str.size();
  }
  return arg_strings.front() + format_string;
}

inline bool debugAllowed() { return allowed(LOG_LEVEL_DEBUG); }
inline bool infoAllowed() { return allowed(LOG_LEVEL_INFO); }
inline bool warnAllowed() { return allowed(LOG_LEVEL_WARN); }
inline bool errorAllowed() { return allowed(LOG_LEVEL_ERROR); }
inline bool fatalAllowed() { return allowed(LOG_LEVEL_FATAL); }
inline bool verboseAllowed() { return allowed(LOG_LEVEL_VERBOSE); }

using MsCStr = const char*;
inline constexpr MsCStr constexprPastLastSlash(MsCStr str, MsCStr last_slash) {
#if defined(_WINDOWS) || defined(WIN32)
  constexpr char slash = '\\';
#else
  constexpr char slash = '/';
#endif
  return *str == '\0'    ? last_slash
         : *str == slash ? constexprPastLastSlash(str + 1, str + 1)
                         : constexprPastLastSlash(str + 1, last_slash);
}

inline constexpr MsCStr constexprPastLastSlash(MsCStr str) {
  return constexprPastLastSlash(str, str);
}

}  // namespace bitcask::logger
#define BITCASK_LOG_LEVEL_SILENCE 0
#define BITCASK_LOG_LEVEL_DEBUG 1
#define BITCASK_LOG_LEVEL_INFO 2
#define BITCASK_LOG_LEVEL_WARN 4
#define BITCASK_LOG_LEVEL_ERROR 8
#define BITCASK_LOG_LEVEL_FATAL 16
#define BITCASK_LOG_LEVEL_VERBOSE 32

#ifndef BITCASK_MIN_ALLOWED_LOG_LEVEL
#define BITCASK_MIN_ALLOWED_LOG_LEVEL BITCASK_LOG_LEVEL_INFO
#endif

#define BITCASK_LOGGER_DEBUG(...)                                            \
  do {                                                                       \
    if (bitcask::logger::debugAllowed()) {                                   \
      bitcask::logger::debug(__VA_ARGS__, "  --> [[ ",                       \
                             BITCASK_SHORT_FILE_NAME, ":", __LINE__, " ]]"); \
    }                                                                        \
  } while (false)

#define BITCASK_LOGGER_WRITE(logtype, ...)                               \
  do {                                                                   \
    if (bitcask::logger::logtype##Allowed()) {                           \
      if (!bitcask::logger::debugAllowed()) {                            \
        bitcask::logger::logtype(__VA_ARGS__);                           \
      } else {                                                           \
        bitcask::logger::logtype(__VA_ARGS__, "  --> [[ ",               \
                                 BITCASK_SHORT_FILE_NAME, ":", __LINE__, \
                                 " ]]");                                 \
      }                                                                  \
    }                                                                    \
  } while (false)

#if BITCASK_MIN_ALLOWED_LOG_LEVEL <= BITCASK_LOG_LEVEL_VERBOSE
#define BITCASK_LOGGER_VERBOSE(...) BITCASK_LOGGER_WRITE(verbose, __VA_ARGS__)
#else
#define BITCASK_LOGGER_VERBOSE(...) while (false)
#endif

#if BITCASK_MIN_ALLOWED_LOG_LEVEL <= BITCASK_LOG_LEVEL_INFO
#define BITCASK_LOGGER_INFO(...) BITCASK_LOGGER_WRITE(info, __VA_ARGS__)
#else
#define BITCASK_LOGGER_INFO(...) while (false)
#endif

#if BITCASK_MIN_ALLOWED_LOG_LEVEL <= BITCASK_LOG_LEVEL_WARN
#define BITCASK_LOGGER_WARN(...) BITCASK_LOGGER_WRITE(warn, __VA_ARGS__)
#else
#define BITCASK_LOGGER_WARN(...) while (false)
#endif

#if BITCASK_MIN_ALLOWED_LOG_LEVEL <= BITCASK_LOG_LEVEL_ERROR
#define BITCASK_LOGGER_ERROR(...) BITCASK_LOGGER_WRITE(error, __VA_ARGS__)
#else
#define BITCASK_LOGGER_ERROR(...) while (false)
#endif

#if BITCASK_MIN_ALLOWED_LOG_LEVEL <= BITCASK_LOG_LEVEL_FATAL
#define BITCASK_LOGGER_FATAL(...) BITCASK_LOGGER_WRITE(fatal, __VA_ARGS__)
#else
#define BITCASK_LOGGER_FATAL(...) while (false)
#endif

#define BITCASK_SHORT_FILE_NAME \
  bitcask::logger::constexprPastLastSlash(__FILE__)
