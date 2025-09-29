cmake_minimum_required(VERSION 3.30)

project(mem-fuse CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(PkgConfig REQUIRED)

pkg_check_modules(FUSE REQUIRED fuse)

file(GLOB MEMFS_SOURCES *.c *.cc)

add_executable(memfs ${MEMFS_SOURCES})

target_compile_definitions(
  memfs
  PRIVATE _FILE_OFFSET_BITS=64
  PRIVATE FUSE_USE_VERSION=29
)

target_compile_options(
  memfs
  PRIVATE -Wall -Werror
)

target_include_directories(memfs PUBLIC ${FUSE_INCLUDE_DIRS})

target_link_libraries(memfs PUBLIC ${FUSE_LIBRARIES})
