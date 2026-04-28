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


void TwoTreeSolver::cleanSingletonLeaves(std::shared_ptr<Forest> mainForest, std::shared_ptr<Forest> otherForest) {
    std::unordered_set<std::shared_ptr<TreeNode>> newRoots;
    for (const auto& root : mainForest->getRoots()) {
        if (root->isLeaf) {
            otherForest->detachByLabel(root->label);
            continue;
        }
    }
}

std::vector<std::shared_ptr<Forest>> TwoTreeSolver::cloneForests(std::vector<std::shared_ptr<Forest>> forests) {
    std::vector<std::shared_ptr<Forest>> forestClones = {};
    for (const auto& forest : forests) {
        forestClones.push_back(forest->cloneForest());
    }
    return forestClones;
}

std::shared_ptr<Forest> TwoTreeSolver::solve() {
    std::cout << "Solving..." << std::endl;
    bool isSolved = false;
    std::shared_ptr<Forest> solution;
    int k = 0;
    // No reason to add a looping condition on k, theres is always a trivial solution with k = nr. of leaves
    while (!isSolved) {

        auto res = solve(k++);
        isSolved = res.first;
        solution = res.second;
    }
    return solution;
}

std::pair<bool, std::shared_ptr<Forest>> TwoTreeSolver::solve(int k) {
    std::cout << "Looking for solution size " << k << std::endl;
    std::vector<std::shared_ptr<Forest>> forests = {forest1_, forest2_};
    auto res = solve(k, forests);
    return {res.first, res.second[0]};
}

std::pair<bool, std::vector<std::shared_ptr<Forest>>> TwoTreeSolver::solve(int k, std::vector<std::shared_ptr<Forest>> forests) {
    // Forests should never be empty
    auto fs = cloneForests(forests);
    auto f1 = fs[0];
    auto f2 = fs[1];
    if (fs.size() < 1) {
        throw 1;
    }
    // Step 1 (if there's only 1 forest left, this forest is the MAF)
    if (fs.size() == 1) {
        return {true, fs};
    }
    // Step 2 (if there are more components on the MAF we're building than k, the solution is too big (i.e. invalid))
    if (f1->getComponentCount() > k) {
        return {false, {nullptr}};
    }
    // Step 3 (clean the single vertex trees)
    cleanSingletonLeaves(f1, f2);
    cleanSingletonLeaves(f2, f1);
    // Step 4 (try to get a sibling pair in F2. If it can't be done, move on to solving F3)
    auto [idx, siblingPair] = f2->getOneSiblingPair();
    if (idx == -1) {
        fs.erase(fs.begin() + 1);
        f1->expandMergedSubtrees(); // Undoes the local merge (necessary to step through the algorithm)
        return solve(k, fs);
    }
    // Step 5
    // labels of the nodes
    auto lab_u = siblingPair.first->label;
    auto lab_v = siblingPair.second->label;

    auto [ancestor, dist] = f1->lca(lab_u, lab_v);
    // Step 6 (when u and v are in different components in F1, branch on cutting either u or v)
    if (dist == -1) {
        // clone the forests to run 1 version on each branch
        auto fsClone = cloneForests(fs);
        auto fc1 = fsClone[0];
        auto fc2 = fsClone[1];

        // remember, f1 and f2 (from fs) is already a clone of the argument passed in
        f1->detachByLabel(lab_u);
        f2->detachByLabel(lab_u);
        return solve(k, fs);

        fc1->detachByLabel(lab_v);
        fc2->detachByLabel(lab_v);
        return solve(k, fsClone);
    }
    // Step 7 (when u and v are siblings in F1, do a local merge of u and v in both F1 and F2)
    else if (dist == 1) {
        // TODO: merge cherries in forests
        f1->forestMergeCherry(f1->getLeafByLabel(lab_u));
        f2->forestMergeCherry(f2->getLeafByLabel(lab_u));
    }
    // Step 8 (when the distance between u and v in F1 is >= 2) // TODO: maybe case 2 (ie dist = 2) isnt a special case when there are n trees?
    else if (dist > 1){
        auto fsClone1 = cloneForests(fs);
        auto f1c1 = fsClone1[0];
        auto f2c1 = fsClone1[1];

        auto fsClone2 = cloneForests(fs);
        auto f1c2 = fsClone2[0];
        auto f2c2 = fsClone2[1];

        // branch 1, cut u from F1 and F2
        f1->detachByLabel(lab_u);
        f2->detachByLabel(lab_u);
        solve(k, fs);

        // branch 2, cut v from F1 and F2
        f1c1->detachByLabel(lab_v);
        f2c1->detachByLabel(lab_v);
        solve(k, fsClone1);

        // branch 3, cut all pendant subtrees between u and v from F1 only
        
    }
    // wildcard, you should on paper never reach this case
    std::cout << "You've reached the Wildcard!!!! You should not be able to reach here" << std::endl;
    return {false, {nullptr}};
}
// std::pair<bool, std::shared_ptr<TreeNode>> TwoTreeSolver::solve(int k, std::shared_ptr<Forest> forest1, std::shared_ptr<Forest> forest2) {
//     forest1->print("Forest 1, k=" + std::to_string(k) + ":");
//     forest2->print("Forest 2, k=" + std::to_string(k) + ":");

