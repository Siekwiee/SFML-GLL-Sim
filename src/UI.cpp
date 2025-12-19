#include "UI.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <set>
#include <cctype>

UI::UI(const Program& prog, Simulator& sim) 
  : prog_(prog), sim_(sim) {
  loadFont();
  updateSimSpeed();
  wasStepping_ = sim_.isSteppingThrough();
  
  // Layout will be updated when window size is known
  // Default initialization
  updateLayout({1920, 1080});
}

void UI::loadFont() {
  // Try to load Geist font
  if (font_.openFromFile("../vendored/Geist/static/Geist-Regular.ttf") ||
      font_.openFromFile("vendored/Geist/static/Geist-Regular.ttf") ||
      font_.openFromFile("../../vendored/Geist/static/Geist-Regular.ttf")) {
    fontLoaded_ = true;
  }
}

void UI::updateSimSpeed() {
  // Map slider value (0.0-1.0) to speed (0.5 - 50 Hz)
  // Use exponential scale for better control at low speeds
  simSpeed_ = 0.5f * std::pow(100.0f, sliderValue_);
}

void UI::updateLayout(const sf::Vector2u& windowSize) {
  windowSize_ = windowSize;
  sidebarWidth_ = windowSize.x * 0.3f;
  contentAreaX_ = sidebarWidth_;
  contentAreaWidth_ = windowSize.x * 0.7f;
  
  // Update control buttons in left sidebar (vertical layout)
  float currentY = sidebarPadding_;
  float buttonWidth = sidebarWidth_ - 2 * sidebarPadding_;
  
  // Play button takes 75% width, repeat button takes 25% on the right
  float playButtonWidth = buttonWidth * 0.75f;
  float repeatButtonWidth = buttonWidth * 0.25f - buttonSpacing_ * 0.5f; // Account for spacing
  float repeatButtonX = sidebarPadding_ + playButtonWidth + buttonSpacing_ * 0.5f;
  
  playPauseBtn_ = sf::FloatRect({sidebarPadding_, currentY}, {playButtonWidth, buttonHeight_});
  playRepeatBtn_ = sf::FloatRect({repeatButtonX, currentY}, {repeatButtonWidth, buttonHeight_});
  currentY += buttonHeight_ + buttonSpacing_;
  
  stepBtn_ = sf::FloatRect({sidebarPadding_, currentY}, {buttonWidth, buttonHeight_});
  currentY += buttonHeight_ + buttonSpacing_;
  
  // Speed slider
  float sliderHeight = buttonHeight_;
  speedSlider_ = sf::FloatRect({sidebarPadding_, currentY}, {buttonWidth, sliderHeight});
  
  // Update text start position
  textStartX_ = contentAreaX_ + 20.0f;
  textStartY_ = 20.0f;
  
  // Update BTN widgets
  updateBTNWidgets();
}
static float parseTimeStringToFloat(const std::string& timeString){
  if (timeString.empty()){
    return 3.0f;
  } else{
    // parse number (amount of time)
    size_t end = 0;
    while (end < timeString.size() && (std::isdigit(timeString[end]) || timeString[end] == '.')){
      end++;
    }
    if (end == 0) return 3.0f;  // No number found, return default
    float number = std::stof(timeString.substr(0, end));
    // parse format (s seconds, m minutes, h hours)
    if(end < timeString.size()){
      char unit = std::tolower(timeString[end]);
      if (unit =='m'){
        return number * 60.0f;
      } else if (unit =='h'){
        return number * 3600.0f;
      }
    }
    return number;
  }
}

static std::string parseFloatToTimeString(float floatInSeconds){
  if (floatInSeconds <= 0.0f){
    return "3s";
  }
  
  if (floatInSeconds >= 3600.0f && std::fmod(floatInSeconds, 3600.0f) < 0.01f) {
    return std::to_string(static_cast<int>(floatInSeconds / 3600.0f)) + "h";
  }

  else if (floatInSeconds >= 60.0f && std::fmod(floatInSeconds, 60.0f) < 0.01f) {
    return std::to_string(static_cast<int>(floatInSeconds / 60.0f)) + "m";
  }

  else {
    return std::to_string(static_cast<int>(floatInSeconds)) + "s";
  }
}

