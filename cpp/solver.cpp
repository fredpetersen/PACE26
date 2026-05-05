#include <solver.h>

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

void Solver::printForests() const {
    // TODO: Rework this to work actually good
    // forest1_->print("Forest 1");
    // forest2_->print("Forest 2");
    std::cout << "not implemented, printForests()" << std::endl;
}


void Solver::cleanSingletonLeaves(std::shared_ptr<Forest> mainForest, std::shared_ptr<Forest> otherForest, MutationTrail* trail) {
    for (const auto& root : mainForest->getRoots()) {
        if (root != nullptr && root->isLeaf) {
            // std::cout << "# CSL" << std::endl;
            otherForest->detachByLabel(root->label, trail);
        }
    }
}

void Solver::initCpsReduction() {
    auto dummyTrail = MutationTrail(); // We don't need to keep a track of the trail, since these changes are ALWAYS correct
    auto leftChildLabels = std::vector<std::string>();
    for (auto kv : cpsMap_) {
        if (kv.second == forests_.size()) {
            auto str = kv.first;
            auto comma = str.find(',');
            auto l = str.substr(1, comma - 1);
            leftChildLabels.push_back(l);
        }
    }
    // for (auto label : leftChildLabels) {
    //     for (auto& forest : forests_) {
    //         // Have to move the recursiveCpsReduction to the forest class, so it can update the forest stuff (like leaves_)
    //         forest->recursiveCpsReduction(forest->getLeafByLabel(label)->parent);
    //         auto node = forest->getLeafByLabel(label)->parent.get();
    //         forest->recursiveCpsReduction(node, &dummyTrail);
    //     }
    // }
}


std::shared_ptr<Forest> Solver::solve() {
    initCpsReduction();
    bool isSolved = false;
    std::shared_ptr<Forest> solution;
    int k = 1;
    // No reason to add a looping condition on k, theres is always a trivial solution with k = nr. of leaves
    while (!isSolved) {
        std::cout << "# Looking for solution with size k = " << k << std::endl;
        auto res = solve(k++);
        isSolved = res.first;
        solution = res.second;
    }
    return solution;
}

std::pair<bool, std::shared_ptr<Forest>> Solver::solve(int k) {
    auto res = solve(k, forests_);
    return {res.first, res.second[0]};
}

std::pair<bool, std::vector<std::shared_ptr<Forest>>> Solver::solve(int k, std::vector<std::shared_ptr<Forest>> forests) {
    MutationTrail trail;
    auto checkpoint = trail.checkpoint();
    auto result = solveRecursive(k, std::move(forests), trail);
    if (!result.first) {
        trail.rollback(checkpoint);
    }
    return result;
}

std::pair<bool, std::vector<std::shared_ptr<Forest>>> Solver::solveRecursive(int k, std::vector<std::shared_ptr<Forest>> forests,
                                                                            MutationTrail& trail) {
    // std::cout << "# 1" << std::endl;    
    if (forests.size() < 1) {
        return {true, {nullptr}};
    }

    auto f1 = forests[0];

    // std::cout << "# 2" << std::endl;
    // Step 1 (if there's only 1 forest left, this forest is the MAF)
    if (forests.size() == 1) {
        return {true, forests};
    }

    auto f2 = forests[1];
    // f1->print("Forest 1");
    // f2->print("Forest 2");
    // std::cout << "# 3" << std::endl;
    // Step 2 (if there are more components on the MAF we're building than k, the solution is too big (i.e. invalid))
    if (f1->getComponentCount() > k) {
        return {false, {nullptr}};
    }

    // std::cout << "# 4" << std::endl;
    // Step 3 (clean the single vertex trees)
    cleanSingletonLeaves(f1, f2, &trail);
    cleanSingletonLeaves(f2, f1, &trail);

    // std::cout << "# 5" << std::endl;
    // Step 4 (try to get a sibling pair in F2. If it can't be done, move on to solving F3)
    auto [idx, siblingPair] = f2->getOneSiblingPair();
    // std::cout << siblingPair.first << ", " << siblingPair.second << std::endl;
    if (idx == -1) {
        auto checkpoint = trail.checkpoint();
        f1->expandMergedSubtrees(&trail);

        auto nextForests = forests;
        nextForests.erase(nextForests.begin() + 1);
        auto result = solveRecursive(k, std::move(nextForests), trail);
        if (result.first) {
            return result;
        }

        trail.rollback(checkpoint);
        return {false, {nullptr}};
    }

    // std::cout << "# 6" << std::endl;
    // Step 5
    auto lab_u = siblingPair.first->label;
    auto lab_v = siblingPair.second->label;
    auto [ancestor, dist] = f1->lca(lab_u, lab_v);

    // std::cout << "# 7" << std::endl;
    if (dist == -1) {
        auto checkpoint = trail.checkpoint();
        f1->detachByLabel(lab_u, &trail);
        f2->detachByLabel(lab_u, &trail);
        auto result = solveRecursive(k, forests, trail);
        if (result.first) {
            return result;
        }
        trail.rollback(checkpoint);

        checkpoint = trail.checkpoint();
        f1->detachByLabel(lab_v, &trail);
        f2->detachByLabel(lab_v, &trail);
        result = solveRecursive(k, forests, trail);
        if (result.first) {
            return result;
        }
        trail.rollback(checkpoint);
    // std::cout << "# 8" << std::endl;
    } else if (dist == 1) {
        auto checkpoint = trail.checkpoint();
        auto leaf = f1->getLeafByLabel(lab_u);
        auto otherLeaf = f2->getLeafByLabel(lab_u);
        if (leaf != nullptr && leaf->parent != nullptr && otherLeaf != nullptr && otherLeaf->parent != nullptr) {
            f1->forestLocalMergeCherry(leaf->parentShared(), &trail);
            f2->forestLocalMergeCherry(otherLeaf->parentShared(), &trail);
            auto result = solveRecursive(k, forests, trail);
            if (result.first) {
                return result;
            }
        }
        trail.rollback(checkpoint);
    // std::cout << "# 9" << std::endl;
    } else if (dist > 1) {
        auto checkpoint = trail.checkpoint();
        f1->detachByLabel(lab_u, &trail);
        f2->detachByLabel(lab_u, &trail);
        auto result = solveRecursive(k, forests, trail);
        if (result.first) {
            return result;
        }
        trail.rollback(checkpoint);

        checkpoint = trail.checkpoint();
        f1->detachByLabel(lab_v, &trail);
        f2->detachByLabel(lab_v, &trail);
        result = solveRecursive(k, forests, trail);
        if (result.first) {
            return result;
        }
        trail.rollback(checkpoint);

        checkpoint = trail.checkpoint();
        auto cloneAncestor = ancestor;
        auto newRoots = f1->collectPendantSubtreesBetweenLeaves(lab_u, lab_v, cloneAncestor);
        for (const auto& r : newRoots) {
            f1->detachChild(r, false, &trail);
        }
        f1->contractIntoCherry(lab_u, lab_v, cloneAncestor, &trail);
        result = solveRecursive(k, forests,trail);
        if (result.first) {
            return result;
        }
        trail.rollback(checkpoint);
    }

    return {false, {nullptr}};
}