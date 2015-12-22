#ifndef __SILVERNEGI_TRIE_HPP__
#define __SILVERNEGI_TRIE_HPP__

#include <string>
#include <memory>
#include <unordered_map>

namespace{

using namespace std;

template <class T>
class Trie{
  public:
    unordered_map<char, shared_ptr<Trie<T>>> branches;
    bool endHere;
    T cargo;

    Trie() : endHere(false) {}
    Trie(const T& t) : cargo(t) {}

    bool exist(const string& str, int i = 0) const {
      if(i == str.size()){
        return endHere;
      }
      char c = str[i];
      auto branch = branches.find(c);
      if(branch != branches.end()){
        return branch->second->exist(str, i+1);
      }else{
        return false;
      }
    }

    const T& find(const string& str, int i = 0) const {
      if(i == str.size()){
        if(endHere){
          return cargo;
        }else{
          cerr << "[Trie] Nothing here" << endl;
          exit(1);
        }
      }
      char c = str[i];
      auto branch = branches.find(c);
      if(branch != branches.end()){
        return branch->second->find(str, i+1);
      }else{
        cerr << "[Trie] Nothing here" << endl;
        exit(1);
      }
    }

    shared_ptr<Trie<T>> insert(const string& str, const T& t, int i = 0) const {
      shared_ptr<Trie<T>> newNode(new Trie<T>(*this));
      if(i == str.size()){
        newNode->endHere = true;
        newNode->cargo = t;
      }else{
        char c = str[i];
        auto branch = branches.find(c);
        if(branch != branches.end()){
          newNode->branches[c] = branch->second->insert(str, t, i+1);
        }else{
          shared_ptr<Trie<T>> empty(new Trie<T>());
          newNode->branches[c] = empty->insert(str, t, i+1);
        }
      }
      return newNode;
    }

    shared_ptr<Trie<T>> erase(const string& str, int i = 0) const {
      shared_ptr<Trie<T>> newNode(new Trie<T>(*this));
      if(i == str.size()){
        newNode->endHere = false;
      }else{
        char c = str[i];
        auto branch = branches.find(c);
        if(branch != branches.end()){
          newNode->branches[c] = branch->second->erase(str, i+1);
        }
      }
      return newNode;
    }
};

template<class T>
class Dictionary{
    shared_ptr<Trie<T>> _trie;
  public:
    Dictionary() : _trie(new Trie<T>()) {}
    Dictionary(const shared_ptr<Trie<T>>& t) : _trie(t) {}
    Dictionary(const Dictionary& dict) : _trie(dict._trie) {}

    const T& operator [] (const string& str) const {
      return _trie->find(str);
    }

    bool exist(const string& str) const {
      return _trie->exist(str);
    }

    Dictionary insert(const string& str, const T& t) const {
      return _trie->insert(str, t);
    }

    Dictionary erase(const string& str) const {
      return _trie->erase(str);
    }
};

}
#endif
