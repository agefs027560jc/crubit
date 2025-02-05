// Part of the Crubit project, under the Apache License v2.0 with LLVM
// Exceptions. See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include "nullability/inference/infer_tu.h"

#include <optional>
#include <vector>

#include "nullability/inference/inference.proto.h"
#include "nullability/proto_matchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Index/USRGeneration.h"
#include "clang/Testing/TestAST.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringRef.h"
#include "third_party/llvm/llvm-project/third-party/unittest/googlemock/include/gmock/gmock.h"
#include "third_party/llvm/llvm-project/third-party/unittest/googletest/include/gtest/gtest.h"

namespace clang::tidy::nullability {
namespace {
using ast_matchers::hasName;

MATCHER_P2(inferredSlot, I, Nullability, "") {
  return arg.slot() == I && arg.nullability() == Nullability;
}
MATCHER_P2(inferenceMatcher, USR, SlotsMatcher, "") {
  if (arg.symbol().usr() != USR) return false;
  return testing::ExplainMatchResult(SlotsMatcher, arg.slot_inference(),
                                     result_listener);
}

AST_MATCHER(Decl, isCanonical) { return Node.isCanonicalDecl(); }

class InferTUTest : public ::testing::Test {
  std::optional<TestAST> AST;

 protected:
  void build(llvm::StringRef Code) {
    TestInputs Inputs = Code;
    Inputs.ExtraFiles["nullability.h"] = R"cc(
      template <typename T>
      using Nullable [[clang::annotate("Nullable")]] = T;
      template <typename T>
      using Nonnull [[clang::annotate("Nonnull")]] = T;
    )cc";
    Inputs.ExtraArgs.push_back("-include");
    Inputs.ExtraArgs.push_back("nullability.h");
    AST.emplace(Inputs);
  }

  auto infer() { return inferTU(AST->context()); }

  // Returns a matcher for an Inference.
  // The DeclMatcher should uniquely identify the symbol being described.
  // (We use this to compute the USR we expect to find in the inference proto).
  // Slots should describe the slots that were inferred.
  template <typename MatcherT>
  testing::Matcher<const Inference &> inference(
      MatcherT DeclMatcher,
      std::vector<testing::Matcher<const Inference::SlotInference &>> Slots) {
    llvm::SmallString<128> USR;
    auto Matches = ast_matchers::match(
        ast_matchers::namedDecl(isCanonical(), DeclMatcher).bind("decl"),
        AST->context());
    EXPECT_EQ(Matches.size(), 1);
    if (auto *D = ast_matchers::selectFirst<Decl>("decl", Matches))
      EXPECT_FALSE(index::generateUSRForDecl(D, USR));
    return inferenceMatcher(USR, testing::ElementsAreArray(Slots));
  }
};

TEST_F(InferTUTest, UncheckedDeref) {
  build(R"cc(
    void target(int *p, bool cond) {
      if (cond) *p;
    }

    void guarded(int *p) {
      if (p) *p;
    }
  )cc");

  EXPECT_THAT(infer(),
              ElementsAre(inference(hasName("target"),
                                    {inferredSlot(1, Inference::NONNULL)})));
}

TEST_F(InferTUTest, Samples) {
  llvm::StringRef Code =
      "void target(int * p) { *p + *p; }\n"
      "void another(int x) { target(&x); }";
  //   123456789012345678901234567890123456789
  //   0        1         2         3

  build(Code);
  auto Results = infer();
  ASSERT_THAT(Results,
              ElementsAre(inference(hasName("target"),
                                    {inferredSlot(1, Inference::NONNULL)})));
  EXPECT_THAT(Results.front().slot_inference(0).sample_evidence(),
              testing::UnorderedElementsAre(
                  EqualsProto(R"pb(location: "input.mm:2:30"
                                   kind: NONNULL_ARGUMENT)pb"),
                  EqualsProto(R"pb(location: "input.mm:1:24"
                                   kind: UNCHECKED_DEREFERENCE)pb"),
                  EqualsProto(R"pb(location: "input.mm:1:29"
                                   kind: UNCHECKED_DEREFERENCE)pb")));
}

TEST_F(InferTUTest, Annotations) {
  build(R"cc(
    Nonnull<int *> target(int *a, int *b);
    Nonnull<int *> target(int *a, Nullable<int *> p) { *p; }
  )cc");

  EXPECT_THAT(infer(),
              ElementsAre(inference(hasName("target"),
                                    {
                                        inferredSlot(0, Inference::NONNULL),
                                        inferredSlot(2, Inference::NULLABLE),
                                    })));
}

TEST_F(InferTUTest, AnnotationsConflict) {
  build(R"cc(
    Nonnull<int *> target();
    Nullable<int *> target();
  )cc");

  EXPECT_THAT(infer(),
              ElementsAre(inference(hasName("target"),
                                    {inferredSlot(0, Inference::UNKNOWN)})));
}

TEST_F(InferTUTest, ParamsFromCallSite) {
  build(R"cc(
    void callee(int* p, int* q, int* r);
    void target(int* a, Nonnull<int*> b, Nullable<int*> c) { callee(a, b, c); }
  )cc");

  ASSERT_THAT(infer(),
              Contains(inference(hasName("callee"),
                                 {
                                     inferredSlot(1, Inference::UNKNOWN),
                                     inferredSlot(2, Inference::NONNULL),
                                     inferredSlot(3, Inference::NULLABLE),
                                 })));
}

TEST_F(InferTUTest, ReturnTypeNullable) {
  build(R"cc(
    int* target() { return nullptr; }
  )cc");
  EXPECT_THAT(infer(),
              ElementsAre(inference(hasName("target"),
                                    {inferredSlot(0, Inference::NULLABLE)})));
}

TEST_F(InferTUTest, ReturnTypeNonnull) {
  build(R"cc(
    Nonnull<int*> providesNonnull();
    int* target() { return providesNonnull(); }
  )cc");
  EXPECT_THAT(infer(),
              Contains(inference(hasName("target"),
                                 {inferredSlot(0, Inference::NONNULL)})));
}

TEST_F(InferTUTest, ReturnTypeNonnullAndUnknown) {
  build(R"cc(
    Nonnull<int*> providesNonnull();
    int* target(bool b, int* q) {
      if (b) return q;
      return providesNonnull();
    }
  )cc");
  EXPECT_THAT(infer(),
              Contains(inference(hasName("target"),
                                 {inferredSlot(0, Inference::UNKNOWN)})));
}

TEST_F(InferTUTest, ReturnTypeNonnullAndNullable) {
  build(R"cc(
    Nonnull<int*> providesNonnull();
    int* target(bool b) {
      if (b) return nullptr;
      return providesNonnull();
    }
  )cc");
  EXPECT_THAT(infer(),
              Contains(inference(hasName("target"),
                                 {inferredSlot(0, Inference::NULLABLE)})));
}

}  // namespace
}  // namespace clang::tidy::nullability
