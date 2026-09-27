// StarCanopy -- a minimal check macro for the tests.
// Copyright (C) 2026 ChalkWalk
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdio>

namespace starcanopy::test {

inline int failures = 0;

// ctest reads this exit code as "skipped" (SKIP_RETURN_CODE in test/CMakeLists.txt).
constexpr int kSkipped = 77;

inline int finish() {
  if (failures) {
    std::printf("%d check(s) failed\n", failures);
  }
  return failures ? 1 : 0;
}

}  // namespace starcanopy::test

#define CHECK(cond)                                                             \
  do {                                                                          \
    if (!(cond)) {                                                              \
      std::printf("%s:%d: check failed: %s\n", __FILE__, __LINE__, #cond);      \
      starcanopy::test::failures++;                                             \
    }                                                                           \
  } while (0)
