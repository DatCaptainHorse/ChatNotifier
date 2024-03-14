#define GLFW_INCLUDE_NONE // Don't include OpenGL headers, we are using glbinding
#define ASIO_STANDALONE // Standalone Asio, no Boost

#ifdef __MINGW32__
#if __has_include(<thread>)
/* bin: silly workaround, force this if we have <thread> so we don't fallback to boost */
#define _WEBSOCKETPP_CPP11_THREAD_
#endif /* __has_include(<thread>) */
#endif /* __MINGW32__ */

#include <fmt/format.h>
#include <glbinding/gl33core/gl.h>
#include <glbinding/glbinding.h>
#include <GLFW/glfw3.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <imgui_internal.h>

#include <thread>
#include <random>
#include <ranges>
#include <filesystem>

#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

constexpr auto amogus_ascii = R"(
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣠⣤⣤⣤⣤⣤⣤⣤⣤⣄⡀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⢀⣴⣿⡿⠛⠉⠙⠛⠛⠛⠛⠻⢿⣿⣷⣤⡀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⣼⣿⠋⠀⠀⠀⠀⠀⠀⠀⢀⣀⣀⠈⢻⣿⣿⡄⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⣸⣿⡏⠀⠀⠀⣠⣶⣾⣿⣿⣿⠿⠿⠿⢿⣿⣿⣿⣄⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⣿⣿⠁⠀⠀⢰⣿⣿⣯⠁⠀⠀⠀⠀⠀⠀⠀⠈⠙⢿⣷⡄⠀
⠀⠀⣀⣤⣴⣶⣶⣿⡟⠀⠀⠀⢸⣿⣿⣿⣆⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣿⣷⠀
⠀⢰⣿⡟⠋⠉⣹⣿⡇⠀⠀⠀⠘⣿⣿⣿⣿⣷⣦⣤⣤⣤⣶⣶⣶⣶⣿⣿⣿⠀
⠀⢸⣿⡇⠀⠀⣿⣿⡇⠀⠀⠀⠀⠹⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡿⠃⠀
⠀⣸⣿⡇⠀⠀⣿⣿⡇⠀⠀⠀⠀⠀⠉⠻⠿⣿⣿⣿⣿⡿⠿⠿⠛⢻⣿⡇⠀⠀
⠀⣿⣿⠁⠀⠀⣿⣿⡇⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢸⣿⣧⠀⠀
⠀⣿⣿⠀⠀⠀⣿⣿⡇⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢸⣿⣿⠀⠀
⠀⣿⣿⠀⠀⠀⣿⣿⡇⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢸⣿⣿⠀⠀
⠀⢿⣿⡆⠀⠀⣿⣿⡇⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢸⣿⡇⠀⠀
⠀⠸⣿⣧⡀⠀⣿⣿⡇⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣿⣿⠃⠀⠀
⠀⠀⠛⢿⣿⣿⣿⣿⣇⠀⠀⠀⠀⠀⣰⣿⣿⣷⣶⣶⣶⣶⠶⠀⢠⣿⣿⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⣿⣿⠀⠀⠀⠀⠀⣿⣿⡇⠀⣽⣿⡏⠁⠀⠀⢸⣿⡇⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⣿⣿⠀⠀⠀⠀⠀⣿⣿⡇⠀⢹⣿⡆⠀⠀⠀⣸⣿⠇⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⢿⣿⣦⣄⣀⣠⣴⣿⣿⠁⠀⠈⠻⣿⣿⣿⣿⡿⠏⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠈⠛⠻⠿⠿⠿⠿⠋⠁⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
)";

