#pragma once

#include <memory>

struct TreeNode {
    bool isLeaf = false;
    int label = 0;
    TreeNode* parent = nullptr;
    std::unique_ptr<TreeNode> left;
    std::unique_ptr<TreeNode> right;
};
