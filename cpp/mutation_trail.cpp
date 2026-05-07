#include <mutation_trail.h>

#include <forest.h>
#include <tree_node.h>

void MutationTrail::rollback(std::size_t cp) {
    while (entries_.size() > cp) {
        UndoEntry e = std::move(entries_.back());
        entries_.pop_back();
        apply(e);
    }
    // Trim lambdas_ that are now beyond any remaining Lambda entry. Keeping it
    // simple: if entries are empty we can clear lambdas. Otherwise leave them;
    // they are only invoked through indices already consumed.
    if (entries_.empty()) {
        lambdas_.clear();
    }
}

void MutationTrail::apply(UndoEntry& e) {
    switch (e.op) {
        case UndoOp::Lambda: {
            auto& fn = lambdas_[static_cast<std::size_t>(e.int_aux)];
            if (fn) {
                auto local = std::move(fn);
                local();
            }
            break;
        }

        case UndoOp::NodeParent:
            e.a->parent = e.node_aux;
            break;
        case UndoOp::NodeLeftSlot:
            e.a->left = std::move(e.shared_aux);
            break;
        case UndoOp::NodeRightSlot:
            e.a->right = std::move(e.shared_aux);
            break;
        case UndoOp::NodeLabelFlags:
            e.a->label = std::move(e.str_aux);
            e.a->isLeaf = e.bool_a;
            e.a->isMerged = e.bool_b;
            break;
        case UndoOp::NodeUnmergeCherry: {
            // Inverse of globalMergeCherry: restore internal-node state.
            TreeNode* node = e.a;
            auto& l = e.shared_aux;
            auto& r = e.shared_aux2;
            node->isLeaf = false;
            node->label = "0";
            node->left = l;
            node->right = r;
            if (l) l->parent = node;
            if (r) r->parent = node;
            break;
        }

        case UndoOp::ForestSiblingInsert:
            e.forest->siblingPairParents_.insert(std::move(e.shared_aux));
            break;
        case UndoOp::ForestSiblingErase:
            e.forest->siblingPairParents_.erase(e.shared_aux);
            break;
        case UndoOp::ForestRootsInsert:
            e.forest->roots_.insert(std::move(e.shared_aux));
            break;
        case UndoOp::ForestRootsErase:
            e.forest->roots_.erase(e.shared_aux);
            break;
        case UndoOp::ForestLeavesInsert:
            e.forest->leaves_.insert(std::move(e.shared_aux));
            break;
        case UndoOp::ForestLeavesErase:
            e.forest->leaves_.erase(e.shared_aux);
            break;
        case UndoOp::ForestLeafByLabelSet:
            e.forest->leafByLabel_[std::move(e.str_aux)] = std::move(e.shared_aux);
            break;
        case UndoOp::ForestLeafByLabelErase:
            e.forest->leafByLabel_.erase(e.str_aux);
            break;
        case UndoOp::ForestNodeByCpsSet:
            e.forest->nodeByCps_[std::move(e.str_aux)] = std::move(e.shared_aux);
            break;
        case UndoOp::ForestNodeByCpsErase:
            e.forest->nodeByCps_.erase(e.str_aux);
            break;
        case UndoOp::ForestComponentDelta:
            e.forest->componentCount_ += e.int_aux;
            break;
        case UndoOp::ForestComponentSet:
            e.forest->componentCount_ = e.int_aux;
            break;

        case UndoOp::CpsMapDelta:
            (*e.cpsMapPtr)[e.str_aux] += e.int_aux;
            break;

        case UndoOp::CpsReductionRestore: {
            // a is unused; nodes are held via shared_aux/2/3 to keep them alive.
            auto& l = e.shared_aux;
            auto& r = e.shared_aux2;
            auto& node = e.shared_aux3;

            e.forest->leaves_.insert(l);
            e.forest->leaves_.insert(r);
            e.forest->leaves_.erase(node);

            // Match the original (including its existing quirk on the merged label):
            e.forest->leafByLabel_.erase(node->label);
            e.forest->leafByLabel_[l->label] = l;
            e.forest->leafByLabel_[r->label] = r;

            e.forest->nodeByCps_[node->label] = node;
            break;
        }
    }
}
