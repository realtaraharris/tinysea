#ifndef STDAFX_H
#define STDAFX_H

// System headers
#include <fstream>
#include <iostream>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// Clang/LLVM headers
#include "clang/AST/ASTContext.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Frontend/TextDiagnosticPrinter.h"
#include "clang/Lex/PPCallbacks.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"

// LLVM headers
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/JSON.h"

// our headers
#include "renamer.h"
#include "ASTVisitor.h"
#include "PPCallbacks.h"

#endif // STDAFX_H