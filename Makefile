CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c17 -Iinclude
LDFLAGS = -lm
TARGET = jogo_velha
SRC_DIR = src
BUILD_DIR = build
ASSETS_DIR = assets

SOURCES = $(wildcard $(SRC_DIR)/*.c)
OBJECTS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SOURCES))

all: create_dir $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $(BUILD_DIR)/$(TARGET) $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

create_dir:
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)

run: all
	@if [ -f "assets/board.png" ]; then \
		cp assets/board.png $(BUILD_DIR)/ && \
		cd $(BUILD_DIR) && ./$(TARGET); \
	else \
		echo "Erro: Arquivo assets/board.png não encontrado"; \
		echo "Crie o diretório assets e coloque board.png lá"; \
		exit 1; \
	fi

.PHONY: all clean run create_dir