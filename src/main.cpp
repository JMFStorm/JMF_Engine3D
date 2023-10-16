#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <ft2build.h>
#include FT_FREETYPE_H  

#include <array>
#include <iostream>
#include <fstream>

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>

#include "utils.h"

typedef struct UserSettings {
	int screen_size_px[2];
	int screen_aspect[2];
} UserSettings;

UserSettings g_user_settings = { 0 };

int g_simple_rectangle_shader;
unsigned int g_simple_rectangle_vao;
unsigned int g_simple_rectangle_vbo;

int g_mesh_shader;
unsigned int g_mesh_vao;
unsigned int g_mesh_vbo;

int g_ui_text_shader;
unsigned int g_ui_text_vao;
unsigned int g_ui_text_vbo;

int g_text_buffer_size = 0;
int g_text_indicies = 0;

typedef struct FrameData {
	int draw_calls;
	float deltatime;
	int mouse_click_x;
	int mouse_click_y;
	float prev_mouse_x;
	float prev_mouse_y;
	bool mouse_clicked;
} FrameData;

typedef struct ButtonState {
	int key;
	bool pressed;
	bool is_down;
} ButtonState;

typedef struct GameInputs {
	ButtonState mouse1;
	ButtonState mouse2;
	ButtonState q;
	ButtonState w;
	ButtonState e;
	ButtonState r;
	ButtonState a;
	ButtonState s;
	ButtonState d;
	ButtonState f;
	ButtonState z;
	ButtonState x;
	ButtonState c;
	ButtonState v;
	ButtonState esc;
	ButtonState plus;
	ButtonState minus;
} GameInputs;

GameInputs g_game_inputs = {};

typedef struct GameMetrics {
	unsigned long frames;
	int game_width_px;
	int game_height_px;
	int game_aspect_ratio_x;
	int game_aspect_ratio_y;
	double game_time;
	double prev_frame_game_time;
	int fps;
	int fps_frames;
	int fps_prev_second;
} GameMetrics;

typedef struct {
	int bot_left_x;
	int bot_left_y;
	int width;
	int height;
} Rectangle2D;

typedef struct CharData {
	float UV_x0;
	float UV_y0;
	float UV_x1;
	float UV_y1;
	int width;
	int height;
	int x_offset;
	int y_offset;
	int advance;
	char character;
} CharData;

typedef struct FontData {
	std::array<CharData, 96> char_data = { 0 };
	int texture_id;
	float font_scale;
	int font_height_px;
} FontData;

GameMetrics g_game_metrics = { 0 };

float debug_font_vh = 1.0f;
const char* g_debug_font_path = "G:/projects/game/Engine3D/resources/fonts/Inter-Regular.ttf";

FontData g_debug_font;

FrameData g_frame_data = { 0 };

int g_max_UI_chars = 1000;

MemoryBuffer g_temp_memory = { 0 };
MemoryBuffer g_ui_text_vertex_buffer = { 0 };

typedef struct GameCamera {
	glm::vec3 position;
	glm::vec3 front_vec;
	glm::vec3 up_vec;
	float yaw = -90.0f;	
	float pitch = 0.0f;
	float fov = 45.0f;
	float look_sensitivity = 0.1f;
	float move_speed = 2.5f;
} GameCamera;

GameCamera g_game_camera = {};

float g_mouse_movement_x = 0;
float g_mouse_movement_y = 0;

float mesh_rotation = 0;

typedef struct Plane {
	glm::vec3 translation;
	glm::vec3 rotation;
	glm::vec3 scale;
	int texture_id;
} Plane;

void read_file_to_memory(const char* file_path, MemoryBuffer* buffer)
{
	std::ifstream file_stream(file_path, std::ios::binary | std::ios::ate);
	ASSERT_TRUE(file_stream.is_open(), "Open file");

	std::streampos file_size = file_stream.tellg();
	ASSERT_TRUE(file_size <= buffer->size, "File size fits buffer");

	file_stream.seekg(0, std::ios::beg);

	char* read_pointer = (char*)buffer->memory;
	file_stream.read(read_pointer, file_size);
	file_stream.close();

	null_terminate_string(read_pointer, file_size);
}

bool check_shader_compile_error(GLuint shader)
{
	GLint success;
	constexpr int length = 512;
	GLchar infoLog[length];

	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

	if (!success)
	{
		glGetShaderInfoLog(shader, length, NULL, infoLog);
		std::cout 
			<< "ERROR::SHADER_COMPILATION_ERROR: "
			<< infoLog << std::endl;

		return false;
	}

	return true; // Returns success
}

