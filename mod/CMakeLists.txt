cmake_minimum_required(VERSION 3.24)
project(IsaacCS LANGUAGES CXX)
if(NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
  message(FATAL_ERROR "IsaacCS requires MSVC v143 / Win32")
endif()
set(REPENTOGON_ROOT "F:/projects_isaac/REPENTOGON-1.1.3" CACHE PATH "REPENTOGON 1.1.3 SDK")
find_package(Python3 REQUIRED COMPONENTS Interpreter)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreadedDLL)
# core 独立使用 C++20 标准库；不继承 SDK include、Win32 宏或游戏绑定依赖。
add_library(isaac_cs_core STATIC src/core/session.cpp src/core/tick_clock.cpp
  src/core/tick_pipeline.cpp src/core/input_core.cpp src/core/participant.cpp)
target_include_directories(isaac_cs_core PUBLIC src)
target_compile_features(isaac_cs_core PUBLIC cxx_std_20)
target_compile_options(isaac_cs_core PRIVATE /W4 /Zi /utf-8)
# 网络层独立于 SDK/游戏头；只通过值队列与后续主线程 peer 桥接。
add_library(isaac_cs_net STATIC src/net/protocol.cpp src/net/endpoint.cpp src/net/worker.cpp)
target_include_directories(isaac_cs_net PUBLIC src)
target_compile_features(isaac_cs_net PUBLIC cxx_std_20)
target_compile_definitions(isaac_cs_net PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_compile_options(isaac_cs_net PRIVATE /W4 /Zi /utf-8 /EHsc)
target_link_libraries(isaac_cs_net PUBLIC isaac_cs_core PRIVATE ws2_32 bcrypt)
# world 采集：纯值模型/身份/存储/写入。不依赖 SDK、游戏头或 runtime.h，
# 也不调用任何原生函数；SDK-facing 接线在后续 game/world_bridge。
add_library(isaac_cs_world STATIC src/world/model.cpp src/world/identity.cpp
  src/world/storage.cpp src/world/writer.cpp)
target_include_directories(isaac_cs_world PUBLIC src)
target_compile_features(isaac_cs_world PUBLIC cxx_std_20)
target_compile_definitions(isaac_cs_world PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_compile_options(isaac_cs_world PRIVATE /W4 /Zi /utf-8 /EHsc)
set(GENERATED "${CMAKE_CURRENT_BINARY_DIR}/generated")
add_custom_command(OUTPUT "${GENERATED}/hooks_generated.cpp" "${GENERATED}/binding_v1_9_7_12.inc" "${GENERATED}/hook-manifest.json" "${GENERATED}/permanent_patches.inc" "${GENERATED}/host_patches.inc"
  COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_binding.py" --out "${GENERATED}"
  DEPENDS tools/gen_binding.py binding/hooks.json binding/v1_9_7_12-static.json ../docs/hooks/repentogon-binding.json VERBATIM)
add_library(zhlIsaacCS SHARED src/bootstrap/mod_init.cpp src/binding/definitions.cpp
  src/hooks/registry.cpp src/hooks/context.cpp src/hooks/pointer_patch.cpp src/hooks/odd_patch.cpp src/hooks/host_patch.cpp
  src/game/context.cpp src/game/topology.cpp src/game/tick.cpp src/game/input_a.cpp
  src/game/query.cpp src/game/consumers.cpp src/game/lifecycle.cpp src/game/remote_life.cpp
  src/game/device_k.cpp src/game/join.cpp src/game/peer.cpp src/game/input_lease.cpp src/game/persistence.cpp
  src/ops/config.cpp src/ops/trace.cpp src/ops/command.cpp src/ops/control_port.cpp
  "${GENERATED}/hooks_generated.cpp")
# tag 1.1.3 的生成头不兼容 C++20，只有 SDK-facing 编译单元使用 C++17。
set_source_files_properties(src/binding/definitions.cpp "${GENERATED}/hooks_generated.cpp" PROPERTIES COMPILE_OPTIONS "/std:c++17")
target_include_directories(zhlIsaacCS PRIVATE src "${GENERATED}" "${REPENTOGON_ROOT}/include" "${REPENTOGON_ROOT}/libzhl"
  "${REPENTOGON_ROOT}/libs/rapidjson/include" "${REPENTOGON_ROOT}/libs/rapidxml" "${REPENTOGON_ROOT}/libs/lua")
# 官方 ASMPatcher 公共头引用 Zydis/Zycore 类型；实现仍从 libzhl 导入，不另编译补丁库。
target_include_directories(zhlIsaacCS PRIVATE "${REPENTOGON_ROOT}/libs/zydis/include"
  "${REPENTOGON_ROOT}/libs/zydis/dependencies/zycore/include")
target_compile_definitions(zhlIsaacCS PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX _CRT_SECURE_NO_WARNINGS)
target_compile_options(zhlIsaacCS PRIVATE /EHs /W4 /Zi /utf-8 /wd4251 /wd4996)
target_link_libraries(zhlIsaacCS PRIVATE isaac_cs_core isaac_cs_net isaac_cs_world "${REPENTOGON_ROOT}/build/Release/libzhl.lib" ws2_32 bcrypt)
target_link_options(zhlIsaacCS PRIVATE /DEBUG)
set_target_properties(zhlIsaacCS PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/../out/repentogon")
