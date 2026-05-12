#include <solver.h>
#include <cstdlib>

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

#include <input_reader.h>
#include <problem_instance.h>
#include <tree_node.h>
#include <forest.h>

namespace {

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
    for (auto h : hashes) {
        tryCpsReductionForHash(h);
    }
}

void Solver::analyzeCommonClusters() const {
    // Diagnostic-only Phase 1 of cluster reduction. For every active forest,
    // walk every internal node, compute (leafSetHash, leafSetSize) and the
    // structural hash. A leaf-set hash present in ALL active forests with
    // 1 < size < L is a common-cluster candidate; we count both candidates
    // and the subset of those that are ALSO structurally identical (already
    // caught by CPS reduction). The delta is the headroom cluster reduction
    // gives us over what we already have.
    const int N = activeForestCount_;
    if (N == 0) return;

    struct Agg {
        int forestCount = 0;        // # of active forests containing this leaf set
        int size = 0;               // |C|
        int lastForestIdx = -1;     // dedupe within a single forest
        uint64_t structHash = 0;    // structural hash if all sightings agree, else 0
        bool structConsistent = true;
    };
    std::unordered_map<uint64_t, Agg> agg;

    int activeIdx = 0;
    for (const auto& f : forests_) {
        if (f == nullptr) continue;
        if (inactiveForests_.count(f.get())) continue;
        const int forestIdx = activeIdx++;

        // DFS every internal node in every root.
        std::vector<const TreeNode*> stack;
        for (auto* r : f->getRoots()) stack.push_back(r);
        while (!stack.empty()) {
            const TreeNode* n = stack.back();
            stack.pop_back();
            if (n == nullptr) continue;
            if (!n->isLeaf) {
                if (n->left)  stack.push_back(n->left);
                if (n->right) stack.push_back(n->right);
                const uint64_t lh = hashLeafSet(n);
                const int sz = countLeaves(n);
                if (sz < 2 || sz >= leafCount_) continue;
                const uint64_t sh = hashSubtree(n);
                auto& a = agg[lh];
                if (a.lastForestIdx == forestIdx) continue; // count each forest once
                a.lastForestIdx = forestIdx;
                a.forestCount += 1;
                a.size = sz;
                if (a.forestCount == 1) {
                    a.structHash = sh;
                } else if (a.structHash != sh) {
                    a.structConsistent = false;
                }
            }
        }
    }

    int totalCandidates = 0;
    int structIdentical = 0;
    int leafSetOnly = 0;       // candidates only equal as leaf sets (the headroom)
    std::vector<std::pair<int,uint64_t>> bySize;
    for (const auto& kv : agg) {
        if (kv.second.forestCount != N) continue;
        totalCandidates += 1;
        if (kv.second.structConsistent) structIdentical += 1;
        else                            leafSetOnly += 1;
        bySize.emplace_back(kv.second.size, kv.first);
    }
    std::sort(bySize.begin(), bySize.end());

    std::cout << "# cluster: active_forests=" << N
              << " leaves=" << leafCount_
              << " candidates=" << totalCandidates
              << " (struct_identical=" << structIdentical
              << ", leafset_only=" << leafSetOnly << ")"
              << std::endl;
    int shown = 0;
    for (const auto& p : bySize) {
        if (shown++ >= 10) break;
        const auto& a = agg[p.second];
        std::cout << "# cluster:  size=" << a.size
                  << " struct_consistent=" << (a.structConsistent ? "yes" : "no")
                  << " leafSetHash=" << p.second
                  << std::endl;
    }
}

void Solver::tryCpsReductionForHash(uint64_t cpsHash, MutationTrail* trail) {
    if (cpsHash != 0) {
        auto val = cpsMap_[cpsHash];
        if (val == activeForestCount_) {
            cpsReductionForCpsHash(cpsHash, trail);
        }
    }
}

