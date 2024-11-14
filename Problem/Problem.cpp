#include <sstream>
#include <string>
#include <stdexcept>
#include <vector>
#include <algorithm>
#include "Problem.h"
#include "Utilities/Utilities.h"


namespace Sudoku{
    Problem::Problem():
        m_solved(false)
    {
        m_matrix = std::array<std::array<int, 9>, 9>();
        for(int row=0; row<SIZE; ++row)
        {
            for(int col=0; col<SIZE; ++col)
            {
                m_matrix[row][col] = 0;
                m_unsolved.push_back(Coord(row, col));
            }
        }
    }

    Problem::Problem(const char* filename):
        m_solved(false)
    {
        auto tic = std::chrono::system_clock::now();
        m_matrix = std::array<std::array<int, 9>, 9>();
        for(int row=0; row<SIZE; ++row)
        {
            for(int col=0; col<SIZE; ++col)
            {
                m_matrix[row][col] = 0;
                m_unsolved.push_back(Coord(row, col));
            }
        }
        std::ifstream f;
        f.open(filename);
        if(!f.is_open())
        {
            std::cerr << "Failed to open file: " << filename << "\n";
            exit(-1);
        }
        std::stringstream ss;
        ss << f.rdbuf();
        std::string line;
        while(std::getline(ss, line))
        {
            if (line.length() != 5)
            {
                std::cerr << "Invalid line: " << line << "\n";
                exit(-1);
            }
            // row[sep]col[sep]value
            int row = line[0] - '0';
            int col = line[2] - '0';
            int value = line[4] - '0';
            setCell(Coord(row, col), value);
        }
        #ifdef DEBUG
        std::cout << "[Problem] initialization: " << getTimeDiff(tic) << "us\n";
        #endif
    }

    void Problem::setCell(Coord coord, int value)
    {
        unsigned int row = coord.first, column = coord.second;
        if(value != 0)
        {
            if(getCell(Coord(row, column)) == 0)
            {
                if(valueInside(getRow(row), value) ||
                    valueInside(getColumn(column), value) ||
                    valueInside(flattenBlock(getBlock(computeBlockId(coord))), value))
                {
                    throw std::runtime_error("Duplicated value found");
                }
                m_matrix[row][column] = value;
                auto it = std::find(m_unsolved.begin(), m_unsolved.end(), coord);
                m_unsolved.erase(it);
                if(m_unsolved.size() == 0)
                    m_solved = true;
            }
            else
            {
                throw std::runtime_error("Cell already has value");
            }
        }
        else
        {
            if(m_matrix[row][column] != 0)
            {
                m_matrix[row][column] = 0;
                m_unsolved.push_back(coord);
            }
        }
    }

    std::array<std::array<int, 3>, 3> Problem::getBlock(unsigned int block_id)
    {
        std::array<std::array<int, 3>, 3> block;
        for(int row=0; row<3; ++row)
        {
            for(int col=0; col<3; ++col)
            {
                block[row][col] = getCell(
                    Coord((block_id / 3) * 3 + row, (block_id % 3) * 3 + col));
            }
        }
        return block;
    }

    std::array<int, 9> Problem::getRow(unsigned int row)
    {
        return m_matrix[row];
    }

    std::array<int, 9> Problem::getColumn(unsigned int column)
    {
        std::array<int, 9> col;
        for(int row=0; row<SIZE; ++row)
        {
            col[row] = getCell(Coord(row, column));
        }
        return col;
    }

    void Problem::display(void)
    {
        print2DArray<9, 9>(m_matrix);
        std::cout << "(" << m_unsolved.size() << " remains)\n";
    }

    void Problem::displayBlock(unsigned int block_id)
    {
        print2DArray<3, 3>(getBlock(block_id));
    }
}