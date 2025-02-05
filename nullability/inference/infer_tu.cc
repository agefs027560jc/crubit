// Part of the Crubit project, under the Apache License v2.0 with LLVM
// Exceptions. See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include "nullability/inference/infer_tu.h"

#include <utility>
#include <vector>

#include "nullability/inference/collect_evidence.h"
#include "nullability/inference/inference.proto.h"
#include "nullability/inference/merge.h"
#include "clang/AST/ASTContext.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/raw_ostream.h"

namespace clang::tidy::nullability {

std::vector<Inference> inferTU(ASTContext& Ctx) {
  std::vector<Evidence> AllEvidence;

  // Collect all evidence.
  auto Sites = EvidenceSites::discover(Ctx);
  auto Emitter = evidenceEmitter([&](auto& E) { AllEvidence.push_back(E); });
  for (const auto* Decl : Sites.Declarations)
    collectEvidenceFromTargetDeclaration(*Decl, Emitter);
  for (const auto* Impl : Sites.Implementations) {
    if (auto Err = collectEvidenceFromImplementation(*Impl, Emitter)) {
      llvm::errs() << "Skipping function: " << toString(std::move(Err)) << "\n";
      Impl->print(llvm::errs());
    }
  }
  // Group by symbol.
  llvm::sort(AllEvidence, [&](const Evidence& L, const Evidence& R) {
    return L.symbol().usr() < R.symbol().usr();
  });
  // For each symbol, combine evidence into an inference.
  llvm::ArrayRef<Evidence> RemainingEvidence = AllEvidence;
  std::vector<Inference> AllInference;
  while (!RemainingEvidence.empty()) {
    auto Batch = RemainingEvidence.take_while([&](const Evidence& E) {
      return E.symbol().usr() == RemainingEvidence.front().symbol().usr();
    });
    RemainingEvidence = RemainingEvidence.drop_front(Batch.size());
    AllInference.push_back(mergeEvidence(Batch));
  }

  return AllInference;
}

}  // namespace clang::tidy::nullability
