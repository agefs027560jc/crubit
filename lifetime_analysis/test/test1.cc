#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "lifetime_analysis/test/lifetime_analysis_test.h"

namespace clang {
namespace tidy {
namespace lifetimes {
namespace {

TEST_F(LifetimeAnalysisTest, ArrayOfInts) {
  EXPECT_THAT(GetLifetimes(R"(
    #include <cstdio>
    #include <iostream>
    void target() {
      int x[] = {0};
      std::cout << "test";
    }
  )"),
              LifetimesAre({{"target", ""}}));
}


}  // namespace
}  // namespace lifetimes
}  // namespace tidy
}  // namespace clang

