module;

#include <map>
#include <print>
#include <string>
#include <functional>
#include <filesystem>
#include <source_location>

#include <Python.h>

export module scripting;

import types;
import common;
import runner;
import filesystem;
import pythonmodule;

// Helper to convert backslashes to forward slashes
std::string to_posix_path(const std::string &path) {
	std::string posix_path = path;
	std::replace(posix_path.begin(), posix_path.end(), '\\', '/');
	return posix_path;
}

// Helper for decrementing Python reference count, checking that GIL is held
void decref_pyobject(PyObject *obj,
					 const std::source_location &loc = std::source_location::current()) {
	if (!obj) return;
	if (!PyGILState_Check()) {
		std::println("decref_pyobject called without holding the GIL at {}::{}", loc.file_name(),
					 loc.line());
		return;
	}
	Py_DECREF(obj);
}

// Method for printing out any occurred Python errors
void print_python_error(const std::source_location &loc = std::source_location::current()) {
	if (PyErr_Occurred()) {
		PyObject *ptype, *pvalue, *ptraceback;
		PyErr_Fetch(&ptype, &pvalue, &ptraceback);
		if (pvalue) {
			PyObject *str = PyObject_Str(pvalue);
			const char *str_value = PyUnicode_AsUTF8(str);
			std::println("Python error at {}::{}: {}", loc.file_name(), loc.line(), str_value);
			decref_pyobject(str);
		}
		PyErr_Restore(ptype, pvalue, ptraceback);
	}
	PyErr_Clear();
}

export class ScriptingHandler;

export class Script {
	std::filesystem::path path;
	PyObject *scriptmodule = nullptr;

public:
	explicit Script(std::filesystem::path filepath, PyObject *module) : path(std::move(filepath)) {
		if (!std::filesystem::exists(path)) {
			std::println("Script file '{}' does not exist", path.string());
			return;
		}

		// Import the script
		scriptmodule = PyImport_ImportModule(path.stem().string().c_str());
		if (!scriptmodule) {
			print_python_error();
			return;
		}

		if (!PyModule_AddObjectRef(scriptmodule, "chatnotifier", module)) {
			print_python_error();
			return;
		}
	}

	~Script() { decref_pyobject(scriptmodule); }

	[[nodiscard]] auto is_valid() const -> bool { return scriptmodule != nullptr; }
	[[nodiscard]] auto get_name() const -> std::string { return path.filename().string(); }
	[[nodiscard]] auto get_call_string() const -> std::string { return path.stem().string(); }

private:
	friend class ScriptingHandler;

	[[nodiscard]] auto has_method(const std::string &method) const -> bool {
		if (!scriptmodule) return false;
		return PyObject_HasAttrString(scriptmodule, method.c_str());
	}

	template <typename... Args>
	auto run_method(const std::string &method, Args... args) const -> void {
		if (!scriptmodule) return;

		// Get the method
		const auto pymethod = PyObject_GetAttrString(scriptmodule, method.c_str());
		if (!pymethod) {
			print_python_error();
			return;
		}

		// Create the arguments if any
		if constexpr (sizeof...(args) > 0) {
			auto pyargs = PyTuple_New(sizeof...(args));
			if (!pyargs) {
				print_python_error();
				decref_pyobject(pymethod);
				return;
			}

			// Get args into tuple
			for (size_t i = 0; const auto &arg : {args...}) {
				if constexpr (std::is_same_v<std::decay_t<decltype(arg)>, std::string>)
					PyTuple_SetItem(pyargs, i, Py_BuildValue("s", arg.c_str()));
				else if constexpr (std::is_same_v<std::decay_t<decltype(arg)>,
												  std::vector<std::string>>)
					PyTuple_SetItem(pyargs, i, Py_BuildValue("[s]", arg.c_str()));
				else if constexpr (std::is_same_v<std::decay_t<decltype(arg)>, std::vector<float>>)
					PyTuple_SetItem(pyargs, i, Py_BuildValue("[f]", arg));
				else if constexpr (std::is_same_v<std::decay_t<decltype(arg)>, TwitchChatMessage>) {
					const auto pymsg = PyCreateTwitchChatMessage(arg);
					PyTuple_SetItem(pyargs, i, pymsg);
				} else
					PyTuple_SetItem(pyargs, i, arg);
			}

			// Call the method
			const auto result = PyObject_CallObject(pymethod, pyargs);
			if (!result) print_python_error();

			decref_pyobject(pyargs);
			decref_pyobject(result);
		} else {
			// Call the method
			const auto result = PyObject_CallObject(pymethod, nullptr);
			if (!result) print_python_error();

			decref_pyobject(result);
		}
		decref_pyobject(pymethod);
	}
};

