#include <Python.h>
#include "wowmem.h"

class PyObjectManager_Wrapper {
    private:
        ObjectManager OM;
        ObjectManager::iterator current;
    public:
        PyObject* __iter__() {
            Py_INCREF(this);
            this->current = OM.begin();
            return reinterpret_cast<PyObject*>(&(*this->current));
        }

        PyObject* __next__() {
            ++this->current;
            if ((*this->current).valid()) {
                return reinterpret_cast<PyObject*>(&(*this->current));
            } else {
                PyErr_SetNone(PyExc_StopIteration);
                return nullptr;
            }
        }
};

static PyMethodDef ObjectManager_methods[] = {
    {NULL, NULL, 0, NULL}
};


static PyTypeObject PyObjectManager = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "wowmem.ObjectManager",          // tp_name
    sizeof(PyObjectManager_Wrapper),              // tp_basicsize
    0,                      // tp_itemsize
    NULL,                   // tp_dealloc
    NULL,                   // tp_print
    NULL,                   // tp_getattr
    NULL,                   // tp_setattr
    NULL,                   // tp_reserved
    NULL,                   // tp_repr
    NULL,                   // tp_as_number
    NULL,                   // tp_as_sequence
    NULL,                   // tp_as_mapping
    NULL,                   // tp_hash
    NULL,                   // tp_call
    NULL,                   // tp_str
    NULL,                   // tp_getattro
    NULL,                   // tp_setattro
    NULL,                   // tp_as_buffer
    Py_TPFLAGS_DEFAULT,     // tp_flags
    "C++ ObjectManager",             // tp_doc
    NULL,                   // tp_traverse
    NULL,                   // tp_clear
    NULL,                   // tp_richcompare
    0,                      // tp_weaklistoffset
    NULL,                   // tp_iter
    NULL,                   // tp_iternext
    ObjectManager_methods,              // tp_methods
    NULL,                   // tp_members
    NULL,                   // tp_getset
    NULL,                   // tp_base
    NULL,                   // tp_dict
    NULL,                   // tp_descr_get
    NULL,                   // tp_descr_set
    0,                      // tp_dictoffset
    NULL,                   // tp_init
    NULL,                   // tp_alloc
    NULL,                   // tp_new
};

static PyObject* get_xyzr(PyObject* self, PyObject* args) {
    auto w = reinterpret_cast<WowObject*>(self);
    auto [x,y,z,r] = w->get_xyzr();
    return Py_BuildValue("ffff", x, y, z, r);
}

static PyObject* get_name(PyObject* self, PyObject* args) {
    auto w = reinterpret_cast<WowObject*>(self);
    auto name = w->get_name();
    return PyUnicode_DecodeUTF8(name, strnlen_s(name, 64), "strict");
}

static PyMethodDef WowObject_methods[] = {
    {"get_xyzr", get_xyzr, METH_NOARGS, "Get xyzr"},
    {"get_name", get_name, METH_NOARGS, "Get name"},
    {NULL, NULL, 0, NULL}
};

static PyTypeObject PyWowObject = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "wowmem.WowObject",          // tp_name
    sizeof(WowObject),              // tp_basicsize
    0,                      // tp_itemsize
    NULL,                   // tp_dealloc
    NULL,                   // tp_print
    NULL,                   // tp_getattr
    NULL,                   // tp_setattr
    NULL,                   // tp_reserved
    NULL,                   // tp_repr
    NULL,                   // tp_as_number
    NULL,                   // tp_as_sequence
    NULL,                   // tp_as_mapping
    NULL,                   // tp_hash
    NULL,                   // tp_call
    NULL,                   // tp_str
    NULL,                   // tp_getattro
    NULL,                   // tp_setattro
    NULL,                   // tp_as_buffer
    Py_TPFLAGS_DEFAULT,     // tp_flags
    "C++ WowObject",             // tp_doc
    NULL,                   // tp_traverse
    NULL,                   // tp_clear
    NULL,                   // tp_richcompare
    0,                      // tp_weaklistoffset
    NULL,                   // tp_iter
    NULL,                   // tp_iternext
    WowObject_methods,              // tp_methods
    NULL,                   // tp_members
    NULL,                   // tp_getset
    NULL,                   // tp_base
    NULL,                   // tp_dict
    NULL,                   // tp_descr_get
    NULL,                   // tp_descr_set
    0,                      // tp_dictoffset
    NULL,                   // tp_init
    NULL,                   // tp_alloc
    NULL,                   // tp_new
};

// Module initialization
static struct PyModuleDef my_module = {
    PyModuleDef_HEAD_INIT,
    "wowmem",
    NULL,
    -1,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

#define REGISTER_PY_TYPE(mod, t, name) { if (PyType_Ready(&t) < 0) { return NULL; } Py_INCREF(&t); PyModule_AddObject(mod, name, (PyObject*)&t); }

// Module initialization function
PyMODINIT_FUNC PyInit_my_module() {
    PyObject* module = PyModule_Create(&my_module);
    if (module == NULL)
        return NULL;
    
    REGISTER_PY_TYPE(module, PyObjectManager, "ObjectManager");
    REGISTER_PY_TYPE(module, PyWowObject, "WowObject");

    return module;
}