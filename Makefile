CPPFLAGS	+=	-I.
CFLAGS		+=	-fPIC
all: libtbl.a libtbl.so.1

clean:
	rm -f libtbl.a libtbl.so* libtbl.o

libtbl.a: libtbl.o
	ar cr libtbl.a libtbl.o

libtbl.so.1: libtbl.o
	${CC} -shared -Xlinker --soname=libtbl.so.1 -o libtbl.so.1 libtbl.o

example: libtbl.a example.c
	${CC} -static -o example example.c libtbl.a
