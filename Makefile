YACC = bison -y
LEX = flex
TARGET = bin/streem

TESTS=$(wildcard example/*.strm)

all : $(TARGET)

.PHONY : all

test : all
	$(TARGET) $(TESTS)

.PHONY : test

src/y.tab.c : src/parse.y
	$(YACC) -o src/y.tab.c src/parse.y

src/lex.yy.c : src/lex.l
	$(LEX) -o src/lex.yy.c src/lex.l

src/parse.o : src/y.tab.c src/lex.yy.c
	$(CC) -g -c src/y.tab.c -o src/parse.o

$(TARGET) : src/parse.o
	mkdir -p "$$(dirname $(TARGET))"
	$(CC) -g src/parse.o -o $(TARGET)

clean :
	rm -f src/y.output src/y.tab.c
	rm -f src/lex.yy.c
	rm -f src/*.o $(TARGET)
.PHONY : clean
