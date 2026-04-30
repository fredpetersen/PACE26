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
    std::shared_ptr<TreeNode> lookupLeaf(const std::unordered_map<std::string, std::shared_ptr<TreeNode>>& leafByLabel,
                                         const std::string& label) {
        auto it = leafByLabel.find(label);
        if (it == leafByLabel.end()) {
            return nullptr;
        }
        return it->second;
    }

}

bool Forest::isSiblingPairNode(const std::shared_ptr<TreeNode>& node) const {
    return node != nullptr
        && !node->isMerged
        && node->left != nullptr
        && node->right != nullptr
        && node->left->isLeaf
        && node->right->isLeaf;
}

void Forest::updateSiblingPairParent(const std::shared_ptr<TreeNode>& node, MutationTrail* trail) {
    if (node == nullptr) {
        return;
    }

    bool shouldExist = isSiblingPairNode(node);
    auto key = node.get();
    auto it = siblingPairParents_.find(key);
    bool exists = it != siblingPairParents_.end();

    if (shouldExist && !exists) {
        siblingPairParents_.insert(key);
        if (trail != nullptr) {
            trail->record([this, key]() {
                siblingPairParents_.erase(key);
            });
        }
    } else if (!shouldExist && exists) {
        siblingPairParents_.erase(it);
        if (trail != nullptr) {
            trail->record([this, key]() {
                siblingPairParents_.insert(key);
            });
        }
    }
}

void Forest::rebuildSiblingPairCache() {
    siblingPairParents_.clear();

    std::unordered_map<std::shared_ptr<TreeNode>, std::vector<std::shared_ptr<TreeNode>>> parentToLeaves;
    for (const auto& leaf : leaves_) {
        if (leaf != nullptr && leaf->parent != nullptr) {
            parentToLeaves[leaf->parent].push_back(leaf);
        }
    }
    for (const auto& [parent, leaves] : parentToLeaves) {
        if (leaves.size() == 2 && parent != nullptr && !parent->isMerged && parent->left && parent->right && parent->left->isLeaf && parent->right->isLeaf) {
            siblingPairParents_.insert(parent.get());
        }
    }
}


