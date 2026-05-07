#pragma once

#include <vector>
#include <memory>
#include <deque>
#include <unordered_map>

#include <forest.h>
#include <tree_node.h>

struct Instance {
    bool hasCounts = false;
    int forestCount = 0;
    int leafCount = 0;
    std::unordered_map<uint64_t, int> cpsMap;

    std::vector<std::string> rawTrees;
    std::vector<std::size_t> forestLineNumbers;
    std::vector<std::shared_ptr<Forest>> forests;
    std::deque<TreeNode> nodeArena;
    std::unordered_map<std::string, std::string> parameters;
    std::unordered_map<std::string, std::string> systemValues;
};
