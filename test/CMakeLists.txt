set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)

# Define compile flags for Debug and Release modes
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -O0")

include_directories(${CMAKE_SOURCE_DIR}/src)
macro(add_test_executable name)
  add_executable(${name} ${name}_test.cpp)
  target_link_libraries(${name} 
    PRIVATE
        GTest::gtest
        GTest::gtest_main
        bitcask_cpp
  )
  add_test(${name} ${name})
endmacro()

add_test_executable(record)
add_test_executable(active_map)
add_test_executable(record_map)
add_test_executable(init_from_existed_db)
add_test_executable(bitcask)
add_test_executable(bitcask_benchmark)
