#include "two_tree_solver.cpp"

class TwoTreeSolverTest : public TwoTreeSolver {
	public:
		TwoTreeSolverTest(Forest* forest1, Forest* forest2, int leafCount)
			: TwoTreeSolver(forest1, forest2, leafCount){}

		// TODO: Rewrite tests so they work with new Forest class

		// int test_lca(std::shared_ptr<TreeNode> tree1_, std::shared_ptr<TreeNode> tree2_) {

		// 	//1
		// 	auto [ancestor, dist] = lca(tree1_->left, tree2_->right);
		// 	std::cout << "Dist measured = "<< dist << ", expected = -1" << std::endl << std::endl;

		// 	//2
		// 	dist = lca(tree1_, tree1_).second;
		// 	std::cout << "Dist measured = "<< dist << ", expected = 0" << std::endl << std::endl;

		// 	//3
		// 	dist = lca(tree1_->left, tree1_->left).second;
		// 	std::cout << "Dist measured = "<< dist << ", expected = 0" << std::endl << std::endl;

		// 	//4
		// 	dist = lca(tree1_->left, tree1_->left->left).second;
		// 	std::cout << "Dist measured = "<< dist << ", expected = 1" << std::endl << std::endl;

		// 	//5
		// 	dist = lca(tree1_->left, tree1_->right).second;
		// 	std::cout << "Dist measured = "<< dist << ", expected = 1" << std::endl << std::endl;

		// 	//6
		// 	dist = lca(tree1_->left, tree1_->right->right).second;
		// 	std::cout << "Dist measured = "<< dist << ", expected = 2" << std::endl << std::endl;

		// 	//7
		// 	dist = lca(tree1_->left->left, tree1_->right).second;
		// 	std::cout << "Dist measured = "<< dist << ", expected = 2" << std::endl << std::endl;

		// 	return 0;
		// }

		// int test_contraction() {
		// 	std::cout << std::endl << "Testing the Contraction function" << std::endl;

		// 	auto v = std::make_shared<TreeNode>();
		// 	v->isLeaf = false;
		// 	v->label = 1;

		// 	auto parent = std::make_shared<TreeNode>();
		// 	parent->isLeaf = false;
		// 	parent->label = 2;
		// 	v->parent = parent;
		// 	parent->left = v;

		// 	auto child = std::make_shared<TreeNode>();
		// 	child->isLeaf = true;
		// 	child->label = 3;
		// 	child->parent = v;
		// 	v->right = child;
		// 	auto forest = std::make_shared<Forest>();
		// 	forest->roots.insert(parent);

		// 	printForest(forest.get(), "Before contraction:");
		// 	contract(v, forest);
		// 	printForest(forest.get(), "After contraction step 1:");
		// 	contract(parent, forest);
		// 	printForest(forest.get(), "After contraction step 2:");
		// 	contract(child, forest);
		// 	printForest(forest.get(), "After contraction step 3:");

		// 	return 0;
		// }

		// int test_singelton_leaf() {
		// 	std::cout << std::endl << "Testing the Clean Singelton Leaf function" << std::endl;

		// 	auto f1 = std::make_shared<Forest>();
		// 	auto f2 = std::make_shared<Forest>();

		// 	auto t1_l = std::make_shared<TreeNode>();
		// 	t1_l->isLeaf = true;
		// 	t1_l->label = 1;

		// 	auto t1_r = std::make_shared<TreeNode>();
		// 	t1_r->isLeaf = true;
		// 	t1_r->label = 3;

		// 	auto t1_p = std::make_shared<TreeNode>();
		// 	t1_p->left = t1_l;
		// 	t1_l->parent = t1_p;

		// 	t1_p->right = t1_r;
		// 	t1_r->parent = t1_p;

		// 	auto t1_single1 = std::make_shared<TreeNode>();
		// 	t1_single1->isLeaf = true;
		// 	t1_single1->label = 2;

		// 	auto t1_single2 = std::make_shared<TreeNode>();
		// 	t1_single2->isLeaf = true;
		// 	t1_single2->label = 4;

