#ifndef __SILVERNEGI_PARSERS_HPP__
#define __SILVERNEGI_PARSERS_HPP__

#include <cstdlib>
#include <iostream>
#include <string>
#include <algorithm>
#include <functional>
#include <cctype>

namespace Parsers{

// Let us not pollute the namespace outside
using namespace std;

class Parser{
  public:
    using Iterator = std::string::const_iterator;
    using ParserFunction = std::function<std::tuple<bool, Iterator> (Iterator, Iterator)>;

    ParserFunction runParser;

    Parser();
    Parser(ParserFunction parser) : runParser(parser) {}
    Parser(const Parser & parser) : runParser(parser.runParser) {}
    Parser(char);

    // Bind action to parser
    Parser operator [] (std::function<void (const std::string&)>) const;

};

// tuple<bool, Iterator>
// <Parse state, moved position>
std::tuple<bool, Parser::Iterator> badParse(Parser::Iterator it){
  return std::make_tuple(false, it);
}

std::tuple<bool, Parser::Iterator> goodParse(Parser::Iterator it){
  return std::make_tuple(true, it);
}

Parser::Parser() : runParser( [](Parser::Iterator _, Parser::Iterator __){
  return goodParse(_);}) {
}

Parser Parser::operator [] (function<void (const std::string&)> f) const {
  auto parser = this->runParser;
  return Parser([f, parser](Parser::Iterator first, Parser::Iterator last){
    bool good;
    Parser::Iterator second;
    std::tie(good, second) = parser(first, last);
    if(! good) return badParse(second);
    f({first, second});
    return goodParse(second);
  });
}

// Parser combinators
Parser some(Parser);
Parser many(Parser);
Parser operator * (Parser);
Parser operator + (Parser);
Parser satisfy(std::function<bool (char)>);
Parser charp(char);
Parser word(const std::string&);
Parser oneOf(const std::string&);
Parser noneOf(const std::string&);
Parser operator >> (Parser, Parser);
Parser operator | (Parser, Parser);
Parser say(const std::string&, std::function<void (const std::string&)>);

Parser::Parser(char c) : Parser(charp(c)) {
}

Parser say(const std::string& msg, std::function<void (const std::string&)> f){
  return Parser([msg, f](Parser::Iterator _, Parser::Iterator __){
    f(msg);
    return goodParse(_);
  });
}

Parser word(const std::string& str){
  Parser accu;
  for(char c : str)
    accu = accu >> charp(c);
  return accu;
}

Parser operator * (Parser parser){
  return many(parser);
}

Parser operator + (Parser parser){
  return some(parser);
}

// One or more.
Parser some(Parser parser){
  return Parser([parser](Parser::Iterator first, Parser::Iterator last){
    bool good;
    std::tie(good, first) = parser.runParser(first, last);
    if(! good) return badParse(first);
    std::tie(ignore, first) = many(parser).runParser(first, last);
    return goodParse(first);
  });
}

// Zero or more.
Parser many(Parser parser){
  return Parser([parser](Parser::Iterator first, Parser::Iterator last){
    bool good = true;
    Parser::Iterator second = first;
    while(good){
      std::tie(good, second) = parser.runParser(first, last);
      swap(first, second);
    }
    return goodParse(second);
  });
}

// Monadic bind :P
Parser operator >> (Parser f, Parser g){
  return Parser([f, g](Parser::Iterator first, Parser::Iterator last){
    bool good;
    std::tie(good, first) = f.runParser(first, last);
    if(! good) return badParse(first);
    std::tie(good, first) = g.runParser(first, last);
    if(! good) return badParse(first);
    return goodParse(first);
  });
}

// Use `f` to parse, if fail, use `g` to parse
Parser operator | (Parser f, Parser g){
  return Parser([f, g](Parser::Iterator first, Parser::Iterator last){
    bool good;
    std::tie(good, first) = f.runParser(first, last);
    if(good) return goodParse(first);
    std::tie(good, first) = g.runParser(first, last);
    if(good) return goodParse(first);
    return badParse(first);
  });
}

Parser charp(char c){
  return satisfy([c](char x){ return c == x;});
}

Parser oneOf(const std::string& sig){
  return Parser([sig](Parser::Iterator first, Parser::Iterator last){
    if(first == last) return badParse(first);
    if(find(sig.begin(), sig.end(), *first) == sig.end())
      return badParse(first);
    return goodParse(++first);
  });
}

Parser noneOf(const std::string& sig){
  return Parser([sig](Parser::Iterator first, Parser::Iterator last){
    if(first == last) return badParse(first);
    if(find(sig.begin(), sig.end(), *first) != sig.end())
      return badParse(first);
    return goodParse(++first);
  });
}

Parser satisfy(std::function<bool (char)> f){
  return Parser([f](Parser::Iterator first, Parser::Iterator last){
    if(first == last) return badParse(first);
    if(! f( *first )) return badParse(first);
    return goodParse(++first);
  });
}

const Parser letter(satisfy([](char c){
  return isalpha(c);
}));

const Parser digit(satisfy([](char c){
  return isdigit(c);
}));

const Parser alphanum(satisfy([](char c){
  return isalnum(c);
}));

const Parser anyChar(satisfy([](char c){
  return true;
}));

const Parser space(oneOf(" \t\n"));
const Parser spaces(*space);

// end of namespace "Parsers"
}

#endif
