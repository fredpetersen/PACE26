#pragma once

#include <vector>
#include <memory>
#include <unordered_map>

#include <tree_node.h>

struct Instance {
    bool hasCounts = false;
    int treeCount = 0;
    int leafCount = 0;

    std::vector<std::string> rawTrees;
    std::vector<std::size_t> treeLineNumbers;
    std::vector<std::unique_ptr<TreeNode>> trees;
    std::unordered_map<std::string, std::string> parameters;
    std::unordered_map<std::string, std::string> systemValues;
};
