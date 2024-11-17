#include <sstream>
#include <cassert>
#include <algorithm>
#ifdef OPENMP
#include <omp.h>
#endif
#include "Solver.h"
#include "Utilities/Utilities.h"


namespace Sudoku
{
    Solver::Solver(const char* filename)
        :Problem(filename),
        m_iter(0),
        m_guess_num(0),
        m_backtrace_num(0)
    {
        auto tic = std::chrono::system_clock::now();
        auto coords = getUnsolved();
        #ifdef OPENMP
        #pragma omp parallel for
        #endif
        for(size_t i=0; i<coords.size(); ++i)
        {
            Coord coord = coords[i];
            #ifdef OPENMP
            std::lock_guard<std::mutex> guard(m_mutex);
            #endif
            generateAux(coord);
        }
        #ifdef VERBOSE
        std::cout << "[Solver] generateAux: " << getTimeDiff(tic) << " us\n";
        #endif
    }

    void Solver::setCell(Coord coord, int value, bool add_in_stack)
    {
        Problem::setCell(coord, value);
        if(!m_guessed.empty() && value != 0 && add_in_stack)
        {
            Node node = Node(coord, Aux{}, std::vector<std::pair<Coord, Coord>>());
            #ifdef VERBOSE
            std::cout << "Stack add node " << &node << "\n";
            #endif
            m_guessed.push(node);
        }
        /*
         * removeAux may raise exception
         * make sure node is added into stack for backtrace
         */
        if(value != 0)
        {
            removeAux(coord, {value});
            updateStatus(coord);
        }
    }

    void Solver::generateAux(Coord coord)
    {
        if(getCell(coord) != 0)
        {
            throw std::runtime_error("Cell already occupied");
        }
        unsigned int row = coord.first, column = coord.second;
        std::array<int, 9> row_content = getRow(row);
        std::array<int, 9> col_content = getColumn(column);
        std::array<int, 9> block_content = flattenBlock(getBlock(computeBlockId(coord)));
        std::vector<int> aux_values;
        for(int i=1; i<=9; ++i)
        {
            if(!valueInside(row_content, i) &&
               !valueInside(col_content, i) &&
               !valueInside(block_content, i))
            {
                aux_values.push_back(i);
            }
        }
        m_aux[coord] = aux_values;
    }

    void Solver::removeSameRowAux(Coord coord, std::vector<int> values, std::vector<Coord> excluded_coords)
    {
        unsigned int row = coord.first;
        for(auto value: values)
        {
            for(unsigned int col = 0; col < SIZE; ++col)
            {
                if(col == coord.second)
                    continue;
                Coord same_row = Coord(row, col);
                if(coordInside(excluded_coords, same_row))
                    continue;
                if(m_aux.find(same_row) != m_aux.end())
                {
                    std::vector<int>& aux = m_aux[same_row];
                    bool removed = eraseValue(aux, value);
                    #ifdef VERBOSE
                    if(removed)
                        std::cout << "[Row] " << same_row << " remove " << value << "\n";
                    #endif
                    if(aux.size() == 0)
                    {
                        std::stringstream ss;
                        ss << same_row << " without any auxiliary number";
                        throw std::runtime_error(ss.str());
                    }
                }
            }
        }
    }

    void Solver::removeSameColumnAux(Coord coord, std::vector<int> values, std::vector<Coord> excluded_coords)
    {
        unsigned int column = coord.second;
        for(auto value: values)
        {
            for(unsigned int _row = 0; _row < SIZE; ++_row)
            {
                if(_row == coord.first)
                    continue;
                Coord same_col = Coord(_row, column);
                if(coordInside(excluded_coords, same_col))
                    continue;
                if(m_aux.find(same_col) != m_aux.end())
                {
                    std::vector<int>& aux = m_aux[same_col];
                    bool removed = eraseValue(aux, value);
                    #ifdef VERBOSE
                    if(removed)
                        std::cout << "[Column] " << same_col << " remove " << value << "\n";
                    #endif
                    if(aux.size() == 0)
                    {
                        std::stringstream ss;
                        ss << same_col << " without any auxiliary number";
                        throw std::runtime_error(ss.str());
                    }
                }
            }
        }
    }

