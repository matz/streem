YACC = bison -y
LEX = flex
TARGET = ../bin/streem

all : $(TARGET)

.PHONY : all

test : all
	$(TARGET) < example.strm
.PHONY : test

y.tab.c : parse.y
	$(YACC) parse.y

lex.yy.c : lex.l
	$(LEX) lex.l

parse.o : y.tab.c lex.yy.c
	$(CC) -g -c y.tab.c -o parse.o

$(TARGET) : parse.o
	mkdir -p "$$(dirname $(TARGET))"
	$(CC) -g parse.o -o $(TARGET)

clean :
	rm -f y.output y.tab.c
	rm -f lex.yy.c
	rm -f *.o $(TARGET)
.PHONY : clean
