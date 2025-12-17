#pragma once
#include "AST.hpp"
#include <vector>
#include <unordered_map>

struct Simulator {
  explicit Simulator(const Program& p);
  void update(float dt, float simHz, bool running, bool stepOnce);

  // BTN control hooks
  void setMomentary(const std::string& btnName, bool down);
  void toggleLatch(const std::string& btnName);
  
  // Direct signal control (for IN signals without BTN nodes)
  void toggleSignal(const std::string& signalName);
  void setSignal(const std::string& signalName, bool value);
  
  // Query BTN state
  bool isButtonPressed(const std::string& btnName) const;
  bool isButtonLatched(const std::string& btnName) const;
  bool getSignalValue(const std::string& signalName) const;

  const std::vector<uint8_t>& signals() const { return cur_; }
  int currentEvaluatingLine() const { return curLine_; }
  int currentEvaluatingNode() const { return curNodeIdx_; }
  bool isValidTopology() const { return !topo_.empty() && topo_.size() == prog_.nodes.size(); }
  bool isSteppingThrough() const { return stepping_; }

private:
  const Program& prog_;
  std::vector<int> topo_;
  std::vector<uint8_t> cur_, next_;
  float acc_ = 0.f;
  int curLine_ = -1;
  int curNodeIdx_ = -1;       // Current node being evaluated (for visualization)
  int lastVisibleLine_ = -1;  // Last non-internal node line for highlighting
  int lastVisibleNodeIdx_ = -1;
  size_t stepIdx_ = 0;        // Which node in topo_ we're at during slow-step
  bool stepping_ = false;     // Are we in the middle of a slow-step cycle?
  bool hasCycles_ = false;   // Whether the circuit has cycles
  std::unordered_map<int, bool> latch_, momentary_; // by node index

  void stepOnce_();           // Full step (all nodes at once)
  void stepOneNode_();        // Step single node (for visualization)
  void finishStep_();         // Finish the current step cycle
  bool evaluateNode_(int nodeIdx);  // Evaluate a single node
  int findBtnIndex(const std::string& btnName) const;
};

