CC = gcc

MAJOR = 0
MINOR = 2

LIBNAME  = libnotlib.so
LIBMAJOR = ${LIBNAME}.${MAJOR}
LIBFULL  = ${LIBNAME}.${MAJOR}.${MINOR}

INCLUDE = notlib.h
HSRC    = _notlib_internal.h
CSRC    = dbus.c note.c queue.c notlib.c
OBJS    = dbus.o note.o queue.o notlib.o

DEPS     = gio-2.0 gobject-2.0 glib-2.0
LIBS     = $(shell pkg-config --libs ${DEPS})
INCLUDES = $(shell pkg-config --cflags ${DEPS})

LIBFLAGS = -Wl,-soname,${LIBMAJOR}
CFLAGS   = -fPIC -Wall -Werror -pthread -O2 ${LIBS} ${INCLUDES} ${DEFINES}

${LIBFULL} : ${OBJS} Makefile
	${CC} -shared -o $@ ${LIBFLAGS} ${OBJS}
	ln -s ${LIBFULL} ${LIBMAJOR}
	ln -s ${LIBMAJOR} ${LIBNAME}
	mkdir -p $(addprefix build/, include lib src)
	install -m 644 ${CSRC} ${HSRC} build/src
	install -m 644 ${INCLUDE} build/include
	mv -f ${LIBFULL} ${LIBMAJOR} ${LIBNAME} build/lib


static	: ${OBJS} Makefile
	ar rcs libnotlib.a ${OBJS}

install : ${LIBFULL}
	mkdir -p $(addprefix /usr/local/, src lib include)
	cp -r $(wildcard build/*) /usr/local


clean :
	rm -r ${OBJS} libnotlib.a build/


dbus.o      : dbus.c   notlib.h _notlib_internal.h
notlib.o    : notlib.c notlib.h _notlib_internal.h
note.o      : note.c   notlib.h _notlib_internal.h
queue.o     : queue.c  notlib.h _notlib_internal.h
