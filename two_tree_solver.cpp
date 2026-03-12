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
public:
    TwoTreeSolver(TreeNode* tree1, TreeNode* tree2, int leafCount)
        : tree1_(tree1), tree2_(tree2), leafCount_(leafCount), debug_(false) {}

    TwoTreeSolver(TreeNode* tree1, TreeNode* tree2, int leafCount, bool debug)
        : tree1_(tree1), tree2_(tree2), leafCount_(leafCount), debug_(debug) {}

    std::pair<TreeNode*, int> lca(TreeNode* u, TreeNode* v) {
        /*
            Finds the lowest shared ancestor between TreeNodes u and v, and writes the pointer to the shared ancestor to the res address,
            as well as the distance to the dist address.

            This algorithm works by working up from v and u and placing each parent in a set (until the root), once a parent is attempted to
            be entered into the set, but that parent is already in the set, then you know you have found the lowest common ancestor. If both
            vertices climb all the way up to root and there is still no intersection, then they are in different components.
        */

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

        debug_ = curDebug;
    }
};

int main() {
    auto problemInstance = parseInput();
    TwoTreeSolver solver(problemInstance.trees[0].get(), problemInstance.trees[1].get(), problemInstance.leafCount, true);
    int result = solver.solve();
    std::cout << result << std::endl;
    return 0;
}