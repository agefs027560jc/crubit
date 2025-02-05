// Part of the Crubit project, under the Apache License v2.0 with LLVM
// Exceptions. See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef CRUBIT_NULLABILITY_INFERENCE_INFER_TU_H_
#define CRUBIT_NULLABILITY_INFERENCE_INFER_TU_H_

#include <vector>

#include "nullability/inference/inference.proto.h"
#include "clang/AST/ASTContext.h"

namespace clang::tidy::nullability {

// Performs nullability inference within the scope of a single translation unit.
//
// This is not as powerful as running inference over the whole codebase, but is
// useful in observing the behavior of the inference system.
// It also lets us write tests for the whole inference system.
std::vector<Inference> inferTU(ASTContext &);

}  // namespace clang::tidy::nullability

#endif
