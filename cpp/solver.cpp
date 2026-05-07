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
#include <stack>

#include <problem_instance.h>
#include <tree_node.h>
#include <forest.h>

void Solver::printForests() const {
    int i = 1;
    for (auto f : forests_) {
        f->print("Forest " + i++);
    }
}


void Solver::cleanSingletonLeaves(std::shared_ptr<Forest> mainForest, std::shared_ptr<Forest> otherForest, MutationTrail* trail) {
    for (const auto& root : mainForest->getRoots()) {
        if (root != nullptr && root->isLeaf) {
            otherForest->detachByLabel(root->label, cpsMap_, trail);
        }
    }
}

void Solver::initCpsReduction() {
    auto keys = std::stack<std::string>{};
    for (auto kv : cpsMap_) {
        // debug(kv.first + ", " + std::to_string(kv.second));
        if (kv.second == forests_.size()) { // TODO: is it forests_.size() - solvedForests
            keys.push(kv.first);
        }
    }
    while (!keys.empty()) {
        tryCpsReductionForHash(keys.top());
        keys.pop();
    }
}

void Solver::tryCpsReductionForHash(std::string cpsHash, MutationTrail* trail) {
    if (cpsHash != "") {
        auto val = cpsMap_[cpsHash];
        if (val == forests_.size()) { // TODO: Here as well
            // debug(cpsHash + " has reached the criteria!");
            cpsReductionForCpsHash(cpsHash, trail);
        }
    }
}

void Solver::cpsReductionForCpsHash(std::string cpsHash, MutationTrail* trail) {
    // debug("Trying to reduce " + cpsHash);
    for (auto forest : forests_) {
        auto h = forest->cpsReduction(forest->getNodeByCps(cpsHash), cpsMap_, trail);
        if (h != "") {
            tryCpsReductionForHash(h, trail);
        }
    }
}

void Solver::detachByLabel(std::shared_ptr<Forest> forest, std::string label, MutationTrail* trail) {
    auto h = forest->detachByLabel(label, cpsMap_, trail);
    tryCpsReductionForHash(h, trail);
}

void Solver::detachChild(std::shared_ptr<Forest> forest, std::shared_ptr<TreeNode> node, bool shouldContract, MutationTrail* trail) {
    auto h = forest->detachChild(node, cpsMap_, shouldContract, trail);
    tryCpsReductionForHash(h, trail);
}

std::shared_ptr<Forest> Solver::solve() {
    debug("Started Solving...");
    initCpsReduction();
    debug("Init Reduction Complete");
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
    //debug("Setting up checkpoint");
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
    if (forests.size() < 1) {
        return {true, {nullptr}};
    }

    debug("1");
    // Step 1 (if there's only 1 forest left, this forest is the MAF)
    if (forests.size() == 1) {
        return {true, forests};
    }

    auto f1 = forests[0];
    auto f2 = forests[1];

    // f1->print("Forest 1");
    // f2->print("Forest 2");

    debug("2");
    // Step 2 (if there are more components on the MAF we're building than k, the solution is too big (i.e. invalid))
    if (f1->getComponentCount() > k) {
        return {false, {nullptr}};
    }

    debug("3");
    // Step 3 (clean the single vertex trees)
    cleanSingletonLeaves(f1, f2, &trail);
    cleanSingletonLeaves(f2, f1, &trail);

    debug("4");
    // Step 4 (try to get a sibling pair in F2. If it can't be done, move on to solving F3)
    auto [idx, siblingPair] = f2->getOneSiblingPair();
    // debug(siblingPair.first->label + ", " + siblingPair.second->label);
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

    debug("5");
    // Step 5
    auto lab_u = siblingPair.first->label;
    auto lab_v = siblingPair.second->label;
    auto [ancestor, dist] = f1->lca(lab_u, lab_v);

    debug("6");
    if (dist == -1) {
        debug("6.1");
        auto checkpoint = trail.checkpoint();
        detachByLabel(f1, lab_u, &trail);
        detachByLabel(f2, lab_u, &trail);
        auto result = solveRecursive(k, forests, trail);
        if (result.first) {
            return result;
        }
        trail.rollback(checkpoint);

        debug("6.2");
        checkpoint = trail.checkpoint();
        detachByLabel(f1, lab_v, &trail);
        detachByLabel(f2, lab_v, &trail);
        result = solveRecursive(k, forests, trail);
        if (result.first) {
            return result;
        }
        trail.rollback(checkpoint);

    debug("7");
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

    debug("8");
    } else if (dist > 1) {
        debug("8.1");
        auto checkpoint = trail.checkpoint();
        detachByLabel(f1, lab_u, &trail);
        detachByLabel(f2, lab_u, &trail);
        auto result = solveRecursive(k, forests, trail);
        if (result.first) {
            return result;
        }
        trail.rollback(checkpoint);

        debug("8.2");
        checkpoint = trail.checkpoint();
        detachByLabel(f1, lab_v, &trail);
        detachByLabel(f2, lab_v, &trail);
        result = solveRecursive(k, forests, trail);
        if (result.first) {
            return result;
        }
        trail.rollback(checkpoint);

        debug("8.3");
        checkpoint = trail.checkpoint();
        auto cloneAncestor = ancestor;
        auto newRoots = f1->collectPendantSubtreesBetweenLeaves(lab_u, lab_v, cloneAncestor);
        for (const auto& r : newRoots) {
            detachChild(f1, r, false, &trail);
        }
        f1->contractIntoCherry(lab_u, lab_v, cloneAncestor, &trail);
        result = solveRecursive(k, forests, trail);
        if (result.first) {
            return result;
        }
        trail.rollback(checkpoint);
    }

    return {false, {nullptr}};
}