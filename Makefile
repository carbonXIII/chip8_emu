build/main: main.cpp core.cpp sdl.cpp
	g++ -g -std=c++11 $^ -lSDL2 -o $@
