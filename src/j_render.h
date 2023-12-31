#pragma once

#include <glad/glad.h>

#include "j_buffers.h"
#include "structs.h"

bool check_shader_compile_error(GLuint shader);

bool check_shader_link_error(GLuint shader);

int compile_shader(const char* vertex_shader_path, const char* fragment_shader_path, MemoryBuffer* buffer);

void draw_billboard(glm::vec3 position, Texture texture, float scale);

void draw_mesh_shadow_map(Mesh* mesh, Spotlight* spotlight);

void draw_mesh(Mesh* mesh);

void draw_mesh_wireframe(Mesh* mesh, glm::vec3 color);

void append_line(glm::vec3 start, glm::vec3 end, glm::vec3 color);

void draw_lines(float thickness);

void draw_lines_ontop(float thickness);

void init_all_shaders();

void draw_shadow_map_debug_screen(s64 spotlight_index);

void append_ui_text(FontData* font_data, char* text, float pos_x_vw, float pos_y_vh);

void draw_ui_text(FontData* font_data, float red, float green, float blue);

unsigned int load_cubemap();

void draw_skybox();

void append_simple_rect(glm::vec2 offset, glm::vec3 color, int texture_index);

void draw_simple_rects();

int load_image_into_texture_id(char* image_path);

void update_ubos();

void draw_shadow_map_framebuffers();

void draw_scene_framebuffer();

void draw_editor_framebuffer();

void draw_main_framebuffer();
