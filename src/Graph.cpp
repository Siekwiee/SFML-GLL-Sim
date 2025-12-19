#include "Graph.hpp"
#include <queue>
#include <unordered_map>
#include <algorithm>

bool computeTopologicalOrder(const Program& prog, std::vector<int>& topoOrder) {
  topoOrder.clear();
  
  if (prog.nodes.empty()) {
    return true;
  }

  const size_t nodeCount = prog.nodes.size();

  // Simply execute nodes in the order they appear in the program
  // This preserves the exact order as written in the source file.
  // This is a standard single-pass scan behavior used in industrial logic (PLCs).
  // Any forward references (line 10 using signal from line 15) will naturally 
  // have a 1-scan lag, which is predictable and prevents "scary" oscillations.
  for (size_t i = 0; i < nodeCount; ++i) {
    topoOrder.push_back(static_cast<int>(i));
  }

  // Detect cycles/forward references for informational purposes.
  // A forward reference exists if a node reads a signal produced by a later node.
  std::unordered_map<int, int> signalToProducer; // signal -> producer node index
  for (size_t i = 0; i < nodeCount; ++i) {
    const auto& node = prog.nodes[i];
    for (int outputSig : node.outputs) {
      signalToProducer[outputSig] = static_cast<int>(i);
    }
  }
  
  // Now check if any node reads a signal produced by a later node
  bool hasForwardReference = false;
  for (size_t i = 0; i < nodeCount; ++i) {
    const auto& node = prog.nodes[i];
    if (node.type == Program::Node::BTN) continue;
    
    for (int inputSig : node.inputs) {
      auto it = signalToProducer.find(inputSig);
      if (it != signalToProducer.end() && it->second > static_cast<int>(i)) {
        hasForwardReference = true;
        break;
      }
    }
    if (hasForwardReference) break;
  }

  // Return false if forward references detected
  return !hasForwardReference;
}