module;

#include <vector>
#include <filesystem>

#include <Python.h>

export module pythonmodule;

import assets;
import audio;

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
	return PyModule_Create(&scripting_py_module);
}
