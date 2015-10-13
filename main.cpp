#include <cstdlib>
#include <iostream>
#include <string>
#include <algorithm>
#include <functional>
#include <vector>
#include <deque>
#include <map>
#include <unordered_map>
#include <memory>

#include <cstdio>
#include <readline/readline.h>
#include "Parsers.hpp"

using namespace std;

using Parsers::Parser;
using Parsers::satisfy;
using Parsers::anyChar;
using Parsers::spaces;

const Parser alpha(satisfy([](char c){
  if(isgraph(c) && c != '(' && c != ')' && c != '\\'){
    return true;
  }
  return false;
}));

const Parser identifier(+alpha);
const Parser lambda('\\');
const Parser leftBracket('(');
const Parser rightBracket(')');
const Parser panic(anyChar[ ([](const string& str){ cerr << "[Lex] Unexpected input: " << str << endl;}) ]);

class Token{
  public:
    enum Type{Undefined, Identifier, Lambda, LeftBracket, RightBracket, EndOfFile} type;
    std::string name;

    Token() : type(Undefined), name() {}
    Token(const string& str) : type(Undefined), name(str) {}
    Token(Type t) : type(t), name() {}
    Token(Type t, const string& str) : type(t), name(str) {}
};

Parser tokenParser(Token& token){
  auto f = [&token](Token::Type t){
    return [&token, t](const string& str){ token = Token(t, str);};
  };
  return (identifier[f(Token::Identifier)]
      | lambda[f(Token::Lambda)]
      | leftBracket[f(Token::LeftBracket)]
      | rightBracket[f(Token::RightBracket)]
      | panic[f(Token::Undefined)]
      );
}

class Scanner{
    deque<Token> tokens;
  public:
    Scanner(const string& buffer){
      auto first = buffer.begin(), last = buffer.end();
      Token token;
      while(first != last){
        tie(ignore, first) = spaces.runParser(first, last);
        if(first == last) break;
        tie(ignore, first) = tokenParser(token).runParser(first, last);
        tokens.push_back(token);
      }
    }

    Token getToken(){
      if(tokens.empty()) return Token(Token::EndOfFile, "EOF");
      Token token = tokens.front();
      tokens.pop_front();
      return token;
    }

    Token peekToken(){
      if(tokens.empty()) return Token(Token::EndOfFile, "EOF");
      return tokens.front();
    }

    void ungetToken(const Token& token){
      if(token.type == Token::EndOfFile) return ;
      tokens.push_front(token);
    }

    bool eof(){
      return tokens.empty();
    }
};

class Expression{
  public:
    enum Type{Nothing, Var, Lambda, Ap} type;
    string name;
    shared_ptr<Expression> body, arg;

    Expression() : type(Nothing), name(), body(), arg() {}
    Expression(Type t) : type(t), name(), body(), arg() {}
    Expression(const Expression& expr) : type(expr.type), name(expr.name), body(expr.body), arg(expr.arg) {}

    bool isVar() const { return type == Var;}
    bool isLam() const { return type == Lambda;}
    bool isAp() const { return type == Ap;}
    void print() const ;
    void prettyPrint() const ;
};

shared_ptr<Expression> parseExpression(Scanner&);
shared_ptr<Expression> parseExpressionTail(Scanner&);

shared_ptr<Expression> parseExpression(Scanner &scanner){
  shared_ptr<Expression> expr(parseExpressionTail(scanner));
  while(true){
    switch(scanner.peekToken().type){
      case Token::RightBracket:
      case Token::EndOfFile:
        return expr;

      default: break;
    }
    shared_ptr<Expression> expr1(new Expression(Expression::Ap));
    expr1->body = expr;
    expr1->arg  = parseExpressionTail(scanner);
    expr = expr1;
  }
}

shared_ptr<Expression> parseExpressionTail(Scanner &scanner){
  Token token = scanner.getToken();
  shared_ptr<Expression> expr(nullptr);
  switch(token.type){
    case Token::Lambda:
      token = scanner.getToken();
      if(token.type != Token::Identifier){
        cerr << "[Parse] Expected an identifier: " << token.name << endl;
        exit(1);
      }
      expr.reset(new Expression(Expression::Lambda));
      expr->name = token.name;
      expr->body = parseExpression(scanner);
      return expr;

    case Token::Identifier:
      expr.reset(new Expression(Expression::Var));
      expr->name = token.name;
      return expr;

    case Token::LeftBracket:
      expr = parseExpression(scanner);
      token = scanner.getToken();
      if(token.type != Token::RightBracket){
        cerr << "[Parse] Expected a `)`: " << token.name << endl;
        exit(1);
      }
      return expr;

    case Token::RightBracket:
    case Token::EndOfFile:
    default:
      cerr << "[Parse] Unexpected token: " << token.name << endl;
      exit(1);
  }
}

class Context;
using Thunk = tuple<shared_ptr<Expression>, shared_ptr<Context>>;

