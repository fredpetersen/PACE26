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

#include <utils.h>
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
  std::unordered_map<uint64_t, int> cpsMap_;
  // Number of forests still considered active. Step 4 may logically remove
  // a forest from the search; cpsMap_ entries are decremented accordingly
  // so tryCpsReductionForHash compares against the active count rather than
  // the original forest count.
  int activeForestCount_ = 0;
  // Forests that have been deactivated by an enclosing Step 4 frame.
  // cpsReductionForCpsHash skips these so we don't mutate them.
  std::unordered_set<Forest*> inactiveForests_;
  // Memoization: maps canonical forest-state hash -> largest k that was already
  // proven infeasible for that exact state. A future visit with budget k' <=
  // cached value can be pruned immediately.
  std::unordered_map<uint64_t, int> failureCache_;

public:
    inline Solver(std::vector<std::shared_ptr<Forest>> forests, int leafCount, std::unordered_map<uint64_t, int> cpsMap)
    : forests_(forests), leafCount_(leafCount), cpsMap_(cpsMap),
      activeForestCount_(static_cast<int>(forests.size())) {}

    void printForests() const;

    void cleanSingletonLeaves(std::shared_ptr<Forest> mainForest, std::shared_ptr<Forest> otherForest, MutationTrail* trail = nullptr);

    void initCpsReduction();

    void tryCpsReductionForHash(uint64_t cpsHash, MutationTrail* trail = nullptr);

    void cpsReductionForCpsHash(uint64_t cpsHash, MutationTrail* trail = nullptr);

    void detachByLabel(std::shared_ptr<Forest> forest, std::string label, MutationTrail* trail = nullptr);
    void detachChild(std::shared_ptr<Forest> forest, TreeNode* node, bool shouldContract = true, MutationTrail* trail = nullptr);

    // Logically remove `f` from the active forest set: decrements cpsMap_
    // for every still-consistent nodeByCps entry of `f`, decrements the
    // active forest count, marks `f` inactive, and re-fires CPS reduction
    // for any hash whose count just dropped to the new active count. All
    // mutations recorded on the trail.
    void deactivateForest(std::shared_ptr<Forest> f, MutationTrail& trail);

    std::shared_ptr<Forest> solve();
    std::pair<bool, std::shared_ptr<Forest>> solve(int k);
    std::pair<bool, std::vector<std::shared_ptr<Forest>>> solve(int k, const std::vector<std::shared_ptr<Forest>>& forests);

private:
  std::pair<bool, std::vector<std::shared_ptr<Forest>>> solveRecursive(int k, const std::vector<std::shared_ptr<Forest>>& forests, MutationTrail& trail);
};
