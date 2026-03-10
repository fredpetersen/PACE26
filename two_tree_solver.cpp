#include <cctype>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include <problem_instance.h>
#include <tree_node.h>

#include "input_reader.cpp"

class TwoTreeSolver {
  TreeNode* tree1_;
  TreeNode* tree2_;
  int leafCount_;
public:
    TwoTreeSolver(TreeNode* tree1, TreeNode* tree2, int leafCount)
        : tree1_(tree1), tree2_(tree2), leafCount_(leafCount) {}


    int solve() {

        std::cout << tree1_->left.get()->left.get()->left.get()->parent->right.get()->label << std::endl;
        // Placeholder for the actual solution logic.

        // Merge matching sibling pairs to one leaf

        // Remove vertices that are both root and leaf

        // Contract edges with degree 2 nodes

        // Branch on remaining sibling pairs in tree1
        // // Case 1: u,v are siblings in tree 1 but in different components in tree2
        //
        // // Case 2: u,v are siblings in tree 1, but u is sibling with parent of v in tree2 (or vice versa)

        // // Case 3: u,v are siblings in tree 1, but there are 2 or more pendant subtrees in tree2 between u and v

        return 0;
    }
};

int main() {
    auto problemInstance = parseInput();
    TwoTreeSolver solver(problemInstance.trees[0].get(), problemInstance.trees[1].get(), problemInstance.leafCount);
    int result = solver.solve();
    std::cout << result << std::endl;
    return 0;
}