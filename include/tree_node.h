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
