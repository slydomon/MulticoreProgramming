// Maze Demo Main File
// Adapted from original by Joe Wingbermuehle

#include <ctime>
#include "maze.hpp"

/** Generate and display a random maze. */
int main(int argc, char* argv[]) {
	std::srand(std::time(0));			// Remember to do this!
	Maze m(31, 31);
	std::cout << m;
	std::cout << "Start at (" << m.getStart().row << ", " << m.getStart().col << ")" << std::endl;
	std::cout << "Finish at (" << m.getFinish().row << ", " << m.getFinish().col << ")" << std::endl;
	return 0;
}
