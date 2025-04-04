#include "stdafx.h"

using namespace clang;
using namespace clang::tooling;

CustomPPCallbacks::CustomPPCallbacks(Renamer &r, SourceManager &sm,
                                     Rewriter &rw)
    : renamer(r), sm(sm), rewriter(rw) {}

void CustomPPCallbacks::MacroDefined(const Token &MacroNameTok,
                                     const MacroDirective *MD) {
    SourceLocation loc = MacroNameTok.getLocation();
    if (!sm.isInMainFile(loc) || sm.isInSystemHeader(loc)) {
        return;
    }

    // check against a list of known identifiers
    std::string macroName = MacroNameTok.getIdentifierInfo()->getName().str();
    if (processedMacros.count(macroName)) {
        return;
    } // already processed
    processedMacros.insert(macroName);

    return; // don't bother with the rest of this method. we may want to delete
            // the stuff below?

    // now actually process the macro
    bool isInvalid = loc.isInvalid();
    bool isInMainFile = sm.isInMainFile(loc);
    bool isInSystemHeader = sm.isInSystemHeader(loc);
    std::string filename = sm.getFilename(loc).str();
    std::string shortName = renamer.getShortName(macroName);

    llvm::errs() << "MacroDefined: " << macroName
                 << "\n shortName: " << shortName
                 << "\n location: " << loc.printToString(sm)
                 << "\n filename: " << filename << "\n isInvalid: " << isInvalid
                 << "\n isInMainFile: " << isInMainFile
                 << "\n isInSystemHeader: " << isInSystemHeader << "\n";

    if (shortName.empty()) {
        return;
    }
    // rewriter.ReplaceText(loc, macroName.length(), shortName);
}

void CustomPPCallbacks::MacroExpands(const Token &MacroNameTok,
                                     const MacroDefinition &MD,
                                     SourceRange Range, const MacroArgs *Args) {
    SourceLocation loc = MacroNameTok.getLocation();
    if (!sm.isInMainFile(loc) || sm.isInSystemHeader(loc)) {
        return;
    }

    bool isInvalid = loc.isInvalid();
    bool isInMainFile = sm.isInMainFile(loc);
    bool isInSystemHeader = sm.isInSystemHeader(loc);
    std::string filename = sm.getFilename(loc).str();
    std::string macroName = MacroNameTok.getIdentifierInfo()->getName().str();
    std::string shortName = renamer.getShortName(macroName, true);

    /*
        llvm::errs() << "MacroExpands: " << macroName
                     << "\n shortName: " << shortName
                     << "\n location: " << loc.printToString(sm)
                     << "\n filename: " << filename
                     << "\n isInvalid: " << isInvalid
                     << "\n isInMainFile: " << isInMainFile
                     << "\n isInSystemHeader: " << isInSystemHeader
                     << "\n"; */

    if (shortName.empty()) {
        std::cout << "empty shortname" << std::endl;
        return;
    }

    rewriter.ReplaceText(loc, macroName.length(), shortName);
}

CustomASTConsumer::CustomASTConsumer(clang::ASTContext &ctx, Renamer &r,
                                     clang::Rewriter &rw)
    : visitor(std::make_unique<CustomASTVisitor>(ctx, r, rw)) {}

void CustomASTConsumer::HandleTranslationUnit(clang::ASTContext &context) {
    visitor->TraverseDecl(context.getTranslationUnitDecl());
}

CustomFrontendAction::CustomFrontendAction(Renamer &r)
    : renamer(r), rewriter(std::make_unique<Rewriter>()) {}

std::unique_ptr<clang::ASTConsumer>
CustomFrontendAction::CreateASTConsumer(clang::CompilerInstance &ci,
                                        llvm::StringRef) {
    // rewriter->setSourceMgr(ci.getSourceManager(), ci.getLangOpts());
    return std::make_unique<CustomASTConsumer>(ci.getASTContext(), renamer,
                                               *rewriter);
}

void CustomFrontendAction::ExecuteAction() {
    clang::CompilerInstance &ci = getCompilerInstance();
    ci.getPreprocessor().addPPCallbacks(std::make_unique<CustomPPCallbacks>(
        renamer, ci.getSourceManager(), *rewriter));
    clang::ASTFrontendAction::ExecuteAction();
    rewriter->overwriteChangedFiles();
}

CustomActionFactory::CustomActionFactory(Renamer &r) : renamer(r) {}

std::unique_ptr<FrontendAction> CustomActionFactory::create() {
    return std::make_unique<CustomFrontendAction>(renamer);
}
