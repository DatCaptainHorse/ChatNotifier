module;

#include <pybind11/embed.h>
#include <pybind11/stl.h>

export module scripting;

import standard;
import common;
import runner;
import filesystem;
import pythonmodule;

namespace py = pybind11;

// Helper to convert backslashes to forward slashes
std::string to_posix_path(const std::string &path) {
	std::string posix_path = path;
	std::replace(posix_path.begin(), posix_path.end(), '\\', '/');
	return posix_path;
}

export class ScriptingHandler;

export class Script {
	std::filesystem::path path;
	py::module_ scriptmodule;
	bool valid = false;

public:
	explicit Script(std::filesystem::path filepath) : path(std::move(filepath)) {
		if (!std::filesystem::exists(path)) {
			std::println("Script file '{}' does not exist", path.string());
			return;
		}

		// Import the script
		try {
			scriptmodule = py::module_::import(path.stem().string().c_str());
		} catch (const std::exception &e) {
			std::println("Error loading script '{}':", path.filename().string());
			std::println("{}", e.what());
			return;
		}
		valid = true;
	}

	[[nodiscard]] auto is_valid() const -> bool { return valid; }
	[[nodiscard]] auto get_name() const -> std::string { return path.filename().string(); }
	[[nodiscard]] auto get_call_string() const -> std::string { return path.stem().string(); }
	[[nodiscard]] auto has_method(const std::string &method) const -> bool {
		if (!valid) return false;
		return py::hasattr(scriptmodule, method.c_str());
	}

private:
	friend class ScriptingHandler;

	template <typename... Args>
	auto run_method(const std::string method, Args... args) const -> void {
		if (!valid) return;
		try {
			scriptmodule.attr(method.c_str())(args...);
		} catch (const std::exception &e) {
			std::println("Error in script '{}':", path.filename().string());
			std::println("{}", e.what());
		}
	}
};

class ScriptingHandler {
	static inline std::map<std::string, std::unique_ptr<Script>> scripts;
	static inline Runner python_runner;

public:
	static auto initialize() -> Result {
		// Initialize Python in a separate thread
		python_runner.add_job_sync([]() -> void {
			PyPreConfig preconfig = {};
			preconfig.utf8_mode = true;
			PyPreConfig_InitIsolatedConfig(&preconfig);
			Py_PreInitialize(&preconfig);

			PyConfig config = {};
			config.user_site_directory = 0;
			config.module_search_paths_set = 1;
			config.install_signal_handlers = 0;
			// We don't want to generate __pycache__ directories
			config.write_bytecode = 0;
			PyConfig_InitIsolatedConfig(&config);

			// If "python-embed" directory is found, set that as home
			if (const auto python_embed = Filesystem::get_root_path() / "python-embed";
				std::filesystem::exists(python_embed))
				PyConfig_SetString(&config, &config.home,
								   Py_DecodeLocale(python_embed.string().c_str(), nullptr));

			// Set module search paths, add any subdirectories of get_scripts_path() as well
			PyWideStringList_Insert(&config.module_search_paths, 0,
									Py_DecodeLocale(get_scripts_path().string().c_str(), nullptr));
			for (const auto &entry : std::filesystem::directory_iterator(get_scripts_path())) {
				if (entry.is_directory())
					PyWideStringList_Append(
						&config.module_search_paths,
						Py_DecodeLocale(entry.path().string().c_str(), nullptr));
			}

			py::initialize_interpreter();

			// Add scripts paths and subdirectories to sys.path, always use POSIX path
			std::string sysAppends = "import sys\n";
			sysAppends +=
				std::format("sys.path.append('{}')\n", to_posix_path(get_scripts_path().string()));
			for (const auto &entry : std::filesystem::directory_iterator(get_scripts_path())) {
				if (entry.is_directory())
					sysAppends += std::format("sys.path.append('{}')\n",
											  to_posix_path(entry.path().string()));
			}

			auto globals = py::globals();
			py::exec(sysAppends, globals, globals);
		});

		return Result();
	}

	static void cleanup() {
		// Run in the Python thread
		python_runner.add_job_sync([] {
			scripts.clear();
			py::finalize_interpreter();
		});
	}

	template <typename... Args>
	static void execute_script_method(const Script *script, const std::string method,
									  Args... args) {
		// Run in the Python thread
		python_runner.add_job([script, method, args...] {
			if (!script) return;
			script->run_method(method, args...);
		});
	}

	// Calls all scripts which have specified method
	template <typename... Args>
	static void execute_all_scripts_method(const std::string method, Args... args) {
		// Run in the Python thread
		python_runner.add_job([method, args...] {
			for (const auto &[_, script] : scripts)
				if (script->has_method(method)) script->run_method(method, args...);
		});
	}

	static void refresh_scripts(const RunnerFinish &onRefreshed = {}) {
		// Run in the Python thread
		python_runner.add_job(
			[onRefreshed] {
				// Clear the scripts
				scripts.clear();

				const auto script_path = get_scripts_path();
				if (!std::filesystem::exists(script_path)) {
					// Create the directory if it doesn't exist
					if (!std::filesystem::create_directory(script_path)) {
						// TODO: List of error codes in some common file, which are used instead
						return Result(1, "Failed to create scripts directory");
					}
				}

				for (const auto &entry : std::filesystem::directory_iterator(script_path)) {
					if (entry.is_regular_file() && entry.path().extension() == ".py") {
						std::println("Loading script: {}", entry.path().filename().string());
						scripts[entry.path().filename().string()] =
							std::make_unique<Script>(entry.path());
					}
				}

				// For each script, check and call "on_load" method
				for (const auto &[_, script] : scripts) {
					if (script->has_method("on_load"))
						execute_script_method(script.get(), "on_load");
				}

				return Result();
			},
			onRefreshed);
	}

	// Returns vector of script pointers
	static auto get_scripts() -> const std::vector<Script *> {
		std::vector<Script *> script_ptrs;
		for (const auto &[_, script] : scripts) script_ptrs.emplace_back(script.get());

		return script_ptrs;
	}

	static auto get_scripts_path() -> std::filesystem::path {
		return Filesystem::get_root_path() / "Scripts";
	}
};
