#include <solver.h>

#include <algorithm>
#include <cstdint>
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

namespace {
    using LeafMask = std::vector<std::uint64_t>;

    struct LeafMaskHash {
        std::size_t operator()(const LeafMask& mask) const noexcept {
            std::size_t seed = 0;
            for (auto word : mask) {
                seed ^= std::hash<std::uint64_t>{}(word) + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
            }
            return seed;
        }
    };

    using ClusterSet = std::unordered_set<LeafMask, LeafMaskHash>;

    std::size_t labelToIndex(const std::string& label) {
        std::size_t value = 0;
        for (char c : label) {
            value = value * 10 + static_cast<std::size_t>(c - '0');
        }
        return value - 1;
    }

    std::size_t countBits(const LeafMask& mask) {
        std::size_t count = 0;
        for (auto word : mask) {
            count += static_cast<std::size_t>(__builtin_popcountll(word));
        }
        return count;
    }

    std::pair<LeafMask, std::size_t> collectClusterMasks(const std::shared_ptr<TreeNode>& node,
                                                         std::size_t wordCount,
                                                         ClusterSet& clusters) {
        if (node == nullptr) {
            return {LeafMask(wordCount, 0), 0};
        }

        if (node->left == nullptr && node->right == nullptr) {
            LeafMask mask(wordCount, 0);
            auto index = labelToIndex(node->label);
            mask[index / 64] |= (1ULL << (index % 64));
            return {std::move(mask), 1};
        }

        auto [leftMask, leftCount] = collectClusterMasks(node->left, wordCount, clusters);
        auto [rightMask, rightCount] = collectClusterMasks(node->right, wordCount, clusters);

        if (leftMask.size() < rightMask.size()) {
            leftMask.resize(rightMask.size(), 0);
        }
        for (std::size_t idx = 0; idx < rightMask.size(); ++idx) {
            leftMask[idx] |= rightMask[idx];
        }

        auto count = leftCount + rightCount;
        if (count > 1) {
            clusters.insert(leftMask);
        }

        return {std::move(leftMask), count};
    }

    ClusterSet collectForestClusters(const Forest& forest, std::size_t wordCount) {
        ClusterSet clusters;
        const auto& roots = forest.getRoots();
        for (const auto& root : roots) {
            collectClusterMasks(root, wordCount, clusters);
        }
        return clusters;
    }

}

void Solver::printForests() const {
    // TODO: Rework this to work actually good
    // forest1_->print("Forest 1");
    // forest2_->print("Forest 2");
    std::cout << "not implemented, printForests()" << std::endl;
}


void Solver::cleanSingletonLeaves(std::shared_ptr<Forest> mainForest, std::shared_ptr<Forest> otherForest, MutationTrail* trail) {
    for (const auto& root : mainForest->getRoots()) {
        if (root != nullptr && root->isLeaf) {
            otherForest->detachByLabel(root->label, trail);
        }
    }
}

std::vector<std::shared_ptr<Forest>> Solver::cloneForests(std::vector<std::shared_ptr<Forest>> forests) {
    std::vector<std::shared_ptr<Forest>> forestClones = {};
    for (const auto& forest : forests) {
        forestClones.push_back(forest->cloneForest());
    }
    return forestClones;
}