void UI::updateBTNWidgets() {
  btnWidgets_.clear();
  tNodeWidgets_.clear();
  inputWidgets_.clear();
  
  if (!fontLoaded_) return;
  
  float widgetY = speedSlider_.position.y + speedSlider_.size.y + buttonSpacing_ * 4;
  float widgetWidth = sidebarWidth_ - 2 * sidebarPadding_;
  float widgetHeight = Theme::LineHeight + 10.0f;
  
  // First, collect which signals already have explicit BTN nodes
  std::set<std::string> btnControlledSignals;
  for (const auto& node : prog_.nodes) {
    if (node.type == Program::Node::BTN) {
      for (int outputSig : node.outputs) {
        for (const auto& [sym, sigId] : prog_.symbolToSignal) {
          if (sigId == outputSig) {
            btnControlledSignals.insert(sym);
            break;
          }
        }
      }
    }
  }
  
  std::vector<const Program::Node*> tNodes;
  for (const auto& node : prog_.nodes){
     if (node.type == Program::Node::TOF_ || node.type == Program::Node::TON_){
      tNodes.push_back(&node);
     }
  }

  // Create widgets for all IN signals (inputs you can toggle)
  for (const auto& inputName : prog_.inputNames) {
    // Skip if already has an explicit BTN
    if (btnControlledSignals.count(inputName) > 0) continue;
    
    inputWidgets_[inputName] = sf::FloatRect({sidebarPadding_, widgetY}, {widgetWidth, widgetHeight});
    widgetY += widgetHeight + 5.0f;
  }

  // Create widgets for all T nodes
  for (const auto& node : tNodes){
    tNodeWidgets_[node->name] = sf::FloatRect({sidebarPadding_, widgetY}, {widgetWidth, widgetHeight});
    // init text input with default or current preset time
    if (timerTextInputs_.count(node->name) == 0){
      float pt = sim_.getPresetTime(node->name);
      if (pt > 0.0f){
        timerTextInputs_[node->name] = parseFloatToTimeString(pt);
      } else {
        timerTextInputs_[node->name] = "3s";
      }
    }
    widgetY += widgetHeight + 5.0f;
  }
  
  // Create widgets for explicit BTN nodes
  for (const auto& node : prog_.nodes) {
    if (node.type == Program::Node::BTN) {
      btnWidgets_[node.name] = sf::FloatRect({sidebarPadding_, widgetY}, {widgetWidth, widgetHeight});
      widgetY += widgetHeight + 5.0f;
    }
  }
}

