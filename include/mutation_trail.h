#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

class Forest;
struct TreeNode;

/*
 * Tagged-struct undo log.
 *
 * Replaces the previous std::function<void()>-only trail. Each entry is a fixed-size
 * record with a tag plus a small set of fields. This avoids per-record heap allocation
 * for closure storage and removes the indirect call through std::function on the hot
 * rollback path.
 *
 * Strings are kept inline (std::string SSO covers labels like "12" and short cps hashes
 * with zero heap allocs in libc++/libstdc++). Shared_ptr fields are still required for
 * restoring the owning left/right child slots on TreeNode until the planned raw-pointer
 * migration lands; even so, they are moved (not copied) into the entry where possible.
 *
 * A legacy std::function-based record(UndoAction) overload is preserved for any sites
 * not yet migrated.
 */
enum class UndoOp : uint8_t {
    Lambda,                 // legacy: invoke lambdas_[int_aux]

    // --- TreeNode field restores ---
    NodeParent,             // a->parent = node_aux
    NodeLeftSlot,           // a->left  = shared_aux
    NodeRightSlot,          // a->right = shared_aux
    NodeLabelFlags,         // a->label = str_aux; a->isLeaf = bool_a; a->isMerged = bool_b
    // For globalMergeCherry undo: restore a->isLeaf=false, a->label="0",
    // a->left=shared_aux (=l), a->right=shared_aux2 (=r), b->parent=a, c->parent=a
    NodeUnmergeCherry,

    // --- Forest container ops ---
    ForestSiblingInsert,    // forest->siblingPairParents_.insert(shared_aux)
    ForestSiblingErase,     // forest->siblingPairParents_.erase(shared_aux)
    ForestRootsInsert,      // forest->roots_.insert(shared_aux)
    ForestRootsErase,       // forest->roots_.erase(shared_aux)
    ForestLeavesInsert,     // forest->leaves_.insert(shared_aux)
    ForestLeavesErase,      // forest->leaves_.erase(shared_aux)
    ForestLeafByLabelSet,   // forest->leafByLabel_[str_aux] = shared_aux
    ForestLeafByLabelErase, // forest->leafByLabel_.erase(str_aux)
    ForestNodeByCpsSet,     // forest->nodeByCps_[str_aux] = shared_aux
    ForestNodeByCpsErase,   // forest->nodeByCps_.erase(str_aux)
    ForestComponentDelta,   // forest->componentCount_ += int_aux
    ForestComponentSet,     // forest->componentCount_  = int_aux

    // --- Solver / cpsMap ---
    CpsMapDelta,            // (*cpsMapPtr)[str_aux] += int_aux

    // --- Composite (preserves existing semantics for hot cpsReduction body) ---
    // a = mergedNode (raw), shared_aux=l, shared_aux2=r, shared_aux3=mergedNode.
    // Restores leaves_/leafByLabel_/nodeByCps_ for {l, r, mergedNode}.
    CpsReductionRestore,
};

struct UndoEntry {
    UndoOp op = UndoOp::Lambda;
    Forest* forest = nullptr;
    TreeNode* a = nullptr;
    TreeNode* node_aux = nullptr;                  // e.g., previous parent pointer
    std::shared_ptr<TreeNode> shared_aux;          // owning slot restore / set membership
    std::shared_ptr<TreeNode> shared_aux2;
    std::shared_ptr<TreeNode> shared_aux3;
    std::string str_aux;
    int int_aux = 0;
    bool bool_a = false;
    bool bool_b = false;
    std::unordered_map<std::string, int>* cpsMapPtr = nullptr;
};

class MutationTrail {
public:
    using UndoAction = std::function<void()>;

    MutationTrail() {
        entries_.reserve(1024);
    }

    std::size_t checkpoint() const {
        return entries_.size();
    }

    // Fast path: append a tagged entry.
    void record(UndoEntry&& e) {
        entries_.push_back(std::move(e));
    }

    // Legacy lambda-based record. Wraps the lambda via a Lambda-tagged sentinel entry.
    void record(UndoAction action) {
        UndoEntry e{};
        e.op = UndoOp::Lambda;
        e.int_aux = static_cast<int>(lambdas_.size());
        lambdas_.push_back(std::move(action));
        entries_.push_back(std::move(e));
    }

    void rollback(std::size_t cp);

private:
    std::vector<UndoEntry> entries_;
    std::vector<UndoAction> lambdas_;

    void apply(UndoEntry& e);
};

