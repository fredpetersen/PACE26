#include <tree_node.h>

#include <iostream>

std::shared_ptr<TreeNode> TreeNode::parentShared() const {
    if (!parent) return nullptr;
    return parent->shared_from_this();
}

std::shared_ptr<TreeNode> TreeNode::self() {
    return shared_from_this();
}

/*
This function pretends that the parent of 2 leaves is a leaf, which constitutes a local level merge. This is done in a way that doesn't remove information, since
it will have to be un-merged later.
*/
void localMergeCherry(std::shared_ptr<TreeNode> node) {
    // if (node == nullptr) {return;}
    auto l = node->left;
    auto r = node->right;

    auto lab_l = l->label;
    auto lab_r = r->label;

    if (l->isLeaf && r->isLeaf) {
        node->label = lab_l < lab_r ? "("+lab_l+","+lab_r+")" : "("+lab_r+","+lab_l+")"; // Make sure that the order will be the same for all instances of the sibling pair
        node->isLeaf = true;
        node->isMerged = true;
    } else {
        std::cout << "Left or Right node not leaf" << std::endl;
    }
}

void globalMergeCherry(std::shared_ptr<TreeNode> node, MutationTrail* hashTrail) {
    // if (node == nullptr) {return;}
    if (hashTrail != nullptr) {

    }
}