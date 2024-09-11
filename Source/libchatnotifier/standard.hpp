#pragma once

#ifndef CN_SUPPORTS_MODULES_STD
#include <print>
#include <locale>
#include <thread>
#include <chrono>
#include <format>
#include <iostream>
#include <algorithm>
#include <filesystem>
#include <map>
#include <array>
#include <tuple>
#include <vector>
#include <ranges>
#include <string>
#include <random>
#include <utility>
#include <numeric>
#include <algorithm>
#include <type_traits>
#include <fstream>
#include <memory>
#include <optional>
#include <source_location>
#include <functional>
#include <condition_variable>
#include <semaphore>
#include <mutex>
#include <cmath>
#include <numbers>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define VC_EXTRALEAN
#include <Windows.h>
#endif
#endif