bool check_shader_link_error(GLuint shader)
{
	GLint success;
	constexpr int length = 512;
	GLchar infoLog[length];

	glGetProgramiv(shader, GL_LINK_STATUS, &success);

	if (!success)
	{
		glGetProgramInfoLog(shader, length, NULL, infoLog);
		std::cout 
			<< "ERROR::PROGRAM_LINKING_ERROR: "
			<< infoLog << std::endl;

		return false;
	}

	return true; // Returns success
}

int compile_shader(const char* vertex_shader_path, const char* fragment_shader_path, MemoryBuffer* buffer)
{
	int shader_id;
	char* vertex_shader_code = nullptr;
	char* fragment_shader_code = nullptr;

	read_file_to_memory(vertex_shader_path, buffer);
	auto vertex_code = (char*)buffer->memory;
	int vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex_shader, 1, &vertex_code, NULL);
	glCompileShader(vertex_shader);

	bool vs_compile_success = check_shader_compile_error(vertex_shader);
	ASSERT_TRUE(vs_compile_success, "Vertex shader compile");

	read_file_to_memory(fragment_shader_path, buffer);
	auto fragment_code = (char*)buffer->memory;
	int fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment_shader, 1, &fragment_code, NULL);
	glCompileShader(fragment_shader);

	bool fs_compile_success = check_shader_compile_error(fragment_shader);
	ASSERT_TRUE(fs_compile_success, "Fragment shader compile");

	shader_id = glCreateProgram();
	glAttachShader(shader_id, vertex_shader);
	glAttachShader(shader_id, fragment_shader);
	glLinkProgram(shader_id);

	bool link_success = check_shader_link_error(shader_id);
	ASSERT_TRUE(link_success, "Shader program link");

	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);

	return shader_id;
}

int load_image_into_texture(const char* image_path, bool use_nearest)
{
	unsigned int texture;
	int x, y, n;

	stbi_set_flip_vertically_on_load(1);
	byte* data = stbi_load(image_path, &x, &y, &n, 0);
	ASSERT_TRUE(data != NULL, "Load texture");

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	GLint texture_mode = use_nearest ? GL_NEAREST : GL_LINEAR;

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, texture_mode);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, texture_mode);

	ASSERT_TRUE(n == 3 || n == 4, "Image format is RGB or RGBA");

	GLint use_format = n == 3
		? GL_RGB
		: GL_RGBA;

	glTexImage2D(GL_TEXTURE_2D, 0, use_format, x, y, 0, use_format, GL_UNSIGNED_BYTE, data);
	stbi_image_free(data);
	return texture;
}

