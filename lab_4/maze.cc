// Maze generator in C++
// (c) Joe Wingbermuehle 2013-11-15
// Modified by Christopher Mitchell, Ph.D.
// Source: https://github.com/joewing/maze/
// License: BSD 3-Clause

#include "maze.hpp"

Maze::Maze(const size_t rows, const size_t cols)
	: rows_(rows)
	, cols_(cols)
{
	maze_.resize(cols_ * rows_);
	generate();
}

void Maze::generate() {
	initialize();
	carve(1, 1);

	start_ = Coord(1, 1);
	reset(start_.row, start_.col);
	finish_ = Coord(rows_ - 2, cols_ - 2);
	reset(finish_.row, finish_.col);
	//maze_[m_width + 2] = true;
	//maze_[(m_height - 2) * m_width + m_width - 3] = true;
}

void Maze::set(const size_t row, const size_t col, const bool val) {
	maze_[(row * cols_) + col] = val;
}

const bool Maze::get(const size_t row, const size_t col) const {
	return maze_[(row * cols_) + col];
}

void Maze::reset(const size_t row, const size_t col) {
	set(row, col, false);
}

std::ostream &Maze::show(std::ostream &os) const {
	for(unsigned y = 0; y < rows_; y++) {
		for(unsigned x = 0; x < cols_; x++) {
			os << (get(y, x) ? "#" : " ");
		}
		os << "\n";
	}
	return os;
}

/** Initialize the maze array. */
void Maze::initialize() {
	std::fill(maze_.begin(), maze_.end(), true);
}

/** Carve starting at x, y. */
void Maze::carve(const size_t x, const size_t y) {
	static const int dirs[] = {1, -1, 0, 0};
	reset(y, x);
	const unsigned d = std::rand();
	for(unsigned i = 0; i < 4; i++) {
		const int dx = dirs[(i + d + 0) % 4];
		const int dy = dirs[(i + d + 2) % 4];
		const int x1 = x + dx, y1 = y + dy;
		const int x2 = x1 + dx, y2 = y1 + dy;
		if(y1 > 0 && y1 < rows_ - 1 && x1 > 0 && x1 < cols_ - 1 && get(y1, x1) && get(y2, x2)) {
			reset(y1, x1);
			carve(x2, y2);
		}
	}
}

std::ostream &operator<<(std::ostream &os, const Maze &maze) {
	return maze.show(os);
}

const Coord Maze::getStart() {
	return start_;
}

const Coord Maze::getFinish() {
	return finish_;
}
