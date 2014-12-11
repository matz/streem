YACC = yacc
LEX = flex
CC = gcc

all : streem

.PHONY : all

test : all
	streem example.stm
.PHONY : test

y.tab.c : parse.y
	$(YACC) parse.y

lex.yy.c : lex.l
	$(LEX) lex.l

parse.o : y.tab.c lex.yy.c
	$(CC) -g -c y.tab.c -o parse.o

streem : parse.o
	$(CC) -g parse.o -o streem

clean :
	rm -f y.output y.tab.c
	rm -f lex.yy.c
	rm -f *.o streem
.PHONY : clean
