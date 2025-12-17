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

  // Map each produced signal -> list of producer nodes
  std::unordered_map<int, std::vector<int>> producers;
  producers.reserve(nodeCount * 2);
  for (size_t i = 0; i < nodeCount; ++i) {
    for (int outputSig : prog.nodes[i].outputs) {
      producers[outputSig].push_back(static_cast<int>(i));
    }
  }

  // Build adjacency and in-degree in one pass
  std::vector<std::vector<int>> adj(nodeCount);
  std::vector<int> inDegree(nodeCount, 0);

  for (size_t i = 0; i < nodeCount; ++i) {
    const auto& node = prog.nodes[i];

    // BTN nodes are sources - their inputs are user-controlled
    if (node.type == Program::Node::BTN) {
      continue;
    }

    for (int inputSig : node.inputs) {
      auto it = producers.find(inputSig);
      if (it == producers.end()) continue;

      for (int producerIdx : it->second) {
        if (producerIdx == static_cast<int>(i)) continue;
        adj[producerIdx].push_back(static_cast<int>(i));
        inDegree[i]++;
      }
    }
  }

  // Min-heap to keep evaluation order stable (top-to-bottom by source line, then index)
  auto cmp = [&](int a, int b) {
    const auto& na = prog.nodes[a];
    const auto& nb = prog.nodes[b];
    if (na.sourceLine != nb.sourceLine) return na.sourceLine > nb.sourceLine;
    return a > b;
  };
  std::priority_queue<int, std::vector<int>, decltype(cmp)> ready(cmp);

  for (size_t i = 0; i < nodeCount; ++i) {
    if (inDegree[i] == 0) {
      ready.push(static_cast<int>(i));
    }
  }

  while (!ready.empty()) {
    int u = ready.top();
    ready.pop();
    topoOrder.push_back(u);

    for (int v : adj[u]) {
      if (--inDegree[v] == 0) {
        ready.push(v);
      }
    }
  }

  // If not all nodes were scheduled, a cycle exists
  // Add remaining nodes (cycle nodes) at the end in source line order
  if (topoOrder.size() < nodeCount) {
    std::vector<int> cycleNodes;
    for (size_t i = 0; i < nodeCount; ++i) {
      bool found = false;
      for (int scheduled : topoOrder) {
        if (static_cast<int>(i) == scheduled) {
          found = true;
          break;
        }
      }
      if (!found) {
        cycleNodes.push_back(static_cast<int>(i));
      }
    }
    
    // Sort cycle nodes by source line
    std::sort(cycleNodes.begin(), cycleNodes.end(), [&](int a, int b) {
      return prog.nodes[a].sourceLine < prog.nodes[b].sourceLine;
    });
    
    topoOrder.insert(topoOrder.end(), cycleNodes.begin(), cycleNodes.end());
  }

  return topoOrder.size() == nodeCount;
}