constexpr auto awoo_ascii = R"(
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⣤⣤⣄⣀⠀⠀⠀⠀⠀⠀⠀⠀⢀⣠⠤⢲⠆⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⣤⣤⣄⣀⠀⠀⠀⠀⠀⠀⢀⣤⣾⣿⡟⠉⠻⣌⠙⢲⣤⠀⠀⢀⡴⠚⣁⠀⠀⣏⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⡿⠁⠀⠀⠉⠓⠦⣀⠀⣰⠋⡵⢋⡟⠀⠀⠀⣈⣷⡟⠉⣧⡴⢋⡴⠚⢂⡖⠒⠿⡇⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⢰⠃⠀⢀⠜⠓⠢⡀⠈⠛⣯⢹⡿⢾⡷⠶⠚⠛⠉⠉⢿⣠⡿⡀⠀⠈⠑⠲⢟⠀⢀⡇⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⢸⠀⠀⢉⣷⢂⠀⠘⠀⠀⠈⠦⢷⣘⣧⠀⠀⠀⣀⣠⠼⠟⠀⠈⠢⠀⠀⠀⠀⠙⠺⡀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⢸⠀⠀⢙⠟⠁⠀⠀⠀⠀⠀⠀⠀⢮⡙⠛⠛⠉⠁⠀⠀⠀⠀⠀⠀⠀⢀⠀⠀⠀⠀⠈⠢⡀⠀⠀⠀⠀⠀
⠀⠀⠀⢸⠀⡰⠃⠀⠀⠀⠀⠀⠀⠀⡄⢸⡈⡝⢆⠀⠀⢄⠀⠀⠀⠈⢢⡀⠀⠈⢦⡀⠀⠀⠀⠀⠘⢄⠀⠀⠀⠀
⠀⠀⠀⠀⣷⠁⠀⠀⠀⠀⠀⠀⠀⢰⠁⢸⡇⢱⠀⢣⠀⠘⡄⠀⠀⠀⠀⣿⣄⡠⠚⡆⠀⠀⠀⠀⠠⡘⡆⠀⠀⠀
⠀⠀⠀⠀⡇⠀⠀⠀⠀⠀⠀⠀⠀⡿⠤⣼⡃⣸⠀⠀⢣⠀⣧⠀⠀⠀⠀⣿⣿⠀⢀⠿⢸⠀⠀⠀⠀⢣⣹⠀⠀⠀
⠀⠀⠀⢸⠀⠀⠀⠀⠀⡇⠀⠀⡼⠀⣠⠋⣿⢧⡀⠀⠀⣧⠿⣀⡤⢤⣾⣿⣿⠶⣾⣄⣿⠀⠀⠀⠀⢸⣾⠀⠀⠀
⠀⠀⠀⢸⠀⠀⠀⠀⡜⠀⢀⡞⣁⣴⣷⢾⣿⣓⡪⠀⠀⠋⠀⠁⠀⠀⠿⡽⢻⣷⣦⠙⣿⡀⠀⠀⠀⣼⡟⠀⠀⠀
⠀⠀⠀⢸⠀⠀⠀⡼⢁⡠⢻⠏⣰⠏⢡⣛⣿⣿⣿⣦⠀⠀⠀⠀⠀⠀⠀⣿⣿⣿⣿⡇⠈⡇⠀⠀⣴⠁⠀⠀⠀⠀
⠀⠀⠀⡇⠀⠀⠸⠗⠉⠀⣸⣴⡟⠀⢸⡟⢿⣿⡟⣿⠀⠀⠀⠀⠀⠀⠀⢻⣮⣿⣷⠃⠀⠘⣄⠀⠀⠙⢦⠀⠀⠀
⠀⠀⠀⡇⠀⠀⠀⠀⠀⠀⠁⠈⠳⡀⠈⠻⠶⠤⠿⠋⠀⠀⠀⠀⠀⠀⠀⠀⠈⠉⣡⢶⣶⣄⡌⢦⠀⠀⢸⡆⠀⠀
⠀⠀⢸⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣧⣴⠟⣤⣴⡆⢀⣀⠀⣀⣀⠤⣤⣤⣤⣤⣾⠁⠻⠟⠞⠀⢸⠆⠀⢸⠀⠀⠆
⠀⢠⠇⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⡟⠋⠚⠋⠉⠀⠘⠙⠋⠀⠀⠀⠀⠀⠚⢿⡏⠀⠀⠀⠀⢀⠎⠀⠀⠘⢤⣤⡞
⠀⡎⠀⠀⠀⠀⠀⠀⠀⡀⠀⢀⠀⢣⠀⠀⡀⠀⠀⢠⠀⠀⠀⠀⠀⠀⠀⠀⡸⠁⠀⠀⢀⡤⠋⠀⠈⠛⠒⡶⠊⠁
⢰⠁⠀⠀⠀⠀⠀⠀⠀⢰⠀⠀⠑⠤⣽⣿⠁⠀⠀⠈⢦⡀⠀⠀⠀⢀⣠⠞⠁⢀⡠⠔⠋⠀⠀⠀⢠⡤⠊⠀⠀⠀
⠘⣄⢤⢀⠀⠀⠀⠀⠀⠀⠳⢄⣉⣛⣻⣟⣃⡲⢦⣄⠀⠈⠉⡉⠯⣽⢶⣎⡉⠀⠀⠀⠀⠀⣠⠋⠉⠀⠀⠀⠀⠀
⠀⠀⠉⠛⠑⠤⠄⢀⡄⠀⠀⠀⢀⣾⠋⠀⠘⡄⠘⢦⣍⡐⠬⢧⡄⠈⢆⠈⠻⣆⣀⣠⠄⠚⠁⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠉⠉⠉⢐⠿⢇⣀⡀⠀⣱⠀⠀⣽⠀⠀⠈⣷⠀⠈⣦⠤⠋⠳⡄⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⠉⠁⠀⠀⠀⠀⠀⠀⠁⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
)";

