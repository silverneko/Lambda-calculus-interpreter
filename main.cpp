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
#include <readline/history.h>
#include "Parsers.hpp"

using namespace std;

using Parsers::Parser;
using Parsers::satisfy;
using Parsers::anyChar;
using Parsers::spaces;
using Parsers::oneOf;
using Parsers::digit;

const Parser alpha(satisfy([](char c){
  if(isgraph(c) && c != '(' && c != ')' && c != '\\'){
    return true;
  }
  return false;
}));

const Parser integer(maybe(oneOf("+-")) >> +digit);
const Parser identifier(+alpha);
const Parser lambda('\\');
const Parser leftBracket('(');
const Parser rightBracket(')');
const Parser panic(anyChar[ ([](const string& str){ cerr << "[Lex] Unexpected input: " << str << endl;}) ]);

class Token{
  public:
    enum Type{Undefined, Identifier, Integer, Lambda, LeftBracket, RightBracket, EndOfFile} type;
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
  return (/*integer[f(Token::Integer)]
      |*/ identifier[f(Token::Identifier)]
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
    enum Type{Nothing, Var, Constant, Lambda, Ap} type;

    int val;
    string name;
    shared_ptr<Expression> body, arg;

    Expression() : type(Nothing), name(), body(), arg() {}
    Expression(Type t) : type(t), name(), body(), arg() {}
    Expression(const Expression& expr) : type(expr.type), val(expr.val), name(expr.name), body(expr.body), arg(expr.arg) {}

    bool isVar() const { return type == Var;}
    bool isLam() const { return type == Lambda;}
    bool isAp() const { return type == Ap;}
    bool isNum() const { return type == Constant;}

    void print() const ;
    void prettyPrint() const ;
};

int fromString(const string& str){
  int sign = 1;
  int i = 0;
  if(str[0] == '+'){
    ++i;
  }else if(str[0] == '-'){
    ++i;
    sign = -1;
  }
  int res = 0;
  for(; i < str.size(); ++i){
    res = res * 10 + str[i] - '0';
  }
  return sign * res;
}

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

    case Token::Integer:
      expr.reset(new Expression(Expression::Constant));
      expr->name = token.name;
      expr->val = fromString(token.name);
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

/* Data structures
 *
 * data Expression = Var | Lambda | Ap
 *
 * data Object  = Callable Expression
 * data Context = Map Var/String Object
 * normalForm :: Context -> Expression -> Object
 * weakNormalForm :: Context -> Expression -> Object
 *
 * type Primitive = Var -> Expression
 * data Object = Object Expression Context | Primitive Context
 * data Context = [(Var, Object)]
 *
 * weakNormalForm :: Object -> Object
 * normalForm :: Object -> Object
 *
 * weakNormalForm / normalForm morphs a object (beta reduction)
 *
 */

class Object;
class Context;

class Context{
    unordered_map<string, shared_ptr<Object>> _env;
  public:
    Context() {}
    Context(const Context& context) : _env(context._env) {}

    Context& add(const string&, const string&);
    Context insert(const string&, const Object&) const;
    Context erase(const string&) const;
    Object& lookup(const string&) const;
    Object& operator [] (const string&) const;
    bool exist(const string&) const;
};

class Object{
  public:
    using Func = function<Object(const Context&, const Expression&)>;
    enum Type{Primitive, Ordinary};
  private:
    Type _type;
    Context _env;
    Expression _expr;
    Func _func;
  public:
    Object(const Expression& expr) : _type(Ordinary), _expr(expr) {}
    Object(const Context& env, const Expression& expr) : _type(Ordinary), _env(env), _expr(expr) {}
    Object(const Func& func) : _type(Primitive), _func(func) {}

    const Expression& expr() const { return _expr;}
    const Context& env() const { return _env;}

    Object call(const Context& env, const Expression& expr) const;
    bool callable() const {
      if(type() == Primitive){
        /* TODO */
      }else{
        if(_expr.isLam()){
          return true;
        }else{
          return false;
        }
      }
    }
    Type type() const { return _type;}

