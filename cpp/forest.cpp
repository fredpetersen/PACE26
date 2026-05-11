#include <forest.h>

#include <cctype>
#include <iterator>
#include <limits>


/* Hashing and equality functions assume that the TreeNodes will always be given in the same order

Thus they are sensitive to the order of the pair, as (u,v) and (v,u) will be considered different pairs
*/
size_t SiblingPairHash::operator()(const std::pair<TreeNode*, TreeNode*>& p) const noexcept {
        auto a = p.first->label;
        auto b = p.second->label;
        if (a > b) {
            auto c = a;
            a = b;
            b = c;
        }
		std::hash<std::string> hasher;
		auto str = "(" + a + "," + b + ")";
        return hasher(str);
    }

bool SiblingPairEq::operator()(const std::pair<TreeNode*, TreeNode*>& a,
                    const std::pair<TreeNode*, TreeNode*>& b) const noexcept {
        return (a.first == b.first && a.second == b.second) || (a.first == b.second && a.second == b.first);
    }


namespace {
    template <typename Map, typename Key>
    TreeNode* lookupDict(const Map& m, const Key& k) {
        auto it = m.find(k);
        if (it == m.end()) {
            return nullptr;
        }
        return it->second;
    }
}

bool Forest::isSiblingPairNode(TreeNode* node) const {
    return node != nullptr
        && !node->isMerged
        && node->left != nullptr
        && node->right != nullptr
        && node->left->isLeaf
        && node->right->isLeaf;
}

void Forest::updateSiblingPairParent(TreeNode* node, MutationTrail* trail) {
    if (node == nullptr) {
        return;
    }

    bool shouldExist = isSiblingPairNode(node);
    auto key = node;
    auto it = siblingPairParents_.find(key);
    bool exists = it != siblingPairParents_.end();

    if (shouldExist && !exists) {
        siblingPairParents_.insert(key);
        if (trail != nullptr) {
            UndoEntry e{};
            e.op = UndoOp::ForestSiblingErase;
            e.forest = this;
            e.t_aux = key;
            trail->record(std::move(e));
        }
    } else if (!shouldExist && exists) {
        siblingPairParents_.erase(it);
        if (trail != nullptr) {
            UndoEntry e{};
            e.op = UndoOp::ForestSiblingInsert;
            e.forest = this;
            e.t_aux = key;
            trail->record(std::move(e));
        }
    }
}

void Forest::rebuildSiblingPairCache() {
    siblingPairParents_.clear();

    std::unordered_map<TreeNode*, std::vector<TreeNode*>> parentToLeaves;
    for (const auto& leaf : leaves_) {
        if (leaf != nullptr && leaf->parent != nullptr) {
            parentToLeaves[leaf->parent].push_back(leaf);
        }
    }
    for (const auto& [parent, leaves] : parentToLeaves) {
        if (leaves.size() == 2 && parent != nullptr && !parent->isMerged && parent->left && parent->right && parent->left->isLeaf && parent->right->isLeaf) {
            siblingPairParents_.insert(parent);
        }
    }
}


void Forest::forestLocalMergeCherry(TreeNode* node, MutationTrail* trail) {
    if (node == nullptr) {
        return;
    }
    auto l = node->left;
    auto r = node->right;
    if (l == nullptr || r == nullptr || !l->isLeaf || !r->isLeaf) {
        return;
    }

    removeLeaf(l, trail);
    removeLeaf(r, trail);

    localMergeCherry(node, trail);

    addLeaf(node, trail);
    updateSiblingPairParent(node, trail);
    updateSiblingPairParent(node->parent, trail);
}

void Forest::expandMergedSubtrees(MutationTrail* trail) {
    std::vector<TreeNode*> mergedLeaves;
    mergedLeaves.reserve(leaves_.size());
    for (const auto& leaf : leaves_) {
        if (leaf->isMerged) {
            mergedLeaves.push_back(leaf);
        }
    }
    for (const auto& leaf : mergedLeaves) {
        expandRecursive(leaf, trail);
    }
}


