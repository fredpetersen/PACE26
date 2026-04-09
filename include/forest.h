#pragma once

#include <memory>
#include <unordered_set>

#include <tree_node.h>

struct Forest {
	std::unordered_set<std::shared_ptr<TreeNode>> roots;
	std::unordered_set<std::shared_ptr<TreeNode>> leaves;
	int componentCount;
	std::unordered_map<int, std::shared_ptr<TreeNode>> leafByLabel;
};

static std::string treeToNewick(const std::shared_ptr<TreeNode>& node) {
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

static void printForestNewick(const Forest* forest) {
		for (const auto& root : forest->roots) {
				std::cout << treeToNewick(root) << ";\n";
		}
}
