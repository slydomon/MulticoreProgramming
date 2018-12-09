// Maze generator in C++
// (c) Joe Wingbermuehle 2013-11-15
// Modified by Christopher Mitchell, Ph.D.
// Source: https://github.com/joewing/maze/
// License: BSD 3-Clause

#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

struct Coord {
	Coord(void)
		: row(0), col(0)
	{}
	Coord(const size_t row0, const size_t col0)
		: row(row0), col(col0)
	{}
	size_t row;
	size_t col;
};

class Maze {

	void generate(void);
	void set(const size_t row, const size_t col, const bool val = true);
	void reset(const size_t row, const size_t col);

	void initialize(void);
	void carve(const size_t x, const size_t y);

	const size_t rows_, cols_;
	std::vector<bool> maze_;
	Coord start_, finish_;
	
  protected:
	std::ostream& show(std::ostream &os) const;
	friend std::ostream &operator<<(std::ostream &os, const Maze &maze);

  public:
	Maze(const size_t rows, const size_t cols);
	const bool get(const size_t row, const size_t col) const;		// Get whether there's a wall at the given (row, col)
	const Coord getStart();
	const Coord getFinish();
};
