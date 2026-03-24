#pragma once

#include <memory>

struct TreeNode {
    bool isLeaf = false;
    int label = 0;
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

static int cantorPair(std::shared_ptr<TreeNode> parent) {
    if (parent->left->isLeaf && parent->right->isLeaf) {
        auto a = parent->left->label;
        auto b = parent->right->label;
        
        // Sorts the children such that a is always less than b
        if (a > b) {
            auto c = a;
            a = b;
            b = c;
        }
        return ((a + b)*(a + b + 1))/2 + b;
    }
    std::cout << "Both children should be leaves, for cantorPair() to be called" << std::endl;
    return 0;
}