#include "app/ScriptEngine.h"
#include "app/App.h"
#include "core/PrimitivesAdvanced.h"

#include <cctype>
#include <cstdlib>
#include <sstream>

namespace smidr {

Lexer::Lexer(const std::string& src) : src_(src) {}

Token Lexer::next() {
    while (pos_ < src_.size() && (src_[pos_] == ' ' || src_[pos_] == '\t')) ++pos_;
    if (pos_ < src_.size() && src_[pos_] == '#') {
        while (pos_ < src_.size() && src_[pos_] != '\n') ++pos_;
    }
    if (pos_ >= src_.size()) return {TokenType::EOF_Tok, "", 0.0, line_};

    char c = src_[pos_];
    if (c == '\n') { ++pos_; ++line_; return {TokenType::Newline, "\\n", 0.0, line_ - 1}; }

    if (std::isdigit(c) || (c == '.' && pos_+1 < src_.size() && std::isdigit(src_[pos_+1]))) {
        size_t s = pos_;
        while (pos_ < src_.size() && (std::isdigit(src_[pos_]) || src_[pos_] == '.' || src_[pos_] == '-')) ++pos_;
        Token t{TokenType::Number, src_.substr(s, pos_ - s), 0.0, line_};
        t.number = std::atof(t.text.c_str());
        return t;
    }
    if (std::isalpha(c) || c == '_') {
        size_t s = pos_;
        while (pos_ < src_.size() && (std::isalnum(src_[pos_]) || src_[pos_] == '_')) ++pos_;
        std::string id = src_.substr(s, pos_ - s);
        if (id == "var")   return {TokenType::KW_Var, id, 0.0, line_};
        if (id == "if")    return {TokenType::KW_If, id, 0.0, line_};
        if (id == "else")  return {TokenType::KW_Else, id, 0.0, line_};
        if (id == "for")   return {TokenType::KW_For, id, 0.0, line_};
        if (id == "while") return {TokenType::KW_While, id, 0.0, line_};
        return {TokenType::Identifier, id, 0.0, line_};
    }
    if (c == '"') {
        ++pos_;
        size_t s = pos_;
        while (pos_ < src_.size() && src_[pos_] != '"') ++pos_;
        std::string st = src_.substr(s, pos_ - s);
        if (pos_ < src_.size()) ++pos_;
        return {TokenType::String, st, 0.0, line_};
    }
    ++pos_;
    switch (c) {
        case '+': return {TokenType::Plus, "+", 0.0, line_};
        case '-': return {TokenType::Minus, "-", 0.0, line_};
        case '*': return {TokenType::Star, "*", 0.0, line_};
        case '/': return {TokenType::Slash, "/", 0.0, line_};
        case '%': return {TokenType::Percent, "%", 0.0, line_};
        case '(': return {TokenType::LParen, "(", 0.0, line_};
        case ')': return {TokenType::RParen, ")", 0.0, line_};
        case '{': return {TokenType::LBrace, "{", 0.0, line_};
        case '}': return {TokenType::RBrace, "}", 0.0, line_};
        case ',': return {TokenType::Comma, ",", 0.0, line_};
        case ';': return {TokenType::Semi, ";", 0.0, line_};
        case '=':
            if (pos_ < src_.size() && src_[pos_] == '=') { ++pos_; return {TokenType::EQ, "==", 0.0, line_}; }
            return {TokenType::Assign, "=", 0.0, line_};
        case '!':
            if (pos_ < src_.size() && src_[pos_] == '=') { ++pos_; return {TokenType::NE, "!=", 0.0, line_}; }
            break;
        case '<':
            if (pos_ < src_.size() && src_[pos_] == '=') { ++pos_; return {TokenType::LE, "<=", 0.0, line_}; }
            return {TokenType::LT, "<", 0.0, line_};
        case '>':
            if (pos_ < src_.size() && src_[pos_] == '=') { ++pos_; return {TokenType::GE, ">=", 0.0, line_}; }
            return {TokenType::GT, ">", 0.0, line_};
    }
    return {TokenType::EOF_Tok, "", 0.0, line_};
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> out;
    while (true) {
        Token t = next();
        out.push_back(t);
        if (t.type == TokenType::EOF_Tok) break;
    }
    return out;
}

class Parser {
public:
    Parser(std::vector<Token> tokens, ScriptInterpreter& interp)
        : tokens_(std::move(tokens)), interp_(interp) {}

