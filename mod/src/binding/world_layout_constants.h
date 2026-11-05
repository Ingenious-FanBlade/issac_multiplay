#pragma once
#include <cstdint>
namespace cs::world::layout {
// SDK-facing C++17只读取生成的布局常量，不引入C++20观测模型。定义仍由world_layout.cpp唯一拥有。
struct Constants final {
  std::uint32_t sprite_null_names_offset;
  std::uint32_t sprite_event_names_offset;
  // Player ctor逐成员构造的13份内嵌ANM；物理slot不猜测未知用途，不按文件合并。
  std::uint32_t player_inline_sprite_first_offset;
  std::uint32_t player_inline_sprite_stride;
  std::uint32_t player_inline_sprite_count;
  std::uint32_t player_sub_storage_offset;
  std::uint32_t player_twin_storage_offset;
  std::uint32_t player_backup_storage_offset;
  std::uint32_t player_replaced_storage_offset;
  std::uint32_t entity_valid_offset;
  std::uint32_t entity_exists_offset;
  std::uint32_t player_type_offset;
  std::uint32_t player_index_offset;
  std::uint32_t player_active_desc_first_offset;
  std::uint32_t player_active_desc_stride;
  std::uint32_t player_active_var_data_offset;
  std::uint32_t game_player_borrow_first_offset;
  std::uint32_t game_player_borrow_count;

  std::uint32_t entity_text_td_rva;
  std::uint32_t entity_dummy_td_rva;
  std::uint32_t entity_text_counter_offset;
  std::uint32_t entity_text_duration_offset;
  std::uint32_t entity_text_buffer_offset;
  std::uint32_t entity_text_capacity;
  std::uint32_t entity_text_color_offset;
  std::uint32_t entity_text_variant_offset;
  std::uint32_t entity_text_writer_return_rva;
  std::uint32_t entity_laser_td_rva;
  std::uint32_t entity_laser_sample_mode_offset;
  std::uint32_t entity_laser_start_offset_mode_offset;

  std::uint32_t game_root_rva;
  std::uint32_t game_frame_offset;
  std::uint32_t game_room_offset;
  std::uint32_t game_room_index_offset;
  std::uint32_t game_dimension_offset;
  std::uint32_t game_object_size;
  std::uint32_t room_object_size;
  std::uint32_t player_manager_object_size;
  std::uint32_t game_player_manager_offset;
  std::uint32_t room_grid_width_offset;
  std::uint32_t room_grid_height_offset;
  std::uint32_t room_entity_list_offset;
  std::uint32_t room_entity_list_first_el_offset;
  std::uint32_t room_entity_list_stride;
  std::uint32_t room_entity_list_count;
  std::uint32_t room_grid_slot_offset;
  std::uint32_t room_grid_slot_count;
  std::uint32_t room_grid_max_index;
  std::uint32_t room_door_slot_offset;
  std::uint32_t room_door_slot_count;
  std::uint32_t room_door_grid_map_offset;
  std::uint32_t room_constructor_rva;
  std::uint32_t room_constructor_clear_offset;
  std::uint32_t room_constructor_clear_bytes;
  std::uint32_t player_manager_roster_offset;
  std::uint32_t player_manager_esau_offset;
  std::uint32_t esau_slot_count;
  std::uint32_t relation_node_budget;
  std::uint32_t entity_flags_offset;
  std::uint32_t entity_hp_offset;
  std::uint32_t entity_max_hp_offset;
  std::uint32_t entity_frame_stamp_offset;
  float grid_position_step;
  float grid_position_origin_x;
  float grid_position_origin_y;
  std::uint32_t grid_position_getter_rva;
  std::uint32_t grid_index_by_tile_rva;
  std::uint32_t rtti_complete_object_locator_size;
  std::uint32_t rtti_class_hierarchy_descriptor_size;
  std::uint32_t rtti_base_class_descriptor_size;
  std::uint32_t rtti_expected_signature;
  std::uint32_t rtti_expected_hierarchy_attributes;
  std::uint32_t rtti_expected_base_attributes;
  std::uint32_t entity_list_descriptor_size;
  std::uint32_t entity_list_descriptor_data_offset;
  std::uint32_t entity_list_descriptor_capacity_offset;
  std::uint32_t entity_list_descriptor_size_offset;
  std::uint32_t roster_vector_size;
  std::uint32_t roster_vector_begin_offset;
  std::uint32_t roster_vector_end_offset;
  std::uint32_t roster_vector_capacity_offset;
};
const Constants &LayoutConstants() noexcept;
}
