# DISK SURVIVOR - Makefile
# Target: macOS (arm64/x86_64) and Windows x64

CC = cc
CFLAGS = -std=c99 -Os -flto -Wall -Wextra -ffunction-sections -fdata-sections \
         -fno-asynchronous-unwind-tables -fno-ident
LDFLAGS = -flto

SRCS = src/main.c src/player.c src/enemy.c src/gem.c src/render.c \
       src/input.c src/upgrade.c src/boss.c src/scene.c src/audio.c src/particle.c \
       src/bgm.c src/score.c src/item.c src/debug.c \
       src/params.c src/bot.c src/settings.c src/weapon_util.c \
       src/weapons/pulse.c src/weapons/orbiters.c src/weapons/beam.c \
       src/weapons/nova.c src/weapons/mines.c src/weapons/chain.c \
       src/weapons/boomerang.c src/weapons/trail.c src/weapons/whip.c
TARGET = disk_survivor

# Size limit (1.44 MB floppy)
SIZE_LIMIT = 1474560

# raylib paths (custom-built minimal lib)
RAYLIB_INCLUDE = deps/raylib-5.5/src
RAYLIB_MAC_LIB = deps/raylib-build/mac/libraylib.a
RAYLIB_WIN_LIB = deps/raylib-build/win/libraylib.a

.PHONY: mac mac-arm64 mac-x86_64 win win-native clean size-check run raylibwin raylibmac

# === macOS ===

mac: mac-arm64

mac-arm64:
	$(CC) $(CFLAGS) -arch arm64 $(SRCS) -o $(TARGET) \
		-I$(RAYLIB_INCLUDE) $(RAYLIB_MAC_LIB) \
		-framework CoreVideo -framework IOKit -framework Cocoa \
		-framework GLUT -framework OpenGL \
		$(LDFLAGS) -Wl,-dead_strip
	strip $(TARGET)
	@$(MAKE) size-check

mac-x86_64:
	$(CC) $(CFLAGS) -arch x86_64 $(SRCS) -o $(TARGET)_x86 \
		-I$(RAYLIB_INCLUDE) $(RAYLIB_MAC_LIB) \
		-framework CoreVideo -framework IOKit -framework Cocoa \
		-framework GLUT -framework OpenGL \
		$(LDFLAGS) -Wl,-dead_strip
	strip $(TARGET)_x86
	@$(MAKE) size-check BIN=$(TARGET)_x86

# === Windows cross-compile ===

win:
	x86_64-w64-mingw32-gcc $(CFLAGS) $(SRCS) -o $(TARGET).exe \
		-I$(RAYLIB_INCLUDE) $(RAYLIB_WIN_LIB) \
		-lopengl32 -lgdi32 -lwinmm -lm \
		-Wl,--gc-sections -mwindows
	x86_64-w64-mingw32-strip --strip-all $(TARGET).exe
	@$(MAKE) size-check BIN=$(TARGET).exe

win-native:
	gcc $(CFLAGS) $(SRCS) -o $(TARGET).exe \
		-lraylib -lopengl32 -lgdi32 -lwinmm -lm \
		-Wl,--gc-sections
	strip --strip-all $(TARGET).exe
	@$(MAKE) size-check BIN=$(TARGET).exe

# === Build raylib (run once or after config changes) ===

raylibwin:
	cd $(RAYLIB_INCLUDE) && make clean && \
	make CC=x86_64-w64-mingw32-gcc AR=x86_64-w64-mingw32-ar \
		PLATFORM=PLATFORM_DESKTOP OS=Windows_NT \
		RAYLIB_LIBTYPE=STATIC RAYLIB_BUILD_MODE=RELEASE \
		CUSTOM_CFLAGS="-Os -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident"
	mkdir -p deps/raylib-build/win
	mv $(RAYLIB_INCLUDE)/libraylib.a $(RAYLIB_WIN_LIB)

raylibmac:
	cd $(RAYLIB_INCLUDE) && make clean && \
	make PLATFORM=PLATFORM_DESKTOP \
		RAYLIB_LIBTYPE=STATIC RAYLIB_BUILD_MODE=RELEASE \
		CUSTOM_CFLAGS="-Os -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident"
	mkdir -p deps/raylib-build/mac
	mv $(RAYLIB_INCLUDE)/libraylib.a $(RAYLIB_MAC_LIB)

# === Utility ===

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
