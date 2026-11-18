cmake_minimum_required(VERSION 3.24)
project(IsaacCS LANGUAGES C CXX)
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
# 官方zstd源码静态并入同一Win32 /MD产物；不构建工具、测试或下载运行时DLL。
file(GLOB ZSTD_SOURCES CONFIGURE_DEPENDS third_party/zstd/lib/common/*.c
  third_party/zstd/lib/compress/*.c third_party/zstd/lib/decompress/*.c)
add_library(isaac_cs_zstd STATIC ${ZSTD_SOURCES})
target_include_directories(isaac_cs_zstd PUBLIC third_party/zstd/lib)
target_compile_definitions(isaac_cs_zstd PRIVATE ZSTD_DISABLE_ASM ZSTD_LEGACY_SUPPORT=0)
target_compile_options(isaac_cs_zstd PRIVATE /W3 /utf-8)
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
  src/world/storage.cpp src/world/writer.cpp src/world/observe.cpp src/world/collector.cpp src/world/player_child_source.cpp
  src/world/common/contracts.cpp src/world/common/service.cpp src/world/common/publication.cpp src/world/common/live_consumer.cpp
  src/world/common/native_source.cpp src/world/common/membership.cpp src/world/compatibility/min_read_v1.cpp src/world/compatibility/encode.cpp
  src/world/domains/entity/collect.cpp src/world/domains/grid/collect.cpp
  src/world/common/frame_read_regions.cpp
  src/world/binary/read_session.cpp src/world/binary/entity_source.cpp src/world/binary/entity_flat_capture.cpp src/world/binary/entity_source_reuse.cpp src/world/binary/entity_containers.cpp src/world/binary/entity_resources.cpp src/world/binary/resource_export.cpp
  src/world/binary/entity_player.cpp src/world/binary/grid_source.cpp src/world/binary/scene_source.cpp src/world/binary/scene_map.cpp src/world/binary/run_source.cpp src/world/binary/display_source.cpp src/world/binary/presentation_source.cpp src/world/binary/hud_probability.cpp)
target_include_directories(isaac_cs_world PUBLIC src)
target_compile_features(isaac_cs_world PUBLIC cxx_std_20)
target_compile_definitions(isaac_cs_world PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_compile_options(isaac_cs_world PRIVATE /W4 /Zi /utf-8 /EHsc)
target_link_libraries(isaac_cs_world PUBLIC isaac_cs_core)
set(GENERATED "${CMAKE_CURRENT_BINARY_DIR}/generated")
# 原生二进制布局与旧 stream 目录分开生成；已登记域共用正式复制／解码及嵌套表。
# 第 10—14 步再接入正式同步并退役旧路径，这里不改变运行中的采集与协议。
file(GLOB STATE_LAYOUT_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/binding/state/*.json")
add_custom_command(OUTPUT "${GENERATED}/state_capture.inc" "${GENERATED}/state_wire.inc"
  "${GENERATED}/state_client.inc" "${GENERATED}/state_members.inc" "${GENERATED}/state_grid_view.inc" "${GENERATED}/state_flat.inc"
  "${GENERATED}/state_room_view.inc" "${GENERATED}/state_body_codec.inc" "${GENERATED}/state_resource_protocol.inc" "${GENERATED}/state_protocol.inc" "${GENERATED}/state_budget.inc"
    "${GENERATED}/state_display_install.inc" "${GENERATED}/state_host_fields.inc" "${GENERATED}/state_grid_factory.inc"
  COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_state_layout.py"
    --out "${GENERATED}" --docs "${CMAKE_CURRENT_SOURCE_DIR}/../docs/design/binary-state/layouts"
  DEPENDS tools/gen_state_layout.py tools/gen_flat_layout.py ${STATE_LAYOUT_SOURCES} binding/world_layout.json
    "${CMAKE_CURRENT_SOURCE_DIR}/../docs/design/tx-04-19/entity-application.json"
    "${CMAKE_CURRENT_SOURCE_DIR}/../docs/design/tx-04-20/player-application.json"
    "${CMAKE_CURRENT_SOURCE_DIR}/../docs/design/observation-expansion/grid-catalog.json"
    "${CMAKE_CURRENT_SOURCE_DIR}/../docs/design/observation-expansion/run-catalog.json"
    "${CMAKE_CURRENT_SOURCE_DIR}/../docs/design/tx-04-21/stream-catalog.json" VERBATIM)
# AVX2 独立为机器码目标，链接期优化不能把它内联到 SSE2 基线路径。
add_library(isaac_cs_flat_avx2 OBJECT src/state/flat_simd_avx2.cpp)
target_compile_options(isaac_cs_flat_avx2 PRIVATE /arch:AVX2 /W4 /Zi /utf-8)
set_property(TARGET isaac_cs_flat_avx2 PROPERTY INTERPROCEDURAL_OPTIMIZATION FALSE)
add_library(isaac_cs_state STATIC src/state/layout.cpp src/state/nested.cpp src/state/owned_payload.cpp src/state/entity_extension.cpp src/state/entity_counters.cpp src/state/npc_presentation.cpp src/state/body_codec.cpp src/state/resource_protocol.cpp src/state/wire_protocol.cpp src/state/acceptance.cpp src/state/budget.cpp
  src/state/capture.cpp src/state/capture_values.cpp src/state/flat_capture.cpp src/state/capture_exchange.cpp src/state/capture_executor.cpp src/state/production_cache.cpp src/state/events.cpp src/state/host_publish.cpp
  src/state/flat_encode.cpp src/state/flat_simd_sse2.cpp $<TARGET_OBJECTS:isaac_cs_flat_avx2>
  src/state/client_memory.cpp src/state/client_tables.cpp src/state/client_shadow.cpp src/state/client_events.cpp src/state/client_receive.cpp src/state/client_validation.cpp src/state/client_plan.cpp src/state/client_apply.cpp
  "${GENERATED}/state_capture.inc" "${GENERATED}/state_wire.inc" "${GENERATED}/state_flat.inc"
  "${GENERATED}/state_client.inc" "${GENERATED}/state_members.inc" "${GENERATED}/state_body_codec.inc" "${GENERATED}/state_resource_protocol.inc" "${GENERATED}/state_protocol.inc" "${GENERATED}/state_budget.inc")
set_source_files_properties(src/state/flat_simd_sse2.cpp PROPERTIES COMPILE_OPTIONS "/arch:SSE2")
target_include_directories(isaac_cs_state PUBLIC src PRIVATE "${GENERATED}")
target_include_directories(isaac_cs_world PRIVATE "${GENERATED}")
target_compile_features(isaac_cs_state PUBLIC cxx_std_20)
target_compile_options(isaac_cs_state PRIVATE /W4 /Zi /utf-8 /EHsc)
target_link_libraries(isaac_cs_world PUBLIC isaac_cs_state)
# 第6步只编译由完整影子记录生成纯值描述的模块；第12/13步再接解码调度与原生安装。
add_library(isaac_cs_grid_description STATIC src/game/client/grid/client_grid_description.cpp
  "${GENERATED}/state_grid_view.inc")
target_include_directories(isaac_cs_grid_description PUBLIC src PRIVATE "${GENERATED}")
target_compile_features(isaac_cs_grid_description PUBLIC cxx_std_20)
target_compile_options(isaac_cs_grid_description PRIVATE /W4 /Zi /utf-8 /EHsc)
target_link_libraries(isaac_cs_grid_description PUBLIC isaac_cs_state)
# 第7步Room／Level描述只消费完整影子值；实际解码调度和原生安装仍由第12/13步接线。
add_library(isaac_cs_room_description STATIC src/game/client/room/client_room_description.cpp
  src/game/client/display/client_display.cpp src/game/client/scene/client_install_plan.cpp "${GENERATED}/state_display_install.inc"
  "${GENERATED}/state_room_view.inc")
target_include_directories(isaac_cs_room_description PUBLIC src PRIVATE "${GENERATED}")
target_compile_features(isaac_cs_room_description PUBLIC cxx_std_20)
target_compile_options(isaac_cs_room_description PRIVATE /W4 /Zi /utf-8 /EHsc)
target_link_libraries(isaac_cs_room_description PUBLIC isaac_cs_state isaac_cs_grid_description)
# 发布元数据只读取同一源码版本表，不探测游戏，也不运行校验程序。
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/src/replica/stream_version.h" STREAM_VERSION_SOURCE)
string(REGEX MATCH "kFraming=([0-9]+),kSchema=([0-9]+),kCompactSchema=([0-9]+)" _ "${STREAM_VERSION_SOURCE}")
set(STREAM_FRAMING "${CMAKE_MATCH_1}")
set(STREAM_SCHEMA "${CMAKE_MATCH_2}")
set(STREAM_COMPACT "${CMAKE_MATCH_3}")
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/../VERSION" ISAACCS_VERSION)
string(STRIP "${ISAACCS_VERSION}" ISAACCS_VERSION)
# 旧观测目录仅由离线对照目标生成，不参与正式协议。
file(GLOB REPLICA_CATALOGS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/observation/isaac-cs-observation/*catalog.json")
add_custom_command(OUTPUT "${GENERATED}/replication_catalog.inc" "${GENERATED}/replication-catalog.json"
  COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_replica_catalog.py" --out "${GENERATED}"
  DEPENDS tools/gen_replica_catalog.py tools/cpp_catalog.py tools/replica_projection_policy.py ${REPLICA_CATALOGS} VERBATIM)
add_custom_command(OUTPUT "${GENERATED}/room_grid_application.inc" "${GENERATED}/room-grid-application.json"
  COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_room_grid_catalog.py" --out "${GENERATED}"
  DEPENDS tools/gen_room_grid_catalog.py tools/gen_replica_catalog.py tools/cpp_catalog.py tools/replica_projection_policy.py ${REPLICA_CATALOGS} VERBATIM)
add_custom_command(OUTPUT "${GENERATED}/entity_application.inc" "${GENERATED}/entity-application.json"
  COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_entity_application_catalog.py" --out "${GENERATED}"
  # 静态合同随功能目录生成；修改研究索引也必须更新嵌入目录，避免残留旧采用约束。
  DEPENDS tools/gen_entity_application_catalog.py tools/gen_replica_catalog.py tools/cpp_catalog.py tools/replica_projection_policy.py
   src/game/client/entity/client_entity_fields.inc src/game/client/entity/client_entity_class_fields.inc src/game/client/entity/client_entity_links.inc
    "${CMAKE_CURRENT_SOURCE_DIR}/../docs/design/tx-04-19/entity-native-contracts.json" ${REPLICA_CATALOGS} VERBATIM)
add_custom_command(OUTPUT "${GENERATED}/player_application.inc" "${GENERATED}/player-application.json"
  COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_player_application_catalog.py" --out "${GENERATED}"
  DEPENDS tools/gen_player_application_catalog.py tools/gen_entity_application_catalog.py tools/gen_replica_catalog.py tools/cpp_catalog.py tools/replica_projection_policy.py
   src/game/client/player/client_player_fields.inc src/game/client/player/client_player_roles.inc
    "${CMAKE_CURRENT_SOURCE_DIR}/../docs/design/tx-04-20/player-native-contracts.json" ${REPLICA_CATALOGS} VERBATIM)
# 控制编码只生成固定名称，状态字段由二进制布局生成器管理。
add_custom_command(OUTPUT "${GENERATED}/stream_names_by_text.inc" "${GENERATED}/stream_names_by_code.inc"
  COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_control_names.py"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/replica/stream_fixed_names.json" "${GENERATED}"
  DEPENDS tools/gen_control_names.py src/replica/stream_fixed_names.json VERBATIM)
add_library(isaac_cs_replica STATIC src/replica/model.cpp src/replica/stream_scope.cpp
  src/replica/stream_config.cpp src/replica/stream_metrics.cpp src/replica/stream_capture_cost.cpp
  src/replica/stream_codec.cpp src/replica/stream_binary.cpp src/replica/stream_transport.cpp
  "${GENERATED}/stream_names_by_text.inc" "${GENERATED}/stream_names_by_code.inc")
target_include_directories(isaac_cs_replica PUBLIC src "${REPENTOGON_ROOT}/libs/rapidjson/include" PRIVATE "${GENERATED}")
target_compile_options(isaac_cs_replica PRIVATE /W4 /Zi /utf-8 /EHsc)
target_compile_definitions(isaac_cs_replica PUBLIC _SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING)
target_link_libraries(isaac_cs_replica PUBLIC isaac_cs_world isaac_cs_state PRIVATE isaac_cs_zstd)
# 真实游戏连接复用复制合同，网络后台不引用SDK或游戏对象。
target_sources(isaac_cs_net PRIVATE src/net/live.cpp src/net/stream_session.cpp)
target_link_libraries(isaac_cs_net PUBLIC isaac_cs_replica)
# E2E工具与正式DLL分开打包；同一库用于正式主线程纯值适配和统一验收。
option(ISAACCS_BUILD_E2E_TOOLS "Build explicitly requested E2E tools outside product install tree" OFF)
if(ISAACCS_BUILD_E2E_TOOLS)
  target_compile_definitions(isaac_cs_replica PUBLIC ISAACCS_E2E=1)
  # Client 的 M0 聚合在状态库及调用方共同编译，保持诊断账户布局一致。
  target_compile_definitions(isaac_cs_state PUBLIC ISAACCS_E2E=1)
  # 旧目录只服务离线对照工具，正式网络与原生场景均不消费。
  target_sources(isaac_cs_replica PRIVATE src/replica/stream_e2e.cpp src/replica/codec.cpp
    src/replica/stream_source_value.cpp src/replica/projection_graph.cpp
    src/replica/room_grid_target.cpp src/replica/room_grid_budget.cpp
    src/replica/entity_target.cpp src/replica/player_target.cpp
    "${GENERATED}/replication_catalog.inc" "${GENERATED}/room_grid_application.inc"
    "${GENERATED}/entity_application.inc" "${GENERATED}/player_application.inc")
set(ISAACCS_BUILD_OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/../out/repentogon/e2e")
add_executable(replica-contract ../tools/replica-contract/main.cpp)
target_link_libraries(replica-contract PRIVATE isaac_cs_replica)
target_compile_options(replica-contract PRIVATE /W4 /Zi /utf-8 /EHsc)
set_target_properties(replica-contract PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE "${ISAACCS_BUILD_OUTPUT}")
add_executable(client-fixture ../tools/client-fixture/main.cpp)
target_link_libraries(client-fixture PRIVATE isaac_cs_replica)
target_compile_options(client-fixture PRIVATE /W4 /Zi /utf-8 /EHsc)
set_target_properties(client-fixture PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE "${ISAACCS_BUILD_OUTPUT}")
else()
set(ISAACCS_BUILD_OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/../out/repentogon")
endif()
add_custom_command(OUTPUT "${GENERATED}/hooks_generated.cpp" "${GENERATED}/binding_v1_9_7_12.inc" "${GENERATED}/hook-manifest.json" "${GENERATED}/permanent_patches.inc" "${GENERATED}/host_patches.inc" "${GENERATED}/hook_capacity.h" "${GENERATED}/world_layout.inc" "${GENERATED}/world-layout.json"
  BYPRODUCTS "${GENERATED}/exit_patch.inc" "${GENERATED}/shutdown_patch.inc" "${GENERATED}/backdrop_patch.inc" "${GENERATED}/display_patch.inc" "${GENERATED}/random_runtime.inc" "${GENERATED}/client_display_memory_imports.inc" "${GENERATED}/hook-manifest.md"
  COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_binding.py" --out "${GENERATED}"
  DEPENDS tools/gen_binding.py binding/hooks.json binding/world_layout.json binding/v1_9_7_12-static.json ../docs/hooks/repentogon-binding.json VERBATIM)
# game 按职责分目录；保持既有源码登记顺序、目标和编译条件，只迁移路径。
add_library(zhlIsaacCS SHARED src/bootstrap/mod_init.cpp src/binding/definitions.cpp
  src/binding/world_layout.cpp src/hooks/registry.cpp src/hooks/context.cpp
  src/hooks/pointer_patch.cpp src/hooks/odd_patch.cpp src/hooks/host_patch.cpp src/hooks/exit_patch.cpp
  src/game/common/context.cpp src/game/common/player_api.cpp
  src/game/observation/sources/loot.cpp
  src/game/observation/sources/observation.cpp src/game/participants/topology.cpp
  src/game/authority/tick.cpp src/game/input/input_a.cpp
  src/game/input/query.cpp src/game/input/consumers.cpp src/game/authority/lifecycle.cpp
  src/game/participants/remote_life.cpp src/game/observation/world_bridge.cpp
  src/game/observation/binary_entities.cpp src/game/observation/binary_production.cpp src/game/observation/binary_display_capture.cpp src/game/observation/binary_intentions.cpp src/game/observation/binary_probability.cpp
  src/game/observation/sources/global_random.cpp
  src/game/observation/sources/random_sources.cpp src/game/observation/sources/random_runtime_lua.cpp
  src/game/observation/sources/random_owners.cpp src/hooks/random_runtime.cpp
  "${REPENTOGON_ROOT}/libzhl/hde32/src/hde32.cpp"
  src/game/participants/device_k.cpp src/game/participants/join.cpp src/game/participants/peer.cpp
  src/game/input/input_lease.cpp src/game/authority/persistence.cpp
  src/game/client/session/client.cpp src/game/client/scene/client_scene.cpp
  src/game/client/display/client_display_native.cpp
  src/game/client/display/client_display_memory.cpp
  src/game/client/scene/client_preparation_native.cpp
  src/game/client/scene/client_preparation.cpp
  src/game/client/scene/client_field_staging.cpp src/game/client/scene/client_field_staging_native.cpp
  src/game/client/scene/client_atomic_install.cpp src/game/client/scene/client_atomic_install_native.cpp
  src/game/client/scene/client_binary_scene.cpp
  src/game/client/scene/client_atomic_bindings.cpp src/game/client/scene/client_atomic_bindings_native.cpp
  src/game/client/scene/client_native_bridge.cpp
  src/game/client/room/client_backdrop_native.cpp
  src/game/client/player/client_player_control.cpp
  src/game/client/isolation/client_player_isolation.cpp src/game/client/isolation/client_isolation.cpp
  src/game/client/entity/client_entity_adapters_native.cpp src/game/client/entity/client_entity_pool_native.cpp
  src/game/client/entity/client_entity_npc_native.cpp
  src/hooks/backdrop_patch.cpp src/hooks/display_patch.cpp
  src/ops/config.cpp src/ops/trace.cpp src/ops/command.cpp src/ops/world_commands.cpp src/ops/control_port.cpp
  "${GENERATED}/hook_capacity.h" "${GENERATED}/world_layout.inc"
  "${GENERATED}/exit_patch.inc"
  "${GENERATED}/shutdown_patch.inc"
  "${GENERATED}/hooks_generated.cpp")
# 文件导入/导出、故障注入与原生测试只进入显式 E2E DLL；正式目标没有这些实现。
if(ISAACCS_BUILD_E2E_TOOLS)
  target_sources(zhlIsaacCS PRIVATE src/ops/e2e_ui.cpp
   src/game/observation/sources/lua_entity.cpp src/game/observation/sources/display_observation.cpp
   src/game/client/room/client_room_native.cpp src/game/client/room/client_map_native.cpp
   src/game/client/grid/client_grid_observation.cpp src/game/client/entity/client_entity_collections_native.cpp
   src/game/client/entity/client_entity_owned_native.cpp
   src/game/client/player/client_player_state_native.cpp src/game/client/player/client_player_roles_native.cpp
   src/game/client/player/client_player_draw_native.cpp src/game/client/player/client_player_hud_native.cpp src/game/client/player/client_player_equipment_native.cpp
   src/game/client/player/client_player_weapon_native.cpp
   src/game/client/resources/client_resource_shaders_native.cpp
   src/game/tests/client_e2e_native.cpp src/game/tests/client_entity_e2e.cpp
   src/game/tests/client_player_e2e.cpp src/game/tests/random_e2e.cpp)
  target_compile_definitions(zhlIsaacCS PRIVATE ISAACCS_E2E=1)
  target_compile_definitions(zhlIsaacCS PRIVATE "ISAACCS_REPOSITORY_ROOT=\"${CMAKE_CURRENT_SOURCE_DIR}/..\"")
  set(ISAACCS_DLL_OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/../out/repentogon/e2e")
  set(ISAACCS_BUILD_FLAVOR "e2e")
else()
  set(ISAACCS_DLL_OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/../out/repentogon")
  set(ISAACCS_BUILD_FLAVOR "product")
endif()
file(GENERATE OUTPUT "${ISAACCS_DLL_OUTPUT}/build-flavor.json"
  CONTENT "{\"format\":\"isaac.cs.build\",\"version\":2,\"product_version\":\"${ISAACCS_VERSION}\",\"flavor\":\"${ISAACCS_BUILD_FLAVOR}\",\"e2e_tools\":$<IF:$<BOOL:${ISAACCS_BUILD_E2E_TOOLS}>,true,false>,\"framing\":${STREAM_FRAMING},\"stream_schema\":${STREAM_SCHEMA},\"compact_schema\":${STREAM_COMPACT},\"target\":\"REPENTOGON-1.1.3-J273-Win32-v143-MD\",\"compression\":\"zstd-static\",\"state_contract\":\"binary-state-v1\"}\n")
# SDK 1.1.3 的生成头和 ASMPatcher.hpp 的 friend-static 声明不兼容当前 C++20。
# SDK-facing 单元只使用 C++17 值接口；Join/输入/world 核心保持 C++20。
# 显式覆盖逐文件标准，不降低整个 DLL 标准，也不修改 SDK 头或补丁/汇编行为。
set_source_files_properties(src/game/observation/sources/loot.cpp
  src/game/client/scene/client_preparation_native.cpp
  src/game/client/scene/client_field_staging_native.cpp
  src/game/client/scene/client_atomic_install_native.cpp
  src/game/client/scene/client_atomic_bindings_native.cpp
  src/game/common/player_api.cpp src/game/observation/binary_intentions.cpp src/game/observation/binary_probability.cpp src/game/client/scene/client_native_bridge.cpp
  src/binding/definitions.cpp "${GENERATED}/hooks_generated.cpp"
  src/game/client/room/client_backdrop_native.cpp
  src/game/client/entity/client_entity_adapters_native.cpp src/game/client/entity/client_entity_pool_native.cpp
  src/game/client/entity/client_entity_npc_native.cpp
  src/game/client/display/client_display_native.cpp
  src/game/tests/client_e2e_native.cpp
  src/game/observation/sources/lua_entity.cpp src/game/observation/sources/display_observation.cpp
  src/game/client/room/client_room_native.cpp src/game/client/room/client_map_native.cpp
  src/game/client/grid/client_grid_observation.cpp src/game/client/entity/client_entity_collections_native.cpp
  src/game/client/entity/client_entity_owned_native.cpp
  src/game/client/player/client_player_state_native.cpp src/game/client/player/client_player_roles_native.cpp
  src/game/client/player/client_player_draw_native.cpp src/game/client/player/client_player_hud_native.cpp src/game/client/player/client_player_equipment_native.cpp
  src/game/client/player/client_player_weapon_native.cpp src/game/client/resources/client_resource_shaders_native.cpp
  src/game/observation/sources/random_runtime_lua.cpp src/game/tests/random_e2e.cpp
  src/game/observation/sources/random_owners.cpp
  src/hooks/host_patch.cpp src/hooks/odd_patch.cpp src/hooks/exit_patch.cpp
  src/hooks/backdrop_patch.cpp src/hooks/display_patch.cpp
  PROPERTIES COMPILE_OPTIONS "/std:c++17")
target_include_directories(zhlIsaacCS PRIVATE src "${GENERATED}" "${REPENTOGON_ROOT}/include" "${REPENTOGON_ROOT}/libzhl"
  "${REPENTOGON_ROOT}/repentogon/LuaInterfaces"
  "${REPENTOGON_ROOT}/repentogon/Patches"
  "${REPENTOGON_ROOT}/libs/rapidjson/include/rapidjson"
  "${REPENTOGON_ROOT}/libs/rapidjson/include" "${REPENTOGON_ROOT}/libs/rapidxml" "${REPENTOGON_ROOT}/libs/lua")
# 官方 ASMPatcher 公共头引用 Zydis/Zycore 类型；实现仍从 libzhl 导入，不另编译补丁库。
target_include_directories(zhlIsaacCS PRIVATE "${REPENTOGON_ROOT}/libs/zydis/include"
  "${REPENTOGON_ROOT}/libs/zydis/dependencies/zycore/include")
target_compile_definitions(zhlIsaacCS PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX _CRT_SECURE_NO_WARNINGS)
target_compile_options(zhlIsaacCS PRIVATE /EHs /W4 /Zi /utf-8 /wd4251 /wd4996)
target_link_libraries(zhlIsaacCS PRIVATE isaac_cs_core isaac_cs_net isaac_cs_world isaac_cs_replica isaac_cs_grid_description isaac_cs_room_description "${REPENTOGON_ROOT}/build/Release/libzhl.lib" "${REPENTOGON_ROOT}/build/Release/Lua5.3.3r.lib" ws2_32 bcrypt opengl32)
target_link_options(zhlIsaacCS PRIVATE /DEBUG)
# 热路径跨采集／布局／写入多个静态库，Release允许链接期内联和常量传播。
# 仅优化本工程对象，保留原有检查与异常语义；外部官方SDK仍按其原ABI链接。
set_property(TARGET isaac_cs_core isaac_cs_net isaac_cs_world isaac_cs_state
  isaac_cs_grid_description isaac_cs_room_description isaac_cs_replica
  isaac_cs_zstd zhlIsaacCS PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
# 只编译官方头的布局声明；其全局配置初始化不属于本DLL，不能链接或执行。
add_library(isaac_cs_player_sdk_abi OBJECT src/game/client/player/client_player_sdk_abi.cpp)
target_include_directories(isaac_cs_player_sdk_abi PRIVATE $<TARGET_PROPERTY:zhlIsaacCS,INCLUDE_DIRECTORIES>)
target_compile_definitions(isaac_cs_player_sdk_abi PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX _CRT_SECURE_NO_WARNINGS)
target_compile_options(isaac_cs_player_sdk_abi PRIVATE /std:c++17 /utf-8 /EHsc)
add_dependencies(zhlIsaacCS isaac_cs_player_sdk_abi)
set_target_properties(zhlIsaacCS PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE "${ISAACCS_DLL_OUTPUT}")
