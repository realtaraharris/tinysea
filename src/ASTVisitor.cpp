#include "stdafx.h"

CustomASTVisitor::CustomASTVisitor(ASTContext &ctx, Renamer &r, Rewriter &rw)
    : context(ctx), renamer(r), sm(ctx.getSourceManager()), rewriter(rw) {}

bool CustomASTVisitor::VisitNamedDecl(NamedDecl *decl) {
    if (!decl || processedDecls.count(decl))
        return true;
    processedDecls.insert(decl);

    SourceManager &sm = decl->getASTContext().getSourceManager();
    SourceLocation loc = decl->getLocation();

    // Validate declaration location
    if (loc.isInvalid() || !sm.isWrittenInMainFile(loc) ||
        sm.isInSystemHeader(loc)) {
        return true;
    }

    // Generate short name
    std::string qualifiedName = decl->getQualifiedNameAsString();
    std::string shortName = renamer.getShortName(qualifiedName);

    // Perform replacement
    if (decl->getIdentifier() && !decl->getName().empty()) {
        SourceLocation nameLoc = sm.getSpellingLoc(decl->getLocation());
        rewriter.ReplaceText(nameLoc, decl->getName().size(), shortName);
        llvm::errs() << "Renamed: " << qualifiedName << " => " << shortName
                     << "\n";
    }

    return true;
}

bool CustomASTVisitor::VisitDeclRefExpr(DeclRefExpr *expr) {
    if (NamedDecl *decl = expr->getDecl()) {
        llvm::errs() << "Processing declaration reference expression: " << decl
                     << "\n";
        if (shouldSkip(decl))
            return true;

        std::string qualifiedName = decl->getQualifiedNameAsString();
        std::string shortName = renamer.getIdentifierShortName(qualifiedName);
        llvm::errs() << "VisitDeclRefExpr, qualifiedName: " << qualifiedName
                     << " shortName: " << shortName << "\n";
        if (!shortName.empty()) {
            rewriter.ReplaceText(expr->getLocation(), decl->getName().size(),
                                 shortName);
        }
    }
    return true;
}

bool CustomASTVisitor::TraverseDecl(Decl *D) {
    if (!D)
        return true;

    SourceManager &sm = context.getSourceManager();
    SourceLocation loc = D->getLocation();

    if (loc.isValid() &&
        (!sm.isWrittenInMainFile(loc) || sm.isInSystemHeader(loc))) {
        return true;
    }

    return RecursiveASTVisitor<CustomASTVisitor>::TraverseDecl(D);
}

bool CustomASTVisitor::shouldSkip(NamedDecl *decl) {
    SourceLocation loc = decl->getLocation();
    return loc.isInvalid() || decl->isImplicit();
}