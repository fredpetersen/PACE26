#include <cctype>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <problem_instance.h>
#include <tree_node.h>
#include <forest.h>

/* Hashing and equality functions assume that the TreeNodes will always be given in the same order

Thus they are sensitive to the order of the pair, as (u,v) and (v,u) will be considered different pairs
*/
struct SiblingPairHash {
    size_t operator()(const std::pair<std::shared_ptr<TreeNode>, std::shared_ptr<TreeNode>>& p) const noexcept {
        auto a = p.first->label;
        auto b = p.second->label;
        if (a > b) {
            auto c = a;
            a = b;
            b = c;
        }
        return ((a + b)*(a + b + 1))/2 + b;
    }
};

struct SiblingPairEq {
    bool operator()(const std::pair<std::shared_ptr<TreeNode>, std::shared_ptr<TreeNode>>& a,
                    const std::pair<std::shared_ptr<TreeNode>, std::shared_ptr<TreeNode>>& b) const noexcept {
        return (a.first == b.first && a.second == b.second) || (a.first == b.second && a.second == b.first);
    }
};

class TwoTreeSolver {
  Forest* forest1_;
  Forest* forest2_;
  int leafCount_;

    void printTreeRecursive(const TreeNode* node, const std::string& prefix, bool isLeft) const {
            if (node == nullptr) {
                    std::cout << prefix << (isLeft ? "├── " : "└── ") << "(null)" << std::endl;
                    return;
            }

            std::cout << prefix << (isLeft ? "├── " : "└── ")
                                << (node->isLeaf ? "Leaf(" : "Node(") << node->label << "[" << node->hash << "]" << ")" << std::endl;

            if (node->isLeaf) {
                    return;
            }

            const std::string childPrefix = prefix + (isLeft ? "│   " : "    ");
            printTreeRecursive(node->left.get(), childPrefix, true);
            printTreeRecursive(node->right.get(), childPrefix, false);
    }

public:
    TwoTreeSolver(Forest* forest1, Forest* forest2, int leafCount)
        : forest1_(forest1), forest2_(forest2), leafCount_(leafCount) {}

    void printTree(const TreeNode* root, const std::string& name) const {
        std::cout << name << std::endl;
        if (root == nullptr) {
            std::cout << "└── (null)" << std::endl;
            return;
        }

        std::cout << "└── " << (root->isLeaf ? "Leaf(" : "Node(") << root->label << ")" << std::endl;
        if (root->isLeaf) {
            return;
        }

        printTreeRecursive(root->left.get(), "    ", true);
        printTreeRecursive(root->right.get(), "    ", false);
    }

    void printForests() const {
        std::cout << "Forest 1:" << std::endl;
        for (const auto& root : forest1_->getRoots()) {
            printTree(root.get(), "Tree:");
        }
        std::cout << "Forest 2:" << std::endl;
        for (const auto& root : forest2_->getRoots()) {
            printTree(root.get(), "Tree:");
        }
    }

    void printForest(Forest* forest, const std::string& name) const {
        std::cout << name << std::endl;
        for (const auto& root : forest->getRoots()) {
            printTree(root.get(), "Tree:");
        }
    }


    void cleanSingletonLeaves(std::shared_ptr<Forest> mainForest, std::shared_ptr<Forest> otherForest) {
        std::unordered_set<std::shared_ptr<TreeNode>> newRoots;
        for (const auto& root : mainForest->getRoots()) {
            if (root->isLeaf) {
                mainForest->removeRoot(root);
                mainForest->removeLeaf(root);
                otherForest->removeRoot(root);
                otherForest->removeLeaf(root);
                continue; // Skip singleton leaf roots
            }
        }
    }


    /**
     * Contracts the edge between v and its only child, if it has one.
     *
     * If v is a root node, then the child becomes the new root.
     * If v is an internal node, then the child takes the place of v in the tree.
     * This should run in O(1) time, as it only involves a constant number of pointer updates.
     *
     * Does nothing if v has 0 or 2 children, as it is not possible to contract in those cases.
     */
    void contract(std::shared_ptr<TreeNode> v, std::shared_ptr<Forest> forest) {
        bool hasLeftChild = v->left != nullptr;
        bool hasRightChild = v->right.get() != nullptr;
        if (hasLeftChild && hasRightChild) return; // Both children are present; can't contract
        if (v->isLeaf) return; // can't contract leaf nodes

        auto child = hasLeftChild ? v->left : v->right; // The only child of v

        if (v->parent != nullptr) { // v is not root node
            bool isRightChild = v->parent->right.get() == v.get();
            if (isRightChild) {
                v->parent->right = std::move(child);
            } else {
                v->parent->left = std::move(child);
            }

        } else { // v is root node
            child->parent = nullptr;
            forest->removeRoot(v);
            forest->addRoot(child);
        }
    }