void UI::handleEvent(sf::RenderWindow& win, const sf::Event& ev) {
  // Handle text input for timer widgets FIRST (before other events)
  if (isEditingTimer_ && !activeTimerWidget_.empty()) {
    if (auto* textEntered = ev.getIf<sf::Event::TextEntered>()) {
      // Handle printable characters
      char c = static_cast<char>(textEntered->unicode);
      if (std::isprint(c) || c == 's' || c == 'm' || c == 'h' || c == 'S' || c == 'M' || c == 'H') {
        timerTextInputs_[activeTimerWidget_] += c;
      }
    }
    
    if (auto* keyPressed = ev.getIf<sf::Event::KeyPressed>()) {
      if (keyPressed->code == sf::Keyboard::Key::Enter) {
        // Commit changes
        const std::string& text = timerTextInputs_[activeTimerWidget_];
        float seconds = parseTimeStringToFloat(text);
        if (seconds > 0.0f) {
          sim_.setPresetTime(activeTimerWidget_, seconds);
        }
        isEditingTimer_ = false;
        activeTimerWidget_.clear();
        return;  // Don't process other events
      } else if (keyPressed->code == sf::Keyboard::Key::Escape) {
        // Cancel editing - restore original value
        float pt = sim_.getPresetTime(activeTimerWidget_);
        timerTextInputs_[activeTimerWidget_] = parseFloatToTimeString(pt);
        isEditingTimer_ = false;
        activeTimerWidget_.clear();
        return;
      } else if (keyPressed->code == sf::Keyboard::Key::Backspace) {
        // Delete last character
        if (!timerTextInputs_[activeTimerWidget_].empty()) {
          timerTextInputs_[activeTimerWidget_].pop_back();
        }
        return;  // Don't process other events
      }
    }
    // If we're editing, don't process other events
    return;
  }
  
  // SFML 3.x uses variant-based events
  if (auto* keyPressed = ev.getIf<sf::Event::KeyPressed>()) {
    if (keyPressed->code == sf::Keyboard::Key::Space) {
      running_ = !running_;
      wasStepping_ = sim_.isSteppingThrough(); // Reset tracking when toggling run state
    } else if (keyPressed->code == sf::Keyboard::Key::Period) {
      stepRequested_ = true;
    } else if (keyPressed->code == sf::Keyboard::Key::Equal) {
      sliderValue_ = std::min(1.0f, sliderValue_ + 0.1f);
      updateSimSpeed();
    } else if (keyPressed->code == sf::Keyboard::Key::Hyphen) {
      sliderValue_ = std::max(0.0f, sliderValue_ - 0.1f);
      updateSimSpeed();
    }
  }
  
  if (auto* mousePressed = ev.getIf<sf::Event::MouseButtonPressed>()) {
    sf::Vector2f mousePos(static_cast<float>(mousePressed->position.x), static_cast<float>(mousePressed->position.y));
    
    // Check BTN widgets
    bool ctrlPressed = mousePressed->button == sf::Mouse::Button::Left && 
                       (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl) || 
                        sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RControl));
    
    for (const auto& [btnName, rect] : btnWidgets_) {
      if (isPointInRect(mousePos, rect)) {
        if (ctrlPressed) {
          sim_.toggleLatch(btnName);
        } else {
          mouseDown_ = true;
          activeBtn_ = btnName;
          sim_.setMomentary(btnName, true);
        }
        return;
      }
    }
    
    // Check input signal widgets (click to toggle)
    for (const auto& [inputName, rect] : inputWidgets_) {
      if (isPointInRect(mousePos, rect)) {
        sim_.toggleSignal(inputName);
        return;
      }
    }
    
    // Check T node widgets (click to start editing)
    for (const auto& [nodeName, rect] : tNodeWidgets_) {
      if (isPointInRect(mousePos, rect)) {
        // Start editing this timer widget
        activeTimerWidget_ = nodeName;
        isEditingTimer_ = true;
        // Initialize text if empty
        if (timerTextInputs_.count(nodeName) == 0) {
          float pt = sim_.getPresetTime(nodeName);
          timerTextInputs_[nodeName] = parseFloatToTimeString(pt);
        }
        return;
      }
    }
    
    // If clicking elsewhere while editing, commit changes
    if (isEditingTimer_ && !activeTimerWidget_.empty()) {
      const std::string& text = timerTextInputs_[activeTimerWidget_];
      float seconds = parseTimeStringToFloat(text);
      if (seconds > 0.0f) {
        sim_.setPresetTime(activeTimerWidget_, seconds);
      }
      isEditingTimer_ = false;
      activeTimerWidget_.clear();
    }
    
    // Check control buttons
    if (isPointInRect(mousePos, playPauseBtn_)) {
      running_ = !running_;
      wasStepping_ = sim_.isSteppingThrough(); // Reset tracking when toggling run state
    } else if (isPointInRect(mousePos, playRepeatBtn_)) {
      repeatEnabled_ = !repeatEnabled_;
    } else if (isPointInRect(mousePos, stepBtn_)) {
      stepRequested_ = true;
    } else if (isPointInRect(mousePos, speedSlider_)) {
      float relX = mousePos.x - speedSlider_.position.x;
      sliderValue_ = std::clamp(relX / speedSlider_.size.x, 0.0f, 1.0f);
      updateSimSpeed();
    }
  }
  
  if (auto* mouseReleased = ev.getIf<sf::Event::MouseButtonReleased>()) {
    if (mouseDown_ && !activeBtn_.empty()) {
      sim_.setMomentary(activeBtn_, false);
      mouseDown_ = false;
      activeBtn_.clear();
    }
  }
  
  if (auto* mouseMoved = ev.getIf<sf::Event::MouseMoved>()) {
    if (mouseDown_) {
      sf::Vector2f mousePos(static_cast<float>(mouseMoved->position.x), static_cast<float>(mouseMoved->position.y));
      if (!activeBtn_.empty()) {
        bool stillInWidget = btnWidgets_.count(activeBtn_) > 0 && 
                            isPointInRect(mousePos, btnWidgets_.at(activeBtn_));
        sim_.setMomentary(activeBtn_, stillInWidget);
      }
    }
  }
}

