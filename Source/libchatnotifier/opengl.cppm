module;

#ifndef CN_SUPPORTS_MODULES_STD
#include <standard.hpp>
#endif

#define GLFW_INCLUDE_NONE
#include <glad/gl.h>
#include <GLFW/glfw3.h>

#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#endif

export module opengl;

import standard;
import common;
import filesystem;

// Shader class (just minimal for fullscreen quads)
export class OpenGLShader {
	GLuint m_id = 0;

public:
	OpenGLShader() = default;
	OpenGLShader(const OpenGLShader &) = delete;
	OpenGLShader(OpenGLShader &&other) noexcept : m_id(other.m_id) { other.m_id = 0; }
	OpenGLShader(const std::string &vertexSrc, const std::string &fragmentSrc) {
		create(vertexSrc, fragmentSrc);
	}
	~OpenGLShader() { glDeleteProgram(m_id); }

	auto operator=(const OpenGLShader &) -> OpenGLShader & = delete;
	auto operator=(OpenGLShader &&other) noexcept -> OpenGLShader & {
		glDeleteProgram(m_id);
		m_id = other.m_id;
		other.m_id = 0;
		return *this;
	}

	auto create(const std::string &vertexSrc, const std::string &fragmentSrc) -> bool {
		const auto vertexShader = glCreateShader(GL_VERTEX_SHADER);
		const auto fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

		const char *vertexSrcC = vertexSrc.c_str();
		const char *fragmentSrcC = fragmentSrc.c_str();

		glShaderSource(vertexShader, 1, &vertexSrcC, nullptr);
		glCompileShader(vertexShader);

		int success;
		char infoLog[512];
		glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
		if (!success) {
			glGetShaderInfoLog(vertexShader, sizeof(infoLog), nullptr, infoLog);
			std::println("Vertex shader compilation failed: {}", infoLog);
			return false;
		}

		glShaderSource(fragmentShader, 1, &fragmentSrcC, nullptr);
		glCompileShader(fragmentShader);

		glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
		if (!success) {
			glGetShaderInfoLog(fragmentShader, sizeof(infoLog), nullptr, infoLog);
			std::println("Fragment shader compilation failed: {}", infoLog);
			return false;
		}

		m_id = glCreateProgram();
		glAttachShader(m_id, vertexShader);
		glAttachShader(m_id, fragmentShader);
		glLinkProgram(m_id);

		glGetProgramiv(m_id, GL_LINK_STATUS, &success);
		if (!success) {
			glGetProgramInfoLog(m_id, sizeof(infoLog), nullptr, infoLog);
			std::println("Shader program linking failed: {}", infoLog);
			return false;
		}

		glDeleteShader(vertexShader);
		glDeleteShader(fragmentShader);
		return true;
	}

	auto bind() const -> void { glUseProgram(m_id); }
	auto unbind() const -> void { glUseProgram(0); }

	auto get_id() const -> GLuint { return m_id; }

	auto set_uniform(const std::string &name, int value) const -> void {
		glUniform1i(glGetUniformLocation(m_id, name.c_str()), value);
	}
};

// Fullscreen quad
constexpr float quadVertices[] = {
	// positions        // texture Coords
	-1.0f, 1.0f, 0.0f, 1.0f, // top left
	-1.0f, -1.0f, 0.0f, 0.0f, // bottom left
	1.0f, 1.0f, 1.0f, 1.0f, // top right
	1.0f, -1.0f, 1.0f, 0.0f // bottom right
};

// Fullscreen quad VAO and VBO
export class OpenGLQuad {
	GLuint m_vao = 0;
	GLuint m_vbo = 0;

public:
	OpenGLQuad() = default;
	explicit OpenGLQuad(const std::vector<float> &vertices) {
		glGenVertexArrays(1, &m_vao);
		glGenBuffers(1, &m_vbo);

		glBindVertexArray(m_vao);
		glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(),
					 GL_STATIC_DRAW);

		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
							  reinterpret_cast<void *>(2 * sizeof(float)));
		glEnableVertexAttribArray(1);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}

	~OpenGLQuad() {
		glDeleteVertexArrays(1, &m_vao);
		glDeleteBuffers(1, &m_vbo);
	}

	auto bind() const -> void { glBindVertexArray(m_vao); }
	auto unbind() const -> void { glBindVertexArray(0); }
};

// Offscreen framebuffer helper class
export class OpenGLOffscreenFramebuffer {
	GLuint m_fbo = 0;
	GLuint m_texture = 0;
	GLuint m_rbo = 0;
	uint32_t m_width = 0;
	uint32_t m_height = 0;

public:
	OpenGLOffscreenFramebuffer() = default;
	explicit OpenGLOffscreenFramebuffer(const uint32_t width, const uint32_t height)
		: m_width(width), m_height(height) {
		glGenFramebuffers(1, &m_fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

		// Color attachment
		glGenTextures(1, &m_texture);
		glBindTexture(GL_TEXTURE_2D, m_texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_width, m_height, 0, GL_RGB, GL_UNSIGNED_BYTE,
					 nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_texture, 0);

		// Renderbuffer object
		glGenRenderbuffers(1, &m_rbo);
		glBindRenderbuffer(GL_RENDERBUFFER, m_rbo);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, m_width, m_height);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
								  m_rbo);

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			std::println("Framebuffer not complete!");

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	~OpenGLOffscreenFramebuffer() {
		glDeleteFramebuffers(1, &m_fbo);
		glDeleteTextures(1, &m_texture);
		glDeleteRenderbuffers(1, &m_rbo);
	}

