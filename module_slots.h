#ifndef VERTCOINHASH_MODULE_SLOTS_H
#define VERTCOINHASH_MODULE_SLOTS_H

#include <Python.h>

/* The slots of every module here (multi-phase initialization, PEP 489). The modules keep no
 * state: their functions only use their arguments and locals, and already run without the
 * GIL while hashing. So each subinterpreter can load them with its own GIL, and free-threaded
 * CPython (3.13 and later) needn't enable the GIL for them. Each slot is only set where the
 * Python headers define it: neither is in the stable ABI of 3.11 that the abi3 wheels use. */
static PyModuleDef_Slot vertcoinhash_module_slots[] = {
#ifdef Py_mod_multiple_interpreters
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
#endif
#ifdef Py_mod_gil
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
#endif
    {0, NULL}
};

#endif