void UI::update(float dt) {
  bool currentlyStepping = sim_.isSteppingThrough();
  if (wasStepping_ && !currentlyStepping && running_) {
    if (!repeatEnabled_) {
      running_ = false;
    }
  }
  wasStepping_ = currentlyStepping;
}

void UI::draw(sf::RenderWindow& win) {
  sf::Vector2u currentSize = win.getSize();
  if (currentSize != windowSize_) {
    updateLayout(currentSize);
  }
  
  drawControls(win);
  drawBTNWidgets(win);
  drawTimerWidgets(win);
  drawLineHighlight(win);
  drawText(win);
  drawTokenHighlights(win);
}

void UI::drawControls(sf::RenderWindow& win) {
  if (!fontLoaded_) return;
  
  // Draw sidebar background first
  sf::RectangleShape sidebarBg(sf::Vector2f(sidebarWidth_, static_cast<float>(windowSize_.y)));
  sidebarBg.setPosition({0, 0});
  sidebarBg.setFillColor(sf::Color(Theme::Background.r + 10, Theme::Background.g + 10, Theme::Background.b + 10));
  win.draw(sidebarBg);
  
  // Draw divider line
  sf::RectangleShape divider(sf::Vector2f(2.0f, static_cast<float>(windowSize_.y)));
  divider.setPosition({sidebarWidth_, 0});
  divider.setFillColor(sf::Color(Theme::TextDefault.r, Theme::TextDefault.g, Theme::TextDefault.b, 50));
  win.draw(divider);
  
  // Check if simulation is valid
  bool validSim = sim_.isValidTopology();
  
  // Play/Pause button - color changes based on running state
  sf::RectangleShape playPauseRect(playPauseBtn_.size);
  playPauseRect.setPosition(playPauseBtn_.position);
  if (!validSim) {
    playPauseRect.setFillColor(Theme::ErrorColor);
  } else if (running_) {
    playPauseRect.setFillColor(Theme::ButtonRunning);
  } else {
    playPauseRect.setFillColor(Theme::ButtonDefault);
  }
  win.draw(playPauseRect);
  
  std::string playPauseLabel = !validSim ? "ERROR" : (running_ ? "RUNNING" : "PAUSED");
  sf::Text playPauseText(font_, playPauseLabel, 14);
  playPauseText.setPosition(playPauseBtn_.position + sf::Vector2f(10, 10));
  playPauseText.setFillColor(Theme::TextDefault);
  win.draw(playPauseText);
  
  // Repeat button
  sf::RectangleShape repeatRect(playRepeatBtn_.size);
  repeatRect.setPosition(playRepeatBtn_.position);
  if (!validSim) {
    repeatRect.setFillColor(Theme::ErrorColor);
  } else if (repeatEnabled_) {
    repeatRect.setFillColor(Theme::ButtonRunning);
  } else {
    repeatRect.setFillColor(Theme::ButtonDefault);
  }
  win.draw(repeatRect);
  
  sf::Text repeatText(font_, repeatEnabled_ ? "REPEAT" : "ONCE", 12);
  repeatText.setPosition(playRepeatBtn_.position + sf::Vector2f(5, 12));
  repeatText.setFillColor(Theme::TextDefault);
  win.draw(repeatText);
  
  // Step button
  sf::RectangleShape stepRect(stepBtn_.size);
  stepRect.setPosition(stepBtn_.position);
  stepRect.setFillColor(validSim ? Theme::ButtonDefault : Theme::ErrorColor);
  win.draw(stepRect);
  
  sf::Text stepText(font_, "Step [.]", 14);
  stepText.setPosition(stepBtn_.position + sf::Vector2f(10, 10));
  stepText.setFillColor(Theme::TextDefault);
  win.draw(stepText);
  
  // Speed slider
  sf::RectangleShape sliderBg(speedSlider_.size);
  sliderBg.setPosition(speedSlider_.position);
  sliderBg.setFillColor(Theme::ButtonDefault);
  win.draw(sliderBg);
  
  // Filled portion of slider
  sf::RectangleShape sliderFill(sf::Vector2f(sliderValue_ * speedSlider_.size.x, speedSlider_.size.y));
  sliderFill.setPosition(speedSlider_.position);
  sliderFill.setFillColor(sf::Color(80, 100, 120, 150));
  win.draw(sliderFill);
  
  float sliderPos = speedSlider_.position.x + sliderValue_ * speedSlider_.size.x;
  sf::RectangleShape sliderHandle(sf::Vector2f(6, speedSlider_.size.y));
  sliderHandle.setPosition(sf::Vector2f(sliderPos - 3, speedSlider_.position.y));
  sliderHandle.setFillColor(Theme::TextDefault);
  win.draw(sliderHandle);
  
  // Speed text below slider
  char speedStr[32];
  snprintf(speedStr, sizeof(speedStr), "Speed: %.1f Hz [+/-]", simSpeed_);
  sf::Text speedText(font_, speedStr, 12);
  speedText.setPosition(speedSlider_.position + sf::Vector2f(5, speedSlider_.size.y + 5));
  speedText.setFillColor(Theme::TextDefault);
  win.draw(speedText);
  
  // Show error message if topology invalid
  if (!validSim) {
    sf::Text errorText(font_, "Invalid circuit topology!", 12);
    float textWidth = errorText.getLocalBounds().size.x;
    errorText.setPosition(sf::Vector2f(sidebarWidth_ - textWidth - sidebarPadding_, speedSlider_.position.y + speedSlider_.size.y + 35));
    errorText.setFillColor(Theme::TextRed);
    win.draw(errorText);
  }
  
  // Show stepping indicator when in slow-step mode
  if (sim_.isSteppingThrough()) {
    sf::Text stepText(font_, "Stepping...", 12);
    float textWidth = stepText.getLocalBounds().size.x;
    stepText.setPosition(sf::Vector2f(sidebarWidth_ - textWidth - sidebarPadding_, speedSlider_.position.y + speedSlider_.size.y + 35));
    stepText.setFillColor(Theme::TextYellow);
    win.draw(stepText);
  }
}

