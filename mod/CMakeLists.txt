cmake_minimum_required(VERSION 3.24)
project(IsaacCS LANGUAGES CXX)
if(NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
  message(FATAL_ERROR "IsaacCS requires MSVC v143 / Win32")
endif()
set(REPENTOGON_ROOT "F:/projects_isaac/REPENTOGON-1.1.3" CACHE PATH "REPENTOGON 1.1.3 SDK")
find_package(Python3 REQUIRED COMPONENTS Interpreter)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreadedDLL)
# 新版 STL 的 constexpr mutex 初始化依赖新版 MSVCP140；游戏可先加载旧运行库。
# 统一使用微软提供的兼容构造路径，让实际运行库初始化锁；所有链接单元保持一致。
add_compile_definitions(_DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR)
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
# world 采集：纯值模型/身份/存储/写入/观察策略。不依赖 SDK、游戏头或
# runtime.h，也不调用任何原生函数；SDK-facing 接线在后续 game/world_bridge。
add_library(isaac_cs_world STATIC src/world/model.cpp src/world/identity.cpp
  src/world/storage.cpp src/world/writer.cpp src/world/observe.cpp src/world/collector.cpp
  src/world/common/contracts.cpp src/world/common/service.cpp src/world/common/publication.cpp
  src/world/common/native_source.cpp src/world/common/membership.cpp src/world/compatibility/min_read_v1.cpp src/world/compatibility/encode.cpp
  src/world/domains/entity/collect.cpp src/world/domains/grid/collect.cpp)
