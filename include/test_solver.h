#pragma once

#include <solver.h>

class TestSolver : public Solver {
	public:
		inline TestSolver(std::vector<std::shared_ptr<Forest>> forests, int leafCount, std::unordered_map<std::string, int> cpsMap)
	: Solver(forests, leafCount, cpsMap) {}

		int test_lca();

		int test_contraction();

		int testRecursiveContraction();

		int test_singelton_leaf();

		int testgetLeafByLabel();

		int testGetSiblingsPairs();

		int test();
};