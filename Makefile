CC = gcc
CFLAGS = -Wall -Wextra -g -Iinclude
LDFLAGS = 

SOURCES = src/main.c src/server.c src/http.c src/mime.c src/files.c
OBJECTS = $(SOURCES:.c=.o)
TARGET = minihttpd

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

run: $(TARGET)
	./$(TARGET) 8080

.PHONY: all clean run