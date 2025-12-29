#include "Sim.hpp"
#include "Graph.hpp"
#include <algorithm>

Simulator::Simulator(const Program &p) : prog_(p)
{
  size_t n = prog_.symbolToSignal.size();
  cur_.assign(n, 0);
  next_ = cur_;
  prevStateAtCycleStart_.assign(n, 0);

  // Compute topological order - cycles are now handled by including all nodes
  bool allNodesIncluded = computeTopologicalOrder(prog_, topo_);
  hasCycles_ = !allNodesIncluded || topo_.size() < prog_.nodes.size();

  // If not all nodes included, try to include them anyway
  if (topo_.size() < prog_.nodes.size())
  {
    // Find missing nodes and add them
    for (size_t i = 0; i < prog_.nodes.size(); ++i)
    {
      bool found = false;
      for (int scheduled : topo_)
      {
        if (static_cast<int>(i) == scheduled)
        {
          found = true;
          break;
        }
      }
      if (!found)
      {
        topo_.push_back(static_cast<int>(i));
      }
    }
    hasCycles_ = true;
  }

  // Initialize hardcoded preset times and counter values
  for (const auto &node : prog_.nodes)
  {
    if ((node.type == Program::Node::TON_ || node.type == Program::Node::TOF_) && node.hardcodedPresetTime > 0.0f)
    {
      setPresetTime(node.name, node.hardcodedPresetTime);
    }
    if (node.type == Program::Node::CTU_ || node.type == Program::Node::CTD_)
    {
      if (node.hardcodedPresetValue >= 0)
      {
        setPresetCounterValue(node.name, node.hardcodedPresetValue);
        if (node.type == Program::Node::CTD_)
        {
          setCurrentCounterValue(node.name, node.hardcodedPresetValue);
        }
      }
    }
  }
}

void Simulator::commitPendingInputs_()
{
  for (auto const &[idx, val] : pendingMomentary_)
  {
    momentary_[idx] = val;
  }
  // We don't clear pendingMomentary because UI sets it every frame while button is held

  for (auto const &[idx, val] : pendingLatch_)
  {
    latch_[idx] = val;
  }
  pendingLatch_.clear();

  for (auto const &[idx, val] : pendingSignals_)
  {
    if (idx >= 0 && idx < static_cast<int>(cur_.size()))
    {
      cur_[idx] = val;
    }
  }
  pendingSignals_.clear();
}

