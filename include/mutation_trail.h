#pragma once

#include <functional>
#include <unordered_set>
#include <unordered_map>
#include <iostream>
#include <string>
#include <memory>
#include <vector>

class MutationTrail {
public:
	using UndoAction = std::function<void()>;

	std::size_t checkpoint() const {
		return actions_.size();
	}

	void record(UndoAction action) {
		actions_.push_back(std::move(action));
	}

	void rollback(std::size_t checkpoint) {
		while (actions_.size() > checkpoint) {
			auto action = std::move(actions_.back());
			actions_.pop_back();
			action();
		}
	}

private:
	std::vector<UndoAction> actions_;
};
