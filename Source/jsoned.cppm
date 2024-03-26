module;

#include <map>
#include <string>
#include <vector>
#include <format>
#include <fstream>
#include <variant>
#include <iostream>
#include <optional>
#include <algorithm>
#include <filesystem>
#include <type_traits>

export module jsoned;

import common;

template <typename T>
concept JsonableAsValue =
	std::same_as<T, std::string> || std::is_integral_v<T> || std::same_as<T, bool> ||
	std::is_floating_point_v<T> || std::is_integral_v<std::underlying_type_t<T>>;

template <typename T>
concept JsonableAsArray = requires(T t) {
	{ t.begin() } -> std::input_or_output_iterator;
	{ t.end() } -> std::input_or_output_iterator;
} && !JsonableAsValue<T>;

template <typename T>
concept Jsonable = JsonableAsValue<T> || JsonableAsArray<T>;

export namespace JSONed {
	class ValueAccessor {
		std::string value;

	public:
		ValueAccessor() : value("") {}
		explicit ValueAccessor(const std::string &value) { set(value); }

		template <JsonableAsValue T>
		requires(!JsonableAsArray<T>)
		void set(const T &v) {
			value = t_to_string<T>(v);
		}

		template <JsonableAsValue T>
		requires(!JsonableAsArray<T>)
		[[nodiscard]] auto get() const -> std::optional<T> {
			// Make sure the value is not empty
			if (value.empty()) return std::nullopt;

			return std::make_optional(t_from_string<T>(value));
		}

		auto to_string() const -> std::string { return std::format("\"{}\"", value); }
		static auto from_string(const std::string &str) -> ValueAccessor {
			return ValueAccessor(str);
		}
	};

	class ArrayAccessor {
		std::vector<ValueAccessor> values;

	public:
		ArrayAccessor() {}
		explicit ArrayAccessor(const std::vector<ValueAccessor> &values) : values(values) {}

		template <JsonableAsArray T>
		requires(!JsonableAsValue<T>)
		void set(const T &v) {
			values.clear();
			for (const auto &elem : v) {
				ValueAccessor accessor;
				accessor.set(elem);
				values.push_back(accessor);
			}
		}

		template <JsonableAsArray T>
		requires(!JsonableAsValue<T>)
		[[nodiscard]] auto get() const -> T {
			// Make sure the values is not empty
			if (values.empty()) return T{};

			T array;
			for (const auto &value : values) array.push_back(value.get<typename T::value_type>().value());
			return array;
		}

		// Prettified output
		auto to_string() const -> std::string {
			std::string str = "[\n";
			for (const auto &value : values)
				str += std::format("\t\t{}{}", value.to_string(),
								   &value == &values.back() ? "" : ",\n");

			str += "\n\t]";
			return str;
		}

		static auto from_string(const std::string &str) -> ArrayAccessor {
			ArrayAccessor accessor;
			const auto trimmed = trim_string(str);
			const auto between = get_string_between(trimmed, "[", "]");
			if (between.empty()) return accessor;

			for (const auto splitted = split_string(between, ","); const auto &elem : splitted) {
				auto trimmedElem = trim_string(elem);
				trimmedElem = trim_string(trimmedElem, "\"");
				accessor.values.push_back(ValueAccessor::from_string(trimmedElem));
			}

			return accessor;
		}
	};

	// Class which brings together ValueAccessor and ArrayAccessor
	class Accessor {
		std::variant<ValueAccessor, ArrayAccessor> m_accessor;

	public:
		Accessor() : m_accessor(ValueAccessor()) {}
		explicit Accessor(const ValueAccessor &value) : m_accessor(value) {}
		explicit Accessor(const ArrayAccessor &array) : m_accessor(array) {}

		Accessor &operator=(const ValueAccessor &value) {
			m_accessor = value;
			return *this;
		}
		Accessor &operator=(const ArrayAccessor &array) {
			m_accessor = array;
			return *this;
		}

		template <Jsonable T>
		requires(!JsonableAsArray<T>)
		void set(const T &v) {
			m_accessor = ValueAccessor();
			std::get<ValueAccessor>(m_accessor).set(v);
		}

		template <Jsonable T>
		requires(JsonableAsArray<T>)
		void set(const T &v) {
			m_accessor = ArrayAccessor();
			std::get<ArrayAccessor>(m_accessor).set(v);
		}

		template <Jsonable T>
		requires(!JsonableAsArray<T>)
		[[nodiscard]] auto get() const -> std::optional<T> {
			return std::get<ValueAccessor>(m_accessor).get<T>();
		}

		template <Jsonable T>
		requires(JsonableAsArray<T>)
		[[nodiscard]] auto get() const -> std::optional<T> {
			return std::get<ArrayAccessor>(m_accessor).get<T>();
		}

		auto visiter() const -> const std::variant<ValueAccessor, ArrayAccessor> & {
			return m_accessor;
		}
	};

	class JSON {
		std::map<std::string, Accessor> m_data;

	public:
		// Accessor for JSON data operators
		auto operator[](const std::string &key) -> Accessor & {
			if (!m_data.contains(key)) m_data[key] = Accessor();

			return m_data[key];
		}

		// Save JSON to file
		auto save(const std::filesystem::path &path) -> bool {
			std::ofstream file(path);
			if (!file.is_open()) return false;

			file << "{\n";
			for (const auto &[key, value] : m_data) {
				file << std::format(
					"\t\"{}\": {}{}", key,
					std::visit([](const auto &v) { return v.to_string(); }, value.visiter()),
					&key == &m_data.rbegin()->first ? "" : ",\n");
			}

			file << "\n}\n";
			file.close();
			return true;
		}

		// Load JSON from file
		auto load(const std::filesystem::path &path) -> bool {
			std::ifstream file(path);
			if (!file.is_open()) return false;

			std::string line;
			while (std::getline(file, line)) {
				// Trim line and remove quotes
				auto trimmed = trim_string(line);
				trimmed = trim_string(trimmed, "\"");
				trimmed = trim_string(trimmed, ",");
				if (trimmed.empty()) continue;

				// Split key and value
				const auto splitted = split_string(trimmed, ":");
				if (splitted.size() != 2) continue;

				auto key = splitted[0];
				key = trim_string(key);
				auto value = splitted[1];
				value = trim_string(value);

				// Check if array
				if (value.starts_with("[")) {
					// Read whole array if it's multi-line
					std::string array = value;
					while (!array.ends_with("]") && !array.ends_with("],") && !file.eof()) {
						std::getline(file, line);
						array += line;
					}
					m_data[key] = ArrayAccessor::from_string(array);
				} else
					m_data[key] = ValueAccessor::from_string(value);
			}

			file.close();
			return true;
		}
	};

} // namespace JSONed
