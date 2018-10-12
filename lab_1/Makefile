# Makefile for the malloc lab driver
#
make CXX = g++-6.2.0
CXXFLAGS += -Wall -g
CXXFLAGS += -std=c++11
CXXFLAGS += -stdlib=libc++
CXXFLAGS += -lgtest
CXXFLAGS += -lpthread
TEST = test.o
LAB1 = lab1.o 

all: lab1 test

test: test.o
	$(CXX) $(CXXFLAGS) -o test $(TEST)

lab1: lab1.o
	$(CXX) $(CXXFLAGS) -o lab1 $(LAB1)

test.o: test.cpp
lab1.o: lab1.cpp

clean:
	rm -f *~ *.o lib0
