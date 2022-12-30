CXX = g++

BINARY = Hyper
CFLAGS = -std=c++2a
CPP    = Hyper.cpp Geometry.cpp Sheet.cpp PicoPNG.cpp
HPP    = Sheet.hpp Geometry.hpp PicoPNG.hpp Matrix.hpp Gyrovector.hpp Fuchsian.hpp Fundamentals.hpp Enumerable.hpp

ifeq ($(OS),Windows_NT)
	BINARY = Hyper.exe
	LIBS = -lglfw3 -lglew32 -lopengl32 -lglu32
else
	BINARY = Hyper
	LIBS = -lglfw -lGLEW -lGL -lGLU
endif

all: Hyper

Hyper: $(HPP) $(CPP)
	$(CXX) $(CFLAGS) $(CPP) -o $(BINARY) $(LIBS)

run: Hyper
	./$(BINARY)

clean:
	rm -f $(BINARY)