//     if (k <= 0) {
//         return 0;
//     }

//     if (forest1 == nullptr || forest2 == nullptr) {
//         return k;
//     }

//     // Placeholder for the actual solution logic.

//     // Merge matching sibling pairs to one leaf

//     // Remove vertices that are both root and leaf

//     // Contract edges with degree 2 nodes

//     auto collectPendantSubtreesBetweenLeaves = [](const std::shared_ptr<TreeNode>& leftLeaf,
//                                                     const std::shared_ptr<TreeNode>& rightLeaf,
//                                                     const std::shared_ptr<TreeNode>& lcaNode) {
//         std::vector<std::shared_ptr<TreeNode>> pendantSubtrees;
//         if (leftLeaf == nullptr || rightLeaf == nullptr || lcaNode == nullptr) {
//             return pendantSubtrees;
//         }

//         std::unordered_set<TreeNode*> seen;
//         auto collectFromLeaf = [&](std::shared_ptr<TreeNode> leaf) {
//             auto current = leaf;
//             while (current != nullptr && current->parent != nullptr && current->parent != lcaNode) {
//                 auto parent = current->parent;
//                 auto sibling = (parent->left == current) ? parent->right : parent->left;
//                 if (sibling != nullptr && seen.insert(sibling.get()).second) {
//                     pendantSubtrees.push_back(sibling);
//                 }
//                 current = parent;
//             }
//         };

//         collectFromLeaf(leftLeaf);
//         collectFromLeaf(rightLeaf);
//         return pendantSubtrees;
//     };

//     int index = 0;

//     // Branch on remaining sibling pairs in tree1
//     auto siblingLeafPairInForest1 = forest1->getOneSiblingPair(index);

//     index = siblingLeafPairInForest1.first;
//     auto u = siblingLeafPairInForest1.second.first;
//     auto v = siblingLeafPairInForest1.second.second;
//     auto uInForest2 = forest2->getLeafByLabel(u->label);
//     auto vInForest2 = forest2->getLeafByLabel(v->label);

//     // if (uInForest2 == nullptr || vInForest2 == nullptr) {
//     //     continue;
//     // }

//     auto lcaResult = lca(uInForest2, vInForest2);
//     auto distanceInForest2 = lcaResult.second;
//     while (distanceInForest2 == 1) {
//         // If u and v are siblings in both trees, skip for now
//         // Maybe we should merge, but unsure if that would cause problems when we have more than 2 trees
//         auto siblingLeafPairInForest1 = forest1->getOneSiblingPair(index);
//         index = siblingLeafPairInForest1.first;
//         auto u = siblingLeafPairInForest1.second.first;
//         auto v = siblingLeafPairInForest1.second.second;
//         auto uInForest2 = forest2->getLeafByLabel(u->label);
//         auto vInForest2 = forest2->getLeafByLabel(v->label);

//         auto lcaResult = lca(uInForest2, vInForest2);
//         auto distanceInForest2 = lcaResult.second;
//     }

//     // Case 1: u,v are siblings in tree 1 but in different components in tree2
//     if (distanceInForest2 == -1) {
//         auto forest1Copy = forest1->cloneForest();
//         auto forest2Copy = forest2->cloneForest();

//         auto uCopy = forest1Copy->getLeafByLabel(u->label);
//         auto uCopyInForest2 = forest2Copy->getLeafByLabel(u->label);
//         if (uCopy != nullptr && uCopyInForest2 != nullptr && uCopy->parent != nullptr && uCopyInForest2->parent != nullptr) {
//             forest1Copy->detachChild(uCopy);
//             forest2Copy->detachChild(uCopyInForest2);

//             if (solve(k - 1, forest1Copy, forest2Copy) == 0) {
//                 return 0;
//             }
//         }

