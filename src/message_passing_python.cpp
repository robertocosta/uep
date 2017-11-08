#include "message_passing.hpp"

#include <Python.h>
#include <structmember.h>

namespace uep { namespace mp {
/** Specialize the xor between bool symbols. A decoded symbol always
 *  remains decoded.
 */
template<>
void symbol_traits<bool>::inplace_xor(bool &lhs, const bool &rhs) {
  if (!lhs) lhs = rhs;
}
}}

using mp_ctx_t = uep::mp::mp_context<bool>;

extern "C" {

struct mp_ctx_py {
  PyObject_HEAD
  mp_ctx_t *mp_ctx;
};

static PyObject *mp_context_new(PyTypeObject *type,
				PyObject *args,
				PyObject *kwds) {
  mp_ctx_py *self;

  self = (mp_ctx_py*)type->tp_alloc(type, 0);
  if (self != NULL) {
    self->mp_ctx = NULL;
  }

  return (PyObject *)self;
}

static void mp_context_dealloc(mp_ctx_py *self) {
  if (self->mp_ctx) {
    delete self->mp_ctx;
  }
  Py_TYPE(self)->tp_free((PyObject*)self);
}

static int mp_context_init(mp_ctx_py *self,
			   PyObject *args, PyObject *kwds) {
  std::size_t in_size;
  if (!PyArg_ParseTuple(args, "n", &in_size)) {
    return -1;
  }

  if (self->mp_ctx) {
    delete self->mp_ctx;
  }
  self->mp_ctx = new mp_ctx_t(in_size);

  return 0;
}

static PyMemberDef mp_context_members[] = {
  {NULL}  /* Sentinel */
};

static PyObject *mp_context_hello(mp_ctx_py *self)
{
  return PyUnicode_FromString("Hello World");
}

struct sequence_iterator {
  explicit sequence_iterator(PyObject *seq, std::size_t start = 0) {
    _seq = seq;
    _next = start;
  }

  bool operator==(const sequence_iterator &other) const {
    return _seq == other._seq && _next == other._next;

  }

  bool operator!=(const sequence_iterator &other) const {
    return !(operator==(other));
  }

  std::size_t operator*() const {
    PyObject *n = PySequence_GetItem(_seq, _next);
    return PyLong_AsUnsignedLongLong(n);
  }

  sequence_iterator &operator++() {
    ++_next;
    return *this;
  }

  sequence_iterator operator++(int) {
    sequence_iterator tmp(*this);
    operator++();
    return tmp;
  }

private:
  PyObject *_seq;
  std::size_t _next;
};

static PyObject *mp_context_add_output(mp_ctx_py *self, PyObject *args) {
  PyObject *seq;
  if (!PyArg_ParseTuple(args, "O", &seq)) {
    return NULL;
  }

  if (!PySequence_Check(seq)) {
    return NULL;
  }

  self->mp_ctx->add_output(true,
			   sequence_iterator(seq),
			   sequence_iterator(seq, PySequence_Size(seq)));
  Py_RETURN_NONE;
}

static PyObject *mp_context_run(mp_ctx_py *self) {
  self->mp_ctx->run();
  Py_RETURN_NONE;
}

static PyObject *mp_context_reset(mp_ctx_py *self) {
  self->mp_ctx->reset();
  Py_RETURN_NONE;
}

static PyObject *mp_context_input_size(mp_ctx_py *self) {
  return PyLong_FromSize_t(self->mp_ctx->input_size());
}

static PyObject *mp_context_output_size(mp_ctx_py *self) {
  return PyLong_FromSize_t(self->mp_ctx->output_size());
}

static PyObject *mp_context_decoded_count(mp_ctx_py *self) {
  return PyLong_FromSize_t(self->mp_ctx->decoded_count());
}

static PyObject *mp_context_has_decoded(mp_ctx_py *self) {
  if (self->mp_ctx->has_decoded()) {
    Py_RETURN_TRUE;
  }
  else {
    Py_RETURN_FALSE;
  }
}

static PyObject *mp_context_run_duration(mp_ctx_py *self) {
  return PyFloat_FromDouble(self->mp_ctx->run_duration());
}

static PyObject *mp_context_input_symbols(mp_ctx_py *self) {
  const mp_ctx_t &mp_ctx = *(self->mp_ctx);
  PyObject *out = PyList_New(mp_ctx.input_size());
  std::size_t j = 0;
  for (auto i = mp_ctx.input_symbols_begin();
       i != mp_ctx.input_symbols_end();
       ++i) {
    PyList_SET_ITEM(out, j++, PyBool_FromLong(*i ? 1 : 0));
  }
  return out;
}

static PyMethodDef mp_context_methods[] = {
  {"hello", (PyCFunction)mp_context_hello, METH_NOARGS,
   "Return the hello world"
  },
  {"add_output", (PyCFunction)mp_context_add_output, METH_VARARGS,
   "Add an output symbol"
  },
  {"run", (PyCFunction)mp_context_run, METH_NOARGS,
   "Run"
  },
  {"reset", (PyCFunction)mp_context_reset, METH_NOARGS,
   "Reset"
  },
  {"input_size", (PyCFunction)mp_context_input_size, METH_NOARGS,
   "Input size"
  },
  {"output_size", (PyCFunction)mp_context_output_size, METH_NOARGS,
   "Output size"
  },
  {"decoded_count", (PyCFunction)mp_context_decoded_count, METH_NOARGS,
   "Decoded count"
  },
  {"has_decoded", (PyCFunction)mp_context_has_decoded, METH_NOARGS,
   "Has decoded"
  },
  {"run_duration", (PyCFunction)mp_context_run_duration, METH_NOARGS,
   "Run duration"
  },
  {"input_symbols", (PyCFunction)mp_context_input_symbols, METH_NOARGS,
   "Input symbols"
  },
  {NULL}  /* Sentinel */
};

static PyTypeObject mp_ctx_py_type = {
  PyVarObject_HEAD_INIT(NULL, 0)
  "mppy.mp_context",                /* tp_name */
  sizeof(mp_ctx_py),                /* tp_basicsize */
  0,                                /* tp_itemsize */
  (destructor) mp_context_dealloc,  /* tp_dealloc */
  0,                                /* tp_print */
  0,                                /* tp_getattr */
  0,                         /* tp_setattr */
  0,                         /* tp_reserved */
  0,                         /* tp_repr */
  0,                         /* tp_as_number */
  0,                         /* tp_as_sequence */
  0,                         /* tp_as_mapping */
  0,                         /* tp_hash  */
  0,                         /* tp_call */
  0,                         /* tp_str */
  0,                         /* tp_getattro */
  0,                         /* tp_setattro */
  0,                         /* tp_as_buffer */
  Py_TPFLAGS_DEFAULT,        /* tp_flags */
  "mp_context objects",           /* tp_doc */
  0,                         /* tp_traverse */
  0,                         /* tp_clear */
  0,                         /* tp_richcompare */
  0,                         /* tp_weaklistoffset */
  0,                         /* tp_iter */
  0,                         /* tp_iternext */
  mp_context_methods,             /* tp_methods */
  mp_context_members,             /* tp_members */
  0,                         /* tp_getset */
  0,                         /* tp_base */
  0,                         /* tp_dict */
  0,                         /* tp_descr_get */
  0,                         /* tp_descr_set */
  0,                         /* tp_dictoffset */
  (initproc)mp_context_init,      /* tp_init */
  0,                         /* tp_alloc */
  mp_context_new,                 /* tp_new */
};

static PyModuleDef mppymodule = {
    PyModuleDef_HEAD_INIT,
    "mppy",
    "Module that uses the C++ message passing implementation.",
    -1,
    NULL, NULL, NULL, NULL, NULL
};

PyMODINIT_FUNC
PyInit_mppy(void)
{
    PyObject* m;

    if (PyType_Ready(&mp_ctx_py_type) < 0)
	return NULL;

    m = PyModule_Create(&mppymodule);
    if (m == NULL)
	return NULL;

    Py_INCREF(&mp_ctx_py_type);
    PyModule_AddObject(m, "mp_context", (PyObject *)&mp_ctx_py_type);
    return m;
}

}
