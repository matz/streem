TARGET = bin/streem

ifeq (Windows_NT,$(OS))
TARGET:=$(TARGET).exe
endif

TESTS=$(wildcard examples/*.strm)

.PHONY : all test clean

all clean:
	$(MAKE) -C src $@

test : all
	$(TARGET) -c $(TESTS)