class ScriptingHandler {
	static inline std::map<std::string, std::unique_ptr<Script>> scripts;
	static inline PyObject *module = nullptr;
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

			Py_InitializeFromConfig(&config);

			module = ChatNotifierModuleCreate();
			if (!module) {
				std::println("Failed to create Python module");
				return;
			}

			// Add scripts paths and subdirectories to sys.path, always use POSIX path since '\'s
			// break Python
			std::string scriptPath = "import sys\n";
			scriptPath +=
				std::format("sys.path.append('{}')\n", to_posix_path(get_scripts_path().string()));
			for (const auto &entry : std::filesystem::directory_iterator(get_scripts_path())) {
				if (entry.is_directory())
					scriptPath += std::format("sys.path.append('{}')\n",
											  to_posix_path(entry.path().string()));
			}

			PyRun_SimpleStringFlags(scriptPath.c_str(), nullptr);

			print_python_error();
		});

		return Result();
	}

	static void cleanup() {
		// Run in the Python thread
		python_runner.add_job_sync([] {
			scripts.clear();
			decref_pyobject(module);
			Py_Finalize();
		});
	}

	static void execute_script_msg(const Script *script, const TwitchChatMessage &msg) {
		// Run in the Python thread
		python_runner.add_job([script, msg] {
			if (!script) return;
			// Get script from map and run the method, if it exists
			if (scripts.contains(script->get_name()))
				scripts[script->get_name()]->run_method("on_message", msg);
		});
	}

	static void has_script_method(const Script *script, const std::string &method,
								  const std::function<void(bool)> &onResult) {
		// Run in the Python thread
		python_runner.add_job([script, method, onResult] {
			if (!script) {
				onResult(false);
				return;
			}
			// Get script from map and check if the method exists
			if (scripts.contains(script->get_name())) {
				const auto result = scripts[script->get_name()]->has_method(method);
				onResult(result);
			} else
				onResult(false);
		});
	}

	template <typename... Args>
	static void execute_script_method(const Script *script, const std::string &method,
									  Args... args) {
		// Run in the Python thread
		python_runner.add_job([script, method, args...] {
			if (!script) return;
			// Get script from map and run the method, if it exists
			if (scripts.contains(script->get_name()))
				scripts[script->get_name()]->run_method(method, args...);
		});
	}

	// Calls all scripts which have specified method
	template <typename... Args>
	static void execute_all_scripts_method(const std::string &method, Args... args) {
		// Run in the Python thread
		python_runner.add_job([method, args...] {
			for (const auto &[_, script] : scripts)
				if (script->has_method(method)) script->run_method(method, args...);
		});
	}

	static void refresh_scripts(const RunnerFinish &onRefreshed = {}) {
		// Run in the Python thread
		python_runner.add_job(
			[] {
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
							std::make_unique<Script>(entry.path(), module);
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

	static auto get_scripts() -> std::vector<Script *> {
		std::vector<Script *> script_ptrs;
		for (const auto &[_, script] : scripts) script_ptrs.push_back(script.get());
		return script_ptrs;
	}

	static auto get_scripts_path() -> std::filesystem::path {
		return Filesystem::get_root_path() / "Scripts";
	}
};