//         auto forest1Copy2 = forest1->cloneForest();
//         auto forest2Copy2 = forest2->cloneForest();

//         auto vCopy = forest1Copy2->getLeafByLabel(v->label);
//         auto vCopyInForest2 = forest2Copy2->getLeafByLabel(v->label);
//         if (vCopy != nullptr && vCopyInForest2 != nullptr && vCopy->parent != nullptr && vCopyInForest2->parent != nullptr) {
//             forest1Copy2->detachChild(vCopy);
//             forest2Copy2->detachChild(vCopyInForest2);

//             if (solve(k - 1, forest1Copy2, forest2Copy2) == 0) {
//                 return 0;
//             }
//         }

//     }
//     // Case 2: u,v are siblings in tree 1, but u is sibling with parent of v in tree2 (or vice versa)
//     else if (distanceInForest2 == 2) {
//         auto forest1Copy = forest1->cloneForest();
//         auto forest2Copy = forest2->cloneForest();
//         auto uCopyInForest2 = forest2Copy->getLeafByLabel(u->label);
//         auto vCopyInForest2 = forest2Copy->getLeafByLabel(v->label);

//         // Detach the pendant subtree in tree 2 that is between u and v
//         auto lcaResultInCopy = lca(uCopyInForest2, vCopyInForest2);
//         auto pendantSubtrees = collectPendantSubtreesBetweenLeaves(uCopyInForest2, vCopyInForest2, lcaResultInCopy.first);

//         if (!pendantSubtrees.empty() && pendantSubtrees[0] != nullptr && pendantSubtrees[0]->parent != nullptr) {
//             forest2Copy->detachChild(pendantSubtrees[0]);

//             if (solve(k - 1, forest1Copy, forest2Copy) == 0) {
//                 return 0;
//             }
//         }


//     }
//     // Case 3: u,v are siblings in tree 1, but there are 2 or more pendant subtrees in tree2 between u and v
//     else {
//         // Branch on three cases: detach u, detach v, or detach the b many pendant subtrees in tree 2 that are between u and v
//         auto forest1Copy = forest1->cloneForest();
//         auto forest2Copy = forest2->cloneForest();

//         auto uCopy = forest1Copy->getLeafByLabel(u->label);
//         auto uCopyInForest2 = forest2Copy->getLeafByLabel(u->label);
//         if (uCopy != nullptr && uCopyInForest2 != nullptr && uCopy->parent != nullptr && uCopyInForest2->parent != nullptr) {
//             forest1Copy->detachChild(uCopy);
//             forest2Copy->detachChild(uCopyInForest2);

//             if (solve(k - 1, forest1Copy, forest2Copy) == 0) {
//                 return 0;
//             }
//         }

//         auto forest1Copy2 = forest1->cloneForest();
//         auto forest2Copy2 = forest2->cloneForest();

//         auto vCopy = forest1Copy2->getLeafByLabel(v->label);
//         auto vCopyInForest2 = forest2Copy2->getLeafByLabel(v->label);
//         if (vCopy != nullptr && vCopyInForest2 != nullptr && vCopy->parent != nullptr && vCopyInForest2->parent != nullptr) {
//             forest1Copy2->detachChild(vCopy);
//             forest2Copy2->detachChild(vCopyInForest2);

//             if (solve(k - 1, forest1Copy2, forest2Copy2) == 0) {
//                 return 0;
//             }
//         }

//         auto forest1Copy3 = forest1->cloneForest();
//         auto forest2Copy3 = forest2->cloneForest();
//         auto uCopyInForest2Case3 = forest2Copy3->getLeafByLabel(u->label);
//         auto vCopyInForest2Case3 = forest2Copy3->getLeafByLabel(v->label);

//         auto lcaResultInCopy = lca(uCopyInForest2Case3, vCopyInForest2Case3);
//         auto pendantSubtrees = collectPendantSubtreesBetweenLeaves(
//             uCopyInForest2Case3,
//             vCopyInForest2Case3,
//             lcaResultInCopy.first
//         );

//         if (!pendantSubtrees.empty()) {
//             int detachedPendantCount = 0;
//             for (const auto& subtreeRoot : pendantSubtrees) {
//                 if (subtreeRoot != nullptr && subtreeRoot->parent != nullptr) {
//                     forest2Copy3->detachChild(subtreeRoot);
//                     detachedPendantCount++;
//                 }
//             }

//             if (detachedPendantCount > 0 && solve(k - detachedPendantCount, forest1Copy3, forest2Copy3) == 0) {
//                 return 0;
//             }
//         }
//     }


//     return k;
// }
