# tx-04-06 Hook manifest

Generated from mod/binding/hooks.json. Session controls gameplay activation.

| id | group | definition | RVA | priority | required nonzero |
|---|---|---|---|---|---|
| manager_update | loop | Manager::Update | 0x52e3e0 | -10000 | True |
| game_update | loop | Game::Update | 0x2f6d30 | 0 | True |
| post_update | loop | IsaacCS::PostUpdateCaller | 0x446c90 | 0 | True |
| game_exit | loop | Game::Exit | 0x2f6270 | 0 | True |
| new_room | loop | IsaacCS::NewRoomCaller | 0x44b930 | 0 | True |
| manager_render | loop | Manager::Render | 0x52ec40 | 0 | True |
| start_game | loop | Manager::ExecuteStartGame | 0x532a20 | 0 | True |
| post_start | loop | LuaEngine::PostGameStart | 0x44b660 | 0 | True |
| console_check | loop | IsaacCS::ConsoleGate | 0x287020 | 0 | True |
| room_update | loop | IsaacCS::RoomUpdate | 0x3f7f60 | 0 | True |
| entity_list | loop | IsaacCS::EntityListUpdate | 0x13f00 | 0 | True |
| restore | loop | Game::RestoreState | 0x2f43a0 | 0 | False |
| room_lifecycle | loop | IsaacCS::RoomLifecycleFSM | 0x509100 | 0 | False |
| door_update | loop | RoomTransition::Update | 0x426430 | 0 | False |
| door_nested | loop | IsaacCS::DoorNestedOwner | 0x2fa0f0 | 0 | False |
| odd_helper | loop | IsaacCS::OddHelper | 0x2f9340 | 0 | False |
| input_update | input | IsaacCS::InputManagerUpdate | 0x5edcc0 | 0 | True |
| device_update | input | IsaacCS::DeviceCommonUpdate | 0x5ec900 | 0 | True |
| player_update | player | Entity_Player::Update | 0x37bd40 | -10000 | True |
| player_native | player | Entity_Player::Update | 0x37bd40 | 10000 | True |
| move | player | IsaacCS::ConsumerMove | 0x372540 | 0 | True |
| bomb | player | IsaacCS::ConsumerBomb | 0x37a210 | 0 | False |
| active_item | player | Entity_Player::ControlActiveItem | 0x389e50 | 0 | False |
| card_pill | player | IsaacCS::ConsumerCardPill | 0x39bdd0 | 0 | False |
| shoot | player | IsaacCS::ConsumerShoot | 0x373990 | 0 | True |
| drop | player | IsaacCS::ConsumerDrop | 0x39c300 | 0 | True |
| join_pass | join | IsaacCS::JoinPass | 0x591360 | 0 | True |
| console_command | control | Console::RunCommand | 0x288100 | -10000 | True |

Pointer hooks: Keyboard/Gamepad slots 8, 9, 10, 35. Permanent.
OddNormal: 0x2F9559, expected FF 52 0C 6A 00, resume 0x2F955E. Permanent.

Host/K: join_mode_1 @ 0x5915ae, expected E8 ED FA FF FF, resume 0x5915b3. Permanent.

Host/K: join_mode_2 @ 0x59174c, expected E8 4F F9 FF FF, resume 0x591751. Permanent.

Host/K: join_mode_3 @ 0x591e67, expected E8 34 F2 FF FF, resume 0x591e6c. Permanent.

Host/K: join_create @ 0x591fe6, expected E8 65 E2 FF FF, resume 0x591feb. Permanent.

Host/K: k_register_notify @ 0x5eeb7c, expected 8B 77 14 8B 5B 08, resume 0x5eeb82. Permanent.

Host/K: k_remove_notify @ 0x5eebc3, expected 8B 73 20 3B 73 24, resume 0x5eebc9. Permanent.

Host/K: k_poll_notify @ 0x5edd2d, expected 8B 73 20 8B 41 08, resume 0x5edd33. Permanent.

Host/K: k_physical_disconnect @ 0x5ec37c, expected 8B 4D 08 8B 87 F8 03 00 00, resume 0x5ec385. Permanent.

Host/K: k_controller_list @ 0x4af970, expected 85 C0 0F 84 D5 00 00 00, resume 0x4af978. Permanent.

Host/K: k_keyconfig_draw @ 0x4b8d3d, expected 83 B8 8C 04 00 00 00, resume 0x4b8d44. Permanent.

Host/K: k_pause_disconnect @ 0x534768, expected 83 B8 8C 04 00 00 00, resume 0x53476f. Permanent.
