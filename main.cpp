#include <cstdlib>
#include <iostream>
#include <string>
#include <algorithm>
#include <functional>
#include <vector>
#include <deque>
#include <memory>
#include <sstream>

#include <cstdio>

#include "Parsers.hpp"
#include "Dictionary.hpp"

using namespace std;

using Parsers::Parser;
using Parsers::satisfy;
using Parsers::anyChar;
using Parsers::spaces;
using Parsers::oneOf;
using Parsers::digit;
using Parsers::charp;

const Parser alpha(satisfy([](char c){
  if(isgraph(c) && c != '\\' && c != '(' && c != ')'){
    return true;
  }
  return false;
}));

const Parser integer(maybe(oneOf("+-")) >> +digit);
const Parser charLiteral(charp('\'') >> satisfy([](char c){ return isprint(c);}) >> charp('\''));
const Parser identifier(+alpha);
const Parser lambda('\\');
const Parser leftBracket('(');
const Parser rightBracket(')');
const Parser panic(anyChar[ ([](const string& str){ cerr << "[Lex] Unexpected input: " << str << endl;}) ]);

class Token{
  public:
    enum Type{Undefined, Constant, Identifier, Keyword, Lambda, LeftBracket, RightBracket, EndOfFile} type;
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
  auto g = [&token](const string& str){
    if(str == "let") token = Token(Token::Keyword, "let");
    else if(str == "in") token = Token(Token::Keyword, "in");
    else token = Token(Token::Identifier, str);
  };
  return (integer[f(Token::Constant)]
      | charLiteral[f(Token::Constant)]
      | identifier[g]
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

template<class T>
T stringTo(const string& str){
  istringstream iss(str);
  T res;
  iss >> res;
  return res;
}

template<class T>
string toString(const T& t){
  ostringstream oss;
  oss << t;
  return oss.str();
}

class Expression{
  public:
    enum Type{Nothing, Var, Constant, Lambda, Ap} type;

    int val;
    string name;
    shared_ptr<Expression> body, arg;

    Expression() : type(Nothing), name(), body(), arg() {}
    Expression(Type t) : type(t), name(), body(), arg() {}
    Expression(int v) : type(Constant), val(v), name(toString(v)) {}
    Expression(const string& str) : type(Var), name(str) {}
    Expression(const Expression& expr) : type(expr.type), val(expr.val), name(expr.name), body(expr.body), arg(expr.arg) {}

    bool isVar() const { return type == Var;}
    bool isLam() const { return type == Lambda;}
    bool isAp() const { return type == Ap;}
    bool isNum() const { return type == Constant;}

    void print() const ;
    void prettyPrint() const ;
};

shared_ptr<Expression> parseExpression(Scanner&);
shared_ptr<Expression> parseExpressionTail(Scanner&);

shared_ptr<Expression> parseExpression(Scanner &scanner){
  shared_ptr<Expression> expr(parseExpressionTail(scanner));
  if(expr == nullptr){
    cerr << "[Parse expression] Unexpected token: " << scanner.peekToken().name << endl;
    exit(1);
    return nullptr;
  }
  while(true){
    shared_ptr<Expression> expr1(parseExpressionTail(scanner));
    if(expr1 == nullptr){
      return expr;
    }else{
      shared_ptr<Expression> expr2(new Expression(Expression::Ap));
      expr2->body = expr;
      expr2->arg  = expr1;
      expr = expr2;
    }
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

    case Token::Keyword:
      if(token.name == "$"){
        return parseExpression(scanner);
      }else if(token.name == "let"){
        token = scanner.getToken();
        if(token.type != Token::Identifier){
          cerr << "[Parse] Expected an identifier: " << token.name << endl;
          exit(1);
        }
        expr.reset(new Expression(Expression::Ap));
        expr->body.reset(new Expression(Expression::Lambda));
        expr->body->name = token.name;
        expr->arg = parseExpression(scanner);
        token = scanner.getToken();
        if(token.type != Token::Keyword || token.name != "in"){
          cerr << "[Parse] Expected a keyword `in`: " << token.name << endl;
          exit(1);
        }
        expr->body->body = parseExpression(scanner);
        return expr;
      }else if(token.name == "in"){
        scanner.ungetToken(token);
        return nullptr;
      }

    case Token::Constant:
      expr.reset(new Expression(Expression::Constant));
      expr->name = token.name;
      if(token.name[0] == '\''){
        // character literal
        expr->val = (int)token.name[1];
      }else{
        expr->val = stringTo<int>(token.name);
      }
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
      scanner.ungetToken(token);
      return nullptr;

    case Token::EndOfFile:
    default:
      return nullptr;
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
    Dictionary<shared_ptr<Object>> _env;
  public:
    Context() {}
    Context(const Context& context) : _env(context._env) {}
    Context(const Dictionary<shared_ptr<Object>>& d) : _env(d) {}

    Context& add(const string&, const string&);
    Context& add(const string&, const Object&);
    Context insert(const string&, const Object&) const;
    Context erase(const string&) const;
    Object& lookup(const string&) const;
    Object& operator [] (const string&) const;
    bool exist(const string&) const;
};

class Object{
  public:
    using Func = function<Object(const Expression&, const Context&)>;
    enum Type{Primitive, Closure, NormalForm};
  private:
    Type _type;
    Expression _expr;
    Context _env;
    Func _func;

    Object(Type t, const Expression& expr) : _type(t), _expr(expr) {}
  public:
    Object(const Expression& expr) : _type(Closure), _expr(expr), _env() {}
    Object(const Expression& expr, const Context& env) : _type(Closure), _expr(expr), _env(env) {}
    Object(const Func& func) : _type(Primitive), _func(func) {}

    const Expression& expr() const { return _expr;}
    const Context& env() const { return _env;}

    Object call(const Expression& expr, const Context& env) const;

    bool callable() const;

    Type type() const { return _type;}
    bool isNormalForm() const { return _type == NormalForm;}
    bool isPrimitive() const { return _type == Primitive;}

    friend Object makeNormalForm(const Expression&);
};

Context& Context::add(const string& name, const string& rule) {
  Scanner scanner(rule);
  auto expr = parseExpression(scanner);
  return add(name, Object(*expr, *this));
}

Context& Context::add(const string& name, const Object& rule){
  *this = _env.insert(name, shared_ptr<Object>(new Object(rule)));
  return *this;
}

Context Context::insert(const string& str, const Object& obj) const {
  return this->_env.insert(str, shared_ptr<Object>(new Object(obj)));
}

Context Context::erase(const string& str) const {
  return this->_env.erase(str);
}

Object& Context::lookup(const string& str) const {
  return (*this)[str];
}

Object& Context::operator [] (const string& str) const {
  if(_env.exist(str)){
    return *_env[str];
  }else{
    cerr << "[Context lookup] Unbounded variable: " << str << endl;
    exit(1);
  }
}

bool Context::exist(const string& str) const {
  return _env.exist(str);
}

bool Object::callable() const {
  if(type() == Primitive){
    /* TODO */
    return true;
  }else{
    if(_expr.isLam()){
      return true;
    }else{
      return false;
    }
  }
}

Object Object::call(const Expression& expr, const Context& env) const {
  if(type() == Primitive){
    /* TODO */
    return _func(expr, env);
  }else{
    if(_expr.isLam()){
      return Object(*_expr.body, _env.insert(_expr.name, Object(expr, env)));
    }else{
      cerr << "[Object call] Object not callable (not a `Lambda`): " << _expr.type << endl;
      exit(1);
    }
  }
}

Object makeNormalForm(const Expression& expr){
  return Object(Object::NormalForm, expr);
}

Object weakNormalForm(const Expression& expr, const Context& env);
Object normalForm(const Expression& expr, const Context& env);

Object weakNormalForm(const Expression& expr, const Context& env){
  if( expr.isNum() ){
    return makeNormalForm( expr );
  }else if( expr.isVar() ){
    if( env.exist(expr.name) ){
      Object& res = env[expr.name];
      if( res.isNormalForm() )
        return res;
      if( res.isPrimitive() )
        return res;
      return res = weakNormalForm( res.expr(), res.env() );
    }else{
      return makeNormalForm( expr );
    }
  }else if( expr.isLam() ){
    return Object(expr, env);
  }else if( expr.isAp() ){
    const Expression& body = *expr.body;
    const Expression& arg = *expr.arg;
    Object callee(weakNormalForm(body, env));
    if( callee.callable() ){
      Object res = callee.call(arg, env);
      if( res.isNormalForm() )
        return res;
      if( res.isPrimitive() )
        return res;
      return weakNormalForm(res.expr(), res.env());
    }else{
      Expression expr2(Expression::Ap);
      expr2.body.reset(new Expression(callee.expr()));
      expr2.arg.reset(new Expression(normalForm(arg, env).expr()));
      return makeNormalForm( expr2 );
    }
  }else{
    cerr << "[Weak normal form] Unexpected expression type: " << expr.type << endl;
    exit(1);
  }
}

Object normalForm(const Expression& expr, const Context& env){
  if( expr.isNum() ){
    return makeNormalForm( expr );
  }else if( expr.isVar() ){
    if( env.exist(expr.name) ){
      Object &res = env[expr.name];
      if( res.isNormalForm() )
        return res;
      if( res.isPrimitive() )
        return res;
      return res = normalForm( res.expr(), res.env() );
    }else{
      return makeNormalForm( expr );
    }
  }else if( expr.isLam() ){
    Expression expr2(expr);
    expr2.body.reset( new Expression(normalForm(*expr.body, env.erase( expr.name )).expr()) );
    // return Object(expr2, env);
    return makeNormalForm( expr2 );
  }else if( expr.isAp() ){
    const Expression& body = *expr.body;
    const Expression& arg = *expr.arg;
    Object callee(weakNormalForm(body, env));
    if( callee.callable() ){
      Object res = callee.call(arg, env);
      if( res.isNormalForm() )
        return res;
      if( res.isPrimitive() )
        return res;
      return normalForm(res.expr(), res.env());
    }else{
      Expression expr2(Expression::Ap);
      expr2.body.reset(new Expression(callee.expr()));
      expr2.arg.reset(new Expression(normalForm(arg, env).expr()));
      return makeNormalForm( expr2 );
    }
  }else{
    cerr << "[Normal form] Unexpected expression type: " << expr.type << endl;
    exit(1);
  }
}

void stripTrailingSpace(string&);

int main(int argc, char *argv[])
{
  Context prelude;
  prelude.add("true", "\\a \\b a");
  prelude.add("false", "\\a \\b b");
  prelude.add("if", "\\pred \\then \\else pred then else");
  prelude.add("not", "\\x x false true");
  prelude.add("and", "\\x \\y x y false");
  prelude.add("or", "\\x \\y x true y");
  // Y f = f (Y f)
  prelude.add("Y", Object([](const Expression& expr, const Context& env){
          Expression expr1( Expression::Ap );
          expr1.body.reset(new Expression(expr));
            Expression expr2( Expression::Ap );
            expr2.body.reset(new Expression("Y"));
            expr2.arg.reset(new Expression(expr));
          expr1.arg.reset(new Expression(expr2));
          return Object(expr1, env);
        }));
  prelude.add("+", Object([](const Expression& expr, const Context& env){
          int a = normalForm(expr, env).expr().val;
          return Object([a](const Expression& expr, const Context& env){
              int b = normalForm(expr, env).expr().val;
              return Expression( a + b );
            });
        }));
  prelude.add("-", Object([](const Expression& expr, const Context& env){
          int a = normalForm(expr, env).expr().val;
          return Object([a](const Expression& expr, const Context& env){
              int b = normalForm(expr, env).expr().val;
              return Expression( a - b );
            });
        }));
  prelude.add("*", Object([](const Expression& expr, const Context& env){
          int a = normalForm(expr, env).expr().val;
          return Object([a](const Expression& expr, const Context& env){
              if(a == 0)
                return Expression( 0 );
              int b = normalForm(expr, env).expr().val;
              return Expression( a * b );
            });
        }));
  prelude.add("/", Object([](const Expression& expr, const Context& env){
          int a = normalForm(expr, env).expr().val;
          return Object([a](const Expression& expr, const Context& env){
              int b = normalForm(expr, env).expr().val;
              return Expression( a / b );
            });
        }));
  prelude.add("%", Object([](const Expression& expr, const Context& env){
          int a = normalForm(expr, env).expr().val;
          return Object([a](const Expression& expr, const Context& env){
              int b = normalForm(expr, env).expr().val;
              return Expression( a % b );
            });
        }));
  prelude.add("==", Object([](const Expression& expr, const Context& env){
          int a = normalForm(expr, env).expr().val;
          return Object([a](const Expression& expr, const Context& env){
              int b = normalForm(expr, env).expr().val;
              if(a == b){
                return Object(Expression("true"), env);
              }else{
                return Object(Expression("false"), env);
              }
            });
        }));
  prelude.add("id", "\\x x");
  prelude.add("flip", "\\f \\x \\y f y x");
  prelude.add(".", "\\f \\g \\x f(g x)");
  prelude.add("!=", "\\a . not (== a)");
  prelude.add("<", Object([](const Expression& expr, const Context& env){
          int a = normalForm(expr, env).expr().val;
          return Object([a](const Expression& expr, const Context& env){
              int b = normalForm(expr, env).expr().val;
              if(a < b){
                return Object(Expression("true"), env);
              }else{
                return Object(Expression("false"), env);
              }
            });
        }));
  prelude.add("<=", Object([](const Expression& expr, const Context& env){
          int a = normalForm(expr, env).expr().val;
          return Object([a](const Expression& expr, const Context& env){
              int b = normalForm(expr, env).expr().val;
              if(a <= b){
                return Object(Expression("true"), env);
              }else{
                return Object(Expression("false"), env);
              }
            });
        }));
  prelude.add(">", "flip <");
  prelude.add(">=", "flip >=");

  prelude.add(">>=", "\\m \\f \\s (m s) \\s' \\a f a s'");
  prelude.add(">>", "\\m \\k \\s (m s) \\s' \\_ k s'");

  prelude.add("runIO", "\\m m s");
  prelude.add("putChar", Object([](const Expression& expr, const Context& env){
        int c = normalForm(expr, env).expr().val;
        return Object([c](const Expression& s, const Context& env){
          fputc(c, stdout);
          /* Lam "p" (Ap (Ap "p" `s`) "()") */
          return Object([s](const Expression& p, const Context& env){
            Object res = weakNormalForm(p, env).call(s, env);
            return weakNormalForm(res.expr(), res.env()).call(Expression("()"), res.env());
          });
        });
      }));

  string rawInput, input;
  while(getline(cin, input)){
    // stripTrailingSpace(input);
    rawInput += input;
    rawInput += "\n";
  }

  Scanner scanner(rawInput);
  shared_ptr<Expression> expr = parseExpression(scanner);
  Object res = normalForm(*expr, prelude);

  cout << endl;

  return 0;
}

void stripTrailingSpace(string& str){
  int i = str.size() - 1;
  while(i >= 0){
    if(str[i] != ' ' && str[i] != '\t' && str[i] != '\n'){
      break;
    }
    --i;
  }
  str = str.substr(0, i+1);
  return;
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
