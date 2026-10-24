cmake_minimum_required(VERSION 3.24)
project(IsaacCS LANGUAGES CXX)
if(NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
  message(FATAL_ERROR "IsaacCS requires MSVC v143 / Win32")
endif()
set(REPENTOGON_ROOT "F:/projects_isaac/REPENTOGON-1.1.3" CACHE PATH "REPENTOGON 1.1.3 SDK")
find_package(Python3 REQUIRED COMPONENTS Interpreter)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreadedDLL)
set(GENERATED "${CMAKE_CURRENT_BINARY_DIR}/generated")
add_custom_command(OUTPUT "${GENERATED}/hooks_generated.cpp" "${GENERATED}/binding_v1_9_7_12.inc" "${GENERATED}/hook-manifest.json"
  COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_binding.py" --out "${GENERATED}"
  DEPENDS tools/gen_binding.py binding/hooks.json binding/v1_9_7_12-static.json ../docs/hooks/repentogon-binding.json VERBATIM)
add_library(zhlIsaacCS SHARED src/bootstrap/mod_init.cpp src/binding/definitions.cpp
  src/hooks/registry.cpp src/hooks/context.cpp src/hooks/pointer_patch.cpp
  src/ops/config.cpp src/ops/trace.cpp src/ops/command.cpp src/ops/control_port.cpp
  "${GENERATED}/hooks_generated.cpp")
# Tag 1.1.3's generated SDK contains qualified members rejected by C++20.
# Keep SDK-facing units on C++17; game-independent code remains C++20.
set_source_files_properties(src/binding/definitions.cpp "${GENERATED}/hooks_generated.cpp" PROPERTIES COMPILE_OPTIONS "/std:c++17")
target_include_directories(zhlIsaacCS PRIVATE src "${GENERATED}" "${REPENTOGON_ROOT}/include" "${REPENTOGON_ROOT}/libzhl"
  "${REPENTOGON_ROOT}/libs/rapidjson/include" "${REPENTOGON_ROOT}/libs/rapidxml" "${REPENTOGON_ROOT}/libs/lua")
target_compile_definitions(zhlIsaacCS PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX _CRT_SECURE_NO_WARNINGS)
target_compile_options(zhlIsaacCS PRIVATE /EHs /W4 /Zi /utf-8 /wd4251 /wd4996)
target_link_libraries(zhlIsaacCS PRIVATE "${REPENTOGON_ROOT}/build/Release/libzhl.lib" ws2_32)
target_link_options(zhlIsaacCS PRIVATE /DEBUG)
set_target_properties(zhlIsaacCS PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/../out/repentogon")
