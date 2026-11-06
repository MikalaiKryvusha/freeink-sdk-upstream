#pragma once

#include <cmath>
#include <cstdint>

// Quantize each channel after mixing, retaining its fractional duty between samples.
class FrontlightPwmDither {
 public:
  struct Sample {
    uint32_t cool;
    uint32_t warm;
  };

  Sample next(float totalDuty, uint8_t warmth, uint32_t full) {
    if (!std::isfinite(totalDuty) || totalDuty < 0.0f) totalDuty = 0.0f;
    if (totalDuty > static_cast<float>(full)) totalDuty = static_cast<float>(full);
    if (warmth > 100) warmth = 100;
    const float warm = totalDuty * static_cast<float>(warmth) / 100.0f;
    const float cool = totalDuty - warm;
    needsUpdates = warm != std::floor(warm) || cool != std::floor(cool);
    // Returning to neutral must also align the pulse phases left by an asymmetric mix.
    if (warmth == 50) warmError = coolError;
    return {quantize(cool, coolError), quantize(warm, warmError)};
  }

  bool pending() const { return needsUpdates; }

 private:
  static uint32_t quantize(float target, float& error) {
    if (target <= 0.0f) {
      error = 0.0f;
      return 0;
    }
    auto whole = static_cast<uint32_t>(target);
    error += target - static_cast<float>(whole);
    if (error >= 1.0f) {
      error -= 1.0f;
      ++whole;
    }
    return whole;
  }

  float coolError = 0.0f;
  float warmError = 0.0f;
  bool needsUpdates = false;
};
