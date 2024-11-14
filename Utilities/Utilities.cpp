#include <algorithm>
#include "Utilities.h"

std::ostream& operator<<(std::ostream& os, const Coord coord)
{
    os << "(" << coord.first << ", " << coord.second << ")";
    return os;
}

std::ostream& operator<<(std::ostream& os, const std::vector<int> vec)
{
    for(auto v: vec)
        os << v << " ";
    return os;
}

std::array<int, 9> flattenBlock(std::array<std::array<int, 3>, 3> block)
{
    std::array<int, 9> result;
    for(int row=0; row<3; ++row)
    {
        for(int col=0; col<3; ++col)
        {
            result[3 * row + col] = block[row][col];
        }
    }

    return result;
}

bool valueInside(std::array<int, 9> array, int target)
{
    auto it = std::find(array.begin(), array.end(), target);
    return it != array.end();
}

bool valueInside(std::vector<int> values, int target)
{
    auto it = std::find(values.begin(), values.end(), target);
    return it != values.end();
}

bool eraseValue(std::vector<int>& values, int target)
{
    auto it = std::find(values.begin(), values.end(), target);
    if(it != values.end())
    {
        values.erase(it);
        return true;
    }
    return false;
}

bool coordInside(std::vector<Coord> coords, Coord coord)
{
    auto it = std::find(coords.begin(), coords.end(), coord);
    return it != coords.end();
}

unsigned int computeBlockId(Coord coord)
{
    unsigned int row = coord.first, column = coord.second;
    return (row / 3) * 3 + column / 3;
}

std::vector<Coord> getBlockCoords(unsigned int block_id)
{
    unsigned int row = (block_id / 3) * 3;
    unsigned int col = (block_id % 3) * 3;
    std::vector<Coord> coords;
    for(int i=0; i<3; ++i)
    {
        for(int j=0; j<3; ++j)
        {
            coords.push_back(std::make_pair(row + i, col + j));
        }
    }
    return coords;
}

std::vector<Coord> getBlockCoords(Coord coord)
{
    unsigned int row = coord.first, column = coord.second;
    int row_start, row_stop, col_start, col_stop;
    switch(row % 3)
    {
        case 0:
            row_start = 0;
            row_stop = 2;
            break;

        case 1:
            row_start = -1;
            row_stop = 1;
            break;

        case 2:
            row_start = -2;
            row_stop = 0;
            break;
    }
    switch(column % 3)
    {
        case 0:
            col_start = 0;
            col_stop = 2;
            break;

        case 1:
            col_start = -1;
            col_stop = 1;
            break;

        case 2:
            col_start = -2;
            col_stop = 0;
            break;
    }

    std::vector<Coord> coords;
    for(int i = row_start; i<= row_stop; ++i)
    {
        for(int j = col_start; j<= col_stop; ++j)
        {
            coords.push_back(std::make_pair(row + i, column + j));
        }
    }
    return coords;
}

double getTimeDiff(std::chrono::time_point<std::chrono::system_clock> tic)
{
    auto toc = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(toc - tic).count();
}