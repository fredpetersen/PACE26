#pragma once

#include <memory>
#include <stdexcept>
#include <string>

#include <utils.h>
#include <mutation_trail.h>

struct TreeNode : std::enable_shared_from_this<TreeNode> {
    bool isMerged = false;
    bool isLeaf = false;
    std::string cpsHash = "";
    std::string label = "0";

    TreeNode* parent = nullptr;
    std::shared_ptr<TreeNode> left;
    std::shared_ptr<TreeNode> right;

    void setCps();
    bool isCpsNode();

    std::shared_ptr<TreeNode> parentShared() const;
    std::shared_ptr<TreeNode> self();
};

void localMergeCherry(std::shared_ptr<TreeNode> node);
void globalMergeCherry(std::shared_ptr<TreeNode> node, MutationTrail* trail = nullptr);