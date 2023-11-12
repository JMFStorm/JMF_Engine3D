#include "globals.h"

bool DEBUG_SHADOWMAP = false;

GameCamera g_scene_camera = {};
bool g_camera_move_mode = false;

GameMetrics g_game_metrics = {};

GameInputsU g_inputs = {};

FrameData g_frame_data = {};

std::unordered_map<char*, s64, CharPtrHash, CharPtrEqual> g_mat_data_map = {};
std::unordered_map<char*, s64, CharPtrHash, CharPtrEqual> g_materials_index_map = {};
std::unordered_map<s64, char*> g_mat_data_map_inverse = {};

TransformationMode g_transform_mode = {};
