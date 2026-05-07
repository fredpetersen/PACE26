#include <forest.h>

#include <iterator>


/* Hashing and equality functions assume that the TreeNodes will always be given in the same order

Thus they are sensitive to the order of the pair, as (u,v) and (v,u) will be considered different pairs
*/
size_t SiblingPairHash::operator()(const std::pair<std::shared_ptr<TreeNode>, std::shared_ptr<TreeNode>>& p) const noexcept {
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

//TODO: Check string equality actually works
bool SiblingPairEq::operator()(const std::pair<std::shared_ptr<TreeNode>, std::shared_ptr<TreeNode>>& a,
                    const std::pair<std::shared_ptr<TreeNode>, std::shared_ptr<TreeNode>>& b) const noexcept {
        return (a.first == b.first && a.second == b.second) || (a.first == b.second && a.second == b.first);
    }


namespace {
    std::shared_ptr<TreeNode> lookupDict(const std::unordered_map<std::string, std::shared_ptr<TreeNode>>& leafByLabel,
                                         const std::string& label) {
        auto it = leafByLabel.find(label);
        if (it == leafByLabel.end()) {
            return nullptr;
        }
        return it->second;
    }

}

bool Forest::isSiblingPairNode(std::shared_ptr<TreeNode> node) const {
    return node != nullptr
        && !node->isMerged
        && node->left != nullptr
        && node->right != nullptr
        && node->left->isLeaf
        && node->right->isLeaf;
}

void Forest::updateSiblingPairParent(std::shared_ptr<TreeNode> node, MutationTrail* trail) {
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
            e.shared_aux = key;
            trail->record(std::move(e));
        }
    } else if (!shouldExist && exists) {
        siblingPairParents_.erase(it);
        if (trail != nullptr) {
            UndoEntry e{};
            e.op = UndoOp::ForestSiblingInsert;
            e.forest = this;
            e.shared_aux = key;
            trail->record(std::move(e));
        }
    }
}

void Forest::rebuildSiblingPairCache() {
    siblingPairParents_.clear();

    std::unordered_map<std::shared_ptr<TreeNode>, std::vector<std::shared_ptr<TreeNode>>> parentToLeaves;
    for (const auto& leaf : leaves_) {
        if (leaf != nullptr && leaf->parent != nullptr) {
            parentToLeaves[leaf->parentShared()].push_back(leaf);
        }
    }
    for (const auto& [parent, leaves] : parentToLeaves) {
        if (leaves.size() == 2 && parent != nullptr && !parent->isMerged && parent->left && parent->right && parent->left->isLeaf && parent->right->isLeaf) {
            siblingPairParents_.insert(parent);
        }
    }
}


void Forest::forestLocalMergeCherry(std::shared_ptr<TreeNode> node, MutationTrail* trail) {
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
    updateSiblingPairParent(node->parentShared(), trail);
}