std::shared_ptr<Forest> Solver::solve() {
    return solveWithClusterPartitioning(forests_);
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

std::shared_ptr<Forest> Solver::solveWithoutClusterPartitioning(std::vector<std::shared_ptr<Forest>> forests) {
    if (forests.empty()) {
        return nullptr;
    }

    const auto& firstForest = *forests.front();
    auto currentLeafCount = static_cast<int>(firstForest.getLeaves().size());
    for (int k = 1; k <= currentLeafCount; ++k) {
        std::cout << "# Looking for solution with size k = " << k << std::endl;
        auto result = solve(k, forests);
        if (result.first) {
            return result.second[0];
        }
    }

    return nullptr;
}

bool Solver::findCommonClusterMask(const std::vector<std::shared_ptr<Forest>>& forests,
                                   std::vector<std::uint64_t>& clusterMask) const {
    if (forests.size() < 2) {
        return false;
    }

    auto wordCount = (static_cast<std::size_t>(leafCount_) + 63) / 64;
    LeafMask universeMask(wordCount, 0);
    {
        const auto& firstForest = *forests.front();
        for (const auto& leaf : firstForest.getLeaves()) {
            auto index = labelToIndex(leaf->label);
            universeMask[index / 64] |= (1ULL << (index % 64));
        }
    }

    auto commonClusters = collectForestClusters(*forests.front(), wordCount);
    for (std::size_t forestIndex = 1; forestIndex < forests.size() && !commonClusters.empty(); ++forestIndex) {
        auto forestClusters = collectForestClusters(*forests[forestIndex], wordCount);
        for (auto it = commonClusters.begin(); it != commonClusters.end();) {
            if (forestClusters.find(*it) == forestClusters.end()) {
                it = commonClusters.erase(it);
            } else {
                ++it;
            }
        }
    }

    LeafMask bestMask;
    std::size_t bestSize = 0;
    for (const auto& mask : commonClusters) {
        if (mask == universeMask) {
            continue;
        }
        auto size = countBits(mask);
        if (size > bestSize) {
            bestSize = size;
            bestMask = mask;
        }
    }

    if (bestSize <= 1) {
        return false;
    }

    clusterMask.assign(bestMask.begin(), bestMask.end());
    return true;
}

std::vector<std::shared_ptr<Forest>> Solver::restrictForestsToMask(const std::vector<std::shared_ptr<Forest>>& forests,
                                                                   const std::vector<std::uint64_t>& clusterMask,
                                                                   bool keepSelected) const {
    std::vector<std::shared_ptr<Forest>> restricted;
    restricted.reserve(forests.size());
    for (const auto& forest : forests) {
        restricted.push_back(forest->cloneRestrictedForest(clusterMask, keepSelected));
    }
    return restricted;
}

std::shared_ptr<Forest> Solver::solveWithClusterPartitioning(std::vector<std::shared_ptr<Forest>> forests) {
    if (forests.empty()) {
        return nullptr;
    }

    if (forests.size() < 2) {
        return solveWithoutClusterPartitioning(std::move(forests));
    }

    std::vector<std::uint64_t> clusterMask;
    if (findCommonClusterMask(forests, clusterMask)) {
        constexpr const char* placeholderLabel = "0";

        auto clusterForests = restrictForestsToMask(forests, clusterMask, true);
        std::shared_ptr<TreeNode> clusterRootHandle = nullptr;
        if (!clusterForests.empty() && clusterForests.front() != nullptr) {
            const auto& clusterRoots = static_cast<const Forest&>(*clusterForests.front()).getRoots();
            if (!clusterRoots.empty()) {
                clusterRootHandle = *clusterRoots.begin();
            }
        }

        auto clusterSolution = solveWithoutClusterPartitioning(std::move(clusterForests));
        if (clusterSolution == nullptr || clusterRootHandle == nullptr) {
            return solveWithoutClusterPartitioning(std::move(forests));
        }

        const auto& solvedClusterRoots = static_cast<const Forest&>(*clusterSolution).getRoots();
        if (solvedClusterRoots.size() != 1) {
            return solveWithoutClusterPartitioning(std::move(forests));
        }

        auto graftRoot = *solvedClusterRoots.begin();

        auto reducedForests = std::vector<std::shared_ptr<Forest>>();
        reducedForests.reserve(forests.size());
        for (const auto& forest : forests) {
            reducedForests.push_back(forest->cloneContractedForest(clusterMask, placeholderLabel));
        }

        auto reducedSolution = solveWithoutClusterPartitioning(std::move(reducedForests));
        if (reducedSolution != nullptr && graftRoot != nullptr) {
            reducedSolution->graftForestAtLeaf(placeholderLabel, graftRoot, *clusterSolution);
        } else if (reducedSolution == nullptr) {
            return solveWithoutClusterPartitioning(std::move(forests));
        }
        return reducedSolution;
    }

    return solveWithoutClusterPartitioning(std::move(forests));
}

std::pair<bool, std::vector<std::shared_ptr<Forest>>> Solver::solveRecursive(int k, std::vector<std::shared_ptr<Forest>> forests, MutationTrail& trail) {

    if (forests.size() < 1) {
        return {true, {nullptr}};
    }

    auto f1 = forests[0];

    // Step 1 (if there's only 1 forest left, this forest is the MAF)
    if (forests.size() == 1) {
        return {true, forests};
    }

    auto f2 = forests[1];

    // Step 2 (if there are more components on the MAF we're building than k, the solution is too big (i.e. invalid))
    if (f1->getComponentCount() > k) {
        return {false, {nullptr}};
    }

    // Step 3 (clean the single vertex trees)
    cleanSingletonLeaves(f1, f2, &trail);
    cleanSingletonLeaves(f2, f1, &trail);

    // Step 4 (try to get a sibling pair in F2. If it can't be done, move on to solving F3)
    auto [idx, siblingPair] = f2->getOneSiblingPair();
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

    // Step 5
    auto lab_u = siblingPair.first->label;
    auto lab_v = siblingPair.second->label;
    auto [ancestor, dist] = f1->lca(lab_u, lab_v);

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
    } else if (dist == 1) {
        auto checkpoint = trail.checkpoint();
        auto leaf = f1->getLeafByLabel(lab_u);
        auto otherLeaf = f2->getLeafByLabel(lab_u);
        if (leaf != nullptr && leaf->parent != nullptr && otherLeaf != nullptr && otherLeaf->parent != nullptr) {
            f1->forestMergeCherry(leaf->parent, &trail);
            f2->forestMergeCherry(otherLeaf->parent, &trail);
            auto result = solveRecursive(k, forests, trail);
            if (result.first) {
                return result;
            }
        }
        trail.rollback(checkpoint);
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
        result = solveRecursive(k, forests, trail);
        if (result.first) {
            return result;
        }
        trail.rollback(checkpoint);
    }

    return {false, {nullptr}};
}