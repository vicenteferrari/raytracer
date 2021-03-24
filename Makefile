CC=g++

main_source := $(wildcard src/*.cpp)
main_headers := $(wildcard src/*.h)
data := $(wildcard data/*)

.PHONY: all
all: raytracer.exe

raytracer.exe: raytracer.o
	$(CC) raytracer.o -o raytracer.exe -lmingw32 -lSDL2main -lSDL2 -lm

raytracer.o: $(main_source) $(main_headers)
	$(CC) -g -c src/raytracer.cpp -o raytracer.o


.PHONY: clean
clean:
	rm -f raytracer.o
	rm -f raytracer.exe
