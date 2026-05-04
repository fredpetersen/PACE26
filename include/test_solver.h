#pragma once

#include <solver.h>

class TestSolver : public Solver {
	public:
		inline TestSolver(std::vector<std::shared_ptr<Forest>> forests, int leafCount)
	: Solver(forests, leafCount) {}

		int test_lca();

		int test_contraction();

		int testRecursiveContraction();

		int test_singelton_leaf();

		int testgetLeafByLabel();

		int testGetSiblingsPairs();

		int test();
};