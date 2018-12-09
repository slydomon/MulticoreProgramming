INC = 
LIB = 
CFLAGS += -std=c++11 -Wall
CC = g++

maze-demo: maze-demo.cc maze.o
	$(CC) $(INC) $(LIB) maze-demo.cc maze.o $(CFLAGS) -o $@

maze.o: maze.cc
	$(CC) $(INC) -c maze.cc $(CFLAGS)