	auto bind() const -> void {
		glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
		glViewport(0, 0, m_width, m_height);
	}

	auto unbind() const -> void {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, m_width / 4, m_height / 4);
	}

	auto get_texture() const -> GLuint { return m_texture; }
};

// Function type for render callback
export using RenderCallback = std::function<void()>;

// Class for OpenGL and GLFW handling
export class OpenGLHandler {
	static inline GLFWwindow *m_mainWindow = nullptr;
	static inline GLFWmonitor *m_monitor = nullptr;
	static inline const GLFWvidmode *m_mode;

	// Main offscreen framebuffer
	static inline OpenGLOffscreenFramebuffer m_offscreenFramebuffer;
	// Quad for offscreen framebuffer rendering
	static inline OpenGLQuad m_quad;
	// Shader for offscreen framebuffer rendering
	static inline OpenGLShader m_shader;

	// Render callback
	static inline RenderCallback m_renderCallback = nullptr;

public:
	static auto initialize(const RenderCallback &renderCB) -> Result {
		m_renderCallback = renderCB;

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
		glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
		glfwWindowHint(GLFW_AUTO_ICONIFY, GLFW_FALSE);
		glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);
		glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
		glfwWindowHint(GLFW_MOUSE_PASSTHROUGH, GLFW_TRUE);

		int xpos = 0, ypos = 0;
		// Find left-top most monitor
		int monitor_count = 0;
		const auto monitors = glfwGetMonitors(&monitor_count);
		for (int i = 0; i < monitor_count; i++) {
			int x, y;
			glfwGetMonitorPos(monitors[i], &x, &y);
			if (x < xpos || y < ypos) {
				xpos = x;
				ypos = y;
			}
		}

		// Position upcoming window outside the left-top most monitor
		glfwWindowHint(GLFW_POSITION_X, xpos + 2);
		glfwWindowHint(GLFW_POSITION_Y, ypos);

		// Main window
		m_mainWindow = glfwCreateWindow(m_mode->width - 4, m_mode->height, "ChatNotifier Notifications",
										nullptr, nullptr);
		if (!m_mainWindow) return Result(2, "Failed to create ChatNotifier notifications window");

		glfwMakeContextCurrent(m_mainWindow);
		glfwSwapInterval(1); // V-Sync

#ifdef _WIN32
		// Hide from toolbar
		/*const auto hwnd = glfwGetWin32Window(m_mainWindow);
		auto ex_style = GetWindowLong(hwnd, GWL_EXSTYLE);
		ex_style &= ~WS_EX_APPWINDOW;
		ex_style |= WS_EX_TOOLWINDOW;
		SetWindowLong(hwnd, GWL_EXSTYLE, ex_style);*/
#endif

		// GLAD INITIALIZATION //
		const auto version = gladLoadGL(glfwGetProcAddress);
		if (version == 0) return Result(3, "Failed to initialize GLAD");

		// Print OpenGL version
		std::println("OpenGL Version: {}.{}", GLAD_VERSION_MAJOR(version),
					 GLAD_VERSION_MINOR(version));

		// Create offscreen framebuffer
		std::println("Creating offscreen framebuffer of size {}x{}", m_mode->width / 4, m_mode->height / 4);
		m_offscreenFramebuffer = OpenGLOffscreenFramebuffer(m_mode->width / 4, m_mode->height / 4);

		// Create quad
		std::println("Creating fullscreen quad");
		m_quad = OpenGLQuad(std::vector(quadVertices, quadVertices + sizeof(quadVertices) / sizeof(float)));

		// Create shader
		std::println("Creating fullscreen shader");
		m_shader = OpenGLShader(R"(
			#version 330 core
			layout (location = 0) in vec2 aPos;
			layout (location = 1) in vec2 aTexCoords;

			out vec2 TexCoords;

			void main() {
				gl_Position = vec4(aPos, 0.0, 1.0);
				TexCoords = aTexCoords;
			}
		)",
								R"(
			#version 330 core
			out vec4 FragColor;

			in vec2 TexCoords;

			uniform sampler2D screenTexture;

			void main() {
				FragColor = texture(screenTexture, TexCoords);
			}
		)");

		return Result();
	}

	static void cleanup() {
		// Clear OpenGL and destroy window
		glfwMakeContextCurrent(nullptr);
		glfwDestroyWindow(m_mainWindow);
		glfwTerminate();
	}

	static void render() {
		// Render to offscreen framebuffer
		m_offscreenFramebuffer.bind();
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		m_renderCallback();
		m_offscreenFramebuffer.unbind();


		// Render the framebuffer as quad on to screen
		m_shader.bind();
		m_shader.set_uniform("screenTexture", 0);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, m_offscreenFramebuffer.get_texture());
		m_quad.bind();
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		m_quad.unbind();
		m_shader.unbind();

		// Swap buffers
		glfwSwapBuffers(m_mainWindow);
	}

	static auto get_main_window() -> GLFWwindow * { return m_mainWindow; }
	static auto get_monitor() -> GLFWmonitor * { return m_monitor; }
	static auto get_mode() -> const GLFWvidmode * { return m_mode; }

private:
	// GLFW error callback using format
	static void glfw_error_callback(int error, const char *description) {
		// Ignore GLFW_FEATURE_UNAVAILABLE as they're expected with Wayland
		if (error != GLFW_FEATURE_UNAVAILABLE)
			std::println("GLFW error {}: {}", error, description);
	}
};
