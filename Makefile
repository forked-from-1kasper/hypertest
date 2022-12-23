CXX = g++-11

BINARY = Hyper
CFLAGS = -std=c++2a
CPP    = Hyper.cpp
HPP    = Matrix.hpp Gyrovector.hpp Fuchsian.hpp Fundamentals.hpp Enumerable.hpp

ifeq ($(OS),Windows_NT)
	BINARY = Hyper.exe
	LIBS = -lSOIL -lglfw3 -lglew32 -lopengl32 -lglu32
else
	BINARY = Hyper
	LIBS = -lSOIL -lglfw -lGLEW -lGL -lGLU
endif

ifeq ($(UNAME), Darwin)
endif

all: Hyper

Hyper: $(HPP) $(CPP)
	$(CXX) $(CFLAGS) $(CPP) -o $(BINARY) $(LIBS)

run: Hyper
	./$(BINARY)

clean:
	rm -f $(BINARY)
