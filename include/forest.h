#pragma once

#include <functional>
#include <unordered_set>
#include <unordered_map>
#include <iostream>
#include <string>
#include <memory>
#include <vector>

#include <utils.h>
#include <mutation_trail.h>
#include <tree_node.h>

struct SiblingPairHash {
    size_t operator()(const std::pair<std::shared_ptr<TreeNode>, std::shared_ptr<TreeNode>>& p) const noexcept;
};

struct SiblingPairEq {
    bool operator()(const std::pair<std::shared_ptr<TreeNode>, std::shared_ptr<TreeNode>>& a,
                    const std::pair<std::shared_ptr<TreeNode>, std::shared_ptr<TreeNode>>& b) const noexcept;
};

class Forest {
	public:
		using SiblingPair = std::pair<std::shared_ptr<TreeNode>, std::shared_ptr<TreeNode>>;
		using SiblingPairSet = std::unordered_set<SiblingPair, SiblingPairHash, SiblingPairEq>;

	private:
	std::unordered_set<std::shared_ptr<TreeNode>> roots_;
	std::unordered_set<std::shared_ptr<TreeNode>> leaves_;
	int componentCount_;
	std::unordered_map<std::string, std::shared_ptr<TreeNode>> leafByLabel_; // Could probably be merged into nodeByLabel_ if label and cps can be merged
	std::unordered_map<std::string, std::shared_ptr<TreeNode>> nodeByCps_;
	std::unordered_set<std::shared_ptr<TreeNode>> siblingPairParents_;

	bool isSiblingPairNode(std::shared_ptr<TreeNode> node) const;
	void updateSiblingPairParent(std::shared_ptr<TreeNode> node, MutationTrail* trail);
	void rebuildSiblingPairCache();

	public:
		inline Forest() : roots_({}), leaves_({}), leafByLabel_({}), nodeByCps_({}), siblingPairParents_({}) {}

		inline Forest(std::unordered_set<std::shared_ptr<TreeNode>> roots, std::unordered_set<std::shared_ptr<TreeNode>> leaves,
			std::unordered_map<std::string, std::shared_ptr<TreeNode>> leafByLabel, std::unordered_map<std::string, std::shared_ptr<TreeNode>> nodeByCps)
			: roots_(roots), leaves_(leaves), componentCount_(1), leafByLabel_(leafByLabel), nodeByCps_(nodeByCps) {
			rebuildSiblingPairCache();
		}

		void forestLocalMergeCherry(std::shared_ptr<TreeNode> node, MutationTrail* trail = nullptr);
		void expandMergedSubtrees(MutationTrail* trail = nullptr);
		void expandRecursive(std::shared_ptr<TreeNode> node, MutationTrail* trail = nullptr);

		std::string detachChild(std::shared_ptr<TreeNode> child, std::unordered_map<std::string, int>& cpsMap, bool shouldContract = true, MutationTrail* trail = nullptr);
		std::string detachByLabel(const std::string& label, std::unordered_map<std::string, int>& cpsMap, MutationTrail* trail = nullptr);
		void contract(std::shared_ptr<TreeNode> v, MutationTrail* trail = nullptr);
		void contractIntoCherry(const std::string& lab_u, const std::string& lab_v, std::shared_ptr<TreeNode> ancestor, MutationTrail* trail = nullptr);

		std::pair<std::shared_ptr<TreeNode>, int> lca(const std::string& label_u, const std::string& label_v);
		std::vector<std::shared_ptr<TreeNode>> collectPendantSubtreesBetweenLeaves(const std::string& leftLeaf,
	                                                    const std::string& rightLeaf,
	                                                    std::shared_ptr<TreeNode> lcaNode);

		std::string cpsReduction(std::shared_ptr<TreeNode> node, std::unordered_map<std::string, int>& cpsMap, MutationTrail* trail = nullptr);

		std::string treeToNewick(const std::shared_ptr<TreeNode>& node);
		void printForestNewick();

		void setComponentCount(int newCount, MutationTrail* trail = nullptr);
		int getComponentCount();

		void print(const std::string& name) const;
		void printTreeRecursive(const std::shared_ptr<TreeNode> node, const std::string& prefix, bool isLeft) const;
		void printTree(const std::shared_ptr<TreeNode> root, const std::string& name) const;

		void printCps() const;

		void addRoot(std::shared_ptr<TreeNode> node, MutationTrail* trail = nullptr);
		void addLeaf(std::shared_ptr<TreeNode> node, MutationTrail* trail = nullptr);

		void removeRoot(std::shared_ptr<TreeNode> node, MutationTrail* trail = nullptr);
		void removeLeaf(std::shared_ptr<TreeNode> node, MutationTrail* trail = nullptr);

		std::unordered_set<std::shared_ptr<TreeNode>> getRoots();
		std::unordered_set<std::shared_ptr<TreeNode>> getLeaves();

		std::shared_ptr<TreeNode> getLeafByLabel(const std::string& label) const;
		std::shared_ptr<TreeNode> getNodeByCps(const std::string& label) const;

		SiblingPairSet getSiblingLeafPairs();
		std::pair<int, SiblingPair> getOneSiblingPair(int startIndex = 0);

		void setRoots(std::unordered_set<std::shared_ptr<TreeNode>> newRoots);
		void setLeaves(std::unordered_set<std::shared_ptr<TreeNode>> newLeaves);
		void setLeavesByLabel(std::unordered_map<std::string, std::shared_ptr<TreeNode>> newLeavesByLabel);
		void setNodesByCps(std::unordered_map<std::string, std::shared_ptr<TreeNode>> newNodesByCps);
};