constexpr auto nya_ascii = R"(
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⣀⣤⣤⣄⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⣀⣀⣀⢀⡀⣀⣀⣀⠀⠀⠀⠀⠀⣀⠴⠚⢩⠟⠉⣠⣇⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⣀⡤⠖⠋⠁⠀⠀⠉⠉⠉⠉⠀⠈⠉⠒⢤⣴⠚⠁⠀⢰⠃⠀⠀⠉⣩⠿
⠀⠀⠀⠀⠀⠀⠀⣀⠤⠒⣫⠟⠉⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⠑⢤⣀⡏⠀⠀⠀⠘⠉⡟
⠀⠀⢀⣀⠤⠖⠋⢁⣤⡞⠁⠀⠀⠀⠀⠀⠀⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⢳⢄⡀⠀⢀⡼⠁
⢶⡋⠉⠀⠀⠀⠉⢹⠋⠁⠀⠀⠀⠀⠀⠀⡴⠁⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⢶⠛⠟⠛⢹⠇
⠘⣷⣄⠀⠀⠀⢠⡷⠃⠀⠀⠀⠀⠀⠀⡸⠁⠀⠀⠀⠀⠀⠀⢸⠀⠀⠀⠀⠀⠀⠀⠀⠀⠘⡆⠀⠶⡏⠀
⠀⠘⢎⣑⢶⣖⠉⣠⠃⠀⠀⠀⢰⠀⣰⠁⠀⠀⠀⠀⠀⠀⠀⢀⡄⠀⠀⠐⡄⠀⠀⠀⠀⠀⣿⣤⠞⠁⠀
⠀⠀⠘⢏⣀⡈⢹⠃⠀⠀⠀⠀⣇⡴⢿⠀⡆⠀⠀⠀⠰⡄⠀⢹⢯⠁⢣⠀⢳⠀⠀⠀⠀⠀⠘⢧⣀⠀⢀
⠀⠀⠀⠀⠈⠳⡏⢀⠀⠀⠀⣾⡏⠁⠘⣆⣿⣄⠀⠀⠀⣧⢠⣧⣼⣷⣼⣷⣼⠀⠀⠀⠀⠀⠠⣄⣨⠽⠟
⠀⠀⠀⠀⠀⢸⡁⢸⠀⠀⠀⣏⢧⠀⠀⠈⠙⠊⠓⢦⣠⡿⠻⣿⣏⣀⢙⣿⣿⡄⠀⠀⠀⢰⠀⢷⠀⠀⠀
⠀⠀⠀⠀⠀⠈⣧⢸⣧⡀⠀⢹⡛⠷⠿⢶⣷⡀⠀⠀⠉⠀⠀⣯⠀⠛⢃⣽⠟⢁⠀⠀⠀⡜⠀⡞⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠈⣛⠟⠉⠓⠾⡧⠀⠀⠀⠈⠁⠀⠀⠀⠀⠀⠉⠉⢉⣴⣫⣴⡇⠀⢀⣼⡧⠞⠀⠀⠀⠀
⠀⠀⠀⣀⣠⠤⠚⠁⠀⠀⢀⠀⠱⣄⠀⠀⠀⠀⢠⣀⣄⣀⠀⠀⠀⠀⢀⡴⢻⡤⠔⠋⢸⡇⠀⠀⠀⠀⠀
⢀⡴⠋⢁⡴⠀⠀⠀⠀⢀⠌⠀⠀⠀⣹⡶⢤⣄⣀⠀⠀⠀⣀⣠⡴⠚⠁⠀⠀⢀⠀⠀⠈⢷⡀⠀⠀⠀⠀
⢸⠀⢀⣾⠁⠀⠀⠀⠀⠘⠀⠀⣠⠾⣿⣿⣦⡌⢻⡍⠉⠉⢠⠿⣄⡀⠀⠀⠀⢸⠀⠀⢀⠈⢿⡲⠤⠔⠀
⠸⡄⢸⡇⠀⢠⠀⠀⠀⠸⡀⢰⠃⠀⠀⢻⣿⣿⣾⣿⣦⣤⡟⢀⣏⡿⣆⠀⠀⠀⡇⠀⠀⢳⡀⠙⣆⠀⠀
⠀⠘⠾⠷⣀⠈⢧⠀⠀⠀⢣⡎⠀⠀⠀⠀⢿⣿⣿⣷⣽⣦⣧⣾⣿⣥⠿⡄⠀⠀⡀⠀⠀⠀⣿⢆⠈⢷⡀
⠀⠀⠀⠀⠀⠉⠛⠿⠦⢤⣠⠇⠀⠀⠀⠀⠀⠙⣿⣿⣿⣿⣿⣿⣿⠃⠀⠙⣆⣠⠃⠀⠀⠀⢸⠈⢧⠀⠇
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⠀⠀⠀⠀⠀⠀⠀⠈⠙⠛⢹⠿⣏⠁⠀⠀⠀⠈⠳⣄⠀⠀⣀⡼⠀⢸⣦⠇
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⣿⠀⠘⡆⠀⠀⠀⠀⠀⠀⠉⠉⠁⠀⠀⠘⠁⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠙⠓⠒⠒⠋⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
)";

