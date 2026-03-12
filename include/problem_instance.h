#pragma once

#include <vector>
#include <memory>
#include <unordered_map>

#include <forest.h>

struct Instance {
    bool hasCounts = false;
    int forestCount = 0;
    int leafCount = 0;

    std::vector<std::string> rawTrees;
    std::vector<std::size_t> forestLineNumbers;
    std::vector<std::unique_ptr<Forest>> forests;
    std::unordered_map<std::string, std::string> parameters;
    std::unordered_map<std::string, std::string> systemValues;
};
