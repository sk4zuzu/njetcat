CC=gcc
OUTPUT=njetcat
OBJS=$(patsubst %.c,%.o, $(wildcard *.c))

all: $(OUTPUT)

$(OUTPUT): $(OBJS)
	$(CC) -o $@ $^

%.o: %.c
	$(CC) -g -c -o $@ $^

clean:
	-rm $(OUTPUT) $(OBJS)

