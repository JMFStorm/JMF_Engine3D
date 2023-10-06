#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#include <array>
#include <iostream>
#include <fstream>
#include <thread>
#include <filesystem>
#include <mutex>

#define KILOBYTES(x) (x * 1024)

typedef unsigned char byte;


std::mutex global_log_mutex;
std::ofstream global_log_file_output;
std::filesystem::path global_log_file_path = {};
constexpr const char* GLOBAL_LOG_FILE_NAME = "Engine3D_mylog.txt";

constexpr size_t GLOBAL_STRING_POOL_SIZE = 2048;
std::array<char, GLOBAL_STRING_POOL_SIZE> g_string_pool = {0}; 


int g_simple_rectangle_shader;
unsigned int g_simple_rectangle_vao;
unsigned int g_simple_rectangle_vbo;

int g_ui_text_shader;
unsigned int g_ui_text_vao;

int g_game_width_px = 1800;
int g_game_height_px = 900;

typedef struct {
	int bot_left_x;
	int bot_left_y;
	int width;
	int height;
} Rectangle2D;


void assert_true(bool assertion, const char* assertion_title, const char* file, const char* func, int line)
{
	if (!assertion)
	{
		std::cerr << "ERROR: Assertion (" << assertion_title << ") failed in: " << file << " at function: " << func << "(), line: " << line << "." << std::endl;
		exit(1);
	}
}

#define ASSERT_TRUE(assertion, assertion_title) assert_true(assertion, assertion_title, __FILE__, __func__, __LINE__)


bool read_file_to_global_string_pool(const char* file_path, char** str_pointer)
{
	std::ifstream file_stream(file_path);

	if (!file_stream.is_open())
	{
		std::cerr << "Failed to open file: " << file_path << std::endl;
		return false;
	}

	g_string_pool = { 0 }; // Reset global string pool to 0

	file_stream.read(g_string_pool.data(), GLOBAL_STRING_POOL_SIZE);
	file_stream.close();

	*str_pointer = g_string_pool.data();

	return true;
}

bool game_log_init(char* executable_filepath)
{
	std::filesystem::path executable_dir = std::filesystem::path(executable_filepath).parent_path();
	global_log_file_path = executable_dir / GLOBAL_LOG_FILE_NAME;

	global_log_file_output.open(global_log_file_path, std::ios::trunc);

	if (!global_log_file_output.is_open())
	{
		std::cerr << "Error: Could not open the log file (" << global_log_file_path << ")." << std::endl;
		return false;
	}

	return true;
}

void game_log(const char* message)
{
	if (global_log_file_output.is_open())
	{
		global_log_file_output << message << "\n";
	}
}

void game_log_flush_and_end()
{
	global_log_file_output.flush();
	global_log_file_output.close();
}

int compile_shader(const char* vertex_shader_path, const char* fragment_shader_path)
{
	int shader_result = -1;
	char* vertex_shader_code = nullptr;
	char* fragment_shader_code = nullptr;

	bool read_vertex_success = read_file_to_global_string_pool(vertex_shader_path, &vertex_shader_code);
	ASSERT_TRUE(read_vertex_success, "Read vertex shader");

	int vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex_shader, 1, &vertex_shader_code, NULL);
	glCompileShader(vertex_shader);

	bool read_fragment_success = read_file_to_global_string_pool(fragment_shader_path, &fragment_shader_code);
	ASSERT_TRUE(read_fragment_success, "Read fragment shader");

	int fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment_shader, 1, &fragment_shader_code, NULL);
	glCompileShader(fragment_shader);

	shader_result = glCreateProgram();
	glAttachShader(shader_result, vertex_shader);
	glAttachShader(shader_result, fragment_shader);
	glLinkProgram(shader_result);

	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);

	return shader_result;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	g_game_width_px = width;
	g_game_height_px = height;
	glViewport(0, 0, width, height);
}

