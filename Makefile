YACC = bison -y -d
LEX = flex
CC = gcc
TARGET = bin/streem
CDEFS =
CFLAGS = -g -Wall $(CDEFS)
LIBS = -lpthread

ifeq (Windows_NT,$(OS))
TARGET:=$(TARGET).exe
LIBS += -lws2_32
endif

TESTS=$(wildcard examples/*.strm)

SRCS=$(filter-out src/y.tab.c src/lex.yy.c, $(wildcard src/*.c))
OBJS:=$(SRCS:.c=.o) src/parse.o
DEPS:=$(OBJS:.o=.d)

all : $(TARGET)

.PHONY : all

test : all
	$(TARGET) -c $(TESTS)

%.o: %.c
	$(CC) $(CFLAGS) -MMD -MP -c -o $@ $<

.PHONY : test

src/y.tab.c src/y.tab.h : src/parse.y
	$(YACC) -o src/y.tab.c src/parse.y

src/lex.yy.c src/lex.yy.h : src/lex.l
	$(LEX) --header-file=src/lex.yy.h -osrc/lex.yy.c src/lex.l

src/parse.o : src/y.tab.c src/lex.yy.c
	$(CC) -g -MMD -MP -c src/y.tab.c -o src/parse.o

src/node.o : src/y.tab.h src/lex.yy.h

$(TARGET) : $(OBJS)
	mkdir -p "$$(dirname $(TARGET))"
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET) $(LIBS)

clean :
	rm -f src/y.output src/y.tab.c src/y.tab.h
	rm -f src/lex.yy.c src/lex.yy.h
	rm -f src/*.d src/*.o $(TARGET)
.PHONY : clean

-include $(DEPS)
