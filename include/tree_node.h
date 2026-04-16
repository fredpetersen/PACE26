#pragma once

#include <memory>
#include <stdexcept>

struct TreeNode {
    bool isLeaf = false;
    int label = 0;
    long hash = 0;
    std::shared_ptr<TreeNode> parent;
    std::shared_ptr<TreeNode> left;
    std::shared_ptr<TreeNode> right;
};

int getCantorHash(int a, int b);

void setCantorHashOfNode(std::shared_ptr<TreeNode> node);