class Context{
    unordered_map<string, Thunk> _env;
  public:
    Context() : _env() {}
    Context(const Context& context) : _env(context._env) {}
    Context(const unordered_map<string, Thunk> &env) : _env(env) {}

    shared_ptr<Context> insert(const string&, const Thunk&) const;
    shared_ptr<Context> erase(const string&) const;
    Thunk lookup(const string&) const;
    Thunk operator [] (const string&) const;
    bool find(const string&) const;
};

shared_ptr<Context> Context::insert(const string& str, const Thunk& thunk) const {
  shared_ptr<Context> env(new Context(*this));
  env->_env[str] = thunk;
  return env;
}

shared_ptr<Context> Context::erase(const string& str) const {
  shared_ptr<Context> env(new Context(*this));
  env->_env.erase(str);
  return env;
}

Thunk Context::lookup(const string& str) const {
  auto it = _env.find(str);
  if(it == _env.cend()){
    cerr << "[Context lookup] Unbounded variable: " << str << endl;
    exit(1);
  }
  return it->second;
}

Thunk Context::operator [] (const string& str) const {
  return lookup(str);
}

bool Context::find(const string& str) const {
  if(_env.count(str) > 0) return true;
  return false;
}

Thunk normalForm(Thunk);

Thunk weakNormalForm(Thunk thunk){
  auto expr = get<0>(thunk);
  auto env  = get<1>(thunk);
  if(expr->isLam()){
    return thunk;
  }else if(expr->isVar()){
    if(env->find(expr->name)){
      return weakNormalForm(env->lookup(expr->name));
    }else{
      return thunk;
    }
  }else{
    shared_ptr<Expression> expr2;
    shared_ptr<Context> env2;
    tie(expr2, env2) = weakNormalForm(Thunk(expr->body, env));
    if( !expr2->isLam() ){
      shared_ptr<Expression> expr3(new Expression(Expression::Ap));
      expr3->body = expr2;
      tie(expr3->arg, ignore)  = normalForm(Thunk(expr->arg, env));
      return Thunk(expr3, env);
    }
    return weakNormalForm(Thunk(expr2->body, env2->insert(expr2->name, Thunk(expr->arg, env))));
  }
}

Thunk normalForm(Thunk thunk){
  auto expr = get<0>(thunk);
  auto env  = get<1>(thunk);
  if(expr->isLam()){
    shared_ptr<Expression> expr2(new Expression(*expr));
    // Unbound variable
    tie(expr2->body, ignore) = normalForm(Thunk(expr->body, env->erase(expr->name)));
    return Thunk(expr2, env);
  }else if(expr->isVar()){
    if(env->find(expr->name)){
      return normalForm(env->lookup(expr->name));
    }else{
      return thunk;
    }
  }else{
    shared_ptr<Expression> expr2;
    shared_ptr<Context> env2;
    tie(expr2, env2) = weakNormalForm(Thunk(expr->body, env));
    if( !expr2->isLam() ){
      shared_ptr<Expression> expr3(new Expression(Expression::Ap));
      expr3->body = expr2;
      tie(expr3->arg, ignore)  = normalForm(Thunk(expr->arg, env));
      return Thunk(expr3, env);
    }
    return normalForm(Thunk(expr2->body, env2->insert(expr2->name, Thunk(expr->arg, env))));
  }
}

int main(int argc, char *argv[])
{
  string rawInput;
  while(true){
    char *input = readline("Prelude> ");
    if(input == nullptr){
      cout << endl;
      break;
    }
    rawInput += input;
    rawInput += "\n";
    free(input);
  }

  Scanner scanner(rawInput);

  shared_ptr<Expression> expr = parseExpression(scanner);
  shared_ptr<Context> env(new Context);
  Thunk thunk(expr, env);

  expr->print();
  cout << endl;
  cout << endl;

  get<0>(normalForm(thunk))->prettyPrint();
  cout << endl;
  cout << endl;
  get<0>(normalForm(thunk))->print();
  cout << endl;

  cout << "\nLeaving main\n";
  return 0;
}

void Expression::print() const {
  switch(type){
    case Nothing:
      cerr << "[Expression] Unexpected expression type: Nothing" << endl;
      break;

    case Var:
      cout << "[\"var\",\"" << name << "\"]";
      break;

    case Lambda:
      cout << "[\"lam\",\"" << name << "\",";
      body->print();
      cout << "]";
      break;

    case Ap:
      cout << "[\"app\",";
      body->print();
      cout << ",";
      arg->print();
      cout << "]";
      break;

  }
}

void Expression::prettyPrint() const {
  if(isVar()){
    cout << name;
  }else if(isLam()){
    cout << "\\" << name << " ";
    body->prettyPrint();
  }else if(isAp()){
    body->prettyPrint();
    cout << " (";
    arg->prettyPrint();
    cout << ")";
  }else{
    cerr << "[Expression] Unexpected expression type: Nothing" << endl;
  }
}
