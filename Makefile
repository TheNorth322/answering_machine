CC := gcc

SRC_DIR := src
HEADERS_DIR := headers
BIN_DIR := bin

INCLUDES := -I$(HEADERS_DIR) $(shell pkg-config --cflags libpjproject) 
LIBS := $(shell pkg-config --libs libpjproject)

SOURCES := $(wildcard $(SRC_DIR)/*.c)
OBJECTS := $(patsubst $(SRC_DIR)/%.c, $(BIN_DIR)/%.o, $(SOURCES))

TARGET := $(BIN_DIR)/answering_machine

all: $(BIN_DIR) $(TARGET)

$(BIN_DIR):
	@mkdir -p $@

# Link object files 
$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LIBS)

# Compile source files to object files
$(BIN_DIR)/%.o: $(SRC_DIR)/%.c | $(BIN_DIR)
	$(CC) -c $(INCLUDES) -o $@ $<

clean:
	@rm -rf $(BIN_DIR)

.PHONY: all clean
