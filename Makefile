BINARY = Hyper
CFLAGS = -std=c++2a
CPP    = Hyper.cpp
HPP    = Matrix.hpp Gyrovector.hpp Fuchsian.hpp Fundamentals.hpp Enumerable.hpp
LIBS   = -lglfw -lSOIL -lGLEW -lGL -lGLU -lgmpxx -lgmp

Hyper: $(HPP) $(CPP)
	g++ $(CFLAGS) $(CPP) -o $(BINARY) $(LIBS)

run: Hyper
	./$(BINARY)

clean:
	rm -f $(BINARY)
