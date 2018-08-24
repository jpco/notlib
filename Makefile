# NOTE: `xxd` is a build-time dependency

CC = gcc

MAJOR = 0
MINOR = 2

LIBNAME  = libnotlib.so
LIBMAJOR = ${LIBNAME}.${MAJOR}
LIBFULL  = ${LIBNAME}.${MAJOR}.${MINOR}

INCLUDE = notlib.h
HSRC    = _notlib_internal.h
CSRC    = dbus.c note.c queue.c
GEN     = generated_xml.c

LINKFLAGS = -Wl,-soname,${LIBMAJOR}
CFLAGS    = -fPIC -Wall -Werror -pthread -O2

DEPS     = gio-2.0 gobject-2.0 glib-2.0
LIBS     = $(shell pkg-config --libs ${DEPS})
INCLUDES = $(shell pkg-config --cflags ${DEPS})


${LIBFULL} : ${CSRC} ${HSRC} ${INCLUDE} ${GEN} Makefile
	${CC} -shared -o $@ ${LINKFLAGS} ${LIBS} ${DEFINES} ${CSRC} ${GEN} ${CFLAGS} ${INCLUDES}
	ln -s ${LIBFULL} ${LIBMAJOR}
	ln -s ${LIBMAJOR} ${LIBNAME}
	mkdir -p $(addprefix build/, include lib src)
	install -m 644 ${CSRC} ${HSRC} introspection.xml build/src
	install -m 644 ${INCLUDE} build/include
	mv -f ${LIBFULL} ${LIBMAJOR} ${LIBNAME} build/lib


install : ${LIBFULL}
	mkdir -p $(addprefix /usr/local/, src lib include)
	cp -r $(wildcard build/*) /usr/local


clean :
	rm -r build/
	rm ${GEN}


# strange hacks here, in order to make sure the generated string is
# NUL-terminated
generated_xml.c	: introspection.xml
	cp introspection.xml dbus_introspection.xml
	echo -en '\0' >> dbus_introspection.xml
	xxd -i dbus_introspection.xml > generated_xml.c
	rm dbus_introspection.xml
