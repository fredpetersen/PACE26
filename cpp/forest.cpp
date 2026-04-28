#include <forest.h>


/* Hashing and equality functions assume that the TreeNodes will always be given in the same order

Thus they are sensitive to the order of the pair, as (u,v) and (v,u) will be considered different pairs
*/
size_t SiblingPairHash::operator()(const std::pair<std::shared_ptr<TreeNode>, std::shared_ptr<TreeNode>>& p) const noexcept {
        auto a = stoi(p.first->label);
        auto b = stoi(p.second->label);
        if (a > b) {
            auto c = a;
            a = b;
            b = c;
        }
        return ((a + b)*(a + b + 1))/2 + b;
    }

//TODO: Check string equality actually works
bool SiblingPairEq::operator()(const std::pair<std::shared_ptr<TreeNode>, std::shared_ptr<TreeNode>>& a,
                    const std::pair<std::shared_ptr<TreeNode>, std::shared_ptr<TreeNode>>& b) const noexcept {
        return (a.first == b.first && a.second == b.second) || (a.first == b.second && a.second == b.first);
    }


void Forest::forestMergeCherry(std::shared_ptr<TreeNode> node) {
	auto l = node->left;
	auto r = node->right;
    if (l->isLeaf && r->isLeaf) {
		leaves_.erase(l);
		leaves_.erase(r);
		leafByLabel_.erase(l->label);
		leafByLabel_.erase(r->label);

		mergeCherry(node);

		leaves_.insert(node);
		leafByLabel_[node->label] = node;
	}
}

void Forest::expandMergedSubtrees() {
	for (const auto leaf: leaves_) {
		if (leaf->isMerged) {
			expandRecursive(leaf);
		}
	}
}

void Forest::expandRecursive(std::shared_ptr<TreeNode> node) {
		leaves_.erase(node);
		leafByLabel_.erase(node->label);

		node->isMerged = false;
		node->isLeaf = false;

		auto l = node->left;
		auto r = node->right;

		if (!l->isMerged) {
			leaves_.insert(l);
			leafByLabel_[l->label] = l;
		} else {
			expandRecursive(l);
		}
		if (!r->isMerged) {
			leaves_.insert(r);
			leafByLabel_[r->label] = r;
		} else {
			expandRecursive(r);
		}
	}

void Forest::detachChild(std::shared_ptr<TreeNode> child) {
	//TODO: Is this really how you assign shared pointers?
	auto parent = child->parent;
	roots_.insert(child);
	if (parent->left == child) {
		child->parent = nullptr;
		parent->left = nullptr;
	} else if (parent->right == child) {
		child->parent = nullptr;
		parent->right = nullptr;
	} else {
		std::cout << "The given node does not have that child" << std::endl;
	}
	roots_.insert(child);
	contract(parent);
}

void Forest::detachByLabel(std::string label) {
	auto leaf = getLeafByLabel(label);
	detachChild(leaf);
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
void Forest::contract(std::shared_ptr<TreeNode> v) {
    bool hasLeftChild = v->left != nullptr;
    bool hasRightChild = v->right.get() != nullptr;
    if (hasLeftChild && hasRightChild) return; // Both children are present; can't contract
    if (v->isLeaf) return; // can't contract leaf nodes

    auto child = hasLeftChild ? v->left : v->right; // The only child of v

    if (v->parent != nullptr) { // v is not root node
        bool isRightChild = v->parent->right.get() == v.get();
        if (isRightChild) {
            v->parent->right = std::move(child);
        } else {
            v->parent->left = std::move(child);
        }

    } else { // v is root node
        child->parent = nullptr;
        removeRoot(v);
        addRoot(child);
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
std::pair<std::shared_ptr<TreeNode>, int> Forest::lca(std::string label_u, std::string label_v) {
    auto u = leafByLabel_[label_u];
    auto v = leafByLabel_[label_v];
	
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

std::vector<std::shared_ptr<TreeNode>> Forest::collectPendantSubtreesBetweenLeaves(std::string u_label,
                                                    std::string v_label,
                                                    std::shared_ptr<TreeNode>& lcaNode) {
        std::vector<std::shared_ptr<TreeNode>> pendantSubtrees;
		auto u = leafByLabel_[u_label];
		auto v = leafByLabel_[v_label];

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

void Forest::setComponentCount(int newCount) {
	componentCount_ = newCount;
}

int Forest::getComponentCount() {
	return componentCount_;
}

void Forest::addRoot(std::shared_ptr<TreeNode> node) {
	roots_.insert(node);
}

void Forest::addLeaf(std::shared_ptr<TreeNode> node) {
	leaves_.insert(node);
	leafByLabel_[node->label] = node;
}

void Forest::removeRoot(std::shared_ptr<TreeNode> node) {
	roots_.erase(node);
}

void Forest::removeLeaf(std::shared_ptr<TreeNode> node) {
	leaves_.erase(node);
}

std::unordered_set<std::shared_ptr<TreeNode>> Forest::getRoots() {
	return roots_;
}
std::unordered_set<std::shared_ptr<TreeNode>> Forest::getLeaves() {
	return leaves_;
}
std::shared_ptr<TreeNode> Forest::getLeafByLabel(std::string label) {
	return leafByLabel_[label];
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
    std::pair<std::shared_ptr<TreeNode>, std::shared_ptr<TreeNode>>,
    SiblingPairHash,SiblingPairEq
> Forest::getSiblingLeafPairs() {

    std::unordered_set<
        std::pair<std::shared_ptr<TreeNode>, std::shared_ptr<TreeNode>>,
        SiblingPairHash,SiblingPairEq> siblingLeafPairs;

    std::unordered_map<std::shared_ptr<TreeNode>, std::vector<std::shared_ptr<TreeNode>>> parentToLeaves;
    for (const auto& leaf : leaves_) {
        if (leaf->parent != nullptr) {
            parentToLeaves[leaf->parent].push_back(leaf);
        }
    }
    for (const auto& [parent, leaves] : parentToLeaves) {
        if (leaves.size() == 2) {
            siblingLeafPairs.insert({leaves[0], leaves[1]});
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
std::pair<int, std::pair<std::shared_ptr<TreeNode>, std::shared_ptr<TreeNode>>> Forest::getOneSiblingPair(int startIndex) {
		int currentIndex = startIndex;
		auto it = leaves_.begin();
		it = std::next(it, currentIndex);
		for (; it != leaves_.end(); ++it) {
				auto leaf = *it;
				if (leaf->parent != nullptr) {
						auto parent = leaf->parent;
						if (parent->left && parent->right && parent->left->isLeaf && parent->right->isLeaf) {
								return {currentIndex, {parent->left, parent->right}};
						}
				}
				currentIndex++;
		}
		return {-1, {nullptr, nullptr}}; // No sibling pairs found
}

void Forest::setRoots(std::unordered_set<std::shared_ptr<TreeNode>> newRoots) {
	roots_ = newRoots;
}
void Forest::setLeaves(std::unordered_set<std::shared_ptr<TreeNode>> newLeaves) {
	leaves_ = newLeaves;
}
void Forest::setLeavesByLabel(std::unordered_map<std::string, std::shared_ptr<TreeNode>> newLeavesByLabel) {
	leafByLabel_ = newLeavesByLabel;
}