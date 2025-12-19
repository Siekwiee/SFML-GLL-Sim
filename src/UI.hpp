#pragma once
#include "AST.hpp"
#include "Sim.hpp"
#include "Theme.hpp"
#include <SFML/Graphics.hpp>
#include <string>
#include <unordered_map>

struct UI {
  UI(const Program& prog, Simulator& sim);
  
  void handleEvent(sf::RenderWindow& win, const sf::Event& ev);
  void update(float dt);
  void draw(sf::RenderWindow& win);
  void updateLayout(const sf::Vector2u& windowSize);

  float simSpeed() const { return simSpeed_; }
  bool isRunning() const { return running_; }
  bool stepOnceRequested() { 
    bool val = stepRequested_; 
    stepRequested_ = false; 
    return val; 
  }

private:
  const Program& prog_;
  Simulator& sim_;
  
  sf::Font font_;
  bool fontLoaded_ = false;
  
  bool running_ = false;
  float simSpeed_ = 1.0f; // Hz
  bool stepRequested_ = false;
  bool repeatEnabled_ = true; // Whether to loop the simulation
  bool wasStepping_ = false; // Track previous stepping state to detect cycle completion
  
  // Mouse state
  bool mouseDown_ = false;
  std::string activeBtn_;
  
  // UI layout
  sf::Vector2u windowSize_ = {1920, 1080}; // Default, updated from window
  float sidebarWidth_ = 0.0f; // 30% of window width
  float contentAreaX_ = 0.0f; // Start of content area (70% of window)
  float contentAreaWidth_ = 0.0f; // Width of content area
  
  float textStartX_ = 20.0f;
  float textStartY_ = 20.0f;
  
  // Button positions (in left sidebar, vertical layout)
  sf::FloatRect playPauseBtn_;
  sf::FloatRect playRepeatBtn_;
  sf::FloatRect stepBtn_;
  sf::FloatRect speedSlider_;
  float sliderValue_ = 0.5f; // 0.0 = 0.1x, 1.0 = 10x
  float buttonHeight_ = 40.0f;
  float buttonSpacing_ = 15.0f;
  float sidebarPadding_ = 20.0f;
  
  // BTN widget positions
  std::unordered_map<std::string, sf::FloatRect> btnWidgets_;
  // T (timer node) widget positions
  std::unordered_map<std::string, sf::FloatRect> tNodeWidgets_;
  std::unordered_map<std::string, std::string> timerTextInputs_;
  std::string activeTimerWidget_; 
  bool isEditingTimer_ = false;  
  // Input signal widgets (auto-generated for IN signals without explicit BTN)
  std::unordered_map<std::string, sf::FloatRect> inputWidgets_;
  
  void loadFont();
  void updateSimSpeed();
  void drawText(sf::RenderWindow& win);
  void drawControls(sf::RenderWindow& win);
  void drawTokenHighlights(sf::RenderWindow& win);
  void drawLineHighlight(sf::RenderWindow& win);
  void drawBTNWidgets(sf::RenderWindow& win);
  void drawTimerWidgets(sf::RenderWindow& win);
  sf::Color getSignalColor(int signalId) const;
  void updateBTNWidgets();
  bool isPointInRect(const sf::Vector2f& point, const sf::FloatRect& rect) const;
};

