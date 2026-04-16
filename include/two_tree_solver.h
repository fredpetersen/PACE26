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

class TwoTreeSolver {
  std::shared_ptr<Forest> forest1_;
  std::shared_ptr<Forest> forest2_;
  int leafCount_;
  std::unordered_map<int, int> cantorMap_; //could be <int, byte>

public:
    inline TwoTreeSolver(std::shared_ptr<Forest> forest1, std::shared_ptr<Forest> forest2, int leafCount)
    : forest1_(forest1), forest2_(forest2), leafCount_(leafCount) {}


    void printForests() const;

    void printCantorMap() const;

    void cleanSingletonLeaves(std::shared_ptr<Forest> mainForest, std::shared_ptr<Forest> otherForest); //should use Forest* instead

    void initCantorMap(std::vector<std::shared_ptr<Forest>> forests);

    void contract(std::shared_ptr<TreeNode> v, std::shared_ptr<Forest> forest);

    std::pair<std::shared_ptr<TreeNode>, int> lca(std::shared_ptr<TreeNode> u, std::shared_ptr<TreeNode> v);

    int solve();
    int solve(int k);
    int solve(int k, std::shared_ptr<Forest> forest1, std::shared_ptr<Forest> forest2);
};
