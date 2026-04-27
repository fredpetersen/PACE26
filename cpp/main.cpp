
#include <iostream>

#include <forest.h>
#include <two_tree_test_solver.h>
#include <two_tree_solver.h>
#include <input_reader.h>

// #include "input_reader.cpp"

int main() {
    std::cout << "Starting Program" << std::endl;

    auto problemInstance = parseInput();

    std::cout << "Problem Parsed" << std::endl;
    
    TwoTreeSolver solver(problemInstance.forests[0], problemInstance.forests[1], problemInstance.leafCount);
    // TwoTreeTestSolver solver(problemInstance.forests[0], problemInstance.forests[1], problemInstance.leafCount);

    std::cout << "Solver Created" << std::endl;

    auto result = solver.solve();
    // int result = solver.test();
    
    std::cout << "Solution Found" << std::endl;

    problemInstance.forests[0]->printForestNewick();
    return 0;
}