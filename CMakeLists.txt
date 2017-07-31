cmake_minimum_required (VERSION 3.2)
project (uep CXX C) # C required for lib checks
enable_testing()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

find_package(Boost 1.62 REQUIRED COMPONENTS
  date_time
  filesystem
  log
  log_setup
  system
  thread
  unit_test_framework
)
include_directories(${Boost_INCLUDE_DIRS})
add_definitions(-DBOOST_ALL_DYN_LINK)

find_package(Protobuf REQUIRED)
include_directories(${PROTOBUF_INCLUDE_DIRS})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -ggdb -O0 -pg" )
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pg" )

add_subdirectory(src)
add_subdirectory(test)
