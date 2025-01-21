CC = gcc

INC_DIR = inc
SRC_DIR = src
BUILD_DIR = build

SRC_FILES := $(shell find $(SRC_DIR) -name '*.c')
OBJ_FILES := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRC_FILES))
INC_FILES := $(shell find $(INC_DIR) -name '*.h')

CFLAGS = -g -Wall -I$(INC_DIR) 

TARGET = ex2

all: $(TARGET)

$(TARGET): $(OBJ_FILES)
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

.PHONY: all clean