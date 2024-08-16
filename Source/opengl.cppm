module;

#include <print>
#include <array>
#include <vector>
#include <memory>

#define GLFW_INCLUDE_NONE
#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

export module opengl;

import common;
import filesystem;

// Basic vector2 struct with helper methods
struct Vector2 {
	float x = 0.0f, y = 0.0f;

	Vector2() = default;
	Vector2(const float x, const float y) : x(x), y(y) {}

	auto operator+(const Vector2 &other) const -> Vector2 {
		return Vector2(x + other.x, y + other.y);
	}
	auto operator-(const Vector2 &other) const -> Vector2 {
		return Vector2(x - other.x, y - other.y);
	}
	auto operator*(const float scalar) const -> Vector2 { return Vector2(x * scalar, y * scalar); }
	auto operator/(const float scalar) const -> Vector2 { return Vector2(x / scalar, y / scalar); }

	auto operator+=(const Vector2 &other) -> Vector2 & {
		x += other.x;
		y += other.y;
		return *this;
	}
	auto operator-=(const Vector2 &other) -> Vector2 & {
		x -= other.x;
		y -= other.y;
		return *this;
	}
	auto operator*=(const float scalar) -> Vector2 & {
		x *= scalar;
		y *= scalar;
		return *this;
	}
	auto operator/=(const float scalar) -> Vector2 & {
		x /= scalar;
		y /= scalar;
		return *this;
	}

	auto length() const -> float { return std::sqrt(x * x + y * y); }
	auto normalized() const -> Vector2 {
		const auto len = length();
		return len > 0.0f ? *this / len : *this;
	}
};

// Shader helper struct
struct Shader {
	GLuint id = 0;

	Shader(const std::string &vertex_source, const std::string &fragment_source) {
		const auto vertex_shader = compile_shader(GL_VERTEX_SHADER, vertex_source.c_str());
		const auto fragment_shader = compile_shader(GL_FRAGMENT_SHADER, fragment_source.c_str());

		id = glCreateProgram();
		glAttachShader(id, vertex_shader);
		glAttachShader(id, fragment_shader);
		glLinkProgram(id);

		// Check for linking errors
		int success;
		glGetProgramiv(id, GL_LINK_STATUS, &success);
		if (!success) {
			char info_log[512];
			glGetProgramInfoLog(id, sizeof(info_log), nullptr, info_log);
			std::println("Shader linking failed: {}", info_log);
		}

		// Delete shaders as they're linked into the program
		glDeleteShader(vertex_shader);
		glDeleteShader(fragment_shader);
	}

	~Shader() { glDeleteProgram(id); }

	void use() const { glUseProgram(id); }
	void unuse() const { glUseProgram(0); }

	auto compile_shader(const GLenum type, const char *source) -> GLuint {
		const auto shader = glCreateShader(type);
		glShaderSource(shader, 1, &source, nullptr);
		glCompileShader(shader);

		// Check for compilation errors
		int success;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
		if (!success) {
			char info_log[512];
			glGetShaderInfoLog(shader, sizeof(info_log), nullptr, info_log);
			std::println("Shader compilation failed: {}", info_log);
		}

		return shader;
	}

	auto get_uniform_location(const std::string &name) const -> GLint {
		return glGetUniformLocation(id, name.c_str());
	}

	void set_bool(const std::string &name, const bool value) const {
		glUniform1i(get_uniform_location(name), static_cast<int>(value));
	}
	void set_int(const std::string &name, const int value) const {
		glUniform1i(get_uniform_location(name), value);
	}
	void set_float(const std::string &name, const float value) const {
		glUniform1f(get_uniform_location(name), value);
	}
	void set_vec2(const std::string &name, const Vector2 &value) const {
		glUniform2f(get_uniform_location(name), value.x, value.y);
	}
};

// Struct holding some basic variables usable in shaders (time, resolution, etc.)
struct ShaderVariables {
	float time = 0.0f;
	Vector2 resolution = Vector2(0.0f, 0.0f);
	float intensity = 1.0f;
	float speed = 1.0f;
	float frequency = 1.0f;
	float phase = 0.0f;
};

// Struct for holding vertex data
struct Vertex {
	Vector2 position;
	Vector2 tex_coords;
};

// Fullscreen quad helper struct
struct FullscreenQuad {
	GLuint vao = 0;
	GLuint vbo = 0;
	Shader *shader = nullptr;

	FullscreenQuad() {
		const std::array vertices = {
			Vertex{{-1.0f, -1.0f}, {0.0f, 0.0f}},
			Vertex{{1.0f, -1.0f}, {1.0f, 0.0f}},
			Vertex{{1.0f, 1.0f}, {1.0f, 1.0f}},
			Vertex{{-1.0f, 1.0f}, {0.0f, 1.0f}},
		};

		glGenVertexArrays(1, &vao);
		glGenBuffers(1, &vbo);
		glBindVertexArray(vao);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices.data(), GL_STATIC_DRAW);