    void Solver::removeSameBlockAux(Coord coord, std::vector<int> values, std::vector<Coord> excluded_coords)
    {
        for(auto value: values)
        {
            for(auto _coord: getBlockCoords(computeBlockId(coord)))
            {
                if(_coord == coord)
                    continue;
                if(coordInside(excluded_coords, _coord))
                    continue;
                if(m_aux.find(_coord) != m_aux.end())
                {
                    std::vector<int>& aux = m_aux[_coord];
                    bool removed = eraseValue(aux, value);
                    #ifdef VERBOSE
                    if(removed)
                        std::cout << "[Block] " << _coord << " remove " << value << "\n";
                    #endif
                    if(aux.size() == 0)
                    {
                        std::stringstream ss;
                        ss << _coord << " without any auxiliary number";
                        throw std::runtime_error(ss.str());
                    }
                }
            }
        }
    }

    void Solver::removeAux(Coord coord, std::vector<int> values, std::vector<Coord> excluded_coords)
    {
        removeSameRowAux(coord, values, excluded_coords);
        removeSameColumnAux(coord, values, excluded_coords);
        removeSameBlockAux(coord, values, excluded_coords);
    }

    void Solver::solve(bool show_status)
    {
        bool is_block = false;
        while(!is_block && !getSolved())
        {
            is_block = true;
            ++m_iter;
            try
            {
                for(auto it=m_aux.begin(); it!=m_aux.end(); )
                {
                    if(it->second.size() == 1)
                    {
                        is_block = false;
                        Coord coord = it->first;
                        int value = it->second[0];
                        #ifdef VERBOSE
                        std::cout << "Set " << it->first << " to " << value
                            << " (Only one aux)\n";
                        #endif
                        it = m_aux.erase(it);
                        setCell(coord, value);
                    }
                    else if(it->second.size() == 2)  // same row/column/block share two aux numbers
                    {
                        if(isRecordedCell(it->first))
                        {
                            ++it;
                            continue;
                        }
                        unsigned int row = it->first.first, column = it->first.second;
                        // same column
                        for(int _row = 0; _row < SIZE; ++_row)
                        {
                            if(_row == row)
                                continue;
                            auto _it = m_aux.find(Coord(_row, column));
                            if(_it != m_aux.end())
                            {
                                if(m_aux[Coord(_row, column)] == it->second)
                                {
                                    #ifdef VERBOSE
                                    std::cout << it->first << " shares same 2 aux numbers with "
                                        << Coord(_row, column) << " (same column)\n";
                                    #endif
                                    m_common_aux.push_back(std::make_pair(it->first, Coord(_row, column)));
                                    removeSameColumnAux(it->first, it->second, {it->first, Coord(_row, column)});
                                    if(coordInside(getBlockCoords(it->first), Coord(_row, column)))  // same block
                                    {
                                        removeSameBlockAux(it->first, it->second, {it->first, Coord(_row, column)});
                                    }
                                    is_block = false;
                                }
                            }
                        }
                        // same row
                        for(int _col = 0; _col < SIZE; ++_col)
                        {
                            if(_col == column)
                                continue;
                            auto _it = m_aux.find(Coord(row, _col));
                            if(_it != m_aux.end())
                            {
                                if(m_aux[Coord(row, _col)] == it->second && !isRecordedCell(Coord(row, _col)))
                                {
                                    #ifdef VERBOSE
                                    std::cout << it->first << " shares same 2 aux numbers with "
                                        << Coord(row, _col) << " (same row)\n";
                                    #endif
                                    m_common_aux.push_back(std::make_pair(it->first, Coord(row, _col)));
                                    removeSameRowAux(it->first, it->second, {it->first, Coord(row, _col)});
                                    if(coordInside(getBlockCoords(it->first), Coord(row, _col)))  // same block
                                    {
                                        removeSameBlockAux(it->first, it->second, {it->first, Coord(row, _col)});
                                    }
                                    is_block = false;
                                }
                            }
                        }
                        // same block
                        std::vector<Coord> coords = getBlockCoords(it->first);
                        for(auto coord: coords)
                        {
                            if(coord == it->first)
                                continue;
                            auto _it = m_aux.find(coord);
                            if(_it != m_aux.end())
                            {
                                if(m_aux[coord] == it->second && !isRecordedCell(coord))
                                {
                                    #ifdef VERBOSE
                                    std::cout << it->first << " shares same 2 aux numbers with "
                                        << coord << " (same block)\n";
                                    #endif
                                    m_common_aux.push_back(std::make_pair(it->first, coord));
                                    removeSameBlockAux(it->first, it->second, {it->first, coord});
                                    is_block = false;
                                }
                            }
                        }
                        ++it;
                    }
                    else
                    {
                        ++it;
                    }
                }
                // aux number only appear in one cell of row/column/block
                for(int row=0; row<SIZE; ++row)
                {
                    auto freqMap = countSameRowAuxFreqMap(row);
                    #ifdef VERBOSE
                    displayFrequencyMap(Unit::Row, row, freqMap);
                    #endif
                    for(auto pair: freqMap)
                    {
                        if(pair.second.first != 1)
                            continue;
                        #ifdef VERBOSE
                        std::cout << "Set " << pair.second.second << " to " << pair.first
                            << " (One aux in row)\n";
                        #endif
                        setCell(pair.second.second, pair.first);
                        is_block = false;
                    }
                }
                for(int column=0; column<SIZE; ++column)
                {
                    auto freqMap = countSameRowAuxFreqMap(column);
                    #ifdef VERBOSE
                    displayFrequencyMap(Unit::Column, column, freqMap);
                    #endif
                    for(auto pair: freqMap)
                    {
                        if(pair.second.first != 1)
                            continue;
                        #ifdef VERBOSE
                        std::cout << "Set " << pair.second.second << " to " << pair.first
                            << " (One aux in column)\n";
                        #endif
                        setCell(pair.second.second, pair.first);
                        is_block = false;
                    }
                }
                for(unsigned int block_id = 0; block_id<SIZE; ++block_id)
                {
                    auto freqMap = countSameBlockAuxFreqMap(block_id);
                    #ifdef VERBOSE
                    displayFrequencyMap(Unit::Block, block_id, freqMap);
                    #endif
                    for(auto pair: freqMap)
                    {
                        if(pair.second.first == 1)
                        {
                            #ifdef VERBOSE
                            std::cout << "Set " << pair.second.second << " to " << pair.first
                                << " (One aux in block)\n";
                            #endif
                            setCell(pair.second.second, pair.first);
                            is_block = false;
                        }
                        else if(pair.second.first <= 3)
                        {
                            std::vector<Coord> coords;
                            std::vector<Coord> block_coords = getBlockCoords(block_id);
                            for(auto coord: block_coords)
                            {
                                if(m_aux.find(coord) == m_aux.end())
                                    continue;
                                if(valueInside(m_aux[coord], pair.first))
                                    coords.push_back(coord);
                            }
                            unsigned int row = coords[0].first, column = coords[0].second;
                            if(std::all_of(coords.begin(), coords.end(), [row](const Coord& c)
                                {return c.first == row;}))
                            {
                                #ifdef VERBOSE
                                std::cout << "Row " << row << ": " << pair.first << " must appear in block "
                                    << block_id << "\n";
                                #endif
                                removeSameRowAux(coords[0], {pair.first}, coords);
                            }
                            if(std::all_of(coords.begin(), coords.end(), [column](const Coord& c)
                                {return c.second == column;}))
                            {
                                #ifdef VERBOSE
                                std::cout << "Column " << column << " " << pair.first << " must appear in block "
                                    << block_id << "\n";
                                #endif
                                removeSameColumnAux(coords[0], {pair.first}, coords);
                            }
                        }
                    }
                }
            }
            catch(const std::exception& e)
            {
                #ifdef VERBOSE
                std::cout << "Exception: " << e.what() << "\n";
                #endif
                if(m_guess_num == 0)
                {
                    std::cerr << "The quiz may be problematic, please check!\n";
                    exit(-1);
                }
                backtrace();
                #ifdef VERBOSE
                std::cout << "After backtrace, stack top set to " << &m_guessed.top() << "\n";
                #endif
                int guessed_number = m_guessed.top().getGuessedNumber();
                setCell(m_guessed.top().coord, guessed_number, false);
            }
            if(show_status)
                showStatus();
            if(is_block && !getSolved())
            {
                guess();
                is_block = false;
            }
            #ifdef MAX_ITERS
            if(m_iter==MAX_ITERS)
                break;
            #endif
        }
    }

