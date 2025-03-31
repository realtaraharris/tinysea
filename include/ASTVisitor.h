#pragma once

using namespace clang;
using namespace clang::tooling;

class CustomASTVisitor : public RecursiveASTVisitor<CustomASTVisitor> {
    ASTContext &context;
    Renamer &renamer;
    SourceManager &sm;
    Rewriter &rewriter;
    std::set<Decl *> processedDecls;

public:
    CustomASTVisitor(ASTContext &ctx, Renamer &r, Rewriter &rw);
    bool VisitNamedDecl(NamedDecl *decl);
    bool VisitDeclRefExpr(DeclRefExpr *expr);
    bool TraverseDecl(Decl *D);

private:
    bool shouldSkip(NamedDecl *decl);
};