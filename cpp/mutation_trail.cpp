#include <mutation_trail.h>

#include <forest.h>
#include <tree_node.h>

void MutationTrail::rollback(std::size_t cp) {
    while (entries_.size() > cp) {
        UndoEntry e = std::move(entries_.back());
        entries_.pop_back();
        apply(e);
    }
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
            e.a->left = e.t_aux;
            invalidateSubtreeHash(e.a);
            break;
        case UndoOp::NodeRightSlot:
            e.a->right = e.t_aux;
            invalidateSubtreeHash(e.a);
            break;
        case UndoOp::NodeLabelFlags:
            e.a->label = std::move(e.str_aux);
            e.a->isLeaf = e.bool_a;
            e.a->isMerged = e.bool_b;
            invalidateSubtreeHash(e.a);
            break;
        case UndoOp::NodeUnmergeCherry: {
            TreeNode* node = e.a;
            TreeNode* l = e.t_aux;
            TreeNode* r = e.t_aux2;
            node->isLeaf = false;
            node->label = "0";
            node->left = l;
            node->right = r;
            if (l) l->parent = node;
            if (r) r->parent = node;
            invalidateSubtreeHash(node);
            break;
        }

        case UndoOp::ForestSiblingInsert:
            e.forest->siblingPairParents_.insert(e.t_aux);
            break;
        case UndoOp::ForestSiblingErase:
            e.forest->siblingPairParents_.erase(e.t_aux);
            break;
        case UndoOp::ForestRootsInsert:
            e.forest->roots_.insert(e.t_aux);
            break;
        case UndoOp::ForestRootsErase:
            e.forest->roots_.erase(e.t_aux);
            break;
        case UndoOp::ForestLeavesInsert:
            e.forest->leaves_.insert(e.t_aux);
            break;
        case UndoOp::ForestLeavesErase:
            e.forest->leaves_.erase(e.t_aux);
            break;
        case UndoOp::ForestLeafByLabelSet:
            e.forest->leafByLabel_[std::move(e.str_aux)] = e.t_aux;
            break;
        case UndoOp::ForestLeafByLabelErase:
            e.forest->leafByLabel_.erase(e.str_aux);
            break;
        case UndoOp::ForestNodeByCpsSet:
            e.forest->nodeByCps_[e.u64_aux] = e.t_aux;
            break;
        case UndoOp::ForestNodeByCpsErase:
            e.forest->nodeByCps_.erase(e.u64_aux);
            break;
        case UndoOp::ForestComponentDelta:
            e.forest->componentCount_ += e.int_aux;
            break;
        case UndoOp::ForestComponentSet:
            e.forest->componentCount_ = e.int_aux;
            break;

        case UndoOp::CpsMapDelta:
            (*e.cpsMapPtr)[e.u64_aux] += e.int_aux;
            break;

        case UndoOp::CpsReductionRestore: {
            TreeNode* l = e.t_aux;
            TreeNode* r = e.t_aux2;
            TreeNode* node = e.t_aux3;

            e.forest->leaves_.insert(l);
            e.forest->leaves_.insert(r);
            e.forest->leaves_.erase(node);

            e.forest->leafByLabel_.erase(node->label);
            e.forest->leafByLabel_[l->label] = l;
            e.forest->leafByLabel_[r->label] = r;

            e.forest->nodeByCps_[e.u64_aux] = node;
            invalidateSubtreeHash(node);
            break;
        }
    }
}