void Forest::forestMergeCherry(std::shared_ptr<TreeNode> node, MutationTrail* trail) {
    if (node == nullptr) {
        return;
    }
    auto l = node->left;
    auto r = node->right;
    if (l == nullptr || r == nullptr || !l->isLeaf || !r->isLeaf) {
        return;
    }

    auto oldLabel = node->label;
    auto oldIsLeaf = node->isLeaf;
    auto oldIsMerged = node->isMerged;

    removeLeaf(l, trail);
    removeLeaf(r, trail);

    mergeCherry(node);
    if (trail != nullptr) {
        trail->record([node, oldLabel, oldIsLeaf, oldIsMerged]() {
            node->label = oldLabel;
            node->isLeaf = oldIsLeaf;
            node->isMerged = oldIsMerged;
        });
    }

    addLeaf(node, trail);
    updateSiblingPairParent(node, trail);
    updateSiblingPairParent(node->parent, trail);
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
        trail->record([node, oldLabel, oldIsLeaf, oldIsMerged]() {
            node->label = oldLabel;
            node->isLeaf = oldIsLeaf;
            node->isMerged = oldIsMerged;
        });
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

void Forest::detachChild(std::shared_ptr<TreeNode> child, bool shouldContract, MutationTrail* trail) {
    if (child == nullptr) {
        return;
    }
    auto parent = child->parent;
    if (parent != nullptr) {
        bool isLeftChild = parent->left == child;
        bool isRightChild = parent->right == child;
        if (!isLeftChild && !isRightChild) {
            std::cout << "# The given node does not have that child" << std::endl;
            return;
        }

        auto inserted = roots_.insert(child).second;
        if (trail != nullptr && inserted) {
            trail->record([this, child]() {
                roots_.erase(child);
            });
        }

        auto previousParent = child->parent;
        child->parent = nullptr;
        if (trail != nullptr) {
            trail->record([child, previousParent]() {
                child->parent = previousParent;
            });
        }

        if (isLeftChild) {
            auto previousLeft = parent->left;
            parent->left = nullptr;
            if (trail != nullptr) {
                trail->record([parent, previousLeft]() {
                    parent->left = previousLeft;
                });
            }
        } else {
            auto previousRight = parent->right;
            parent->right = nullptr;
            if (trail != nullptr) {
                trail->record([parent, previousRight]() {
                    parent->right = previousRight;
                });
            }
        }

        updateSiblingPairParent(parent, trail);

        ++componentCount_;
        if (trail != nullptr) {
            trail->record([this]() {
                --componentCount_;
            });
        }

        if (shouldContract) {
            contract(parent, trail);
        }
    }
}

void Forest::detachByLabel(const std::string& label, MutationTrail* trail) {
    auto leaf = getLeafByLabel(label);
    if (leaf == nullptr) {
        return;
    }
    detachChild(leaf, true, trail);
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
    if (child == nullptr) return;

    if (v->parent != nullptr) { // v is not root node
        auto parent = v->parent;
        bool isRightChild = parent->right.get() == v.get();

        auto previousChildParent = child->parent;
        child->parent = parent;
        if (trail != nullptr) {
            trail->record([child, previousChildParent]() {
                child->parent = previousChildParent;
            });
        }

        if (isRightChild) {
            auto previousRight = parent->right;
            parent->right = child;
            if (trail != nullptr) {
                trail->record([parent, previousRight]() {
                    parent->right = previousRight;
                });
            }
        } else {
            auto previousLeft = parent->left;
            parent->left = child;
            if (trail != nullptr) {
                trail->record([parent, previousLeft]() {
                    parent->left = previousLeft;
                });
            }
        }

        updateSiblingPairParent(parent, trail);

    } else { // v is root node
        auto previousChildParent = child->parent;
        child->parent = nullptr;
        if (trail != nullptr) {
            trail->record([child, previousChildParent]() {
                child->parent = previousChildParent;
            });
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

std::vector<std::shared_ptr<TreeNode>> Forest::collectPendantSubtreesBetweenLeaves(const std::string& u_label,
                                                    const std::string& v_label,
                                                    std::shared_ptr<TreeNode>& lcaNode) {
        std::vector<std::shared_ptr<TreeNode>> pendantSubtrees;
        auto u = getLeafByLabel(u_label);
        auto v = getLeafByLabel(v_label);

        if (u == nullptr || v == nullptr || lcaNode == nullptr) {
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

        collectFromLeaf(u);
        collectFromLeaf(v);
        return pendantSubtrees;
    };

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

std::shared_ptr<TreeNode> Forest::cloneTree(
        const std::shared_ptr<TreeNode>& node) {
        if (node == nullptr) {
            return nullptr;
        }

        auto clone = std::make_shared<TreeNode>();
        clone->isLeaf = node->isLeaf;
        clone->label = node->label;
		clone->isMerged = node->isMerged;

        if (node->left != nullptr) {
            clone->left = cloneTree(node->left);
            clone->left->parent = clone;
        }
        if (node->right != nullptr) {
            clone->right = cloneTree(node->right);
            clone->right->parent = clone;
        }

        if (clone->isLeaf) {
            this->addLeaf(clone);
        }

        return clone;
    }

std::shared_ptr<Forest> Forest::cloneForest() const {
	auto clone = std::make_shared<Forest>();
	clone->setComponentCount(componentCount_);

	for (const auto& root : roots_) {
		auto clonedRoot = clone->cloneTree(root);
		clone->addRoot(clonedRoot);
	}
    clone->rebuildSiblingPairCache();

	return clone;
}

void Forest::print(const std::string& name) const {
	for (const auto& root : roots_) {
		this->printTree(root.get(), name);
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

	this->printTreeRecursive(node->left.get(), "    ", true);
	this->printTreeRecursive(node->right.get(), "    ", false);
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
	this->printTreeRecursive(node->left.get(), childPrefix, true);
	this->printTreeRecursive(node->right.get(), childPrefix, false);
}

void Forest::printForestNewick() {
	for (const auto& root : roots_) {
			std::cout << treeToNewick(root) << ";\n";
	}
}

void Forest::setComponentCount(int newCount, MutationTrail* trail) {
    if (trail != nullptr) {
        auto previousCount = componentCount_;
        trail->record([this, previousCount]() {
            componentCount_ = previousCount;
        });
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
        trail->record([this, node]() {
            roots_.erase(node);
        });
    }
}

void Forest::addLeaf(std::shared_ptr<TreeNode> node, MutationTrail* trail) {
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
        trail->record([this, node, label, previousNode, hadPrevious]() {
            leaves_.erase(node);
            if (hadPrevious) {
                leafByLabel_[label] = previousNode;
            } else {
                leafByLabel_.erase(label);
            }
        });
    }

    updateSiblingPairParent(node->parent, trail);
}

void Forest::removeRoot(std::shared_ptr<TreeNode> node, MutationTrail* trail) {
    if (node == nullptr) {
        return;
    }
    auto erased = roots_.erase(node);
    if (trail != nullptr && erased > 0) {
        trail->record([this, node]() {
            roots_.insert(node);
        });
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
        trail->record([this, node, label, previousNode, hadMapping]() {
            leaves_.insert(node);
            if (hadMapping) {
                leafByLabel_[label] = previousNode;
            }
        });
    }

    updateSiblingPairParent(node->parent, trail);
}

std::unordered_set<std::shared_ptr<TreeNode>> Forest::getRoots() {
	return roots_;
}
std::unordered_set<std::shared_ptr<TreeNode>> Forest::getLeaves() {
	return leaves_;
}
std::shared_ptr<TreeNode> Forest::getLeafByLabel(const std::string& label) const {
    return lookupLeaf(leafByLabel_, label);
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
std::pair<int, Forest::SiblingPair> Forest::getOneSiblingPair(int startIndex) {
    if (siblingPairParents_.empty()) {
        return {-1, {nullptr, nullptr}};
    }

    (void)startIndex;
    auto it = siblingPairParents_.begin();
    auto parent = *it;
    if (parent == nullptr || parent->left == nullptr || parent->right == nullptr) {
        return {-1, {nullptr, nullptr}};
    }
    return {0, {parent->left, parent->right}};
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