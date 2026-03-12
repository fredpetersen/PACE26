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

#include "input_reader.cpp"

class TwoTreeSolver {
  TreeNode* tree1_;
  TreeNode* tree2_;
  int leafCount_;
  bool debug_;

    void printTreeRecursive(const TreeNode* node, const std::string& prefix, bool isLeft) const {
            if (node == nullptr) {
                    std::cout << prefix << (isLeft ? "├── " : "└── ") << "(null)" << std::endl;
                    return;
            }

            std::cout << prefix << (isLeft ? "├── " : "└── ")
                                << (node->isLeaf ? "Leaf(" : "Node(") << node->label << ")" << std::endl;

            if (node->isLeaf) {
                    return;
            }

            const std::string childPrefix = prefix + (isLeft ? "│   " : "    ");
            printTreeRecursive(node->left.get(), childPrefix, true);
            printTreeRecursive(node->right.get(), childPrefix, false);
    }

public:
    TwoTreeSolver(TreeNode* tree1, TreeNode* tree2, int leafCount)
        : tree1_(tree1), tree2_(tree2), leafCount_(leafCount), debug_(false) {}

    TwoTreeSolver(TreeNode* tree1, TreeNode* tree2, int leafCount, bool debug)
        : tree1_(tree1), tree2_(tree2), leafCount_(leafCount), debug_(debug) {}

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





