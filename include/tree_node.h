#pragma once

#include <memory>

struct TreeNode {
    bool isLeaf = false;
    int label = 0;
    std::shared_ptr<TreeNode> parent;
    std::shared_ptr<TreeNode> left;
    std::shared_ptr<TreeNode> right;
};
