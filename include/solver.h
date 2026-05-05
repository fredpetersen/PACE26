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

#include <mutation_trail.h>
#include <problem_instance.h>
#include <tree_node.h>
#include <forest.h>

/* Hashing and equality functions assume that the TreeNodes will always be given in the same order

Thus they are sensitive to the order of the pair, as (u,v) and (v,u) will be considered different pairs
*/

class Solver {
  std::vector<std::shared_ptr<Forest>> forests_;
  int leafCount_;
  std::unordered_map<std::string, int> cpsMap_;

public:
    inline Solver(std::vector<std::shared_ptr<Forest>> forests, int leafCount, std::unordered_map<std::string, int> cpsMap)
    : forests_(forests), leafCount_(leafCount), cpsMap_(cpsMap) {}


    void printForests() const;

    void cleanSingletonLeaves(std::shared_ptr<Forest> mainForest, std::shared_ptr<Forest> otherForest, MutationTrail* trail = nullptr); //should use Forest* instead

    void initCpsReduction();

    std::shared_ptr<Forest> solve();
    std::pair<bool, std::shared_ptr<Forest>> solve(int k);
    std::pair<bool, std::vector<std::shared_ptr<Forest>>> solve(int k, std::vector<std::shared_ptr<Forest>> forests);

private:
  std::pair<bool, std::vector<std::shared_ptr<Forest>>> solveRecursive(int k, std::vector<std::shared_ptr<Forest>> forests, MutationTrail& trail);
};
