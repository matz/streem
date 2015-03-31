YACC = bison -y
LEX = flex
CC = gcc
TARGET = bin/streem
CFLAGS = -g -Wall
LIBS = -lpthread

ifeq (Windows_NT,$(OS))
TARGET:=$(TARGET).exe
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

src/y.tab.c : src/parse.y
	$(YACC) -o src/y.tab.c src/parse.y

src/lex.yy.c : src/lex.l
	$(LEX) -osrc/lex.yy.c src/lex.l

src/parse.o : src/y.tab.c src/lex.yy.c
	$(CC) -g -MMD -MP -c src/y.tab.c -o src/parse.o

$(TARGET) : $(OBJS)
	mkdir -p "$$(dirname $(TARGET))"
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET) $(LIBS)

clean :
	rm -f src/y.output src/y.tab.c
	rm -f src/lex.yy.c
	rm -f src/*.o $(TARGET)
.PHONY : clean

-include $(DEPS)
