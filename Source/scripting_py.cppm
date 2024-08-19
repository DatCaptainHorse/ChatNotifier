module;

#include <vector>
#include <filesystem>

#include <Python.h>
#include <structmember.h>

export module pythonmodule;

import assets;
import audio;
import types;

/* Types */
struct PyTwitchChatMessage {
	PyObject_HEAD;
	PyObject *user;
	PyObject *message;
	PyObject *command;
	PyObject *time;
	PyObject *args;
};

static int PyTwitchChatMessage_init(PyTwitchChatMessage *self, PyObject *args, PyObject *kwds) {
	const char *user;
	const char *message;
	const char *command;
	double time;
	PyObject *py_args;

	if (!PyArg_ParseTuple(args, "sssOd", &user, &message, &command, &py_args, &time)) return -1;

	self->user = PyUnicode_FromString(user);
	self->message = PyUnicode_FromString(message);
	self->command = PyUnicode_FromString(command);
	self->time = PyFloat_FromDouble(time);
	self->args = py_args;
	Py_INCREF(self->args);

	return 0;
}

static PyMemberDef PyTwitchChatMessage_members[] = {
	{"user", T_OBJECT_EX, offsetof(PyTwitchChatMessage, user), 0, "User"},
	{"message", T_OBJECT_EX, offsetof(PyTwitchChatMessage, message), 0, "Message"},
	{"command", T_OBJECT_EX, offsetof(PyTwitchChatMessage, command), 0, "Command"},
	{"time", T_OBJECT_EX, offsetof(PyTwitchChatMessage, time), 0, "Time"},
	{"args", T_OBJECT_EX, offsetof(PyTwitchChatMessage, args), 0, "Arguments"},
	{nullptr}};

static PyTypeObject PyTwitchChatMessageType = {
	.ob_base = PyVarObject_HEAD_INIT(NULL, 0).tp_name = "chatnotifier.TwitchChatMessage",
	.tp_basicsize = sizeof(PyTwitchChatMessage),
	.tp_itemsize = 0,
	.tp_dealloc = reinterpret_cast<destructor>(PyObject_Del),
	.tp_flags = Py_TPFLAGS_DEFAULT,
	.tp_doc = "TwitchChatMessage object",
	.tp_members = PyTwitchChatMessage_members,
	.tp_init = reinterpret_cast<initproc>(PyTwitchChatMessage_init),
	.tp_new = PyType_GenericNew,
};

export auto PyCreateTwitchChatMessage(const TwitchChatMessage &msg) -> PyObject * {
	const auto p = PyObject_New(PyTwitchChatMessage, &PyTwitchChatMessageType);
	if (!p) return nullptr;

	p->user = PyUnicode_FromString(msg.user.c_str());
	p->message = PyUnicode_FromString(msg.message.c_str());
	p->command = PyUnicode_FromString(msg.command.c_str());

	const auto duration = msg.time.time_since_epoch();
	const double seconds =
		std::chrono::duration_cast<std::chrono::duration<double>>(duration).count();
	p->time = PyFloat_FromDouble(seconds);

	PyObject *py_dict = PyDict_New();
	for (const auto &[key, vec] : msg.args) {
		PyObject *py_key = PyUnicode_FromString(key.c_str());
		PyObject *py_list = PyList_New(vec.size());
		for (size_t i = 0; i < vec.size(); ++i)
			PyList_SetItem(py_list, i, PyUnicode_FromString(vec[i].c_str()));

		PyDict_SetItem(py_dict, py_key, py_list);
		Py_DECREF(py_key);
		Py_DECREF(py_list);
	}
	p->args = py_dict;

	return reinterpret_cast<PyObject *>(p);
}

/* Assets */
static PyObject *Py_Assets_getAssetsPath(PyObject *self, PyObject *args) {
	return PyUnicode_FromString(AssetsHandler::get_assets_path().string().c_str());
}

