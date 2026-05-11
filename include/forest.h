#pragma once

#include <cstdint>
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
    size_t operator()(const std::pair<TreeNode*, TreeNode*>& p) const noexcept;
};

struct SiblingPairEq {
    bool operator()(const std::pair<TreeNode*, TreeNode*>& a,
                    const std::pair<TreeNode*, TreeNode*>& b) const noexcept;
};

class Forest {
	public:
		using SiblingPair = std::pair<TreeNode*, TreeNode*>;
		using SiblingPairSet = std::unordered_set<SiblingPair, SiblingPairHash, SiblingPairEq>;

	private:
	friend class MutationTrail;
	std::unordered_set<TreeNode*> roots_;
	std::unordered_set<TreeNode*> leaves_;
	int componentCount_;
	std::unordered_map<std::string, TreeNode*> leafByLabel_;
	std::unordered_map<uint64_t, TreeNode*> nodeByCps_;
	std::unordered_set<TreeNode*> siblingPairParents_;

	bool isSiblingPairNode(TreeNode* node) const;
	void updateSiblingPairParent(TreeNode* node, MutationTrail* trail);
	void rebuildSiblingPairCache();

	public:
		inline Forest() : roots_({}), leaves_({}), componentCount_(0), leafByLabel_({}), nodeByCps_({}), siblingPairParents_({}) {}

		inline Forest(std::unordered_set<TreeNode*> roots, std::unordered_set<TreeNode*> leaves,
			std::unordered_map<std::string, TreeNode*> leafByLabel, std::unordered_map<uint64_t, TreeNode*> nodeByCps)
			: roots_(std::move(roots)), leaves_(std::move(leaves)), componentCount_(1), leafByLabel_(std::move(leafByLabel)), nodeByCps_(std::move(nodeByCps)) {
			rebuildSiblingPairCache();
		}

		void forestLocalMergeCherry(TreeNode* node, MutationTrail* trail = nullptr);
		void expandMergedSubtrees(MutationTrail* trail = nullptr);
		void expandRecursive(TreeNode* node, MutationTrail* trail = nullptr);

		uint64_t detachChild(TreeNode* child, std::unordered_map<uint64_t, int>& cpsMap, bool shouldContract = true, MutationTrail* trail = nullptr);
		uint64_t detachByLabel(const std::string& label, std::unordered_map<uint64_t, int>& cpsMap, MutationTrail* trail = nullptr);
		void contract(TreeNode* v, MutationTrail* trail = nullptr);
		void contractIntoCherry(const std::string& lab_u, const std::string& lab_v, TreeNode* ancestor, MutationTrail* trail = nullptr);

		std::pair<TreeNode*, int> lca(const std::string& label_u, const std::string& label_v);
		std::vector<TreeNode*> collectPendantSubtreesBetweenLeaves(const std::string& leftLeaf,
	                                                    const std::string& rightLeaf,
	                                                    TreeNode* lcaNode);

		uint64_t cpsReduction(TreeNode* node, std::unordered_map<uint64_t, int>& cpsMap, MutationTrail* trail = nullptr);

		std::string treeToNewick(const TreeNode* node);
		// Canonical Newick: child subtrees are emitted in lexicographic order
		// so that two structurally-identical subtrees with their children in
		// opposite orders (which hash equally — mixHashes is symmetric)
		// produce the SAME label. Required by cpsReduction so merged-leaf
		// labels match across forests for cascading lookups.
		static std::string canonicalNewick(const TreeNode* node);
		void printForestNewick();

		void setComponentCount(int newCount, MutationTrail* trail = nullptr);
		int getComponentCount();

		void print(const std::string& name) const;
		void printTreeRecursive(const TreeNode* node, const std::string& prefix, bool isLeft) const;
		void printTree(const TreeNode* root, const std::string& name) const;

		void printCps() const;

		void addRoot(TreeNode* node, MutationTrail* trail = nullptr);
		void addLeaf(TreeNode* node, MutationTrail* trail = nullptr);

		void removeRoot(TreeNode* node, MutationTrail* trail = nullptr);
		void removeLeaf(TreeNode* node, MutationTrail* trail = nullptr);

		const std::unordered_set<TreeNode*>& getRoots() const;
		const std::unordered_set<TreeNode*>& getLeaves() const;

		TreeNode* getLeafByLabel(const std::string& label) const;
		TreeNode* getNodeByCps(uint64_t hash) const;

		SiblingPairSet getSiblingLeafPairs();
		std::pair<int, SiblingPair> getOneSiblingPair(int startIndex = 0);

		void setRoots(std::unordered_set<TreeNode*> newRoots);
		void setLeaves(std::unordered_set<TreeNode*> newLeaves);
		void setLeavesByLabel(std::unordered_map<std::string, TreeNode*> newLeavesByLabel);
		void setNodesByCps(std::unordered_map<uint64_t, TreeNode*> newNodesByCps);
};
