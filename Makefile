Hyper: Matrix.hpp Gyrovector.hpp Hyper.cpp
	g++-10 -std=c++2a Hyper.cpp -o Hyper -lglfw -lGLEW -lGL -lGLU

run: Hyper
	./Hyper

clean:
	rm -f Hyper