void Solver::cpsReductionForCpsHash(uint64_t cpsHash, MutationTrail* trail) {
    for (auto forest : forests_) {
        if (inactiveForests_.count(forest.get())) continue;
        auto h = forest->cpsReduction(forest->getNodeByCps(cpsHash), cpsMap_, trail);
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
    // Cluster reduction must run on pristine trees (before CPS reduction
    // merges cherries into nodes whose label is a Newick fragment like
    // "(41,42)"). After CPS merge, countLeaves on the parent of a merged
    // cherry returns the wrong (smaller) count because the merged child has
    // isLeaf=true and left=right=nullptr, while emit/treeToNewick still
    // produces the multi-leaf string from the merged label, leading to a
    // size mismatch between the inner sub-instance and what the inner
    // solver actually receives.
    analyzeCommonClusters();
    int extracted = std::getenv("NO_CLUSTER") ? 0 : reduceClusters();
    if (extracted > 0) {
        std::cout << "# cluster: extracted " << extracted
                  << " cluster(s); residual leaves=" << leafCount_
                  << std::endl;
    }
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
    if (pendingExpansions_.empty()) return solution;

    // --- Stitch cluster expansions back in (reverse order of extraction) ---
    // Convert solution's components to Newicks; for each pending expansion,
    // find the placeholder leaf label as a token, substitute the spine,
    // then append the extras as new components.
    std::vector<std::string> components;
    for (auto* r : solution->getRoots()) {
        components.push_back(solution->treeToNewick(r));
    }
    for (auto it = pendingExpansions_.rbegin(); it != pendingExpansions_.rend(); ++it) {
        const auto& exp = *it;
        // Locate the unique component containing the placeholder leaf.
        // Token boundaries: Newick leaf labels are bounded by '(', ',', ')'.
        bool spliced = false;
        for (auto& comp : components) {
            // Search for the placeholder label as a maximal digit run.
            std::string needle = exp.placeholderLabel;
            std::size_t pos = 0;
            while ((pos = comp.find(needle, pos)) != std::string::npos) {
                bool leftOk  = (pos == 0)
                    || comp[pos - 1] == '(' || comp[pos - 1] == ',';
                std::size_t end = pos + needle.size();
                bool rightOk = (end == comp.size())
                    || comp[end] == ')' || comp[end] == ',';
                if (leftOk && rightOk) {
                    comp.replace(pos, needle.size(), exp.spineNewick);
                    spliced = true;
                    break;
                }
                pos += needle.size();
            }
            if (spliced) break;
        }
        if (!spliced) {
            // Placeholder must appear exactly once; if not, something is wrong.
            std::cerr << "# cluster: WARNING placeholder " << exp.placeholderLabel
                      << " not found in any output component" << std::endl;
        }
        for (const auto& extra : exp.extraNewicks) {
            components.push_back(extra);
        }
    }

    // Re-parse the stitched components into a single Forest so the existing
    // main() interface (`result->printForestNewick()`) works unchanged.
    std::unordered_map<uint64_t, int> dummyCps;
    auto stitched = rebuildForestsFromNewicks(components, dummyCps);
    // rebuildForestsFromNewicks returns one Forest per Newick string; merge
    // their roots into a single Forest.
    auto merged = std::make_shared<Forest>();
    std::unordered_set<TreeNode*> mergedRoots;
    std::unordered_set<TreeNode*> mergedLeaves;
    std::unordered_map<std::string, TreeNode*> mergedLeafByLabel;
    for (const auto& f : stitched) {
        for (auto* r : f->getRoots()) mergedRoots.insert(r);
        for (auto* l : f->getLeaves()) {
            mergedLeaves.insert(l);
            mergedLeafByLabel[l->label] = l;
        }
    }
    *merged = Forest(mergedRoots, mergedLeaves, mergedLeafByLabel,
                     std::unordered_map<uint64_t, TreeNode*>{});
    return merged;
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

// ---------------------------------------------------------------------------
// Cluster reduction (Phase 2)
// ---------------------------------------------------------------------------

std::optional<Solver::ClusterCandidate> Solver::findSmallestCommonCluster() const {
    const int N = activeForestCount_;
    if (N <= 0) return std::nullopt;

    struct Agg {
        int forestCount = 0;
        int size = 0;
        int lastForestIdx = -1;
        std::vector<TreeNode*> nodes;  // first sighting in each active forest
    };
    std::unordered_map<uint64_t, Agg> agg;

    int activeIdx = 0;
    for (const auto& f : forests_) {
        if (f == nullptr) continue;
        if (inactiveForests_.count(f.get())) continue;
        const int forestIdx = activeIdx++;
        std::vector<TreeNode*> stack;
        for (auto* r : f->getRoots()) stack.push_back(r);
        while (!stack.empty()) {
            TreeNode* n = stack.back();
            stack.pop_back();
            if (n == nullptr || n->isLeaf) continue;
            if (n->left)  stack.push_back(n->left);
            if (n->right) stack.push_back(n->right);
            const uint64_t lh = hashLeafSet(n);
            const int sz = countLeaves(n);
            if (sz < 2 || sz >= leafCount_) continue;
            auto& a = agg[lh];
            if (a.lastForestIdx == forestIdx) continue;
            a.lastForestIdx = forestIdx;
            a.forestCount += 1;
            a.size = sz;
            if ((int)a.nodes.size() < forestIdx + 1) a.nodes.resize(forestIdx + 1, nullptr);
            a.nodes[forestIdx] = n;
        }
    }

    // Helper: collect sorted leaf labels of a subtree.
    auto collectLeaves = [](const TreeNode* n) {
        std::vector<std::string> out;
        std::vector<const TreeNode*> st{n};
        while (!st.empty()) {
            const TreeNode* x = st.back(); st.pop_back();
            if (!x) continue;
            if (x->isLeaf) out.push_back(x->label);
            else { st.push_back(x->left); st.push_back(x->right); }
        }
        std::sort(out.begin(), out.end());
        return out;
    };

    // Pick the smallest candidate present in all N active forests whose
    // actual leaf-label set agrees across every sighting (defends against
    // 64-bit XOR hash collisions on leaf sets).
    std::vector<std::pair<int, uint64_t>> ranked;
    for (const auto& kv : agg) {
        if (kv.second.forestCount != N) continue;
        ranked.emplace_back(kv.second.size, kv.first);
    }
    std::sort(ranked.begin(), ranked.end());
    for (const auto& pr : ranked) {
        const Agg& a = agg.at(pr.second);
        std::vector<std::string> ref = collectLeaves(a.nodes[0]);
        bool ok = true;
        for (int i = 1; i < N; ++i) {
            if (collectLeaves(a.nodes[i]) != ref) { ok = false; break; }
        }
        if (!ok) {
            std::cerr << "# cluster: WARNING leaf-set hash collision at hash="
                      << pr.second << " size=" << pr.first << "; skipping" << std::endl;
            continue;
        }
        ClusterCandidate cc;
        cc.leafSetHash = pr.second;
        cc.size = a.size;
        cc.nodePerForest = a.nodes;
        cc.nodePerForest.resize(N, nullptr);
        return cc;
    }
    return std::nullopt;
}

std::string Solver::newickWithReplacement(const TreeNode* node,
                                          const TreeNode* replaceTarget,
                                          const std::string& placeholderLabel) {
    if (!node) return "";
    if (node == replaceTarget) return placeholderLabel;
    if (node->isLeaf) return node->label;
    std::string result = "(";
    if (node->left)  result += newickWithReplacement(node->left,  replaceTarget, placeholderLabel);
    if (node->left && node->right) result += ",";
    if (node->right) result += newickWithReplacement(node->right, replaceTarget, placeholderLabel);
    result += ")";
    if (node->label != "0") result += node->label;
    return result;
}

std::vector<std::shared_ptr<Forest>> Solver::rebuildForestsFromNewicks(
    const std::vector<std::string>& newicks,
    std::unordered_map<uint64_t, int>& outCpsMap) {
    std::vector<std::shared_ptr<Forest>> out;
    out.reserve(newicks.size());
    for (const auto& nw : newicks) {
        // The parser's grammar requires a trailing ';'; tolerate either.
        std::string text = nw;
        if (text.empty() || text.back() != ';') text += ';';
        NewickParser parser(text, nodeArena_);
        std::unordered_map<std::string, TreeNode*> leafByLabel;
        std::unordered_map<uint64_t, TreeNode*> nodeByCps;
        auto parsed = parser.parseTree(leafByLabel, nodeByCps, outCpsMap);
        auto* root = parsed.first;
        auto leaves = std::move(parsed.second);
        out.push_back(std::make_shared<Forest>(
            std::unordered_set<TreeNode*>{root}, leaves, leafByLabel, nodeByCps));
    }
    return out;
}

int Solver::reduceClusters() {
    int extracted = 0;
    while (true) {
        auto found = findSmallestCommonCluster();
        if (!found) break;
        const auto& cc = *found;

        // Build the placeholder label for this extraction.
        const std::string placeholder = std::to_string(nextPlaceholderLabel_++);

        // 1. Generate inner Newicks (cluster subtree per active forest, in
        //    the same order as nodePerForest).
        std::vector<std::string> innerNewicks;
        innerNewicks.reserve(cc.nodePerForest.size());
        for (auto* n : cc.nodePerForest) {
            if (n == nullptr) {
                std::cerr << "# cluster: ERROR null cluster node" << std::endl;
                return extracted;
            }
            innerNewicks.push_back(Forest::canonicalNewick(n).empty()
                                    ? std::string()
                                    : std::string());
            // Use plain treeToNewick (we don't need canonicalization for I/O).
            innerNewicks.back().clear();
        }
        // Re-emit using non-canonical, faithful Newick (preserves original
        // labels and structure exactly). Build a temp Forest just to call
        // treeToNewick, or replicate the recursion. The free function below
        // mirrors Forest::treeToNewick.
        std::function<std::string(const TreeNode*)> emit =
            [&](const TreeNode* n) -> std::string {
            if (!n) return "";
            if (n->isLeaf) return n->label;
            std::string s = "(";
            if (n->left)  s += emit(n->left);
            if (n->left && n->right) s += ",";
            if (n->right) s += emit(n->right);
            s += ")";
            if (n->label != "0") s += n->label;
            return s;
        };
        for (std::size_t i = 0; i < cc.nodePerForest.size(); ++i) {
            innerNewicks[i] = emit(cc.nodePerForest[i]);
        }

        // 2. Generate outer Newicks (whole tree with cluster replaced by the
        //    placeholder leaf). Build per active forest in the same order.
        std::vector<std::string> outerNewicks;
        outerNewicks.reserve(cc.nodePerForest.size());
        std::size_t fi = 0;
        for (const auto& f : forests_) {
            if (f == nullptr || inactiveForests_.count(f.get())) continue;
            // Each forest in our setup has exactly one root (it's a tree).
            // If multiple roots existed we'd need to pick the one containing
            // the cluster; not expected here at top-level.
            TreeNode* root = nullptr;
            for (auto* r : f->getRoots()) { root = r; break; }
            outerNewicks.push_back(newickWithReplacement(root, cc.nodePerForest[fi], placeholder));
            ++fi;
        }

        // 3. Solve the inner instance recursively.
        // Capture the leaf-label sets of the cluster's two child subtrees in
        // the FIRST input tree. The "spine" inner block is the unique one
        // whose leaves intersect both children (i.e., whose LCA in T_0[C]
        // equals the cluster root). Picking the largest is unsound: gluing
        // a non-root inner block as the spine produces an invalid AF in the
        // full trees (the mixed block's path through the cluster region
        // overlaps other inner blocks' edges).
        std::unordered_set<std::string> clusterLeftLeaves;
        std::unordered_set<std::string> clusterRightLeaves;
        if (!cc.nodePerForest.empty() && cc.nodePerForest[0]) {
            auto* clusterRoot = cc.nodePerForest[0];
            std::function<void(const TreeNode*, std::unordered_set<std::string>&)> gather =
                [&](const TreeNode* n, std::unordered_set<std::string>& out) {
                    if (!n) return;
                    if (n->isLeaf) { out.insert(n->label); return; }
                    gather(n->left, out);
                    gather(n->right, out);
                };
            gather(clusterRoot->left,  clusterLeftLeaves);
            gather(clusterRoot->right, clusterRightLeaves);
        }

        std::unordered_map<uint64_t, int> innerCpsMap;
        auto innerForests = rebuildForestsFromNewicks(innerNewicks, innerCpsMap);
        Solver innerSolver(innerForests, cc.size, innerCpsMap);
        std::cout << "# cluster: extracting size=" << cc.size
                  << " placeholder=" << placeholder
                  << " (residual would be " << (leafCount_ - cc.size + 1) << ")"
                  << std::endl;
        if (std::getenv("DUMP_OUTER")) {
            std::cerr << "# cluster: inner Newicks (size=" << cc.size << "):\n";
            for (const auto& nw : innerNewicks) std::cerr << "  " << nw << "\n";
        }
        auto innerResult = innerSolver.solve();
        if (innerResult == nullptr) {
            std::cerr << "# cluster: ERROR inner solve returned null" << std::endl;
            return extracted;
        }
        // Collect inner components; identify the spine as the unique
        // component whose leaves intersect both children of the cluster
        // root in T_0[C]. This is the block whose LCA in T_0[C] equals the
        // cluster root, which is the block that must glue with the outer
        // block via the placeholder.
        std::vector<std::string> innerComponents;
        std::vector<std::vector<std::string>> innerLeafLists;
        for (auto* r : innerResult->getRoots()) {
            std::vector<std::string> ls;
            std::function<void(const TreeNode*)> walk = [&](const TreeNode* n) {
                if (!n) return;
                if (n->isLeaf) { ls.push_back(n->label); return; }
                walk(n->left); walk(n->right);
            };
            walk(r);
            innerLeafLists.push_back(std::move(ls));
            innerComponents.push_back(emit(r));
        }
        if (innerComponents.empty()) {
            std::cerr << "# cluster: ERROR inner MAF has 0 components" << std::endl;
            return extracted;
        }
        std::size_t spineIdx = static_cast<std::size_t>(-1);
        for (std::size_t i = 0; i < innerLeafLists.size(); ++i) {
            bool hitsLeft = false, hitsRight = false;
            for (const auto& lbl : innerLeafLists[i]) {
                if (!hitsLeft  && clusterLeftLeaves.count(lbl))  hitsLeft  = true;
                if (!hitsRight && clusterRightLeaves.count(lbl)) hitsRight = true;
                if (hitsLeft && hitsRight) break;
            }
            if (hitsLeft && hitsRight) {
                if (spineIdx != static_cast<std::size_t>(-1)) {
                    std::cerr << "# cluster: ERROR multiple inner blocks span both "
                                 "cluster children; AF invariant violated" << std::endl;
                }
                spineIdx = i;
            }
        }
        if (spineIdx == static_cast<std::size_t>(-1)) {
            std::cerr << "# cluster: ERROR no inner block spans both cluster children; "
                         "cannot identify spine" << std::endl;
            return extracted;
        }
        ClusterExpansion exp;
        exp.placeholderLabel = placeholder;
        exp.spineNewick = innerComponents[spineIdx];
        for (std::size_t i = 0; i < innerComponents.size(); ++i) {
            if (i == spineIdx) continue;
            exp.extraNewicks.push_back(innerComponents[i]);
        }
        pendingExpansions_.push_back(std::move(exp));

        // 4. Replace this Solver's instance with the outer instance.
        std::unordered_map<uint64_t, int> outerCpsMap;
        auto outerForests = rebuildForestsFromNewicks(outerNewicks, outerCpsMap);
        if (std::getenv("DUMP_OUTER")) {
            std::cerr << "# outer trees after extracting placeholder=" << placeholder << ":\n";
            for (const auto& nw : outerNewicks) std::cerr << nw << ";\n";
        }
        forests_ = outerForests;
        leafCount_ = leafCount_ - cc.size + 1;
        cpsMap_ = std::move(outerCpsMap);
        activeForestCount_ = static_cast<int>(forests_.size());
        inactiveForests_.clear();
        failureCache_.clear();
        // NOTE: do NOT call initCpsReduction() here. Cluster reduction must
        // operate on pristine (non-CPS-merged) trees so that countLeaves and
        // newick emission agree on the leaf count of every candidate cluster.
        // The caller (Solver::solve) will run initCpsReduction() once after
        // all cluster extractions complete.
        ++extracted;
    }
    return extracted;
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