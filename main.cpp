#include "Utilities/Utilities.h"
#include "Solver/Solver.h"

int main(int argc, char** argv)
{
    Sudoku::Solver p(argv[1]);
    // p.display();
    /*
    p.showStatus();
    std::cout << "==================\n";
    p.solve(true);
    if(p.getSolved())
    {
        p.display();
        std::cout << "Solved after " << p.getIter() << " iterations ";
        if(p.getGuessNum() != 0)
        {
            std::cout << "and " << p.getGuessNum() << " assumptions (";
            std::cout << p.getBacktraceNum() << " backtraces)";
        }
        std::cout << "\n";
    }
    */
    return 0;
}