    void Solver::displayAux(void)
    {
        for(auto pair: m_aux)
        {
            std::cout << pair.first << ": " << pair.second << "\n";
        }
    }

    void Solver::displayCommonAux(void)
    {
        if(!m_common_aux.empty())
        {
            std::cout << "Two cells share same auxiliary numbers:\n";
            for(auto pair: m_common_aux)
            {
                std::cout << "\t" << pair.first << ", " << pair.second << "\n";
            }
        }
    }

    void Solver::displayGuessHistory(void)
    {
        if(m_guessed.size() == 0)
            return;
        std::stack<Node> guess_history(m_guessed);
        std::vector<Coord> history;
        while(!guess_history.empty())
        {
            while(guess_history.top().candidates.empty())
                guess_history.pop();
            history.push_back(guess_history.top().coord);
            guess_history.pop();
        }
        std::reverse(history.begin(), history.end());
        std::cout << "Guess history: \n";
        for(auto coord: history)
        {
            std::cout << "  " << coord << ": " << getCell(coord) << "\n";
        }
    }

    void Solver::showStatus(void)
    {
        std::cout << "==================\n";
        std::cout << "Iteration: " << m_iter;
        if(m_guess_num != 0)
            std::cout << ", guess num: " << m_guess_num;
        std::cout << "\n";
        display();
        #ifdef VERBOSE
        displayAux();
        displayCommonAux();
        #endif
        std::cout << "==================\n";
    }