using client = websocketpp::client<websocketpp::config::asio_client>;
using message_ptr = websocketpp::config::asio_client::message_type::ptr;

bool connected = false;

auto connect_twitch_chat() -> std::shared_ptr<client>;

// Array of various notification strings, that tell to look at chat for once..
constexpr std::array notificationStrings = {
    "Check the chat, nerd!",
    "Chat requires your attention",
    "You should check the chat..",
    "Feed the chatters, they're hungry",
    "Did you know that chat exists?",
    "Chat exploded! Just kidding, check it!",
    "Free chat check, limited time offer!",
    "Insert funny chat notification here",
    "Chatters are waffling, provide syrup",
};

// Map of easter-egg custom message words linked to asset sound wavs
const std::map<const char*, const char*> eggWordSounds = {
    {"tutturuu", "Assets/tutturuu.wav"},
    {"ding", "Assets/ding.wav"},
    {"amogus", "Assets/amogus.wav"},
};

// GLFW error callback using fmt
void glfw_error_callback(int error, const char* description)
{
    fmt::println(stderr, "Glfw Error {}: {}", error, description);
}

// Method for returning program with integer + message about the reason
auto return_program(const int code, const char* reason) -> int
{
    fmt::println(stderr, "{}", reason);
    return code;
}

// Method that provides better glGetString, returns const char* instead of GLubyte*
auto get_gl_string(const gl::GLenum name) -> const char*
{
    return reinterpret_cast<const char*>(gl::glGetString(name));
}

// Method for returning random integer between min, max
auto random_int(const int min, const int max) -> int
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution dist(min, max);
    return dist(gen);
}

// Lambda for converting string to lowercase
const auto lowercase = [](std::string str) -> std::string
{
    std::ranges::transform(str, str.begin(), ::tolower);
    return str;
};


bool show_notifier_window = false;
std::string current_notification = notificationStrings[0];
// Timer for showing the notifier window
float timer = 0.0f;
float showTime = 5.0f;
ma_engine audioEngine;

// Command/Trigger keywords length sorted map, of command -> function
std::map<std::string, std::function<void(const std::string&)>> commands_map = {
    {
        "cc", [](const std::string&)
        {
            // Reset timer
            timer = 0.0f;
            const auto randomIndex = random_int(0, notificationStrings.size() - 1);
            current_notification = notificationStrings[randomIndex];
            show_notifier_window = true;
        }
    },
    {
        "ccc", [](const std::string& msg)
        {
            // Play egg sound if customString contains any of the eggWordSounds keys
            for (const auto& [word, sound] : eggWordSounds)
            {
                if (lowercase(msg).find(word) != std::string::npos)
                {
                    if (std::filesystem::exists(sound))
                        ma_engine_play_sound(&audioEngine, sound, nullptr);

                    break; // Let's not play multiple sounds, don't want ears to bleed right?
                }
            }

            // Reset timer
            timer = 0.0f;
            current_notification = msg;

            // SPECIAL: If str contains "amogus", append the amogus ascii art
            if (lowercase(msg).find("amogus") != std::string::npos)
                current_notification += amogus_ascii;
            else if (lowercase(msg).find("awoo") != std::string::npos)
                current_notification += awoo_ascii;
            else if (lowercase(msg).find("nya") != std::string::npos)
                current_notification += nya_ascii;

            show_notifier_window = true;
        }
    },
};

