#ifndef _SOLVER_H
#define _SOLVER_H

#include <unordered_map>
#include <map>
#include <memory>
#include "Problem/Problem.h"
#ifdef OPENMP
#include <mutex>
#endif

namespace Sudoku
{
    #ifdef USE_ORDERED_MAP
    using Aux = std::unordered_map<Coord, std::vector<int>, CoordHash, CoordEqual>;
    #else
    using Aux = std::map<Coord, std::vector<int>>;
    #endif
    using FrequencyMap = std::unordered_map<int, std::pair<int, Coord>>;

    struct TreeNode
    {
        Coord coord;
        Aux aux;
        std::vector<int> candidates;
        std::vector<bool> guessed;
        std::vector<std::pair<Coord, Coord>> common;
        std::shared_ptr<TreeNode> left;
        std::shared_ptr<TreeNode> right;
        std::weak_ptr<TreeNode> parent;

        TreeNode(Coord coord, std::vector<int> candidates,
                Aux aux, std::vector<std::pair<Coord, Coord>> common,
                std::shared_ptr<TreeNode> parent=nullptr)
            : coord(coord), aux(aux), common(common), candidates(candidates), parent(parent)
        {
            #ifdef VERBOSE
            std::cout << "Init node (" << static_cast<void*>(this)
                << ") with " << coord << ", "
                << aux.size() << " aux state, "
                << candidates.size() << " aux and parent "
                << parent.get() << ")\n";
            #endif
            guessed = std::vector<bool>(candidates.size(), false);
        }

        int getGuessedNumber(void)
        {
            for(int i = 0; i < guessed.size(); ++i)
            {
                if(!guessed[i])
                {
                    guessed[i] = true;
                    return candidates[i];
                }
            }
            throw std::runtime_error("All available numbers are guessed");
        }

        bool getAllGuessed(void)
        {
            for(auto is_guessed: guessed)
            {
                if(!is_guessed)
                    return false;
            }
            return true;
        }
    };

    class Solver: public Problem
    {
    public:
        Solver(const char*);
        void setCell(Coord coord, int value);
        void generateAux(Coord coord);
        void removeSameRowAux(Coord coord, std::vector<int> values, std::vector<Coord> excluded_coords={});
        void removeSameColumnAux(Coord coord, std::vector<int> values, std::vector<Coord> excluded_coords={});
        void removeSameBlockAux(Coord coord, std::vector<int> values, std::vector<Coord> excluded_coords={});
        void removeAux(Coord coord, std::vector<int> values, std::vector<Coord> excluded_coords={});
        FrequencyMap countSameRowAuxFreqMap(unsigned int row);
        FrequencyMap countSameColumnAuxFreqMap(unsigned int column);
        FrequencyMap countSameBlockAuxFreqMap(unsigned int block_id);
        void displayAux(void);
        void displayCommonAux(void);
        void displayFrequencyMap(Unit unit, unsigned int number, FrequencyMap map);
        void solve(bool show_status=false);
        void showStatus(void);
        bool isRecordedCell(Coord coord);
        void updateStatus(Coord coord);
        void guess(void);
        void backtrace(void);
        int getIter(void) {return m_iter;}
        int getGuessNum(void) {return m_guess_num;}
        int getBacktraceNum(void) {return m_backtrace_num;}
    private:
        Aux m_aux;
        std::vector<std::pair<Coord, Coord>> m_common_aux;
        int m_iter;
        int m_guess_num;
        int m_backtrace_num;
        std::shared_ptr<TreeNode> m_guessed_root;
        std::shared_ptr<TreeNode> m_guessed_curr;
        #ifdef OPENMP
        std::mutex m_mutex;
        #endif
    };
};
#endif