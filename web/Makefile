EMCC = emcc
CXXFLAGS = -O2 -std=c++17 -s WASM=1

SRCS = \
  main.cpp \
  ../bloop/bloop_entry.cpp \
  ../bloop/platform_web.cpp \
  ../bloop/GameManager.cpp \
  ../bloop/SnakeGame.cpp \
  ../bloop/Pong.cpp

OUT = bloop.js

all: $(OUT)

$(OUT): $(SRCS)
	$(EMCC) $(CXXFLAGS) -o $(OUT) $(SRCS) \
	  -s EXPORTED_FUNCTIONS="['_main']" \
	  -s EXPORTED_RUNTIME_METHODS="['ccall','cwrap']"

clean:
	rm -f $(OUT) $(OUT:.js=.wasm) $(OUT:.js=.wasm.map)
