#pragma once

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>

#include <utils.h>
#include <mutation_trail.h>

struct TreeNode {
    bool isMerged = false;
    bool isLeaf = false;
    // Cached structural hash of the subtree rooted at this node. 0 = invalid
    // (must be recomputed). Cleared upward via invalidateSubtreeHash whenever
    // anything below this node changes. This is the single source of truth
    // for both common-pendant-subtree (CPS) reduction and the failure cache.
    mutable uint64_t subtreeHash = 0;
    std::string label = "0";

    TreeNode* parent = nullptr;
    TreeNode* left  = nullptr;
    TreeNode* right = nullptr;

    bool isCpsNode();
};

inline void invalidateSubtreeHash(TreeNode* n) {
    while (n != nullptr && n->subtreeHash != 0) {
        n->subtreeHash = 0;
        n = n->parent;
    }
}

// Recursive symmetric structural hash, memoized in n->subtreeHash. (a,b) and
// (b,a) hash identically. Returns 0 for nullptr.
uint64_t hashSubtree(const TreeNode* n);

// Order-independent hash of the LEAF SET below n (XOR of per-leaf atoms).
// Two subtrees with the same leaf set hash equally regardless of internal
// structure. Returns 0 for nullptr or empty subtrees. Not memoized; intended
// for one-shot preprocessing passes (e.g. cluster discovery).
uint64_t hashLeafSet(const TreeNode* n);

// Number of leaves in the subtree rooted at n.
int countLeaves(const TreeNode* n);

void localMergeCherry(TreeNode* node,  MutationTrail* trail = nullptr);
void globalMergeCherry(TreeNode* node, MutationTrail* trail = nullptr);
// Generalized variant: collapses an arbitrary internal subtree rooted at
// `node` into a single merged-leaf node whose label is `newickLabel` (the
// caller supplies the Newick string so we don't have to drag forest helpers
// in here). Records the inverse on the trail. The two subtrees rooted at
// node->left / node->right remain in memory (their parent pointers are
// cleared) so the trail can restore them on rollback.
void globalMergeSubtree(TreeNode* node, std::string newickLabel, MutationTrail* trail = nullptr);