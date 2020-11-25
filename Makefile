CC = gcc
TARGET=Memory
CFLAGS = -Wall -pedantic
RANLIB=ranlib
LIBSX = -lsx -L/usr/lib


LIBS = $(LIBSX) -lm -lXpm  -lXmu -lXt -lXext -lX11 -L/usr/lib

OBJS = main.o utils.o loadbmp.o libnsbmp.o

$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(LIBS)

main.o : main.c main.h utils.h loadbmp.h
utils.o : utils.c main.h
loadbmp.o: loadbmp.c libnsbmp.h
libnsbmp.o: libnsbmp.c

clean :
	rm -f *.o *~ core $(TARGET)
