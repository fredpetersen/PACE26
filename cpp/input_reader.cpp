#include <input_reader.h>

#include <cctype>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <problem_instance.h>
#include <tree_node.h>
#include <forest.h>

std::pair<std::shared_ptr<TreeNode>, std::unordered_set<std::shared_ptr<TreeNode>>> NewickParser::parseTree(std::unordered_map<int, std::shared_ptr<TreeNode>>& leafByLabel) {
    std::unordered_set<std::shared_ptr<TreeNode>> leaves;
    auto root = parseSubtree(leaves, leafByLabel);
    expect(';');
    if (position_ != text_.size()) {
        throw ParseError("Unexpected trailing characters after semicolon");
    }
    return {root, leaves};
}

std::string_view text_;
std::size_t position_ = 0;

std::shared_ptr<TreeNode> NewickParser::parseSubtree(std::unordered_set<std::shared_ptr<TreeNode>>& leaves, std::unordered_map<int, std::shared_ptr<TreeNode>>& leafByLabel) {
    if (position_ >= text_.size()) {
        throw ParseError("Unexpected end of input while parsing subtree");
    }

    char current = text_[position_];
    if (current == '(') {
        ++position_;
        auto left = parseSubtree(leaves, leafByLabel);
        expect(',');
        auto right = parseSubtree(leaves, leafByLabel);
        expect(')');
        auto node = std::make_shared<TreeNode>();
        node->left = left;
        node->left->parent = node;
        node->right = right;
        node->right->parent = node;

        if (left->isLeaf && right->isLeaf) {
            setCantorHashOfNode(node);
        }
        return node;
    }

    if (!std::isdigit(static_cast<unsigned char>(current))) {
        throw ParseError("Expected digit while parsing leaf label");
    }

    int label = parseNumber();
    auto node = std::make_shared<TreeNode>();
    node->isLeaf = true;
    node->label = label;
    setCantorHashOfNode(node);
    if (leafByLabel.count(label)) {
        throw ParseError("Duplicate leaf label: " + std::to_string(label));
    }
    leafByLabel[label] = node;
    leaves.insert(node);
    return node;
}

int NewickParser::parseNumber() {
    int value = 0;
    bool hasDigit = false;
    while (position_ < text_.size() && std::isdigit(static_cast<unsigned char>(text_[position_]))) {
        hasDigit = true;
        value = value * 10 + (text_[position_] - '0');
        ++position_;
    }
    if (!hasDigit) {
        throw ParseError("Expected numeric leaf label");
    }
    return value;
}

void NewickParser::expect(char expected) {
    if (position_ >= text_.size() || text_[position_] != expected) {
        throw ParseError(std::string("Expected '") + expected + "'");
    }
    ++position_;
}

void parseCountsLine(const std::string& line, Instance& instance, std::size_t lineNumber) {
    if (instance.hasCounts) {
        throw ParseError("Line " + std::to_string(lineNumber) + ": duplicate #p line");
    }
    std::istringstream iss(line.substr(2));
    if (!(iss >> instance.forestCount >> instance.leafCount)) {
        throw ParseError("Line " + std::to_string(lineNumber) + ": invalid #p line");
    }
    if (instance.forestCount <= 0 || instance.leafCount <= 0) {
        throw ParseError("Line " + std::to_string(lineNumber) + ": #p values must be positive");
    }
    instance.hasCounts = true;
}

void parseKeyValueLine(const std::string& line, std::unordered_map<std::string, std::string>& target,
                       std::size_t lineNumber, char directive) {
    std::istringstream iss(line.substr(2));
    std::string key;
    std::string value;
    if (!(iss >> key >> value)) {
        throw ParseError("Line " + std::to_string(lineNumber) + ": invalid #" + std::string(1, directive) + " line");
    }
    std::string extra;
    if (iss >> extra) {
        throw ParseError("Line " + std::to_string(lineNumber) + ": unexpected tokens in #" + std::string(1, directive) + " line");
    }
    if (target.count(key)) {
        throw ParseError("Line " + std::to_string(lineNumber) + ": duplicate key '" + key + "'");
    }
    target.emplace(std::move(key), std::move(value));
}

