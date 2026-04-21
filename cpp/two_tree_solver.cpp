#include <two_tree_solver.h>

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

void TwoTreeSolver::printForests() const {
    forest1_->print("Forest 1");
    forest2_->print("Forest 2");
}

void TwoTreeSolver::printCantorMap() const {
    std::cout << "Printing Cantor Map..." << std::endl;
    	for (auto const& pair : cantorMap_) {
            std::cout << "{" << pair.first << ", " << pair.second << "}" << std::endl;
	}
}


void TwoTreeSolver::cleanSingletonLeaves(std::shared_ptr<Forest> mainForest, std::shared_ptr<Forest> otherForest) {
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

//TODO: Finish implementation (actually merge the found common cherries, and check any new pairs)
void TwoTreeSolver::initCantorMap(std::vector<std::shared_ptr<Forest>> forests) {
    for (auto &forest : forests) {
        auto siblingPairs = forest->getSiblingLeafPairs();
        for (auto &pair : siblingPairs) {
            auto parent = pair.first->parent;
            auto cantor = parent->hash;
            if (cantorMap_.find(cantor) == cantorMap_.end()) {
                cantorMap_.insert({cantor, 1});
            } else {
                auto count = cantorMap_[cantor];
                if (count == forests.size() - 1) { //If the specific cantor value is found in all trees (i.e. common pendant subtree), merge
                    //TODO: implement merging logic
                    std::cout << "HASH VALUE: " << cantor << ", FOUND IN EACH TREE!!"  << std::endl;
                    for (auto &mergeForest : forests) {
                        mergeForest->forestMergeCherry(mergeForest->getNodeByCantor(cantor));
                        
                    }
                } else {
                    cantorMap_[cantor] += 1;
                }
            }
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
void TwoTreeSolver::contract(std::shared_ptr<TreeNode> v, std::shared_ptr<Forest> forest) {
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

    This function returns {ancestor, distance}, where distance == -1 if u and v are in different trees.
*/
std::pair<std::shared_ptr<TreeNode>, int> TwoTreeSolver::lca(std::shared_ptr<TreeNode> u, std::shared_ptr<TreeNode> v) {
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

int TwoTreeSolver::solve() {
    initCantorMap({forest1_, forest2_});
    return solve(1, forest1_, forest2_);
}

int TwoTreeSolver::solve(int k) {
    return solve(k, forest1_, forest2_);
}

int TwoTreeSolver::solve(int k, std::shared_ptr<Forest> forest1, std::shared_ptr<Forest> forest2) {
    forest1->print("Forest 1, k=" + std::to_string(k) + ":");
    forest2->print("Forest 2, k=" + std::to_string(k) + ":");

    if (k <= 0) {
        return 0;
    }

    if (forest1 == nullptr || forest2 == nullptr) {
        return k;
    }

    // Placeholder for the actual solution logic.

    // Merge matching sibling pairs to one leaf

    // Remove vertices that are both root and leaf

    // Contract edges with degree 2 nodes

    auto collectPendantSubtreesBetweenLeaves = [](const std::shared_ptr<TreeNode>& leftLeaf,
                                                    const std::shared_ptr<TreeNode>& rightLeaf,
                                                    const std::shared_ptr<TreeNode>& lcaNode) {
        std::vector<std::shared_ptr<TreeNode>> pendantSubtrees;
        if (leftLeaf == nullptr || rightLeaf == nullptr || lcaNode == nullptr) {
            return pendantSubtrees;
        }

        std::unordered_set<TreeNode*> seen;
        auto collectFromLeaf = [&](std::shared_ptr<TreeNode> leaf) {
            auto current = leaf;
            while (current != nullptr && current->parent != nullptr && current->parent != lcaNode) {
                auto parent = current->parent;
                auto sibling = (parent->left == current) ? parent->right : parent->left;
                if (sibling != nullptr && seen.insert(sibling.get()).second) {
                    pendantSubtrees.push_back(sibling);
                }
                current = parent;
            }
        };

        collectFromLeaf(leftLeaf);
        collectFromLeaf(rightLeaf);
        return pendantSubtrees;
    };

    // Branch on remaining sibling pairs in tree1
    auto siblingLeafPairInForest1 = forest1->getOneSiblingPair();
    /* Maybe it's actually better to just get a random sibling pair and branch on that,
    as that way you can update the sibling pairs after each merge/contract operation,
    which will likely reduce the number of sibling pairs faster than just branching on all of them at once.
    This is something to experiment with later.*/
    auto u = siblingLeafPairInForest1.first;
    auto v = siblingLeafPairInForest1.second;
    auto uInForest2 = forest2->getLeafByLabel(u->label);
    auto vInForest2 = forest2->getLeafByLabel(v->label);

    // if (uInForest2 == nullptr || vInForest2 == nullptr) {
    //     continue;
    // }

    auto lcaResult = lca(uInForest2, vInForest2);
    auto distanceInForest2 = lcaResult.second;

    if (distanceInForest2 == 1) {
        // This sibling pair is fine for this pair of forests, but isn't common for all trees
        // Fidn the next sibling pair to branch on.
    }

    // Case 1: u,v are siblings in tree 1 but in different components in tree2
    if (distanceInForest2 == -1) {
        auto forest1Copy = forest1->cloneForest();
        auto forest2Copy = forest2->cloneForest();

        auto uCopy = forest1Copy->getLeafByLabel(u->label);
        auto uCopyInForest2 = forest2Copy->getLeafByLabel(u->label);
        if (uCopy != nullptr && uCopyInForest2 != nullptr && uCopy->parent != nullptr && uCopyInForest2->parent != nullptr) {
            forest1Copy->detachChild(uCopy);
            forest2Copy->detachChild(uCopyInForest2);

            if (solve(k - 1, forest1Copy, forest2Copy) == 0) {
                return 0;
            }
        }

        auto forest1Copy2 = forest1->cloneForest();
        auto forest2Copy2 = forest2->cloneForest();

        auto vCopy = forest1Copy2->getLeafByLabel(v->label);
        auto vCopyInForest2 = forest2Copy2->getLeafByLabel(v->label);
        if (vCopy != nullptr && vCopyInForest2 != nullptr && vCopy->parent != nullptr && vCopyInForest2->parent != nullptr) {
            forest1Copy2->detachChild(vCopy);
            forest2Copy2->detachChild(vCopyInForest2);

            if (solve(k - 1, forest1Copy2, forest2Copy2) == 0) {
                return 0;
            }
        }

    }
    // Case 2: u,v are siblings in tree 1, but u is sibling with parent of v in tree2 (or vice versa)
    else if (distanceInForest2 == 2) {
        auto forest1Copy = forest1->cloneForest();
        auto forest2Copy = forest2->cloneForest();
        auto uCopyInForest2 = forest2Copy->getLeafByLabel(u->label);
        auto vCopyInForest2 = forest2Copy->getLeafByLabel(v->label);

        // Detach the pendant subtree in tree 2 that is between u and v
        auto lcaResultInCopy = lca(uCopyInForest2, vCopyInForest2);
        auto pendantSubtrees = collectPendantSubtreesBetweenLeaves(uCopyInForest2, vCopyInForest2, lcaResultInCopy.first);

        if (!pendantSubtrees.empty() && pendantSubtrees[0] != nullptr && pendantSubtrees[0]->parent != nullptr) {
            forest2Copy->detachChild(pendantSubtrees[0]);

            if (solve(k - 1, forest1Copy, forest2Copy) == 0) {
                return 0;
            }
        }


    }
    // Case 3: u,v are siblings in tree 1, but there are 2 or more pendant subtrees in tree2 between u and v
    else {
        // Branch on three cases: detach u, detach v, or detach the b many pendant subtrees in tree 2 that are between u and v
        auto forest1Copy = forest1->cloneForest();
        auto forest2Copy = forest2->cloneForest();

        auto uCopy = forest1Copy->getLeafByLabel(u->label);
        auto uCopyInForest2 = forest2Copy->getLeafByLabel(u->label);
        if (uCopy != nullptr && uCopyInForest2 != nullptr && uCopy->parent != nullptr && uCopyInForest2->parent != nullptr) {
            forest1Copy->detachChild(uCopy);
            forest2Copy->detachChild(uCopyInForest2);

            if (solve(k - 1, forest1Copy, forest2Copy) == 0) {
                return 0;
            }
        }

        auto forest1Copy2 = forest1->cloneForest();
        auto forest2Copy2 = forest2->cloneForest();

        auto vCopy = forest1Copy2->getLeafByLabel(v->label);
        auto vCopyInForest2 = forest2Copy2->getLeafByLabel(v->label);
        if (vCopy != nullptr && vCopyInForest2 != nullptr && vCopy->parent != nullptr && vCopyInForest2->parent != nullptr) {
            forest1Copy2->detachChild(vCopy);
            forest2Copy2->detachChild(vCopyInForest2);

            if (solve(k - 1, forest1Copy2, forest2Copy2) == 0) {
                return 0;
            }
        }

        auto forest1Copy3 = forest1->cloneForest();
        auto forest2Copy3 = forest2->cloneForest();
        auto uCopyInForest2Case3 = forest2Copy3->getLeafByLabel(u->label);
        auto vCopyInForest2Case3 = forest2Copy3->getLeafByLabel(v->label);

        auto lcaResultInCopy = lca(uCopyInForest2Case3, vCopyInForest2Case3);
        auto pendantSubtrees = collectPendantSubtreesBetweenLeaves(
            uCopyInForest2Case3,
            vCopyInForest2Case3,
            lcaResultInCopy.first
        );

        if (!pendantSubtrees.empty()) {
            int detachedPendantCount = 0;
            for (const auto& subtreeRoot : pendantSubtrees) {
                if (subtreeRoot != nullptr && subtreeRoot->parent != nullptr) {
                    forest2Copy3->detachChild(subtreeRoot);
                    detachedPendantCount++;
                }
            }

            if (detachedPendantCount > 0 && solve(k - detachedPendantCount, forest1Copy3, forest2Copy3) == 0) {
                return 0;
            }
        }
    }


    return k;
}
