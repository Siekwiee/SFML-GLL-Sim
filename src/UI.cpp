#include "UI.hpp"
#include "TimeUtils.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <set>
#include <cctype>
#include <vector>

UI::UI(const Program& prog, Simulator& sim, ModbusManager& modbus) 
  : prog_(prog), sim_(sim), modbus_(modbus) {
  loadFont();
  updateSimSpeed();
  wasStepping_ = sim_.isSteppingThrough();
  
  // Initialize Modbus input strings
  ipInput_ = modbus_.getIp();
  portInput_ = std::to_string(modbus_.getPort());
  slaveIdInput_ = std::to_string(modbus_.getSlaveId());
  numInputsInput_ = std::to_string(modbus_.getNumInputs());
  numOutputsInput_ = std::to_string(modbus_.getNumOutputs());

  // Layout will be updated when window size is known
  // Default initialization
  updateLayout({1920, 1080});
}

void UI::loadFont() {
  // Potential font paths in order of preference
  std::vector<std::string> fontPaths = {
    // 1. Vendored Geist (relative to executable)
    "../vendored/Geist/static/Geist-Regular.ttf",
    "vendored/Geist/static/Geist-Regular.ttf",
    "../../vendored/Geist/static/Geist-Regular.ttf",
    
    // 2. System-installed Geist (Windows)
    "C:/Windows/Fonts/Geist-Regular.ttf",
    "C:/Windows/Fonts/Geist.ttf",
    
    // 3. Common system fallbacks (Windows)
    "C:/Windows/Fonts/consola.ttf",    // Consolas
    "C:/Windows/Fonts/arial.ttf",      // Arial
    "C:/Windows/Fonts/segoeui.ttf",    // Segoe UI
    "C:/Windows/Fonts/cour.ttf",       // Courier New

    // 4. Linux/macOS system paths (common locations)
    "/usr/share/fonts/truetype/liberation/LiberationMono-Regular.ttf",
    "/usr/share/fonts/TTF/DejaVuSansMono.ttf",
    "/System/Library/Fonts/SFNS.ttf",
    "/System/Library/Fonts/Helvetica.ttc",
    "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"
  };

  for (const auto& path : fontPaths) {
    if (font_.openFromFile(path)) {
      fontLoaded_ = true;
      return; // Found a font, we're good
    }
  }

  // If we reach here, no font was found
  fontLoaded_ = false;
  fprintf(stderr, "Warning: No suitable font found. UI rendering will be disabled.\n");
}