void Simulator::update(float dt, float simHz, bool running, bool stepOnce)
{
  if (topo_.empty() || topo_.size() != prog_.nodes.size())
  {
    return; // Invalid topology
  }

  // Update timer elapsed times if running
  if (running && dt > 0.0f)
  {
    for (const auto &node : prog_.nodes)
    {
      if (node.type == Program::Node::TON_)
      {
        // Check if input is active
        bool inputActive = false;
        if (!node.inputs.empty())
        {
          int inputSig = node.inputs[0];
          if (inputSig >= 0 && inputSig < static_cast<int>(cur_.size()))
          {
            inputActive = (cur_[inputSig] != 0);
          }
        }
        // TON
        if (inputActive)
        {
          timerElapsedTime[node.name] += dt;
        }
        float epTime = timerElapsedTime[node.name];
        if (epTime >= getPresetTime(node.name))
        {
          timerElapsedTime[node.name] = 0.0f;
          setTGateStatus(node.name, true);
        }
        if (!inputActive && getTGateStatus(node.name))
        {
          // Reset
          timerElapsedTime[node.name] = 0.0f;
          setTGateStatus(node.name, false);
        }
      }
      if (node.type == Program::Node::TOF_)
      {
        // Check if input is active
        bool inputActive = false;
        if (!node.inputs.empty())
        {
          int inputSig = node.inputs[0];
          if (inputSig >= 0 && inputSig < static_cast<int>(cur_.size()))
          {
            inputActive = (cur_[inputSig] != 0);
          }
        }
        if (inputActive)
        {
          timerElapsedTime[node.name] = 0.0f;
        }
        else if (!inputActive && getTGateStatus(node.name))
        {
          // TOF: input is low, status is true, timer is counting down
          timerElapsedTime[node.name] += dt;
        }
        float epTime = timerElapsedTime[node.name];
        if (epTime >= getPresetTime(node.name))
        {
          // TOF: timer elapsed, reset
          timerElapsedTime[node.name] = 0.0f;
          setTGateStatus(node.name, false);
        }
      }
    }
  }

  // Manual step button - step one node at a time for visibility
  if (stepOnce)
  {
    if (!stepping_)
    {
      // Apply pending inputs before starting new cycle
      commitPendingInputs_();

      // Start a new step cycle
      stepping_ = true;
      stepIdx_ = 0;
      next_ = cur_;
      lastVisibleLine_ = -1;
      lastVisibleNodeIdx_ = -1;
      curLine_ = -1; // Reset so first node sets it properly
      prevStateAtCycleStart_ = next_;
    }
    stepOneNode_();
    return;
  }

  if (!running || simHz <= 0.0f)
  {
    // If paused, keep showing current evaluation line
    return;
  }

  acc_ += dt;

  // Speed controls how fast we step through nodes for visualization
  // simHz is "nodes per second" - we step one node at a time to show the blue highlight
  float stepTime = 1.0f / simHz;

  // Step through nodes one at a time for visualization
  // This shows the blue line highlight as each node is evaluated
  while (acc_ >= stepTime)
  {
    if (!stepping_)
    {
      // Apply pending inputs before starting new cycle
      commitPendingInputs_();

      stepping_ = true;
      stepIdx_ = 0;
      next_ = cur_; // Start new cycle with current state
      lastVisibleLine_ = -1;
      lastVisibleNodeIdx_ = -1;
      curLine_ = -1;                  // Reset to ensure first node sets it properly
      prevStateAtCycleStart_ = next_; // Store state at iteration start
    }
    stepOneNode_();
    acc_ -= stepTime;
  }
}

int Simulator::findBtnIndex(const std::string &btnName) const
{
  for (size_t i = 0; i < prog_.nodes.size(); ++i)
  {
    if (prog_.nodes[i].type == Program::Node::BTN && prog_.nodes[i].name == btnName)
    {
      return static_cast<int>(i);
    }
  }
  return -1;
}

void Simulator::setMomentary(const std::string &btnName, bool down)
{
  int idx = findBtnIndex(btnName);
  if (idx >= 0)
  {
    pendingMomentary_[idx] = down;
  }
}

void Simulator::toggleLatch(const std::string &btnName)
{
  int idx = findBtnIndex(btnName);
  if (idx >= 0)
  {
    // We need to know current state to toggle, but for pending we might not have it yet
    // If not in pending, use current latch_
    bool current = latch_[idx];
    if (pendingLatch_.count(idx))
      current = pendingLatch_[idx];
    pendingLatch_[idx] = !current;
  }
}

bool Simulator::isButtonPressed(const std::string &btnName) const
{
  int idx = findBtnIndex(btnName);
  if (idx >= 0)
  {
    // Return pending value if available for immediate UI feedback
    auto itP = pendingMomentary_.find(idx);
    if (itP != pendingMomentary_.end())
      return itP->second;

    auto it = momentary_.find(idx);
    return it != momentary_.end() && it->second;
  }
  return false;
}

bool Simulator::isButtonLatched(const std::string &btnName) const
{
  int idx = findBtnIndex(btnName);
  if (idx >= 0)
  {
    // Return pending value if available for immediate UI feedback
    auto itP = pendingLatch_.find(idx);
    if (itP != pendingLatch_.end())
      return itP->second;

    auto it = latch_.find(idx);
    return it != latch_.end() && it->second;
  }
  return false;
}

