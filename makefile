TARGET = pwatch

BUILD_DIR := ./build
SRC_DIR := ./src

SRC_FILES_WITH_PATH :=$(shell find $(SRC_DIR) -name "*.c" )
HEADER_FILES_WITH_PATH :=$(shell find $(SRC_DIR) -iname "*.h" )
#OJB_FILES := $(addsuffix .o, $(notdir $(SRC_FILES_WITH_PATH)))

CC = gcc
CFLAGS = -Wall -Wextra -Werror

$(BUILD_DIR)/$(TARGET): $(SRC_FILES_WITH_PATH) $(HEADER_FILES_WITH_PATH)
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(SRC_FILES_WITH_PATH) -o $@

clean:
	-rm -r -f $(BUILD_DIR)
