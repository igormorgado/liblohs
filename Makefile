CC     = gcc
CFLAGS = -Wall -ggdb -g3
OBJS   = crc16-ansi.o lohs.o main.o
TARGET = lohsbin

.SUFFIXES: .c
.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<

all: $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) 

clean:
	-rm -rf $(OBJS) $(TARGET)
