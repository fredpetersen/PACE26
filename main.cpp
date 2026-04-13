
#include <iostream>

#include <forest.h>
#include <two_tree_test_solver.h>
#include <two_tree_solver.h>
#include <input_reader.h>

// #include "input_reader.cpp"

int main() {
    auto problemInstance = parseInput();

    TwoTreeSolver solver(problemInstance.forests[0].get(), problemInstance.forests[1].get(), problemInstance.leafCount);
    // TwoTreeTestSolver solver(problemInstance.forests[0].get(), problemInstance.forests[1].get(), problemInstance.leafCount);

    int result = solver.solve();
    // int result = solver.test();
    solver.solve();
    problemInstance.forests[0]->printForestNewick();
    return 0;
}