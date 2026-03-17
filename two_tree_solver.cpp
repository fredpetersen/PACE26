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

#include "input_reader.cpp"

class TwoTreeSolver {
  Forest* forest1_;
  Forest* forest2_;
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
    TwoTreeSolver(Forest* forest1, Forest* forest2, int leafCount)
        : forest1_(forest1), forest2_(forest2), leafCount_(leafCount), debug_(false) {}

    TwoTreeSolver(Forest* forest1, Forest* forest2, int leafCount, bool debug)
        : forest1_(forest1), forest2_(forest2), leafCount_(leafCount), debug_(debug) {}

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
        for (const auto& root : forest1_->roots) {
            printTree(root.get(), "Tree:");
        }
        std::cout << "Forest 2:" << std::endl;
        for (const auto& root : forest2_->roots) {
            printTree(root.get(), "Tree:");
        }
    }

    void printForest(const Forest* forest, const std::string& name) const {
        std::cout << name << std::endl;
        for (const auto& root : forest->roots) {
            printTree(root.get(), "Tree:");
        }
    }


    void cleanSingletonLeaves(std::shared_ptr<Forest> mainForest, std::shared_ptr<Forest> otherForest) {
        std::unordered_set<std::shared_ptr<TreeNode>> newRoots;
        for (const auto& root : mainForest->roots) {
            std::cout << root << std::endl;
            if (root->isLeaf) {
                // mainForest->leaves.erase(root);
                // auto child = *otherForest->leaves.find(root);
                // removeChild(child->parent, child);
                otherForest->leaves.erase(root);
                otherForest->roots.erase(root);
                continue; // Skip singleton leaf roots
            }
            newRoots.insert(root);
        }
        mainForest->roots = std::move(newRoots);
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
        if(debug_) {
            std::cout << "Attempting to contract vertex " << v.get() << " with label " << v.get()->label << std::endl;
            std::cout << "Has left child: " << hasLeftChild << ", has right child: " << hasRightChild << std::endl;
        }
        if (hasLeftChild && hasRightChild) return; // Both children are present; can't contract
        if (v->isLeaf) return; // can't contract leaf nodes

        auto child = hasLeftChild ? v->left : v->right; // The only child of v

        if (v->parent != nullptr) { // v is not root node
            if(debug_) {
                std::cout << "Vertex is not root, updating parent pointers..." << std::endl;
            }
            bool isRightChild = v->parent->right.get() == v.get();
            if (isRightChild) {
                v->parent->right = std::move(child);
            } else {
                v->parent->left = std::move(child);
            }

        } else { // v is root node
            if(debug_) {
                std::cout << "Vertex is root, updating child pointers..." << std::endl;
            }
            child->parent = nullptr;
            if(forest->roots.erase(v) > 0) {
                forest->roots.insert(child);
            } else {
                if(debug_) {
                    std::cout << "Error: Vertex to contract not found in forest roots!" << std::endl;
                }
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
    std::pair<std::shared_ptr<TreeNode>, int> lca(std::shared_ptr<TreeNode> u, std::shared_ptr<TreeNode> v) {
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
        std::unordered_set<std::shared_ptr<TreeNode>> parentSet{u};

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
        std::shared_ptr<TreeNode> lca = nullptr;

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
            printForests();
        } else {
            std::cout << "Activate Debug Mode to see more information" << std::endl;
        }

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

    int test_lca(std::shared_ptr<TreeNode> tree1_, std::shared_ptr<TreeNode> tree2_) {

        //1
        auto [ancestor, dist] = lca(tree1_->left, tree2_->right);
        std::cout << "Dist measured = "<< dist << ", expected = -1" << std::endl << std::endl;

        //2
        dist = lca(tree1_, tree1_).second;
        std::cout << "Dist measured = "<< dist << ", expected = 0" << std::endl << std::endl;

        //3
        dist = lca(tree1_->left, tree1_->left).second;
        std::cout << "Dist measured = "<< dist << ", expected = 0" << std::endl << std::endl;

        //4
        dist = lca(tree1_->left, tree1_->left->left).second;
        std::cout << "Dist measured = "<< dist << ", expected = 1" << std::endl << std::endl;

        //5
        dist = lca(tree1_->left, tree1_->right).second;
        std::cout << "Dist measured = "<< dist << ", expected = 1" << std::endl << std::endl;

        //6
        dist = lca(tree1_->left, tree1_->right->right).second;
        std::cout << "Dist measured = "<< dist << ", expected = 2" << std::endl << std::endl;

        //7
        dist = lca(tree1_->left->left, tree1_->right).second;
        std::cout << "Dist measured = "<< dist << ", expected = 2" << std::endl << std::endl;

        return 0;
    }

    int test_contraction() {
        std::cout << std::endl << "Testing the Contraction function" << std::endl;

        auto v = std::make_shared<TreeNode>();
        v->isLeaf = false;
        v->label = 1;

        auto parent = std::make_shared<TreeNode>();
        parent->isLeaf = false;
        parent->label = 2;
        v->parent = parent;
        parent->left = v;

        auto child = std::make_shared<TreeNode>();
        child->isLeaf = true;
        child->label = 3;
        child->parent = v;
        v->right = child;
        auto forest = std::make_shared<Forest>();
        forest->roots.insert(parent);

        printForest(forest.get(), "Before contraction:");
        contract(v, forest);
        printForest(forest.get(), "After contraction step 1:");
        contract(parent, forest);
        printForest(forest.get(), "After contraction step 2:");
        contract(child, forest);
        printForest(forest.get(), "After contraction step 3:");

        return 0;
    }

    int test_singelton_leaf() {
        std::cout << std::endl << "Testing the Clean Singelton Leaf function" << std::endl;

        auto f1 = std::make_shared<Forest>();
        auto f2 = std::make_shared<Forest>();

        auto t1_l = std::make_shared<TreeNode>();
        t1_l->isLeaf = true;
        t1_l->label = 1;

        auto t1_r = std::make_shared<TreeNode>();
        t1_r->isLeaf = true;
        t1_r->label = 3;

        auto t1_p = std::make_shared<TreeNode>();
        t1_p->left = t1_l;
        t1_l->parent = t1_p; 

        t1_p->right = t1_r;
        t1_r->parent = t1_p; 

        auto t1_single1 = std::make_shared<TreeNode>();
        t1_single1->isLeaf = true;
        t1_single1->label = 2;

        auto t1_single2 = std::make_shared<TreeNode>();
        t1_single2->isLeaf = true;
        t1_single2->label = 4;

        f1->roots = {t1_p, t1_single1, t1_single2};
        f1->leaves = {t1_l, t1_r, t1_single1, t1_single2};

        printForest(f1.get(), "Forest 1");

        auto t2_p = std::make_shared<TreeNode>();

        auto t2_l = std::make_shared<TreeNode>();
        t2_l->isLeaf = true;
        t2_l->label = 2;

        auto t2_r1 = std::make_shared<TreeNode>();
        t2_r1->isLeaf = true;
        t2_r1->label = 3;

        auto t2_lp = std::make_shared<TreeNode>();
        t2_lp->left = t2_l;
        t2_l->parent = t2_lp; 

        t2_lp->right = t2_r1;
        t2_r1->parent = t2_lp; 

        auto t2_r2 = std::make_shared<TreeNode>();
        t2_r2->isLeaf = true;
        t2_r2->label = 1;

        t2_p->left = t2_lp;
        t2_p->right = t2_r2;

        auto t2_single1 = std::make_shared<TreeNode>();
        t2_single1->isLeaf = true;
        t2_single1->label = 4;

        f2->roots = {t2_p, t2_single1};
        f2->leaves = {t2_l, t2_r1, t2_r2, t2_single1};

        printForest(f2.get(), "Forest 2");

        std::cout << "Cleaning singleton leaves..," << std::endl;

        cleanSingletonLeaves(f1, f2);
        
        printForest(f1.get(), "Forest 1");
        printForest(f2.get(), "Forest 2");

        return 0;
    }

    int test() {
        bool curDebug = debug_;
        debug_ = true;

        // auto tree1_ = *forest1_->roots.begin();
        // auto tree2_ = *forest2_->roots.begin();

        // std::cout << "Leaves in forest 1:" << std::endl;
        // for (const auto& leaf : forest1_->leaves) {
        //     std::cout << leaf->label << " ";
        // }
        // std::cout << std::endl << std::endl;

        // std::cout << "Leaves in forest 2:" << std::endl;
        // for (const auto& leaf : forest2_->leaves) {
        //     std::cout << leaf->label << " ";
        // }
        // std::cout << std::endl;

        // test_lca(tree1_, tree2_);
        // test_contraction();

        test_singelton_leaf();

        debug_ = curDebug;
        return 0;
    }
};

int main() {
    auto problemInstance = parseInput();
    TwoTreeSolver solver(problemInstance.forests[0].get(), problemInstance.forests[1].get(), problemInstance.leafCount, true);
    int result = solver.solve();
    solver.test();
    std::cout << result << std::endl;
    return 0;
}