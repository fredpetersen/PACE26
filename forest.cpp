#include <forest.h>

std::unordered_set<std::shared_ptr<TreeNode>> roots_;
std::unordered_set<std::shared_ptr<TreeNode>> leaves_;
int componentCount_;
std::unordered_map<int, std::shared_ptr<TreeNode>> leafByLabel_;

Forest::Forest(std::unordered_set<std::shared_ptr<TreeNode>> roots, std::unordered_set<std::shared_ptr<TreeNode>> leaves,
	std::unordered_map<int, std::shared_ptr<TreeNode>> leafByLabel)
	: roots_(roots), leaves_(leaves), componentCount_(1), leafByLabel_(leafByLabel) {}

void Forest::detachChild(std::shared_ptr<TreeNode> child) {
	//TODO: Is this really how you assign shared pointers?
	auto parent = child->parent;
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

void Forest::printForestNewick() {
	for (const auto& root : roots_) {
			std::cout << treeToNewick(root) << ";\n";
	}
}

void Forest::addRoot(std::shared_ptr<TreeNode> node) {
	roots_.insert(node);
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