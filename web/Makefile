# web/Makefile
EMCC = emcc
CXXFLAGS = -O2 -std=c++17 -s USE_SDL=2 -s USE_SDL_IMAGE=2 -s WASM=1

SRCS = \
  main.cpp \
  ../src/bloop_entry.cpp \
  ../src/GameManager.cpp \
  ../src/SnakeGame.cpp \
  ../src/Pong.cpp

OBJS = $(SRCS:.cpp=.o)

OUT = bloop.js

all: $(OUT)

$(OUT): $(SRCS)
	$(EMCC) $(CXXFLAGS) -o $(OUT) $(SRCS) \
	  -s EXPORTED_FUNCTIONS="['_main','_setup','_loop']" \
	  -s EXPORTED_RUNTIME_METHODS="['ccall','cwrap']"

clean:
	rm -f $(OBJS) $(OUT) $(OUT:.js=.wasm) $(OUT:.js=.wasm.map)