void Forest::expandRecursive(TreeNode* node, MutationTrail* trail) {
    if (node == nullptr || !node->isMerged) {
        return;
    }

    auto oldLabel = node->label;
    auto oldIsLeaf = node->isLeaf;
    auto oldIsMerged = node->isMerged;

    removeLeaf(node, trail);

    node->isMerged = false;
    node->isLeaf = false;
    node->label = "0";
    invalidateSubtreeHash(node);
    if (trail != nullptr) {
        UndoEntry e{};
        e.op = UndoOp::NodeLabelFlags;
        e.a = node;
        e.str_aux = std::move(oldLabel);
        e.bool_a = oldIsLeaf;
        e.bool_b = oldIsMerged;
        trail->record(std::move(e));
    }

    updateSiblingPairParent(node->parent, trail);

    auto l = node->left;
    auto r = node->right;

    if (l != nullptr) {
        if (!l->isMerged) {
            addLeaf(l, trail);
        } else {
            expandRecursive(l, trail);
        }
    }
    if (r != nullptr) {
        if (!r->isMerged) {
            addLeaf(r, trail);
        } else {
            expandRecursive(r, trail);
        }
    }

    updateSiblingPairParent(node, trail);
}

uint64_t Forest::detachChild(TreeNode* child, std::unordered_map<uint64_t, int>& cpsMap, bool shouldContract, MutationTrail* trail) {
    if (child == nullptr) {
        return 0;
    }
    auto parent = child->parent;
    if (parent != nullptr) {
        bool isLeftChild = parent->left == child;
        bool isRightChild = parent->right == child;
        if (!isLeftChild && !isRightChild) {
            std::cout << "# The given node does not have that child" << std::endl;
            return 0;
        }

        auto inserted = roots_.insert(child).second;
        if (trail != nullptr && inserted) {
            UndoEntry e{};
            e.op = UndoOp::ForestRootsErase;
            e.forest = this;
            e.t_aux = child;
            trail->record(std::move(e));
        }

        auto previousParent = child->parent;
        child->parent = nullptr;
        if (trail != nullptr) {
            UndoEntry e{};
            e.op = UndoOp::NodeParent;
            e.a = child;
            e.node_aux = previousParent;
            trail->record(std::move(e));
        }

        if (isLeftChild) {
            auto previousLeft = parent->left;
            parent->left = nullptr;
            invalidateSubtreeHash(parent);
            if (trail != nullptr) {
                UndoEntry e{};
                e.op = UndoOp::NodeLeftSlot;
                e.a = parent;
                e.t_aux = previousLeft;
                trail->record(std::move(e));
            }
        } else {
            auto previousRight = parent->right;
            parent->right = nullptr;
            invalidateSubtreeHash(parent);
            if (trail != nullptr) {
                UndoEntry e{};
                e.op = UndoOp::NodeRightSlot;
                e.a = parent;
                e.t_aux = previousRight;
                trail->record(std::move(e));
            }
        }

        updateSiblingPairParent(parent, trail);

        ++componentCount_;
        if (trail != nullptr) {
            UndoEntry e{};
            e.op = UndoOp::ForestComponentDelta;
            e.forest = this;
            e.int_aux = -1;
            trail->record(std::move(e));
        }

        if (shouldContract) {
            contract(parent, trail);
        } else if (parent->parent == nullptr && parent->left == nullptr && parent->right == nullptr) {
            contract(parent, trail);
        }
        return cpsReduction(parent, cpsMap, trail);
    }
    return 0;
}

uint64_t Forest::detachByLabel(const std::string& label, std::unordered_map<uint64_t, int>& cpsMap, MutationTrail* trail) {
    auto leaf = getLeafByLabel(label);
    if (leaf == nullptr) {
        return 0;
    }
    return detachChild(leaf, cpsMap, true, trail);
}

