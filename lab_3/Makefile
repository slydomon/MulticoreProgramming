# Makefile for the malloc lab driver
#
make CXX = g++-6.2.0
CXXFLAGS += -Wall -g
CXXFLAGS += -std=c++11
CXXFLAGS += -stdlib=libc++
CXXFLAGS += -lgtest
CXXFLAGS += -lpthread
#CXXFLAGS += -lrt
#CXXFLAGS += -v

TEST = test.o
LAB2 = lab2.o 

all: lab3 test

test: test.o
	$(CXX) $(CXXFLAGS) -o test $(TEST)

lab2: lab2.o
	$(CXX) $(CXXFLAGS) -o lab3 $(LAB2)

test.o: test.cpp
lab2.o: lab3.cpp

clean:
	rm -f *~ *.o lib0