		// 	f1->roots = {t1_p, t1_single1, t1_single2};
		// 	f1->leaves = {t1_l, t1_r, t1_single1, t1_single2};
		// 	f1->leafByLabel = {{1, t1_l}, {2, t1_single1}, {3, t1_r}, {4, t1_single2}};

		// 	printForest(f1.get(), "Forest 1");

		// 	auto t2_p = std::make_shared<TreeNode>();

		// 	auto t2_l = std::make_shared<TreeNode>();
		// 	t2_l->isLeaf = true;
		// 	t2_l->label = 2;

		// 	auto t2_r1 = std::make_shared<TreeNode>();
		// 	t2_r1->isLeaf = true;
		// 	t2_r1->label = 3;

		// 	auto t2_lp = std::make_shared<TreeNode>();
		// 	t2_lp->left = t2_l;
		// 	t2_l->parent = t2_lp;

		// 	t2_lp->right = t2_r1;
		// 	t2_r1->parent = t2_lp;

		// 	auto t2_r2 = std::make_shared<TreeNode>();
		// 	t2_r2->isLeaf = true;
		// 	t2_r2->label = 1;

		// 	t2_p->left = t2_lp;
		// 	t2_p->right = t2_r2;

		// 	auto t2_single1 = std::make_shared<TreeNode>();
		// 	t2_single1->isLeaf = true;
		// 	t2_single1->label = 4;

		// 	f2->roots = {t2_p, t2_single1};
		// 	f2->leaves = {t2_l, t2_r1, t2_r2, t2_single1};
		// 	f2->leafByLabel = {{1, t2_r2}, {2, t2_l}, {3, t2_r1}, {4, t2_single1}};

		// 	printForest(f2.get(), "Forest 2");

		// 	std::cout << "Cleaning singleton leaves..," << std::endl;

		// 	cleanSingletonLeaves(f1, f2);

		// 	printForest(f1.get(), "Forest 1");
		// 	printForest(f2.get(), "Forest 2");

		// 	return 0;
		// }

		// int testgetLeafByLabel() {
		// 	auto forest = std::make_shared<Forest>();

		// 	auto n1 = std::make_shared<TreeNode>();
		// 	n1->isLeaf = false;
		// 	n1->label = 1;

		// 	auto n3 = std::make_shared<TreeNode>();
		// 	n3->isLeaf = true;
		// 	n3->label = 3;

		// 	auto n2 = std::make_shared<TreeNode>();
		// 	n2->isLeaf = true;
		// 	n2->label = 2;

		// 	auto n4 = std::make_shared<TreeNode>();
		// 	n4->isLeaf = true;
		// 	n4->label = 4;

		// 	// Wire up parent/child links
		// 	n1->left = n3;
		// 	n1->right = n2;
		// 	n3->parent = n1;
		// 	n2->parent = n1;

		// 	// Add roots and leaves to forest
		// 	forest->roots.insert(n1);
		// 	forest->roots.insert(n4);

		// 	forest->leaves.insert(n2);
		// 	forest->leaves.insert(n3);
		// 	forest->leaves.insert(n4);
		// 	forest->componentCount = 2;
		// 	forest->leafByLabel = {{2, n2}, {3, n3}, {4, n4}};

		// 	std::cout << "Testing getLeafByLabel..." << std::endl;
		// 	for (int label = 1; label <= 4; label++) {
		// 		auto it = forest->leafByLabel.find(label);
		// 		if (it != forest->leafByLabel.end()) {
		// 			std::cout << "Leaf with label " << label << " found at address " << it->second.get() << std::endl;
		// 		} else {
		// 			std::cout << "Leaf with label " << label << " not found in leafByLabel map" << std::endl;
		// 		}
		// 	}
		// 	return 0;
		// }

		// int testGetSiblingsPairs() {
		// 	auto forest = std::make_shared<Forest>();
		// 	if(true) { // Wrapped in if statement to allow collapsing in IDE, as forest construction is a bit verbose
		// 		auto n1 = std::make_shared<TreeNode>();
		// 		n1->isLeaf = false;
		// 		n1->label = 1;

		// 		auto n2 = std::make_shared<TreeNode>();
		// 		n2->isLeaf = false;
		// 		n2->label = 2;

		// 		auto n4 = std::make_shared<TreeNode>();
		// 		n4->isLeaf = true;
		// 		n4->label = 4;

