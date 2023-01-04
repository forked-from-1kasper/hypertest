CXX = g++

BINARY = Hyper
CFLAGS = -std=c++2a -Iinclude/
CPP    = source/Hyper.cpp source/Shader.cpp source/Geometry.cpp source/Sheet.cpp source/PicoPNG.cpp

HPP    = include/Hyper/Geometry.hpp include/Hyper/Shader.hpp include/Hyper/Sheet.hpp
HPP   += include/PicoPNG.hpp include/Hyper/Gyrovector.hpp include/Hyper/Fuchsian.hpp
HPP   += include/Hyper/Fundamentals.hpp include/Hyper/Moebius.hpp
HPP   += include/Hyper/Tesselation.hpp include/Hyper/Grid.hpp
HPP   += include/Enumerable.hpp include/List.hpp include/Literal.hpp include/Tuple.hpp

ifeq ($(OS),Windows_NT)
	BINARY = Hyper.exe
	LDFLAGS = -lglfw3 -lglew32 -lopengl32 -lglu32
else
	BINARY = Hyper

	UNAME := $(shell uname -s)

	ifeq ($(UNAME),Linux)
		LDFLAGS = -lglfw -lGLEW -lGL -lGLU
	endif

	ifeq ($(UNAME),Darwin)
		LDFLAGS = -lglfw -lGLEW -framework CoreVideo -framework OpenGL -framework IOKit -framework Cocoa -framework Carbon
	endif
endif

all: Hyper

Hyper: $(HPP) $(CPP)
	$(CXX) $(CFLAGS) $(CPP) -o $(BINARY) $(LDFLAGS)

run: Hyper
	./$(BINARY)

clean:
	rm -f $(BINARY)

barbarize:
	python3 barbarize.py $(CPP) $(HPP)
