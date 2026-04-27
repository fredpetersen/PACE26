#include <tree_node.h>

#include <iostream>

void mergeCherry(std::shared_ptr<TreeNode> node) {
    auto l = node->left;
    auto r = node->right;

    if (l->isLeaf && r->isLeaf) {
        node->label = (l->label) < r->label ? l->label : r->label;
        node->newickRep = "(" + std::to_string(l->label) + "," + std::to_string(r->label) + ")";
        node->left = nullptr;
        node->right = nullptr;
        node->isLeaf = true;
    } else {
        std::cout << "Left or Right node not leaf" << std::endl;
    }
}