target_include_directories(isaac_cs_world PUBLIC src)
target_compile_features(isaac_cs_world PUBLIC cxx_std_20)
target_compile_definitions(isaac_cs_world PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_compile_options(isaac_cs_world PRIVATE /W4 /Zi /utf-8 /EHsc)
target_link_libraries(isaac_cs_world PUBLIC isaac_cs_core)
set(GENERATED "${CMAKE_CURRENT_BINARY_DIR}/generated")
# 复制层只依赖纯值观测合同与JSON库；生成目录属于产品协议，不包含游戏地址。
file(GLOB REPLICA_CATALOGS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/observation/isaac-cs-observation/*catalog.json")
add_custom_command(OUTPUT "${GENERATED}/replication_catalog.inc" "${GENERATED}/replication-catalog.json"
  COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_replica_catalog.py" --out "${GENERATED}"
  DEPENDS tools/gen_replica_catalog.py tools/replica_projection_policy.py ${REPLICA_CATALOGS} VERBATIM)
add_custom_command(OUTPUT "${GENERATED}/room_grid_application.inc" "${GENERATED}/room-grid-application.json"
  COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_room_grid_catalog.py" --out "${GENERATED}"
  DEPENDS tools/gen_room_grid_catalog.py tools/gen_replica_catalog.py tools/replica_projection_policy.py ${REPLICA_CATALOGS} VERBATIM)
add_custom_command(OUTPUT "${GENERATED}/entity_application.inc" "${GENERATED}/entity-application.json"
  COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_entity_application_catalog.py" --out "${GENERATED}"
  DEPENDS tools/gen_entity_application_catalog.py tools/gen_replica_catalog.py tools/replica_projection_policy.py ${REPLICA_CATALOGS} VERBATIM)
add_library(isaac_cs_replica STATIC src/replica/model.cpp src/replica/projection_graph.cpp src/replica/codec.cpp src/replica/entity_target.cpp
  src/replica/room_grid_target.cpp src/replica/room_grid_plan.cpp src/replica/room_grid_budget.cpp
  "${GENERATED}/replication_catalog.inc" "${GENERATED}/room_grid_application.inc" "${GENERATED}/entity_application.inc")
target_include_directories(isaac_cs_replica PUBLIC src "${REPENTOGON_ROOT}/libs/rapidjson/include" PRIVATE "${GENERATED}")
target_compile_options(isaac_cs_replica PRIVATE /W4 /Zi /utf-8 /EHsc)
target_compile_definitions(isaac_cs_replica PUBLIC _SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING)
target_link_libraries(isaac_cs_replica PUBLIC isaac_cs_world)
# 真实游戏连接复用复制合同，网络后台不引用SDK或游戏对象。
target_sources(isaac_cs_net PRIVATE src/net/live.cpp)
target_link_libraries(isaac_cs_net PUBLIC isaac_cs_replica)
# E2E工具与正式DLL分开打包；同一库用于正式主线程纯值适配和统一验收。
option(ISAACCS_BUILD_E2E_TOOLS "Build explicitly requested E2E tools outside product install tree" OFF)
if(ISAACCS_BUILD_E2E_TOOLS)
add_executable(replica-contract ../tools/replica-contract/main.cpp)
target_link_libraries(replica-contract PRIVATE isaac_cs_replica)
target_compile_options(replica-contract PRIVATE /W4 /Zi /utf-8 /EHsc)
set_target_properties(replica-contract PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/../out/repentogon")
add_executable(client-fixture ../tools/client-fixture/main.cpp)
target_link_libraries(client-fixture PRIVATE isaac_cs_replica)
target_compile_options(client-fixture PRIVATE /W4 /Zi /utf-8 /EHsc)
set_target_properties(client-fixture PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/../out/repentogon")
endif()
add_custom_command(OUTPUT "${GENERATED}/hooks_generated.cpp" "${GENERATED}/binding_v1_9_7_12.inc" "${GENERATED}/hook-manifest.json" "${GENERATED}/permanent_patches.inc" "${GENERATED}/host_patches.inc" "${GENERATED}/hook_capacity.h" "${GENERATED}/world_layout.inc" "${GENERATED}/world-layout.json"
  BYPRODUCTS "${GENERATED}/exit_patch.inc" "${GENERATED}/shutdown_patch.inc" "${GENERATED}/backdrop_patch.inc" "${GENERATED}/random_runtime.inc"
  COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_binding.py" --out "${GENERATED}"
  DEPENDS tools/gen_binding.py binding/hooks.json binding/world_layout.json binding/v1_9_7_12-static.json ../docs/hooks/repentogon-binding.json VERBATIM)
add_library(zhlIsaacCS SHARED src/bootstrap/mod_init.cpp src/binding/definitions.cpp
  src/binding/world_layout.cpp src/hooks/registry.cpp src/hooks/context.cpp
  src/hooks/pointer_patch.cpp src/hooks/odd_patch.cpp src/hooks/host_patch.cpp src/hooks/exit_patch.cpp
  src/game/context.cpp src/game/player_api.cpp src/game/world_sources/lua_entity.cpp src/game/world_sources/loot.cpp src/game/world_sources/observation.cpp src/game/topology.cpp src/game/tick.cpp src/game/input_a.cpp
  src/game/query.cpp src/game/consumers.cpp src/game/lifecycle.cpp src/game/remote_life.cpp src/game/world_bridge.cpp src/game/world_sources/global_random.cpp
  src/game/world_sources/random_sources.cpp src/game/world_sources/random_runtime_lua.cpp src/game/world_sources/random_e2e.cpp
  src/game/world_sources/random_owners.cpp src/hooks/random_runtime.cpp
  "${REPENTOGON_ROOT}/libzhl/hde32/src/hde32.cpp"
  src/game/device_k.cpp src/game/join.cpp src/game/peer.cpp src/game/input_lease.cpp src/game/persistence.cpp
  src/game/client.cpp src/game/client_scene.cpp src/game/client_native_bridge.cpp
  src/game/client_native_world.cpp src/game/client_room_native.cpp src/game/client_backdrop_native.cpp src/game/client_objects.cpp src/game/client_objects_native.cpp
  src/game/client_grids.cpp src/game/client_grid_native.cpp src/game/client_grid_events.cpp
  src/game/client_map_native.cpp
  src/hooks/backdrop_patch.cpp
  src/game/client_presentation.cpp src/game/client_presentation_native.cpp
  src/game/client_resource_model.cpp src/game/client_resources_native.cpp
  src/game/client_e2e_native.cpp
  src/ops/config.cpp src/ops/trace.cpp src/ops/command.cpp src/ops/client_fixture.cpp src/ops/world_commands.cpp src/ops/control_port.cpp
  "${GENERATED}/hook_capacity.h" "${GENERATED}/world_layout.inc"
  "${GENERATED}/exit_patch.inc"
  "${GENERATED}/shutdown_patch.inc"
  "${GENERATED}/hooks_generated.cpp")
# SDK 1.1.3 的生成头和 ASMPatcher.hpp 的 friend-static 声明不兼容当前 C++20。
# SDK-facing 单元只使用 C++17 值接口；Join/输入/world 核心保持 C++20。
# 显式覆盖逐文件标准，不降低整个 DLL 标准，也不修改 SDK 头或补丁/汇编行为。
set_source_files_properties(src/game/world_sources/loot.cpp src/game/world_sources/lua_entity.cpp src/game/player_api.cpp src/game/client_native_bridge.cpp src/binding/definitions.cpp "${GENERATED}/hooks_generated.cpp"
  src/game/client_native_world.cpp src/game/client_room_native.cpp src/game/client_backdrop_native.cpp src/game/client_objects_native.cpp src/game/client_presentation_native.cpp src/game/client_resources_native.cpp
  src/game/client_grid_native.cpp
  src/game/client_map_native.cpp
  src/game/client_e2e_native.cpp
  src/game/world_sources/random_runtime_lua.cpp src/game/world_sources/random_e2e.cpp src/game/world_sources/random_owners.cpp
  src/hooks/host_patch.cpp src/hooks/odd_patch.cpp src/hooks/exit_patch.cpp
  src/hooks/backdrop_patch.cpp
  PROPERTIES COMPILE_OPTIONS "/std:c++17")
target_include_directories(zhlIsaacCS PRIVATE src "${GENERATED}" "${REPENTOGON_ROOT}/include" "${REPENTOGON_ROOT}/libzhl"
  "${REPENTOGON_ROOT}/libs/rapidjson/include" "${REPENTOGON_ROOT}/libs/rapidxml" "${REPENTOGON_ROOT}/libs/lua")
# 官方 ASMPatcher 公共头引用 Zydis/Zycore 类型；实现仍从 libzhl 导入，不另编译补丁库。
target_include_directories(zhlIsaacCS PRIVATE "${REPENTOGON_ROOT}/libs/zydis/include"
  "${REPENTOGON_ROOT}/libs/zydis/dependencies/zycore/include")
target_compile_definitions(zhlIsaacCS PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX _CRT_SECURE_NO_WARNINGS)
target_compile_options(zhlIsaacCS PRIVATE /EHs /W4 /Zi /utf-8 /wd4251 /wd4996)
target_link_libraries(zhlIsaacCS PRIVATE isaac_cs_core isaac_cs_net isaac_cs_world isaac_cs_replica "${REPENTOGON_ROOT}/build/Release/libzhl.lib" "${REPENTOGON_ROOT}/build/Release/Lua5.3.3r.lib" ws2_32 bcrypt opengl32)
target_link_options(zhlIsaacCS PRIVATE /DEBUG)
set_target_properties(zhlIsaacCS PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/../out/repentogon")
