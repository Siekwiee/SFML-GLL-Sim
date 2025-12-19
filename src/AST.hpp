#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <filesystem>

struct Program {
  std::vector<std::string> inputNames, outputNames;
  std::unordered_map<std::string, int> symbolToSignal;
  
  struct TokenSpan { 
    int line, col0, col1; 
    std::string symbol; 
  };
  std::vector<TokenSpan> tokens;
  
  struct Node {
    enum Type { AND_, OR_, NOT_, SR_, RS_, TON_, TOF_, BTN } type;
    std::string name;
    std::vector<int> inputs;
    std::vector<int> outputs;
    int sourceLine;
  };
  std::vector<Node> nodes;
  std::vector<std::string> sourceLines;
  std::filesystem::file_time_type lastModifiedAt;
};

