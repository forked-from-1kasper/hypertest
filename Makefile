CXX = g++

BINARY = Hyper
CFLAGS = -std=c++2a
CPP    = Hyper.cpp Shader.cpp Geometry.cpp Sheet.cpp PicoPNG.cpp
HPP    = Geometry.hpp Shader.hpp Sheet.hpp PicoPNG.hpp Gyrovector.hpp Fuchsian.hpp Fundamentals.hpp Enumerable.hpp

ifeq ($(OS),Windows_NT)
	BINARY = Hyper.exe
	LIBS = -lglfw3 -lglew32 -lopengl32 -lglu32
else
	BINARY = Hyper

	UNAME := $(shell uname -s)

	ifeq ($(UNAME),Linux)
		LIBS = -lglfw -lGLEW -lGL -lGLU
	endif

	ifeq ($(UNAME),Darwin)
		LIBS = -lglfw -lGLEW -framework CoreVideo -framework OpenGL -framework IOKit -framework Cocoa -framework Carbon
	endif
endif

all: Hyper

Hyper: $(HPP) $(CPP)
	$(CXX) $(CFLAGS) $(CPP) -o $(BINARY) $(LIBS)

run: Hyper
	./$(BINARY)

clean:
	rm -f $(BINARY)
