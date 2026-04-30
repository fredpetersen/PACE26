#pragma once

#include <functional>
#include <unordered_set>
#include <unordered_map>
#include <iostream>
#include <string>
#include <memory>
#include <vector>

#include <tree_node.h>

class MutationTrail {
public:
	using UndoAction = std::function<void()>;

	std::size_t checkpoint() const {
		return actions_.size();
	}

	void record(UndoAction action) {
		actions_.push_back(std::move(action));
	}

	void rollback(std::size_t checkpoint) {
		while (actions_.size() > checkpoint) {
			auto action = std::move(actions_.back());
			actions_.pop_back();
			action();
		}
	}

private:
	std::vector<UndoAction> actions_;
};

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
	std::unordered_map<std::string, std::shared_ptr<TreeNode>> leafByLabel_;
	std::unordered_set<TreeNode*> siblingPairParents_;

	bool isSiblingPairNode(TreeNode* node) const;
	std::shared_ptr<TreeNode> getOwningHandle(TreeNode* node) const;
	void updateSiblingPairParent(TreeNode* node, MutationTrail* trail);
	void rebuildSiblingPairCache();

	public:
		inline Forest() : roots_({}), leaves_({}), leafByLabel_({}), siblingPairParents_({}) {}

		inline Forest(std::unordered_set<std::shared_ptr<TreeNode>> roots, std::unordered_set<std::shared_ptr<TreeNode>> leaves,
			std::unordered_map<std::string, std::shared_ptr<TreeNode>> leafByLabel)
			: roots_(roots), leaves_(leaves), componentCount_(1), leafByLabel_(leafByLabel) {
			rebuildSiblingPairCache();
		}

		void forestMergeCherry(TreeNode* node, MutationTrail* trail = nullptr);
		void expandMergedSubtrees(MutationTrail* trail = nullptr);
		void expandRecursive(std::shared_ptr<TreeNode> node, MutationTrail* trail = nullptr);

		void detachChild(std::shared_ptr<TreeNode> child, bool shouldContract = true, MutationTrail* trail = nullptr);
		void detachByLabel(const std::string& label, MutationTrail* trail = nullptr);
		void contract(TreeNode* v, MutationTrail* trail = nullptr);
		void contract(std::shared_ptr<TreeNode> v, MutationTrail* trail = nullptr) { contract(v.get(), trail); }
		void contractIntoCherry(const std::string& lab_u, const std::string& lab_v, TreeNode* ancestor, MutationTrail* trail = nullptr);

		std::pair<TreeNode*, int> lca(const std::string& label_u, const std::string& label_v);
		std::vector<std::shared_ptr<TreeNode>> collectPendantSubtreesBetweenLeaves(const std::string& leftLeaf,
	                                                    const std::string& rightLeaf,
	                                                    TreeNode* lcaNode);

		std::string treeToNewick(const std::shared_ptr<TreeNode>& node);
		void printForestNewick();

		std::shared_ptr<Forest> cloneForest() const;
		std::shared_ptr<TreeNode> cloneTree(
        const std::shared_ptr<TreeNode>& node);

		void setComponentCount(int newCount, MutationTrail* trail = nullptr);
		int getComponentCount();

		void print(const std::string& name) const;

		void printTreeRecursive(const TreeNode* node, const std::string& prefix, bool isLeft) const;
		void printTree(const TreeNode* root, const std::string& name) const;

		void addRoot(std::shared_ptr<TreeNode> node, MutationTrail* trail = nullptr);
		void addLeaf(std::shared_ptr<TreeNode> node, MutationTrail* trail = nullptr);

		void removeRoot(TreeNode* node, MutationTrail* trail = nullptr);
		void removeLeaf(std::shared_ptr<TreeNode> node, MutationTrail* trail = nullptr);

		std::unordered_set<std::shared_ptr<TreeNode>> getRoots();
		std::unordered_set<std::shared_ptr<TreeNode>> getLeaves();
		std::shared_ptr<TreeNode> getLeafByLabel(const std::string& label) const;
		SiblingPairSet getSiblingLeafPairs();
		std::pair<int, SiblingPair> getOneSiblingPair(int startIndex = 0);

		void setRoots(std::unordered_set<std::shared_ptr<TreeNode>> newRoots);
		void setLeaves(std::unordered_set<std::shared_ptr<TreeNode>> newLeaves);
		void setLeavesByLabel(std::unordered_map<std::string, std::shared_ptr<TreeNode>> newLeavesByLabel);
};