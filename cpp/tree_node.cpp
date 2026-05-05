#include <tree_node.h>

#include <iostream>

void TreeNode::setCps() {
    // debug("Calcing cps hash");
    if (isCpsNode()) {
        cpsHash = left->label < right->label ? "("+left->label+","+right->label+")" : "("+right->label+","+left->label+")";
        // debug("new cps hash = " + cpsHash);
    }
}

bool TreeNode::isCpsNode() {
    if (left == nullptr || right == nullptr) {
        return false;
    }
    return left->isLeaf && right->isLeaf;
}

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

void globalMergeCherry(std::shared_ptr<TreeNode> node, MutationTrail* trail) {
    // if (node == nullptr) {return;} // This should be present in the final version, but its easier to debug with this commented out
    
    if (node->isCpsNode()) {
        auto l = node->left;
        auto r = node->right;

        node->isLeaf = true;
        node->label = l->label < r->label ? "("+l->label+","+r->label+")" : "("+r->label+","+l->label+")";

        node->left = nullptr;
        node->right = nullptr;

        l->parent = nullptr;
        r->parent = nullptr;
        if (trail != nullptr) {
            trail->record([l, r, node]() {
                node->isLeaf = false;
                node->label = "0";

                node->left = l;
                node->right = r;

                l->parent = node.get();
                r->parent = node.get();
            });
        }
    }
}