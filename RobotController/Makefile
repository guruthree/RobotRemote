# https://stackoverflow.com/questions/1079832/how-can-i-configure-my-makefile-for-debug-and-release-builds
# http://make.mad-scientist.net/papers/advanced-auto-dependency-generation/

CC = gcc
DEBUGGER = gdb
CFLAGS = $(shell sdl2-config --cflags) $(shell pkg-config --cflags glib-2.0)
LIBS = $(shell sdl2-config --libs) $(shell pkg-config --libs glib-2.0) -lSDL2_net

SRCS = robotcontroller.c
OBJS = $(SRCS:.c=.o)
EXE = robotcontroller

DBGDIR = debug
DBGEXE = $(DBGDIR)/$(EXE)
DBGOBJS = $(addprefix $(DBGDIR)/, $(OBJS))
DBGCFLAGS = -g -O0 -DDEBUG -Wall -Werror -Wextra

RELDIR = release
RELEXE = $(RELDIR)/$(EXE)
RELOBJS = $(addprefix $(RELDIR)/, $(OBJS))
RELCFLAGS = -O3 -DNDEBUG

DEPDIR = .d
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.Td
POSTCOMPILE = @mv -f $(DEPDIR)/$*.Td $(DEPDIR)/$*.d && touch $@

all: prep release

run: all
	$(RELDIR)/$(EXE)

debugrun: prep debug
	$(DEBUGGER) $(DBGDIR)/$(EXE)

debug: $(DBGEXE)

$(DBGEXE): $(DBGOBJS)
	$(CC) $(CFLAGS) $(DEPFLAGS) $(DBGCFLAGS) $(LIBS) -o $(DBGEXE) $^

$(DBGDIR)/%.o: %.c $(DEPDIR)/%.d
	$(CC) -c $(CFLAGS) $(DEPFLAGS) $(DBGCFLAGS) -o $@ $<
	$(POSTCOMPILE)


release: $(RELEXE)

$(RELEXE): $(RELOBJS)
	$(CC) $(CFLAGS) $(DEPFLAGS) $(RELCFLAGS) $(LIBS) -o $(RELEXE) $^

$(RELDIR)/%.o: %.c $(DEPDIR)/%.d
	$(CC) -c $(CFLAGS) $(DEPFLAGS) $(RELCFLAGS) -o $@ $<
	$(POSTCOMPILE)


prep:
	@mkdir -p $(DBGDIR) $(RELDIR) $(DEPDIR)


remake: clean all


clean:
	rm -f $(RELEXE) $(RELOBJS) $(DBGEXE) $(DBGOBJS) $(DEPDIR)/*


$(DEPDIR)/%.d: ;
.PRECIOUS: $(DEPDIR)/%.d

include $(wildcard $(patsubst %,$(DEPDIR)/%.d,$(basename $(SRCS))))
