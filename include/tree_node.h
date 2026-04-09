#pragma once

#include <memory>
#include <stdexcept>

struct TreeNode {
    bool isLeaf = false;
    int label = 0;
    long hash = 0; // the highest leaf count in public test set is 350 -> the cantor hash would be ~ 12b for a parent of nodes 350 and 349. This is past the int limit.
    std::shared_ptr<TreeNode> parent;
    std::shared_ptr<TreeNode> left;
    std::shared_ptr<TreeNode> right;
};

static int getCantorHash(std::shared_ptr<TreeNode> node) {
    if (node->isLeaf) {
        auto l = node->label;
        // Calculates CantorPair(Label, 0). Can't just use label, since labels and cantor pairs only guarantees uniqueness internally
        return (l^2 + l)/2;
    } else if (node->left->isLeaf && node->right->isLeaf) {
        auto a = node->left->hash;
        auto b = node->right->hash;
        // since (a, b) == (b, a), we sort so a > b
        if (b > a) {
            auto c = a;
            a = b;
            b = c;
        }
        return ((a + b)*(a + b + 1))/2 + b;
    } else {
        std::cout << "Node is neither a leaf, nor the parent of 2 leaves. Please fix arguments." << std::endl;
        throw std::runtime_error("Cantor has not defined for node of this type.");
    }
}