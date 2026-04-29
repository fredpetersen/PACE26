#pragma once

#include <two_tree_solver.h>

class TwoTreeTestSolver : public TwoTreeSolver {
	public:
		inline TwoTreeTestSolver(std::vector<std::shared_ptr<Forest>> forests, int leafCount)
	: TwoTreeSolver(forests, leafCount) {}

		int test_lca();

		int test_contraction();

		int testRecursiveContraction();

		int test_singelton_leaf();

		int testgetLeafByLabel();

		int testGetSiblingsPairs();

		int test();
};