    void parse_program() {
        while (!at_end()) {
            skip_newlines();
            if (at_end()) break;
            parse_statement();
        }
    }

private:
    std::vector<Token> tokens_;
    size_t pos_ = 0;
    ScriptInterpreter& interp_;

    bool at_end() const { return pos_ >= tokens_.size() || tokens_[pos_].type == TokenType::EOF_Tok; }
    const Token& peek() const { return tokens_[pos_]; }
    Token consume() { return tokens_[pos_++]; }
    bool match(TokenType t) {
        if (!at_end() && peek().type == t) { ++pos_; return true; }
        return false;
    }
    void skip_newlines() { while (!at_end() && (peek().type == TokenType::Newline || peek().type == TokenType::Semi)) ++pos_; }

    void parse_statement() {
        if (peek().type == TokenType::KW_Var) {
            consume();
            std::string name = consume().text;
            ScriptValue val;
            if (match(TokenType::Assign)) {
                val = parse_expr();
            }
            interp_.set_var(name, val);
        } else if (peek().type == TokenType::KW_For) {
            consume();
            match(TokenType::LParen);
            std::string var = consume().text;
            match(TokenType::Assign);
            double from = parse_expr().num;
            match(TokenType::Comma);
            double to = parse_expr().num;
            double step = 1.0;
            if (match(TokenType::Comma)) step = parse_expr().num;
            match(TokenType::RParen);
            match(TokenType::LBrace);
            skip_newlines();
            size_t body_start = pos_;
            for (double i = from; (step > 0 ? i < to : i > to); i += step) {
                pos_ = body_start;
                interp_.set_var(var, ScriptValue(i));
                while (!at_end() && peek().type != TokenType::RBrace) {
                    skip_newlines();
                    if (peek().type == TokenType::RBrace) break;
                    parse_statement();
                }
            }
            match(TokenType::RBrace);
        } else if (peek().type == TokenType::Identifier) {
            std::string name = consume().text;
            if (match(TokenType::Assign)) {
                ScriptValue v = parse_expr();
                interp_.set_var(name, v);
            } else if (peek().type == TokenType::LParen) {
                consume();
                std::vector<ScriptValue> args;
                if (peek().type != TokenType::RParen) {
                    args.push_back(parse_expr());
                    while (match(TokenType::Comma)) args.push_back(parse_expr());
                }
                match(TokenType::RParen);
                (void)interp_call_builtin(name, args);
            }
        } else {
            consume();
        }
    }

    ScriptValue parse_expr() { return parse_add(); }

    ScriptValue parse_add() {
        ScriptValue left = parse_mul();
        while (peek().type == TokenType::Plus || peek().type == TokenType::Minus) {
            TokenType op = consume().type;
            ScriptValue right = parse_mul();
            if (op == TokenType::Plus) left.num += right.num;
            else left.num -= right.num;
            left.type = ScriptValue::Number;
        }
        return left;
    }

    ScriptValue parse_mul() {
        ScriptValue left = parse_unary();
        while (peek().type == TokenType::Star || peek().type == TokenType::Slash || peek().type == TokenType::Percent) {
            TokenType op = consume().type;
            ScriptValue right = parse_unary();
            if (op == TokenType::Star) left.num *= right.num;
            else if (op == TokenType::Slash) left.num = right.num != 0 ? left.num / right.num : 0;
            else left.num = right.num != 0 ? std::fmod(left.num, right.num) : 0;
            left.type = ScriptValue::Number;
        }
        return left;
    }

    ScriptValue parse_unary() {
        if (peek().type == TokenType::Minus) {
            consume();
            ScriptValue v = parse_unary();
            v.num = -v.num;
            return v;
        }
        return parse_primary();
    }

