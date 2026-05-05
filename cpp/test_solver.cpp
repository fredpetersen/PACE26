// #include <test_solver.h>

// #include <solver.h>

// int TestSolver::test_lca() {
// 	std::cout << std::endl << "Testing the Lowest Common Ancestor (lca) function" << std::endl;

// 	auto forest = std::make_shared<Forest>();

// 	auto n1 = std::make_shared<TreeNode>();
// 	n1->isLeaf = false;
// 	n1->label = "1";

// 	auto n2 = std::make_shared<TreeNode>();
// 	n2->isLeaf = false;
// 	n2->label = "2";

// 	auto n4 = std::make_shared<TreeNode>();
// 	n4->isLeaf = true;
// 	n4->label = "4";

// 	auto n3 = std::make_shared<TreeNode>();
// 	n3->isLeaf = true;
// 	n3->label = "3";

// 	auto n5 = std::make_shared<TreeNode>();
// 	n5->isLeaf = false;
// 	n5->label = "5";

// 	auto n6 = std::make_shared<TreeNode>();
// 	n6->isLeaf = true;
// 	n6->label = "6";

// 	// Wire up parent/child links
// 	n1->left = n2;
// 	n1->right = n3;
// 	n2->parent = n1;
// 	n2->left = n4;
// 	n4->parent = n2;
// 	n3->parent = n1;
// 	n5->right = n6;
// 	n6->parent = n5;

// 	// Add roots and leaves to forest
// 	forest->setRoots({n1, n5});

// 	forest->setLeaves({n3, n4, n6});
// 	forest->setComponentCount(2);
// 	forest->setLeavesByLabel({{"1", n4}, {"2", n3}, {"3", n6}});

// 	std::pair<std::shared_ptr<TreeNode>, int> expected = {n1, 1};
// 	auto actual = forest->lca(n2->label, n3->label);
// 	std::cout << "Expected: {" << expected.first << ", " << expected.second << "}, Actual: {" << actual.first << ", " << actual.second << "}" << std::endl;

// 	expected = {n1, 2};
// 	actual = forest->lca(n4->label, n3->label);
// 	std::cout << "Expected: {" << expected.first << ", " << expected.second << "}, Actual: {" << actual.first << ", " << actual.second << "}" << std::endl;

// 	expected = {n1, 1};
// 	actual = forest->lca(n4->label, n1->label);
// 	std::cout << "Expected: {" << expected.first << ", " << expected.second << "}, Actual: {" << actual.first << ", " << actual.second << "}" << std::endl;

// 	expected = {nullptr, -1};
// 	actual = forest->lca(n3->label, n6->label);
// 	std::cout << "Expected: {" << expected.first << ", " << expected.second << "}, Actual: {" << actual.first << ", " << actual.second << "}" << std::endl;

// 	return 0;
// }

// int TestSolver::test_contraction() {
// 	std::cout << std::endl << "Testing the Contraction function" << std::endl;

// 	auto v = std::make_shared<TreeNode>();
// 	v->isLeaf = false;
// 	v->label = "1";

// 	auto parent = std::make_shared<TreeNode>();
// 	parent->isLeaf = false;
// 	parent->label = "2";
// 	v->parent = parent;
// 	parent->left = v;

// 	auto child = std::make_shared<TreeNode>();
// 	child->isLeaf = true;
// 	child->label = "3";
// 	child->parent = v;
// 	v->right = child;
// 	auto forest = std::make_shared<Forest>();
// 	forest->getRoots().insert(parent);

// 	forest->print("Before contraction:");
// 	forest->contract(v);
// 	forest->print("After contraction step 1:");
// 	forest->contract(parent);
// 	forest->print("After contraction step 2:");
// 	forest->contract(child);
// 	forest->print("After contraction step 3:");

// 	return 0;
// }

// int TestSolver::testRecursiveContraction() {
// 	auto forest = std::make_shared<Forest>();

// 	auto n1 = std::make_shared<TreeNode>();
// 	n1->isLeaf = false;
// 	n1->label = "1";

// 	auto n2 = std::make_shared<TreeNode>();
// 	n2->isLeaf = false;
// 	n2->label = "2";

// 	auto n3 = std::make_shared<TreeNode>();
// 	n3->isLeaf = false;
// 	n3->label = "3";

// 	auto n4 = std::make_shared<TreeNode>();
// 	n4->isLeaf = true;
// 	n4->label = "4";

// 	auto n8 = nullptr;