void Forest::contract(TreeNode* v, MutationTrail* trail) {
    if (v == nullptr) return;

    bool hasLeftChild = v->left != nullptr;
    bool hasRightChild = v->right != nullptr;
    if (hasLeftChild && hasRightChild) return;
    if (v->isLeaf) return;

    auto child = hasLeftChild ? v->left : v->right;

    if (child == nullptr) {
        auto parent = v->parent;
        if (parent != nullptr) {
            bool isRightChild = parent->right == v;
            if (isRightChild) {
                auto previousRight = parent->right;
                parent->right = nullptr;
                invalidateSubtreeHash(parent);
                if (trail != nullptr) {
                    UndoEntry e{};
                    e.op = UndoOp::NodeRightSlot;
                    e.a = parent;
                    e.t_aux = previousRight;
                    trail->record(std::move(e));
                }
            } else {
                auto previousLeft = parent->left;
                parent->left = nullptr;
                invalidateSubtreeHash(parent);
                if (trail != nullptr) {
                    UndoEntry e{};
                    e.op = UndoOp::NodeLeftSlot;
                    e.a = parent;
                    e.t_aux = previousLeft;
                    trail->record(std::move(e));
                }
            }

            auto previousParent = v->parent;
            v->parent = nullptr;
            if (trail != nullptr) {
                UndoEntry e{};
                e.op = UndoOp::NodeParent;
                e.a = v;
                e.node_aux = previousParent;
                trail->record(std::move(e));
            }

            updateSiblingPairParent(parent, trail);
            contract(parent, trail);
        } else {
            removeRoot(v, trail);
            --componentCount_;
            if (trail != nullptr) {
                UndoEntry e{};
                e.op = UndoOp::ForestComponentDelta;
                e.forest = this;
                e.int_aux = 1;
                trail->record(std::move(e));
            }
        }
        return;
    }

    if (v->parent != nullptr) {
        auto parent = v->parent;
        bool isRightChild = parent->right == v;

        auto previousChildParent = child->parent;
        child->parent = parent;
        if (trail != nullptr) {
            UndoEntry e{};
            e.op = UndoOp::NodeParent;
            e.a = child;
            e.node_aux = previousChildParent;
            trail->record(std::move(e));
        }

        if (isRightChild) {
            auto previousRight = parent->right;
            parent->right = child;
            invalidateSubtreeHash(parent);
            if (trail != nullptr) {
                UndoEntry e{};
                e.op = UndoOp::NodeRightSlot;
                e.a = parent;
                e.t_aux = previousRight;
                trail->record(std::move(e));
            }
        } else {
            auto previousLeft = parent->left;
            parent->left = child;
            invalidateSubtreeHash(parent);
            if (trail != nullptr) {
                UndoEntry e{};
                e.op = UndoOp::NodeLeftSlot;
                e.a = parent;
                e.t_aux = previousLeft;
                trail->record(std::move(e));
            }
        }

        updateSiblingPairParent(parent, trail);

    } else {
        auto previousChildParent = child->parent;
        child->parent = nullptr;
        if (trail != nullptr) {
            UndoEntry e{};
            e.op = UndoOp::NodeParent;
            e.a = child;
            e.node_aux = previousChildParent;
            trail->record(std::move(e));
        }

        removeRoot(v, trail);
        addRoot(child, trail);
    }
}

void Forest::contractIntoCherry(const std::string& lab_u, const std::string& lab_v, TreeNode* ancestor, MutationTrail* trail) {
    auto u = getLeafByLabel(lab_u);
    auto v = getLeafByLabel(lab_v);
    if (u != nullptr && v != nullptr && ancestor != nullptr) {
        auto current = u;
        TreeNode* next = nullptr;
        while (current != ancestor && current->parent != nullptr) {
            next = current->parent;
            contract(current, trail);
            current = next;
        }
        current = v;
        while (current != ancestor && current->parent != nullptr) {
            next = current->parent;
            contract(current, trail);
            current = next;
        }
    }
}

