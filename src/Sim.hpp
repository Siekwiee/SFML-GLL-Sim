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
  // Query BTN state
  bool isButtonPressed(const std::string& btnName) const;
  bool isButtonLatched(const std::string& btnName) const;
  bool getSignalValue(const std::string& signalName) const;
  // Present time control hook, getter and status
  void setPresetTime(const std::string& gateName, float seconds);
  float getPresetTime(const std::string& gateName);
  bool getTGateStatus(const std::string& gateName);
  void setTGateStatus(const std::string& gateName, bool status);

  // Counter control hooks
  void setPresetCounterValue(const std::string& gateName, int value);
  int getPresetCounterValue(const std::string& gateName);
  void setCurrentCounterValue(const std::string& gateName, int value);
  int getCurrentCounterValue(const std::string& gateName);

  // Direct signal control (for IN signals without BTN nodes)
  void toggleSignal(const std::string& signalName);
  void setSignal(const std::string& signalName, bool value);
  
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
  bool hasCycles_ = false;   // Whether the circuit has cycles (unused for execution now)
  std::vector<uint8_t> prevStateAtCycleStart_; // State at start of cycle for UI feedback
  std::unordered_map<int, bool> latch_, momentary_; // by node index
  std::unordered_map<int, bool> pendingLatch_, pendingMomentary_; // buffered inputs
  std::unordered_map<int, uint8_t> pendingSignals_; // buffered signal changes
  std::unordered_map<std::string, float> presentTimeSeconds;
  std::unordered_map<std::string, bool> nodeStatus;
  std::unordered_map<std::string, float> timerElapsedTime; // Elapsed time for each timer (in seconds)
  
  std::unordered_map<std::string, int> presetCounterValue;
  std::unordered_map<std::string, int> currentCounterValue;
  std::unordered_map<std::string, bool> counterPrevInput;
  std::unordered_map<std::string, bool> psPrevInput; // Previous input state for PS (rising edge) nodes
  std::unordered_map<std::string, bool> nsPrevInput; // Previous input state for NS (falling edge) nodes

  void stepOnce_();           // Full step (all nodes at once)
  void stepOneNode_();        // Step single node (for visualization)
  void finishStep_();         // Finish the current step cycle
  void commitPendingInputs_(); // Apply buffered inputs at start of cycle
  bool castSignalToBool_(int sigIdx){
    if (sigIdx < 0 || sigIdx >= static_cast<int>(next_.size())) {
        return false;
      } else {
        return (next_[sigIdx] != 0);
      }
  };
  bool evaluateNode_(int nodeIdx);  // Evaluate a single node
  int findBtnIndex(const std::string& btnName) const;
};