// 	auto n7 = nullptr;

// 	auto n5 = std::make_shared<TreeNode>();
// 	n5->isLeaf = false;
// 	n5->label = "5";

// 	auto n9 = nullptr;

// 	auto n6 = std::make_shared<TreeNode>();
// 	n6->isLeaf = true;
// 	n6->label = "6";

// 	// Wire up parent/child links
// 	n1->left = n2;
// 	n1->right = n5;
// 	n2->parent = n1;
// 	n2->left = n3;
// 	n2->right = n7;
// 	n3->parent = n2;
// 	n3->left = n4;
// 	n3->right = n8;
// 	n4->parent = n3;
// 	n5->parent = n1;
// 	n5->left = n9;
// 	n5->right = n6;
// 	n6->parent = n5;

// 	// Add roots and leaves to forest
// 	forest->setRoots({n1});

// 	forest->setLeaves({n4, n6});
// 	forest->setLeavesByLabel({{"4", n4}, {"6", n6}});
// 	forest->setComponentCount(1);

// 	forest->print("Pre-Contraction");

// 	auto lab_u = n4->label;
// 	auto lab_v = n6->label;

// 	std::cout << 1 << std::endl;

// 	auto [ancestor, dist] = forest->lca(lab_u, lab_v);
// 	std::cout << 2 << std::endl;

// 	forest->contractIntoCherry(lab_u, lab_v, ancestor);
// 	std::cout << 3 << std::endl;

// 	forest->print("Post-Contraction");

// 	return 0;
// }

// int TestSolver::test_singelton_leaf() {
// 	std::cout << std::endl << "Testing the Clean Singelton Leaf function" << std::endl;

// 	auto f1 = std::make_shared<Forest>();
// 	auto f2 = std::make_shared<Forest>();

// 	auto t1_l = std::make_shared<TreeNode>();
// 	t1_l->isLeaf = true;
// 	t1_l->label = "1";

// 	auto t1_r = std::make_shared<TreeNode>();
// 	t1_r->isLeaf = true;
// 	t1_r->label = "3";

// 	auto t1_p = std::make_shared<TreeNode>();
// 	t1_p->left = t1_l;
// 	t1_l->parent = t1_p;

// 	t1_p->right = t1_r;
// 	t1_r->parent = t1_p;

// 	auto t1_single1 = std::make_shared<TreeNode>();
// 	t1_single1->isLeaf = true;
// 	t1_single1->label = "2";

// 	auto t1_single2 = std::make_shared<TreeNode>();
// 	t1_single2->isLeaf = true;
// 	t1_single2->label = "4";

// 	f1->setRoots({t1_p, t1_single1, t1_single2});
// 	f1->setLeaves({t1_l, t1_r, t1_single1, t1_single2});
// 	f1->setLeavesByLabel({{"1", t1_l}, {"2", t1_single1}, {"3", t1_r}, {"4", t1_single2}});

// 	f1->print("Forest 1");

// 	auto t2_p = std::make_shared<TreeNode>();

// 	auto t2_l = std::make_shared<TreeNode>();
// 	t2_l->isLeaf = true;
// 	t2_l->label = "2";

// 	auto t2_r1 = std::make_shared<TreeNode>();
// 	t2_r1->isLeaf = true;
// 	t2_r1->label = "3";

// 	auto t2_lp = std::make_shared<TreeNode>();
// 	t2_lp->left = t2_l;
// 	t2_l->parent = t2_lp;

// 	t2_lp->right = t2_r1;
// 	t2_r1->parent = t2_lp;

// 	auto t2_r2 = std::make_shared<TreeNode>();
// 	t2_r2->isLeaf = true;
// 	t2_r2->label = "1";

// 	t2_p->left = t2_lp;
// 	t2_p->right = t2_r2;

// 	auto t2_single1 = std::make_shared<TreeNode>();
// 	t2_single1->isLeaf = true;
// 	t2_single1->label = "4";

// 	f2->setRoots({t2_p, t2_single1});
// 	f2->setLeaves({t2_l, t2_r1, t2_r2, t2_single1});
// 	f2->setLeavesByLabel({{"1", t2_r2}, {"2", t2_l}, {"3", t2_r1}, {"4", t2_single1}});

// 	f2->print("Forest 2");

// 	std::cout << "Cleaning singleton leaves..," << std::endl;

// 	cleanSingletonLeaves(f1, f2);

// 	f1->print("Forest 1");
// 	f2->print("Forest 2");