void collectLeaves(const TreeNode* node, std::vector<int>& leaves) {
    if (!node) {
        return;
    }
    if (node->isLeaf) {
        leaves.push_back(node->label);
        return;
    }
    collectLeaves(node->left.get(), leaves);
    collectLeaves(node->right.get(), leaves);
}

void validateTree(const TreeNode* root, int expectedLeafCount) {
    if (!root) {
        throw ParseError("Tree is empty");
    }

    std::vector<int> leaves;
    leaves.reserve(expectedLeafCount);
    collectLeaves(root, leaves);

    if (static_cast<int>(leaves.size()) != expectedLeafCount) {
        throw ParseError("Tree does not contain expected number of leaves");
    }

    std::vector<bool> seen(static_cast<std::size_t>(expectedLeafCount) + 1, false);
    for (int label : leaves) {
        if (label <= 0 || label > expectedLeafCount) {
            throw ParseError("Leaf label out of allowed range");
        }
        if (seen[static_cast<std::size_t>(label)]) {
            throw ParseError("Duplicate leaf label detected");
        }
        seen[static_cast<std::size_t>(label)] = true;
    }
}

Instance parseInput() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    Instance instance;
    std::string line;
    std::size_t lineNumber = 0;

    try {
        while (std::getline(std::cin, line)) {
            ++lineNumber;
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            if (line.empty()) {
                continue;
            }

            if (line.front() == '#') {
                if (line.size() < 2) {
                    continue;
                }
                char directive = line[1];
                switch (directive) {
                    case 'p':
                        parseCountsLine(line, instance, lineNumber);
                        break;
                    case 'x':
                        parseKeyValueLine(line, instance.parameters, lineNumber, directive);
                        break;
                    default:
                        break; // ignore other comment types
                }
                continue;
            }

            instance.rawTrees.push_back(line);
            instance.forestLineNumbers.push_back(lineNumber);
        }

        if (!instance.hasCounts) {
            throw ParseError("Missing #p line with tree and leaf counts");
        }
        if (static_cast<int>(instance.rawTrees.size()) != instance.forestCount) {
            throw ParseError("Tree count does not match #p directive");
        }

        instance.forests.reserve(instance.forestCount);
        for (std::size_t idx = 0; idx < instance.rawTrees.size(); ++idx) {
            const auto& newick = instance.rawTrees[idx];
            std::size_t forestLineNumber = instance.forestLineNumbers[idx];
            try {
                NewickParser parser(newick);
                auto leafByLabel = std::unordered_map<int, std::shared_ptr<TreeNode>>();
                auto parsed = parser.parseTree(leafByLabel);
                auto tree = parsed.first;
                auto leaves = std::move(parsed.second);
                validateTree(tree.get(), instance.leafCount);
                auto forest = std::make_shared<Forest>(std::unordered_set<std::shared_ptr<TreeNode>>{tree}, leaves, leafByLabel);
                instance.forests.push_back(std::make_shared<Forest>(*forest));
            } catch (const ParseError& err) {
                throw ParseError("Line " + std::to_string(forestLineNumber) + ": " + err.what());
            }
        }

    } catch (const ParseError& err) {
        std::cerr << "Parsing failed: " << err.what() << '\n';
        return {};
    }

    return instance;
}

// int main() {
//     auto instance = parseInput();

//     std::cout << "Parsed " << instance->trees.size() << " trees with "
//                 << instance->leafCount << " leaves each." << '\n';
//     if (!instance->parameters.empty()) {
//         std::cout << "Recorded #x parameters:" << '\n';
//         for (const auto& [key, value] : instance->parameters) {
//             std::cout << "  " << key << " = " << value << '\n';
//         }
//     }
//     if (!instance->systemValues.empty()) {
//         std::cout << "Recorded #s fields (ignored by solver):" << '\n';
//         for (const auto& [key, value] : instance->systemValues) {
//             std::cout << "  " << key << " = " << value << '\n';
//         }
//     }

//     return 1;
// }