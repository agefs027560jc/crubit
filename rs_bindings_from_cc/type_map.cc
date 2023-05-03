// Part of the Crubit project, under the Apache License v2.0 with LLVM
// Exceptions. See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include "rs_bindings_from_cc/type_map.h"

#include <optional>

#include "absl/container/flat_hash_map.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "common/status_macros.h"
#include "rs_bindings_from_cc/ir.h"
#include "clang/AST/Attr.h"
#include "clang/AST/Attrs.inc"
#include "clang/AST/Decl.h"

namespace crubit {

namespace {
// A mapping of C++ standard types to their equivalent Rust types.
std::optional<absl::string_view> MapKnownCcTypeToRsType(
    absl::string_view cc_type) {
  static const auto* const kWellKnownTypes =
      new absl::flat_hash_map<absl::string_view, absl::string_view>({
          // TODO(lukasza): Try to deduplicate the entries below - for example:
          // - Try to unify `std::int32_t` and `int32_t`
          // One approach would be to desugar the types before calling
          // `MapKnownCcTypeToRsType`, but note that desugaring of type aliases
          // may be undesirable (i.e.  we may want the bindings to refer to
          // `TypeAlias` rather than directly to the type that it desugars to).
          // Note that b/254096006 tracks desire to preserve type aliases in
          // `cc_bindings_from_rs`.
          {"ptrdiff_t", "isize"},
          {"intptr_t", "isize"},
          {"size_t", "usize"},
          {"uintptr_t", "usize"},
          {"std::ptrdiff_t", "isize"},
          {"std::intptr_t", "isize"},
          {"std::size_t", "usize"},
          {"std::uintptr_t", "usize"},

          {"int8_t", "i8"},
          {"int16_t", "i16"},
          {"int32_t", "i32"},
          {"int64_t", "i64"},
          {"std::int8_t", "i8"},
          {"std::int16_t", "i16"},
          {"std::int32_t", "i32"},
          {"std::int64_t", "i64"},

          {"uint8_t", "u8"},
          {"uint16_t", "u16"},
          {"uint32_t", "u32"},

          {"uint64_t", "u64"},
          {"std::uint8_t", "u8"},
          {"std::uint16_t", "u16"},
          {"std::uint32_t", "u32"},
          {"std::uint64_t", "u64"},

          {"char16_t", "u16"},
          {"char32_t", "u32"},
          {"wchar_t", "i32"},
      });
  auto it = kWellKnownTypes->find(cc_type);
  if (it == kWellKnownTypes->end()) return std::nullopt;
  return it->second;
}

// Copied from lifetime_annotations/type_lifetimes.cc, which is expected to move
// into ClangTidy. See:
// https://discourse.llvm.org/t/rfc-lifetime-annotations-for-c/61377
absl::StatusOr<absl::string_view> EvaluateAsStringLiteral(
    const clang::Expr& expr, const clang::ASTContext& ast_context) {
  auto error = []() {
    return absl::InvalidArgumentError(
        "cannot evaluate argument as a string literal");
  };

  clang::Expr::EvalResult eval_result;
  if (!expr.EvaluateAsConstantExpr(eval_result, ast_context) ||
      !eval_result.Val.isLValue()) {
    return error();
  }

  const auto* eval_result_expr =
      eval_result.Val.getLValueBase().dyn_cast<const clang::Expr*>();
  if (!eval_result_expr) {
    return error();
  }

  const auto* string_literal =
      clang::dyn_cast<clang::StringLiteral>(eval_result_expr);
  if (!string_literal) {
    return error();
  }

  return {string_literal->getString()};
}

absl::StatusOr<std::optional<absl::string_view>> GetRustTypeAttribute(
    const clang::Type& cc_type) {
  std::optional<absl::string_view> rust_type;
  if (const clang::TagDecl* tag_decl = cc_type.getAsTagDecl();
      tag_decl != nullptr) {
    for (clang::AnnotateAttr* attr :
         tag_decl->specific_attrs<clang::AnnotateAttr>()) {
      if (attr->getAnnotation() != "crubit_internal_rust_type") continue;

      if (rust_type.has_value())
        return absl::InvalidArgumentError(
            "Only one `crubit_internal_rust_type` attribute may be placed on a "
            "type.");
      if (attr->args_size() != 1)
        return absl::InvalidArgumentError(
            "The `crubit_internal_rust_type` attribute requires a single "
            "string literal "
            "argument, the Rust type.");
      const clang::Expr& arg = **attr->args_begin();
      CRUBIT_ASSIGN_OR_RETURN(
          rust_type, EvaluateAsStringLiteral(arg, tag_decl->getASTContext()));
    }
  }
  return rust_type;
}

}  // namespace

absl::StatusOr<std::optional<MappedType>> TypeMapOverride(
    const clang::Type& cc_type) {
  std::string type_string = clang::QualType(&cc_type, 0).getAsString();
  std::optional<absl::string_view> rust_type;
  CRUBIT_ASSIGN_OR_RETURN(rust_type, GetRustTypeAttribute(cc_type));
  if (!rust_type.has_value()) {
    rust_type = MapKnownCcTypeToRsType(type_string);
  }
  if (rust_type.has_value()) {
    return MappedType::Simple(std::string(*rust_type), type_string);
  }
  return std::nullopt;
}

}  // namespace crubit
