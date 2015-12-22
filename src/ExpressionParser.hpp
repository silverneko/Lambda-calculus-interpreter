#ifndef __PARSE_ULC_EXPRESSION_HPP__
#define __PARSE_ULC_EXPRESSION_HPP__

#include <cstdlib>
#include <string>
#include <algorithm>
#include <functional>
#include <deque>
#include <memory>
#include <sstream>

class Token{
  public:
    enum Type{Undefined, Constant, Identifier, Keyword, Lambda, LeftBracket, RightBracket, EndOfFile} type;
    std::string name;

    Token() : type(Undefined), name() {}
    Token(const std::string& str) : type(Undefined), name(str) {}
    Token(Type t) : type(t), name() {}
    Token(Type t, const std::string& str) : type(t), name(str) {}
};

class Scanner{
  private:
    std::deque<Token> tokens;
  public:
    Scanner(const std::string&);

    Token getToken();
    Token peekToken();
    void ungetToken(const Token&);
    bool eof() const ;
};

class Expression{
  public:
    enum Type{Nothing, Var, Constant, Lambda, Ap} type;

    int val;
    std::string name;
    std::shared_ptr<Expression> body, arg;

    Expression() : type(Nothing), name(), body(), arg() {}
    Expression(Type t) : type(t), name(), body(), arg() {}
    Expression(int v) : type(Constant), val(v), name("Lit Int") {}
    Expression(const std::string& str) : type(Var), name(str) {}
    Expression(const Expression& expr) : type(expr.type), val(expr.val), name(expr.name), body(expr.body), arg(expr.arg) {}

    bool isVar() const { return type == Var;}
    bool isLam() const { return type == Lambda;}
    bool isAp() const { return type == Ap;}
    bool isNum() const { return type == Constant;}

    void print() const ;
    void prettyPrint() const ;
};

std::shared_ptr<Expression> parseExpression(Scanner&);

#endif

