CC = gcc

CFLAGS = -Wall -Wextra -Iinc

LDFLAGS = -lcurl -lcjson

SRC = $(wildcard src/*.c)

OBJ = $(SRC:src/%.c=build/%.o)


TARGET = build/main.exe


all: $(TARGET)


$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET) $(LDFLAGS)


build/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@



clean:
	rm -f $(OBJ) $(TARGET)