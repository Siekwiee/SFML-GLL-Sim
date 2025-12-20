#include "Parser.hpp"
#include "TimeUtils.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <filesystem>

static void trim(std::string& s) {
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
    return !std::isspace(ch);
  }));
  s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
    return !std::isspace(ch);
  }).base(), s.end());
}

static std::vector<std::string> split(const std::string& s, char delim) {
  std::vector<std::string> result;
  std::stringstream ss(s);
  std::string item;
  while (std::getline(ss, item, delim)) {
    trim(item);
    if (!item.empty()) {
      result.push_back(item);
    }
  }
  return result;
}

static int getOrCreateSignal(Program& prog, const std::string& symbol) {
  auto it = prog.symbolToSignal.find(symbol);
  if (it != prog.symbolToSignal.end()) {
    return it->second;
  }
  int id = static_cast<int>(prog.symbolToSignal.size());
  prog.symbolToSignal[symbol] = id;
  return id;
}

static void addTokenSpan(Program& prog, int line, int col0, int col1, const std::string& symbol) {
  Program::TokenSpan span;
  span.line = line;
  span.col0 = col0;
  span.col1 = col1;
  span.symbol = symbol;
  prog.tokens.push_back(span);
}
bool fileWatcher(const std::string& path, Program& out) {
  if (std::filesystem::last_write_time(path) != out.lastModifiedAt) {
    return true;
    printf("File changed: %s\n", path.c_str());
  } else {
    return false;
    printf("File unchanged: %s\n", path.c_str());
  }
}

