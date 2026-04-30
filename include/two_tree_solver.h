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
  std::vector<std::shared_ptr<Forest>> forests_;
  int leafCount_;

public:
    inline TwoTreeSolver(std::vector<std::shared_ptr<Forest>> forests, int leafCount)
    : forests_(forests), leafCount_(leafCount) {}


    void printForests() const;

  void cleanSingletonLeaves(std::shared_ptr<Forest> mainForest, std::shared_ptr<Forest> otherForest, MutationTrail* trail = nullptr); //should use Forest* instead

    std::vector<std::shared_ptr<Forest>> cloneForests(std::vector<std::shared_ptr<Forest>> forests);

    std::shared_ptr<Forest> solve();
    std::pair<bool, std::shared_ptr<Forest>> solve(int k);
    std::pair<bool, std::vector<std::shared_ptr<Forest>>> solve(int k, std::vector<std::shared_ptr<Forest>> forests);

private:
  std::pair<bool, std::vector<std::shared_ptr<Forest>>> solveRecursive(int k, std::vector<std::shared_ptr<Forest>> forests, MutationTrail& trail);
};