void UI::drawText(sf::RenderWindow& win) {
  if (!fontLoaded_) return;
  
  float y = textStartY_;
  
  for (size_t i = 0; i < prog_.sourceLines.size(); ++i) {
    sf::Text text(font_, prog_.sourceLines[i], static_cast<unsigned int>(Theme::FontSize));
    text.setPosition(sf::Vector2f(textStartX_, y));
    text.setFillColor(Theme::TextDefault);
    win.draw(text);
    y += Theme::LineHeight;
  }
}

void UI::drawTokenHighlights(sf::RenderWindow& win) {
  if (!fontLoaded_) return;
  
  const auto& signals = sim_.signals();
  
  for (const auto& token : prog_.tokens) {
    if (token.line >= static_cast<int>(prog_.sourceLines.size())) continue;
    
    // Check if this token is a signal
    bool isSignal = prog_.symbolToSignal.count(token.symbol) > 0;
    
    if (isSignal) {
      int sigId = prog_.symbolToSignal.at(token.symbol);
      sf::Color color = getSignalColor(sigId);
      
      // Measure actual text positions
      const std::string& line = prog_.sourceLines[token.line];
      sf::Text prefixText(font_, line.substr(0, token.col0), static_cast<unsigned int>(Theme::FontSize));
      sf::Text tokenText(font_, token.symbol, static_cast<unsigned int>(Theme::FontSize));
      
      float prefixWidth = prefixText.getLocalBounds().size.x;
      float tokenWidth = tokenText.getLocalBounds().size.x;
      
      float x = textStartX_ + prefixWidth;
      float y = textStartY_ + token.line * Theme::LineHeight;
      
      sf::RectangleShape highlight(sf::Vector2f(tokenWidth, Theme::LineHeight));
      highlight.setPosition(sf::Vector2f(x, y));
      highlight.setFillColor(sf::Color(color.r, color.g, color.b, 100));
      win.draw(highlight);
      
      // Redraw token text with color
      tokenText.setPosition(sf::Vector2f(x, y));
      tokenText.setFillColor(color);
      win.draw(tokenText);
    }
  }
}

