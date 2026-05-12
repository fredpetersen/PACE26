#pragma once

#include <cctype>
#include <deque>
#include <iostream>
#include <memory>
#include <optional>
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

  // Owned arena for any TreeNodes we allocate during cluster reduction (when
  // we re-parse outer / spliced Newick strings into fresh forests). The
  // original input nodes are owned by the Instance in main().
  std::deque<TreeNode> nodeArena_;

  // Next free integer to use as a placeholder leaf label when a cluster is
  // collapsed in the outer instance. Initialized to leafCount_ + 1 and
  // monotonically increased; never reused so cascaded extractions do not
  // collide with one another.
  int nextPlaceholderLabel_ = 0;

  // Records of cluster extractions, applied in reverse at output time.
  // For each extraction we kept a placeholder leaf in the outer instance;
  // when the outer search finishes, we splice the inner MAF back in: the
  // placeholder gets replaced by `spineNewick`, and `extraNewicks` are
  // added as new components of the final MAF.
  struct ClusterExpansion {
      std::string placeholderLabel;
      std::string spineNewick;
      std::vector<std::string> extraNewicks;
  };
  std::vector<ClusterExpansion> pendingExpansions_;
  // Memoization: maps canonical forest-state hash -> largest k that was already
  // proven infeasible for that exact state. A future visit with budget k' <=
  // cached value can be pruned immediately.
  std::unordered_map<uint64_t, int> failureCache_;

public:
    inline Solver(std::vector<std::shared_ptr<Forest>> forests, int leafCount, std::unordered_map<uint64_t, int> cpsMap)
    : forests_(forests), leafCount_(leafCount), cpsMap_(cpsMap),
      activeForestCount_(static_cast<int>(forests.size())),
      nextPlaceholderLabel_(leafCount + 1) {}

    void printForests() const;

    void cleanSingletonLeaves(std::shared_ptr<Forest> mainForest, std::shared_ptr<Forest> otherForest, MutationTrail* trail = nullptr);

    void initCpsReduction();

    // Phase 1 of cluster reduction: discover and report common clusters.
    // A common cluster is a leaf-set C (1 < |C| < L) such that every active
    // forest contains an internal node whose leaf-set equals C exactly
    // (regardless of internal structure). Diagnostic only — does not mutate
    // any forest. Prints a summary and the smallest few candidates to stderr/
    // stdout under the `# cluster:` prefix.
    void analyzeCommonClusters() const;

    // Phase 2 of cluster reduction. Repeatedly finds the smallest common
    // cluster, extracts it into a fresh sub-Solver (which solves it via
    // recursive solve()), replaces the cluster in this instance with a
    // placeholder leaf, and records what to splice back at output time.
    // Mutates forests_, leafCount_, cpsMap_, activeForestCount_,
    // inactiveForests_, failureCache_. Returns the number of extractions
    // that actually happened.
    int reduceClusters();

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

  // Find the smallest common cluster, if any. Returned struct has the
  // cluster's representative node in each forest (in the same order as
  // forests_, with nullptr for inactive forests).
  struct ClusterCandidate {
      uint64_t leafSetHash;
      int size;
      std::vector<TreeNode*> nodePerForest;
  };
  std::optional<ClusterCandidate> findSmallestCommonCluster() const;

  // Build a Newick string for the subtree at `node`, replacing the entire
  // subtree at `replaceTarget` with a leaf labeled `placeholderLabel`.
  // Used to construct the OUTER tree when extracting a cluster.
  static std::string newickWithReplacement(const TreeNode* node,
                                           const TreeNode* replaceTarget,
                                           const std::string& placeholderLabel);

  // Re-parse a list of Newick strings into a fresh set of forests using
  // nodeArena_ as the storage. Bypasses validateTree (labels can be any
  // positive integer). Updates `outCpsMap` with cherry counts from the
  // newly-built forests (existing entries are preserved).
  std::vector<std::shared_ptr<Forest>> rebuildForestsFromNewicks(
      const std::vector<std::string>& newicks,
      std::unordered_map<uint64_t, int>& outCpsMap);
};