void Simulator::setPresetTime(const std::string &gateName, float seconds)
{
  presentTimeSeconds[gateName] = seconds;
}
float Simulator::getPresetTime(const std::string &gateName)
{
  auto it = presentTimeSeconds.find(gateName);
  if (it != presentTimeSeconds.end())
  {
    return it->second;
  }
  return 3.0f;
}
bool Simulator::getTGateStatus(const std::string &gateName)
{
  auto it = nodeStatus.find(gateName);
  if (it != nodeStatus.end())
  {
    return it->second;
  }
  return false;
}

void Simulator::setTGateStatus(const std::string &gateName, bool status)
{
  nodeStatus[gateName] = status;
}

void Simulator::setPresetCounterValue(const std::string &gateName, int value)
{
  presetCounterValue[gateName] = value;
}

int Simulator::getPresetCounterValue(const std::string &gateName)
{
  if (presetCounterValue.count(gateName) > 0)
    return presetCounterValue[gateName];
  return 0;
}

void Simulator::setCurrentCounterValue(const std::string &gateName, int value)
{
  currentCounterValue[gateName] = value;
}

int Simulator::getCurrentCounterValue(const std::string &gateName)
{
  if (currentCounterValue.count(gateName) > 0)
    return currentCounterValue[gateName];
  return 0;
}

void Simulator::toggleSignal(const std::string &signalName)
{
  auto it = prog_.symbolToSignal.find(signalName);
  if (it != prog_.symbolToSignal.end())
  {
    int sigId = it->second;
    if (sigId >= 0 && sigId < static_cast<int>(cur_.size()))
    {
      uint8_t current = cur_[sigId];
      if (pendingSignals_.count(sigId))
        current = pendingSignals_[sigId];
      pendingSignals_[sigId] = current ? 0 : 1;
    }
  }
}

void Simulator::setSignal(const std::string &signalName, bool value)
{
  auto it = prog_.symbolToSignal.find(signalName);
  if (it != prog_.symbolToSignal.end())
  {
    int sigId = it->second;
    if (sigId >= 0 && sigId < static_cast<int>(cur_.size()))
    {
      pendingSignals_[sigId] = value ? 1 : 0;
    }
  }
}

bool Simulator::getSignalValue(const std::string &signalName) const
{
  auto it = prog_.symbolToSignal.find(signalName);
  if (it != prog_.symbolToSignal.end())
  {
    int sigId = it->second;
    if (sigId >= 0 && sigId < static_cast<int>(cur_.size()))
    {
      // Return pending value if available for immediate UI feedback
      auto itP = pendingSignals_.find(sigId);
      if (itP != pendingSignals_.end())
        return itP->second != 0;

      return cur_[sigId] != 0;
    }
  }
  return false;
}