void UI::drawLineHighlight(sf::RenderWindow& win) {
  int curLine = sim_.currentEvaluatingLine();
  if (curLine < 0 || curLine >= static_cast<int>(prog_.sourceLines.size())) {
    return;
  }
  
  float y = textStartY_ + curLine * Theme::LineHeight;
  
  // Draw highlight bar behind the line
  sf::RectangleShape highlight(sf::Vector2f(contentAreaWidth_ - 40.0f, Theme::LineHeight));
  highlight.setPosition(sf::Vector2f(textStartX_, y));
  highlight.setFillColor(sf::Color(80, 120, 180, 100));
  win.draw(highlight);
  
  // Draw left edge indicator (thick bar)
  sf::RectangleShape indicator(sf::Vector2f(4.0f, Theme::LineHeight));
  indicator.setPosition(sf::Vector2f(textStartX_ - 8.0f, y));
  indicator.setFillColor(sf::Color(100, 180, 255, 255));
  win.draw(indicator);
  
  // Draw arrow pointer
  sf::ConvexShape arrow;
  arrow.setPointCount(3);
  float arrowSize = 8.0f;
  float arrowX = textStartX_ - 18.0f;
  float arrowY = y + Theme::LineHeight / 2.0f;
  arrow.setPoint(0, sf::Vector2f(arrowX, arrowY - arrowSize / 2));
  arrow.setPoint(1, sf::Vector2f(arrowX + arrowSize, arrowY));
  arrow.setPoint(2, sf::Vector2f(arrowX, arrowY + arrowSize / 2));
  arrow.setFillColor(sf::Color(100, 180, 255, 255));
  win.draw(arrow);
}

void UI::drawBTNWidgets(sf::RenderWindow& win) {
  if (!fontLoaded_) return;
  
  // Section header
  float headerY = speedSlider_.position.y + speedSlider_.size.y + 60;
  bool hasWidgets = !inputWidgets_.empty() || !btnWidgets_.empty();
  
  if (hasWidgets) {
    sf::Text headerText(font_, "INPUTS (click to toggle)", 11);
    headerText.setPosition(sf::Vector2f(sidebarPadding_, headerY - 25));
    headerText.setFillColor(sf::Color(Theme::TextDefault.r, Theme::TextDefault.g, Theme::TextDefault.b, 150));
    win.draw(headerText);
  }
  
  // Draw input signal widgets (auto-generated for IN signals)
  for (const auto& [inputName, rect] : inputWidgets_) {
    bool signalOn = sim_.getSignalValue(inputName);
    
    sf::Color bgColor = signalOn ? Theme::BTNPressed : Theme::BTNHighlight;
    
    sf::RectangleShape widget(rect.size);
    widget.setPosition(rect.position);
    widget.setFillColor(bgColor);
    widget.setOutlineColor(signalOn ? Theme::TextGreen : Theme::TextDefault);
    widget.setOutlineThickness(signalOn ? 2 : 1);
    win.draw(widget);
    
    std::string label = inputName + (signalOn ? " = 1" : " = 0");
    sf::Text text(font_, label, static_cast<unsigned int>(Theme::FontSize));
    text.setPosition(rect.position + sf::Vector2f(10, 5));
    text.setFillColor(signalOn ? sf::Color::White : Theme::TextDefault);
    win.draw(text);
  }
  
  // Draw explicit BTN widgets
  for (const auto& node : prog_.nodes) {
    if (node.type == Program::Node::BTN && btnWidgets_.count(node.name) > 0) {
      const auto& rect = btnWidgets_.at(node.name);
      
      // Get button state
      bool pressed = sim_.isButtonPressed(node.name);
      bool latched = sim_.isButtonLatched(node.name);
      bool active = pressed || latched;
      
      // Choose color based on state
      sf::Color bgColor = Theme::BTNHighlight;
      if (pressed && latched) {
        bgColor = Theme::BTNBoth;
      } else if (pressed) {
        bgColor = Theme::BTNPressed;
      } else if (latched) {
        bgColor = Theme::BTNLatched;
      }
      
      // Draw widget background
      sf::RectangleShape widget(rect.size);
      widget.setPosition(rect.position);
      widget.setFillColor(bgColor);
      widget.setOutlineColor(active ? Theme::TextGreen : Theme::TextDefault);
      widget.setOutlineThickness(active ? 2 : 1);
      win.draw(widget);
      
      // Find output names
      std::vector<std::string> outputNames;
      for (int outputSig : node.outputs) {
        for (const auto& [sym, sigId] : prog_.symbolToSignal) {
          if (sigId == outputSig) {
            outputNames.push_back(sym);
            break;
          }
        }
      }
      
      if (!outputNames.empty()) {
        std::string label;
        for (size_t i = 0; i < outputNames.size(); ++i) {
          if (i > 0) label += ", ";
          label += outputNames[i];
        }
        if (latched) label += " [HOLD]";
        
        sf::Text text(font_, label, static_cast<unsigned int>(Theme::FontSize));
        text.setPosition(rect.position + sf::Vector2f(10, 5));
        text.setFillColor(active ? sf::Color::White : Theme::TextDefault);
        win.draw(text);
      }
    }
  }
}

