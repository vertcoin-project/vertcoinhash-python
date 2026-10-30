#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "module_slots.h"

#include <errno.h>

#include "h2.h"
#include "verthash_datfile.h"

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

/* state for progress_cb, which create_datafile() calls with the GIL taken back */
struct progress_ctx {
    PyObject *cb;
    PyThreadState *save;
    int failed;
};

static int call_progress(void *arg, double fraction)
{
    struct progress_ctx *ctx = (struct progress_ctx *)arg;
    PyObject *res;

    PyEval_RestoreThread(ctx->save);
    res = PyObject_CallFunction(ctx->cb, "d", fraction);
    if (res == NULL)
        ctx->failed = 1; /* stops the generation; the exception is raised when it returns */
    Py_XDECREF(res);
    ctx->save = PyEval_SaveThread();
    return ctx->failed;
}

/* create_datafile(path, progress_cb=None): writes verthash.dat to path + '.tmp'
 * without the GIL, then renames it to path */
static PyObject *verthash_create_datafile_py(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = {"path", "progress_cb", NULL};
    PyObject *path, *cb = Py_None;
    struct progress_ctx ctx;
    int err;
#ifdef _WIN32
    wchar_t *cpath;
#else
    PyObject *bpath;
    const char *cpath;
#endif

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|O", kwlist, &path, &cb))
        return NULL;
    if (cb != Py_None && !PyCallable_Check(cb)) {
        PyErr_SetString(PyExc_TypeError, "progress_cb must be callable");
        return NULL;
    }
#ifdef _WIN32
    {
        PyObject *fspath = PyOS_FSPath(path);
        if (fspath == NULL)
            return NULL;
        if (!PyUnicode_Check(fspath)) {
            Py_DECREF(fspath);
            PyErr_SetString(PyExc_TypeError, "path must be a str or os.PathLike of str");
            return NULL;
        }
        cpath = PyUnicode_AsWideCharString(fspath, NULL);
        Py_DECREF(fspath);
        if (cpath == NULL)
            return NULL;
    }
#else
    if (!PyUnicode_FSConverter(path, &bpath))
        return NULL;
    cpath = PyBytes_AsString(bpath);
#endif
    ctx.cb = cb;
    ctx.failed = 0;
    ctx.save = PyEval_SaveThread();
    err = verthash_create_datafile(cpath, cb != Py_None ? call_progress : NULL, &ctx);
    PyEval_RestoreThread(ctx.save);
#ifdef _WIN32
    PyMem_Free(cpath);
#else
    Py_DECREF(bpath);
#endif
    if (ctx.failed)
        return NULL;
    if (err) {
#ifdef _WIN32
        return PyErr_SetExcFromWindowsErrWithFilenameObject(PyExc_OSError, err, path);
#else
        errno = err;
        return PyErr_SetFromErrnoWithFilenameObject(PyExc_OSError, path);
#endif
    }
    Py_RETURN_NONE;
}

static PyMethodDef VerthashMethods[] = {
    { "getPoWHash", verthash_getpowhash, METH_VARARGS, "Returns the proof of work hash using Verthash" },
    { "create_datafile", (PyCFunction)(void (*)(void))verthash_create_datafile_py, METH_VARARGS | METH_KEYWORDS,
      "create_datafile(path, progress_cb=None): writes the Verthash datafile (1.2 GiB, sha256 a55531e8...) "
      "to path + '.tmp' through a memory map, then renames it to path. The GIL is released meanwhile, "
      "except to call progress_cb(fraction) about 40 times; an exception from it stops the generation." },
    { NULL, NULL, 0, NULL }
};

static struct PyModuleDef VerthashModule = {
    PyModuleDef_HEAD_INIT,
    "verthash",
    "Verthash proof of work hash used by Vertcoin",
    0,
    VerthashMethods,
    vertcoinhash_module_slots
};

PyMODINIT_FUNC PyInit_verthash(void) {
    return PyModuleDef_Init(&VerthashModule);
}
