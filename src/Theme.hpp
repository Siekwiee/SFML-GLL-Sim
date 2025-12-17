#pragma once
#include <SFML/Graphics.hpp>

namespace Theme {
  // Background
  inline const sf::Color Background(25, 25, 28);
  
  // Text colors
  inline const sf::Color TextDefault(220, 220, 220);
  inline const sf::Color TextGreen(100, 200, 100);
  inline const sf::Color TextRed(200, 100, 100);
  inline const sf::Color TextYellow(220, 180, 80);
  inline const sf::Color TextOrange(230, 140, 80);
  
  // Highlight colors
  inline const sf::Color LineHighlight(60, 60, 70, 150);
  inline const sf::Color TokenHighlight(80, 80, 100, 100);
  inline const sf::Color BTNHighlight(60, 70, 85, 200);
  
  // BTN states
  inline const sf::Color BTNPressed(80, 180, 120, 220);
  inline const sf::Color BTNLatched(180, 130, 70, 220);
  inline const sf::Color BTNBoth(120, 200, 100, 240);
  
  // UI colors
  inline const sf::Color ButtonDefault(50, 50, 60);
  inline const sf::Color ButtonHover(70, 70, 80);
  inline const sf::Color ButtonActive(90, 90, 100);
  inline const sf::Color ButtonRunning(60, 120, 80);
  inline const sf::Color ErrorColor(180, 60, 60);
  
  // Font settings
  inline constexpr float FontSize = 16.0f;
  inline constexpr float LineHeight = 20.0f;
}