std::pair<TreeNode*, int> Forest::lca(const std::string& label_u, const std::string& label_v) {
    auto u = getLeafByLabel(label_u);
    auto v = getLeafByLabel(label_v);
	if (u == nullptr || v == nullptr) {
		return {nullptr, -1};
	}
	if(label_u == label_v) {
        return {u, 0};
    }
    std::unordered_set<TreeNode*> parentSet{u};

    auto uTmp = u;
    auto vTmp = v;

    while (uTmp->parent != nullptr) {
        parentSet.insert(uTmp->parent);
        uTmp = uTmp->parent;
    }

    int dist = 0;
    TreeNode* lca = nullptr;

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
        while (uTmp->parent != lca) {
            dist++;
            uTmp = uTmp->parent;
        }
        return {lca, dist};
    } else {
        return {nullptr, -1};
    }
}

std::vector<TreeNode*> Forest::collectPendantSubtreesBetweenLeaves(const std::string& u_label,
                                                    const std::string& v_label,
                                                    TreeNode* lcaNode) {
        std::vector<TreeNode*> pendantSubtrees;
        auto u = getLeafByLabel(u_label);
        auto v = getLeafByLabel(v_label);

        if (u == nullptr || v == nullptr || lcaNode == nullptr) {
            return pendantSubtrees;
        }

        std::unordered_set<TreeNode*> seen;
        auto collectFromLeaf = [&](TreeNode* leaf) {
            auto current = leaf;
            while (current != nullptr && current->parent != nullptr && current->parent != lcaNode) {
                auto parent = current->parent;
                auto sibling = (parent->left == current) ? parent->right : parent->left;
                if (sibling != nullptr && seen.insert(sibling).second) {
                    pendantSubtrees.push_back(sibling);
                }
                current = parent;
            }
        };

        collectFromLeaf(u);
        collectFromLeaf(v);
        return pendantSubtrees;
    };

std::uint64_t Forest::cpsReduction(TreeNode* node, std::unordered_map<uint64_t, int>& cpsMap, MutationTrail* trail) {
    if (node != nullptr) {
        if (node->isCpsNode()) {
            auto l = node->left;
            auto r = node->right;

            globalMergeCherry(node, trail);

            leaves_.erase(l);
            leaves_.erase(r);
            leaves_.insert(node);

            leafByLabel_[node->label] = node;
            leafByLabel_.erase(l->label);
            leafByLabel_.erase(r->label);

            nodeByCps_.erase(node->cpsHash);

            updateSiblingPairParent(node, trail);
            updateSiblingPairParent(node->parent, trail);

            if (trail != nullptr) {
                UndoEntry e{};
                e.op = UndoOp::CpsReductionRestore;
                e.forest = this;
                e.t_aux  = l;
                e.t_aux2 = r;
                e.t_aux3 = node;
                trail->record(std::move(e));
            }
            if (node->parent != nullptr) {
                auto parent = node->parent;
                if (parent->isCpsNode()) {

                    parent->setCps();
                    auto h = parent->cpsHash;

                    nodeByCps_[h] = parent;
                    if (cpsMap.find(parent->cpsHash) == cpsMap.end()) {
                        cpsMap[h] = 1;
                    } else {
                        cpsMap[h] += 1;
                    }
                    if (trail != nullptr) {
                        trail->record([this, parent, h, &cpsMap]() {
                            parent->cpsHash = 0;
                            nodeByCps_.erase(parent->cpsHash);

                            cpsMap[h] -= 1;
                        });
                    }
                    return h;
                }
            }
        }
    }
    return 0;
}

std::string Forest::treeToNewick(const TreeNode* node) {
	if (!node) return "";
	if (node->isLeaf) return node->label;

	std::string result = "(";
	if (node->left)  result += treeToNewick(node->left);
	if (node->left && node->right) result += ",";
	if (node->right) result += treeToNewick(node->right);
	result += ")";
	if (node->label != "0") result += node->label;
	return result;
}

void Forest::print(const std::string& name) const {
	for (const auto& root : roots_) {
		this->printTree(root, name);
	}
}

void Forest::printTree(const TreeNode* node, const std::string& name) const {
	std::cout << name << std::endl;
	if (node == nullptr) {
		std::cout << "└── (null)" << std::endl;
		return;
	}

	std::cout << "└── " << (node->isLeaf ? "Leaf(" : "Node(") << node->label << ")" << std::endl;
	if (node->isLeaf) {
		return;
	}

	this->printTreeRecursive(node->left, "    ", true);
	this->printTreeRecursive(node->right, "    ", false);
}

