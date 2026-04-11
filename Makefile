SRC := $(shell find . -name "*.c" -not -path "./build/*")
OBJ := $(SRC:.c=.o)
TARGET := main_run

CC := gcc
CFLAGS := -I. -I./include -Wall -g
LDFLAGS := -lpthread 
.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJ)
