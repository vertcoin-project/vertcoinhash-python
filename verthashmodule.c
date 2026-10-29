#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include "h2.h"

#define HEADER_SIZE 80
#define HASH_OUT_SIZE 32

/* getPoWHash(header, datfile): the datafile can be any bytes-like object,
 * e.g. bytes or a read-only mmap of verthash.dat, so it need not be read into memory. */
static PyObject *verthash_getpowhash(PyObject *self, PyObject *args)
{
    Py_buffer input, datfile;
    unsigned char output[HASH_OUT_SIZE];

    if (!PyArg_ParseTuple(args, "y*y*", &input, &datfile))
        return NULL;
    if (input.len != HEADER_SIZE || datfile.len < HASH_OUT_SIZE) {
        const char *msg = input.len != HEADER_SIZE
                          ? "block header must be 80 bytes" : "verthash datafile is too small";
        PyBuffer_Release(&input);
        PyBuffer_Release(&datfile);
        PyErr_SetString(PyExc_ValueError, msg);
        return NULL;
    }
    Py_BEGIN_ALLOW_THREADS
    verthash_hash((const unsigned char *)datfile.buf, (size_t)datfile.len,
                  (const unsigned char *)input.buf, output);
    Py_END_ALLOW_THREADS
    PyBuffer_Release(&input);
    PyBuffer_Release(&datfile);
    return PyBytes_FromStringAndSize((const char *)output, HASH_OUT_SIZE);
}

static PyMethodDef VerthashMethods[] = {
    { "getPoWHash", verthash_getpowhash, METH_VARARGS, "Returns the proof of work hash using Verthash" },
    { NULL, NULL, 0, NULL }
};

static struct PyModuleDef VerthashModule = {
    PyModuleDef_HEAD_INIT,
    "verthash",
    "Verthash proof of work hash used by Vertcoin",
    -1,
    VerthashMethods
};

PyMODINIT_FUNC PyInit_verthash(void) {
    return PyModule_Create(&VerthashModule);
}
