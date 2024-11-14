#ifndef _UTILITIES_H_
#define _UTILITIES_H_

#include <utility>
#include <array>
#include <vector>
#include <iostream>
#include <chrono>

using Coord = std::pair<unsigned int, unsigned int>;  // row, column
struct CoordHash {
    std::size_t operator()(const Coord& coord) const {
        return coord.first * 9 + coord.second;
    }
};

struct CoordEqual {
    bool operator()(const Coord& lhs, const Coord& rhs) const {
        return lhs.first == rhs.first && lhs.second == rhs.second;
    }
};

std::ostream& operator<<(std::ostream& os, const Coord coord);
std::ostream& operator<<(std::ostream& os, const std::vector<int> vec);
template <std::size_t N>
void printArray(const std::array<int, N>& arr)
{
    for(int i=0; i<N; ++i)
    {
        int value = arr[i];
        std::cout << (value == 0? " ": std::to_string(value)) << " ";
    }
}
template <std::size_t N, std::size_t M>
void print2DArray(const std::array<std::array<int, M>, N>& arr)
{
    for(int row=0; row<N; ++row)
    {
        for(int col=0; col<M; ++col)
        {
            int value = arr[row][col];
            std::cout << (value == 0? " ": std::to_string(value)) << " ";
            if(col % 3 == 2)
                std::cout << " ";
        }
        if(row % 3 == 2)
            std::cout << "\n";
        std::cout << "\n";
    }
};
std::array<int, 9> flattenBlock(std::array<std::array<int, 3>, 3>);
bool valueInside(std::array<int, 9>, int);
bool valueInside(std::vector<int>, int);
bool eraseValue(std::vector<int>&, int);
bool coordInside(std::vector<Coord> coords, Coord coord);
unsigned int computeBlockId(Coord coord);
std::vector<Coord> getBlockCoords(unsigned int block_id);
std::vector<Coord> getBlockCoords(Coord coord);
double getTimeDiff(std::chrono::time_point<std::chrono::system_clock> tic);

#endif