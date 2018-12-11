# Makefile for the malloc lab driver
#
make CXX = g++-6.2.0
CXXFLAGS += -Wall 
CXXFLAGS += -g
CXXFLAGS += -std=c++11
CXXFLAGS += -stdlib=libc++
CXXFLAGS += -lgtest
CXXFLAGS += -lpthread
#CXXFLAGS += -lrt
#CXXFLAGS += -v

#TEST = test.o
LAB3 = lab3.o 

all: lab3 

#test: test.o
#	$(CXX) $(CXXFLAGS) -o test $(TEST)

lab3: lab3.o
	$(CXX) $(CXXFLAGS) -o lab3 $(LAB3)

#test.o: test.cpp
lab3.o: lab3.cpp

clean:
	rm -f *~ *.o lib0
