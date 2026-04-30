#pragma once

#include <memory>
#include <stdexcept>
#include <string>

struct TreeNode;

struct ParentLink {
    TreeNode* ptr = nullptr;

    ParentLink() = default;
    ParentLink(std::nullptr_t) : ptr(nullptr) {}
    ParentLink(TreeNode* raw) : ptr(raw) {}
    ParentLink(const std::shared_ptr<TreeNode>& owner) : ptr(owner.get()) {}

    ParentLink& operator=(TreeNode* raw) {
        ptr = raw;
        return *this;
    }

    ParentLink& operator=(const std::shared_ptr<TreeNode>& owner) {
        ptr = owner.get();
        return *this;
    }

    ParentLink& operator=(std::nullptr_t) {
        ptr = nullptr;
        return *this;
    }

    TreeNode* get() const {
        return ptr;
    }

    TreeNode* operator->() const {
        return ptr;
    }

    operator TreeNode*() const {
        return ptr;
    }

    bool operator==(std::nullptr_t) const {
        return ptr == nullptr;
    }

    bool operator!=(std::nullptr_t) const {
        return ptr != nullptr;
    }

    bool operator==(TreeNode* other) const {
        return ptr == other;
    }

    bool operator!=(TreeNode* other) const {
        return ptr != other;
    }
};

struct TreeNode {
    bool isMerged = false;
    bool isLeaf = false;
    std::string label = "0";
    ParentLink parent;
    std::shared_ptr<TreeNode> left;
    std::shared_ptr<TreeNode> right;
};

void mergeCherry(TreeNode* node);