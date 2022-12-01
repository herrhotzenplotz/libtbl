CPPFLAGS	+=	-I.
CFLAGS		+=	-fPIC

PREFIX	?=	/usr/local

all: libtbl.a libtbl.so.1 example euler

clean:
	rm -f libtbl.a libtbl.so* libtbl.o

libtbl.a: libtbl.o
	ar cr libtbl.a libtbl.o

libtbl.so.1: libtbl.o
	${CC} -shared -Xlinker --soname=libtbl.so.1 -o libtbl.so.1 libtbl.o

euler: libtbl.a euler.c
	${CC} -static -o euler euler.c libtbl.a -lm

example: libtbl.a example.c
	${CC} -static -o example example.c libtbl.a -lm

install:
	mkdir -p ${DESTDIR}${PREFIX}/include
	mkdir -p ${DESTDIR}${PREFIX}/lib
	mkdir -p ${DESTDIR}${PREFIX}/share/examples/libtbl
	mkdir -p ${DESTDIR}${PREFIX}/share/man/man3
	cp libtbl.h ${DESTDIR}${PREFIX}/include
	cp libtbl.a ${DESTDIR}${PREFIX}/lib
	cp libtbl.so.1 ${DESTDIR}${PREFIX}/lib/libtbl.so.1.0.0
	ln -fs ${DESTDIR}${PREFIX}/lib/libtbl.so.1.0.0 ${DESTDIR}${PREFIX}/lib/libtbl.so.1
	ln -fs ${DESTDIR}${PREFIX}/lib/libtbl.so.1 ${DESTDIR}${PREFIX}/lib/libtbl.so
	cp euler.c example.c ${DESTDIR}${PREFIX}/share/examples/libtbl/
	gzip < libtbl.3 > ${DESTDIR}${PREFIX}/share/man/man3/libtbl.3.gz
