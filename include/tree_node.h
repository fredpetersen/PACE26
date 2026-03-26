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

static int removeChild(std::shared_ptr<TreeNode> parent, std::shared_ptr<TreeNode> child) {
    //TODO: Is this really how you assign shared pointers?
    if (parent->left == child) {
        child->parent = nullptr;
        parent->left = nullptr;
    } else if (parent->right == child) {
        child->parent = nullptr;
        parent->right = nullptr;
    } else {
        std::cout << "The given node does not have that child" << std::endl;
        return 1;
    }
    
    return 0;
}

static int getCantorHash(std::shared_ptr<TreeNode> node) {
    if (node->isLeaf) {
        auto l = node->label;
        // Calculates CantorPair(Label, Label + 1). Can't just use label, since labels and cantor pairs only guarantees uniqueness internally
        return ((2*l + 1)*(2*l + 2))/2 + (l+1);
    } else if (node->left->isLeaf && node->right->isLeaf) {
        auto a = node->left->hash;
        auto b = node->right->hash;
        return ((a + b)*(a + b + 1))/2 + b;
    } else {
        std::cout << "Node is neither a leaf, nor the parent of 2 leaves. Please fix arguments." << std::endl;
        throw std::runtime_error("Cantor has not defined for node of this type.");
    }
}