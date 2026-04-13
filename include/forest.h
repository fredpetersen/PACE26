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
		inline Forest() : roots_({}), leaves_({}), leafByLabel_({}) {}

		inline Forest(std::unordered_set<std::shared_ptr<TreeNode>> roots, std::unordered_set<std::shared_ptr<TreeNode>> leaves,
			std::unordered_map<int, std::shared_ptr<TreeNode>> leafByLabel)
			: roots_(roots), leaves_(leaves), componentCount_(1), leafByLabel_(leafByLabel) {}

	    void detachChild(std::shared_ptr<TreeNode> child);
		std::string treeToNewick(const std::shared_ptr<TreeNode>& node);
		void printForestNewick();

		std::shared_ptr<Forest> cloneForest() const;
		std::shared_ptr<TreeNode> cloneTree(
        const std::shared_ptr<TreeNode>& node);

		void setComponentCount(int newCount);
		int getComponentCount();

		void addRoot(std::shared_ptr<TreeNode> node);
		void addLeaf(std::shared_ptr<TreeNode> node);

		void removeRoot(std::shared_ptr<TreeNode> node);
		void removeLeaf(std::shared_ptr<TreeNode> node);

		std::unordered_set<std::shared_ptr<TreeNode>> getRoots();
		std::unordered_set<std::shared_ptr<TreeNode>> getLeaves();
		std::shared_ptr<TreeNode> getLeafByLabel(int label);

		void setRoots(std::unordered_set<std::shared_ptr<TreeNode>> newRoots);
		void setLeaves(std::unordered_set<std::shared_ptr<TreeNode>> newLeaves);
		void setLeavesByLabel(std::unordered_map<int, std::shared_ptr<TreeNode>> newLeavesByLabel);
};