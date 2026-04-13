#pragma once

#include <cctype>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <problem_instance.h>
#include <tree_node.h>
#include <forest.h>

/* Hashing and equality functions assume that the TreeNodes will always be given in the same order

Thus they are sensitive to the order of the pair, as (u,v) and (v,u) will be considered different pairs
*/
struct SiblingPairHash {
    size_t operator()(const std::pair<std::shared_ptr<TreeNode>, std::shared_ptr<TreeNode>>& p) const noexcept;
};

struct SiblingPairEq {
    bool operator()(const std::pair<std::shared_ptr<TreeNode>, std::shared_ptr<TreeNode>>& a,
                    const std::pair<std::shared_ptr<TreeNode>, std::shared_ptr<TreeNode>>& b) const noexcept;
};

class TwoTreeSolver {
  Forest* forest1_;
  Forest* forest2_;
  int leafCount_;

public:
    inline TwoTreeSolver(Forest* forest1, Forest* forest2, int leafCount)
    : forest1_(forest1), forest2_(forest2), leafCount_(leafCount) {}


    void printForests() const;

    void cleanSingletonLeaves(std::shared_ptr<Forest> mainForest, std::shared_ptr<Forest> otherForest);

    void contract(std::shared_ptr<TreeNode> v, std::shared_ptr<Forest> forest);

    std::pair<std::shared_ptr<TreeNode>, int> lca(std::shared_ptr<TreeNode> u, std::shared_ptr<TreeNode> v);

    std::unordered_set<
        std::pair<std::shared_ptr<TreeNode>, std::shared_ptr<TreeNode>>,
        SiblingPairHash,SiblingPairEq
    > getSiblingLeafPairs(Forest* forest);

    int solve();
    int solve(int k);
    int solve(int k, Forest* forest1, Forest* forest2);
};
