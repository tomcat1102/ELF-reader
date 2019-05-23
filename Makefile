
CFLAGS = -Wall -g
SRC = ELF-reader.c
OBJECTS = $(patsubst %.c, %.o, $(SRC))
TESTFILE = SimpleSection.o 
TARGET = ELF-reader

default: all

all: $(TARGET) $(TESTFILE)

ELF-reader: $(OBJECTS)
	$(CC) -o $@ $(OBJECTS)

clean:
	rm -f $(OBJECTS)
	rm -f $(TESTFILE)
	rm -f $(TARGET)

SimpleSection.o: SimpleSection.c
	$(CC) -m32 -c $^ -o $@

test: $(TESTFILE) 
	./$(TARGET) -h -S $(TESTFILE)
	
.PHONY:	all clean
