#pragma once

#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) ||  \
    defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || \
    defined(__WIN32__) || defined(__NT__)
#define BITCASK_DECL_EXPORT __declspec(dllexport)
#define BITCASK_DECL_IMPORT __declspec(dllimport)
#else
#define BITCASK_DECL_EXPORT __attribute__((visibility("default")))
#define BITCASK_DECL_IMPORT __attribute__((visibility("default")))
#endif

#if defined(BITCASK_SHARED_LIBRARY)
#define BITCASK_EXPORT BITCASK_DECL_EXPORT
#elif BITCASK_STATIC_LIBRARY
#define BITCASK_EXPORT
#else
#define BITCASK_EXPORT BITCASK_DECL_IMPORT
#endif