void Forest::expandMergedSubtrees(MutationTrail* trail) {
    std::vector<std::shared_ptr<TreeNode>> mergedLeaves;
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


void Forest::expandRecursive(std::shared_ptr<TreeNode> node, MutationTrail* trail) {
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
    if (trail != nullptr) {
        UndoEntry e{};
        e.op = UndoOp::NodeLabelFlags;
        e.a = node.get();
        e.str_aux = std::move(oldLabel);
        e.bool_a = oldIsLeaf;
        e.bool_b = oldIsMerged;
        trail->record(std::move(e));
    }

    updateSiblingPairParent(node->parentShared(), trail);

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

std::string Forest::detachChild(std::shared_ptr<TreeNode> child, std::unordered_map<std::string, int>& cpsMap, bool shouldContract, MutationTrail* trail) {
    // std::cout << "# DC" << std::endl;
    if (child == nullptr) {
        return "";
    }
    auto parent = child->parentShared();
    if (parent != nullptr) {
        bool isLeftChild = parent->left == child;
        bool isRightChild = parent->right == child;
        if (!isLeftChild && !isRightChild) {
            std::cout << "# The given node does not have that child" << std::endl;
            return "";
        }

        auto inserted = roots_.insert(child).second;
        if (trail != nullptr && inserted) {
            UndoEntry e{};
            e.op = UndoOp::ForestRootsErase;
            e.forest = this;
            e.shared_aux = child;
            trail->record(std::move(e));
        }

        auto previousParent = child->parent;
        child->parent = nullptr;
        if (trail != nullptr) {
            UndoEntry e{};
            e.op = UndoOp::NodeParent;
            e.a = child.get();
            e.node_aux = previousParent;
            trail->record(std::move(e));
        }

        if (isLeftChild) {
            auto previousLeft = parent->left;
            parent->left = nullptr;
            if (trail != nullptr) {
                UndoEntry e{};
                e.op = UndoOp::NodeLeftSlot;
                e.a = parent.get();
                e.shared_aux = std::move(previousLeft);
                trail->record(std::move(e));
            }
        } else {
            auto previousRight = parent->right;
            parent->right = nullptr;
            if (trail != nullptr) {
                UndoEntry e{};
                e.op = UndoOp::NodeRightSlot;
                e.a = parent.get();
                e.shared_aux = std::move(previousRight);
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
    return "";
}

std::string Forest::detachByLabel(const std::string& label, std::unordered_map<std::string, int>& cpsMap, MutationTrail* trail) {
    // std::cout << "# DBL" << std::endl;
    // std::cout << label << std::endl;
    auto leaf = getLeafByLabel(label);
    // std::cout << leaf << std::endl;
    if (leaf == nullptr) {
        return "";
    }
    return detachChild(leaf, cpsMap, true, trail);
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
void Forest::contract(std::shared_ptr<TreeNode> v, MutationTrail* trail) {
    if (v == nullptr) return;

    bool hasLeftChild = v->left != nullptr;
    bool hasRightChild = v->right != nullptr;
    if (hasLeftChild && hasRightChild) return; // Both children are present; can't contract
    if (v->isLeaf) return; // can't contract leaf nodes

    auto child = hasLeftChild ? v->left : v->right; // The only child of v

    if (child == nullptr) {
        auto parent = v->parentShared();
        if (parent != nullptr) {
            bool isRightChild = parent->right == v;
            if (isRightChild) {
                auto previousRight = parent->right;
                parent->right = nullptr;
                if (trail != nullptr) {
                    UndoEntry e{};
                    e.op = UndoOp::NodeRightSlot;
                    e.a = parent.get();
                    e.shared_aux = std::move(previousRight);
                    trail->record(std::move(e));
                }
            } else {
                auto previousLeft = parent->left;
                parent->left = nullptr;
                if (trail != nullptr) {
                    UndoEntry e{};
                    e.op = UndoOp::NodeLeftSlot;
                    e.a = parent.get();
                    e.shared_aux = std::move(previousLeft);
                    trail->record(std::move(e));
                }
            }

            auto previousParent = v->parent;
            v->parent = nullptr;
            if (trail != nullptr) {
                UndoEntry e{};
                e.op = UndoOp::NodeParent;
                e.a = v.get();
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

    if (v->parent != nullptr) { // v is not root node
        auto parent = v->parentShared();
        bool isRightChild = parent->right == v;

        auto previousChildParent = child->parent;
        child->parent = parent.get();
        if (trail != nullptr) {
            UndoEntry e{};
            e.op = UndoOp::NodeParent;
            e.a = child.get();
            e.node_aux = previousChildParent;
            trail->record(std::move(e));
        }

        if (isRightChild) {
            auto previousRight = parent->right;
            parent->right = child;
            if (trail != nullptr) {
                UndoEntry e{};
                e.op = UndoOp::NodeRightSlot;
                e.a = parent.get();
                e.shared_aux = std::move(previousRight);
                trail->record(std::move(e));
            }
        } else {
            auto previousLeft = parent->left;
            parent->left = child;
            if (trail != nullptr) {
                UndoEntry e{};
                e.op = UndoOp::NodeLeftSlot;
                e.a = parent.get();
                e.shared_aux = std::move(previousLeft);
                trail->record(std::move(e));
            }
        }

        updateSiblingPairParent(parent, trail);

    } else { // v is root node
        auto previousChildParent = child->parent;
        child->parent = nullptr;
        if (trail != nullptr) {
            UndoEntry e{};
            e.op = UndoOp::NodeParent;
            e.a = child.get();
            e.node_aux = previousChildParent;
            trail->record(std::move(e));
        }

        removeRoot(v, trail);
        addRoot(child, trail);
    }
}

void Forest::contractIntoCherry(const std::string& lab_u, const std::string& lab_v, std::shared_ptr<TreeNode> ancestor, MutationTrail* trail) {
    auto u = getLeafByLabel(lab_u);
    auto v = getLeafByLabel(lab_v);
    if (u != nullptr && v != nullptr && ancestor != nullptr) {
        auto current = u;
        std::shared_ptr<TreeNode> next;
        while (current != ancestor && current->parent != nullptr) {
            next = current->parentShared();
            contract(current, trail);
            current = next;
        }
        current = v;
        while (current != ancestor && current->parent != nullptr) {
            next = current->parentShared();
            contract(current, trail);
            current = next;
        }
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
std::pair<std::shared_ptr<TreeNode>, int> Forest::lca(const std::string& label_u, const std::string& label_v) {
    auto u = getLeafByLabel(label_u);
    auto v = getLeafByLabel(label_v);
	if (u == nullptr || v == nullptr) {
		return {nullptr, -1};
	}
	if(label_u == label_v) {
        return {u, 0};
    }
    // Takes into account if u is a parent of v
    std::unordered_set<std::shared_ptr<TreeNode>> parentSet{u};

    auto uTmp = u;
    auto vTmp = v;

    // u->parent == nullptr means that u->parent is the root node
    while (uTmp->parentShared() != nullptr) {
        parentSet.insert(uTmp->parentShared());
        uTmp = uTmp->parentShared();
    }

    int dist = 0;
    std::shared_ptr<TreeNode> lca = nullptr;

    // Takes into account if V is an ancestor of U
    if (parentSet.count(v) > 0) {
        lca = v;
    } else {
        while (vTmp->parent != nullptr) {
            dist++;
            if (parentSet.count(vTmp->parentShared()) > 0) {
                lca = vTmp->parentShared();
                break;
            }
            vTmp = vTmp->parentShared();
        }
    }

    if (lca == u) {
        return {lca, dist};
    } else if (lca != nullptr) {
        uTmp = u;
        // Calculates the distance from u to the lca to get the final distance (or root)
        while (uTmp->parentShared() != lca) {
            dist++;
            uTmp = uTmp->parentShared();
        }
        return {lca, dist};
    } else {
        return {nullptr, -1};
    }
}

std::vector<std::shared_ptr<TreeNode>> Forest::collectPendantSubtreesBetweenLeaves(const std::string& u_label,
                                                    const std::string& v_label,
                                                    std::shared_ptr<TreeNode> lcaNode) {
        std::vector<std::shared_ptr<TreeNode>> pendantSubtrees;
        auto u = getLeafByLabel(u_label);
        auto v = getLeafByLabel(v_label);

        if (u == nullptr || v == nullptr || lcaNode == nullptr) {
            return pendantSubtrees;
        }

        std::unordered_set<TreeNode*> seen;
        auto collectFromLeaf = [&](std::shared_ptr<TreeNode> leaf) {
            auto current = leaf;
            while (current != nullptr && current->parent != nullptr && current->parentShared() != lcaNode) {
                auto parent = current->parentShared();
                auto sibling = (parent->left == current) ? parent->right : parent->left;
                if (sibling != nullptr && seen.insert(sibling.get()).second) {
                    pendantSubtrees.push_back(sibling);
                }
                current = parent;
            }
        };

        collectFromLeaf(u);
        collectFromLeaf(v);
        return pendantSubtrees;
    };

// This can be called on nodes that aren't common! Make sure this doesn't happen
std::string Forest::cpsReduction(std::shared_ptr<TreeNode> node, std::unordered_map<std::string, int>& cpsMap, MutationTrail* trail) {
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

            nodeByCps_.erase(node->label);

            updateSiblingPairParent(node, trail);
            updateSiblingPairParent(node->parentShared(), trail);

            if (trail != nullptr) {
                UndoEntry e{};
                e.op = UndoOp::CpsReductionRestore;
                e.forest = this;
                e.shared_aux  = l;
                e.shared_aux2 = r;
                e.shared_aux3 = node;
                trail->record(std::move(e));
            }
            if (node->parent != nullptr) {
                auto parent = node->parentShared();
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
                            parent->cpsHash = "";
                            nodeByCps_.erase(parent->cpsHash);

                            cpsMap[h] -= 1;
                        });
                    }
                    // debug("returning " + h);
                    return h;
                }
            }
        }
    }
    return "";
}

std::string Forest::treeToNewick(const std::shared_ptr<TreeNode>& node) {
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

void Forest::printTree(const std::shared_ptr<TreeNode> node, const std::string& name) const {
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

void Forest::printTreeRecursive(const std::shared_ptr<TreeNode> node, const std::string& prefix, bool isLeft) const {
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

void Forest::addRoot(std::shared_ptr<TreeNode> node, MutationTrail* trail) {
    if (node == nullptr) {
        return;
    }
    bool inserted = roots_.insert(node).second;
    if (trail != nullptr && inserted) {
        UndoEntry e{};
        e.op = UndoOp::ForestRootsErase;
        e.forest = this;
        e.shared_aux = node;
        trail->record(std::move(e));
    }
}

void Forest::addLeaf(std::shared_ptr<TreeNode> node, MutationTrail* trail) {
    if (node == nullptr) {
        return;
    }
    // Tries to make node a leaf. If another leaf with this label already exists, then overwrite the leaf with this one
    auto label = node->label;
    auto previousIt = leafByLabel_.find(label);
    auto previousNode = previousIt == leafByLabel_.end() ? nullptr : previousIt->second;
    bool hadPrevious = previousIt != leafByLabel_.end();

    bool inserted = leaves_.insert(node).second;
    leafByLabel_[label] = node;

    if (trail != nullptr && inserted) {
        // Push the leafByLabel restore first so leaves_.erase pops first on undo.
        if (hadPrevious) {
            UndoEntry restore{};
            restore.op = UndoOp::ForestLeafByLabelSet;
            restore.forest = this;
            restore.str_aux = label;
            restore.shared_aux = previousNode;
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
        e.shared_aux = node;
        trail->record(std::move(e));
    }

    updateSiblingPairParent(node->parentShared(), trail);
}

void Forest::removeRoot(std::shared_ptr<TreeNode> node, MutationTrail* trail) {
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
                e.shared_aux = previousRoot;
                trail->record(std::move(e));
            }
            return;
        }
    }
}

void Forest::removeLeaf(std::shared_ptr<TreeNode> node, MutationTrail* trail) {
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
            restore.shared_aux = previousNode;
            trail->record(std::move(restore));
        }
        UndoEntry e{};
        e.op = UndoOp::ForestLeavesInsert;
        e.forest = this;
        e.shared_aux = node;
        trail->record(std::move(e));
    }

    updateSiblingPairParent(node->parentShared(), trail);
}

std::unordered_set<std::shared_ptr<TreeNode>> Forest::getRoots() {
	return roots_;
}
std::unordered_set<std::shared_ptr<TreeNode>> Forest::getLeaves() {
	return leaves_;
}
std::shared_ptr<TreeNode> Forest::getLeafByLabel(const std::string& label) const {
    return lookupDict(leafByLabel_, label);
}

std::shared_ptr<TreeNode> Forest::getNodeByCps(const std::string& label) const {
    return lookupDict(nodeByCps_, label);
}

/**
    * Returns a set of all sibling leaf pairs in the forest.
    *
    * This currently runs in O(n) time, but could be optimized to O(s)
    * where s is the number of sibling pairs, by keeping track of
    * sibling pairs in the forest data structure and updating that set
    * as merges and contractions happen.
    */
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

/**
 * Takes in an optional startIndex parameter to allow for iterating through sibling pairs one at a time
 * across multiple calls, which can be useful for branching on one sibling pair at a time in the solve() method.
 *
 * Returns one sibling leaf pair if it exists, as well as index of pair. Otherwise returns {-1, {nullptr, nullptr}}.
 * This is a helper function that can be used in the solve() method to quickly find a sibling pair
 * without having to compute the entire set of sibling pairs.
 *
 * It runs in O(n) time, but is likely faster in practice than getSiblingLeafPairs() since it can return early as soon as it finds a sibling pair.
 */
// TODO: we are getting stale/bad values in siblingPairParents_ for some reason
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

void Forest::setRoots(std::unordered_set<std::shared_ptr<TreeNode>> newRoots) {
	roots_ = newRoots;
    rebuildSiblingPairCache();
}

void Forest::setLeaves(std::unordered_set<std::shared_ptr<TreeNode>> newLeaves) {
	leaves_ = newLeaves;
    rebuildSiblingPairCache();
}

void Forest::setLeavesByLabel(std::unordered_map<std::string, std::shared_ptr<TreeNode>> newLeavesByLabel) {
	leafByLabel_ = newLeavesByLabel;
    rebuildSiblingPairCache();
}

void Forest::setNodesByCps(std::unordered_map<std::string, std::shared_ptr<TreeNode>> newNodesByCps) {
	nodeByCps_ = newNodesByCps;
}