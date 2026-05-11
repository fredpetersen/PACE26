#include <tree_node.h>

#include <iostream>

uint64_t hashSubtree(const TreeNode* n) {
    if (n == nullptr) return 0;
    if (n->subtreeHash != 0) return n->subtreeHash;
    uint64_t h;
    if (n->isLeaf) {
        h = hashLeafLabel(n->label);
    } else {
        h = mixHashes(hashSubtree(n->left), hashSubtree(n->right));
    }
    n->subtreeHash = h;
    return h;
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
        invalidateSubtreeHash(node);

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
        invalidateSubtreeHash(node);

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

// Same shape as globalMergeCherry, but accepts a precomputed label so it can
// flatten arbitrarily deep subtrees (not just two-leaf cherries). Mirrors the
// cherry-merge invariants exactly: isLeaf=true, isMerged=false (so the rest
// of the algorithm treats this as a terminal leaf whose label IS the Newick
// string of the original subtree). The undo entry restores left/right and
// the original label/flags.
void globalMergeSubtree(TreeNode* node, std::string newickLabel, MutationTrail* trail) {
    if (node == nullptr || node->isLeaf || node->left == nullptr || node->right == nullptr) {
        return;
    }
    auto l = node->left;
    auto r = node->right;

    auto oldLabel = node->label;
    auto oldIsLeaf = node->isLeaf;
    auto oldIsMerged = node->isMerged;

    node->isLeaf = true;
    node->label = std::move(newickLabel);
    node->left = nullptr;
    node->right = nullptr;
    invalidateSubtreeHash(node);

    l->parent = nullptr;
    r->parent = nullptr;

    if (trail != nullptr) {
        // LIFO rollback: NodeUnmergeCherry runs first (restores left/right
        // and resets label="0"/isLeaf=false), then NodeLabelFlags runs and
        // overrides the label/isLeaf/isMerged with the genuine pre-merge
        // values (important when this node was already a non-default-label
        // node, e.g. nested merges where the outer node's pre-merge label
        // wasn't "0").
        UndoEntry labelEntry{};
        labelEntry.op = UndoOp::NodeLabelFlags;
        labelEntry.a = node;
        labelEntry.str_aux = std::move(oldLabel);
        labelEntry.bool_a = oldIsLeaf;
        labelEntry.bool_b = oldIsMerged;
        trail->record(std::move(labelEntry));

        UndoEntry slotEntry{};
        slotEntry.op = UndoOp::NodeUnmergeCherry;
        slotEntry.a = node;
        slotEntry.t_aux  = l;
        slotEntry.t_aux2 = r;
        trail->record(std::move(slotEntry));
    }
}
