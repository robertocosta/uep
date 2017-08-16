cmake_minimum_required (VERSION 3.2)
include(${CMAKE_ROOT}/Modules/ExternalProject.cmake)
project (uep CXX C) # C required for lib checks
enable_testing()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 14)
add_compile_options(-Wall)

if (${CMAKE_BUILD_TYPE} STREQUAL Release)
  add_compile_options(-O3)
elseif(${CMAKE_BUILD_TYPE} STREQUAL Debug)
  add_compile_options(-O0 -ggdb)
  set(CMAKE_POSITION_INDEPENDENT_CODE False)
endif(${CMAKE_BUILD_TYPE} STREQUAL Release)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(GIT_CMD git)
execute_process(COMMAND ${GIT_CMD} log -1 --format='%H'
  OUTPUT_VARIABLE GIT_COMMIT_SHA1
)
string(REPLACE "'" "" GIT_COMMIT_SHA1 ${GIT_COMMIT_SHA1})
string(REPLACE "\n" "" GIT_COMMIT_SHA1 ${GIT_COMMIT_SHA1})

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

find_package(Boost 1.58 REQUIRED COMPONENTS
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

configure_file(cmake_defines.hpp.in cmake_defines.hpp @ONLY)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

include_directories(${CMAKE_SOURCE_DIR}/jsvm/JSVM/H264Extension/include)
link_directories(${CMAKE_SOURCE_DIR}/jsvm/lib)

ExternalProject_Add(jsvm
  SOURCE_DIR "${CMAKE_SOURCE_DIR}/jsvm/JSVM/H264Extension/build/linux"
  BUILD_IN_SOURCE 1
  CONFIGURE_COMMAND ""
  BUILD_COMMAND "make"
  INSTALL_COMMAND ""
)

# Needed by jsvm headers
add_definitions(
  -DDOWN_CONVERT_STATIC
  -DMSYS_LINUX
  -DMSYS_UNIX_LARGEFILE
  -DSHARP_AVC_REWRITE_OUTPUT
  -D_FILE_OFFSET_BITS=64
  -D_LARGEFILE64_SOURCE
)

set(svc_libs
  AvcRewriterLibStatic
  H264AVCCommonLibStatic
  H264AVCDecoderLibStatic
  H264AVCEncoderLibStatic
  H264AVCVideoIoLibStatic
)

foreach(sl IN LISTS svc_libs)
  add_library(${sl} STATIC IMPORTED)
  if(${CMAKE_BUILD_TYPE} STREQUAL Debug)
    set_target_properties(${sl}
      PROPERTIES IMPORTED_LOCATION
      ${CMAKE_SOURCE_DIR}/jsvm/lib/lib${sl}d.a
    )
  else(${CMAKE_BUILD_TYPE} STREQUAL Debug)
    set_target_properties(${sl}
      PROPERTIES IMPORTED_LOCATION
      ${CMAKE_SOURCE_DIR}/jsvm/lib/lib${sl}.a
    )
  endif(${CMAKE_BUILD_TYPE} STREQUAL Debug)
  add_dependencies(${sl} jsvm)
endforeach(sl)

add_library(svc_err OBJECT ${CMAKE_SOURCE_DIR}/jsvm/JSVM/H264Extension/src/lib/H264AVCCommonLib/H264AVCCommonLib.cpp)

include_directories(${CMAKE_BINARY_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/src)

add_subdirectory(src)
add_subdirectory(test)
