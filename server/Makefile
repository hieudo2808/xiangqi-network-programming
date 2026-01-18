CC = gcc
LDFLAGS = -pthread -lodbc -lm
INCLUDES = -I./include

SRC_DIR = src
BIN_DIR = bin

SRCS = $(wildcard $(SRC_DIR)/*.c)

TARGET = $(BIN_DIR)/server

all: directories $(TARGET)

directories:
	@mkdir -p $(BIN_DIR)

$(TARGET):
	$(CC) $(INCLUDES) $(SRCS) -o $@ $(LDFLAGS)
	@echo "Server built successfully: $(TARGET)"

clean:
	rm -rf $(BIN_DIR)
	@echo "Clean complete"

rebuild: clean all

install-deps:
	@echo "Installing ODBC dependencies..."
	@if command -v apt-get > /dev/null; then \
		sudo apt-get update; \
		sudo apt-get install -y build-essential unixodbc-dev; \
	else \
		echo "Please install ODBC Driver manually for Windows"; \
	fi

.PHONY: all clean rebuild install-deps directories
