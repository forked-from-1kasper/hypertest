CXX = g++-11

BINARY = Hyper
CFLAGS = -std=c++2a
CPP    = Hyper.cpp
HPP    = Matrix.hpp Gyrovector.hpp Fuchsian.hpp Fundamentals.hpp Enumerable.hpp

UNAME := $(shell uname)

ifeq ($(UNAME), Linux)
LIBS := -L/usr/local/lib -lglfw -lSOIL -lGLEW -lGL -lGLU
endif

ifeq ($(UNAME), Darwin)
LIBS := -lglfw -lSOIL -lGLEW -lGL -lGLU
endif

Hyper: $(HPP) $(CPP)
	$(CXX) $(CFLAGS) $(CPP) -o $(BINARY) $(LIBS)

run: Hyper
	./$(BINARY)

clean:
	rm -f $(BINARY)
