#include <forest.h>


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
        return ((a + b)*(a + b + 1))/2 + b;
    }

bool SiblingPairEq::operator()(const std::pair<std::shared_ptr<TreeNode>, std::shared_ptr<TreeNode>>& a,
                    const std::pair<std::shared_ptr<TreeNode>, std::shared_ptr<TreeNode>>& b) const noexcept {
        return (a.first == b.first && a.second == b.second) || (a.first == b.second && a.second == b.first);
    }


void Forest::forestMergeCherry(std::shared_ptr<TreeNode> node) {
	
	leaves_.erase(node->left);
	leaves_.erase(node->right);
	leafByLabel_.erase(node->left->label);
	leafByLabel_.erase(node->right->label);
	nodeByCantor_.erase(node->left->hash);
	nodeByCantor_.erase(node->right->hash);

	mergeCherry(node);

	leaves_.insert(node);
	leafByLabel_[node->label] = node;
	nodeByCantor_[node->hash] = node;
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
}

std::string Forest::treeToNewick(const std::shared_ptr<TreeNode>& node) {
	if (!node) return "";
	if (node->isLeaf) return std::to_string(node->label);

	std::string result = "(";
	if (node->left)  result += treeToNewick(node->left);
	if (node->left && node->right) result += ",";
	if (node->right) result += treeToNewick(node->right);
	result += ")";
	if (node->label != 0) result += std::to_string(node->label);
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
        clone->hash = node->hash;

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
std::shared_ptr<TreeNode> Forest::getLeafByLabel(int label) {
	return leafByLabel_[label];
}

std::shared_ptr<TreeNode> Forest::getNodeByCantor(int cantor) {
	return nodeByCantor_[cantor];
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

void Forest::setRoots(std::unordered_set<std::shared_ptr<TreeNode>> newRoots) {
	roots_ = newRoots;
}
void Forest::setLeaves(std::unordered_set<std::shared_ptr<TreeNode>> newLeaves) {
	leaves_ = newLeaves;
}
void Forest::setLeavesByLabel(std::unordered_map<int, std::shared_ptr<TreeNode>> newLeavesByLabel) {
	leafByLabel_ = newLeavesByLabel;
}

void Forest::setNodesByCantor(std::unordered_map<int, std::shared_ptr<TreeNode>> newNodesByCantor) {
	nodeByCantor_ = newNodesByCantor;
}