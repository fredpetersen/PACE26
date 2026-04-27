#include <tree_node.h>

#include <iostream>
/*
This function pretends that the parent of 2 leaves is a leaf, which constitutes a local level merge. This is done in a way that doesn't remove information, since
it will have to be un-merged later.
*/
void mergeCherry(std::shared_ptr<TreeNode> node) {
    auto l = node->left;
    auto r = node->right;

    if (l->isLeaf && r->isLeaf) {
        node->label = l->label + "_" + r->label;
        node->isLeaf = true;
        node->isMerged = true;
    } else {
        std::cout << "Left or Right node not leaf" << std::endl;
    }
}