		// 		auto n5 = std::make_shared<TreeNode>();
		// 		n5->isLeaf = true;
		// 		n5->label = 5;

		// 		auto n3 = std::make_shared<TreeNode>();
		// 		n3->isLeaf = false;
		// 		n3->label = 3;

		// 		auto n7 = std::make_shared<TreeNode>();
		// 		n7->isLeaf = true;
		// 		n7->label = 7;

		// 		auto n6 = std::make_shared<TreeNode>();
		// 		n6->isLeaf = false;
		// 		n6->label = 6;

		// 		auto n8 = std::make_shared<TreeNode>();
		// 		n8->isLeaf = true;
		// 		n8->label = 8;

		// 		auto n9 = std::make_shared<TreeNode>();
		// 		n9->isLeaf = true;
		// 		n9->label = 9;

		// 		auto n10 = std::make_shared<TreeNode>();
		// 		n10->isLeaf = false;
		// 		n10->label = 10;

		// 		auto n11 = std::make_shared<TreeNode>();
		// 		n11->isLeaf = true;
		// 		n11->label = 11;

		// 		auto n12 = std::make_shared<TreeNode>();
		// 		n12->isLeaf = true;
		// 		n12->label = 12;

		// 		auto n13 = std::make_shared<TreeNode>();
		// 		n13->isLeaf = true;
		// 		n13->label = 13;

		// 		// Wire up parent/child links
		// 		n1->left = n2;
		// 		n1->right = n3;
		// 		n2->parent = n1;
		// 		n2->left = n4;
		// 		n2->right = n5;
		// 		n4->parent = n2;
		// 		n5->parent = n2;
		// 		n3->parent = n1;
		// 		n3->left = n7;
		// 		n3->right = n6;
		// 		n7->parent = n3;
		// 		n6->parent = n3;
		// 		n6->left = n8;
		// 		n6->right = n9;
		// 		n8->parent = n6;
		// 		n9->parent = n6;
		// 		n10->left = n11;
		// 		n10->right = n12;
		// 		n11->parent = n10;
		// 		n12->parent = n10;

		// 		// Add roots and leaves to forest
		// 		forest->roots.insert(n1);
		// 		forest->roots.insert(n10);
		// 		forest->roots.insert(n13);

		// 		forest->leaves.insert(n4);
		// 		forest->leaves.insert(n5);
		// 		forest->leaves.insert(n7);
		// 		forest->leaves.insert(n8);
		// 		forest->leaves.insert(n9);
		// 		forest->leaves.insert(n11);
		// 		forest->leaves.insert(n12);
		// 		forest->leaves.insert(n13);
		// 		forest->componentCount = 3;
		// 		forest->leafByLabel = {{4, n4}, {5, n5}, {7, n7}, {8, n8}, {9, n9}, {11, n11}, {12, n12}, {13, n13}};
		// 	}
		// 	std::cout << "Testing getSiblingLeafPairs..." << std::endl;
		// 	auto siblingLeafPairs = getSiblingLeafPairs(forest.get());
		// 	std::cout << "Sibling leaf pairs found: {";
		// 	for (const auto& leaf : siblingLeafPairs) {
		// 		std::cout << "(" << leaf.first->label << ", " << leaf.second->label << ") ";
		// 	}
		// 	std::cout << "}" << std::endl;
		// 	return 0;
		// }

		int test() {
			printForests();

			// auto tree1_ = *forest1_->roots.begin();
			// auto tree2_ = *forest2_->roots.begin();

			

			// std::cout << "Leaves in forest 1:" << std::endl;
			// for (const auto& leaf : forest1_->leaves) {
			//     std::cout << leaf->label << " ";
			// }
			// std::cout << std::endl << std::endl;

			// std::cout << "Leaves in forest 2:" << std::endl;
			// for (const auto& leaf : forest2_->leaves) {
			//     std::cout << leaf->label << " ";
			// }
			// std::cout << std::endl;

			// test_lca(tree1_, tree2_);
			// test_contraction();

			// test_singelton_leaf();
			// testgetLeafByLabel();
			// testGetSiblingsPairs();

			return 0;
		}
};