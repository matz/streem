TARGET = a.out
ifeq (Windows_NT,$(OS))
TARGET:=$(TARGET).exe
endif

CFLAGS=-g -Wall
SRC=$(wildcard *.c)
OBJ=${SRC:.c=.o}
DEPENDS=$(wildcard *.d)

%.o: %.c
	@$(CC) -MM $< > $*.d
	$(CC) -c $(CFLAGS) $< -o $@

all : $(TARGET)

.PHONY : all
.PHONY : test

$(TARGET) : $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(TARGET)

clean :
	rm -f *.o $(TARGET)
.PHONY : clean

-include $(DEPENDS)
