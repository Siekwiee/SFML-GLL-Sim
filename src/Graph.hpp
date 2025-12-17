#pragma once
#include "AST.hpp"
#include <vector>

// Compute topological order of nodes for evaluation
// Returns true if successful, false if cycle detected
bool computeTopologicalOrder(const Program& prog, std::vector<int>& topoOrder);

