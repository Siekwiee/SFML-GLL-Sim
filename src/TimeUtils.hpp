#pragma once
#include <string>
#include <cctype>
#include <algorithm>
#include <cmath>

inline float parseTimeStringToFloat(const std::string& timeString) {
  if (timeString.empty()) {
    return 3.0f;
  } else {
    // parse number (amount of time)
    size_t end = 0;
    while (end < timeString.size() && (std::isdigit(timeString[end]) || timeString[end] == '.')) {
      end++;
    }
    if (end == 0) return 3.0f;  // No number found, return default
    
    try {
        float number = std::stof(timeString.substr(0, end));
        // parse format (ms milliseconds, s seconds, m minutes, h hours)
        if (end < timeString.size()) {
            std::string unitStr = "";
            for (size_t i = end; i < timeString.size(); ++i) {
                unitStr += static_cast<char>(std::tolower(timeString[i]));
            }
            if (unitStr == "ms") {
                return number / 1000.0f;
            } else if (unitStr == "m") {
                return number * 60.0f;
            } else if (unitStr == "h") {
                return number * 3600.0f;
            }
        }
        return number;
    } catch (...) {
        return 3.0f;
    }
  }
}

inline std::string parseFloatToTimeString(float floatInSeconds) {
  if (floatInSeconds <= 0.0f) {
    return "3s";
  }
  
  if (floatInSeconds >= 3600.0f && std::fmod(floatInSeconds, 3600.0f) < 0.01f) {
    return std::to_string(static_cast<int>(floatInSeconds / 3600.0f)) + "h";
  }
  else if (floatInSeconds >= 60.0f && std::fmod(floatInSeconds, 60.0f) < 0.01f) {
    return std::to_string(static_cast<int>(floatInSeconds / 60.0f)) + "m";
  }
  else if (floatInSeconds < 1.0f || std::fmod(floatInSeconds, 1.0f) > 0.001f) {
    // If less than 1s or has fractional part, show as milliseconds
    return std::to_string(static_cast<int>(std::round(floatInSeconds * 1000.0f))) + "ms";
  }
  else {
    return std::to_string(static_cast<int>(std::round(floatInSeconds))) + "s";
  }
}
