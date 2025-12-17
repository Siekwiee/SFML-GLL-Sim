#pragma once
#include "AST.hpp"
#include <string>

struct ParseResult { 
  bool ok; 
  std::string msg; 
};

ParseResult parseFile(const std::string& path, Program& out);

