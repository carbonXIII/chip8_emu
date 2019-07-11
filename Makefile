main: main.cpp
	g++ -g -std=c++11 $^ -lSDL2 -o $@
