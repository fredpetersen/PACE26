
#include <iostream>

// #include <forest.h> //Will update everything so its done properly. Then it should be .h files not .cpp

#include "forest.cpp"
#include "input_reader.cpp"
#include "two_tree_solver_test.cpp"

int main() {
    auto problemInstance = parseInput();

    // TwoTreeSolver solver(problemInstance.forests[0].get(), problemInstance.forests[1].get(), problemInstance.leafCount);
    TwoTreeSolverTest solver(problemInstance.forests[0].get(), problemInstance.forests[1].get(), problemInstance.leafCount);

    // int result = solver.solve();
    int result = solver.test();
    solver.solve();
    problemInstance.forests[0]->printForestNewick();
    return 0;
}