# mruby is using Rake (http://rake.rubyforge.org) as a build tool.
# We provide a minimalistic version called minirake inside of our
# codebase.

CC = gcc

all : streem

.PHONY : all

test : all
	streem example.stm
.PHONY : test

y.tab.c : parse.y
	yacc parse.y

lex.yy.c : lex.l
	flex lex.l

parse.o : y.tab.c lex.yy.c
	$(CC) -g -c y.tab.c -o parse.o

streem : parse.o
	$(CC) -g parse.o -o streem

clean :
	rm -f y.output y.tab.c
	rm -f lex.yy.c
	rm -f *.o streem
.PHONY : clean
