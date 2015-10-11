#include <cstdlib>
#include <iostream>
#include <string>
#include <algorithm>
#include <functional>
#include <vector>
#include <deque>

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
const Parser panic(anyChar[ ([](const string& str){ cerr << "[Lex] Unknown input: " << str << endl;}) ]);

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
      Token token =  tokens.front();
      tokens.pop_front();
      return token;
    }

    Token peekToken(){
      if(tokens.empty()) return Token(Token::EndOfFile, "EOF");
      Token token =  tokens.front();
      return token;
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
    Expression * body, * arg;

    Expression() : type(Nothing), name(), body(nullptr), arg(nullptr) {}
    Expression(Type t) : type(t), name(), body(nullptr), arg(nullptr) {}
    Expression(const Expression& expr) : type(expr.type), name(expr.name), body(expr.body), arg(expr.arg){}

    ~Expression(){
      delete body;
      delete arg;
    }

    void print() const ;
};

Expression * parseVar(Scanner&);
Expression * parseLambda(Scanner&);
Expression * parseAp(Scanner&);
Expression * parseExpression(Scanner&);

Expression * parseVar(Scanner &scanner){
  Token token = scanner.getToken();
  if(token.type == Token::Identifier){
    Expression * expr = new Expression(Expression::Var);
    expr->name = token.name;
    return expr;
  }else{
    cerr << "[Parse] Expect an identifier: " << token.name << endl;
    exit(1);
  }
}

Expression * parseLambda(Scanner &scanner){
  Token token = scanner.getToken();
  if(token.type != Token::Identifier){
    cerr << "[Parse] Expected an identifier: " << token.name << endl;
    exit(1);
  }
  Expression * expr = new Expression(Expression::Lambda);
  expr->name = token.name;
  expr->body = parseExpression(scanner);
  return expr;
}

Expression * parseAp(Scanner &scanner){
  Token token = scanner.getToken();
  Expression * expr = nullptr;
  switch(token.type){
    case Token::Identifier:
      scanner.ungetToken(token);
      expr = parseVar(scanner);
      break;

    case Token::LeftBracket:
      expr = parseExpression(scanner);
      token = scanner.getToken();
      if(token.type != Token::RightBracket){
        cerr << "[Parse] Expect a `)`: " << token.name << endl;
        exit(1);
      }
      break;

    default:
      cerr << "[Parse] Unexpected: " << token.name << endl;
      exit(1);
  }
  switch(scanner.peekToken().type){
    case Token::RightBracket:
    case Token::EndOfFile:
      return expr;

    default: break;
  }
  Expression * ap = new Expression(Expression::Ap);
  ap->body = expr;
  ap->arg  = parseExpression(scanner);
  return ap;
}

Expression * parseExpression(Scanner &scanner){
  Token token = scanner.getToken();
  switch(token.type){
    case Token::Lambda:
      return parseLambda(scanner);

    case Token::Identifier:
    case Token::LeftBracket:
      scanner.ungetToken(token);
      return parseAp(scanner);

    case Token::RightBracket:
    case Token::EndOfFile:
    case Token::Undefined:
      cerr << "[Parse] Unexpected: " << token.name << endl;
      exit(1);
  }
  return nullptr;
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

  Expression * expr = parseExpression(scanner);

  expr->print();

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
