# Windows Toolkit (C) - build com MinGW-w64 (gcc) em CMD/PowerShell/MSYS2.
#   mingw32-make            -> gera windows-toolkit.exe
#   mingw32-make clean
CC      ?= gcc
CFLAGS  ?= -std=gnu11 -O2 -Wall -Wextra -Iinclude -D_WIN32_WINNT=0x0A00
LDLIBS  := -liphlpapi -lws2_32 -lpsapi -lpowrprof -ladvapi32
SRC     := $(wildcard src/*.c)
TARGET  := windows-toolkit.exe

all: $(TARGET)

$(TARGET): $(SRC) $(wildcard include/*.h)
	$(CC) $(CFLAGS) -o $@ $(SRC) $(LDLIBS)

clean:
	-del /Q $(TARGET) 2>NUL
	-rm -f $(TARGET)

.PHONY: all clean
