#include "rng.hpp"

#include <iostream>

#include <boost/python.hpp>
#include <Python.h>

using namespace std;

void test() {
  fountain f(10);
  for (int i = 0; i < 100; i++) {
    fountain::row_type ps = f.next_row();
    for (auto j = ps.begin(); j != ps.end(); j++) {
      cout << *j << ' ';
    }
    cout << endl;
  }
}

PyObject *py_test(PyObject *, PyObject *) {
  test();
  Py_INCREF(Py_None);
  return Py_None;
}

static PyMethodDef test_modules[] = {
  {"test", py_test, METH_NOARGS, "Test func"}
};

static PyModuleDef TestModule = {PyModuleDef_HEAD_INIT,
				 "rngtest",
				 NULL, -1, test_modules,
    NULL, NULL, NULL, NULL};

PyObject *init_test_module() {
  return PyModule_Create(&TestModule);
}

int main(int, char**) {
  PyImport_AppendInittab("rngtest", &init_test_module);
  Py_Initialize();

  PyObject *module_name = PyUnicode_FromString("rngtest");
  PyObject *py_test_mod = PyImport_Import(module_name);
  Py_DECREF(module_name);

  PyObject *test_func = PyObject_GetAttrString(py_test_mod, "test");
  PyObject_CallObject(test_func, NULL);

  Py_DECREF(test_func);
  Py_DECREF(module_name);

  Py_Finalize();
  
  return 0;
}
