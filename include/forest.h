#pragma once

#include <unordered_set>
#include <unordered_map>
#include <iostream>
#include <string>
#include <memory>

#include <tree_node.h>

class Forest {
	std::unordered_set<std::shared_ptr<TreeNode>> roots_;
	std::unordered_set<std::shared_ptr<TreeNode>> leaves_;
	int componentCount_;
	std::unordered_map<int, std::shared_ptr<TreeNode>> leafByLabel_;

	public:
		Forest(std::unordered_set<std::shared_ptr<TreeNode>> roots, std::unordered_set<std::shared_ptr<TreeNode>> leaves,
			std::unordered_map<int, std::shared_ptr<TreeNode>> leafByLabel);

	    void detachChild(std::shared_ptr<TreeNode> child);
		std::string treeToNewick(const std::shared_ptr<TreeNode>& node);
		void printForestNewick();

		void addRoot(std::shared_ptr<TreeNode> node);

		void removeRoot(std::shared_ptr<TreeNode> node);
		void removeLeaf(std::shared_ptr<TreeNode> node);

		std::unordered_set<std::shared_ptr<TreeNode>> getRoots();
		std::unordered_set<std::shared_ptr<TreeNode>> getLeaves();
		std::shared_ptr<TreeNode> getLeafByLabel(int label);

};