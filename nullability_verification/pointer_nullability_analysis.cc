// Part of the Crubit project, under the Apache License v2.0 with LLVM
// Exceptions. See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include "nullability_verification/pointer_nullability_analysis.h"

#include <string>

#include "common/check.h"
#include "nullability_verification/pointer_nullability_lattice.h"
#include "nullability_verification/pointer_nullability_matchers.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Expr.h"
#include "clang/AST/OperationKinds.h"
#include "clang/AST/Stmt.h"
#include "clang/AST/Type.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Analysis/FlowSensitive/DataflowEnvironment.h"
#include "clang/Analysis/FlowSensitive/MatchSwitch.h"
#include "clang/Analysis/FlowSensitive/Value.h"
#include "clang/Basic/LLVM.h"

namespace clang {
namespace tidy {
namespace nullability {

using ast_matchers::MatchFinder;
using dataflow::BoolValue;
using dataflow::Environment;
using dataflow::MatchSwitchBuilder;
using dataflow::PointerValue;
using dataflow::SkipPast;
using dataflow::TransferState;
using dataflow::Value;

namespace {

BoolValue& getPointerNotNullProperty(
    const Expr* PointerExpr, TransferState<PointerNullabilityLattice>& State) {
  auto* PointerVal =
      cast<PointerValue>(State.Env.getValue(*PointerExpr, SkipPast::Reference));
  CHECK(State.Lattice.hasPointerNotNullProperty(PointerVal));
  return *State.Lattice.getPointerNotNullProperty(PointerVal);
}

void initialisePointerNotNullProperty(
    const Expr* Expr, const MatchFinder::MatchResult&,
    TransferState<PointerNullabilityLattice>& State) {
  if (auto* PointerVal = cast_or_null<PointerValue>(
          State.Env.getValue(*Expr, SkipPast::Reference))) {
    if (!State.Lattice.hasPointerNotNullProperty(PointerVal)) {
      State.Lattice.setPointerNotNullProperty(PointerVal,
                                              &State.Env.makeAtomicBoolValue());
    }
  }
}

void transferNullPointerLiteral(
    const Expr* NullPointer, const MatchFinder::MatchResult& Result,
    TransferState<PointerNullabilityLattice>& State) {
  auto* NullPointerVal = cast_or_null<PointerValue>(
      State.Env.getValue(*NullPointer, SkipPast::None));
  if (NullPointerVal == nullptr) {
    // Create storage location and value for null pointer if it doesn't exist
    auto& NullPointerLoc = State.Env.createStorageLocation(*NullPointer);
    NullPointerVal = &State.Env.takeOwnership(
        std::make_unique<PointerValue>(NullPointerLoc));
    State.Env.setStorageLocation(*NullPointer, NullPointerLoc);
    State.Env.setValue(NullPointerLoc, *NullPointerVal);
  }
  if (!State.Lattice.hasPointerNotNullProperty(NullPointerVal)) {
    // Set null pointer to be known null if not already set
    State.Lattice.setPointerNotNullProperty(
        NullPointerVal, &State.Env.getBoolLiteralValue(false));
  }
}

void transferAddrOf(const UnaryOperator* UnaryOp,
                    const MatchFinder::MatchResult& Result,
                    TransferState<PointerNullabilityLattice>& State) {
  auto* PointerVal =
      cast<PointerValue>(State.Env.getValue(*UnaryOp, SkipPast::None));
  State.Lattice.setPointerNotNullProperty(PointerVal,
                                          &State.Env.getBoolLiteralValue(true));
}

void transferDereference(const UnaryOperator* UnaryOp,
                         const MatchFinder::MatchResult&,
                         TransferState<PointerNullabilityLattice>& State) {
  auto* PointerExpr = UnaryOp->getSubExpr();
  auto& PointerNotNull = getPointerNotNullProperty(PointerExpr, State);
  if (!State.Env.flowConditionImplies(PointerNotNull)) {
    State.Lattice.addViolation(PointerExpr);
  }
}

void transferNullCheckComparison(
    const BinaryOperator* BinaryOp, const MatchFinder::MatchResult& result,
    TransferState<PointerNullabilityLattice>& State) {
  // Boolean representing the comparison between the two pointer values,
  // automatically created by the dataflow framework
  auto& PointerComparison =
      *cast<BoolValue>(State.Env.getValue(*BinaryOp, SkipPast::None));

  CHECK(BinaryOp->getOpcode() == BO_EQ || BinaryOp->getOpcode() == BO_NE);
  auto& PointerEQ = BinaryOp->getOpcode() == BO_EQ
                        ? PointerComparison
                        : State.Env.makeNot(PointerComparison);
  auto& PointerNE = BinaryOp->getOpcode() == BO_EQ
                        ? State.Env.makeNot(PointerComparison)
                        : PointerComparison;

  auto& LHSNotNull = getPointerNotNullProperty(BinaryOp->getLHS(), State);
  auto& RHSNotNull = getPointerNotNullProperty(BinaryOp->getRHS(), State);

  // !LHS && !RHS => LHS == RHS
  State.Env.addToFlowCondition(State.Env.makeImplication(
      State.Env.makeAnd(State.Env.makeNot(LHSNotNull),
                        State.Env.makeNot(RHSNotNull)),
      PointerEQ));
  // !LHS && RHS => LHS != RHS
  State.Env.addToFlowCondition(State.Env.makeImplication(
      State.Env.makeAnd(State.Env.makeNot(LHSNotNull), RHSNotNull), PointerNE));
  // LHS && !RHS => LHS != RHS
  State.Env.addToFlowCondition(State.Env.makeImplication(
      State.Env.makeAnd(LHSNotNull, State.Env.makeNot(RHSNotNull)), PointerNE));
}

void transferNullCheckImplicitCastPtrToBool(
    const Expr* CastExpr, const MatchFinder::MatchResult&,
    TransferState<PointerNullabilityLattice>& State) {
  if (auto* PointerVal = cast_or_null<PointerValue>(State.Env.getValue(
          *CastExpr->IgnoreImplicit(), SkipPast::Reference))) {
    auto* PointerNotNull = State.Lattice.getPointerNotNullProperty(PointerVal);
    CHECK(PointerNotNull != nullptr);

    auto& CastExprLoc = State.Env.createStorageLocation(*CastExpr);
    State.Env.setValue(CastExprLoc, *PointerNotNull);
    State.Env.setStorageLocation(*CastExpr, CastExprLoc);
  }
}

auto buildTransferer() {
  return MatchSwitchBuilder<TransferState<PointerNullabilityLattice>>()
      // Handles initialization of the null states of pointers
      .CaseOf<Expr>(isPointerVariableReference(),
                    initialisePointerNotNullProperty)
      .CaseOf<Expr>(isPointerMemberExpr(), initialisePointerNotNullProperty)
      // Handles nullptr
      .CaseOf<Expr>(isNullPointerLiteral(), transferNullPointerLiteral)
      // Handles address of operator (&var)
      .CaseOf<UnaryOperator>(isAddrOf(), transferAddrOf)
      // Handles pointer dereferencing (*ptr)
      .CaseOf<UnaryOperator>(isPointerDereference(), transferDereference)
      // Handles comparison between 2 pointers
      .CaseOf<BinaryOperator>(isPointerCheckBinOp(),
                              transferNullCheckComparison)
      // Handles checking of pointer as boolean
      .CaseOf<Expr>(isImplicitCastPointerToBool(),
                    transferNullCheckImplicitCastPtrToBool)
      .Build();
}
}  // namespace

PointerNullabilityAnalysis::PointerNullabilityAnalysis(ASTContext& Context)
    : DataflowAnalysis<PointerNullabilityAnalysis, PointerNullabilityLattice>(
          Context),
      Transferer(buildTransferer()) {}

void PointerNullabilityAnalysis::transfer(const Stmt* Stmt,
                                          PointerNullabilityLattice& Lattice,
                                          Environment& Env) {
  TransferState<PointerNullabilityLattice> State(Lattice, Env);
  Transferer(*Stmt, getASTContext(), State);
}

bool PointerNullabilityAnalysis::merge(QualType Type, const Value& Val1,
                                       const Environment& Env1,
                                       const Value& Val2,
                                       const Environment& Env2,
                                       Value& MergedVal,
                                       Environment& MergedEnv) {
  return false;
}
}  // namespace nullability
}  // namespace tidy
}  // namespace clang
