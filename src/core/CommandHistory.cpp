#include "core/CommandHistory.h"

namespace smidr {

const std::string CommandHistory::kEmpty;

void CommandHistory::execute(std::unique_ptr<Command> cmd) {
    cmd->execute();
    stack_.erase(stack_.begin() + static_cast<ptrdiff_t>(pos_), stack_.end());
    stack_.push_back(std::move(cmd));
    pos_ = stack_.size();
}

bool CommandHistory::can_undo() const { return pos_ > 0; }
bool CommandHistory::can_redo() const { return pos_ < stack_.size(); }

void CommandHistory::undo() {
    if (!can_undo()) return;
    --pos_;
    stack_[pos_]->undo();
}

void CommandHistory::redo() {
    if (!can_redo()) return;
    stack_[pos_]->execute();
    ++pos_;
}

const std::string& CommandHistory::undo_description() const {
    return can_undo() ? stack_[pos_ - 1]->description() : kEmpty;
}

const std::string& CommandHistory::redo_description() const {
    return can_redo() ? stack_[pos_]->description() : kEmpty;
}

void CommandHistory::clear() {
    stack_.clear();
    pos_ = 0;
}

}  // namespace smidr
