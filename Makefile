# Set the compiler to use.
CXX = g++
# Dependencies.
# Fmt
FMT_PATH = ./deps/fmt
FMT_INC = -I$(FMT_PATH)/include/
FMT_LIB = -L$(FMT_PATH)/lib -lfmt
# Set the flags to pass to the compiler.
CXXFLAGS = -c -Wall -g -std=c++17 $(FMT_INC) 
# Set optimization.
OPT = -O3
# Set linker flags.
LDFLAGS = -g -pthread -static-libgcc -static-libstdc++
# Set application name.
NAME = gymon.out

build: main.o server.o resparse.o
	$(CXX) $(LDFLAGS) $(OPT) main.o server.o resparse.o $(FMT_LIB) -o $(NAME)

main.o: main.cpp
	$(CXX) $(CXXFLAGS) $(OPT) -c main.cpp

server.o: server.cpp
	$(CXX) $(CXXFLAGS) $(OPT) -c server.cpp

resparse.o: resparse.cpp
	$(CXX) $(CXXFLAGS) $(OPT) -c resparse.cpp

clean:
	rm -rf *.o $(NAME)

rebuild: clean build

directory: bin

bin:
	mkdir bin
