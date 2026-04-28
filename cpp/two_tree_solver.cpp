#include <two_tree_solver.h>

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

void TwoTreeSolver::printForests() const {
    // TODO: Rework this to work actually good
    // forest1_->print("Forest 1");
    // forest2_->print("Forest 2");
    std::cout << "not implemented, printForests()" << std::endl;
}


void TwoTreeSolver::cleanSingletonLeaves(std::shared_ptr<Forest> mainForest, std::shared_ptr<Forest> otherForest) {
    std::unordered_set<std::shared_ptr<TreeNode>> newRoots;
    for (const auto& root : mainForest->getRoots()) {
        if (root->isLeaf) {
            otherForest->detachByLabel(root->label);
            continue;
        }
    }
}

std::vector<std::shared_ptr<Forest>> TwoTreeSolver::cloneForests(std::vector<std::shared_ptr<Forest>> forests) {
    std::vector<std::shared_ptr<Forest>> forestClones = {};
    for (const auto& forest : forests) {
        forestClones.push_back(forest->cloneForest());
    }
    return forestClones;
}

std::shared_ptr<Forest> TwoTreeSolver::solve() {
    bool isSolved = false;
    std::shared_ptr<Forest> solution;
    int k = 1;
    // No reason to add a looping condition on k, theres is always a trivial solution with k = nr. of leaves
    while (!isSolved) {

        auto res = solve(k++);
        isSolved = res.first;
        solution = res.second;
    }
    return solution;
}

std::pair<bool, std::shared_ptr<Forest>> TwoTreeSolver::solve(int k) {
    std::vector<std::shared_ptr<Forest>> forests = {forest1_, forest2_};
    auto res = solve(k, forests);
    return {res.first, res.second[0]};
}

std::pair<bool, std::vector<std::shared_ptr<Forest>>> TwoTreeSolver::solve(int k, std::vector<std::shared_ptr<Forest>> forests) {

    std::pair<bool, std::vector<std::shared_ptr<Forest>>> sol;

    auto fs = cloneForests(forests);
    auto f1 = fs[0];


    // technically a MAF on 0 forests is true...
    if (fs.size() < 1) {
        return {true, {nullptr}};
    }

    std::cout << "# Step 1" << std::endl;
    // Step 1 (if there's only 1 forest left, this forest is the MAF)
    if (fs.size() == 1) {
        return {true, fs};
    }

    auto f2 = fs[1];

    std::cout << "# Step 2" << std::endl;
    // Step 2 (if there are more components on the MAF we're building than k, the solution is too big (i.e. invalid))
    if (f1->getComponentCount() > k) {
        return {false, {nullptr}};
    }


    std::cout << "# Step 3" << std::endl;
    // Step 3 (clean the single vertex trees)
    cleanSingletonLeaves(f1, f2);
    cleanSingletonLeaves(f2, f1);


    std::cout << "# Step 4" << std::endl;
    // Step 4 (try to get a sibling pair in F2. If it can't be done, move on to solving F3)
    auto [idx, siblingPair] = f2->getOneSiblingPair();
    if (idx == -1) {
        fs.erase(fs.begin() + 1);
        f1->expandMergedSubtrees(); // Undoes the local merge (necessary to step through the algorithm)
        sol = solve(k, fs);
        if (sol.first) {
            return sol;
        }
    }

    std::cout << "# Step 5" << std::endl;
    // Step 5
    // labels of the nodes
    auto lab_u = siblingPair.first->label;
    auto lab_v = siblingPair.second->label;
    auto [ancestor, dist] = f1->lca(lab_u, lab_v);

    std::cout << "# Step 6" << std::endl;
    // Step 6 (when u and v are in different components in F1, branch on cutting either u or v)
    if (dist == -1) {
        // clone the forests to run 1 version on each branch
        auto fsClone = cloneForests(fs);
        auto fc1 = fsClone[0];
        auto fc2 = fsClone[1];

        std::cout << "# Step 6.1" << std::endl;
        // remember, f1 and f2 (from fs) is already a clone of the argument passed in
        f1->detachByLabel(lab_u);
        f2->detachByLabel(lab_u);
        sol = solve(k, fs);
        if (sol.first) {
            return sol;
        }

        std::cout << "# Step 6.2" << std::endl;
        fc1->detachByLabel(lab_v);
        fc2->detachByLabel(lab_v);
        sol = solve(k, fsClone);
        if (sol.first) {
            return sol;
        }
    }

    // Step 7 (when u and v are siblings in F1, do a local merge of u and v in both F1 and F2)
    else if (dist == 1) {
        std::cout << "# Step 7" << std::endl;
        f1->forestMergeCherry(f1->getLeafByLabel(lab_u)->parent);
        f2->forestMergeCherry(f2->getLeafByLabel(lab_u)->parent);
        sol = solve(k, fs);
        if (sol.first) {
            return sol;
        }
    }

    // Step 8 (when the distance between u and v in F1 is >= 2) // TODO: maybe case 2 (ie dist = 2) isnt a special case when there are n trees?
    else if (dist > 1){
        std::cout << "# Step 8" << std::endl;
        auto fsClone1 = cloneForests(fs);
        auto f1c1 = fsClone1[0];
        auto f2c1 = fsClone1[1];

        auto fsClone2 = cloneForests(fs);
        auto f1c2 = fsClone2[0];

        std::cout << "# Step 8.1" << std::endl;
        // branch 1, cut u from F1 and F2
        f1->detachByLabel(lab_u);
        f2->detachByLabel(lab_u);
        sol = solve(k, fs);
        if (sol.first) {
            return sol;
        }

        std::cout << "# Step 8.2" << std::endl;
        // branch 2, cut v from F1 and F2
        f1c1->detachByLabel(lab_v);
        f2c1->detachByLabel(lab_v);
        sol = solve(k, fsClone1);
        if (sol.first) {
            return sol;
        }

        std::cout << "# Step 8.3" << std::endl;
        // branch 3, cut all pendant subtrees between u and v from F1 only
        auto newRoots = f1c2->collectPendantSubtreesBetweenLeaves(lab_u, lab_v, ancestor);
        for (const auto & r : newRoots) {
            f1c2->detachChild(r);
        }
        sol = solve(k, fsClone2);
        if (sol.first) {
            return sol;
        }
    }
    std::cout << "# Dead End" << std::endl;
    return {false, {nullptr}};
}