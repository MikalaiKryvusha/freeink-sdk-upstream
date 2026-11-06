#include <FrontlightPwmDither.h>

#include <cassert>
#include <cmath>
#include <cstdio>
#include <initializer_list>
#include <limits>

int main() {
  // Exact whole-channel targets must not keep a software sampling timer alive.
  for (unsigned total = 1; total <= 2047; ++total) {
    for (unsigned warmth = 0; warmth <= 100; ++warmth) {
      if (total * warmth % 100 != 0) continue;
      FrontlightPwmDither dither;
      const auto s = dither.next(total, warmth, 2047);
      assert(!dither.pending());
      assert(s.warm == total * warmth / 100 && s.cool + s.warm == total);
    }
  }
  for (unsigned percent = 1; percent <= 100; ++percent) {
    for (unsigned warmth = 0; warmth <= 100; ++warmth) {
      FrontlightPwmDither dither;
      const float total = static_cast<float>(percent);
      const double idealWarm = total * warmth / 100.0;
      const double idealCool = total - idealWarm;
      unsigned warmSum = 0, coolSum = 0;
      for (unsigned i = 1; i <= 400; ++i) {
        const auto s = dither.next(total, warmth, 2047);
        warmSum += s.warm;
        coolSum += s.cool;
        assert(std::abs(warmSum - i * idealWarm) < 1.1);
        assert(std::abs(coolSum - i * idealCool) < 1.1);
        if (warmth == 50) assert(s.cool == s.warm);
        if (warmth == 0) assert(s.warm == 0);
        if (warmth == 100) assert(s.cool == 0);
      }
    }
  }

  for (unsigned warmth : {0u, 1u, 30u, 50u, 70u, 99u, 100u}) {
    FrontlightPwmDither dither;
    double idealCool = 0, idealWarm = 0;
    unsigned coolSum = 0, warmSum = 0;
    for (unsigned i = 1; i < 4000; ++i) {
      const float k = i / 4000.0f;
      const float total = 7.0f * k * k;
      const auto s = dither.next(total, warmth, 2047);
      coolSum += s.cool;
      warmSum += s.warm;
      idealWarm += total * warmth / 100.0;
      idealCool += total * (100 - warmth) / 100.0;
      assert(std::abs(warmSum - idealWarm) < 1.1);
      assert(std::abs(coolSum - idealCool) < 1.1);
      if (warmth == 50) assert(s.cool == s.warm);
    }
    // Off must not emit a residual pulse; pure-channel full output needs no timer.
    auto s = dither.next(0, warmth, 2047);
    assert(s.cool == 0 && s.warm == 0 && !dither.pending());
    s = dither.next(2047, 0, 2047);
    assert(s.cool == 2047 && s.warm == 0 && !dither.pending());
    s = dither.next(2047, 100, 2047);
    assert(s.cool == 0 && s.warm == 2047 && !dither.pending());
    s = dither.next(std::numeric_limits<float>::quiet_NaN(), 50, 2047);
    assert(s.cool == 0 && s.warm == 0 && !dither.pending());
  }
  std::puts(
      "Per-channel dither: all brightness/warmth mixes, fractional fade energy, neutral symmetry and off/full passed");
}