    // friend Object weakNormalForm(Object&);
    // friend Object normalForm(Object&);
};

Context& Context::add(const string& name, const string& rule) {
  Scanner scanner(rule);
  auto expr = parseExpression(scanner);
  _env[name].reset(new Object(*this, *expr));
  return *this;
}

Context Context::insert(const string& str, const Object& obj) const {
  Context env(*this);
  env._env[str].reset(new Object(obj));
  return env;
}

Context Context::erase(const string& str) const {
  Context env(*this);
  env._env.erase(str);
  return env;
}

Object& Context::lookup(const string& str) const {
  return (*this)[str];
}

Object& Context::operator [] (const string& str) const {
  auto it = _env.find(str);
  if(it == _env.cend()){
    cerr << "[Context lookup] Unbounded variable: " << str << endl;
    exit(1);
  }
  return *(it->second);
}

bool Context::exist(const string& str) const {
  if(_env.count(str) > 0) return true;
  return false;
}

Object weakNormalForm(const Context& env, const Expression& expr);
Object normalForm(const Context& env, const Expression& expr);

Object weakNormalForm(Object& obj){
  if(obj.type() == Object::Primitive){
    /* TODO */
  }else{
    obj = weakNormalForm(obj.env(), obj.expr());
    return obj;
  }
}

Object normalForm(Object& obj){
  if(obj.type() == Object::Primitive){
    /* TODO */
  }else{
    obj = normalForm(obj.env(), obj.expr());
    return obj;
  }
}

Object Object::call(const Context& env, const Expression& expr) const {
  if(type() == Primitive){
    /* TODO */
  }else{
    if(_expr.isLam()){
      return Object(_env.insert(_expr.name, Object(env, expr)), *_expr.body);
    }else{
      cerr << "[Object call] Object not callable (not a `Lambda`): " << _expr.type << endl;
      exit(1);
    }
  }
}

Object weakNormalForm(const Context& env, const Expression& expr){
  if( expr.isVar() ){
    if( env.exist(expr.name) ){
      return weakNormalForm(env[expr.name]);
    }else{
      return Object(expr);
    }
  }else if( expr.isLam() ){
    return Object(env, expr);
  }else if( expr.isAp() ){
    Object callee(weakNormalForm(env, *expr.body));
    if(callee.callable()){
      auto res = callee.call(env, *expr.arg);
      return weakNormalForm(res);
    }else{
      Expression expr2(Expression::Ap);
      expr2.body.reset(new Expression(callee.expr()));
      expr2.arg.reset(new Expression(normalForm(env, *expr.arg).expr()));
      return Object(env, expr2);
    }
  }
}

Object normalForm(const Context& env, const Expression& expr){
  if( expr.isVar() ){
    if( env.exist(expr.name) ){
      return normalForm(env[expr.name]);
    }else{
      return Object(expr);
    }
  }else if( expr.isLam() ){
    Expression expr2(expr);
    expr2.body.reset( new Expression(normalForm(env.erase( expr.name ), *expr.body).expr()) );
    return Object(env, expr2);
  }else if( expr.isAp() ){
    Object callee(weakNormalForm(env, *expr.body));
    if(callee.callable()){
      auto res = callee.call(env, *expr.arg);
      return normalForm(res);
    }else{
      Expression expr2(Expression::Ap);
      expr2.body.reset(new Expression(callee.expr()));
      expr2.arg.reset(new Expression(normalForm(env, *expr.arg).expr()));
      return Object(env, expr2);
    }
  }
}

int main(int argc, char *argv[])
{
  using_history();


  Context prelude;

  prelude.add("bool", "\\x x true false");
  prelude.add("true", "\\a \\b a");
  prelude.add("false", "\\a \\b b");
  prelude.add("if", "\\pred \\then \\else pred then else");
  prelude.add("not", "\\x x false true");
  prelude.add("and", "\\x \\y x y false");
  prelude.add("or", "\\x \\y x true y");
  prelude.add("Y", "\\f (\\x f (x x)) (\\x f (x x))");


  int bracketCount = 0;
  bool allSpace = true;
  string rawInput;
  while(true){
    char *_input = readline(allSpace ? "Prelude> " : "Prelude| ");
    if(_input == nullptr){
      cout << endl;
      break;
      continue;
    }
    string input(_input);
    free(_input);

    rawInput += input;
    rawInput += "\n";

    for(auto it = input.begin(); it != input.end(); ++it){
      if(*it == '('){
        ++bracketCount;
      }else if(*it == ')'){
        --bracketCount;
      }
      if(*it != ' ' && *it != '\t' && *it != '\n'){
        allSpace = false;
        add_history(input.c_str());
      }
    }

    if(bracketCount == 0){
      if(allSpace){
        rawInput = "";
        continue;
      }

      Scanner scanner(rawInput);
      rawInput = "";
      allSpace = true;

      shared_ptr<Expression> expr = parseExpression(scanner);

      /*
      expr->print();
      cout << endl;
      cout << endl;
      */

      normalForm(prelude, *expr).expr().prettyPrint();
      cout << endl;

    }
  }

  cout << "Goodbye." << endl;
  return 0;
}

void Expression::print() const {
  switch(type){
    case Nothing:
      cerr << "[Expression] Unexpected expression type: Nothing" << endl;
      break;

    case Constant:
      cout << "[\"int\"," << val << "]";
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
  switch(type){
    case Var:
      cout << name;
      return ;

    case Constant:
      cout << val;
      return ;

    case Lambda:
      cout << "\\" << name << " ";
      body->prettyPrint();
      return ;

    case Ap:
      if(body->isLam() || body->isAp()) cout << "(";
      body->prettyPrint();
      if(body->isLam() || body->isAp()) cout << ")";
      cout << " ";
      if(arg->isLam() || arg->isAp()) cout << "(";
      arg->prettyPrint();
      if(arg->isLam() || arg->isAp()) cout << ")";
      return ;

    default:
      cerr << "[Expression] Unexpected expression type: Nothing" << endl;
  }
}