    bool Solver::isRecordedCell(Coord coord)
    {
        if(m_common_aux.empty())
            return false;
        for(auto pair: m_common_aux)
        {
            if(coord == pair.first || coord == pair.second)
                return true;
        }
        return false;
    }

    void Solver::updateStatus(Coord coord)
    {
        if(m_aux.find(coord) != m_aux.end())
        {
            m_aux.erase(m_aux.find(coord));
        }
        if(isRecordedCell(coord))
        {
            for(auto it=m_common_aux.begin(); it!=m_common_aux.end();)
            {
                if(it->first == coord || it->second == coord)
                {
                    #ifdef VERBOSE
                    std::cout << "[Two cells share two same aux] Remove "
                        << it->first << " and " << it->second << "\n";
                    #endif
                    m_common_aux.erase(it);
                    break;
                }
                else
                    ++it;
            }
        }
    }

    FrequencyMap Solver::countSameRowAuxFreqMap(unsigned int row)
    {
        FrequencyMap freqMap;
        for(int column=0; column<SIZE; ++column)
        {
            if(m_aux.find(Coord(row, column)) == m_aux.end())
                continue;
            for(auto aux: m_aux[Coord(row, column)])
            {
                if(freqMap.find(aux) != freqMap.end())
                {
                    int count = freqMap[aux].first;
                    ++count;
                    freqMap[aux] = std::make_pair(count, Coord(row, column));
                }
                else
                    freqMap[aux] = std::make_pair(1, Coord(row, column));
            }
        }
        return freqMap;
    }