void draw_simple_reactangle(Rectangle2D rect, float r, float g, float b)
{
	glUseProgram(g_simple_rectangle_shader);
	glBindVertexArray(g_simple_rectangle_vao);

	float x0 = normalize_screen_px_to_ndc(rect.bot_left_x, g_game_metrics.game_width_px);
	float y0 = normalize_screen_px_to_ndc(rect.bot_left_y, g_game_metrics.game_height_px);

	float x1 = normalize_screen_px_to_ndc(rect.bot_left_x + rect.width, g_game_metrics.game_width_px);
	float y1 = normalize_screen_px_to_ndc(rect.bot_left_y + rect.height, g_game_metrics.game_height_px);

	float vertices[] =
	{
		// Coords		// Color
		x0, y0, 0.0f,	r, g, b, // bottom left
		x1, y0, 0.0f,	r, g, b, // bottom right
		x0, y1, 0.0f,	r, g, b, // top left

		x0, y1, 0.0f,	r, g, b, // top left 
		x1, y1, 0.0f,	r, g, b, // top right
		x1, y0, 0.0f,	r, g, b  // bottom right
	};

	glBindBuffer(GL_ARRAY_BUFFER, g_simple_rectangle_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glDrawArrays(GL_TRIANGLES, 0, 6);
	g_frame_data.draw_calls++;

	glUseProgram(0);
	glBindVertexArray(0);
}

void draw_mesh(Plane* mesh)
{
	glUseProgram(g_mesh_shader);
	glBindVertexArray(g_mesh_vao);

	glm::mat4 model = glm::mat4(1.0f);

	model = glm::translate(model, mesh->translation);

	model = glm::rotate(model, glm::radians(mesh->rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::rotate(model, glm::radians(mesh->rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::rotate(model, glm::radians(mesh->rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

	model = glm::scale(model, mesh->scale);

	glm::mat4 projection = glm::perspective(glm::radians(g_game_camera.fov), (float)4 / (float)3, 0.1f, 100.0f);

	auto new_mat_4 = g_game_camera.position + g_game_camera.front_vec;
	glm::mat4 view = glm::lookAt(g_game_camera.position, new_mat_4, g_game_camera.up_vec);
	
	unsigned int model_loc = glGetUniformLocation(g_mesh_shader, "model");
	unsigned int view_loc = glGetUniformLocation(g_mesh_shader, "view");
	unsigned int projection_loc = glGetUniformLocation(g_mesh_shader, "projection");

	glUniformMatrix4fv(model_loc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(view_loc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projection_loc, 1, GL_FALSE, glm::value_ptr(projection));

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, mesh->texture_id);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	g_frame_data.draw_calls++;

	glUseProgram(0);
	glBindVertexArray(0);
}

void append_ui_text(FontData* font_data, char* text, float pos_x_vw, float pos_y_vh)
{
	auto chars = font_data->char_data.data();
	char* text_string = text;
	int length = strlen(text);

	int text_offset_x_px = 0;
	int text_offset_y_px = 0;
	int line_height_px = font_data->font_height_px;

	float* memory_location = (float*)g_ui_text_vertex_buffer.memory;

	// Assume font start position is top left corner

	for (int i = 0; i < length; i++)
	{
		char current_char = *text_string++;

		if (current_char == '\n')
		{
			text_offset_y_px -= line_height_px;
			text_offset_x_px = 0;
			continue;
		}

		int char_index = static_cast<int>(current_char) - 32;
		CharData current = chars[char_index];

		int char_height_px = current.height;
		int char_width_px = current.width;

		int x_start = vw_into_screen_px(pos_x_vw, g_game_metrics.game_width_px) + current.x_offset + text_offset_x_px;
		int char_y_offset = current.y_offset;
		int y_start = vh_into_screen_px(pos_y_vh, g_game_metrics.game_height_px) + text_offset_y_px - line_height_px + char_y_offset;

		float x0 = normalize_screen_px_to_ndc(x_start, g_game_metrics.game_width_px);
		float y0 = normalize_screen_px_to_ndc(y_start, g_game_metrics.game_height_px);

		float x1 = normalize_screen_px_to_ndc(x_start + char_width_px, g_game_metrics.game_width_px);
		float y1 = normalize_screen_px_to_ndc(y_start + char_height_px, g_game_metrics.game_height_px);

		float vertices[] =
		{
			// Coords			// UV
			x0, y0, 0.0f,		current.UV_x0, current.UV_y0, // bottom left
			x1, y0, 0.0f,		current.UV_x1, current.UV_y0, // bottom right
			x0, y1, 0.0f,		current.UV_x0, current.UV_y1, // top left

			x0, y1, 0.0f,		current.UV_x0, current.UV_y1, // top left 
			x1, y1, 0.0f,		current.UV_x1, current.UV_y1, // top right
			x1, y0, 0.0f,		current.UV_x1, current.UV_y0  // bottom right
		};

		memcpy(&memory_location[g_text_indicies], vertices, sizeof(vertices));

		g_text_buffer_size += sizeof(vertices);
		g_text_indicies += 30;

		text_offset_x_px += current.advance;
		text++;
	}
}

void draw_ui_text(FontData* font_data, float red, float green, float blue)
{
	glUseProgram(g_ui_text_shader);
	glBindVertexArray(g_ui_text_vao);

	int color_uniform = glGetUniformLocation(g_ui_text_shader, "textColor");
	glUniform3f(color_uniform, red, green, blue);

	glBindBuffer(GL_ARRAY_BUFFER, g_ui_text_vbo);
	glBufferData(GL_ARRAY_BUFFER, g_text_buffer_size, g_ui_text_vertex_buffer.memory, GL_DYNAMIC_DRAW);

	glBindTexture(GL_TEXTURE_2D, font_data->texture_id);
	glDrawArrays(GL_TRIANGLES, 0, g_text_buffer_size);

	glUseProgram(0);
	glBindVertexArray(0);

	g_text_buffer_size = 0;
	g_text_indicies = 0;
	g_frame_data.draw_calls++;
}

void draw_ui_character(FontData* font_data, const char character, int x, int y)
{
	int char_as_int = static_cast<int>(character);
	int char_index = char_as_int - 32;

	CharData current = font_data->char_data.data()[char_index];

	float x0 = normalize_screen_px_to_ndc(x, g_game_metrics.game_width_px);
	float y0 = normalize_screen_px_to_ndc(y, g_game_metrics.game_height_px);

	float x1 = normalize_screen_px_to_ndc(x + current.width, g_game_metrics.game_width_px);
	float y1 = normalize_screen_px_to_ndc(y + current.height, g_game_metrics.game_height_px);

	float vertices[] =
	{
		// Coords		// UV
		x0, y0, 0.0f,	current.UV_x0, current.UV_y0, // bottom left
		x1, y0, 0.0f,	current.UV_x1, current.UV_y0, // bottom right
		x0, y1, 0.0f,	current.UV_x0, current.UV_y1, // top left

		x0, y1, 0.0f,	current.UV_x0, current.UV_y1, // top left 
		x1, y1, 0.0f,	current.UV_x1, current.UV_y1, // top right
		x1, y0, 0.0f,	current.UV_x1, current.UV_y0  // bottom right
	};

	glUseProgram(g_ui_text_shader);
	glBindVertexArray(g_ui_text_vao);

	glBindBuffer(GL_ARRAY_BUFFER, g_ui_text_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

	glBindTexture(GL_TEXTURE_2D, font_data->texture_id);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	g_frame_data.draw_calls++;

	glUseProgram(0);
	glBindVertexArray(0);
}

void load_font(FontData* font_data, int font_height_px, const char* font_path)
{
	FT_Library ft_lib;
	FT_Face ft_face;

	FT_Error init_ft_err = FT_Init_FreeType(&ft_lib);
	ASSERT_TRUE(init_ft_err == 0, "Init FreeType Library");

	FT_Error new_face_err = FT_New_Face(ft_lib, font_path, 0, &ft_face);
	ASSERT_TRUE(new_face_err == 0, "Load FreeType font face");

	FT_Set_Pixel_Sizes(ft_face, 0, font_height_px);
	font_data->font_height_px = font_height_px;

	unsigned int bitmap_width = 0;
	unsigned int bitmap_height = 0;

	constexpr int starting_char = 33; // ('!')
	constexpr int last_char = 128;

	int spacebar_width = font_height_px / 4;

	// Add spacebar
	bitmap_width += spacebar_width;

	for (int c = starting_char; c < last_char; c++)
	{
		char character = static_cast<char>(c);

		FT_Error load_char_err = FT_Load_Char(ft_face, character, FT_LOAD_RENDER);
		ASSERT_TRUE(load_char_err == 0, "Load FreeType Glyph");

		unsigned int glyph_width = ft_face->glyph->bitmap.width;
		unsigned int glyph_height = ft_face->glyph->bitmap.rows;

		if (bitmap_height < glyph_height)
		{
			bitmap_height = glyph_height;
		}

		bitmap_width += glyph_width;
	}

	int bitmap_size = bitmap_width * bitmap_height;
	ASSERT_TRUE(bitmap_size < g_temp_memory.size, "Font bitmap buffer fits");

	byte* bitmap_memory = g_temp_memory.memory;

	// Add spacebar
	{
		for (int y = 0; y < bitmap_height; y++)
		{
			int src_index = ((bitmap_height - 1) * spacebar_width) - y * spacebar_width;
			int dest_index = y * bitmap_width;
			memset(&bitmap_memory[dest_index], 0x00, spacebar_width);
		}

		CharData new_char_data = { 0 };
		new_char_data.character = static_cast<char>(' ');
		new_char_data.width = spacebar_width;
		new_char_data.advance = spacebar_width;
		new_char_data.height = font_height_px;
		new_char_data.UV_x0 = 0.0f;
		new_char_data.UV_y0 = 1.0f;
		new_char_data.UV_x1 = normalize_value(spacebar_width, bitmap_width, 1.0f);
		new_char_data.UV_y1 = 1.0f;
		font_data->char_data[0] = new_char_data;
	}

	int bitmap_x_offset = spacebar_width;
	int char_data_index = 1;

	for (int c = starting_char; c < last_char; c++)
	{
		int character = c;

		FT_Error load_char_err = FT_Load_Char(ft_face, character, FT_LOAD_RENDER);
		ASSERT_TRUE(load_char_err == 0, "Load FreeType Glyph");

		unsigned int glyph_width = ft_face->glyph->bitmap.width;
		unsigned int glyph_height = ft_face->glyph->bitmap.rows;

		int x_offset = ft_face->glyph->bitmap_left;
		int y_offset = ft_face->glyph->bitmap_top - ft_face->glyph->bitmap.rows;

		for (int y = 0; y < glyph_height; y++)
		{
			int src_index = ((glyph_height - 1) * glyph_width) - y * glyph_width;
			int dest_index = y * bitmap_width + bitmap_x_offset;
			memcpy(&bitmap_memory[dest_index], &ft_face->glyph->bitmap.buffer[src_index], glyph_width);
		}

		// To advance cursors for next glyph
		// bitshift by 6 to get value in pixels
		// (2^6 = 64 (divide amount of 1/64th pixels by 64 to get amount of pixels))
		// (note that advance is number of 1/64 pixels)
		int advance = (ft_face->glyph->advance.x >> 6);

		CharData new_char_data = { 0 };
		new_char_data.character = static_cast<char>(c);

		new_char_data.width = glyph_width;
		new_char_data.height = glyph_height;
		new_char_data.x_offset = x_offset;
		new_char_data.y_offset = y_offset;
		new_char_data.advance = advance;

		float atlas_width = static_cast<float>(bitmap_width);
		float atlas_height = static_cast<float>(bitmap_height);

		new_char_data.UV_x0 = normalize_value(bitmap_x_offset, atlas_width, 1.0f);
		new_char_data.UV_y0 = normalize_value(0.0f, atlas_height, 1.0f);

		float x1 = bitmap_x_offset + glyph_width;
		new_char_data.UV_x1 = normalize_value(x1, atlas_width, 1.0f);
		new_char_data.UV_y1 = normalize_value(glyph_height, atlas_height, 1.0f);

		font_data->char_data[char_data_index++] = new_char_data;

		bitmap_x_offset += glyph_width;
	}

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	GLuint prev_texture = font_data->texture_id;
	glDeleteTextures(1, &prev_texture);

	GLuint new_texture;
	glGenTextures(1, &new_texture);

	font_data->texture_id = static_cast<int>(new_texture);
	glBindTexture(GL_TEXTURE_2D, font_data->texture_id);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, bitmap_width, bitmap_height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, bitmap_memory);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	float screen_x = (float)width;
	float screen_y = (float)height;

	float x_aspect = screen_x / (float)g_game_metrics.game_aspect_ratio_x;
	float y_aspect = screen_y / (float)g_game_metrics.game_aspect_ratio_y;

	bool is_wider_for_aspect = y_aspect < x_aspect;

	if (is_wider_for_aspect)
	{
		int new_width = (screen_y / (float)g_game_metrics.game_aspect_ratio_y) * g_game_metrics.game_aspect_ratio_x;
		g_game_metrics.game_width_px = new_width;
		g_game_metrics.game_height_px = height;

		int x_start = (screen_x / 2) - ((float)g_game_metrics.game_width_px / 2);
		glViewport(x_start, 0, new_width, height);
	}
	else
	{
		int new_height = (screen_x / (float)g_game_metrics.game_aspect_ratio_x) * g_game_metrics.game_aspect_ratio_y;
		g_game_metrics.game_width_px = width;
		g_game_metrics.game_height_px = new_height;

		int y_start = (screen_y / 2) - ((float)g_game_metrics.game_height_px / 2);
		glViewport(0, y_start, width, new_height);
	}

	int font_height_px = normalize_value(debug_font_vh, 100.0f, g_game_metrics.game_height_px);
	load_font(&g_debug_font, font_height_px, g_debug_font_path);
}

void mouse_move_callback(GLFWwindow* window, double xposIn, double yposIn)
{
	float xpos = static_cast<float>(xposIn);
	float ypos = static_cast<float>(yposIn);

	g_mouse_movement_x = xpos - g_frame_data.prev_mouse_x;
	g_mouse_movement_y = g_frame_data.prev_mouse_y - ypos;

	g_frame_data.prev_mouse_x = xpos;
	g_frame_data.prev_mouse_y = ypos;
}

int main(int argc, char* argv[])
{
	memory_buffer_mallocate(&g_temp_memory, MEGABYTES(5), const_cast<char*>("Temp memory"));

	g_game_metrics.game_aspect_ratio_x = 4;
	g_game_metrics.game_aspect_ratio_y = 3;
	g_game_metrics.game_width_px = 1600;
	g_game_metrics.game_height_px = 1200;

	// Init window and context
	GLFWwindow* window;
	{
		int glfw_init_result = glfwInit();
		ASSERT_TRUE(glfw_init_result == GLFW_TRUE, "glfw init");

		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		window = glfwCreateWindow(g_game_metrics.game_width_px, g_game_metrics.game_height_px, "My Window", NULL, NULL);
		ASSERT_TRUE(window, "window creation");

		glfwMakeContextCurrent(window);
		glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
		glfwSetCursorPosCallback(window, mouse_move_callback);

		int glew_init_result = glewInit();
		ASSERT_TRUE(glew_init_result == GLEW_OK, "glew init");
	}

	// Init ImGui
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

		ImGui_ImplGlfw_InitForOpenGL(window, true);
		ImGui_ImplOpenGL3_Init("#version 330 core");
	}

	// Init game inputs
	{
		g_game_inputs.mouse1 = ButtonState{ .key = GLFW_MOUSE_BUTTON_1 };
		g_game_inputs.mouse2 = ButtonState{ .key = GLFW_MOUSE_BUTTON_2 };

		g_game_inputs.q = ButtonState{ .key = GLFW_KEY_Q };
		g_game_inputs.w = ButtonState{ .key = GLFW_KEY_W };
		g_game_inputs.e = ButtonState{ .key = GLFW_KEY_E };
		g_game_inputs.r = ButtonState{ .key = GLFW_KEY_R };
		g_game_inputs.a = ButtonState{ .key = GLFW_KEY_A };
		g_game_inputs.s = ButtonState{ .key = GLFW_KEY_S };
		g_game_inputs.d = ButtonState{ .key = GLFW_KEY_D };
		g_game_inputs.f = ButtonState{ .key = GLFW_KEY_F };
		g_game_inputs.z = ButtonState{ .key = GLFW_KEY_Z };
		g_game_inputs.x = ButtonState{ .key = GLFW_KEY_X };
		g_game_inputs.c = ButtonState{ .key = GLFW_KEY_C };
		g_game_inputs.v = ButtonState{ .key = GLFW_KEY_V };
		g_game_inputs.esc = ButtonState{ .key = GLFW_KEY_ESCAPE };
		g_game_inputs.plus = ButtonState{ .key = GLFW_KEY_KP_ADD };
		g_game_inputs.minus = ButtonState{ .key = GLFW_KEY_KP_SUBTRACT };
	}

	// Init simple rectangle shader
	{
		const char* vertex_shader_path = "G:/projects/game/Engine3D/resources/shaders/simple_reactangle_vs.glsl";
		const char* fragment_shader_path = "G:/projects/game/Engine3D/resources/shaders/simple_reactangle_fs.glsl";

		g_simple_rectangle_shader = compile_shader(vertex_shader_path, fragment_shader_path, &g_temp_memory);
		{
			glGenVertexArrays(1, &g_simple_rectangle_vao);
			glGenBuffers(1, &g_simple_rectangle_vbo);

			glBindVertexArray(g_simple_rectangle_vao);
			glBindBuffer(GL_ARRAY_BUFFER, g_simple_rectangle_vbo);

			// Coord attribute
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
			glEnableVertexAttribArray(0);

			// Color attribute
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
			glEnableVertexAttribArray(1);
		}
	}

	// Init UI text shader
	{
		const char* vertex_shader_path = "G:/projects/game/Engine3D/resources/shaders/ui_text_vs.glsl";
		const char* fragment_shader_path = "G:/projects/game/Engine3D/resources/shaders/ui_text_fs.glsl";

		g_ui_text_shader = compile_shader(vertex_shader_path, fragment_shader_path, &g_temp_memory);
		{
			glGenVertexArrays(1, &g_ui_text_vao);
			glGenBuffers(1, &g_ui_text_vbo);

			glBindVertexArray(g_ui_text_vao);
			glBindBuffer(GL_ARRAY_BUFFER, g_ui_text_vbo);

			// Coord attribute
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
			glEnableVertexAttribArray(0);

			// UV attribute
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
			glEnableVertexAttribArray(1);

			// Reserve vertex array
			int vertex_data_for_char = 120;
			int text_buffer_size = vertex_data_for_char * g_max_UI_chars;
			memory_buffer_mallocate(&g_ui_text_vertex_buffer, text_buffer_size, const_cast<char*>("UI text verticies"));
		}
	}

	// Init mesh shader
	{
		const char* vertex_shader_path = "G:/projects/game/Engine3D/resources/shaders/mesh_vs.glsl";
		const char* fragment_shader_path = "G:/projects/game/Engine3D/resources/shaders/mesh_fs.glsl";

		g_mesh_shader = compile_shader(vertex_shader_path, fragment_shader_path, &g_temp_memory);
		{
			glGenVertexArrays(1, &g_mesh_vao);
			glGenBuffers(1, &g_mesh_vbo);

			glBindVertexArray(g_mesh_vao);
			glBindBuffer(GL_ARRAY_BUFFER, g_mesh_vbo);

			float vertices[] =
			{
				// Coords				// UV
				-0.5f, -0.5f, 0.0f,		0.0f, 0.0f, // bottom left
				 0.5f, -0.5f, 0.0f,		1.0f, 0.0f, // bottom right
				-0.5f,  0.5f, 0.0f,		0.0f, 1.0f, // top left

				-0.5f,  0.5f, 0.0f,		0.0f, 1.0f, // top left 
				 0.5f,  0.5f, 0.0f,		1.0f, 1.0f, // top right
				 0.5f, -0.5f, 0.0f,		1.0f, 0.0f  // bottom right
			};

			glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

			// Coord attribute
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
			glEnableVertexAttribArray(0);

			// UV attribute
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
			glEnableVertexAttribArray(1);
		}
	}

	const char* image_path = "G:/projects/game/Engine3D/resources/images/debug_img_01.png";
	int debug_texture = load_image_into_texture(image_path, true);

	int font_height_px = normalize_value(debug_font_vh, 100.0f, g_game_metrics.game_height_px);
	load_font(&g_debug_font, font_height_px, g_debug_font_path);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glClearColor(1.0f, 0.0f, 1.0f, 1.0f);

	g_game_camera.position = glm::vec3(0.0f, 0.0f, 3.0f);
	g_game_camera.front_vec = glm::vec3(0.0f, 0.0f, -1.0f);
	g_game_camera.up_vec = glm::vec3(0.0f, 1.0f, 0.0f);

	Plane my_plane01 = {
		.translation =	glm::vec3(0.0f, 0.0f, 0.0f),
		.rotation =		glm::vec3(0.0f, 0.0f, 0.0f),
		.scale =		glm::vec3(1.0f, 1.0f, 1.0f),
		.texture_id =	debug_texture
	};

	float* test1 = &my_plane01.translation[0];
	float* test2 = &my_plane01.rotation[0];
	float* test3 = &my_plane01.scale[0];

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();

		// ImGui logic
		{
			ImGui_ImplGlfw_NewFrame();
			ImGui_ImplOpenGL3_NewFrame();
			ImGui::NewFrame();

			{
				ImGui::Begin("Properties", nullptr, 0);

				ImGui::Text("Mesh properties");
				ImGui::InputFloat3("Translation", &my_plane01.translation[0], "%.2f");
				ImGui::InputFloat3("Rotation", &my_plane01.rotation[0], "%.2f");
				ImGui::InputFloat3("Scale", &my_plane01.scale[0], "%.2f");

				ImGui::Text("Game window");
				ImGui::InputInt2("Screen width px", &g_user_settings.screen_size_px[0]);

				if (ImGui::Button("Apply changes"))
				{
					glfwSetWindowSize(window, g_user_settings.screen_size_px[0], g_user_settings.screen_size_px[1]);
				}

				ImGui::End();
			}
		}

		// Register inputs
		{
			g_frame_data.mouse_clicked = false;

			if (glfwGetKey(window, g_game_inputs.esc.key) == GLFW_PRESS)
			{
				glfwSetWindowShouldClose(window, true);
			}

			int key_state;

			key_state = glfwGetMouseButton(window, g_game_inputs.mouse1.key);
			g_game_inputs.mouse1.pressed = !g_game_inputs.mouse1.is_down && key_state == GLFW_PRESS;
			g_game_inputs.mouse1.is_down = key_state == GLFW_PRESS;

			key_state = glfwGetMouseButton(window, g_game_inputs.mouse2.key);
			g_game_inputs.mouse2.pressed = !g_game_inputs.mouse2.is_down && key_state == GLFW_PRESS;
			g_game_inputs.mouse2.is_down = key_state == GLFW_PRESS;

			key_state = glfwGetKey(window, g_game_inputs.esc.key);
			g_game_inputs.esc.pressed = !g_game_inputs.esc.is_down && key_state == GLFW_PRESS;
			g_game_inputs.esc.is_down = key_state == GLFW_PRESS;
		}

		// Logic

		g_game_metrics.prev_frame_game_time = g_game_metrics.game_time;
		g_game_metrics.game_time = glfwGetTime();
		g_frame_data.deltatime = (g_game_metrics.game_time - g_game_metrics.prev_frame_game_time);

		int current_game_second = (int)g_game_metrics.game_time;

		if (current_game_second - g_game_metrics.fps_prev_second > 0)
		{
			g_game_metrics.fps = g_game_metrics.fps_frames;
			g_game_metrics.fps_frames = 0;
			g_game_metrics.fps_prev_second = current_game_second;
		}

		if (g_game_inputs.mouse1.pressed)
		{
			double xpos, ypos;
			glfwGetCursorPos(window, &xpos, &ypos);

			g_frame_data.mouse_clicked = true;
			g_frame_data.mouse_click_x = xpos;
			g_frame_data.mouse_click_y = g_game_metrics.game_height_px - ypos;

			std::cout 
				<< "Mouse click - x:" << g_frame_data.mouse_click_x 
				<< " y:" << g_frame_data.mouse_click_y 
				<< std::endl;
		}

		if (g_game_inputs.mouse2.is_down)
		{
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

			g_mouse_movement_x *= g_game_camera.look_sensitivity;
			g_mouse_movement_y *= g_game_camera.look_sensitivity;

			g_game_camera.yaw += g_mouse_movement_x;
			g_game_camera.pitch += g_mouse_movement_y;

			if (g_game_camera.pitch > 89.0f)
			{
				g_game_camera.pitch = 89.0f;
			}
			else if (g_game_camera.pitch < -89.0f)
			{
				g_game_camera.pitch = -89.0f;
			}

			glm::vec3 new_camera_front;
			new_camera_front.x = cos(glm::radians(g_game_camera.yaw)) * cos(glm::radians(g_game_camera.pitch));
			new_camera_front.y = sin(glm::radians(g_game_camera.pitch));
			new_camera_front.z = sin(glm::radians(g_game_camera.yaw)) * cos(glm::radians(g_game_camera.pitch));

			g_game_camera.front_vec = glm::normalize(new_camera_front);
		}
		else
		{
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}

		float camera_speed = g_game_camera.move_speed * g_frame_data.deltatime;

		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		{
			g_game_camera.position += camera_speed * g_game_camera.front_vec;
		}
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		{
			g_game_camera.position -= camera_speed * g_game_camera.front_vec;
		}
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		{
			g_game_camera.position -= glm::normalize(glm::cross(g_game_camera.front_vec, g_game_camera.up_vec)) * camera_speed;
		}
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		{
			g_game_camera.position += glm::normalize(glm::cross(g_game_camera.front_vec, g_game_camera.up_vec)) * camera_speed;
		}

		// Draw
		
		glClear(GL_COLOR_BUFFER_BIT);

		Rectangle2D rect1 = { 0 };
		rect1.bot_left_x = 0;
		rect1.bot_left_y = 0;
		rect1.height = g_game_metrics.game_height_px;
		rect1.width = g_game_metrics.game_width_px;

		draw_simple_reactangle(rect1, 0.9f, 0.9f, 0.9f);

		draw_mesh(&my_plane01);

		// Print debug info
		{
			char debug_str[512];

			const char* fps_debug_str_format = "FPS: %d";
			sprintf_s(debug_str, fps_debug_str_format, g_game_metrics.fps);
			append_ui_text(&g_debug_font, debug_str, 0.5f, 100.0f);

			const char* delta_debug_str_format = "Delta: %.2fms";
			float display_deltatime = g_frame_data.deltatime * 1000;
			sprintf_s(debug_str, delta_debug_str_format, display_deltatime);
			append_ui_text(&g_debug_font, debug_str, 4.5f, 100.0f);

			const char* frames_debug_str_format = "Frames: %lu";
			sprintf_s(debug_str, frames_debug_str_format, g_game_metrics.frames);
			append_ui_text(&g_debug_font, debug_str, 10.5f, 100.0f);

			const char* draw_calls_debug_str_format = "Draw calls: %d";
			sprintf_s(debug_str, draw_calls_debug_str_format, ++g_frame_data.draw_calls);
			append_ui_text(&g_debug_font, debug_str, 17.0f, 100.0f);

			const char* camera_pos_debug_str_format = "Camera X=%.2f Y=%.2f Z=%.2f";
			sprintf_s(debug_str, camera_pos_debug_str_format, g_game_camera.position.x, g_game_camera.position.y, g_game_camera.position.z);
			append_ui_text(&g_debug_font, debug_str, 0.5f, 99.0f);

			draw_ui_text(&g_debug_font, 0.1f, 0.1f, 0.1f);
		}

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
		g_game_metrics.frames++;
		g_game_metrics.fps_frames++;
		g_frame_data.draw_calls = 0;
	}

	glfwTerminate(); // @Performance: Unnecessary slowdown

	return 0;
}
