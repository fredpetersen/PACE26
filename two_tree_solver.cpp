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
  std::unique_ptr<TreeNode> tree1_;
  std::unique_ptr<TreeNode> tree2_;
  int leafCount_;
public:
    TwoTreeSolver(std::unique_ptr<TreeNode> tree1, std::unique_ptr<TreeNode> tree2, int leafCount)
        : tree1_(std::move(tree1)), tree2_(std::move(tree2)), leafCount_(leafCount) {}


    int solve() {
        auto instance = parseInput();
        

        // Placeholder for the actual solution logic.

        // Merge matching sibling pairs to one leaf

        // Remove vertices that are both root and leaf

        // Contract edges with degree 2 nodes

        // Branch on remaining sibling pairs in tree1

        // Case 1: u,v are siblings in tree 1 but in different components in tree2

        // Case 2: u,v are siblings in tree 1, but u is sibling with parent of v in tree2 (or vice versa)

        // Case 3: u,v are siblings in tree 1, but there are 2 or more pendant subtrees in tree2 between u and v

        return 0;
    }
};