    ScriptValue parse_primary() {
        const Token& t = peek();
        if (t.type == TokenType::Number) {
            consume();
            return ScriptValue(t.number);
        }
        if (t.type == TokenType::String) {
            consume();
            return ScriptValue(t.text);
        }
        if (t.type == TokenType::LParen) {
            consume();
            ScriptValue v = parse_expr();
            match(TokenType::RParen);
            return v;
        }
        if (t.type == TokenType::Identifier) {
            std::string name = consume().text;
            if (peek().type == TokenType::LParen) {
                consume();
                std::vector<ScriptValue> args;
                if (peek().type != TokenType::RParen) {
                    args.push_back(parse_expr());
                    while (match(TokenType::Comma)) args.push_back(parse_expr());
                }
                match(TokenType::RParen);
                return interp_call_builtin(name, args);
            }
            return interp_.get_var(name);
        }
        return ScriptValue();
    }

    ScriptValue interp_call_builtin(const std::string& name, const std::vector<ScriptValue>& args);
};

ScriptInterpreter::ScriptInterpreter(App& a) : app_(a) {}

ScriptValue ScriptInterpreter::get_var(const std::string& name) const {
    auto it = vars_.find(name);
    return it != vars_.end() ? it->second : ScriptValue();
}

void ScriptInterpreter::set_var(const std::string& name, ScriptValue v) {
    vars_[name] = v;
}

ScriptValue ScriptInterpreter::call_builtin(const std::string& name,
                                              const std::vector<ScriptValue>& args) {
    auto narg = [&](size_t i, double def = 0.0) -> double {
        return i < args.size() ? args[i].num : def;
    };

    if (name == "sphere" || name == "box" || name == "cylinder" || name == "cone" ||
        name == "torus" || name == "ellipsoid") {
        auto add_with_pos = [&](std::unique_ptr<Primitive> p, const std::string& base) {
            auto id = app_.document().scene().add_primitive(
                base + "." + std::to_string(app_.document().scene().nodes().size()),
                std::move(p));
            auto* n = app_.document().scene().find(id);
            if (n) {
                n->position = {static_cast<float>(narg(0)),
                                static_cast<float>(narg(1)),
                                static_cast<float>(narg(2))};
            }
            app_.document().mark_dirty();
            return ScriptValue(static_cast<double>(id));
        };
        if (name == "sphere") {
            auto p = std::make_unique<Sphere>();
            if (args.size() >= 4) p->radius = static_cast<float>(narg(3, 1.0));
            return add_with_pos(std::move(p), "Sphere");
        }
        if (name == "box") {
            auto p = std::make_unique<Box>();
            if (args.size() >= 6) p->half_extents = {static_cast<float>(narg(3)),
                                                       static_cast<float>(narg(4)),
                                                       static_cast<float>(narg(5))};
            return add_with_pos(std::move(p), "Box");
        }
        if (name == "cylinder") {
            auto p = std::make_unique<Cylinder>();
            if (args.size() >= 5) { p->radius = static_cast<float>(narg(3, 0.5)); p->height = static_cast<float>(narg(4, 2.0)); }
            return add_with_pos(std::move(p), "Cylinder");
        }
    }
    if (name == "print" || name == "echo") {
        std::string s;
        for (auto& a : args) {
            if (a.type == ScriptValue::String) s += a.str;
            else { char buf[64]; std::snprintf(buf, sizeof buf, "%g", a.num); s += buf; }
            s += " ";
        }
        app_.log(LogEntry::Info, s);
        return ScriptValue();
    }
    if (name == "sin") return ScriptValue(std::sin(narg(0)));
    if (name == "cos") return ScriptValue(std::cos(narg(0)));
    if (name == "sqrt") return ScriptValue(std::sqrt(narg(0)));
    if (name == "abs") return ScriptValue(std::abs(narg(0)));
    if (name == "pow") return ScriptValue(std::pow(narg(0), narg(1)));

    return ScriptValue();
}

ScriptValue Parser::interp_call_builtin(const std::string& name,
                                          const std::vector<ScriptValue>& args) {
    return interp_.call_builtin(name, args);
}

void ScriptInterpreter::execute(const std::string& source) {
    Lexer lex(source);
    auto tokens = lex.tokenize();
    Parser parser(std::move(tokens), *this);
    parser.parse_program();
}

bool ScriptInterpreter::execute_line(const std::string& line) {
    execute(line);
    return true;
}

}  // namespace smidr
