#pragma once

#include <functional>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace smidr {

class App;

struct CommandInfo {
    std::string name;
    std::string usage;
    std::string help;
    std::function<void(App&, std::istringstream&)> handler;
    std::vector<std::string> aliases;
};

class CommandRegistry {
public:
    static CommandRegistry& instance();

    void register_command(CommandInfo info);

    bool execute(App& app, const std::string& input);

    std::vector<const CommandInfo*> list() const;
    const CommandInfo* find(const std::string& name) const;

    static void install_default_commands();

private:
    std::unordered_map<std::string, std::shared_ptr<CommandInfo>> by_name_;
    std::vector<std::shared_ptr<CommandInfo>> all_;
};

}  // namespace smidr
