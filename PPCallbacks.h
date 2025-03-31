#pragma once

using namespace clang;
using namespace clang::tooling;

class CustomPPCallbacks : public PPCallbacks {
    Renamer &renamer;
    SourceManager &sm;
    Rewriter &rewriter;
    std::set<std::string> processedMacros;

public:
    CustomPPCallbacks(Renamer &r, SourceManager &sm, Rewriter &rw);
    void MacroDefined(const Token &MacroNameTok,
                      const MacroDirective *MD) override;
    void MacroExpands(const Token &MacroNameTok, const MacroDefinition &MD,
                      SourceRange Range, const MacroArgs *Args) override;
};

class CustomASTConsumer : public clang::ASTConsumer {
    std::unique_ptr<CustomASTVisitor> visitor;

public:
    CustomASTConsumer(clang::ASTContext &ctx, Renamer &r, clang::Rewriter &rw);
    void HandleTranslationUnit(clang::ASTContext &context) override;
};

class CustomFrontendAction : public clang::ASTFrontendAction {
    Renamer &renamer;
    std::unique_ptr<Rewriter> rewriter;

public:
    CustomFrontendAction(Renamer &r);
    std::unique_ptr<clang::ASTConsumer>
    CreateASTConsumer(clang::CompilerInstance &ci, llvm::StringRef) override;
    void ExecuteAction() override;
};

class CustomActionFactory : public FrontendActionFactory {
    Renamer &renamer;

public:
    CustomActionFactory(Renamer &r);
    std::unique_ptr<FrontendAction> create() override;
};