		// Position attribute
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);

		// Texture coordinate attribute
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
							  reinterpret_cast<void *>(sizeof(Vector2)));

		glBindVertexArray(0);
	}

	~FullscreenQuad() {
		glDeleteBuffers(1, &vbo);
		glDeleteVertexArrays(1, &vao);
	}

	void set_shader(Shader *new_shader) { shader = new_shader; }

	void render(const ShaderVariables &variables = {}) const {
		if (shader) {
			shader->use();
			shader->set_float("time", variables.time);
			shader->set_vec2("resolution", variables.resolution);
			shader->set_float("intensity", variables.intensity);
			shader->set_float("speed", variables.speed);
			shader->set_float("frequency", variables.frequency);
			shader->set_float("phase", variables.phase);
		}

		glBindVertexArray(vao);
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
		glBindVertexArray(0);
		if (shader) shader->unuse();
	}
};

// Class for OpenGL and GLFW handling
export class OpenGLHandler {
	static inline GLFWwindow *m_mainWindow = nullptr;
	static inline GLFWmonitor *m_monitor = nullptr;
	static inline const GLFWvidmode *m_mode;

	static inline std::vector<std::unique_ptr<Shader>> m_shaders;
	static inline std::vector<FullscreenQuad> m_quads;

public:
	static auto initialize() -> Result {
		// GLFW INITIALIZATION //
		glfwSetErrorCallback(glfw_error_callback);
		if (!glfwInit()) return Result(1, "Failed to initialize GLFW!");

		// WINDOW CREATION //
		m_monitor = glfwGetPrimaryMonitor();
		m_mode = glfwGetVideoMode(m_monitor);

		// GLFW window hints
		glfwDefaultWindowHints();
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
		glfwWindowHint(GLFW_FOCUSED, GLFW_FALSE);
		glfwWindowHint(GLFW_FOCUS_ON_SHOW, GLFW_FALSE);
		glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
		//glfwWindowHint(GLFW_AUTO_ICONIFY, GLFW_FALSE);
		//glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);
		//glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
		//glfwWindowHint(GLFW_MOUSE_PASSTHROUGH, GLFW_TRUE);

		int xpos = 0, ypos = 0;
		glfwGetMonitorPos(m_monitor, &xpos, &ypos);
		glfwWindowHint(GLFW_POSITION_X, xpos);
		glfwWindowHint(GLFW_POSITION_Y, ypos);

		// Main window
		m_mainWindow =
			glfwCreateWindow(10, 10, "ChatNotifier", nullptr, nullptr);
		if (!m_mainWindow) return Result(2, "Failed to create ChatNotifier window");

		glfwMakeContextCurrent(m_mainWindow);
		glfwSwapInterval(1); // V-Sync

		// GL3W INITIALIZATION //
		if (gl3wInit()) return Result(3, "Failed to initialize GL3W");
		if (!gl3wIsSupported(3, 3)) return Result(4, "OpenGL 3.3 not supported");

		// Print OpenGL version
		std::println("OpenGL Version: {}", get_gl_string(GL_VERSION));

		return Result();
	}

	static void cleanup() {
		m_quads.clear();
		m_shaders.clear();

		glfwMakeContextCurrent(nullptr);
		glfwDestroyWindow(m_mainWindow);
		glfwTerminate();
	}

	static void render() {
		// Backup context and set
		const auto ctx_backup = glfwGetCurrentContext();
		glfwMakeContextCurrent(m_mainWindow);

		// Enable blend
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		// Clear screen
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		for (auto &quad : m_quads)
			quad.render(ShaderVariables{static_cast<float>(glfwGetTime()),
										Vector2(m_mode->width, m_mode->height), 1.0f, 1.0f, 1.0f,
										0.0f});

		glfwSwapBuffers(m_mainWindow);

		// Restore blend
		glDisable(GL_BLEND);

		// Restore context
		glfwMakeContextCurrent(ctx_backup);
	}

	static auto get_main_window() -> GLFWwindow * { return m_mainWindow; }
	static auto get_monitor() -> GLFWmonitor * { return m_monitor; }
	static auto get_mode() -> const GLFWvidmode * { return m_mode; }

private:
	// Method that provides better glGetString, returns const char* instead of GLubyte*
	static auto get_gl_string(const GLenum name) -> const char * {
		return reinterpret_cast<const char *>(glGetString(name));
	}

	// GLFW error callback using format
	static void glfw_error_callback(int error, const char *description) {
		// Ignore GLFW_FEATURE_UNAVAILABLE as they're expected with Wayland
		if (error != GLFW_FEATURE_UNAVAILABLE)
			std::println("GLFW error {}: {}", error, description);
	}
};
