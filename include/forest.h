#pragma once

#include <memory>
#include <unordered_set>

#include <tree_node.h>

struct Forest {
    std::unordered_set<std::unique_ptr<TreeNode>> roots;
	int componentCount;
};
