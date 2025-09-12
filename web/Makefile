EMCC = emcc
CXXFLAGS = -O2 -std=c++17 -s WASM=1

SRCS = \
  main.cpp \
  ../src/bloop_entry.cpp \
  ../src/platform_web.cpp \
  ../src/GameManager.cpp \
  ../src/SnakeGame.cpp \
  ../src/Pong.cpp

OUT = bloop.js

all: $(OUT)

$(OUT): $(SRCS)
	$(EMCC) $(CXXFLAGS) -o $(OUT) $(SRCS) \
	  -s EXPORTED_FUNCTIONS="['_main']" \
	  -s EXPORTED_RUNTIME_METHODS="['ccall','cwrap']"

clean:
	rm -f $(OUT) $(OUT:.js=.wasm) $(OUT:.js=.wasm.map)
