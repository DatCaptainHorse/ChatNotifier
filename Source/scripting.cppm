module;

#include <map>
#include <print>
#include <string>
#include <functional>
#include <filesystem>
#include <source_location>

#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

export module scripting;

import common;
import runner;
import filesystem;

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

// Class for python module
export class ChatNotifierPyModule {
	static inline std::unique_ptr<nanobind::module_> m;

	static inline PyModuleDef nanobind_module_def_chatnotifierscripting;

	friend class ScriptingHandler;

	static auto initialize() -> Result {
		nanobind::detail::init(nullptr);
		m = std::make_unique<nanobind::module_>(
			nanobind::steal<nanobind::module_>(nanobind::detail::module_new(
				"chatnotifier", &nanobind_module_def_chatnotifierscripting)));

		return Result();
	}

	static auto finalize() -> PyObject * {
		try {
			return m.release()->release().ptr();
		} catch (const std::exception &e) {
			PyErr_SetString(PyExc_ImportError, e.what());
			return nullptr;
		}
	}

	template <typename... Args>
	static void add_command(const std::string &name, std::function<void(Args...)> func) {
		m->def(name.c_str(), func);
	}

	// From actual function
	template <typename... Args>
	static void add_command(const std::string &name, void (*func)(Args...)) {
		m->def(name.c_str(), func);
	}

	// With return
	template <typename Ret, typename... Args>
	static void add_command(const std::string &name, Ret (*func)(Args...)) {
		m->def(name.c_str(), func);
	}
};

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

		// Run the script
		const auto result = PyObject_CallObject(
			pymethod, sizeof...(Args) > 0 ? nanobind::make_tuple(args...).ptr() : nullptr);
		if (!result) {
			print_python_error();
			decref_pyobject(pymethod);
			return;
		}

		decref_pyobject(result);
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
		python_runner.add_job_sync([] {
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

			// Acquire the GIL
			const auto gstate = PyGILState_Ensure();

			// Initialize the module
			if (const auto res = ChatNotifierPyModule::initialize(); !res) {
				PyGILState_Release(gstate);
				return res;
			}

			// Add scripts paths and subdirectories to sys.path
			std::string scriptPath = "import sys\n";
			scriptPath += std::format("sys.path.append('{}')\n", get_scripts_path().string());
			for (const auto &entry : std::filesystem::directory_iterator(get_scripts_path())) {
				if (entry.is_directory())
					scriptPath += std::format("sys.path.append('{}')\n", entry.path().string());
			}

			PyRun_SimpleStringFlags(scriptPath.c_str(), nullptr);

			print_python_error();

			// Release the GIL
			PyGILState_Release(gstate);

			return Result();
		});

		return Result();
	}

	template <typename... Args>
	static void add_function(const std::string &name, std::function<void(Args...)> func) {
		// Run in the Python thread
		python_runner.add_job_sync([name, func] {
			// Acquire the GIL
			const auto gstate = PyGILState_Ensure();

			ChatNotifierPyModule::add_command(name, func);

			// Release the GIL
			PyGILState_Release(gstate);
		});
	}

	template <typename... Args>
	static void add_function(const std::string &name, void (*func)(Args...)) {
		// Run in the Python thread
		python_runner.add_job_sync([name, func] {
			// Acquire the GIL
			const auto gstate = PyGILState_Ensure();

			ChatNotifierPyModule::add_command(name, func);

			// Release the GIL
			PyGILState_Release(gstate);
		});
	}

	template <typename Ret, typename... Args>
	static void add_function(const std::string &name, Ret (*func)(Args...)) {
		// Run in the Python thread
		python_runner.add_job_sync([name, func] {
			// Acquire the GIL
			const auto gstate = PyGILState_Ensure();

			ChatNotifierPyModule::add_command(name, func);

			// Release the GIL
			PyGILState_Release(gstate);
		});
	}

	static auto create_module() -> Result {
		// Run in the Python thread
		python_runner.add_job_sync([] {
			// Acquire the GIL
			const auto gstate = PyGILState_Ensure();

			decref_pyobject(module);
			module = ChatNotifierPyModule::finalize();
			if (!module) return Result(1, "Failed to create module");

			// Release the GIL
			PyGILState_Release(gstate);

			return Result();
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
			if (scripts.contains(script->get_name())) {
				// Acquire the GIL
				const auto gstate = PyGILState_Ensure();
				scripts[script->get_name()]->run_method("on_message", msg.get_message());
				// Release the GIL
				PyGILState_Release(gstate);
			}
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
				// Acquire the GIL
				const auto gstate = PyGILState_Ensure();
				const auto result = scripts[script->get_name()]->has_method(method);
				// Release the GIL
				PyGILState_Release(gstate);
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
			if (scripts.contains(script->get_name())) {
				// Acquire the GIL
				const auto gstate = PyGILState_Ensure();
				scripts[script->get_name()]->run_method(method, args...);
				// Release the GIL
				PyGILState_Release(gstate);
			}
		});
	}

	static void refresh_scripts(const RunnerFinish &onRefreshed = {}) {
		// Run in the Python thread
		python_runner.add_job(
			[] {
				// Acquire the GIL
				const auto gstate = PyGILState_Ensure();

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

				// Release the GIL
				PyGILState_Release(gstate);

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