// 	return 0;
// }

// int TestSolver::testgetLeafByLabel() {
// 	auto forest = std::make_shared<Forest>();

// 	auto n1 = std::make_shared<TreeNode>();
// 	n1->isLeaf = false;
// 	n1->label = "1";

// 	auto n3 = std::make_shared<TreeNode>();
// 	n3->isLeaf = true;
// 	n3->label = "3";

// 	auto n2 = std::make_shared<TreeNode>();
// 	n2->isLeaf = true;
// 	n2->label = "2";

// 	auto n4 = std::make_shared<TreeNode>();
// 	n4->isLeaf = true;
// 	n4->label = "4";

// 	// Wire up parent/child links
// 	n1->left = n3;
// 	n1->right = n2;
// 	n3->parent = n1;
// 	n2->parent = n1;

// 	// Add roots and leaves to forest
// 	forest->addRoot(n1);
// 	forest->addRoot(n4);

// 	forest->addLeaf(n2);
// 	forest->addLeaf(n3);
// 	forest->addLeaf(n4);
// 	forest->setComponentCount(2);
// 	forest->setLeavesByLabel({{"2", n2}, {"3", n3}, {"4", n4}});

// 	std::cout << "Testing getLeafByLabel..." << std::endl;
// 	for (int label = 1; label <= 4; label++) {
// 		auto it = forest->getLeafByLabel(std::to_string(label));
// 	}
// 	return 0;
// }

// int TestSolver::testGetSiblingsPairs() {
// 	auto forest = std::make_shared<Forest>();
// 	if(true) { // Wrapped in if statement to allow collapsing in IDE, as forest construction is a bit verbose
// 		auto n1 = std::make_shared<TreeNode>();
// 		n1->isLeaf = false;
// 		n1->label = "1";

// 		auto n2 = std::make_shared<TreeNode>();
// 		n2->isLeaf = false;
// 		n2->label = "2";

// 		auto n4 = std::make_shared<TreeNode>();
// 		n4->isLeaf = true;
// 		n4->label = "4";

// 		auto n5 = std::make_shared<TreeNode>();
// 		n5->isLeaf = true;
// 		n5->label = "5";

// 		auto n3 = std::make_shared<TreeNode>();
// 		n3->isLeaf = false;
// 		n3->label = "3";

// 		auto n7 = std::make_shared<TreeNode>();
// 		n7->isLeaf = true;
// 		n7->label = "7";

// 		auto n6 = std::make_shared<TreeNode>();
// 		n6->isLeaf = false;
// 		n6->label = "6";

// 		auto n8 = std::make_shared<TreeNode>();
// 		n8->isLeaf = true;
// 		n8->label = "8";

// 		auto n9 = std::make_shared<TreeNode>();
// 		n9->isLeaf = true;
// 		n9->label = "9";

// 		auto n10 = std::make_shared<TreeNode>();
// 		n10->isLeaf = false;
// 		n10->label = "10";

// 		auto n11 = std::make_shared<TreeNode>();
// 		n11->isLeaf = true;
// 		n11->label = "11";

// 		auto n12 = std::make_shared<TreeNode>();
// 		n12->isLeaf = true;
// 		n12->label = "12";

// 		auto n13 = std::make_shared<TreeNode>();
// 		n13->isLeaf = true;
// 		n13->label = "13";

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
// 		forest->setRoots({n1, n10, n13});

// 		forest->setLeaves({n4, n5, n7, n8, n9, n11, n12, n13});

// 		forest->setComponentCount(3);
// 		forest->setLeavesByLabel({{"4", n4}, {"5", n5}, {"7", n7}, {"8", n8}, {"9", n9}, {"11", n11}, {"12", n12}, {"13", n13}});
// 	}
// 	std::cout << "Testing getSiblingLeafPairs..." << std::endl;
// 	auto siblingLeafPairs = forest->getSiblingLeafPairs();
// 	std::cout << "Sibling leaf pairs found: {";
// 	for (const auto& leaf : siblingLeafPairs) {
// 		std::cout << "(" << leaf.first->label << ", " << leaf.second->label << ") ";
// 	}
// 	std::cout << "}" << std::endl;
// 	return 0;
// }

// int TestSolver::test() {
// 	// test_lca();
// 	// test_contraction();
// 	testRecursiveContraction();
// 	// test_singelton_leaf();
// 	// testgetLeafByLabel();
// 	// testGetSiblingsPairs();

// 	return 0;
// }
