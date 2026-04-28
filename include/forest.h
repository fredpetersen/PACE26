#pragma once

#include <unordered_set>
#include <unordered_map>
#include <iostream>
#include <string>
#include <memory>
#include <vector>

#include <tree_node.h>

struct SiblingPairHash {
    size_t operator()(const std::pair<std::shared_ptr<TreeNode>, std::shared_ptr<TreeNode>>& p) const noexcept;
};

struct SiblingPairEq {
    bool operator()(const std::pair<std::shared_ptr<TreeNode>, std::shared_ptr<TreeNode>>& a,
                    const std::pair<std::shared_ptr<TreeNode>, std::shared_ptr<TreeNode>>& b) const noexcept;
};

class Forest {
	std::unordered_set<std::shared_ptr<TreeNode>> roots_;
	std::unordered_set<std::shared_ptr<TreeNode>> leaves_;
	int componentCount_;
	std::unordered_map<std::string, std::shared_ptr<TreeNode>> leafByLabel_;

	public:
		inline Forest() : roots_({}), leaves_({}), leafByLabel_({}) {}

		inline Forest(std::unordered_set<std::shared_ptr<TreeNode>> roots, std::unordered_set<std::shared_ptr<TreeNode>> leaves,
			std::unordered_map<std::string, std::shared_ptr<TreeNode>> leafByLabel)
			: roots_(roots), leaves_(leaves), componentCount_(1), leafByLabel_(leafByLabel) {}

		void forestMergeCherry(std::shared_ptr<TreeNode> node);
		void expandMergedSubtrees();
		void expandRecursive(std::shared_ptr<TreeNode> node);

	    void detachChild(std::shared_ptr<TreeNode> child);
		void detachByLabel(std::string label);
		void contract(std::shared_ptr<TreeNode> v);
		
		std::pair<std::shared_ptr<TreeNode>, int> lca(std::string label_u, std::string label_v);

		std::string treeToNewick(const std::shared_ptr<TreeNode>& node);
		void printForestNewick();

		std::shared_ptr<Forest> cloneForest() const;
		std::shared_ptr<TreeNode> cloneTree(
        const std::shared_ptr<TreeNode>& node);

		void setComponentCount(int newCount);
		int getComponentCount();

		void print(const std::string& name) const;

		void printTreeRecursive(const TreeNode* node, const std::string& prefix, bool isLeft) const;
		void printTree(const TreeNode* root, const std::string& name) const;

		void addRoot(std::shared_ptr<TreeNode> node);
		void addLeaf(std::shared_ptr<TreeNode> node);

		void removeRoot(std::shared_ptr<TreeNode> node);
		void removeLeaf(std::shared_ptr<TreeNode> node);

		std::unordered_set<std::shared_ptr<TreeNode>> getRoots();
		std::unordered_set<std::shared_ptr<TreeNode>> getLeaves();
		std::shared_ptr<TreeNode> getLeafByLabel(std::string label);
		std::unordered_set<std::pair<std::shared_ptr<TreeNode>, std::shared_ptr<TreeNode>>, SiblingPairHash,SiblingPairEq> getSiblingLeafPairs();
		std::pair<int, std::pair<std::shared_ptr<TreeNode>, std::shared_ptr<TreeNode>>> getOneSiblingPair(int startIndex = 0);

		void setRoots(std::unordered_set<std::shared_ptr<TreeNode>> newRoots);
		void setLeaves(std::unordered_set<std::shared_ptr<TreeNode>> newLeaves);
		void setLeavesByLabel(std::unordered_map<std::string, std::shared_ptr<TreeNode>> newLeavesByLabel);
};