ParseResult parseFile(const std::string& path, Program& out) {
  std::ifstream file(path);
  if (!file.is_open()) {
    return {false, "Could not open file: " + path};
  }

  out.inputNames.clear();
  out.outputNames.clear();
  out.symbolToSignal.clear();
  out.nodes.clear();
  out.sourceLines.clear();
  out.tokens.clear();

  std::string line;
  int lineNum = 0;
  bool inDeclarations = true;

  while (std::getline(file, line)) {
    out.sourceLines.push_back(line);
    trim(line);
    
    if (line.empty() || line[0] == '#') {
      lineNum++;
      continue;
    }

    // Parse IN declaration
    if (line.substr(0, 3) == "IN ") {
      std::string rest = line.substr(3);
      auto items = split(rest, ',');
      size_t searchStart = 3; // Start after "IN "
      for (const auto& item : items) {
        // Check for alias: name(alias)
        size_t parenOpen = item.find('(');
        size_t parenClose = item.find(')');
        
        std::string name = item;
        std::string alias = "";
        
        if (parenOpen != std::string::npos && parenClose != std::string::npos && parenClose > parenOpen) {
          name = item.substr(0, parenOpen);
          alias = item.substr(parenOpen + 1, parenClose - parenOpen - 1);
          trim(name);
          trim(alias);
        }

        int sigId = getOrCreateSignal(out, name);
        if (!alias.empty()) {
          out.symbolToSignal[alias] = sigId;
          out.inputNames.push_back(alias);
        } else {
          out.inputNames.push_back(name);
        }

        // Find with word boundary check
        size_t pos = line.find(item, searchStart);
        if (pos != std::string::npos) {
          addTokenSpan(out, lineNum, static_cast<int>(pos), 
                       static_cast<int>(pos + item.length()), item);
          searchStart = pos + item.length();
        }
      }
      lineNum++;
      continue;
    }

    // Parse OUT declaration
    if (line.substr(0, 4) == "OUT ") {
      std::string rest = line.substr(4);
      auto items = split(rest, ',');
      size_t searchStart = 4; // Start after "OUT "
      for (const auto& item : items) {
        // Check for alias: name(alias)
        size_t parenOpen = item.find('(');
        size_t parenClose = item.find(')');
        
        std::string name = item;
        std::string alias = "";
        
        if (parenOpen != std::string::npos && parenClose != std::string::npos && parenClose > parenOpen) {
          name = item.substr(0, parenOpen);
          alias = item.substr(parenOpen + 1, parenClose - parenOpen - 1);
          trim(name);
          trim(alias);
        }

        int sigId = getOrCreateSignal(out, name);
        if (!alias.empty()) {
          out.symbolToSignal[alias] = sigId;
          out.outputNames.push_back(alias);
        } else {
          out.outputNames.push_back(name);
        }

        // Find with word boundary check
        size_t pos = line.find(item, searchStart);
        if (pos != std::string::npos) {
          addTokenSpan(out, lineNum, static_cast<int>(pos), 
                       static_cast<int>(pos + item.length()), item);
          searchStart = pos + item.length();
        }
      }
      lineNum++;
      continue;
    }

    // Parse gate: <GATETYPE> <name>(args...) -> output
    size_t arrowPos = line.find("->");
    if (arrowPos == std::string::npos) {
      lineNum++;
      continue;
    }

    std::string beforeArrow = line.substr(0, arrowPos);
    std::string afterArrow = line.substr(arrowPos + 2);
    trim(beforeArrow);
    trim(afterArrow);

    // Extract gate type and name
    size_t spacePos = beforeArrow.find(' ');
    if (spacePos == std::string::npos) {
      return {false, "Line " + std::to_string(lineNum + 1) + ": Invalid gate syntax"};
    }

    std::string gateType = beforeArrow.substr(0, spacePos);
    std::string gateName = beforeArrow.substr(spacePos + 1);
    
    size_t parenPos = gateName.find('(');
    if (parenPos == std::string::npos) {
      return {false, "Line " + std::to_string(lineNum + 1) + ": Missing '(' in gate definition"};
    }

    std::string name = gateName.substr(0, parenPos);
    std::string argsStr = gateName.substr(parenPos + 1);
    
    // Find matching closing paren (handle nested parens from NOT())
    int depth = 1;
    size_t closeParen = 0;
    while (closeParen < argsStr.size() && depth > 0) {
      if (argsStr[closeParen] == '(') depth++;
      else if (argsStr[closeParen] == ')') depth--;
      if (depth > 0) closeParen++;
    }
    if (depth != 0) {
      return {false, "Line " + std::to_string(lineNum + 1) + ": Missing ')' in gate definition"};
    }
    argsStr = argsStr.substr(0, closeParen);

    // Parse gate type
    Program::Node::Type type;
    if (gateType == "AND") {
      type = Program::Node::AND_;
    } else if (gateType == "OR") {
      type = Program::Node::OR_;
    } else if (gateType == "XOR") {
      type = Program::Node::XOR_;
    } else if (gateType == "NOT") {
      type = Program::Node::NOT_;
    } else if (gateType == "SR") {
      type = Program::Node::SR_;
    } else if (gateType == "RS") {
      type = Program::Node::RS_;
    } else if (gateType == "TON") {
      type = Program::Node::TON_;
    } else if (gateType == "TOF") {
      type = Program::Node::TOF_;
    } else if (gateType == "CTU") {
      type = Program::Node::CTU_;
    } else if (gateType == "CTD") {
      type = Program::Node::CTD_;
    } else if (gateType == "BTN") {
      type = Program::Node::BTN;
    } else {
      return {false, "Line " + std::to_string(lineNum + 1) + ": Unknown gate type: " + gateType};
    }

    // Create node
    Program::Node node;

    // Parse inputs (handle NOT(...) syntax)
    std::vector<int> inputs;
    std::vector<std::string> inputSymbols;
    
    // First, find original token positions in the source line BEFORE processing
    // We need to find the arguments section in the original line
    size_t argsStartInLine = line.find('(') + 1;
    
    if (!argsStr.empty()) {
      // Handle NOT(...) in arguments - find matching parenthesis properly
      std::string processed = argsStr;
      size_t notPos = processed.find("NOT(");
      while (notPos != std::string::npos) {
        // Find the matching closing paren by counting depth
        int depth = 1;
        size_t notEnd = notPos + 4; // Start after "NOT("
        while (notEnd < processed.size() && depth > 0) {
          if (processed[notEnd] == '(') depth++;
          else if (processed[notEnd] == ')') depth--;
          if (depth > 0) notEnd++;
        }
        
        if (depth != 0) {
          return {false, "Line " + std::to_string(lineNum + 1) + ": Unmatched NOT("};
        }
        
        std::string notArg = processed.substr(notPos + 4, notEnd - notPos - 4);
        trim(notArg);
        
        // Add token span for the argument inside NOT() - find it in original line
        size_t notInOriginal = argsStr.find("NOT(");
        if (notInOriginal != std::string::npos) {
          size_t argInOriginal = argsStr.find(notArg, notInOriginal + 4);
          if (argInOriginal != std::string::npos) {
            int col0 = static_cast<int>(argsStartInLine + argInOriginal);
            addTokenSpan(out, lineNum, col0, col0 + static_cast<int>(notArg.length()), notArg);
          }
        }
        
        // Create a NOT node for this
        Program::Node notNode;
        notNode.type = Program::Node::NOT_;
        notNode.name = "_not_" + std::to_string(out.nodes.size());
        int notInputSig = getOrCreateSignal(out, notArg);
        notNode.inputs.push_back(notInputSig);
        std::string notOutputName = "_not_" + std::to_string(out.nodes.size()) + "_out";
        int notOutputSig = getOrCreateSignal(out, notOutputName);
        notNode.outputs.push_back(notOutputSig);
        notNode.sourceLine = lineNum;
        out.nodes.push_back(notNode);
        
        // Replace NOT(...) with the output signal name in processed string
        processed.replace(notPos, notEnd - notPos + 1, notOutputName);
        notPos = processed.find("NOT(");
      }
      
      // Now split by comma and add non-internal signals
      auto argList = split(processed, ',');
      int argIdx = 0;
      for (const auto& arg : argList) {
        // Special handling for TON/TOF first argument as hardcoded time
        if (argIdx == 0 && (type == Program::Node::TON_ || type == Program::Node::TOF_)) {
          // Check if it's a quoted string or looks like a time (starts with digit or dot)
          if (!arg.empty() && ((arg.front() == '"' && arg.back() == '"') || (std::isdigit(arg.front())) || (arg.front() == '.'))) {
            std::string timeStr = arg;
            if (timeStr.front() == '"') {
                timeStr = timeStr.substr(1, timeStr.length() - 2);
            }
            node.hardcodedPresetTime = parseTimeStringToFloat(timeStr);
            argIdx++;
            continue; // Skip adding this as a signal input
          }
        }

        // Special handling for CTU/CTD arguments (PV)
        if (argIdx == 0 && (type == Program::Node::CTU_ || type == Program::Node::CTD_)) {
          if (!arg.empty() && ((arg.front() == '"' && arg.back() == '"') || (std::isdigit(arg.front())) || (arg.size() > 1 && arg.front() == '-'))) {
            std::string valStr = arg;
            if (valStr.front() == '"') valStr = valStr.substr(1, valStr.length() - 2);
            try {
              // Try to parse as int
              int val = std::stoi(valStr);
              node.hardcodedPresetValue = val;
              argIdx++;
              continue;
            } catch (...) {
              // If not a number, fall through to signal handling
            }
          }
        }

        argIdx++;
        inputSymbols.push_back(arg);
        int sigId = getOrCreateSignal(out, arg);
        inputs.push_back(sigId);
        
        // Only add token span for non-internal signals (not _not_X_out)
        if (arg.find("_not_") != 0) {
          // Find this exact token in original argsStr using word boundaries
          size_t pos = 0;
          while (pos < argsStr.size()) {
            size_t found = argsStr.find(arg, pos);
            if (found == std::string::npos) break;
            
            // Check word boundaries
            bool startOk = (found == 0 || !std::isalnum(argsStr[found - 1]));
            bool endOk = (found + arg.size() >= argsStr.size() || 
                         !std::isalnum(argsStr[found + arg.size()]));
            
            if (startOk && endOk) {
              int col0 = static_cast<int>(argsStartInLine + found);
              addTokenSpan(out, lineNum, col0, col0 + static_cast<int>(arg.length()), arg);
              break;
            }
            pos = found + 1;
          }
        }
      }
    }

    // Parse outputs (comma-separated) - find with word boundaries
    std::vector<int> outputSigs;
    auto outputNames = split(afterArrow, ',');
    size_t searchStart = arrowPos + 2; // Start after "->"
    for (const auto& outputName : outputNames) {
      int outputSig = getOrCreateSignal(out, outputName);
      outputSigs.push_back(outputSig);
      // Find this output name in the original line
      size_t outPos = line.find(outputName, searchStart);
      if (outPos != std::string::npos) {
        addTokenSpan(out, lineNum, static_cast<int>(outPos), 
                     static_cast<int>(outPos + outputName.length()), outputName);
        searchStart = outPos + outputName.length();
      }
    }

    // Create node
    node.type = type;
    node.name = name;
    node.inputs = inputs;
    node.outputs = outputSigs;
    node.sourceLine = lineNum;
    out.nodes.push_back(node);

    lineNum++;
  }

  return {true, ""};
}

