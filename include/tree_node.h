#pragma once

#include <memory>
#include <stdexcept>
#include <string>

struct TreeNode {
    bool isMerged = false;
    bool isLeaf = false;
    std::string label = "0";
    std::shared_ptr<TreeNode> parent;
    std::shared_ptr<TreeNode> left;
    std::shared_ptr<TreeNode> right;
};

void mergeCherry(std::shared_ptr<TreeNode> node);