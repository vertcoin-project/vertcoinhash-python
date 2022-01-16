#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include "scrypt.h"

static unsigned char getNfactor(char* blockheader) {
    int n,l = 0;
    unsigned long nTimestamp = *(unsigned int*)(&blockheader[68]);
    unsigned char minNfactor = 10;
    unsigned char maxNfactor = 30;
    unsigned char N;
    uint64_t s;

    if (nTimestamp <= 1389306217) {
        return minNfactor;
    }

    s = nTimestamp - 1389306217;
    while ((s >> 1) > 3) {
      l += 1;
      s >>= 1;
    }

    s &= 3;

    n = (l * 158 + s * 28 - 2670) / 100;

    if (n < 0) n = 0;

    N = (unsigned char) n;
    n = N > minNfactor ? N : minNfactor;
    N = n < maxNfactor ? n : maxNfactor;

    return N;
}

static PyObject *scrypt_getpowhash(PyObject *self, PyObject *args)
{
    char *output;
    PyObject *value;
#if PY_MAJOR_VERSION >= 3
    PyBytesObject *input;
#else
    PyStringObject *input;
#endif
    unsigned int N;
    if (!PyArg_ParseTuple(args, "S", &input))
        return NULL;
    Py_INCREF(input);
    output = PyMem_Malloc(32);
#if PY_MAJOR_VERSION >= 3
    N = 1 << (getNfactor((char *)PyBytes_AsString((PyObject*) input)) + 1);
    scrypt_N_1_1_256((char *)PyBytes_AsString((PyObject*) input), output, N);
#else
    N = 1 << (getNfactor((char *)PyString_AsString((PyObject*) input)) + 1);
    scrypt_N_1_1_256((char *)PyString_AsString((PyObject*) input), output, N);
#endif
    Py_DECREF(input);
    
#if PY_MAJOR_VERSION >= 3
    value = Py_BuildValue("y#", output, 32);
#else
    value = Py_BuildValue("s#", output, 32);
#endif
    PyMem_Free(output);
    return value;
}

static PyMethodDef ScryptMethods[] = {
    { "getPoWHash", scrypt_getpowhash, METH_VARARGS, "Returns the proof of work hash using scrypt" },
    { NULL, NULL, 0, NULL }
};

#if PY_MAJOR_VERSION >= 3
static struct PyModuleDef VTCScryptModule = {
    PyModuleDef_HEAD_INIT,
    "vtc_scrypt_new",
    "...",
    -1,
    ScryptMethods
};

PyMODINIT_FUNC PyInit_vtc_scrypt_new(void) {
    return PyModule_Create(&VTCScryptModule);
}
#else
PyMODINIT_FUNC initvtc_scrypt_new(void) {
    (void) Py_InitModule("vtc_scrypt_new", ScryptMethods);
}
#endif
