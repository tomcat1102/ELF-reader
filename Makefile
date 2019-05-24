
CC = gcc

SRC = ELF-reader.c
OBJECTS = $(patsubst %.c, %.o, $(SRC))
LIB = m 	# libmath
TESTFILE = SimpleSection.o 
TARGET = ELF-reader

CFLAGS = -Wall -g 
LDFLAGS = $(patsubst %, -l%, $(LIB))

default: all

all: $(TARGET) $(TESTFILE)

ELF-reader: $(OBJECTS)
	$(CC) -o $@ $(OBJECTS) $(LDFLAGS)

clean:
	rm -f $(OBJECTS)
	rm -f $(TESTFILE)
	rm -f $(TARGET)

SimpleSection.o: SimpleSection.c
	$(CC) -m32 -c $^ -o $@

test: $(TESTFILE) 
	./$(TARGET) -h -S $(TESTFILE)
	
.PHONY:	all clean
