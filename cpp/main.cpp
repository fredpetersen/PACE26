
#include <iostream>

#include <forest.h>
#include <test_solver.h>
#include <solver.h>
#include <input_reader.h>

// #include "input_reader.cpp"

int main() {
    // std::cout << "# Started Program" << std::endl;

    // std::cout << "# Parsing input" << std::endl;
    auto problemInstance = parseInput();
    // std::cout << "# Input Parsed" << std::endl;

    // std::cout << "# Creating Solver" << std::endl;
    Solver solver(problemInstance.forests, problemInstance.leafCount, problemInstance.cpsMap);
    // TestSolver solver(problemInstance.forests, problemInstance.leafCount, problemInstance.cpsMap);

    // std::cout << "# Solving" << std::endl;
    auto result = solver.solve();
    // int result = solver.test();
    
    // std::cout << "# Solution:" << std::endl;
    result->printForestNewick();
    return 0;
}