void Forest::printTreeRecursive(const TreeNode* node, const std::string& prefix, bool isLeft) const {
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
	this->printTreeRecursive(node->left, childPrefix, true);
	this->printTreeRecursive(node->right, childPrefix, false);
}

void Forest::printForestNewick() {
	for (const auto& root : roots_) {
			std::cout << treeToNewick(root) << ";\n";
	}
}

void Forest::printCps() const {
    for (const auto& kv : nodeByCps_) {
        std::cout << "# " << kv.first << " -> " << kv.second << std::endl;
    }
}

void Forest::setComponentCount(int newCount, MutationTrail* trail) {
    if (trail != nullptr) {
        UndoEntry e{};
        e.op = UndoOp::ForestComponentSet;
        e.forest = this;
        e.int_aux = componentCount_;
        trail->record(std::move(e));
    }
    componentCount_ = newCount;
}

int Forest::getComponentCount() {
	return componentCount_;
}

void Forest::addRoot(TreeNode* node, MutationTrail* trail) {
    if (node == nullptr) {
        return;
    }
    bool inserted = roots_.insert(node).second;
    if (trail != nullptr && inserted) {
        UndoEntry e{};
        e.op = UndoOp::ForestRootsErase;
        e.forest = this;
        e.t_aux = node;
        trail->record(std::move(e));
    }
}

void Forest::addLeaf(TreeNode* node, MutationTrail* trail) {
    if (node == nullptr) {
        return;
    }
    auto label = node->label;
    auto previousIt = leafByLabel_.find(label);
    auto previousNode = previousIt == leafByLabel_.end() ? nullptr : previousIt->second;
    bool hadPrevious = previousIt != leafByLabel_.end();

    bool inserted = leaves_.insert(node).second;
    leafByLabel_[label] = node;

    if (trail != nullptr && inserted) {
        if (hadPrevious) {
            UndoEntry restore{};
            restore.op = UndoOp::ForestLeafByLabelSet;
            restore.forest = this;
            restore.str_aux = label;
            restore.t_aux = previousNode;
            trail->record(std::move(restore));
        } else {
            UndoEntry restore{};
            restore.op = UndoOp::ForestLeafByLabelErase;
            restore.forest = this;
            restore.str_aux = label;
            trail->record(std::move(restore));
        }
        UndoEntry e{};
        e.op = UndoOp::ForestLeavesErase;
        e.forest = this;
        e.t_aux = node;
        trail->record(std::move(e));
    }

    updateSiblingPairParent(node->parent, trail);
}

void Forest::removeRoot(TreeNode* node, MutationTrail* trail) {
    if (node == nullptr) {
        return;
    }
    for (auto it = roots_.begin(); it != roots_.end(); ++it) {
        if (*it == node) {
            auto previousRoot = *it;
            roots_.erase(it);
            if (trail != nullptr) {
                UndoEntry e{};
                e.op = UndoOp::ForestRootsInsert;
                e.forest = this;
                e.t_aux = previousRoot;
                trail->record(std::move(e));
            }
            return;
        }
    }
}

void Forest::removeLeaf(TreeNode* node, MutationTrail* trail) {
    if (node == nullptr) {
        return;
    }
    auto label = node->label;
    auto previousIt = leafByLabel_.find(label);
    auto previousNode = previousIt == leafByLabel_.end() ? nullptr : previousIt->second;
    bool hadMapping = previousIt != leafByLabel_.end() && previousIt->second == node;

    auto erased = leaves_.erase(node);
    if (hadMapping) {
        leafByLabel_.erase(previousIt);
    }

    if (trail != nullptr && erased > 0) {
        if (hadMapping) {
            UndoEntry restore{};
            restore.op = UndoOp::ForestLeafByLabelSet;
            restore.forest = this;
            restore.str_aux = label;
            restore.t_aux = previousNode;
            trail->record(std::move(restore));
        }
        UndoEntry e{};
        e.op = UndoOp::ForestLeavesInsert;
        e.forest = this;
        e.t_aux = node;
        trail->record(std::move(e));
    }

    updateSiblingPairParent(node->parent, trail);
}

