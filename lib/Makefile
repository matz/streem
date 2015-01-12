TARGET = streem_proto
ifeq (Windows_NT,$(OS))
TARGET:=$(TARGET).exe
endif

CFLAGS=-g -Wall
LIBS=-lpthread
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
	$(CC) $(CFLAGS) $(OBJ) -o $(TARGET) $(LIBS)

clean :
	rm -f *.o $(TARGET)
.PHONY : clean

-include $(DEPENDS)
