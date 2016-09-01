#include "common.h"

static int parseIntArr(PyObject *list, int **arr, int *arr_size)
{
    int i;
    PyObject *iter, *item;

    *arr_size = PyObject_Size(list);
    if (*arr_size == 0){
        *arr = NULL;
        return 0;
    }

    *arr = (int *)malloc(sizeof(int) * *arr_size);

    iter = PyObject_GetIter(list);
    for (i = 0; (item = PyIter_Next(iter)) != NULL; ++i){
        (*arr)[i] = PyInt_AsLong(item);
        Py_DECREF(item);
    }
    Py_DECREF(iter);

    return 0;
}

static int _taskInitFacts(task_t *task, PyObject *atoms)
{
    return parseIntArr(atoms, &task->fact, &task->fact_size);
}

int _taskInitInitState(task_t *task, PyObject *atoms)
{
    return parseIntArr(atoms, &task->init_state, &task->init_state_size);
}

static int _taskInitAction(const task_t *task, task_action_t *action,
                           PyObject *pyaction)
{
    PyObject *name, *pre, *add, *del;

    name = PyObject_GetAttrString(pyaction, "name");
    action->name = strdup(PyString_AsString(name));
    Py_DECREF(name);

    pre = PyObject_GetAttrString(pyaction, "pre");
    parseIntArr(pre, &action->pre, &action->pre_size);
    Py_DECREF(pre);

    add = PyObject_GetAttrString(pyaction, "add_eff");
    parseIntArr(add, &action->add, &action->add_size);
    Py_DECREF(add);

    del = PyObject_GetAttrString(pyaction, "del_eff");
    parseIntArr(del, &action->del, &action->del_size);
    Py_DECREF(del);
    return 0;
}

static int _taskInitActions(task_t *task, PyObject *pyactions)
{
    int size;
    PyObject *iter, *item;

    size = PyObject_Size(pyactions);
    task->action = (task_action_t *)calloc(size, sizeof(task_action_t));
    task->action_size = 0;

    iter = PyObject_GetIter(pyactions);
    while ((item = PyIter_Next(iter)) != NULL){
        _taskInitAction(task, task->action + task->action_size, item);
        ++task->action_size;
        Py_DECREF(item);
    }

    Py_DECREF(iter);
    return 0;
}

int taskInit(task_t *task, PyObject *args)
{
    PyObject *pytask, *pyatoms, *pyactions;

    bzero(task, sizeof(*task));

    if (!PyArg_ParseTuple(args, "OOO", &pytask, &pyatoms, &pyactions)){
        PyErr_SetString(PyExc_RuntimeError, "Invalid arguments");
        return -1;
    }

    _taskInitFacts(task, pyatoms);
    _taskInitInitState(task, pytask);
    _taskInitActions(task, pyactions);

    //Py_DECREF(pytask);
    //Py_DECREF(pyatoms);
    //Py_DECREF(pyactions);

    return 0;
}

void taskFree(task_t *task)
{
    int i;

    free(task->fact);
    free(task->init_state);

    for (i = 0; i < task->action_size; ++i){
        free(task->action[i].name);
        free(task->action[i].pre);
        free(task->action[i].add);
        free(task->action[i].del);
    }
    free(task->action);
}
