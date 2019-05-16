#include "clang/StaticAnalyzer/Core/Checker.h"
#include "clang/StaticAnalyzer/Checkers/BuiltinCheckerRegistration.h"
#include "clang/StaticAnalyzer/Core/CheckerManager.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CallEvent.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CheckerContext.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/Support/raw_ostream.h"
#include "clang/AST/ParentMap.h"
#include "clang/Basic/TargetInfo.h"
#include "clang/StaticAnalyzer/Core/BugReporter/BugType.h"
#include <iostream>

using namespace clang;
using namespace ento;

namespace {

class MyChecker
  : public Checker< check::PreStmt<CallExpr>,
		    check::PreCall > {
  public:

    void checkPreStmt(const CallExpr *CE, CheckerContext &c) const;
    void checkPreCall(const CallEvent &Call, CheckerContext &c) const;
};
} // end anonymous namespace

void MyChecker::checkPreStmt(const CallExpr *CE, CheckerContext &c) const {
  //std::string buf;
  //llvm::raw_string_ostream OS(buf);
  std::cout << " Message of checkPreStmt, added by me\n ";
  //OS.str();
}

void MyChecker::checkPreCall(const CallEvent &Call, CheckerContext &c) const {
  //std::string buf;
  //llvm::raw_string_ostream OS(buf);
  std::cout << " Message of checkPreCall, added by me\n ";
  //OS.str();
}

void ento::registerMyChecker(CheckerManager &mgr) {
  mgr.registerChecker<MyChecker>();
}

bool ento::shouldRegisterMyChecker(const LangOptions &LO) {
  return true;
}