int load_image_into_texture(const char* image_path)
{
	unsigned int texture;
	int x, y, n;

	stbi_set_flip_vertically_on_load(1);
	byte* data = stbi_load(image_path, &x, &y, &n, 0);
	ASSERT_TRUE(data != NULL, "Load texture");

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	ASSERT_TRUE(n == 3 || n == 4, "Image format is RGB or RGBA");

	GLint use_format = n == 3
		? GL_RGB
		: GL_RGBA;

	glTexImage2D(GL_TEXTURE_2D, 0, use_format, x, y, 0, use_format, GL_UNSIGNED_BYTE, data);

	stbi_image_free(data);

	return texture;
}

float normalize_screen_px_to_ndc(int value, int max)
{
	float this1 = static_cast<float>(value) / static_cast<float>(max);

	float this2 = 2.0f * this1;

	float res = -1.0f + this2;
	return res;
}

void draw_simple_reactangle(Rectangle2D rect, float r, float g, float b)
{
	glUseProgram(g_simple_rectangle_shader);
	glBindVertexArray(g_simple_rectangle_vao);

	float x0 = normalize_screen_px_to_ndc(rect.bot_left_x, g_game_width_px);
	float y0 = normalize_screen_px_to_ndc(rect.bot_left_y, g_game_height_px);

	float x1 = normalize_screen_px_to_ndc(rect.bot_left_x + rect.width, g_game_width_px);
	float y1 = normalize_screen_px_to_ndc(rect.bot_left_y + rect.height, g_game_height_px);

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

	glBindVertexArray(g_simple_rectangle_vao);

	glBindBuffer(GL_ARRAY_BUFFER, g_simple_rectangle_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

	glDrawArrays(GL_TRIANGLES, 0, 6);

	glUseProgram(0);
	glBindVertexArray(0);
}

int main(int argc, char* argv[])
{
	char* program_filepath = argv[0];

	bool init_logger_success = game_log_init(program_filepath);
	ASSERT_TRUE(init_logger_success, "Game logger init");

	game_log("Game started.");


	int glfw_init_result = glfwInit();
	ASSERT_TRUE(glfw_init_result == GLFW_TRUE, "glfw init");

	GLFWwindow* window = glfwCreateWindow(g_game_width_px, g_game_height_px, "My Window", NULL, NULL);
	ASSERT_TRUE(window, "window creation");

	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);


	int glew_init_result = glewInit();
	ASSERT_TRUE(glew_init_result == GLEW_OK, "glew init");


	// Load debug font
	GLuint font_texture;
	{
		constexpr int ttf_buffer_size = KILOBYTES(1024);
		unsigned char* ttf_buffer = (unsigned char*)malloc(ttf_buffer_size);

		FILE* file;
		fopen_s(&file, "G:/projects/game/Engine3D/resources/fonts/Roboto-Medium.ttf", "rb");
		fread(ttf_buffer, 1, ttf_buffer_size, file);

		stbtt_fontinfo font;
		int font_offset = stbtt_GetFontOffsetForIndex(ttf_buffer, 0);
		stbtt_InitFont(&font, ttf_buffer, font_offset);

		float font_height_px = 128;
		float font_scale = stbtt_ScaleForPixelHeight(&font, font_height_px);

		// Iterate fontinfo and get max height / widths for bitmap atlas

		constexpr int starting_char = 98;
		constexpr int last_char = 117;

		int bitmap_width = 0;
		int bitmap_height = 0;

		for (int c = starting_char; c < last_char; c++)
		{
			int character = c;
			int glyph_index = stbtt_FindGlyphIndex(&font, character);

			int x0, y0, x1, y1;
			stbtt_GetGlyphBitmapBox(&font, glyph_index, font_scale, font_scale, &x0, &y0, &x1, &y1);

			int glyph_width = x1 - x0;
			int glyph_height = y1 - y0;

			if (bitmap_height < glyph_height)
			{
				bitmap_height = glyph_height;
			}

			bitmap_width += glyph_width;
		}

		bitmap_height = static_cast<float>(bitmap_height) * 2.0f; // increase bitmap height to have room for y offset adjust

		int bitmap_size = bitmap_width * bitmap_height;
		byte* texture_bitmap = static_cast<byte*>(malloc(bitmap_size));
		memset(texture_bitmap, 0x00, bitmap_size);

		// Create bitmap buffer

		int bitmap_x_offset = 0;

		for (int c = starting_char; c < last_char; c++)
		{
			int character = c;
			int glyph_index = stbtt_FindGlyphIndex(&font, character);

			int font_width, font_height, x_offset, y_offset;
			byte* bitmap = stbtt_GetGlyphBitmap(&font, 0, font_scale, glyph_index, &font_width, &font_height, &x_offset, &y_offset);

			std::cout << (char)character
				<< " width: " << font_width
				<< ", height: " << font_height
				<< ", x offset: " << x_offset
				<< ", y offset: " << y_offset
				<< std::endl;

			int used_y_offset = font_height + y_offset;

			for (int y = 0; y < font_height; y++)
			{
				int src_index = ((font_height - 1) * font_width) - y * font_width;
				int half_of_bitmap_height = ((static_cast<float>(bitmap_height) / 2) * bitmap_width);
				int dest_index = y * bitmap_width + bitmap_x_offset + half_of_bitmap_height;
				memcpy(&texture_bitmap[dest_index], &bitmap[src_index], font_width);
			}

			bitmap_x_offset += font_width;
		}

		memset(&texture_bitmap[0], 0x44, bitmap_width);
		memset(&texture_bitmap[(bitmap_height - 1) * bitmap_width], 0x99, bitmap_width);

		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

		glGenTextures(1, &font_texture);
		glBindTexture(GL_TEXTURE_2D, font_texture);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, bitmap_width, bitmap_height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, texture_bitmap);

		glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	}

	// Init simple reactangle shader
	{
		const char* vertex_shader_path = "G:/projects/game/Engine3D/resources/shaders/simple_reactangle_vs.glsl";
		const char* fragment_shader_path = "G:/projects/game/Engine3D/resources/shaders/simple_reactangle_fs.glsl";

		g_simple_rectangle_shader = compile_shader(vertex_shader_path, fragment_shader_path);
		{
			glGenVertexArrays(1, &g_simple_rectangle_vao);
			glGenBuffers(1, &g_simple_rectangle_vbo);
			glBindVertexArray(g_simple_rectangle_vao);

			int buffer_size = (6 * 6) * sizeof(float);
			glBindBuffer(GL_ARRAY_BUFFER, g_simple_rectangle_vbo);
			glBufferData(GL_ARRAY_BUFFER, buffer_size, NULL, GL_DYNAMIC_DRAW);

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

		g_ui_text_shader = compile_shader(vertex_shader_path, fragment_shader_path);

		{
			float vertices[] =
			{
				// Coords				// UV
				-0.9f, -0.0f, 0.0f,		0.0f, 0.0f, // bottom left
				 0.9f, -0.0f, 0.0f,		1.0f, 0.0f, // bottom right
				-0.9f,  0.7f, 0.0f,		0.0f, 1.0f, // top left

				-0.9f,  0.7f, 0.0f,		0.0f, 1.0f, // top left 
				 0.9f,  0.7f, 0.0f,		1.0f, 1.0f, // top right
				 0.9f, -0.0f, 0.0f,		1.0f, 0.0f  // bottom right
			};

			unsigned int VBO;
			glGenVertexArrays(1, &g_ui_text_vao);
			glGenBuffers(1, &VBO);
			glBindVertexArray(g_ui_text_vao);

			glBindBuffer(GL_ARRAY_BUFFER, VBO);
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
	unsigned int debug_texture = load_image_into_texture(image_path);


	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glClearColor(0.4f, 0.8f, 0.6f, 1.0f);

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();

		glClear(GL_COLOR_BUFFER_BIT);

		glUseProgram(g_ui_text_shader);
		glBindVertexArray(g_ui_text_vao);
		glBindTexture(GL_TEXTURE_2D, font_texture);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		glfwSwapBuffers(window);
	}


	game_log("Game ended.");

	game_log_flush_and_end(); // Make sure files are written to disk

	glfwTerminate(); // @Perf: Unnecessary slowdown

	return 0;
}
