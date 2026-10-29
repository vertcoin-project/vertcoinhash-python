#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include "Lyra2RE.h"

#define HEADER_SIZE 80

static PyObject *lyra2re2_hash_getpowhash(PyObject *self, PyObject *args)
{
    Py_buffer input;
    char output[32];

    if (!PyArg_ParseTuple(args, "y*", &input))
        return NULL;
    if (input.len != HEADER_SIZE) {
        PyBuffer_Release(&input);
        PyErr_SetString(PyExc_ValueError, "block header must be 80 bytes");
        return NULL;
    }
    Py_BEGIN_ALLOW_THREADS
    lyra2re2_hash((const char *)input.buf, output);
    Py_END_ALLOW_THREADS
    PyBuffer_Release(&input);
    return PyBytes_FromStringAndSize(output, 32);
}

static PyMethodDef Lyra2REv2Methods[] = {
    { "getPoWHash", lyra2re2_hash_getpowhash, METH_VARARGS, "Returns the proof of work hash using Lyra2REv2" },
    { NULL, NULL, 0, NULL }
};

static struct PyModuleDef Lyra2REv2Module = {
    PyModuleDef_HEAD_INIT,
    "lyra2re2_hash",
    "Lyra2REv2 proof of work hash used by Vertcoin",
    -1,
    Lyra2REv2Methods
};

PyMODINIT_FUNC PyInit_lyra2re2_hash(void) {
    return PyModule_Create(&Lyra2REv2Module);
}
