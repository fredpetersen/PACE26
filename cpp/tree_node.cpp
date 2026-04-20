#include <tree_node.h>

#include <iostream>

int getCantorHash(int a, int b) {
        // since (a, b) == (b, a), we sort so a > b
        if (b > a) {
            auto c = a;
            a = b;
            b = c;
        }
        return ((a + b)*(a + b + 1))/2 + b;
}

void setCantorHashOfNode(std::shared_ptr<TreeNode> node) {
    if (node->isLeaf) {
        auto l = node->label;
        // Calculates CantorPair(Label, 0). Can't just use label, since labels and cantor pairs only guarantees uniqueness internally
        node->hash = getCantorHash(l, 0);
    } else if (node->left->isLeaf && node->right->isLeaf) {
        auto a = node->left->hash;
        auto b = node->right->hash;
        node->hash = getCantorHash(a, b);
    } else {
        std::cout << "Node is neither a leaf, nor the parent of 2 leaves. Please fix arguments." << std::endl;
        throw std::runtime_error("Cantor Hash is not defined for node of this type.");
    }
}

void mergeCherry(std::shared_ptr<TreeNode> node) {
    auto l = node->left;
    auto r = node->right;

    if (l->isLeaf && r->isLeaf) {
        node->hash = (l->hash) < r->hash ? l->hash : r->hash;
        node->label = (l->label) < r->label ? l->label : r->label;
        node->newickRep = "(" + std::to_string(l->label) + "," + std::to_string(r->label) + ")";
        node->left = nullptr;
        node->right = nullptr;
        node->isLeaf = true;
    } else {
        std::cout << "Left or Right node not leaf" << std::endl;
    }
}