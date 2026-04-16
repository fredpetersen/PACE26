
#include <iostream>

#include <forest.h>
#include <two_tree_test_solver.h>
#include <two_tree_solver.h>
#include <input_reader.h>

// #include "input_reader.cpp"

int main() {
    auto problemInstance = parseInput();

    // TwoTreeSolver solver(problemInstance.forests[0], problemInstance.forests[1], problemInstance.leafCount);
    TwoTreeTestSolver solver(problemInstance.forests[0], problemInstance.forests[1], problemInstance.leafCount);

    // int result = solver.solve();
    int result = solver.test();
    
    problemInstance.forests[0]->printForestNewick();
    return 0;
}