LIBNAME=libglthread.a
TARGET:${LIBNAME} exe
${LIBNAME}:glthread.o
	ar rs ${LIBNAME} glthread.o
exe:test.o ${LIBNAME}
	gcc -g test.o -o exe -L . -lglthread
glthread.o:glthread.c
	gcc -g -c -I . glthread.c -o glthread.o
test.o:test.c
	gcc -g -c -I . test.c -o test.o

clean:
	rm *.0
	rm exe
	rm ${LIBNAME}
