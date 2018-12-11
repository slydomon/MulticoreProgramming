# Makefile for the malloc lab driver
#
CXX = g++
#CXX = clang++
CXXFLAGS += -Wall 
CXXFLAGS += -g
CXXFLAGS += -std=c++11
#CXXFLAGS += -stdlib=libc++
#CXXFLAGS += -lgtest
CXXFLAGS += -lpthread
#CXXFLAGS += -lrt
#CXXFLAGS += -v

#TEST = test.o
LAB4 = main.o thread_pool.o maze.o

all: lab4 

#test: test.o
#	$(CXX) $(CXXFLAGS) -o test $(TEST)

lab4: main.o thread_pool.o maze.o
	$(CXX) -o lab4 $(LAB4)

#test.o: test.cpp
main.o: thread_pool.hpp main.cpp 
	$(CXX) $(CXXFLAGS) -c thread_pool.hpp main.cpp 
thread_pool.o: thread_pool.cpp maze.hpp 
	$(CXX) $(CXXFLAGS) -c thread_pool.cpp maze.hpp
maze.o: maze.cc
	$(CXX) $(CXXFLAGS) -c maze.cc
clean:
	rm *.o
