#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace smidr {

class App;

enum class TokenType {
    Number, Identifier, String,
    Plus, Minus, Star, Slash, Percent,
    LParen, RParen, LBrace, RBrace, Comma, Semi, Assign,
    EQ, NE, LT, GT, LE, GE,
    KW_Var, KW_If, KW_Else, KW_For, KW_While,
    Newline, EOF_Tok,
};

struct Token {
    TokenType type;
    std::string text;
    double number = 0.0;
    int line = 1;
};

class Lexer {
public:
    explicit Lexer(const std::string& src);
    std::vector<Token> tokenize();
private:
    const std::string& src_;
    size_t pos_ = 0;
    int line_ = 1;
    Token next();
};

struct ScriptValue {
    enum Type { Number, String, Null };
    Type type = Null;
    double num = 0.0;
    std::string str;

    ScriptValue() = default;
    ScriptValue(double n) : type(Number), num(n) {}
    ScriptValue(const std::string& s) : type(String), str(s) {}
};

class ScriptInterpreter {
public:
    explicit ScriptInterpreter(App& app);

    void execute(const std::string& source);
    bool execute_line(const std::string& line);

    ScriptValue get_var(const std::string& name) const;
    void set_var(const std::string& name, ScriptValue v);

    ScriptValue call_builtin(const std::string& name,
                              const std::vector<ScriptValue>& args);

private:
    App& app_;
    std::unordered_map<std::string, ScriptValue> vars_;
};

}  // namespace smidr
