#include <Python.h>
#include "common.h"
#include "fa.h"
#include "h2.h"

#define ERR PyErr_SetString(PyExc_RuntimeError, nn_strerror(errno))

#if 0
typedef struct {
    PyObject_HEAD
    int sock;
    int endpoint;
} mutex_t;

static void sockDealloc(sock_t *sock)
{
}

static PyObject *sockNew(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    sock_t *self;
    self = (sock_t *)type->tp_alloc(type, 0);
    self->sock = -1;
    self->endpoint = -1;
    return (PyObject *)self;
}

static int sockInit(sock_t *self, PyObject *args, PyObject *kwds)
{
    int domain, protocol;

    if (!PyArg_ParseTuple(args, "ii", &domain, &protocol))
        return -1;
    self->sock = nn_socket(domain, protocol);
    if (self->sock == -1){
        ERR;
        return -1;
    }
    return 0;
}

static PyObject *sockBind(sock_t *self, PyObject *address)
{
    const char *addr;

    addr = PyString_AS_STRING(address);
    if (addr == NULL)
        return NULL;

    self->endpoint = nn_bind(self->sock, addr);
    if (self->endpoint == -1){
        ERR;
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject *sockShutdown(sock_t *self)
{
    if (self->sock == -1 || self->endpoint == -1)
        return NULL;
    if (nn_shutdown(self->sock, self->endpoint) != 0){
        ERR;
        return NULL;
    }
    self->endpoint = -1;
    Py_RETURN_NONE;
}

static PyObject *sockClose(sock_t *self)
{
    if (self->sock == -1)
        return NULL;
    if (nn_close(self->sock) != 0){
        ERR;
        return NULL;
    }
    self->sock = self->endpoint = -1;
    Py_RETURN_NONE;
}

static PyObject *sockConnect(sock_t *self, PyObject *address)
{
    const char *addr;

    addr = PyString_AS_STRING(address);
    if (addr == NULL)
        return NULL;

    self->endpoint = nn_connect(self->sock, addr);
    if (self->endpoint == -1){
        ERR;
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject *sockSend(sock_t *self, PyObject *args)
{
    int flags = 0;
    char *buf;
    int buflen;

    if (!PyArg_ParseTuple(args, "t#|i", &buf, &buflen, &flags))
        return NULL;

    if (nn_send(self->sock, buf, buflen, flags) == -1){
        ERR;
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject *sockRecv(sock_t *self, PyObject *args)
{
    int flags = 0;
    void *buf;
    int buflen;
    PyObject *bbuf, *sbuf;

    if (!PyArg_ParseTuple(args, "|i", &flags))
        return NULL;
    buflen = nn_recv(self->sock, &buf, NN_MSG, flags);
    if (buflen >= 0){
        sbuf = PyString_FromStringAndSize(buf, buflen);
        bbuf = PyBuffer_FromObject(sbuf, 0, Py_END_OF_BUFFER);
        Py_DECREF(sbuf);
        nn_freemsg(buf);
        return bbuf;
    }else{
        ERR;
        return NULL;
    }
}

static PyObject *sockSetInt(sock_t *self, PyObject *args, int option)
{
    int timeout = -1;
    int ret;

    if (!PyArg_ParseTuple(args, "i", &timeout))
        return NULL;
    ret = nn_setsockopt(self->sock, NN_SOL_SOCKET, option,
                        (const void *)&timeout, sizeof(int));
    if (ret == -1){
        ERR;
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject *sockSetRecvTimeout(sock_t *self, PyObject *args)
{
    return sockSetInt(self, args, NN_RCVTIMEO);
}

static PyObject *sockSetSendTimeout(sock_t *self, PyObject *args)
{
    return sockSetInt(self, args, NN_SNDTIMEO);
}

static PyObject *sockSetLinger(sock_t *self, PyObject *args)
{
    return sockSetInt(self, args, NN_LINGER);
}

static PyMethodDef sock_methods[] = {
    {"bind", (PyCFunction)sockBind, METH_O, "nn_bind(3)" },
    {"shutdown", (PyCFunction)sockShutdown, METH_NOARGS, "nn_shutdown(3)" },
    {"close", (PyCFunction)sockClose, METH_NOARGS, "nn_close(3)" },
    {"connect", (PyCFunction)sockConnect, METH_O, "nn_connect(3)" },
    {"send", (PyCFunction)sockSend, METH_VARARGS, "nn_send(3)" },
    {"recv", (PyCFunction)sockRecv, METH_VARARGS, "nn_recv(3)" },
    {"setRecvTimeout", (PyCFunction)sockSetRecvTimeout, METH_VARARGS,
        "nn_setsockopt(3) -- NN_RCVTIMEO" },
    {"setSendTimeout", (PyCFunction)sockSetSendTimeout, METH_VARARGS,
        "nn_setsockopt(3) -- NN_SNDTIMEO" },
    {"setLinger", (PyCFunction)sockSetLinger, METH_VARARGS,
        "nn_setsockopt(3) -- NN_LINGER" },
    {NULL}  /* Sentinel */
};

static PyTypeObject SocketType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "nanomsg2.Socket",             /*tp_name*/
    sizeof(sock_t),             /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)sockDealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    "",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    sock_methods,             /* tp_methods */
    0,             /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)sockInit,      /* tp_init */
    0,                         /* tp_alloc */
    sockNew,                 /* tp_new */
};


#define CONST(m, name) \
    PyModule_AddIntConstant(m, #name, name)
#endif

static PyMethodDef methods[] = {
    {"fa", (PyCFunction)mutexFA, METH_VARARGS, "fact-alternating mutexes" },
    {"h2", (PyCFunction)mutexH2, METH_VARARGS, "h^2 mutexes" },
    { NULL, NULL, 0, NULL },
};


#ifndef PyMODINIT_FUNC	/* declarations for DLL import/export */
#define PyMODINIT_FUNC void
#endif
PyMODINIT_FUNC
initmutex(void) 
{
    /*
    SocketType.tp_new = PyType_GenericNew;
    if (PyType_Ready(&SocketType) < 0)
        return;
    */

    Py_InitModule3("mutex", methods, "TODO");


    /*
    Py_INCREF(&SocketType);
    PyModule_AddObject(m, "Socket", (PyObject *)&SocketType);
    */
}