// Approved vector of users to trigger the notifiers
std::vector<const char*> approvedUsers = {};

// Connection variables
std::string twitchAuthToken = "";
std::string twitchAuthUser = "";
std::string twitchChannel = "";

// And their input buffers
static std::array<char, 32> auth_buf = {""};
static std::array<char, 32> authuser_buf = {""};
static std::array<char, 32> channel_buf = {""};

// Main method
auto main() -> int
{
    // MINIAUDIO INITIALIZATION //
    auto mares = ma_engine_init(nullptr, &audioEngine);
    if (mares != MA_SUCCESS)
    {
        fmt::println(stderr, "Failed to initialize miniaudio engine: {}", ma_result_description(mares));
        return 1;
    }

    // Play tutturuu.wav (if it exists) to test if audio works
    if (std::filesystem::exists("Assets/tutturuu.wav"))
        ma_engine_play_sound(&audioEngine, "Assets/tutturuu.wav", nullptr);

    // TWITCH CHAT VARIABLES INITIALIZATION //
    std::shared_ptr<client> twitchClient = nullptr;
    std::thread twitchThread;

    // GLFW INITIALIZATION //
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return return_program(1, "Failed to initialize GLFW");

    // WINDOW CREATION //
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
    const auto mainWindow = glfwCreateWindow(420, 690, "Chat Notifier", nullptr, nullptr);
    if (!mainWindow)
        return return_program(1, "Failed to create window");

    glfwMakeContextCurrent(mainWindow);
    glfwSwapInterval(1); // V-Sync

    // GLBINDING INITIALIZATION //
    glbinding::initialize(glfwGetProcAddress, false); // false for lazy loading

    // Print OpenGL version
    fmt::println("OpenGL Version: {}", get_gl_string(gl::GL_VERSION));

    // IMGUI INITIALIZATION //
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    auto& io = ImGui::GetIO();

    // Enable docking and viewports
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigViewportsNoTaskBarIcon = true;
    io.ConfigViewportsNoDecoration = true;

    // Set more sensible font that supports UTF-8
    const auto robotoFont = io.Fonts->AddFontFromFileTTF("Assets/Roboto-Regular.ttf", 18.0f);
    // Set as default font
    io.FontDefault = robotoFont;

    // Calculate pixel density (DPI = (square root of (horizontal pixels² + vertical pixels²)) / diagonal screen size in inches)
    const auto monitor = glfwGetPrimaryMonitor();
    const auto mode = glfwGetVideoMode(monitor);
    const auto dpi = std::sqrt(std::pow(mode->width, 2) + std::pow(mode->height, 2)) / mode->width;
    const auto fontSize = 64.0 * dpi;
    fmt::println("DPI: {}, Font size: {}", dpi, fontSize);

    // NotoSansSymbols2-Regular.ttf for notifications
    // contains glyphs with braille and more so we need to tell ImGui that
    const ImWchar notif_ranges[] = {0x0020, 0x00FF, 0x2800, 0x28FF, 0};
    const auto notificationFont = io.Fonts->AddFontFromFileTTF("Assets/NotoSansSymbols2-Regular.ttf",
                                                               static_cast<float>(fontSize), nullptr, notif_ranges);

    // MAIN WINDOW IMGUI INITIALIZATION //
    ImGui_ImplGlfw_InitForOpenGL(mainWindow, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    // LOOP //
    while (!glfwWindowShouldClose(mainWindow))
    {
        // GLFW POLL EVENTS //
        glfwPollEvents();

        // IMGUI NEW FRAME //
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // CONTROL WINDOW //
        {
            // Create window that is always mainWindow sized and positioned at mainWindow location
            ImGui::SetNextWindowPos(ImGui::GetMainViewport()->Pos, ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImGui::GetMainViewport()->Size, ImGuiCond_Always);
            ImGui::Begin("Control", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize
                         | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

            // Settings portion, separators n stuff
            ImGui::Separator();
            ImGui::SetCursorPosX((ImGui::GetWindowWidth() / 2) - (ImGui::CalcTextSize("Notification Settings").x / 2));
            ImGui::Text("Notification Settings");
            ImGui::Separator();

            // Slider for showTime, integer from 1 to 10
            ImGui::SliderFloat("Show time", &showTime, 1.0f, 10.0f, "%.1f seconds");

            // Some spacing before the button
            ImGui::Dummy(ImVec2(0, 10));
            // Singular button, take whole window width, adjusted for padding
            if (ImGui::Button("Test Notification", ImVec2(ImGui::GetContentRegionAvail().x, 30)))
            {
                if (!show_notifier_window)
                {
                    // Reset timer
                    timer = 0.0f;
                    current_notification = "Test notification ahoy!";
                    show_notifier_window = true;
                }
            }

            // Add padding before separators
            ImGui::Dummy(ImVec2(0, 10));
            // Dividers + Title for Twitch settings
            ImGui::Separator();
            // Center the text horizontally
            ImGui::SetCursorPosX((ImGui::GetWindowWidth() / 2) - (ImGui::CalcTextSize("Twitch Settings").x / 2));
            ImGui::Text("Twitch Settings");
            ImGui::Separator();

            // Modifiable list of approved users
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "If the list is empty, anyone can trigger commands!");
            ImGui::Text("Approved users:");
            static std::array<char, 32> user_buf = {""};
            if (ImGui::Button("Add"))
            {
                if (user_buf[0] != '\0')
                {
                    // Create string from buffer, as clearing buffer after would clear the newly added user
                    const std::string user = user_buf.data();
                    approvedUsers.push_back(user.c_str());
                    std::ranges::fill(user_buf, '\0');
                }
            }
            ImGui::SameLine();
            ImGui::InputText("##approvedUsers", user_buf.data(), user_buf.size());

            // List of approved users
            for (const auto user : approvedUsers)
            {
                if (ImGui::Button("Remove"))
                {
                    std::erase(approvedUsers, user);
                }
                ImGui::SameLine();
                ImGui::Text(user);
            }

            // Padding
            ImGui::Dummy(ImVec2(0, 10));

            // Dropdown of command_map keys, inputbox and button for changing the command keyword
            ImGui::Text("Choose command to change keyword for:");
            static int selectedCommand = 0;
            ImGui::Combo("##commandsDrop", &selectedCommand, std::accumulate(commands_map.begin(), commands_map.end(),
                                                                             std::string{},
                                                                             [](const std::string& acc,
                                                                             const auto& pair)
                                                                             {
                                                                                 return acc + pair.first + '\0';
                                                                             }).c_str());
            // Button for setting the new keyword
            static std::array<char, 16> cmd_change_buf = {""};
            const auto it = std::next(commands_map.begin(), selectedCommand);
            if (ImGui::Button("Set as keyword"))
            {
                // If the input is not empty, set the new keyword
                if (cmd_change_buf[0] != '\0')
                {
                    // Find the command with the old keyword and erase it
                    const auto oldKey = it->first;
                    const auto func = it->second;
                    commands_map.erase(it);
                    // Insert the new keyword with the old function
                    commands_map[cmd_change_buf.data()] = func;
                    fmt::println("Changed keyword from '{}' to '{}'", oldKey, cmd_change_buf.data());
                    // Clear the input buffer
                    std::ranges::fill(cmd_change_buf, '\0');
                }
            }
            ImGui::SameLine();
            // Input box for changing the command keyword, hint is selected command current key
            ImGui::InputTextWithHint("##cmdChange", it->first.c_str(),
                                     cmd_change_buf.data(), cmd_change_buf.size());

            // Padding
            ImGui::Dummy(ImVec2(0, 10));

            ImGui::BeginDisabled(connected);

            // Input boxes for connection, auth and auth user are password fields, channel is normal
            ImGui::InputText("Twitch Auth Token", auth_buf.data(), auth_buf.size(), ImGuiInputTextFlags_Password);
            ImGui::InputText("Twitch Auth User", authuser_buf.data(), authuser_buf.size(), ImGuiInputTextFlags_Password);
            ImGui::InputText("Twitch Channel", channel_buf.data(), channel_buf.size());

            // Connect button
            if (ImGui::Button("Connect"))
            {
                // Allow only if all fields are filled
                if (auth_buf[0] != '\0' && authuser_buf[0] != '\0' && channel_buf[0] != '\0')
                {
                    // Set the new auth token, user and channel
                    twitchAuthToken = auth_buf.data();
                    twitchAuthUser = authuser_buf.data();
                    twitchChannel = channel_buf.data();

                    // Make sure thread isn't running, if it is, join it
                    if (twitchThread.joinable())
                        twitchThread.join();

                    twitchClient = connect_twitch_chat();
                    twitchThread = std::thread([twitchClient]() { twitchClient->run(); });
                }
            }
            ImGui::EndDisabled();
            ImGui::SameLine();
            // Connection status
            ImGui::Text("Status: %s", connected ? "Connected" : "Disconnected");

            // End window
            ImGui::End();
        }

        // NOTIFIER WINDOW //
        {
            if (show_notifier_window)
            {
                // Position center-top of the monitor
                const auto monitorSize = ImGui::GetViewportPlatformMonitor(ImGui::GetMainViewport())->MainSize;
                ImGui::SetNextWindowSize(monitorSize, ImGuiCond_Always);

                // Animate window going from top to center
                auto windowPos = ImVec2(0, (-monitorSize.y / 2) + (monitorSize.y / 2) * (timer / showTime));
                ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always);

                ImGui::Begin("Notifier window", &show_notifier_window, ImGuiWindowFlags_NoInputs
                             | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration
                             | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav
                             | ImGuiWindowFlags_NoDocking);

                // Set transparency for window viewport
                const auto viewport = ImGui::GetWindowViewport();
                viewport->Flags |= ImGuiViewportFlags_TransparentClearColor;
                viewport->Flags |= ImGuiViewportFlags_TopMost;
                viewport->Flags |= ImGuiViewportFlags_NoInputs;
                viewport->Flags |= ImGuiViewportFlags_NoFocusOnClick;
                viewport->Flags |= ImGuiViewportFlags_NoAutoMerge;

                // Set font to notificationFont
                ImGui::PushFont(notificationFont);
                // Center the text horizontally and vertically
                ImGui::SetCursorPosX(
                    (ImGui::GetWindowWidth() / 2) - (ImGui::CalcTextSize(current_notification.c_str()).x / 2));
                ImGui::SetCursorPosY(
                    (ImGui::GetWindowHeight() / 2) - (ImGui::CalcTextSize(current_notification.c_str()).y / 2));

                // Rainbows!
                const auto rainbow = [](const float frequency, const float phase, const float center,
                                        const float width) -> ImVec4
                {
                    const auto red = std::sin(frequency + 0 + phase) * width + center;
                    const auto green = std::sin(frequency + 2 + phase) * width + center;
                    const auto blue = std::sin(frequency + 4 + phase) * width + center;
                    return {red, green, blue, 1.0f};
                };

                // Use deltatime to change color
                ImGui::PushStyleColor(ImGuiCol_Text,
                                      rainbow(0.1f, static_cast<float>(glfwGetTime() * 5.0), 0.5f, 0.5f));
                ImGui::Text(current_notification.c_str());
                ImGui::PopStyleColor();
                ImGui::PopFont();
                ImGui::End();

                // Timer checks
                timer += io.DeltaTime;
                if (timer >= showTime)
                {
                    show_notifier_window = false;
                }
            }
        }

        // IMGUI RENDERING //
        ImGui::Render();
        int display_w = 0, display_h = 0;
        glfwGetFramebufferSize(mainWindow, &display_w, &display_h);
        gl::glViewport(0, 0, display_w, display_h);
        gl::glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
        gl::glClear(gl::GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            // Backup context
            const auto prev_ctx = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(prev_ctx);
        }

        // GLFW SWAP BUFFERS //
        glfwSwapBuffers(mainWindow);
    }

    // CLEAN //
    ma_engine_uninit(&audioEngine);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwMakeContextCurrent(nullptr);
    glfwDestroyWindow(mainWindow);
    glfwTerminate();

    if (twitchClient)
    {
        twitchClient->stop();
        if (twitchThread.joinable())
            twitchThread.join();
    }

    return 0;
}

