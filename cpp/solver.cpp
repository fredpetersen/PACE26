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

namespace {

// Recursive structural hash of a tree. Symmetric over the two children so that
// (a,b) and (b,a) hash identically (consistent with the rest of the solver).
uint64_t hashNode(const TreeNode* n) {
    if (n == nullptr) return 0;
    if (n->isLeaf) {
        return std::hash<std::string>{}(n->label);
    }
    uint64_t hl = hashNode(n->left);
    uint64_t hr = hashNode(n->right);
    if (hl > hr) std::swap(hl, hr);
    return hl ^ (hr + 0x9e3779b97f4a7c15ULL + (hl << 6) + (hl >> 2));
}

// Multiset hash of a forest's roots (commutative — root order in the set is
// not semantically meaningful).
uint64_t hashForest(const Forest& f) {
    uint64_t acc = 0;
    for (auto* r : f.getRoots()) {
        acc += hashNode(r);
    }
    return acc;
}

// Ordered hash across forests: forests[0] (the MAF being built) is
// distinguished from forests[1..], so the order is significant.
uint64_t hashForests(const std::vector<std::shared_ptr<Forest>>& fs) {
    uint64_t h = fs.size();
    for (const auto& f : fs) {
        uint64_t fh = hashForest(*f);
        h ^= fh + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
    }
    return h;
}

} // namespace

void Solver::printForests() const {
    int i = 1;
    for (auto f : forests_) {
        f->print("Forest " + i++);
    }
}


void Solver::cleanSingletonLeaves(std::shared_ptr<Forest> mainForest, std::shared_ptr<Forest> otherForest, MutationTrail* trail) {
    // Snapshot the singleton root pointers up-front: detachByLabel below can
    // trigger cpsReduction which mutates mainForest->roots_, invalidating any
    // iterator we might hold over it.
    std::vector<TreeNode*> singletons;
    for (const auto& root : mainForest->getRoots()) {
        if (root != nullptr && root->isLeaf) {
            singletons.push_back(root);
        }
    }
    for (auto* root : singletons) {
        otherForest->detachByLabel(root->label, cpsMap_, trail);
    }
}

void Solver::initCpsReduction() {
    auto keys = std::stack<uint64_t>{};
    for (auto kv : cpsMap_) {
        if (kv.second == forests_.size()) { // TODO: is it forests_.size() - solvedForests
            keys.push(kv.first);
        }
    }
    while (!keys.empty()) {
        tryCpsReductionForHash(keys.top());
        keys.pop();
    }
}

void Solver::tryCpsReductionForHash(uint64_t cpsHash, MutationTrail* trail) {
    if (cpsHash != 0) {
        auto val = cpsMap_[cpsHash];
        if (val == forests_.size()) { // TODO: Here as well
            cpsReductionForCpsHash(cpsHash, trail);
        }
    }
}

void Solver::cpsReductionForCpsHash(uint64_t cpsHash, MutationTrail* trail) {
    for (auto forest : forests_) {
        auto h = forest->cpsReduction(forest->getNodeByCps(cpsHash), cpsMap_, trail);
        if (h != 0) {
            tryCpsReductionForHash(h, trail);
        }
    }
}

void Solver::detachByLabel(std::shared_ptr<Forest> forest, std::string label, MutationTrail* trail) {
    auto h = forest->detachByLabel(label, cpsMap_, trail);
    tryCpsReductionForHash(h, trail);
}

void Solver::detachChild(std::shared_ptr<Forest> forest, TreeNode* node, bool shouldContract, MutationTrail* trail) {
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

std::pair<bool, std::vector<std::shared_ptr<Forest>>> Solver::solve(int k, const std::vector<std::shared_ptr<Forest>>& forests) {
    //debug("Setting up checkpoint");
    MutationTrail trail;
    auto checkpoint = trail.checkpoint();
    auto result = solveRecursive(k, forests, trail);
    if (!result.first) {
        trail.rollback(checkpoint);
    }
    return result;
}

std::pair<bool, std::vector<std::shared_ptr<Forest>>> Solver::solveRecursive(int k, const std::vector<std::shared_ptr<Forest>>& forests,
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

    // Memoization: if this exact (canonicalized) state was already proven
    // infeasible at a budget >= k, prune immediately. The hash is also used
    // below to record this state as infeasible if every branch fails.
    const uint64_t stateHash = hashForests(forests);
    {
        auto it = failureCache_.find(stateHash);
        if (it != failureCache_.end() && it->second >= k) {
            return {false, {nullptr}};
        }
    }

    auto recordFailure = [&]() -> std::pair<bool, std::vector<std::shared_ptr<Forest>>> {
        auto& cached = failureCache_[stateHash];
        if (k > cached) cached = k;
        return {false, {nullptr}};
    };

    debug("4");
    // Step 4 (try to get a sibling pair in F2. If it can't be done, move on to solving F3)
    auto [idx, siblingPair] = f2->getOneSiblingPair();
    // debug(siblingPair.first->label + ", " + siblingPair.second->label);
    if (idx == -1) {
        auto checkpoint = trail.checkpoint();
        f1->expandMergedSubtrees(&trail);

        auto nextForests = forests;
        nextForests.erase(nextForests.begin() + 1);
        auto result = solveRecursive(k, nextForests, trail);
        if (result.first) {
            return result;
        }

        trail.rollback(checkpoint);
        return recordFailure();
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
            f1->forestLocalMergeCherry(leaf->parent, &trail);
            f2->forestLocalMergeCherry(otherLeaf->parent, &trail);
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

    return recordFailure();
}