    /**
     * Contracts the edge between v and its only child, if it has one.
     *
     * If v is a root node, then the child becomes the new root.
     * If v is an internal node, then the child takes the place of v in the tree.
     * This should run in O(1) time, as it only involves a constant number of pointer updates.
     *
     * Does nothing if v has 0 or 2 children, as it is not possible to contract in those cases.
     */
    void contract(TreeNode* v) {
        bool hasLeftChild = v->left != nullptr;
        bool hasRightChild = v->right != nullptr;
        if(debug_) {
            std::cout << "Attempting to contract vertex " << v << " with label " << v->label << std::endl;
            std::cout << "Has left child: " << hasLeftChild << ", has right child: " << hasRightChild << std::endl;
        }
        if (hasLeftChild && hasRightChild) return; // Both children are present; can't contract
        if (v->isLeaf) return; // can't contract leaf nodes

        if (v->parent != nullptr) { // v is not root node
            if(debug_) {
                std::cout << "Vertex is not root, updating parent pointers..." << std::endl;
            }
            bool isRightChild = v->parent->right.get() == v;
            if (isRightChild) {
                if(hasLeftChild) {
                    v->parent->right = std::move(v->left);
                } else {
                    v->parent->right = std::move(v->right);
                }
            } else
            {
                if(hasLeftChild) {
                    v->parent->left = std::move(v->left);
                } else {
                    v->parent->left = std::move(v->right);
                }
            }

        }
        else { // v is root node
            if(debug_) {
                std::cout << "Vertex is root, updating child pointers..." << std::endl;
            }
            if (hasLeftChild) {
                v->left->parent = nullptr;
            }
            else if (hasRightChild)
            {
                v->right->parent = nullptr;
            }

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
    std::pair<TreeNode*, int> lca(TreeNode* u, TreeNode* v) {
        if(debug_) {
            std::cout << "Running LCA..." << std::endl;
        }

        if(u == v) {
            if(debug_) {
                std::cout << "u and v are the same vertex!" << std::endl;
            }
            return {u, 0};
        }

        // Takes into account if u is a parent of v
        std::unordered_set<TreeNode*> parentSet{u};

        auto uTmp = u;
        auto vTmp = v;

        if(debug_) {
            std::cout << "Tracing ancestory of u..." << std::endl;
        }

        // u->parent == nullptr means that u->parent is the root node
        while (uTmp->parent != nullptr) {
            parentSet.insert(uTmp->parent);
            uTmp = uTmp->parent;
        }

        if(debug_) {
            std::cout << "Ancestory of u determined: {";
            for (auto x : parentSet)
                std::cout << x << " ";
            std::cout << "}" << std::endl;
        }

        int dist = 0;
        TreeNode* lca = nullptr;

        if(debug_) {
            std::cout << "Tracing ancestory of v..." << std::endl;
        }
        // Takes into account if V is an ancestor of U
        if (parentSet.count(v) > 0) {
            lca = v;
        } else {
            while (vTmp->parent != nullptr) {
                dist++;
                if (parentSet.count(vTmp->parent) > 0) {
                    if(debug_) {
                        std::cout << "Common ancestor " << vTmp->parent << " found at distance " << dist << " from v" << std::endl;
                    }
                    lca = vTmp->parent;
                    break;
                }
                vTmp = vTmp->parent;
            }
        }

        if (lca == u) {
            if(debug_) {
                std::cout << "v is a descendent of u!" << std::endl;
            }
            return {lca, dist};
        } else if (lca != nullptr) {
            uTmp = u;
            // Calculates the distance from u to the lca to get the final distance (or root)
            while (uTmp->parent != lca) {
                dist++;
                uTmp = uTmp->parent;
            }
            if(debug_) {
                std::cout << "Returning result: {" << lca << ", " << dist << "}" << std::endl;
            }
            return {lca, dist};
        } else {
            if(debug_) {
                std::cout << "No common ancestors found, u and v are in different component subtrees" << std::endl;
            }
            return {nullptr, -1};
        }
    }

    int solve() {
        if(debug_) {
            std::cout << "Debug Mode is activated, more information will be printed" << std::endl;
            printTree(tree1_, "Tree 1:");
            printTree(tree2_, "Tree 2:");
        } else {
            std::cout << "Activate Debug Mode to see more information" << std::endl;
        }

        std::cout << tree1_->left.get()->left.get()->left.get()->parent->right.get()->label << std::endl;

        // Placeholder for the actual solution logic.

        // Merge matching sibling pairs to one leaf

        // Remove vertices that are both root and leaf

        // Contract edges with degree 2 nodes

        // Branch on remaining sibling pairs in tree1
        // // Case 1: u,v are siblings in tree 1 but in different components in tree2
        //
        // // Case 2: u,v are siblings in tree 1, but u is sibling with parent of v in tree2 (or vice versa)

        // // Case 3: u,v are siblings in tree 1, but there are 2 or more pendant subtrees in tree2 between u and v

        return 0;
    }

    int test() {
        //1
        bool curDebug = debug_;
        debug_ = true;

        auto [ancestor, dist] = lca(tree1_->left.get(), tree2_->right.get());
        std::cout << "Dist measured = "<< dist << ", expected = -1" << std::endl << std::endl;

        //2
        dist = lca(tree1_, tree1_).second;
        std::cout << "Dist measured = "<< dist << ", expected = 0" << std::endl << std::endl;

        //3
        dist = lca(tree1_->left.get(), tree1_->left.get()).second;
        std::cout << "Dist measured = "<< dist << ", expected = 0" << std::endl << std::endl;

        //4
        dist = lca(tree1_->left.get(), tree1_->left->left.get()).second;
        std::cout << "Dist measured = "<< dist << ", expected = 1" << std::endl << std::endl;

        //5
        dist = lca(tree1_->left.get(), tree1_->right.get()).second;
        std::cout << "Dist measured = "<< dist << ", expected = 1" << std::endl << std::endl;

        //6
        dist = lca(tree1_->left.get(), tree1_->right->right.get()).second;
        std::cout << "Dist measured = "<< dist << ", expected = 2" << std::endl << std::endl;

        //7
        dist = lca(tree1_->left->left.get(), tree1_->right.get()).second;
        std::cout << "Dist measured = "<< dist << ", expected = 2" << std::endl << std::endl;

        auto v = std::make_unique<TreeNode>();
        v->isLeaf = false;
        v->label = 1;
        TreeNode* vRaw = v.get();

        auto parent = std::make_unique<TreeNode>();
        parent->isLeaf = false;
        parent->label = 2;
        vRaw->parent = parent.get();
        parent->left = std::move(v);

        auto child = std::make_unique<TreeNode>();
        child->isLeaf = true;
        child->label = 3;
        child->parent = vRaw;
        vRaw->right = std::move(child);

        printTree(parent.get(), "Before contraction:");
        contract(vRaw);
        printTree(parent.get(), "After contraction:");


        debug_ = curDebug;
        return 0;
    }
};

int main() {
    auto problemInstance = parseInput();
    TwoTreeSolver solver(problemInstance.trees[0].get(), problemInstance.trees[1].get(), problemInstance.leafCount, true);
    int result = solver.solve();
    solver.test();
    std::cout << result << std::endl;
    return 0;
}