#include <stdio.h>
#include <ilcplex/cplex.h>

#include "common.h"
#include "h2.h"

#define FACT(x, y, task) fact[(x) * (task)->fact_size + (y)]

static int isApplicable(task_action_t *act,
                        const int *fact, const task_t *task)
{
    int i, j;

    if (act->appl)
        return 1;

    for (i = 0; i < act->pre_size; ++i){
        for (j = i; j < act->pre_size; ++j){
            if (!FACT(act->pre[i], act->pre[j], task))
                return 0;
        }
    }

    act->appl = 1;
    return 1;
}

static int isIn(int fact, const int *fs, int size)
{
    int i;

    // TODO: binary search
    for (i = 0; i < size; ++i){
        if (fs[i] == fact)
            return 1;
    }
    return 0;
}

static int pre2Sat(const int *fact, int f, const task_action_t *act,
                   const task_t *task)
{
    int i;

    for (i = 0; i < act->pre_size; ++i){
        if (!FACT(f, act->pre[i], task))
            return 0;
    }
    return 1;
}

static int applyAction(const task_t *task, int *fact, task_action_t *act)
{
    int i, j, fact1, fact2;
    int updated = 0;

    if (!isApplicable(act, fact, task))
        return 0;

    for (i = 0; i < act->add_size; ++i){
        fact1 = act->add[i];
        for (j = i; j < act->add_size; ++j){
            fact2 = act->add[j];
            if (!FACT(fact1, fact2, task)){
                FACT(fact1, fact2, task) = FACT(fact2, fact1, task) = 1;
                updated = 1;
            }
        }

        for (j = 0; j < task->fact_size; ++j){
            if (!FACT(j, j, task) || FACT(fact1, j, task))
                continue;
            if (isIn(j, act->add, act->add_size)
                    || isIn(j, act->del, act->del_size))
                continue;
            if (pre2Sat(fact, j, act, task)){
                FACT(fact1, j, task) = FACT(j, fact1, task) = 1;
                updated = 1;
            }
        }
    }

    return updated;
}

PyObject *mutexH2(PyObject *self, PyObject *args, PyObject *kwds)
{
    task_t task;
    int *fact;
    int updated, i, j;
    PyObject *out, *h1, *h2, *list, *pynum;

    if (taskInit(&task, args) != 0)
        return NULL;

    fact = (int *)calloc(task.fact_size * task.fact_size, sizeof(int));

    for (i = 0; i < task.init_state_size; ++i){
        for (j = i; j < task.init_state_size; ++j){
            FACT(task.init_state[i], task.init_state[j], &task) = 1;
            FACT(task.init_state[j], task.init_state[i], &task) = 1;
        }
    }

    do {
        updated = 0;
        for (i = 0; i < task.action_size; ++i)
            updated |= applyAction(&task, fact, task.action + i);
    } while (updated);

    out = PyList_New(2);
    h1 = PyList_New(0);
    h2 = PyList_New(0);
    PyList_SetItem(out, 0, h2);
    PyList_SetItem(out, 1, h1);
    for (i = 0; i < task.fact_size; ++i){
        for (j = i; j < task.fact_size; ++j){
            if (!FACT(i, j, &task)){
                if (i == j){
                    pynum = PyInt_FromLong(i);
                    PyList_Append(h1, pynum);
                    Py_DECREF(pynum);
                }else{
                    list = PyList_New(2);
                    PyList_Append(h2, list);
                    pynum = PyInt_FromLong(i);
                    PyList_SetItem(list, 0, pynum);
                    pynum = PyInt_FromLong(j);
                    PyList_SetItem(list, 1, pynum);
                    Py_DECREF(list);
                }
            }
        }
    }

    free(fact);
    taskFree(&task);
    return out;
}