const std::unordered_set<TreeNode*>& Forest::getRoots() const {
	return roots_;
}
const std::unordered_set<TreeNode*>& Forest::getLeaves() const {
	return leaves_;
}
TreeNode* Forest::getLeafByLabel(const std::string& label) const {
    return lookupDict(leafByLabel_, label);
}

TreeNode* Forest::getNodeByCps(uint64_t hash) const {
    return lookupDict(nodeByCps_, hash);
}

std::unordered_set<
    Forest::SiblingPair,
    SiblingPairHash,SiblingPairEq
> Forest::getSiblingLeafPairs() {
    SiblingPairSet siblingLeafPairs;
    for (const auto parent : siblingPairParents_) {
        if (parent != nullptr && parent->left != nullptr && parent->right != nullptr) {
            siblingLeafPairs.insert({parent->left, parent->right});
        }
    }
    return siblingLeafPairs;
}

std::pair<int, Forest::SiblingPair> Forest::getOneSiblingPair(int startIndex) {
    if (siblingPairParents_.empty()) {
        return {-1, {nullptr, nullptr}};
    }

    for (auto it = siblingPairParents_.begin(); it != siblingPairParents_.end(); ++it) {
        auto parent = *it;
        if (parent != nullptr && parent->left != nullptr && parent->right != nullptr) {
            return {0, {parent->left, parent->right}};
        }
    }

    return {-1, {nullptr, nullptr}};
}

std::pair<int, Forest::SiblingPair> Forest::getOneSiblingPair(const std::vector<int>& priority) {
    if (priority.empty()) {
        return getOneSiblingPair();
    }
    if (siblingPairParents_.empty()) {
        return {-1, {nullptr, nullptr}};
    }

    auto labelPriority = [&](const std::string& lab) -> int {
        // Fast path: pure numeric leaf id. Anything else (e.g. merged-cherry
        // composite labels like "(12,34)") falls back to INT_MAX.
        if (lab.empty() || !std::isdigit(static_cast<unsigned char>(lab.front()))) {
            return std::numeric_limits<int>::max();
        }
        int v = 0;
        for (char c : lab) {
            if (!std::isdigit(static_cast<unsigned char>(c))) {
                return std::numeric_limits<int>::max();
            }
            v = v * 10 + (c - '0');
            if (v >= static_cast<int>(priority.size())) {
                return std::numeric_limits<int>::max();
            }
        }
        return priority[v];
    };

    int bestScore = std::numeric_limits<int>::max();
    SiblingPair bestPair{nullptr, nullptr};
    bool found = false;
    for (auto* parent : siblingPairParents_) {
        if (parent == nullptr || parent->left == nullptr || parent->right == nullptr) continue;
        int pl = labelPriority(parent->left->label);
        int pr = labelPriority(parent->right->label);
        int score = (pl > pr) ? pl : pr; // worst of the two = how "peripheral" the pair is
        if (!found || score < bestScore) {
            bestScore = score;
            bestPair = {parent->left, parent->right};
            found = true;
            if (score == 0) break; // can't do better than both at the TD root
        }
    }
    if (!found) return {-1, {nullptr, nullptr}};
    return {0, bestPair};
}

void Forest::setRoots(std::unordered_set<TreeNode*> newRoots) {
	roots_ = std::move(newRoots);
    rebuildSiblingPairCache();
}

void Forest::setLeaves(std::unordered_set<TreeNode*> newLeaves) {
	leaves_ = std::move(newLeaves);
    rebuildSiblingPairCache();
}

void Forest::setLeavesByLabel(std::unordered_map<std::string, TreeNode*> newLeavesByLabel) {
	leafByLabel_ = std::move(newLeavesByLabel);
    rebuildSiblingPairCache();
}

void Forest::setNodesByCps(std::unordered_map<uint64_t, TreeNode*> newNodesByCps) {
	nodeByCps_ = std::move(newNodesByCps);
}
