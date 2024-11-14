#ifndef _PROBLEM_H
#define _PROBLEM_H
#include <array>
#include <utility>
#include <iostream>
#include <fstream>

#include "Utilities/Utilities.h"

#define SIZE 9

namespace Sudoku
{
    enum class Unit {
        Row,
        Column,
        Block
    };

    class Problem {
    public:
        Problem();
        Problem(const char* filename);
        void setCell(Coord coord, int value);
        int getCell(Coord coord) {return m_matrix[coord.first][coord.second];};
        std::array<std::array<int, 3>, 3> getBlock(unsigned int block_id);
        std::array<int, 9> getRow(unsigned int row);
        std::array<int, 9> getColumn(unsigned int column);
        void display(void);
        void displayBlock(unsigned int block_id);
        std::vector<Coord> getUnsolved(void) {return m_unsolved;}
        bool getSolved(void) {return m_solved;}
    private:
        std::array<std::array<int, 9>, 9> m_matrix;
        std::vector<Coord> m_unsolved;
        bool m_solved;
    };
};
#endif