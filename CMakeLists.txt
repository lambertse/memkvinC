cmake_minimum_required(VERSION 3.16)
project(bitcask_cpp)

# Set C++ standard first
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Force Debug build type
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(BITCASK_SHARED_LIBRARY ON)
set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)

# Define compile flags for Debug and Release modes
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -O0")

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3")

set(CMAKE_FIND_PACKAGE_PREFER_CONFIG ON)
set(CMAKE_FIND_PACKAGE_NO_PACKAGE_REGISTRY ON)

# Disable unused test builds for dependencies
set(CRC32C_BUILD_TESTS OFF)
set(CRC32C_USE_GLOG OFF)
set(CRC32C_INSTALL OFF)

# Set up third-party dependencies
include(FetchContent)
FetchContent_Declare(
  crc32c
  GIT_REPOSITORY https://github.com/google/crc32c.git
  GIT_TAG        main
)
FetchContent_MakeAvailable(crc32c)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

FetchContent_Declare(
  libcuckoo 
  GIT_REPOSITORY https://github.com/efficient/libcuckoo.git
  GIT_TAG        master
)
FetchContent_MakeAvailable(libcuckoo)
include_directories(${libcuckoo_SOURCE_DIR})

# Include all header files in the include directory
file(GLOB HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/include/*/*.hpp")

# Include all source files in the src directory
file(GLOB SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Add all source files to the library
if(BITCASK_SHARED_LIBRARY)
  add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES} ${HEADER_FILES})
else()
  add_library(${PROJECT_NAME} ${SOURCE_FILES} ${HEADER_FILES})
endif()
target_link_libraries(${PROJECT_NAME} PRIVATE crc32c)

enable_testing()
add_subdirectory(test)