    /**
        Finds the lowest shared ancestor between TreeNodes u and v, and writes the pointer to the shared ancestor to the res address,
        as well as the distance to the dist address.

        This algorithm works by working up from v and u and placing each parent in a set (until the root), once a parent is attempted to
        be entered into the set, but that parent is already in the set, then you know you have found the lowest common ancestor. If both
        vertices climb all the way up to root and there is still no intersection, then they are in different components.

        This should run in O(log n) for balanced trees, as worst case both vertices have to climb up to the root of the tree.

        This function returns 0 if it worked as intended.
    */
    std::pair<std::shared_ptr<TreeNode>, int> lca(std::shared_ptr<TreeNode> u, std::shared_ptr<TreeNode> v) {
        if(u == v) {
            return {u, 0};
        }

        // Takes into account if u is a parent of v
        std::unordered_set<std::shared_ptr<TreeNode>> parentSet{u};

        auto uTmp = u;
        auto vTmp = v;

        // u->parent == nullptr means that u->parent is the root node
        while (uTmp->parent != nullptr) {
            parentSet.insert(uTmp->parent);
            uTmp = uTmp->parent;
        }

        int dist = 0;
        std::shared_ptr<TreeNode> lca = nullptr;

        // Takes into account if V is an ancestor of U
        if (parentSet.count(v) > 0) {
            lca = v;
        } else {
            while (vTmp->parent != nullptr) {
                dist++;
                if (parentSet.count(vTmp->parent) > 0) {
                    lca = vTmp->parent;
                    break;
                }
                vTmp = vTmp->parent;
            }
        }

        if (lca == u) {
            return {lca, dist};
        } else if (lca != nullptr) {
            uTmp = u;
            // Calculates the distance from u to the lca to get the final distance (or root)
            while (uTmp->parent != lca) {
                dist++;
                uTmp = uTmp->parent;
            }
            return {lca, dist};
        } else {
            return {nullptr, -1};
        }
    }

    /**
     * Returns a set of all sibling leaf pairs in the forest.
     *
     * This currently runs in O(n) time, but could be optimized to O(s)
     * where s is the number of sibling pairs, by keeping track of
     * sibling pairs in the forest data structure and updating that set
     * as merges and contractions happen.
     */
    std::unordered_set<
        std::pair<std::shared_ptr<TreeNode>, std::shared_ptr<TreeNode>>,
        SiblingPairHash,SiblingPairEq
    > getSiblingLeafPairs(Forest* forest) {

        std::unordered_set<
            std::pair<std::shared_ptr<TreeNode>, std::shared_ptr<TreeNode>>,
            SiblingPairHash,SiblingPairEq
        > siblingLeafPairs;

        std::unordered_map<std::shared_ptr<TreeNode>, std::vector<std::shared_ptr<TreeNode>>> parentToLeaves;
        for (const auto& leaf : forest->getLeaves()) {
            if (leaf->parent != nullptr) {
                parentToLeaves[leaf->parent].push_back(leaf);
            }
        }
        for (const auto& [parent, leaves] : parentToLeaves) {
            if (leaves.size() == 2) {
                siblingLeafPairs.insert({leaves[0], leaves[1]});
            }
        }
        return siblingLeafPairs;
    }


    int solve() {

        // Placeholder for the actual solution logic.

        // Merge matching sibling pairs to one leaf

        // Remove vertices that are both root and leaf

        // Contract edges with degree 2 nodes

        // Branch on remaining sibling pairs in tree1
        auto siblingLeafPairsInForest1 = getSiblingLeafPairs(forest1_);
        /* Maybe it's actually better to just get a random sibling pair and branch on that,
        as that way you can update the sibling pairs after each merge/contract operation,
        which will likely reduce the number of sibling pairs faster than just branching on all of them at once.
        This is something to experiment with later.*/
        for (const auto& siblingPair : siblingLeafPairsInForest1) {
            auto u = siblingPair.first;
            auto v = siblingPair.second;
            auto uInForest2 = forest2_->getLeafByLabel(u->label);
            auto vInForest2 = forest2_->getLeafByLabel(v->label);

            auto forestCopy = *forest1_;
            auto forest2Copy = *forest2_;
            auto uCopy = forestCopy.getLeafByLabel(u->label);
            auto uCopyInForest2 = forest2Copy.getLeafByLabel(u->label);
            auto vCopy = forestCopy.getLeafByLabel(v->label);
            auto vCopyInForest2 = forest2Copy.getLeafByLabel(v->label);

            // Case 1: u,v are siblings in tree 1 but in different components in tree2
            if (lca(uInForest2, vInForest2).second == -1) {

                // first try detaching u in the copy
                forestCopy.detachChild(uCopy);
                forest2Copy.detachChild(uCopyInForest2);

                solve();

                // if that doesn't work, then try detaching v in original forest
                forest1_->detachChild(v);
                forest2_->detachChild(vInForest2);
                solve();

            }
            // Case 2: u,v are siblings in tree 1, but u is sibling with parent of v in tree2 (or vice versa)
            else if (lca(uInForest2, vInForest2).second == 2) {
                // Merge u and v in forest 1, and contract the edge between the sibling and its parent in forest 2
            }
            // Case 3: u,v are siblings in tree 1, but there are 2 or more pendant subtrees in tree2 between u and v
            else {
                // Merge u and v in forest 1, and branch on all possible merges of the pendant subtrees in tree 2
            }
        }


        return 0;
    }
};
