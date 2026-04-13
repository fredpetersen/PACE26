#include <forest.h>

std::unordered_set<std::shared_ptr<TreeNode>> roots_;
std::unordered_set<std::shared_ptr<TreeNode>> leaves_;
int componentCount_;
std::unordered_map<int, std::shared_ptr<TreeNode>> leafByLabel_;

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
        const std::shared_ptr<TreeNode>& node,
        std::unordered_map<const TreeNode*, std::shared_ptr<TreeNode>>& memo) {
        if (node == nullptr) {
            return nullptr;
        }

        auto memoIt = memo.find(node.get());
        if (memoIt != memo.end()) {
            return memoIt->second;
        }

        auto clone = std::make_shared<TreeNode>();
        clone->isLeaf = node->isLeaf;
        clone->label = node->label;
        clone->hash = node->hash;
        memo.emplace(node.get(), clone);

        if (node->left != nullptr) {
            clone->left = cloneTree(node->left, memo);
            clone->left->parent = clone;
        }
        if (node->right != nullptr) {
            clone->right = cloneTree(node->right, memo);
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

	std::unordered_map<const TreeNode*, std::shared_ptr<TreeNode>> memo;
	for (const auto& root : roots_) {
		auto clonedRoot = clone->cloneTree(root, memo);
		clone->addRoot(clonedRoot);
	}

	return clone;
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

void Forest::setRoots(std::unordered_set<std::shared_ptr<TreeNode>> newRoots) {
	roots_ = newRoots;
}
void Forest::setLeaves(std::unordered_set<std::shared_ptr<TreeNode>> newLeaves) {
	leaves_ = newLeaves;
}
void Forest::setLeavesByLabel(std::unordered_map<int, std::shared_ptr<TreeNode>> newLeavesByLabel) {
	leafByLabel_ = newLeavesByLabel;
}