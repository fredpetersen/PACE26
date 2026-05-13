#include <solver.h>

#include <algorithm>
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

// CPS reduction firing counters (process-global, sufficient for benchmarking
// a single solve() invocation per process).
long g_cpsInitFirings = 0;       // reductions that happened during initCpsReduction
long g_cpsRuntimeFirings = 0;    // reductions during search (post-init)
long g_cpsRuntimeTriggers = 0;   // calls to tryCpsReductionForHash during search
long g_cpsRuntimeMatched = 0;    // tryCps... where cpsMap_[h] == activeForestCount_
bool g_cpsInInit = false;

// Multiset hash of a forest's roots (commutative — root order in the set is
// not semantically meaningful). Delegates to TreeNode::hashSubtree so the CPS
// reduction and the failure cache share a single hash implementation.
uint64_t hashForest(const Forest& f) {
    uint64_t acc = 0;
    for (auto* r : f.getRoots()) {
        acc += hashSubtree(r);
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
        f->print("Forest " + std::to_string(i++));
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
#ifdef DISABLE_CPS_REDUCTION
    activeForestCount_ = static_cast<int>(forests_.size());
    return;
#endif
    // Only cherry-of-leaves nodes are enrolled at parse time, so every
    // initial cpsMap entry has subtree size 3 — no need to sort by size.
    // Cascades up the tree are handled by tryCpsReductionForHash on the
    // parent hash returned from cpsReduction.
    activeForestCount_ = static_cast<int>(forests_.size());
    std::vector<uint64_t> hashes;
    hashes.reserve(cpsMap_.size());
    for (const auto& kv : cpsMap_) {
        if (kv.second == activeForestCount_) hashes.push_back(kv.first);
    }
    // During init we always want the reducer to fire and cascade to a fixed
    // point, regardless of the DISABLE_RUNTIME_CPS_REDUCTION flag. We use a
    // local worklist instead of the gated tryCpsReductionForHash recursion.
    g_cpsInInit = true;
    std::vector<uint64_t> work = std::move(hashes);
    while (!work.empty()) {
        uint64_t h = work.back();
        work.pop_back();
        if (h == 0) continue;
        auto it = cpsMap_.find(h);
        if (it == cpsMap_.end() || it->second != activeForestCount_) continue;
        for (auto forest : forests_) {
            if (inactiveForests_.count(forest.get())) continue;
            auto* node = forest->getNodeByCps(h);
            if (node == nullptr) continue;
            auto parentHash = forest->cpsReduction(node, cpsMap_, /*trail=*/nullptr);
            // cpsReduction returns 0 on no-op (early-return guards) or the
            // post-reduction parent hash on success. A successful firing
            // can still legitimately return 0 if the merged node had no
            // parent; we therefore detect "fired" by checking that the
            // node's hash entry was either removed or replaced.
            ++g_cpsInitFirings; // upper bound: counts attempted call after guard pass
            if (parentHash != 0) work.push_back(parentHash);
        }
    }
    g_cpsInInit = false;
}

void Solver::tryCpsReductionForHash(uint64_t cpsHash, MutationTrail* trail) {
#if defined(DISABLE_CPS_REDUCTION) || defined(DISABLE_RUNTIME_CPS_REDUCTION)
    (void)cpsHash; (void)trail;
    return;
#endif
    if (cpsHash != 0) {
        ++g_cpsRuntimeTriggers;
        auto val = cpsMap_[cpsHash];
        if (val == activeForestCount_) {
            ++g_cpsRuntimeMatched;
            cpsReductionForCpsHash(cpsHash, trail);
        }
    }
}

void Solver::cpsReductionForCpsHash(uint64_t cpsHash, MutationTrail* trail) {
#ifdef DISABLE_CPS_REDUCTION
    (void)cpsHash; (void)trail;
    return;
#endif
    for (auto forest : forests_) {
        if (inactiveForests_.count(forest.get())) continue;
        auto* node = forest->getNodeByCps(cpsHash);
        if (node == nullptr) continue;
        auto h = forest->cpsReduction(node, cpsMap_, trail);
        if (!g_cpsInInit) ++g_cpsRuntimeFirings;
        if (h != 0) {
            tryCpsReductionForHash(h, trail);
        }
    }
}

void Solver::deactivateForest(std::shared_ptr<Forest> f, MutationTrail& trail) {
    if (f == nullptr) return;
    Forest* raw = f.get();
    if (inactiveForests_.count(raw)) return;

    // 1. Snapshot which (hash, node) entries from f are still structurally
    //    consistent (subtreeHash matches the key). Stale entries from prior
    //    detach/contract chains are skipped — they were already not being
    //    counted as f's contribution to a meaningful CPS lookup.
    std::vector<uint64_t> touched;
    touched.reserve(f->getNodesByCps().size());
    for (const auto& kv : f->getNodesByCps()) {
        TreeNode* n = kv.second;
        if (n == nullptr) continue;
        if (hashSubtree(n) != kv.first) continue;
        touched.push_back(kv.first);
    }

    // 2. Decrement cpsMap_ for each touched key, with trail-recorded undo.
    for (auto h : touched) {
        cpsMap_[h] -= 1;
        trail.record([this, h]() { cpsMap_[h] += 1; });
    }

    // 3. Decrement active count + mark inactive, with undo entries.
    activeForestCount_ -= 1;
    trail.record([this]() { activeForestCount_ += 1; });

    inactiveForests_.insert(raw);
    trail.record([this, raw]() { inactiveForests_.erase(raw); });

    // 4. Re-fire CPS reduction for any touched hash whose count now equals
    //    the new activeForestCount_. This is exactly the scenario the user
    //    flagged: a CPS shared by f3..fN that wasn't all-N before now is.
    for (auto h : touched) {
        tryCpsReductionForHash(h, &trail);
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
    std::cerr << "# cps init_firings=" << g_cpsInitFirings
              << " runtime_firings=" << g_cpsRuntimeFirings
              << " runtime_triggers=" << g_cpsRuntimeTriggers
              << " runtime_matched=" << g_cpsRuntimeMatched
              << std::endl;
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

        // Logically drop f2: subtract its contribution from cpsMap_, mark it
        // inactive, and re-evaluate any CPS that may now be present in all
        // remaining active forests.
        deactivateForest(f2, trail);

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