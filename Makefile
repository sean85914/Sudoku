CXX = g++-14
INCLUDE_DIRS = -I.
CXXFLAGS = --std=c++11
OBJ = main.o Utilities/utilities.o Problem/Problem.o Solver/Solver.o

ifdef VERBOSE
    CXXFLAGS += -DVERBOSE
endif
ifdef MAX_ITERS
	CXXFLAGS += -DMAX_ITERS=$(MAX_ITERS)
endif
ifdef OPENMP
	INCLUDE_DIRS += -I/opt/homebrew/opt/libomp/include
	CXXFLAGS += -fopenmp -L/opt/homebrew/opt/libomp/lib -lomp
endif

# 目標規則
main: $(OBJ)
	$(CXX) $(INCLUDE_DIRS) -o $@ $(OBJ) $(CXXFLAGS)

main.o: main.cpp
	$(CXX) $(INCLUDE_DIRS) -c main.cpp $(CXXFLAGS)

Utilities/utilities.o: Utilities/utilities.cpp
	$(CXX) $(INCLUDE_DIRS) -c Utilities/utilities.cpp -o $@ $(CXXFLAGS)

Problem/Problem.o: Problem/Problem.cpp
	$(CXX) $(INCLUDE_DIRS) -c Problem/Problem.cpp -o $@ $(CXXFLAGS)

Solver/Solver.o: Solver/Solver.cpp
	$(CXX) $(INCLUDE_DIRS) -c Solver/Solver.cpp -o $@ $(CXXFLAGS)

clean:
	rm -f main $(OBJ)