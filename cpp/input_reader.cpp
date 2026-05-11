#include <input_reader.h>

#include <cctype>
#include <iostream>
#include <limits>
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

std::pair<TreeNode*, std::unordered_set<TreeNode*>> NewickParser::parseTree(std::unordered_map<std::string, TreeNode*>& leafByLabel,
                                                                            std::unordered_map<uint64_t, TreeNode*>& nodeByCps,
                                                                            std::unordered_map<uint64_t, int>& cpsMap) {
    std::unordered_set<TreeNode*> leaves;
    auto root = parseSubtree(leaves, leafByLabel, nodeByCps, cpsMap);
    expect(';');
    if (position_ != text_.size()) {
        throw ParseError("Unexpected trailing characters after semicolon");
    }
    return {root, leaves};
}

TreeNode* NewickParser::allocNode() {
    arena_.emplace_back();
    return &arena_.back();
}

TreeNode* NewickParser::parseSubtree(std::unordered_set<TreeNode*>& leaves, std::unordered_map<std::string, TreeNode*>& leafByLabel,
                                     std::unordered_map<uint64_t, TreeNode*>& nodeByCps,
                                     std::unordered_map<uint64_t, int>& cpsMap) {
    if (position_ >= text_.size()) {
        throw ParseError("Unexpected end of input while parsing subtree");
    }

    char current = text_[position_];
    if (current == '(') {
        ++position_;
        auto left = parseSubtree(leaves, leafByLabel, nodeByCps, cpsMap);
        expect(',');
        auto right = parseSubtree(leaves, leafByLabel, nodeByCps, cpsMap);
        expect(')');
        auto node = allocNode();
        node->left = left;
        node->left->parent = node;
        node->right = right;
        node->right->parent = node;
        if (left->isLeaf && right->isLeaf) {
            node->setCps();
            auto h = node->cpsHash;
            nodeByCps[h] = node;
            if (cpsMap.find(h) == cpsMap.end()) {
                cpsMap[h] = 1;
            } else {
                cpsMap[h] += 1;
            }
        }

        return node;
    }

    if (!std::isdigit(static_cast<unsigned char>(current))) {
        throw ParseError("Expected digit while parsing leaf label");
    }

    std::string label = std::to_string(parseNumber());
    auto node = allocNode();
    node->isLeaf = true;
    node->label = label;
    if (leafByLabel.count(label)) {
        throw ParseError("Duplicate leaf label: " + label);
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
// Parses a #x treedecomp line of the form
//   #x treedecomp [W,[bag1,bag2,...],[edge1,edge2,...]]
// where each bag is "[id,id,id,...]" and each edge is "[i,j]" with 1-based bag
// indices. Populates `bags` (0-based) and `edges` (0-based pairs).
//
// Permissive scanner: skips brackets/commas/whitespace, reads ints; the first
// int read is W (discarded), then all subsequent ints are grouped into bags by
// the bracket nesting.
//
// Returns true on success, false if no parseable structure found.
bool parseTreeDecomp(const std::string& valueText,
                     std::vector<std::vector<int>>& bags,
                     std::vector<std::pair<int,int>>& edges) {
    bags.clear();
    edges.clear();
    // Two phases: phase 0 = bags, phase 1 = edges. Inner-bracket groups in
    // phase 0 are bags; in phase 1 they are 2-element edges. We detect the
    // transition by tracking depth: at depth 1, encountering a '[' starts an
    // inner group; we count how many top-level groups we've seen and treat the
    // last group at depth 1 as the edges section.

    // Build a token list of all inner groups (lists of ints) at depth 2.
    std::vector<std::vector<int>> innerGroups;
    std::vector<int> currentGroup;
    int depth = 0;
    bool sawW = false;
    int firstScalar = 0;
    bool inGroup = false;

    auto flushScalar = [&](int v) {
        if (inGroup) {
            currentGroup.push_back(v);
        } else if (!sawW) {
            firstScalar = v;
            sawW = true;
            (void)firstScalar; // discarded: it is the width / count
        }
    };

    std::size_t i = 0;
    while (i < valueText.size()) {
        char c = valueText[i];
        if (c == '[') {
            ++depth;
            if (depth == 2) {
                inGroup = true;
                currentGroup.clear();
            }
            ++i;
        } else if (c == ']') {
            if (depth == 2) {
                innerGroups.push_back(std::move(currentGroup));
                currentGroup.clear();
                inGroup = false;
            }
            --depth;
            ++i;
        } else if (std::isdigit(static_cast<unsigned char>(c)) || c == '-') {
            int sign = 1;
            if (c == '-') { sign = -1; ++i; }
            int v = 0;
            bool any = false;
            while (i < valueText.size() && std::isdigit(static_cast<unsigned char>(valueText[i]))) {
                v = v * 10 + (valueText[i] - '0');
                ++i;
                any = true;
            }
            if (any) flushScalar(sign * v);
        } else {
            ++i; // skip ',', whitespace, etc.
        }
    }

    if (innerGroups.empty()) return false;

    // Heuristic split: trailing pairs (size 2 with values that index a bag)
    // form the edges section; preceding groups are bags.
    std::size_t edgeStart = innerGroups.size();
    while (edgeStart > 0 && innerGroups[edgeStart - 1].size() == 2) {
        --edgeStart;
    }
    // If everything looked like edges (rare), assume no edges.
    if (edgeStart == 0) edgeStart = innerGroups.size();

    bags.reserve(edgeStart);
    for (std::size_t k = 0; k < edgeStart; ++k) {
        bags.push_back(std::move(innerGroups[k]));
    }
    for (std::size_t k = edgeStart; k < innerGroups.size(); ++k) {
        const auto& e = innerGroups[k];
        if (e.size() == 2) {
            edges.emplace_back(e[0], e[1]);
        }
    }
    return !bags.empty();
}

// BFS the TD (rooted at bag 0), and for each leaf id in [1..leafCount] set
// `out[id]` to the minimum depth (depth 0 = root bag) of any TD bag that
// contains it. Unmapped ids get INT_MAX.
void computeTdLeafDepth(const std::vector<std::vector<int>>& bags,
                        const std::vector<std::pair<int,int>>& edges,
                        int leafCount,
                        std::vector<int>& out) {
    out.assign(static_cast<std::size_t>(leafCount) + 1, std::numeric_limits<int>::max());
    if (bags.empty()) return;

    // Build adjacency over bags (1-based indices in `edges`).
    std::vector<std::vector<int>> adj(bags.size());
    for (auto [a, b] : edges) {
        int ai = a - 1;
        int bi = b - 1;
        if (ai < 0 || bi < 0 || ai >= static_cast<int>(bags.size()) || bi >= static_cast<int>(bags.size())) {
            continue;
        }
        adj[ai].push_back(bi);
        adj[bi].push_back(ai);
    }

    std::vector<int> depth(bags.size(), -1);
    std::deque<int> q;
    depth[0] = 0;
    q.push_back(0);
    while (!q.empty()) {
        int u = q.front(); q.pop_front();
        for (int v : adj[u]) {
            if (depth[v] == -1) {
                depth[v] = depth[u] + 1;
                q.push_back(v);
            }
        }
    }

    for (std::size_t b = 0; b < bags.size(); ++b) {
        int d = depth[b];
        if (d < 0) continue;
        for (int id : bags[b]) {
            if (id >= 1 && id <= leafCount && d < out[id]) {
                out[id] = d;
            }
        }
    }
}
void collectLeaves(const TreeNode* node, std::vector<std::string>& leaves) {
    if (!node) {
        return;
    }
    if (node->isLeaf) {
        leaves.push_back(node->label);
        return;
    }
    collectLeaves(node->left, leaves);
    collectLeaves(node->right, leaves);
}

void validateTree(const TreeNode* root, int expectedLeafCount) {
    if (!root) {
        throw ParseError("Tree is empty");
    }

    std::vector<std::string> leaves;
    leaves.reserve(expectedLeafCount);
    collectLeaves(root, leaves);

    if (static_cast<int>(leaves.size()) != expectedLeafCount) {
        throw ParseError("Tree does not contain expected number of leaves");
    }

    std::vector<bool> seen(static_cast<std::size_t>(expectedLeafCount) + 1, false);
    for (std::string label : leaves) {
        if (stoi(label) <= 0 || stoi(label) > expectedLeafCount) {
            throw ParseError("Leaf label out of allowed range");
        }
        if (seen[static_cast<std::size_t>(stoi(label))]) {
            throw ParseError("Duplicate leaf label detected");
        }
        seen[static_cast<std::size_t>(stoi(label))] = true;
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
                    case 'x': {
                        // Detect "treedecomp" key and stash the raw value for
                        // structured parsing later. Other #x keys go through
                        // the simple key/value path.
                        std::istringstream peek(line.substr(2));
                        std::string key;
                        if (peek >> key && key == "treedecomp") {
                            // Stash the bracketed value (everything after the key) verbatim.
                            auto pos = line.find("treedecomp");
                            std::string value = (pos == std::string::npos)
                                ? std::string{}
                                : line.substr(pos + std::string("treedecomp").size());
                            // Trim leading whitespace.
                            std::size_t start = 0;
                            while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start]))) ++start;
                            instance.parameters.emplace("treedecomp", value.substr(start));
                        } else {
                            parseKeyValueLine(line, instance.parameters, lineNumber, directive);
                        }
                        break;
                    }
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
        auto cpsMap = std::unordered_map<uint64_t, int>();
        for (std::size_t idx = 0; idx < instance.rawTrees.size(); ++idx) {
            const auto& newick = instance.rawTrees[idx];
            std::size_t forestLineNumber = instance.forestLineNumbers[idx];
            try {
                NewickParser parser(newick, instance.nodeArena);
                auto leafByLabel = std::unordered_map<std::string, TreeNode*>();
                auto nodeByCps = std::unordered_map<uint64_t, TreeNode*>();
                auto parsed = parser.parseTree(leafByLabel, nodeByCps, cpsMap);
                auto tree = parsed.first;
                auto leaves = std::move(parsed.second);
                validateTree(tree, instance.leafCount);
                instance.forests.push_back(std::make_shared<Forest>(std::unordered_set<TreeNode*>{tree}, leaves, leafByLabel, nodeByCps));
            } catch (const ParseError& err) {
                throw ParseError("Line " + std::to_string(forestLineNumber) + ": " + err.what());
            }
        }
        instance.cpsMap = cpsMap;

        // If a tree decomposition was provided via #x treedecomp, compute the
        // per-leaf TD depth used for branching-order heuristics. Failures here
        // are non-fatal: the solver simply falls back to its default order.
        auto tdIt = instance.parameters.find("treedecomp");
        if (tdIt != instance.parameters.end()) {
            std::vector<std::vector<int>> bags;
            std::vector<std::pair<int,int>> edges;
            if (parseTreeDecomp(tdIt->second, bags, edges)) {
                computeTdLeafDepth(bags, edges, instance.leafCount, instance.tdLeafDepth);
            }
        }
    } catch (const ParseError& err) {
        std::cerr << "Parsing failed: " << err.what() << '\n';
        return {};
    }
    return instance;
}