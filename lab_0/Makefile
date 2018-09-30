# Makefile for the malloc lab driver
#
make CXX = g++-6.2.0
CXXFLAGS += -Wall -g
CXXFLAGS += -std=c++11
CXXFLAGS += -stdlib=libc++


OBJS = lab0.o 

all: lab0

lab0: lab0.o
	$(CXX) $(CXXFLAGS) -o lab0 $(OBJS)

lab0.o: lab0.cpp

clean:
	rm -f *~ *.o lib0
