#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <functional>
#include <vector>
#include <deque>
#include <memory>
#include <sstream>

#include <cstdio>

#include "ExpressionParser.hpp"
#include "Parsers.hpp"

using namespace std;

using Parsers::Parser;
using Parsers::satisfy;
using Parsers::anyChar;
using Parsers::spaces;
using Parsers::oneOf;
using Parsers::noneOf;
using Parsers::digit;
using Parsers::charp;
using Parsers::word;

const Parser alpha(satisfy([](char c){
  return isgraph(c) && c != '\\' && c != '(' && c != ')';
}));

const Parser integer(maybe(oneOf("+-")) >> +digit);
const Parser charLiteral(charp('\'') >> satisfy([](char c){ return isprint(c);}) >> charp('\''));
const Parser identifier(+alpha);
const Parser lambda('\\');
const Parser leftBracket('(');
const Parser rightBracket(')');
const Parser comment(word("--") >> many(noneOf("\n")));
const Parser panic(anyChar[ ([](const string& str){ cerr << "[Lex] Unexpected input: " << str << endl;}) ]);

Parser tokenParser(Token& token){
  auto f = [&token](Token::Type t){
    return [&token, t](const string& str){ token = Token(t, str);};
  };
  auto g = [&token](const string& str){
    if(str == "let") token = Token(Token::Keyword, "let");
    else if(str == "in") token = Token(Token::Keyword, "in");
    else token = Token(Token::Identifier, str);
  };
  return (comment[f(Token::Undefined)]
      | integer[f(Token::Constant)]
      | charLiteral[f(Token::Constant)]
      | identifier[g]
      | lambda[f(Token::Lambda)]
      | leftBracket[f(Token::LeftBracket)]
      | rightBracket[f(Token::RightBracket)]
      | panic[f(Token::Undefined)]
      );
}

Scanner::Scanner(const string& buffer){
  auto first = buffer.begin(), last = buffer.end();
  Token token;
  while(first != last){
    tie(ignore, first) = spaces.runParser(first, last);
    if(first == last) break;
    tie(ignore, first) = tokenParser(token).runParser(first, last);
    if(token.type != Token::Undefined)
      tokens.push_back(token);
  }
}

Token Scanner::getToken(){
  if(tokens.empty()) return Token(Token::EndOfFile, "EOF");
  Token token = tokens.front();
  tokens.pop_front();
  return token;
}

Token Scanner::peekToken(){
  if(tokens.empty()) return Token(Token::EndOfFile, "EOF");
  return tokens.front();
}

void Scanner::ungetToken(const Token& token){
  if(token.type == Token::EndOfFile) return ;
  tokens.push_front(token);
}

bool Scanner::eof() const {
  return tokens.empty();
}

void Expression::print() const {
  switch(type){
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
    case Nothing:
      cerr << "[Expression] Unexpected expression type: Nothing" << endl;
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
    case Nothing:
      cerr << "[Expression] Unexpected expression type: Nothing" << endl;
  }
}

template<class T>
T stringTo(const string& str){
  istringstream iss(str);
  T res;
  iss >> res;
  return res;
}

Expression * parseExpressionTail(Scanner&);

Expression * parseExpression(Scanner &scanner){
  Expression * expr(parseExpressionTail(scanner));
  if(expr == nullptr){
    cerr << "[Parse expression] Unexpected token: " << scanner.peekToken().name << endl;
    exit(1);
    return nullptr;
  }
  while(true){
    Expression * expr1(parseExpressionTail(scanner));
    if(expr1 == nullptr){
      return expr;
    }else{
      Expression * expr2(new Expression(Expression::Ap));
      expr2->body = expr;
      expr2->arg  = expr1;
      expr = expr2;
    }
  }
}

Expression * parseExpressionTail(Scanner &scanner){
  Token token = scanner.getToken();
  Expression * expr(nullptr);
  switch(token.type){
    case Token::Lambda:
      token = scanner.getToken();
      if(token.type != Token::Identifier){
        cerr << "[Parse] Expected an identifier: " << token.name << endl;
        exit(1);
      }
      expr = new Expression(Expression::Lambda);
      expr->name = token.name;
      expr->body = parseExpression(scanner);
      return expr;

    case Token::Identifier:
      expr = new Expression(Expression::Var);
      expr->name = token.name;
      return expr;

    case Token::Keyword:
      if(token.name == "let"){
        token = scanner.getToken();
        if(token.type != Token::Identifier){
          cerr << "[Parse] Expected an identifier: " << token.name << endl;
          exit(1);
        }
        expr = new Expression(Expression::Ap);
        expr->body = new Expression(Expression::Lambda);
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
      expr = new Expression(Expression::Constant);
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

