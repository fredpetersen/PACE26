
#include <iostream>

#include "two_tree_solver.cpp"
#include "input_reader.cpp"
#include "two_tree_solver_test.cpp"

int main() {
    auto problemInstance = parseInput();

    // TwoTreeSolver solver(problemInstance.forests[0].get(), problemInstance.forests[1].get(), problemInstance.leafCount);
    TwoTreeSolverTest solver(problemInstance.forests[0].get(), problemInstance.forests[1].get(), problemInstance.leafCount);

    // int result = solver.solve();
    int result = solver.test();
    for(int i = 1; i <= problemInstance.leafCount; i++) {
    // Do something with each leaf
        std::cout << i  << ";" << std::endl;
    }
    return 0;
}