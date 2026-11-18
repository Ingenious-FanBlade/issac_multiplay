cmake_minimum_required(VERSION 3.21)
project(isaac_dual_bootstrap LANGUAGES CXX)

if(NOT WIN32 OR NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
  message(FATAL_ERROR "Build this launcher bootstrap with MSVC Win32")
endif()

# 仅由双开工具显式载入，不加入产品或 REPENTOGON 的自动扫描清单。
add_library(isaac-dual-bootstrap SHARED bootstrap.cpp)
target_compile_features(isaac-dual-bootstrap PRIVATE cxx_std_17)
target_compile_options(isaac-dual-bootstrap PRIVATE /utf-8)
