TARGET := pwatch

BUILD_DIR := ./build
SRC_DIRS := ./src

SRC_FILES := $(shell find $(SRC_DIRS) -name '*.c' -or -name '*.s')

OJB_FILES := $(SRC_FILES:%=$(BUILD_DIR)/%.o)

CC = gcc
CFLAGS = -Wall -Wextra -Werror

#Building starts here

$(BUILD_DIR)/$(TARGET): $(OJB_FILES)
	$(CC) $(OJB_FILES) -o $@

$(BUILD_DIR)/%.c.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	-rm -r -f $(BUILD_DIR)