void UI::drawTimerWidgets(sf::RenderWindow& win) {
  if (!fontLoaded_) return;  
  // Draw each timer widget
  for (const auto& [nodeName, rect] : tNodeWidgets_) {
    // Get timer status from simulator
    bool isActive = sim_.getTGateStatus(nodeName);
    
    // Choose background color based on status
    sf::Color bgColor;
    if (isActive) {
      // Yellowish when active (timer is ticking)
      bgColor = sf::Color(220, 200, 100, 220);  // Yellowish
    } 
    if(!isActive) {
      // Grayish when inactive
      bgColor = sf::Color(70, 70, 75, 200);  // Dark grayish
    }
    
    // Draw widget background
    sf::RectangleShape widget(rect.size);
    widget.setPosition(rect.position);
    widget.setFillColor(bgColor);
    
    // Highlight outline if currently being edited
    if (isEditingTimer_ && activeTimerWidget_ == nodeName) {
      widget.setOutlineColor(Theme::TextYellow);
      widget.setOutlineThickness(2);
    } else {
      widget.setOutlineColor(isActive ? Theme::TextYellow : Theme::TextDefault);
      widget.setOutlineThickness(isActive ? 2 : 1);
    }
    win.draw(widget);
    
    // Get the text to display
    std::string displayText;
    if (timerTextInputs_.count(nodeName) > 0) {
      displayText = timerTextInputs_[nodeName];
    } else {
      float pt = sim_.getPresetTime(nodeName);
      displayText = parseFloatToTimeString(pt);
    }
    
    // Format label: "NodeName: PT" or "NodeName: [editing]"
    std::string label = nodeName + ": ";
    if (isEditingTimer_ && activeTimerWidget_ == nodeName) {
      label += displayText + "|";  // Cursor indicator
    } else {
      label += displayText;
    }
    
    // Draw text
    sf::Text text(font_, label, static_cast<unsigned int>(Theme::FontSize));
    text.setPosition(rect.position + sf::Vector2f(10, 5));
    text.setFillColor(isActive ? sf::Color::White : Theme::TextDefault);
    win.draw(text);
  }
}

sf::Color UI::getSignalColor(int signalId) const {
  const auto& signals = sim_.signals();
  if (signalId >= 0 && signalId < static_cast<int>(signals.size())) {
    return signals[signalId] != 0 ? Theme::TextGreen : Theme::TextRed;
  }
  return Theme::TextDefault;
}

bool UI::isPointInRect(const sf::Vector2f& point, const sf::FloatRect& rect) const {
  return point.x >= rect.position.x && point.x <= rect.position.x + rect.size.x &&
         point.y >= rect.position.y && point.y <= rect.position.y + rect.size.y;
}
