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
        for (const auto& leaf : forest->leaves) {
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
        auto siblingLeafPairsInForest1 = getSiblingLeafPairs(forest1_);
        /* Maybe it's actually better to just get a random sibling pair and branch on that,
        as that way you can update the sibling pairs after each merge/contract operation,
        which will likely reduce the number of sibling pairs faster than just branching on all of them at once.
        This is something to experiment with later.*/
        for (const auto& siblingPair : siblingLeafPairsInForest1) {
            auto u = siblingPair.first;
            auto v = siblingPair.second;
            auto uInForest2 = forest2_->leafByLabel[u->label];
            auto vInForest2 = forest2_->leafByLabel[v->label];
            // Case 1: u,v are siblings in tree 1 but in different components in tree2
            if (lca(uInForest2, vInForest2).second == -1) {
                if(debug_) {
                    std::cout << "Case 1: (" << u->label << ", " << v->label << ") are siblings in tree 1, but are in different components in tree 2" << std::endl;
                }
                // Merge u and v in forest 1, and remove the corresponding leaves in forest 2
            }
            // Case 2: u,v are siblings in tree 1, but u is sibling with parent of v in tree2 (or vice versa)
            else if (lca(uInForest2, vInForest2).second == 2) {
                if(debug_) {
                    std::cout << "Case 2: (" << u->label << ", " << v->label << ") are siblings in tree 1, but u is sibling with parent of v in tree 2 (or vice versa)" << std::endl;
                }
                // Merge u and v in forest 1, and contract the edge between the sibling and its parent in forest 2
            }
            // Case 3: u,v are siblings in tree 1, but there are 2 or more pendant subtrees in tree2 between u and v
            else {
                if(debug_) {
                    std::cout << "Case 3: (" << u->label << ", " << v->label << ") are siblings in tree 1, but there are 2 or more pendant subtrees in tree 2 between them" << std::endl;
                }
                // Merge u and v in forest 1, and branch on all possible merges of the pendant subtrees in tree 2
            }
        }


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
        f1->leafByLabel = {{1, t1_l}, {2, t1_single1}, {3, t1_r}, {4, t1_single2}};

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
        f2->leafByLabel = {{1, t2_r2}, {2, t2_l}, {3, t2_r1}, {4, t2_single1}};

        printForest(f2.get(), "Forest 2");

        std::cout << "Cleaning singleton leaves..," << std::endl;

        cleanSingletonLeaves(f1, f2);

        printForest(f1.get(), "Forest 1");
        printForest(f2.get(), "Forest 2");

        return 0;
    }

    int testgetLeafByLabel() {
        auto forest = std::make_shared<Forest>();

        auto n1 = std::make_shared<TreeNode>();
        n1->isLeaf = false;
        n1->label = 1;

        auto n3 = std::make_shared<TreeNode>();
        n3->isLeaf = true;
        n3->label = 3;

        auto n2 = std::make_shared<TreeNode>();
        n2->isLeaf = true;
        n2->label = 2;

        auto n4 = std::make_shared<TreeNode>();
        n4->isLeaf = true;
        n4->label = 4;

        // Wire up parent/child links
        n1->left = n3;
        n1->right = n2;
        n3->parent = n1;
        n2->parent = n1;

        // Add roots and leaves to forest
        forest->roots.insert(n1);
        forest->roots.insert(n4);

        forest->leaves.insert(n2);
        forest->leaves.insert(n3);
        forest->leaves.insert(n4);
        forest->componentCount = 2;
        forest->leafByLabel = {{2, n2}, {3, n3}, {4, n4}};

        std::cout << "Testing getLeafByLabel..." << std::endl;
        for (int label = 1; label <= 4; label++) {
            auto it = forest->leafByLabel.find(label);
            if (it != forest->leafByLabel.end()) {
                std::cout << "Leaf with label " << label << " found at address " << it->second.get() << std::endl;
            } else {
                std::cout << "Leaf with label " << label << " not found in leafByLabel map" << std::endl;
            }
        }
        return 0;
    }

    int testGetSiblingsPairs() {
        auto forest = std::make_shared<Forest>();
        if(true) { // Wrapped in if statement to allow collapsing in IDE, as forest construction is a bit verbose
            auto n1 = std::make_shared<TreeNode>();
            n1->isLeaf = false;
            n1->label = 1;

            auto n2 = std::make_shared<TreeNode>();
            n2->isLeaf = false;
            n2->label = 2;

            auto n4 = std::make_shared<TreeNode>();
            n4->isLeaf = true;
            n4->label = 4;

            auto n5 = std::make_shared<TreeNode>();
            n5->isLeaf = true;
            n5->label = 5;

            auto n3 = std::make_shared<TreeNode>();
            n3->isLeaf = false;
            n3->label = 3;

            auto n7 = std::make_shared<TreeNode>();
            n7->isLeaf = true;
            n7->label = 7;

            auto n6 = std::make_shared<TreeNode>();
            n6->isLeaf = false;
            n6->label = 6;

            auto n8 = std::make_shared<TreeNode>();
            n8->isLeaf = true;
            n8->label = 8;

            auto n9 = std::make_shared<TreeNode>();
            n9->isLeaf = true;
            n9->label = 9;

            auto n10 = std::make_shared<TreeNode>();
            n10->isLeaf = false;
            n10->label = 10;

            auto n11 = std::make_shared<TreeNode>();
            n11->isLeaf = true;
            n11->label = 11;

            auto n12 = std::make_shared<TreeNode>();
            n12->isLeaf = true;
            n12->label = 12;

            auto n13 = std::make_shared<TreeNode>();
            n13->isLeaf = true;
            n13->label = 13;

            // Wire up parent/child links
            n1->left = n2;
            n1->right = n3;
            n2->parent = n1;
            n2->left = n4;
            n2->right = n5;
            n4->parent = n2;
            n5->parent = n2;
            n3->parent = n1;
            n3->left = n7;
            n3->right = n6;
            n7->parent = n3;
            n6->parent = n3;
            n6->left = n8;
            n6->right = n9;
            n8->parent = n6;
            n9->parent = n6;
            n10->left = n11;
            n10->right = n12;
            n11->parent = n10;
            n12->parent = n10;

            // Add roots and leaves to forest
            forest->roots.insert(n1);
            forest->roots.insert(n10);
            forest->roots.insert(n13);

            forest->leaves.insert(n4);
            forest->leaves.insert(n5);
            forest->leaves.insert(n7);
            forest->leaves.insert(n8);
            forest->leaves.insert(n9);
            forest->leaves.insert(n11);
            forest->leaves.insert(n12);
            forest->leaves.insert(n13);
            forest->componentCount = 3;
            forest->leafByLabel = {{4, n4}, {5, n5}, {7, n7}, {8, n8}, {9, n9}, {11, n11}, {12, n12}, {13, n13}};
        }
        std::cout << "Testing getSiblingLeafPairs..." << std::endl;
        auto siblingLeafPairs = getSiblingLeafPairs(forest.get());
        std::cout << "Sibling leaf pairs found: {";
        for (const auto& leaf : siblingLeafPairs) {
            std::cout << "(" << leaf.first->label << ", " << leaf.second->label << ") ";
        }
        std::cout << "}" << std::endl;
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

        // test_singelton_leaf();
        // testgetLeafByLabel();
        // testGetSiblingsPairs();

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