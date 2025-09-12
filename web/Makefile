CC = emcc
CFLAGS = -std=c++17 -O2 -s WASM=1 -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap"]' \
         -s EXPORTED_FUNCTIONS='["_main"]' -s ALLOW_MEMORY_GROWTH=1 \
         -s USE_PTHREADS=0 -s ASSERTIONS=1

SOURCES = main.cpp ../src/game.cpp
TARGET = bloop

all: $(TARGET).js

$(TARGET).js: $(SOURCES)
	$(CC) $(CFLAGS) $(SOURCES) -o $(TARGET).js

clean:
	rm -f $(TARGET).js $(TARGET).wasm

.PHONY: all clean