// Handlers
void on_open(client* c, websocketpp::connection_hdl hdl)
{
    fmt::println("Connection Opened");

    const auto msg1 = std::format("PASS oauth:{}\r\n", twitchAuthToken);
    const auto msg2 = std::format("NICK {}\r\n", twitchAuthUser);
    const auto msg3 = std::format("JOIN #{}\r\n", twitchChannel);
    c->send(hdl, msg1, websocketpp::frame::opcode::text);
    c->send(hdl, msg2, websocketpp::frame::opcode::text);
    c->send(hdl, msg3, websocketpp::frame::opcode::text);

    connected = true;
}

void on_fail(client* c, websocketpp::connection_hdl hdl)
{
    fmt::println("Connection Failed");
    connected = false;
}

void on_message(client* c, websocketpp::connection_hdl hdl, message_ptr msg)
{
    //fmt::println("Received: {}", msg->get_payload());

    // Skip all the IRC stuff from the message so we only get the actual message
    auto ircPos = msg->get_payload().find("PRIVMSG");
    if (ircPos == std::string::npos)
        return;

    // Also skip the channel.. so until ":", then lowercase the message
    ircPos = msg->get_payload().find(':', ircPos);
    const auto realMessage = msg->get_payload().substr(ircPos + 1);

    // Check that the user is in the approvedUsers list, if empty just allow it
    const auto isApprovedUser = approvedUsers.empty() || std::ranges::any_of(approvedUsers, [msg](const auto& user)
    {
        return msg->get_payload().find(fmt::format(":{}", user)) != std::string::npos;
    });

    // Command handling
    if (isApprovedUser)
    {
        // If message does not begin with "!", skip
        if (realMessage[0] != '!')
            return;

        // Check for commands and execute them, from longest to shortest command key to avoid partial matches
        for (const auto& [command, func] : commands_map | std::ranges::views::reverse)
        {
            // Command with the "!" prefix to match the message
            const auto fullCommand = fmt::format("!{}", command);

            // Make sure the message is long enough to contain the command
            if (realMessage.size() < fullCommand.size())
                continue;

            // Extract the command from the message, so until whitespace is met after command or end is reached
            auto extractedCommand = realMessage;
            if (const auto wsPos = realMessage.find(' '); wsPos != std::string::npos)
                extractedCommand = realMessage.substr(0, wsPos);

            // Remove odd characters from the extracted command
            std::erase_if(extractedCommand, [](const auto& ec)
            {
                return ec == '\n' || ec == '\r' || ec == '\0' || ec == '\t' || std::isspace(ec);
            });

            // If command is shorter than the extracted command, skip, prevents partial matches
            if (fullCommand.size() < extractedCommand.size())
                continue;

            // Strict comparison of the command, to prevent partial matches
            if (lowercase(extractedCommand).find(fullCommand) != std::string::npos)
            {
                // Cut off the command from the message + space if there is one
                const auto msgWithoutCommand = realMessage.
                    substr(fullCommand.size() + (realMessage[fullCommand.size()] == ' '));
                func(msgWithoutCommand);
                break;
            }
        }
    }
}

