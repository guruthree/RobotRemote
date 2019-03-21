# http://www.cs.colby.edu/maxwell/courses/tutorials/maketutor/

CC=gcc
CFLAGS=
LIBS=-lSDL
OBJ=robotcontroller.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

robotcontroller: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

clean:
	rm -f *.o robotcontroller
