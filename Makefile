# DISK SURVIVOR - Makefile
# Target: macOS (arm64/x86_64) and Windows x64

CC = cc
CFLAGS = -std=c99 -Os -flto -Wall -Wextra
LDFLAGS = -flto

SRCS = src/main.c src/player.c src/weapon.c src/enemy.c src/gem.c src/render.c src/input.c src/upgrade.c src/boss.c src/scene.c
TARGET = disk_survivor

# Size limit (1.44 MB floppy)
SIZE_LIMIT = 1474560

# macOS
.PHONY: mac mac-arm64 mac-x86_64 win win-native clean size-check run

mac: mac-arm64

mac-arm64:
	$(CC) $(CFLAGS) -arch arm64 $(SRCS) -o $(TARGET) \
		$$(pkg-config --cflags --libs raylib) $(LDFLAGS)
	strip $(TARGET)
	@$(MAKE) size-check

mac-x86_64:
	$(CC) $(CFLAGS) -arch x86_64 $(SRCS) -o $(TARGET)_x86 \
		$$(pkg-config --cflags --libs raylib) $(LDFLAGS)
	strip $(TARGET)_x86
	@$(MAKE) size-check BIN=$(TARGET)_x86

# Windows (MinGW cross-compile from macOS, or native on Windows)
RAYLIB_WIN_PATH ?= deps/raylib-5.5_win64_mingw-w64

win:
	x86_64-w64-mingw32-gcc $(CFLAGS) $(SRCS) -o $(TARGET).exe \
		-I$(RAYLIB_WIN_PATH)/include \
		-L$(RAYLIB_WIN_PATH)/lib \
		-lraylib -lopengl32 -lgdi32 -lwinmm -lm
	x86_64-w64-mingw32-strip $(TARGET).exe
	@$(MAKE) size-check BIN=$(TARGET).exe

# Native Windows build (run on Windows with MinGW or MSVC)
win-native:
	gcc $(CFLAGS) $(SRCS) -o $(TARGET).exe \
		-lraylib -lopengl32 -lgdi32 -lwinmm -lm
	strip $(TARGET).exe
	@$(MAKE) size-check BIN=$(TARGET).exe

# Size check
BIN ?= $(TARGET)
size-check:
	@SIZE=$$(stat -f%z "$(BIN)" 2>/dev/null || stat --printf="%s" "$(BIN)" 2>/dev/null); \
	echo ""; \
	echo "=== Build Size Check ==="; \
	echo "Binary: $(BIN)"; \
	echo "Size: $$SIZE bytes"; \
	echo "Limit: $(SIZE_LIMIT) bytes (1.44 MB)"; \
	if [ $$SIZE -le $(SIZE_LIMIT) ]; then \
		echo "Status: OK ($$(($(SIZE_LIMIT) - SIZE)) bytes remaining)"; \
	else \
		echo "Status: OVER LIMIT by $$((SIZE - $(SIZE_LIMIT))) bytes!"; \
	fi; \
	echo "========================"

run: mac
	./$(TARGET)

clean:
	rm -f $(TARGET) $(TARGET)_x86 $(TARGET).exe
