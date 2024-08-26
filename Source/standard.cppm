#ifndef CN_SUPPORTS_MODULES_STD
module;
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
#endif

export module standard;

#ifdef CN_SUPPORTS_MODULES_STD
export import std;
#endif
