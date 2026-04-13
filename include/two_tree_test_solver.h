#pragma once

#include <two_tree_solver.h>

class TwoTreeTestSolver : public TwoTreeSolver {
	public:
		inline TwoTreeTestSolver(Forest* forest1, Forest* forest2, int leafCount)
	: TwoTreeSolver(forest1, forest2, leafCount) {}

		int test_lca();

		int test_contraction();

		int test_singelton_leaf();

		int testgetLeafByLabel();

		int testGetSiblingsPairs();

		int test();
};