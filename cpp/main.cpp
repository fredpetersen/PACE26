
#include <iostream>

#include <forest.h>
#include <two_tree_test_solver.h>
#include <two_tree_solver.h>
#include <input_reader.h>

// #include "input_reader.cpp"

int main() {
    std::cout << "# Started Program" << std::endl;

    std::cout << "# Parsing input" << std::endl;
    auto problemInstance = parseInput();
    std::cout << "# Input Parsed" << std::endl;

    std::cout << "# Creating Solver" << std::endl;
    TwoTreeSolver solver(problemInstance.forests[0], problemInstance.forests[1], problemInstance.leafCount);
    // TwoTreeTestSolver solver(problemInstance.forests[0], problemInstance.forests[1], problemInstance.leafCount);

    std::cout << "# Solving" << std::endl;
    auto result = solver.solve();
    // int result = solver.test();
    
    std::cout << "# Solution:" << std::endl;
    result->printForestNewick();
    return 0;
}