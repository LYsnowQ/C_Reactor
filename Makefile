# 编译器设置
CC = gcc
CFLAGS = -Wall -g
LDFLAGS = -pthread

# 目录结构
SRC_DIR = .
BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/o
BIN_DIR = $(BUILD_DIR)/application

# 目标可执行文件
TARGET = $(BIN_DIR)/main_run

SOURCES = $(wildcard $(SRC_DIR)/*.c)

OBJECTS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SOURCES))

.PHONY: all clean dirs

all: dirs $(TARGET)


dirs:
	@mkdir -p $(OBJ_DIR) $(BIN_DIR)


$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)
	@echo "link completed: $@"


$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | dirs
	$(CC) $(CFLAGS) -c $< -o $@
	@echo "compilation completed: $@"

# 清理
clean:
	rm -rf $(BUILD_DIR)

print:
	@echo "SOURCES: $(SOURCES)"
	@echo "OBJECTS: $(OBJECTS)"
	@echo "TARGET: $(TARGET)"
