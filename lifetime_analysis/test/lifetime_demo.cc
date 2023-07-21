// Exceptions. See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <fstream>
#include <ostream>
#include <string>
#include <utility>
#include <variant>
#include <string>

#include <iostream>

#include "absl/container/flat_hash_map.h"
#include "lifetime_analysis/analyze.h"
#include "lifetime_annotations/test/named_func_lifetimes.h"
#include "lifetime_annotations/test/run_on_code.h"

namespace clang {
namespace tidy {
namespace lifetimes {

class LifetimeAnalysisTest {
 protected:
  static std::string QualifiedName(const clang::FunctionDecl* func);

  struct GetLifetimesOptions {
    GetLifetimesOptions()
        : with_template_placeholder(false), include_implicit_methods(false) {}
    bool with_template_placeholder;
    bool include_implicit_methods;
  };

  NamedFuncLifetimes GetLifetimes(
      llvm::StringRef source_code,
      const GetLifetimesOptions& options = GetLifetimesOptions());

  NamedFuncLifetimes GetLifetimesWithPlaceholder(llvm::StringRef source_code);

  void AnalyzeBrokenCode() { analyze_broken_code_ = true; }

 private:
  absl::flat_hash_map<std::string, FunctionDebugInfo> debug_info_map_;
  bool analyze_broken_code_ = false;
};

std::string LifetimeAnalysisTest::QualifiedName(
    const clang::FunctionDecl* func) {
  // TODO(veluca): figure out how to name overloaded functions.
  std::string str;
  llvm::raw_string_ostream ostream(str);
  func->printQualifiedName(ostream);
  ostream.flush();
  return str;
}

NamedFuncLifetimes LifetimeAnalysisTest::GetLifetimes(
    llvm::StringRef source_code, const GetLifetimesOptions& options) {
  NamedFuncLifetimes tu_lifetimes;

  auto test = [&tu_lifetimes, &options, this](
                  clang::ASTContext& ast_context,
                  const LifetimeAnnotationContext& lifetime_context) {
    // This will get called even if the code contains compilation errors.
    // So we need to check to avoid performing an analysis on code that
    // doesn't compile.
    if (ast_context.getDiagnostics().hasUncompilableErrorOccurred() &&
        !analyze_broken_code_) {
      tu_lifetimes.Add("", "Compilation error -- see log for details");
      return;
    }

    auto result_callback = [&tu_lifetimes, &options](
                               const clang::FunctionDecl* func,
                               const FunctionLifetimesOrError&
                                   lifetimes_or_error) {
      if (std::holds_alternative<FunctionAnalysisError>(lifetimes_or_error)) {
        tu_lifetimes.Add(
            QualifiedName(func),
            absl::StrCat(
                "ERROR: ",
                std::get<FunctionAnalysisError>(lifetimes_or_error).message));
        return;
      }
      const auto& func_lifetimes =
          std::get<FunctionLifetimes>(lifetimes_or_error);

      // Do not insert in the result set implicitly-defined constructors or
      // assignment operators.
      if (auto* constructor =
              clang::dyn_cast<clang::CXXConstructorDecl>(func)) {
        if (constructor->isImplicit() && !options.include_implicit_methods) {
          return;
        }
      }
      if (auto* method = clang::dyn_cast<clang::CXXMethodDecl>(func)) {
        if (method->isImplicit() && !options.include_implicit_methods) {
          return;
        }
      }

      tu_lifetimes.Add(QualifiedName(func), NameLifetimes(func_lifetimes));
    };

    FunctionDebugInfoMap func_ptr_debug_info_map;
    llvm::DenseMap<const clang::FunctionDecl*, FunctionLifetimesOrError>
        analysis_result;
    if (options.with_template_placeholder) {
      AnalyzeTranslationUnitWithTemplatePlaceholder(
          ast_context.getTranslationUnitDecl(), lifetime_context,
          result_callback,
          /*diag_reporter=*/{}, &func_ptr_debug_info_map);
    } else {
      analysis_result = AnalyzeTranslationUnit(
          ast_context.getTranslationUnitDecl(), lifetime_context,
          /*diag_reporter=*/{}, &func_ptr_debug_info_map);

      for (const auto& [func, lifetimes_or_error] : analysis_result) {
        result_callback(func, lifetimes_or_error);
      }
    }

    for (auto& [func, debug_info] : func_ptr_debug_info_map) {
      debug_info_map_.try_emplace(func->getDeclName().getAsString(),
                                  std::move(debug_info));
    }
  };

  if (!runOnCodeWithLifetimeHandlers(source_code, test,
                                     {"-fsyntax-only", "-std=c++17"})) {
    // We need to disambiguate between two cases:
    // - We were unable to run the analysis at all (because there was some
    //   internal error)
    //   In this case, `tu_lifetimes` will be empty, so add a corresponding
    //   note here.
    // - The analysis emitted an error diagnostic, which will also cause us to
    //   end up here.
    //   In this case, `tu_lifetimes` already contains an error empty, so we
    //   don't need to do anything.
    if (tu_lifetimes.Entries().empty()) {
      tu_lifetimes.Add("", "Error running dataflow analysis");
    }
  }

  return tu_lifetimes;
}

NamedFuncLifetimes LifetimeAnalysisTest::GetLifetimesWithPlaceholder(
    llvm::StringRef source_code) {
  GetLifetimesOptions options;
  options.with_template_placeholder = true;
  return GetLifetimes(source_code, options);
}

}  // namespace lifetimes
}  // namespace tidy
}  // namespace clang

int main(int argc, char **argv){
    class LifetimeAnalysisTestDemo : public clang::tidy::lifetimes::LifetimeAnalysisTest {
        public:
        using LifetimeAnalysisTest::GetLifetimes;
        using LifetimeAnalysisTest::GetLifetimesWithPlaceholder;
    };

    LifetimeAnalysisTestDemo demo;

    std::ifstream ins(argv[1]); // read a file path
    std::string outbuf( (std::istreambuf_iterator<char>(ins) ),
                      (std::istreambuf_iterator<char>()    ) );

    std::cout << demo.GetLifetimes(outbuf) << std::endl;

    return 0;
}