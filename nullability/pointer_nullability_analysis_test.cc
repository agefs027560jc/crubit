// Part of the Crubit project, under the Apache License v2.0 with LLVM
// Exceptions. See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include "nullability/pointer_nullability_analysis.h"

#include <memory>
#include <optional>
#include <utility>

#include "nullability/pointer_nullability.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclBase.h"
#include "clang/Analysis/CFG.h"
#include "clang/Analysis/FlowSensitive/ControlFlowContext.h"
#include "clang/Analysis/FlowSensitive/DataflowAnalysis.h"
#include "clang/Analysis/FlowSensitive/DataflowAnalysisContext.h"
#include "clang/Analysis/FlowSensitive/DataflowEnvironment.h"
#include "clang/Analysis/FlowSensitive/Value.h"
#include "clang/Analysis/FlowSensitive/WatchedLiteralsSolver.h"
#include "clang/Basic/LLVM.h"
#include "clang/Testing/TestAST.h"
#include "llvm/Support/Error.h"
#include "third_party/llvm/llvm-project/third-party/unittest/googletest/include/gtest/gtest.h"

namespace clang::tidy::nullability {
namespace {

NamedDecl *lookup(StringRef Name, const DeclContext &DC) {
  auto Result = DC.lookup(&DC.getParentASTContext().Idents.get(Name));
  EXPECT_TRUE(Result.isSingleResult()) << Name;
  return Result.front();
}

std::optional<bool> evaluate(const dataflow::Formula &B,
                             dataflow::Environment &Env) {
  if (Env.flowConditionImplies(B)) return true;
  if (Env.flowConditionImplies(Env.arena().makeNot(B))) return false;
  return std::nullopt;
}

TEST(PointerNullabilityAnalysis, AssignNullabilityVariable) {
  // Annotations on p constrain nullabiilty of the return value.
  // This tests we can compute that relationship symbolically.
  TestAST AST(R"cpp(
    int *target(int *p) {
      int *q = p;
      return q;
    }
  )cpp");
  auto *Target = cast<FunctionDecl>(
      lookup("target", *AST.context().getTranslationUnitDecl()));
  auto *P = Target->getParamDecl(0);

  // Run the analysis, with p's annotations bound to variables.
  dataflow::DataflowAnalysisContext::Options Opts;
  // Track return values, but don't actually descend into callees
  Opts.ContextSensitiveOpts.emplace();
  Opts.ContextSensitiveOpts->Depth = 0;
  dataflow::DataflowAnalysisContext DACtx(
      std::make_unique<dataflow::WatchedLiteralsSolver>(), Opts);
  auto &A = DACtx.arena();
  auto CFCtx = dataflow::ControlFlowContext::build(*Target);
  PointerNullabilityAnalysis Analysis(AST.context());
  auto PN = Analysis.assignNullabilityVariable(P, A);
  auto ExitState = std::move(
      *cantFail(dataflow::runDataflowAnalysis(
                    *CFCtx, Analysis, dataflow::Environment(DACtx, *Target)))
           .front());
  // Get the nullability model of the return value.
  auto *Ret =
      dyn_cast_or_null<dataflow::PointerValue>(ExitState.Env.getReturnValue());
  ASSERT_NE(Ret, nullptr);
  auto [RetFromNullable, RetNull] = getPointerNullState(*Ret);

  // The param nullability hasn't been fixed.
  EXPECT_EQ(std::nullopt, evaluate(PN.isNonnull(A), ExitState.Env));
  EXPECT_EQ(std::nullopt, evaluate(PN.isNullable(A), ExitState.Env));
  // Nor has the the nullability of the returned pointer.
  EXPECT_EQ(std::nullopt, evaluate(RetFromNullable.formula(), ExitState.Env));
  EXPECT_EQ(std::nullopt, evaluate(RetNull.formula(), ExitState.Env));
  // However, the two are linked as expected.
  EXPECT_EQ(true, evaluate(A.makeImplies(PN.isNonnull(A),
                                         A.makeNot(RetNull.formula())),
                           ExitState.Env));
  EXPECT_EQ(true,
            evaluate(A.makeEquals(PN.isNullable(A), RetFromNullable.formula()),
                     ExitState.Env));
}

}  // namespace
}  // namespace clang::tidy::nullability
