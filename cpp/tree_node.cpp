#include <tree_node.h>

#include <iostream>

/*
This function pretends that the parent of 2 leaves is a leaf, which constitutes a local level merge. This is done in a way that doesn't remove information, since
it will have to be un-merged later.
*/
void mergeCherry(std::shared_ptr<TreeNode> node) {
    auto l = node->left;
    auto r = node->right;

    auto lab_l = l->label;
    auto lab_r = r->label;

    if (l->isLeaf && r->isLeaf) {
        if (lab_r < lab_l) {
            auto tmp = lab_r;
            lab_r = lab_l;
            lab_l = tmp;
        }
        node->label = "(" + lab_l + "," + lab_r + ")";
        node->isLeaf = true;
        node->isMerged = true;
    } else {
        std::cout << "Left or Right node not leaf" << std::endl;
    }
}