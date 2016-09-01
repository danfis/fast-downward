#include <stdio.h>
#include <ilcplex/cplex.h>
#include <boruvka/alloc.h>

#include "common.h"
#include "fa.h"

static void cplexErr(CPXENVptr env, int status, const char *s)
{
    char errmsg[1024];
    CPXgeterrorstring(env, status, errmsg);
    fprintf(stderr, "Error: CPLEX: %s: %s\n", s, errmsg);
    exit(-1);
}

static void lpSetCoef(CPXENVptr env, CPXLPptr lp, int row, int col, double coef)
{
    int st;

    st = CPXchgcoef(env, lp, row, col, coef);
    if (st != 0)
        cplexErr(env, st, "Could not set constraint coeficient.");
}

static void lpSetRHS(CPXENVptr env, CPXLPptr lp, int row, double rhs, char sense)
{
    int st;

    st = CPXchgcoef(env, lp, row, -1, rhs);
    if (st != 0)
        cplexErr(env, st, "Could not set right-hand-side.");

    st = CPXchgsense(env, lp, 1, &row, &sense);
    if (st != 0)
        cplexErr(env, st, "Could not set right-hand-side sense.");
}

static double lpSolve(CPXENVptr env, CPXLPptr lp, double *obj)
{
    int st, solst;
    double ov;

    st = CPXmipopt(env, lp);
    if (st != 0)
        cplexErr(env, st, "Failed to optimize LP");

    st = CPXsolution(env, lp, &solst, &ov, obj, NULL, NULL, NULL);
    if (st != 0)
        return -1.;
    return ov;
}

PyObject *mutexFA(PyObject *self, PyObject *args, PyObject *kwds)
{
    task_t task;
    CPXENVptr env;
    CPXLPptr lp;
    int st, i, j, k, row;
    char var_type = CPX_BINARY;
    const task_action_t *act;
    double *sol, objval;
    PyObject *out, *list, *pynum;

    if (taskInit(&task, args) != 0)
        return NULL;

    env = CPXopenCPLEX(&st);
    if (env == NULL)
        cplexErr(env, st, "Could not open CPLEX environment");

    st = CPXsetintparam(env, CPX_PARAM_THREADS, 1);
    if (st != 0)
        cplexErr(env, st, "Could not set number of threads");

    lp = CPXcreateprob(env, &st, "");
    if (lp == NULL)
        cplexErr(env, st, "Could not create CPLEX problem");

    st = CPXnewcols(env, lp, task.fact_size, NULL, NULL, NULL, NULL, NULL);
    if (st != 0)
        cplexErr(env, st, "Could not initialize variables");

    st = CPXnewrows(env, lp, 1 + task.action_size, NULL, NULL, NULL, NULL);
    if (st != 0)
        cplexErr(env, st, "Could not initialize constraints");

    // Set objective
    CPXchgobjsen(env, lp, CPX_MAX);
    for (i = 0; i < task.fact_size; ++i){
        st = CPXchgcoef(env, lp, -1, i, 1.);
        if (st != 0)
            cplexErr(env, st, "Could not set objective coeficient.");

        st = CPXchgctype(env, lp, 1, &i, &var_type);
        if (st != 0)
            cplexErr(env, st, "Could not set variable as integer.");
    }

    // Initial state constraint
    lpSetRHS(env, lp, 0, 1., 'L');
    for (i = 0; i < task.init_state_size; ++i)
        lpSetCoef(env, lp, 0, task.init_state[i], 1.);

    // Action constraints
    for (i = 0; i < task.action_size; ++i){
        act = task.action + i;
        lpSetRHS(env, lp, i + 1, 0., 'L');
        for (j = 0; j < act->add_size; ++j)
            lpSetCoef(env, lp, i + 1, act->add[j], 1.);

        for (j = 0, k = 0; j < act->pre_size && k < act->del_size;){
            if (act->pre[j] == act->del[k]){
                lpSetCoef(env, lp, i + 1, act->pre[j], -1.);
                ++j;
                ++k;
            }else if (act->pre[j] < act->del[k]){
                ++j;
            }else{
                ++k;
            }
        }
    }

    //CPXwriteprob(env, lp, "out.lp", "LP");

    out = PyList_New(0);
    sol = BOR_ALLOC_ARR(double, task.fact_size);
    row = 1 + task.action_size;
    while ((objval = lpSolve(env, lp, sol)) > 1.5){
        // Remove current mutex from future solutions
        st = CPXnewrows(env, lp, 1, NULL, NULL, NULL, NULL);
        if (st != 0)
            cplexErr(env, st, "Could not add new rows.");

        lpSetRHS(env, lp, row, 1., 'G');
        for (i = 0; i < task.fact_size; ++i){
            if (sol[i] < 0.5)
                lpSetCoef(env, lp, row, i, 1.);
        }
        ++row;

        list = PyList_New(0);
        PyList_Append(out, list);
        for (i = 0; i < task.fact_size; ++i){
            if (sol[i] > 0.5){
                pynum = PyInt_FromLong(i);
                PyList_Append(list, pynum);
                Py_DECREF(pynum);
            }
        }
        Py_DECREF(list);
    }

    BOR_FREE(sol);
    CPXfreeprob(env, &lp);
    CPXcloseCPLEX(&env);
    taskFree(&task);
    return out;
}
