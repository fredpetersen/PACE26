#pragma once

#include <memory>

struct TreeNode {
    bool isLeaf = false;
    int label = 0;
    std::unique_ptr<TreeNode> left;
    std::unique_ptr<TreeNode> right;
};