    FrequencyMap Solver::countSameColumnAuxFreqMap(unsigned int column)
    {
        FrequencyMap freqMap;
        for(int row=0; row<SIZE; ++row)
        {
            if(m_aux.find(Coord(row, column)) == m_aux.end())
                continue;
            for(auto aux: m_aux[Coord(row, column)])
            {
                if(freqMap.find(aux) != freqMap.end())
                {
                    int count = freqMap[aux].first;
                    ++count;
                    freqMap[aux] = std::make_pair(count, Coord(row, column));
                }
                else
                    freqMap[aux] = std::make_pair(1, Coord(row, column));
            }
        }
        return freqMap;
    }

    FrequencyMap Solver::countSameBlockAuxFreqMap(unsigned int block_id)
    {
        FrequencyMap freqMap;
        std::vector<Coord> coords = getBlockCoords(block_id);
        for(auto coord: coords)
        {
            if(m_aux.find(coord) == m_aux.end())
                continue;
            for(auto aux: m_aux[coord])
            {
                if(freqMap.find(aux) != freqMap.end())
                {
                    int count = freqMap[aux].first;
                    ++count;
                    freqMap[aux] = std::make_pair(count, coord);
                }
                else
                    freqMap[aux] = std::make_pair(1, coord);
            }
        }
        return freqMap;
    }

    void Solver::displayFrequencyMap(Unit unit, unsigned int number, FrequencyMap map)
    {
        if(map.empty())
            return;
        switch(unit)
        {
            case Unit::Row:
                std::cout << "Row ";
                break;

            case Unit::Column:
                std::cout << "Column ";
                break;

            case Unit::Block:
                std::cout << "Block ";
                break;
        }
        std::cout << number << " auxiliary numbers occurrence:\n";
        for(auto pair: map)
        {
            std::cout << "    | " << pair.first << " | "
                << pair.second.first << " | "
                << pair.second.second << " |\n";
        }
    }

    void Solver::guess(void)
    {
        #ifdef VERBOSE
        std::cout << "Stuck after " << m_iter << " iterations, starts guessing...\n";
        #endif
        ++m_guess_num;
        Coord coord;
        if(m_common_aux.size() != 0)
        {
            coord = m_common_aux[0].first;
        }
        else
        {
            int cnt = 2;
            bool found = false;
            while(!found)
            {
                for(auto pair: m_aux)
                {
                    if(pair.second.size() == cnt)
                    {
                        coord = pair.first;
                        found = true;
                        break;
                    }
                }
                ++cnt;
            }
        }
        Node node = Node(coord, m_aux, m_common_aux);
        m_guessed.push(node);
        #ifdef VERBOSE
        std::cout << "Stack add node " << &node << "\n";
        #endif
        int guessed_number = node.getGuessedNumber();
        setCell(coord, guessed_number, false);
    }

    void Solver::backtrace(void)
    {
        ++m_backtrace_num;
        #ifdef VERBOSE
        std::cout << "Incorrect assumption, start recovering from " << &m_guessed.top() << " ...\n";
        #endif
        while(!m_guessed.empty())
        {
            Node node = m_guessed.top();
            setCell(node.coord, 0);
            #ifdef VERBOSE
            std::cout << "[Backtrace] " << node.coord << " reset to 0\n";
            #endif
            if(node.candidates.size() != 0)
            {
                bool allGuessed = node.getAllGuessed();
                if(!allGuessed)
                {
                    break;
                }
            }
            m_guessed.pop();
        }
        m_aux = m_guessed.top().aux;
        m_common_aux = m_guessed.top().common;
        #ifdef VERBOSE
        std::cout << "[Backtrace] Status: \n";
        showStatus();
        #endif
    }
}