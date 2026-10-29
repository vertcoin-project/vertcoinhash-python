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

#define HEADER_SIZE 80

static PyObject *scrypt_getpowhash(PyObject *self, PyObject *args)
{
    Py_buffer input;
    char output[32];
    unsigned int N;

    if (!PyArg_ParseTuple(args, "y*", &input))
        return NULL;
    if (input.len != HEADER_SIZE) {
        PyBuffer_Release(&input);
        PyErr_SetString(PyExc_ValueError, "block header must be 80 bytes");
        return NULL;
    }
    Py_BEGIN_ALLOW_THREADS
    N = 1 << (getNfactor((char *)input.buf) + 1);
    scrypt_N_1_1_256((const char *)input.buf, output, N);
    Py_END_ALLOW_THREADS
    PyBuffer_Release(&input);
    return PyBytes_FromStringAndSize(output, 32);
}

static PyMethodDef ScryptMethods[] = {
    { "getPoWHash", scrypt_getpowhash, METH_VARARGS, "Returns the proof of work hash using scrypt" },
    { NULL, NULL, 0, NULL }
};

static struct PyModuleDef VTCScryptModule = {
    PyModuleDef_HEAD_INIT,
    "vtc_scrypt_new",
    "scrypt-N proof of work hash used by Vertcoin",
    -1,
    ScryptMethods
};

PyMODINIT_FUNC PyInit_vtc_scrypt_new(void) {
    return PyModule_Create(&VTCScryptModule);
}
