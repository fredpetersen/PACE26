#include <tree_node.h>

#include <iostream>

void TreeNode::setCps() {
    if (isCpsNode()) {
        cpsHash = left->label < right->label ? "("+left->label+","+right->label+")" : "("+right->label+","+left->label+")";
    }
}

bool TreeNode::isCpsNode() {
    return left != nullptr &&
        right != nullptr &&
        isMerged == false &&
        left->isLeaf == true &&
        right->isLeaf == true;
}

void localMergeCherry(TreeNode* node, MutationTrail* trail) {
    auto l = node->left;
    auto r = node->right;

    auto lab_l = l->label;
    auto lab_r = r->label;

    if (l->isLeaf && r->isLeaf) {
        auto oldLabel = node->label;
        auto oldIsLeaf = node->isLeaf;
        auto oldIsMerged = node->isMerged;

        node->label = lab_l < lab_r ? "("+lab_l+","+lab_r+")" : "("+lab_r+","+lab_l+")";
        node->isLeaf = true;
        node->isMerged = true;

        if (trail != nullptr) {
            UndoEntry e{};
            e.op = UndoOp::NodeLabelFlags;
            e.a = node;
            e.str_aux = std::move(oldLabel);
            e.bool_a = oldIsLeaf;
            e.bool_b = oldIsMerged;
            trail->record(std::move(e));
        }
    } else {
        std::cout << "Left or Right node not leaf" << std::endl;
    }
}

void globalMergeCherry(TreeNode* node, MutationTrail* trail) {
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
            UndoEntry e{};
            e.op = UndoOp::NodeUnmergeCherry;
            e.a = node;
            e.t_aux  = l;
            e.t_aux2 = r;
            trail->record(std::move(e));
        }
    }
}
