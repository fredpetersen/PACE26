#pragma once

#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <problem_instance.h>
#include <tree_node.h>
#include <forest.h>

struct ParseError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

class NewickParser {
public:
    inline NewickParser(std::string_view text) : text_(text) {}

    std::pair<std::shared_ptr<TreeNode>, std::unordered_set<std::shared_ptr<TreeNode>>> parseTree(std::unordered_map<int, std::shared_ptr<TreeNode>>& leafByLabel);

private:
    std::string_view text_;
    std::size_t position_ = 0;

    std::shared_ptr<TreeNode> parseSubtree(std::unordered_set<std::shared_ptr<TreeNode>>& leaves, std::unordered_map<int, std::shared_ptr<TreeNode>>& leafByLabel);

    int parseNumber();

    void expect(char expected);
};

void parseCountsLine(const std::string& line, Instance& instance, std::size_t lineNumber);

void parseKeyValueLine(const std::string& line, std::unordered_map<std::string, std::string>& target,
                       std::size_t lineNumber, char directive);

void collectLeaves(const TreeNode* node, std::vector<int>& leaves);

void validateTree(const TreeNode* root, int expectedLeafCount);

Instance parseInput();