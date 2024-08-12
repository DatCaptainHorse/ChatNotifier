module;

#include <map>
#include <print>
#include <string>
#include <functional>
#include <filesystem>

#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

export module scripting;

import config;
import common;
import runner;
import filesystem;

static constexpr auto hello() { std::println("Hello from ChatNotifier module!"); }

export class ScriptingHandler;

// Class for python module
export class ChatNotifierPyModule {
	static inline std::unique_ptr<nanobind::module_> m;

	static inline PyModuleDef nanobind_module_def_chatnotifierscripting;
	static inline void nanobind_init_chatnotifier() { m->def("hello", &hello); }

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
			nanobind_init_chatnotifier();
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

		// Acquire the GIL
		const auto gstate = PyGILState_Ensure();

		// Import the script
		scriptmodule = PyImport_ImportModule(path.stem().string().c_str());
		if (!scriptmodule) {
			// Get error
			if (PyErr_Occurred()) {
				PyObject *ptype, *pvalue, *ptraceback;
				PyErr_Fetch(&ptype, &pvalue, &ptraceback);
				if (pvalue) {
					PyObject *str = PyObject_Str(pvalue);
					const char *str_value = PyUnicode_AsUTF8(str);
					std::println("Error in script '{}': {}", path.filename().string(), str_value);
					Py_DECREF(str);
				}
				PyErr_Restore(ptype, pvalue, ptraceback);
			}
			// Clear error
			PyErr_Clear();

			// Release the GIL
			PyGILState_Release(gstate);
			return;
		}

		PyModule_AddObject(scriptmodule, "chatnotifier", module);
		const auto main_dict = PyModule_GetDict(scriptmodule);
		PyDict_SetItemString(main_dict, "chatnotifier", module);
		Py_DECREF(main_dict);

		// Release the GIL
		PyGILState_Release(gstate);
	}

	~Script() {
		// Acquire the GIL
		const auto gstate = PyGILState_Ensure();

		if (scriptmodule) Py_DECREF(scriptmodule);

		// Release the GIL
		PyGILState_Release(gstate);
	}

	[[nodiscard]] auto is_valid() const -> bool { return scriptmodule != nullptr; }
	[[nodiscard]] auto get_name() const -> std::string { return path.filename().string(); }
	[[nodiscard]] auto get_call_string() const -> std::string { return path.stem().string(); }

private:
	friend class ScriptingHandler;

	[[nodiscard]] auto has_method(const std::string &method) const -> bool {
		if (!scriptmodule) return false;

		// Acquire the GIL
		const auto gstate = PyGILState_Ensure();

		const auto has_method = PyObject_HasAttrString(scriptmodule, method.c_str());

		// Release the GIL
		PyGILState_Release(gstate);

		return has_method;
	}

	template <typename... Args>
	auto run_method(const std::string &method, Args... args) const -> void {
		if (!scriptmodule) return;

		// Acquire the GIL
		const auto gstate = PyGILState_Ensure();

		// Construct arguments
		auto nb_args = nanobind::make_tuple(args...);
		const auto pyargs = Py_BuildValue("O", nb_args.release().ptr());

		// Run the script
		const auto result = PyObject_CallMethod(scriptmodule, method.c_str(), "O", pyargs);
		if (!result) {
			// Get error
			if (PyErr_Occurred()) {
				PyObject *ptype, *pvalue, *ptraceback;
				PyErr_Fetch(&ptype, &pvalue, &ptraceback);
				if (pvalue) {
					PyObject *str = PyObject_Str(pvalue);
					const char *str_value = PyUnicode_AsUTF8(str);
					std::println("Error in script '{}': {}", path.filename().string(), str_value);
					Py_DECREF(str);
				}
				PyErr_Restore(ptype, pvalue, ptraceback);
			}
			// Clear error
			PyErr_Clear();
		}
		// if (pyargs) Py_DECREF(pyargs);
		if (result) Py_DECREF(result);

		// Release the GIL
		PyGILState_Release(gstate);
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

			// Set module search paths
			PyWideStringList_Insert(&config.module_search_paths, 0,
									Py_DecodeLocale(get_scripts_path().string().c_str(), nullptr));
			PyWideStringList_Insert(
				&config.module_search_paths, 1,
				Py_DecodeLocale(get_script_deps_path().string().c_str(), nullptr));
			PyWideStringList_Insert(
				&config.module_search_paths, 2,
				Py_DecodeLocale(get_scripts_sys_path().string().c_str(), nullptr));
			PyWideStringList_Insert(
				&config.module_search_paths, 3,
				Py_DecodeLocale(get_scripts_deps_sys_path().string().c_str(), nullptr));

			Py_InitializeFromConfig(&config);

			// Acquire the GIL
			const auto gstate = PyGILState_Ensure();

			// Initialize the module
			if (const auto res = ChatNotifierPyModule::initialize(); !res) {
				PyGILState_Release(gstate);
				return res;
			}

			// Add scripts and script deps paths
			const auto scriptPath = std::format(
				R"(import sys;sys.path.append("{}");sys.path.append("{}");sys.path.append("{}");sys.path.append("{}"))",
				get_scripts_path().string(), get_script_deps_path().string(),
				get_scripts_sys_path().string(), get_scripts_deps_sys_path().string());
			PyRun_SimpleStringFlags(scriptPath.c_str(), nullptr);
			PyErr_Clear(); // Clear any errors

			// Release the GIL
			PyGILState_Release(gstate);

			return Result();
		});

		return Result();
	}

	template <typename... Args>
	static void add_command(const std::string &name, std::function<void(Args...)> func) {
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
	static void add_command(const std::string &name, void (*func)(Args...)) {
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

			if (module) Py_DECREF(module);
			module = ChatNotifierPyModule::finalize();
			if (!module) return Result(1, "Failed to create module");

			return Result();
		});
		return Result();
	}

	static void cleanup() {
		// Run in the Python thread
		python_runner.add_job_sync([] {
			scripts.clear();
			if (module) Py_DECREF(module);
			Py_Finalize();
		});
	}

	static void execute_script_msg(const Script *script, const TwitchChatMessage &msg) {
		// Run in the Python thread
		python_runner.add_job([script, msg] {
			if (!script) return;
			// Get script from map and run the method, if it exists
			if (scripts.contains(script->get_name()))
				scripts[script->get_name()]->run_method("on_message", msg.get_message());
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
			onResult(scripts.contains(script->get_name()) &&
					 scripts[script->get_name()]->has_method(method));
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

	static void refresh_scripts(const RunnerFinish &onRefreshed = {}) {
		// Run in the Python thread
		python_runner.add_job(
			[] {
				// Acquire the GIL
				const auto gstate = PyGILState_Ensure();

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
					if (entry.is_regular_file() && entry.path().extension() == ".py")
						scripts[entry.path().filename().string()] =
							std::make_unique<Script>(entry.path(), module);
				}

				// Release the GIL
				PyGILState_Release(gstate);

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

	static auto get_scripts_sys_path() -> std::filesystem::path {
		return get_scripts_path() / "SYSTEM";
	}

	static auto get_script_deps_path() -> std::filesystem::path {
		return Filesystem::get_root_path() / "ScriptDeps";
	}

	static auto get_scripts_deps_sys_path() -> std::filesystem::path {
		return get_script_deps_path() / "SYSTEM";
	}
};
