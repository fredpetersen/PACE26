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
    uint64_t cpsHash = 0;
    // Cached structural hash of the subtree rooted at this node. 0 = invalid
    // (must be recomputed). Cleared upward via invalidateSubtreeHash whenever
    // anything below this node changes.
    mutable uint64_t subtreeHash = 0;
    std::string label = "0";

    TreeNode* parent = nullptr;
    TreeNode* left  = nullptr;
    TreeNode* right = nullptr;

    void setCps();
    bool isCpsNode();
};

inline void invalidateSubtreeHash(TreeNode* n) {
    while (n != nullptr && n->subtreeHash != 0) {
        n->subtreeHash = 0;
        n = n->parent;
    }
}

void localMergeCherry(TreeNode* node,  MutationTrail* trail = nullptr);
void globalMergeCherry(TreeNode* node, MutationTrail* trail = nullptr);