// Evaluate a single node and update its output
bool Simulator::evaluateNode_(int nodeIdx)
{
  const auto &n = prog_.nodes[nodeIdx];
  bool out = false;

  switch (n.type)
  {
  case Program::Node::AND_:
  {
    out = true;
    for (int s : n.inputs)
    {
      bool sigVal = (next_[s] != 0);
      out = out && sigVal;
    }
    break;
  }
  case Program::Node::OR_:
  {
    out = false;
    for (int s : n.inputs)
    {
      bool sigVal = (next_[s] != 0);
      out = out || sigVal;
    }
    break;
  }
  case Program::Node::XOR_:
  {
    out = false;
    for (int s : n.inputs)
    {
      bool sigVal = (next_[s] != 0);
      if (out && sigVal)
      {
        out = false;
        break;
      }
      out = out || sigVal && !out && sigVal;
    }
    break;
  }
  case Program::Node::NOT_:
  {
    if (!n.inputs.empty())
    {
      bool sigVal = (next_[n.inputs[0]] != 0);
      out = !sigVal;
    }
    break;
  }
  case Program::Node::PS_:
  {
    // Positive Signal (Rising Edge) detector
    // Outputs TRUE only when input transitions from FALSE to TRUE
    out = false;
    if (!n.inputs.empty())
    {
      bool currentInput = castSignalToBool_(n.inputs[0]);
      bool prevInput = psPrevInput[n.name]; // defaults to false if not found
      
      // Rising edge: was FALSE, now TRUE
      out = currentInput && !prevInput;
      
      // Update previous state for next evaluation cycle
      psPrevInput[n.name] = currentInput;
    }
    break;
  }
  // Set dominant
  case Program::Node::SR_:
  {
    for (int s : n.inputs)
    {
      bool S = (next_[n.inputs[0]] != 0);
      bool R = (next_[n.inputs[1]] != 0);

      if (S && !R)
        out = true;
      else if (!S && R)
        out = false;
      else if (!S && !R)
      {
        out = !n.outputs.empty() ? (next_[n.outputs[0]] != 0) : false;
      }
      else if (S && R)
        out = true;
    }
    break;
  }
  // Reset dominant
  case Program::Node::RS_:
  {
    for (int s : n.inputs)
    {
      bool S = (next_[n.inputs[0]] != 0);
      bool R = (next_[n.inputs[1]] != 0);

      if (S && !R)
        out = true;
      else if (!S && R)
        out = false;
      else if (!S && !R)
      {
        out = !n.outputs.empty() ? (next_[n.outputs[0]] != 0) : false;
      }
    }
    break;
  }
  case Program::Node::TON_:
  {
    if (n.inputs.empty())
    {
      out = false;
      break;
    }
    // Get input signal value
    int inputSig = n.inputs[0];
    if (inputSig < 0 || inputSig >= static_cast<int>(next_.size()))
    {
      out = false;
      break;
    }
    bool inputActive = (next_[inputSig] != 0);
    float presetTime = getPresetTime(n.name);
    bool status = getTGateStatus(n.name);
    float elapsed = 0.0f;
    auto it = timerElapsedTime.find(n.name);
    if (it != timerElapsedTime.end())
    {
      elapsed = it->second;
    }
    if (inputActive && status)
    {
      out = true;
    }
    if (!inputActive && status)
    {
      setTGateStatus(n.name, false);
      timerElapsedTime[n.name] = 0.0f;
      // if want impulse say out = true;
    }
    if (!inputActive)
    {
      // timer that saves it state???
      timerElapsedTime[n.name] = 0.0f;
    }
    if (inputActive && !status)
    {
      out = false;
    }
    break;
  }
  case Program::Node::TOF_:
  {
    if (n.inputs.empty())
    {
      out = false;
      break;
    }
    // Get input signal value
    int inputSig = n.inputs[0];
    if (inputSig < 0 || inputSig >= static_cast<int>(next_.size()))
    {
      out = false;
      break;
    }
    bool inputActive = (next_[inputSig] != 0);
    // Get preset time and status
    float presetTime = getPresetTime(n.name);
    bool status = getTGateStatus(n.name);
    // Get elapsed time for this timer
    float elapsed = 0.0f;
    auto it = timerElapsedTime.find(n.name);
    if (it != timerElapsedTime.end())
    {
      elapsed = it->second;
    }
    if (inputActive)
    {
      // Input is high - output is high immediately
      out = true;
      setTGateStatus(n.name, true);
    }
    else if (elapsed >= presetTime)
    {
      out = false;
      setTGateStatus(n.name, false);
    }
    else if (!inputActive && getTGateStatus(n.name))
    {
      out = true;
    }
    else
    {
      out = false;
    }
    break;
  }
  case Program::Node::CTU_:
  {
    if (n.inputs.size() < 2)
    {
      out = false;
      break;
    }
    // CTU(PV, CV, CU, R) -> Q
    // In Parser, if PV and CV were hardcoded, they are skipped from inputs.
    // So inputs[0] is CU, inputs[1] is R.
    bool cu = castSignalToBool_(n.inputs[0]);
    bool reset = castSignalToBool_(n.inputs[1]);

    int cv = getCurrentCounterValue(n.name);
    int pv = getPresetCounterValue(n.name);
    bool prevCu = counterPrevInput[n.name];

    if (reset)
    {
      cv = 0;
    }
    else if (cu && !prevCu)
    {
      // Positive edge on CU
      if (cv < 32767)
      {
        cv++;
      }
    }

    setCurrentCounterValue(n.name, cv);
    counterPrevInput[n.name] = cu;
    out = (cv >= pv);
    break;
  }
  case Program::Node::CTD_:
  {
    if (n.inputs.size() < 2)
    {
      out = false;
      break;
    }
    // CTD(PV, CD, LD) -> Q
    bool cd = castSignalToBool_(n.inputs[0]);
    bool load = castSignalToBool_(n.inputs[1]);

    int cv = getCurrentCounterValue(n.name);
    int pv = getPresetCounterValue(n.name);
    bool prevCd = counterPrevInput[n.name];

    if (load)
    {
      cv = pv;
    }
    else if (cd && !prevCd)
    {
      // Positive edge on CD
      if (cv > 0)
      {
        cv--;
      }
    }

    setCurrentCounterValue(n.name, cv);
    counterPrevInput[n.name] = cd;
    out = (cv <= 0);
    break;
  }
  case Program::Node::BTN:
  {
    bool m = momentary_[nodeIdx];
    bool l = latch_[nodeIdx];
    out = m || l;
    break;
  }
  default:
    break;
  }
    for (int outputSig : n.outputs)
    {
      next_[outputSig] = out ? 1 : 0;
    }
    return out;
  }

  // Step through one node at a time (for slow visualization)
  void Simulator::stepOneNode_()
  {
    if (stepIdx_ >= topo_.size())
    {
      finishStep_();
      return;
    }

    int nodeIdx = topo_[stepIdx_];
    const auto &n = prog_.nodes[nodeIdx];

    curNodeIdx_ = nodeIdx;

    // Set line highlight BEFORE evaluating - this ensures it shows immediately
    // Only show line highlight for "real" nodes, not internal _not_ or _ps_ nodes
    // Internal nodes are auto-generated for inline NOT() and PS() and clutter the visualization
    if (n.name.find("_not_") != 0 && n.name.find("_ps_") != 0)
    {
      curLine_ = n.sourceLine;
      lastVisibleLine_ = n.sourceLine;
      lastVisibleNodeIdx_ = nodeIdx;
    }
    else
    {
      // For internal nodes, keep the last visible line but don't update curLine_
      // This prevents the highlight from jumping to internal nodes
    }

    evaluateNode_(nodeIdx);

    stepIdx_++;

    // If we've processed all nodes, finish the step
    if (stepIdx_ >= topo_.size())
    {
      finishStep_();
    }
  }

  // Finish a step cycle - commit next_ to cur_
  void Simulator::finishStep_()
  {
    // Always commit results after one pass (standard PLC scan behavior)
    std::swap(cur_, next_);
    stepping_ = false;
    stepIdx_ = 0;
    curLine_ = lastVisibleLine_;
    curNodeIdx_ = lastVisibleNodeIdx_;
  }

  // Full step - evaluate all nodes at once (for fast simulation)
  void Simulator::stepOnce_()
  {
    // Apply pending inputs before starting cycle
    commitPendingInputs_();

    lastVisibleLine_ = -1;
    lastVisibleNodeIdx_ = -1;
    next_ = cur_;

    // Execute every node exactly once in program order
    for (int nodeIdx : topo_)
    {
      curNodeIdx_ = nodeIdx;
      const auto &node = prog_.nodes[nodeIdx];
      if (node.name.find("_not_") != 0 && node.name.find("_ps_") != 0)
      {
        curLine_ = node.sourceLine;
        lastVisibleLine_ = node.sourceLine;
        lastVisibleNodeIdx_ = nodeIdx;
      }
      evaluateNode_(nodeIdx);
    }

    std::swap(cur_, next_);
    curLine_ = lastVisibleLine_;
    curNodeIdx_ = lastVisibleNodeIdx_;
  }
