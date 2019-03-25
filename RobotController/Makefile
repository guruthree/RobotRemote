# https://stackoverflow.com/questions/1079832/how-can-i-configure-my-makefile-for-debug-and-release-builds

CC=gcc
DEBUGGER=gdb
CFLAGS=$(shell sdl2-config --cflags)
LIBS=$(shell sdl2-config --libs) -lSDL2_net

SRCS = robotcontroller.c
OBJS = $(SRCS:.c=.o)
EXE=robotcontroller

DBGDIR = debug
DBGEXE = $(DBGDIR)/$(EXE)
DBGOBJS = $(addprefix $(DBGDIR)/, $(OBJS))
DBGCFLAGS = -g -O0 -DDEBUG -Wall -Werror -Wextra

RELDIR = release
RELEXE = $(RELDIR)/$(EXE)
RELOBJS = $(addprefix $(RELDIR)/, $(OBJS))
RELCFLAGS = -O3 -DNDEBUG

all: prep release

run: all
	$(RELDIR)/$(EXE)

debugrun: prep debug
	$(DEBUGGER) $(DBGDIR)/$(EXE)

debug: $(DBGEXE)

$(DBGEXE): $(DBGOBJS)
	$(CC) $(CFLAGS) $(DBGCFLAGS) $(LIBS) -o $(DBGEXE) $^

$(DBGDIR)/%.o: %.c
	$(CC) -c $(CFLAGS) $(DBGCFLAGS) -o $@ $<


release: $(RELEXE)

$(RELEXE): $(RELOBJS)
	$(CC) $(CFLAGS) $(RELCFLAGS) $(LIBS) -o $(RELEXE) $^

$(RELDIR)/%.o: %.c
	$(CC) -c $(CFLAGS) $(RELCFLAGS) -o $@ $<


prep:
	@mkdir -p $(DBGDIR) $(RELDIR)


remake: clean all


clean:
	rm -f $(RELEXE) $(RELOBJS) $(DBGEXE) $(DBGOBJS)
