-include Makefile.local
-include Makefile.include

CPLEX_CFLAGS ?= -I/opt/cplex-1263/cplex/include
CPLEX_LDFLAGS ?= -L/opt/cplex-1263/cplex/bin/x86-64_linux -lcplex1263 -Wl,-rpath,/opt/cplex-1263/cplex/bin/x86-64_linux

CFLAGS += -fPIC
CFLAGS += -I.
CFLAGS += $(PYTHON_CFLAGS)
CFLAGS += $(CPLEX_CFLAGS)
LDFLAGS += $(PYTHON_LDFLAGS)
LDFLAGS += $(CPLEX_LDFLAGS)


all: py2
#echo "Specify target py2 or py3"

py2: mutexmodule.so

mutexmodule.so: mutex.c common.c fa.c h2.c
	$(CC) $(CFLAGS) -shared -o $@ $^ $(LDFLAGS)


clean:
	rm -f *.o
	rm -f *.so
	rm -f $(TARGETS)
	
.PHONY: all clean
