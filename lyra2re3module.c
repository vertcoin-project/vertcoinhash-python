#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "module_slots.h"

#include "Lyra2RE.h"

#define HEADER_SIZE 80

static PyObject *lyra2re3_hash_getpowhash(PyObject *self, PyObject *args)
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
    lyra2re3_hash((const char *)input.buf, output);
    Py_END_ALLOW_THREADS
    PyBuffer_Release(&input);
    return PyBytes_FromStringAndSize(output, 32);
}

static PyMethodDef Lyra2REv3Methods[] = {
    { "getPoWHash", lyra2re3_hash_getpowhash, METH_VARARGS, "Returns the proof of work hash using Lyra2REv3" },
    { NULL, NULL, 0, NULL }
};

static struct PyModuleDef Lyra2REv3Module = {
    PyModuleDef_HEAD_INIT,
    "lyra2re3_hash",
    "Lyra2REv3 proof of work hash used by Vertcoin",
    0,
    Lyra2REv3Methods,
    vertcoinhash_module_slots
};

PyMODINIT_FUNC PyInit_lyra2re3_hash(void) {
    return PyModuleDef_Init(&Lyra2REv3Module);
}
