#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace smidr {

class Command {
public:
    virtual ~Command() = default;
    virtual void execute() = 0;
    virtual void undo() = 0;
    virtual std::string description() const = 0;
};

class CommandHistory {
public:
    void execute(std::unique_ptr<Command> cmd);
    bool can_undo() const;
    bool can_redo() const;
    void undo();
    void redo();

    const std::string& undo_description() const;
    const std::string& redo_description() const;

    void clear();
    int undo_count() const { return static_cast<int>(pos_); }
    int redo_count() const { return static_cast<int>(stack_.size()) - static_cast<int>(pos_); }

private:
    std::vector<std::unique_ptr<Command>> stack_;
    size_t pos_ = 0;
    static const std::string kEmpty;
};

}  // namespace smidr
