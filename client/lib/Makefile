CC = gcc
CFLAGS = -fPIC -shared
TARGET = libclient.so

all: $(TARGET)

$(TARGET): client.c
	$(CC) $(CFLAGS) $< -o $@
	@echo "Built: $@"

clean:
	rm -rf libclient.so

.PHONY: all clean