static PyObject *Py_Assets_getFontAssetsPath(PyObject *self, PyObject *args) {
	return PyUnicode_FromString(AssetsHandler::get_font_assets_path().string().c_str());
}

static PyObject *Py_Assets_getTTSAssetsPath(PyObject *self, PyObject *args) {
	return PyUnicode_FromString(AssetsHandler::get_tts_assets_path().string().c_str());
}

static PyObject *Py_Assets_getTriggerASCIIPath(PyObject *self, PyObject *args) {
	return PyUnicode_FromString(AssetsHandler::get_trigger_ascii_path().string().c_str());
}

static PyObject *Py_Assets_getTriggerSoundsPath(PyObject *self, PyObject *args) {
	return PyUnicode_FromString(AssetsHandler::get_trigger_sounds_path().string().c_str());
}

/* Audio */
static PyObject *Py_Audio_playOneshot(PyObject *self, PyObject *args) {
	const char *path;
	if (!PyArg_ParseTuple(args, "s", &path)) return nullptr;

	AudioPlayer::play_oneshot(std::filesystem::path(path));
	Py_RETURN_NONE;
}

static PyObject *Py_Audio_playOneshotMemory(PyObject *self, PyObject *args) {
	std::vector<float> data;
	PyObject *py_data;
	std::uint32_t samplerate, channels;
	if (!PyArg_ParseTuple(args, "Oii", &py_data, &samplerate, &channels)) return nullptr;

	if (PyList_Check(py_data)) {
		const size_t size = PyList_Size(py_data);
		data.reserve(size);
		for (size_t i = 0; i < size; ++i)
			data.push_back(PyFloat_AsDouble(PyList_GetItem(py_data, i)));
	} else if (PyByteArray_Check(py_data)) {
		const size_t size = PyByteArray_Size(py_data);
		data.reserve(size);
		for (size_t i = 0; i < size; ++i) data.push_back(PyByteArray_AS_STRING(py_data)[i]);
	} else {
		PyErr_SetString(PyExc_TypeError, "Data must be a list or a bytearray");
		return nullptr;
	}

	AudioPlayer::play_oneshot_memory(SoundData{data, samplerate, channels}, {});
	Py_RETURN_NONE;
}

/* Module */
static PyMethodDef scripting_py_methods[] = {
	{"get_assets_path", Py_Assets_getAssetsPath, METH_NOARGS, "Get the assets path."},
	{"get_font_assets_path", Py_Assets_getFontAssetsPath, METH_NOARGS, "Get the font assets path."},
	{"get_tts_assets_path", Py_Assets_getTTSAssetsPath, METH_NOARGS, "Get the TTS assets path."},
	{"get_ascii_assets_path", Py_Assets_getTriggerASCIIPath, METH_NOARGS,
	 "Get the trigger ASCII path."},
	{"get_sound_assets_path", Py_Assets_getTriggerSoundsPath, METH_NOARGS,
	 "Get the trigger sounds path."},
	{"play_oneshot_file", Py_Audio_playOneshot, METH_VARARGS, "Play a oneshot audio file."},
	{"play_oneshot_memory", Py_Audio_playOneshotMemory, METH_VARARGS,
	 "Play a oneshot memory audio data."},
	{nullptr, nullptr, 0, nullptr}};

static PyModuleDef scripting_py_module = {PyModuleDef_HEAD_INIT, "chatnotifier",
										  "ChatNotifier scripting module", -1,
										  scripting_py_methods};

export auto ChatNotifierModuleCreate() -> PyObject * {
	const auto module = PyModule_Create(&scripting_py_module);
	// Add types to the module
	if (PyType_Ready(&PyTwitchChatMessageType) < 0) return nullptr;
	Py_INCREF(&PyTwitchChatMessageType);
	if (PyModule_AddObject(module, "TwitchChatMessage",
						   reinterpret_cast<PyObject *>(&PyTwitchChatMessageType)) < 0)
		return nullptr;

	return module;
}
