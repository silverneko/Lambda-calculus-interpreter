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
#include "Dictionary.hpp"

using namespace std;

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
    cerr << "[Context lookup] Unexpected free variable: " << str << endl;
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
  switch( expr.type ){
    case Expression::Constant:
      return makeNormalForm( expr );
      break;
    case Expression::Var:
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
      break;
    case Expression::Lambda:
      return Object(expr, env);
      break;
    case Expression::Ap:
      {
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
          expr2.body = new Expression(callee.expr());
          expr2.arg = new Expression(normalForm(arg, env).expr());
          return makeNormalForm( expr2 );
        }
      }
      break;
    case Expression::Nothing:
      cerr << "[Weak normal form] Unexpected expression type: " << expr.type << endl;
      exit(1);
  }
}

Object normalForm(const Expression& expr, const Context& env){
  switch( expr.type ){
    case Expression::Constant:
      return makeNormalForm( expr );
      break;
    case Expression::Var:
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
      break;
    case Expression::Lambda:
      {
        Expression expr2(expr);
        expr2.body = new Expression(normalForm(*expr.body, env.erase( expr.name )).expr());
        return makeNormalForm( expr2 );
      // return Object(expr2, env);
      }
      break;
    case Expression::Ap:
      {
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
          expr2.body = new Expression(callee.expr());
          expr2.arg = new Expression(normalForm(arg, env).expr());
          return makeNormalForm( expr2 );
        }
      }
      break;
    case Expression::Nothing:
      cerr << "[Normal form] Unexpected expression type: " << expr.type << endl;
      exit(1);
  }
}

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
  prelude.add("Y", "\\f (\\x f (x x)) (\\x f (x x))");
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
  prelude.add("mod", Object([](const Expression& expr, const Context& env){
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
  prelude.add("flip", "\\f \\x \\y f y x");
  prelude.add("!=", "\\a \\b not (== a b)");
  prelude.add(">", "flip <");
  prelude.add(">=", "flip >=");

  prelude.add(">>=", "\\m \\f \\s (m s) \\a \\s' f a s'");
  prelude.add(">>", "\\ma \\mb >>= ma (\\_ mb)");

  prelude.add("runIO", "\\m m s");
  prelude.add("pair", "\\a \\b \\p p a b");
  prelude.add("pureIO", "pair");
  prelude.add("putChar", Object([](const Expression& expr, const Context& env){
        auto promiseChar = [expr, env](){ return normalForm(expr, env).expr().val;};
        return Object([promiseChar](const Expression& s, const Context& env){
          fputc(promiseChar(), stdout);
          /* pair nil s */
          return Object([s, env](const Expression& p, const Context& _env){
            Object res = weakNormalForm(p, _env).call(Expression("nil"), {});
            return weakNormalForm(res.expr(), res.env()).call(s, env);
          });
        });
      }));
  prelude.add("getChar", Object([](const Expression& s, const Context& env){
        int c = fgetc(stdin);
        return Object([c, s, env](const Expression& p, const Context& _env){
          Object res = weakNormalForm(p, _env).call(Expression(c), {});
          return weakNormalForm(res.expr(), res.env()).call(s, env);
        });
      }));

  string rawInput, input;
  ifstream file("samplecode/prelude");
  while(getline(file, input)){
    rawInput += input;
    rawInput += "\n";
  }
  file.close();
  if(argc > 1){
    file.open(argv[1]);
    while(getline(file, input)){
      rawInput += input;
      rawInput += "\n";
    }
    file.close();
  }else{
    while(getline(cin, input)){
      rawInput += input;
      rawInput += "\n";
    }
  }
  Scanner scanner(rawInput);
  Expression * expr = parseExpression(scanner);
  Object res = normalForm(*expr, prelude);

  //res.expr().prettyPrint();
  //cout << endl;

  return 0;
}

