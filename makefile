CC := gcc
CFLAGS := -Wall -Wextra -pthread

SRCDIR := src
BUILDDIR := build
TARGET := $(BUILDDIR)/pwatch

SRCEXT := c
SOURCES := $(shell find $(SRCDIR) -type f -name *.$(SRCEXT))
OBJECTS := $(patsubst $(SRCDIR)/%,$(BUILDDIR)/%,$(SOURCES:.$(SRCEXT)=.o))

DEBUG_FLAGS := -g

$(TARGET): $(OBJECTS)
	@mkdir -p $(BUILDDIR)
	$(CC) $(CFLAGS) $(DEBUG_FLAGS) -o $(TARGET) $^

$(BUILDDIR)/%.o: $(SRCDIR)/%.$(SRCEXT)
	@mkdir -p $(BUILDDIR)
	$(CC) $(CFLAGS) $(DEBUG_FLAGS) -c -o $@ $<

.PHONY: clean

clean:
	@rm -rf $(BUILDDIR)