void UI::updateSimSpeed() {
  // Map slider value (0.0-1.0) to speed (0.5 - 2000 Hz)
  // Use exponential scale for better control at low speeds
  simSpeed_ = 0.5f * std::pow(4000.0f, sliderValue_);
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
  
  settingsBtn_ = sf::FloatRect({sidebarPadding_, currentY}, {buttonWidth, buttonHeight_});
  currentY += buttonHeight_ + buttonSpacing_;
  
  // Speed slider
  float sliderHeight = buttonHeight_;
  speedSlider_ = sf::FloatRect({sidebarPadding_, currentY}, {buttonWidth, sliderHeight});
  
  // Update text start position
  textStartX_ = 20.0f;
  textStartY_ = 20.0f;
  
  // Update BTN widgets
  updateBTNWidgets();
}
void UI::updateBTNWidgets() {
  btnWidgets_.clear();
  tNodeWidgets_.clear();
  counterWidgets_.clear();
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

  // Create widgets for all Counter nodes
  for (const auto& node : prog_.nodes) {
    if (node.type == Program::Node::CTU_ || node.type == Program::Node::CTD_) {
      counterWidgets_[node.name] = sf::FloatRect({sidebarPadding_, widgetY}, {widgetWidth, widgetHeight * 2});
      if (counterPVTextInputs_.count(node.name) == 0) {
        counterPVTextInputs_[node.name] = std::to_string(sim_.getPresetCounterValue(node.name));
      }
      widgetY += widgetHeight * 2 + 5.0f;
    }
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
  // Handle window resize to fix scaling and click misalignment
  if (auto* resized = ev.getIf<sf::Event::Resized>()) {
    sf::FloatRect visibleArea({0.f, 0.f}, {static_cast<float>(resized->size.x), static_cast<float>(resized->size.y)});
    win.setView(sf::View(visibleArea));
    updateLayout(resized->size);
  }

  // Handle settings popup events if open
  if (settingsOpen_) {
    if (auto* keyPressed = ev.getIf<sf::Event::KeyPressed>()) {
      if (keyPressed->code == sf::Keyboard::Key::Escape) {
        settingsOpen_ = false;
        return;
      }
    }

    if (auto* textEntered = ev.getIf<sf::Event::TextEntered>()) {
      if (activeInputField_ >= 0) {
        char c = static_cast<char>(textEntered->unicode);
        std::string* target = nullptr;
        if (activeInputField_ == 0) target = &ipInput_;
        else if (activeInputField_ == 1) target = &portInput_;
        else if (activeInputField_ == 2) target = &slaveIdInput_;
        else if (activeInputField_ == 3) target = &numInputsInput_;
        else if (activeInputField_ == 4) target = &numOutputsInput_;

        if (target) {
          if (c == '\b') {
            if (!target->empty()) target->pop_back();
          } else if (std::isprint(c)) {
            *target += c;
          }
        }
      }
    }

    if (auto* mousePressed = ev.getIf<sf::Event::MouseButtonPressed>()) {
      sf::Vector2f mousePos(static_cast<float>(mousePressed->position.x), static_cast<float>(mousePressed->position.y));
      float cardWidth = 400.0f;
      float cardHeight = 400.0f;
      sf::Vector2f cardPos((windowSize_.x - cardWidth) / 2.0f, (windowSize_.y - cardHeight) / 2.0f);

      // Check input fields
      activeInputField_ = -1;
      for (int i = 0; i < 5; ++i) {
        sf::FloatRect fieldRect({cardPos.x + 150, cardPos.y + 70 + i * 40 - 5}, {200, 30});
        if (isPointInRect(mousePos, fieldRect)) {
          activeInputField_ = i;
        }
      }

      // Connect Button
      sf::FloatRect connectBtnRect({cardPos.x + 20, cardPos.y + 340}, {100, 40});
      if (isPointInRect(mousePos, connectBtnRect)) {
        if (modbus_.isConnected()) {
          modbus_.disconnect();
        } else {
          modbus_.setIp(ipInput_);
          try {
            modbus_.setPort(std::stoi(portInput_));
            modbus_.setSlaveId(std::stoi(slaveIdInput_));
            modbus_.setNumInputs(std::stoi(numInputsInput_));
            modbus_.setNumOutputs(std::stoi(numOutputsInput_));
          } catch (...) {}
          modbus_.connect();
        }
      }

      // Close Button
      sf::FloatRect closeBtnRect({cardPos.x + 280, cardPos.y + 340}, {100, 40});
      if (isPointInRect(mousePos, closeBtnRect)) {
        settingsOpen_ = false;
      }
    }
    return; // Block other events when settings are open
  }

  // Handle text input for timer widgets FIRST (before other events)
  if (isEditingTimer_ && !activeTimerWidget_.empty()) {
    // ... (existing timer logic)
  }

  // Handle text input for counter widgets
  if (isEditingCounter_ && !activeCounterWidget_.empty()) {
    if (auto* textEntered = ev.getIf<sf::Event::TextEntered>()) {
      char c = static_cast<char>(textEntered->unicode);
      if (std::isdigit(c) || (c == '-' && counterPVTextInputs_[activeCounterWidget_].empty())) {
        counterPVTextInputs_[activeCounterWidget_] += c;
      }
    }
    
    if (auto* keyPressed = ev.getIf<sf::Event::KeyPressed>()) {
      if (keyPressed->code == sf::Keyboard::Key::Enter) {
        try {
          int val = std::stoi(counterPVTextInputs_[activeCounterWidget_]);
          sim_.setPresetCounterValue(activeCounterWidget_, val);
        } catch (...) {}
        isEditingCounter_ = false;
        activeCounterWidget_.clear();
        return;
      } else if (keyPressed->code == sf::Keyboard::Key::Escape) {
        counterPVTextInputs_[activeCounterWidget_] = std::to_string(sim_.getPresetCounterValue(activeCounterWidget_));
        isEditingCounter_ = false;
        activeCounterWidget_.clear();
        return;
      } else if (keyPressed->code == sf::Keyboard::Key::Backspace) {
        if (!counterPVTextInputs_[activeCounterWidget_].empty()) counterPVTextInputs_[activeCounterWidget_].pop_back();
        return;
      }
    }
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
        // Check if this node has a hardcoded preset time
        bool hardcoded = false;
        for (const auto& node : prog_.nodes) {
          if (node.name == nodeName && node.hardcodedPresetTime > 0.0f) {
            hardcoded = true;
            break;
          }
        }
        
        if (hardcoded) {
          // Maybe show a message or just don't allow editing
          // For now, just don't allow editing
          return;
        }

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
    
    // Check Counter node widgets (click to start editing)
    for (const auto& [nodeName, rect] : counterWidgets_) {
      if (isPointInRect(mousePos, rect)) {
        bool hardcodedPV = false;
        for (const auto& node : prog_.nodes) {
          if (node.name == nodeName) {
            hardcodedPV = node.hardcodedPresetValue >= 0;
            break;
          }
        }
        
        if (!hardcodedPV) {
          isEditingCounter_ = true;
          activeCounterWidget_ = nodeName;
        }
        return;
      }
    }
    
    // If clicking elsewhere while editing counters, commit changes
    if (isEditingCounter_ && !activeCounterWidget_.empty()) {
      try {
        int val = std::stoi(counterPVTextInputs_[activeCounterWidget_]);
        sim_.setPresetCounterValue(activeCounterWidget_, val);
      } catch (...) {}
      isEditingCounter_ = false;
      activeCounterWidget_.clear();
    }

    // Check control buttons
    if (isPointInRect(mousePos, playPauseBtn_)) {
      running_ = !running_;
      wasStepping_ = sim_.isSteppingThrough(); // Reset tracking when toggling run state
    } else if (isPointInRect(mousePos, playRepeatBtn_)) {
      repeatEnabled_ = !repeatEnabled_;
    } else if (isPointInRect(mousePos, stepBtn_)) {
      stepRequested_ = true;
    } else if (isPointInRect(mousePos, settingsBtn_)) {
      settingsOpen_ = true;
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

  if (auto* scroll = ev.getIf<sf::Event::MouseWheelScrolled>()) {
    if (scroll->wheel == sf::Mouse::Wheel::Vertical) {
      if (scroll->position.x > static_cast<int>(sidebarWidth_)) {
        scrollOffset_ -= scroll->delta * 40.0f; // Scroll speed
        
        // Clamp scrollOffset_
        float maxScroll = std::max(0.0f, static_cast<float>(prog_.sourceLines.size()) * Theme::LineHeight + 40.0f - (windowSize_.y));
        if (scrollOffset_ < 0.0f) scrollOffset_ = 0.0f;
        if (scrollOffset_ > maxScroll) scrollOffset_ = maxScroll;
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
  drawCounterWidgets(win);
  
  // Create a view for the content area to handle clipping and scrolling
  sf::View oldView = win.getView();
  
  sf::View contentView;
  float contentXNorm = static_cast<float>(sidebarWidth_) / currentSize.x;
  float contentWNorm = static_cast<float>(contentAreaWidth_) / currentSize.x;
  
  contentView.setViewport(sf::FloatRect({contentXNorm, 0.0f}, {contentWNorm, 1.0f}));
  contentView.setSize({static_cast<float>(contentAreaWidth_), static_cast<float>(currentSize.y)});
  // The center is at half size, but moved down by scrollOffset_
  contentView.setCenter({contentAreaWidth_ / 2.0f, currentSize.y / 2.0f + scrollOffset_});
  
  win.setView(contentView);
  
  drawLineHighlight(win);
  drawText(win);
  drawTokenHighlights(win);
  
  win.setView(oldView);
  
  drawSettingsPopup(win);
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

  // Settings button
  sf::RectangleShape settingsRect(settingsBtn_.size);
  settingsRect.setPosition(settingsBtn_.position);
  settingsRect.setFillColor(modbus_.isConnected() ? Theme::ButtonRunning : Theme::ButtonDefault);
  win.draw(settingsRect);

  sf::Text settingsText(font_, modbus_.isConnected() ? "Modbus: Connected" : "Modbus: Settings", 14);
  settingsText.setPosition(settingsBtn_.position + sf::Vector2f(10, 10));
  settingsText.setFillColor(Theme::TextDefault);
  win.draw(settingsText);
  
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
    // Check if this node has a hardcoded preset time
    bool hardcoded = false;
    for (const auto& node : prog_.nodes) {
      if (node.name == nodeName && node.hardcodedPresetTime > 0.0f) {
        hardcoded = true;
        break;
      }
    }

    // Get timer status from simulator
    bool isActive = sim_.getTGateStatus(nodeName);
    
    // Choose background color based on status
    sf::Color bgColor;
    if (isActive) {
      // Yellowish when active (timer is ticking)
      bgColor = sf::Color(220, 200, 100, 220);  // Yellowish
    } else {
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
    
    // If hardcoded, use a different style
    if (hardcoded) {
      widget.setOutlineColor(sf::Color(100, 100, 100, 150));
      widget.setOutlineThickness(1);
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
      if (hardcoded) label += " (fixed)";
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

void UI::drawSettingsPopup(sf::RenderWindow& win) {
  if (!settingsOpen_ || !fontLoaded_) return;

  // Semi-transparent overlay
  sf::RectangleShape overlay(sf::Vector2f(static_cast<float>(windowSize_.x), static_cast<float>(windowSize_.y)));
  overlay.setFillColor(sf::Color(0, 0, 0, 150));
  win.draw(overlay);

  // Popup card
  float cardWidth = 400.0f;
  float cardHeight = 400.0f;
  sf::Vector2f cardPos((windowSize_.x - cardWidth) / 2.0f, (windowSize_.y - cardHeight) / 2.0f);

  sf::RectangleShape card(sf::Vector2f(cardWidth, cardHeight));
  card.setPosition(cardPos);
  card.setFillColor(sf::Color(45, 45, 50));
  card.setOutlineColor(sf::Color(100, 100, 110));
  card.setOutlineThickness(2);
  win.draw(card);

  sf::Text title(font_, "Modbus TCP Settings", 20);
  title.setPosition(cardPos + sf::Vector2f(20, 20));
  title.setFillColor(Theme::TextDefault);
  win.draw(title);

  float currentY = cardPos.y + 70;
  auto drawInput = [&](const std::string& label, const std::string& value, int id) {
    sf::Text l(font_, label, 14);
    l.setPosition({cardPos.x + 20, currentY});
    l.setFillColor(Theme::TextDefault);
    win.draw(l);

    sf::RectangleShape inputBg(sf::Vector2f(200, 30));
    inputBg.setPosition({cardPos.x + 150, currentY - 5});
    inputBg.setFillColor(activeInputField_ == id ? sf::Color(60, 60, 70) : sf::Color(30, 30, 35));
    inputBg.setOutlineColor(activeInputField_ == id ? Theme::TextYellow : sf::Color(80, 80, 80));
    inputBg.setOutlineThickness(1);
    win.draw(inputBg);

    sf::Text v(font_, value + (activeInputField_ == id ? "|" : ""), 14);
    v.setPosition({cardPos.x + 160, currentY});
    v.setFillColor(Theme::TextDefault);
    win.draw(v);

    currentY += 40;
  };

  drawInput("IP Address:", ipInput_, 0);
  drawInput("Port:", portInput_, 1);
  drawInput("Slave ID:", slaveIdInput_, 2);
  drawInput("Num Inputs:", numInputsInput_, 3);
  drawInput("Num Outputs:", numOutputsInput_, 4);

  // Status message
  if (!modbus_.getLastError().empty()) {
    sf::Text err(font_, modbus_.getLastError(), 12);
    err.setPosition({cardPos.x + 20, currentY});
    err.setFillColor(Theme::TextRed);
    win.draw(err);
  } else if (modbus_.isConnected()) {
    sf::Text status(font_, "Connected", 12);
    status.setPosition({cardPos.x + 20, currentY});
    status.setFillColor(Theme::TextGreen);
    win.draw(status);
  }
    currentY = cardPos.y + 340;

  // Connect/Disconnect Button
  sf::RectangleShape btn(sf::Vector2f(100, 40));
  btn.setPosition({cardPos.x + 20, currentY});
  btn.setFillColor(modbus_.isConnected() ? Theme::ErrorColor : Theme::ButtonRunning);
  win.draw(btn);

  sf::Text btnText(font_, modbus_.isConnected() ? "Disconnect" : "Connect", 14);
  btnText.setPosition({cardPos.x + 30, currentY + 10});
  btnText.setFillColor(sf::Color::White);
  win.draw(btnText);

  // Close Button
  sf::RectangleShape closeBtn(sf::Vector2f(100, 40));
  closeBtn.setPosition({cardPos.x + 280, currentY});
  closeBtn.setFillColor(Theme::ButtonDefault);
  win.draw(closeBtn);

  sf::Text closeText(font_, "Close", 14);
  closeText.setPosition({cardPos.x + 310, currentY + 10});
  closeText.setFillColor(sf::Color::White);
  win.draw(closeText);
}

void UI::drawCounterWidgets(sf::RenderWindow& win) {
  if (!fontLoaded_) return;

  for (const auto& [nodeName, rect] : counterWidgets_) {
    bool isCTU = false;
    bool hardcodedPV = false;
    for (const auto& node : prog_.nodes) {
        if (node.name == nodeName) {
            isCTU = (node.type == Program::Node::CTU_);
            hardcodedPV = (node.hardcodedPresetValue >= 0);
            break;
        }
    }

    bool isEditingThis = (isEditingCounter_ && activeCounterWidget_ == nodeName);
    bool isActive = (sim_.currentEvaluatingNode() != -1 && prog_.nodes[sim_.currentEvaluatingNode()].name == nodeName);

    // Draw background
    sf::RectangleShape widget(rect.size);
    widget.setPosition(rect.position);
    widget.setFillColor(sf::Color(35, 35, 40));
    widget.setOutlineColor(isActive ? Theme::TextYellow : Theme::TextDefault);
    widget.setOutlineThickness(isActive ? 2 : 1);
    win.draw(widget);

    // Draw divider
    sf::Vertex line[] = {
        sf::Vertex(rect.position + sf::Vector2f(0, rect.size.y / 2), Theme::TextDefault),
        sf::Vertex(rect.position + sf::Vector2f(rect.size.x, rect.size.y / 2), Theme::TextDefault)
    };
    win.draw(line, 2, sf::PrimitiveType::Lines);

    // PV Display
    std::string pvLabel = nodeName + " PV: ";
    if (isEditingThis) {
        pvLabel += counterPVTextInputs_[nodeName] + "|";
    } else {
        pvLabel += std::to_string(sim_.getPresetCounterValue(nodeName));
        if (hardcodedPV) pvLabel += " (fixed)";
    }
    
    sf::Text pvText(font_, pvLabel, static_cast<unsigned int>(Theme::FontSize));
    pvText.setPosition(rect.position + sf::Vector2f(10, 5));
    pvText.setFillColor(isEditingThis ? Theme::TextYellow : Theme::TextDefault);
    win.draw(pvText);

    // CV Display (Not editable)
    std::string cvLabel = nodeName + " CV: " + std::to_string(sim_.getCurrentCounterValue(nodeName));
    
    sf::Text cvText(font_, cvLabel, static_cast<unsigned int>(Theme::FontSize));
    cvText.setPosition(rect.position + sf::Vector2f(10, rect.size.y / 2 + 5));
    cvText.setFillColor(Theme::TextDefault);
    win.draw(cvText);
  }
}