void on_close(client* c, websocketpp::connection_hdl hdl)
{
    fmt::println("Connection Closed");
    connected = false;
}

auto connect_twitch_chat() -> std::shared_ptr<client>
{
    auto c = std::make_shared<client>();

    try
    {
        // Disable websocketpp logging
        c->clear_access_channels(websocketpp::log::alevel::all);
        c->clear_error_channels(websocketpp::log::elevel::all);

        c->init_asio();

        c->set_open_handler([c](auto&& PH1) { on_open(c.get(), std::forward<decltype(PH1)>(PH1)); });
        c->set_fail_handler([c](auto&& PH1) { on_fail(c.get(), std::forward<decltype(PH1)>(PH1)); });
        c->set_message_handler([c](auto&& PH1, auto&& PH2)
        {
            on_message(c.get(), std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2));
        });
        c->set_close_handler([c](auto&& PH1) { on_close(c.get(), std::forward<decltype(PH1)>(PH1)); });

        websocketpp::lib::error_code err;
        const auto endpoint = "ws://irc-ws.chat.twitch.tv:80";
        const auto conn = c->get_connection(endpoint, err);
        c->connect(conn);
    }
    catch (const std::exception& e)
    {
        fmt::println(stderr, "Exception: {}", e.what());
    }

    return c;
}
