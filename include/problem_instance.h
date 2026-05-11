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

    // For each leaf id i in [1..leafCount], the minimum depth of any TD bag
    // containing i (depth 0 = TD root). Empty if no #x treedecomp was supplied
    // or if parsing failed. Indexed by leaf id (so size = leafCount + 1, with
    // entry [0] unused). Unset entries default to INT_MAX so they sort last.
    std::vector<int> tdLeafDepth;
};
