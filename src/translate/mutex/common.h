#ifndef __MUTEX_COMMON_H__
#define __MUTEX_COMMON_H__

#include <Python.h>

struct _task_action_t {
    char *name;
    int *pre;
    int pre_size;
    int *add;
    int add_size;
    int *del;
    int del_size;
    int appl;
};
typedef struct _task_action_t task_action_t;

struct _task_t {
    int *fact;
    int fact_size;
    int *init_state;
    int init_state_size;
    task_action_t *action;
    int action_size;
};
typedef struct _task_t task_t;


int taskInit(task_t *task, PyObject *args);
void taskFree(task_t *task);

#endif /* __MUTEX_COMMON_H__ */
