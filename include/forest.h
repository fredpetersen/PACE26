#pragma once

#include <memory>
#include <unordered_set>

#include <tree_node.h>

struct Forest {
    std::unordered_set<std::shared_ptr<TreeNode>> roots;
	std::unordered_set<std::shared_ptr<TreeNode>> leaves;
	int componentCount;
};
