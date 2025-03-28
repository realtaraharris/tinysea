#include "clang/AST/ASTContext.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Lex/PPCallbacks.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/JSON.h"
#include <fstream>
#include <iostream>
#include <unordered_map>

using namespace clang;
using namespace clang::tooling;

static llvm::cl::OptionCategory MyToolCategory("Identifier Shortener Options");
static llvm::cl::opt<std::string>
    MappingFile("mapping", llvm::cl::desc("Specify mapping file"),
                llvm::cl::value_desc("filename"),
                llvm::cl::cat(MyToolCategory));

class Renamer {
    std::mutex mtx;
    std::unordered_map<std::string, std::string> identifierMap;
    std::unordered_map<std::string, std::string> macroMap;
    std::vector<std::string> nameSequence;
    std::set<std::string> reservedKeywords;
    unsigned currentIndex = 0;

    std::string generateName(unsigned index) {
        std::string name;
        while (index >= 0) {
            name.push_back('a' + (index % 26));
            index = index / 26 - 1;
            if (index < 0)
                break;
        }
        std::reverse(name.begin(), name.end());
        std::cout << "name: " << name << std::endl;
        return name;
    }

    void initKeywords() {
        const std::vector<std::string> keywords = {"alignas",
                                                   "alignof",
                                                   "and",
                                                   "and_eq",
                                                   "asm",
                                                   "auto",
                                                   "bitand",
                                                   "bitor",
                                                   "bool",
                                                   "break",
                                                   "case",
                                                   "catch",
                                                   "char",
                                                   "char8_t",
                                                   "char16_t",
                                                   "char32_t",
                                                   "class",
                                                   "compl",
                                                   "concept",
                                                   "const",
                                                   "consteval",
                                                   "constexpr",
                                                   "const_cast",
                                                   "continue",
                                                   "co_await",
                                                   "co_return",
                                                   "co_yield",
                                                   "decltype",
                                                   "default",
                                                   "delete",
                                                   "do",
                                                   "double",
                                                   "dynamic_cast",
                                                   "else",
                                                   "enum",
                                                   "explicit",
                                                   "export",
                                                   "extern",
                                                   "false",
                                                   "float",
                                                   "for",
                                                   "friend",
                                                   "goto",
                                                   "if",
                                                   "inline",
                                                   "int",
                                                   "long",
                                                   "mutable",
                                                   "namespace",
                                                   "new",
                                                   "noexcept",
                                                   "not",
                                                   "not_eq",
                                                   "nullptr",
                                                   "operator",
                                                   "or",
                                                   "or_eq",
                                                   "private",
                                                   "protected",
                                                   "public",
                                                   "register",
                                                   "reinterpret_cast",
                                                   "requires",
                                                   "return",
                                                   "short",
                                                   "signed",
                                                   "sizeof",
                                                   "static",
                                                   "static_assert",
                                                   "static_cast",
                                                   "struct",
                                                   "switch",
                                                   "template",
                                                   "this",
                                                   "thread_local",
                                                   "throw",
                                                   "true",
                                                   "try",
                                                   "typedef",
                                                   "typeid",
                                                   "typename",
                                                   "union",
                                                   "unsigned",
                                                   "using",
                                                   "virtual",
                                                   "void",
                                                   "volatile",
                                                   "wchar_t",
                                                   "while",
                                                   "xor",
                                                   "xor_eq"};
        reservedKeywords.insert(keywords.begin(), keywords.end());
    }

public:
    Renamer() { initKeywords(); }
    bool hasMacro(const std::string &name) const {
        return macroMap.find(name) != macroMap.end();
    }

    std::string getMacroShortName(const std::string &name) const {
        if (auto it = macroMap.find(name); it != macroMap.end()) {
            return it->second;
        }
        return "";
    }
    void loadMappings(const std::string &filename);
    void saveMappings(const std::string &filename);
    std::string getShortName(const std::string &qualifiedName,
                             bool isMacro = false);

    bool hasIdentifier(const std::string &name) const {
        return identifierMap.find(name) != identifierMap.end();
    }

    std::string getIdentifierShortName(const std::string &name) const {
        if (auto it = identifierMap.find(name); it != identifierMap.end()) {
            return it->second;
        }
        return "";
    }
};

void Renamer::loadMappings(const std::string &filename) {
    std::ifstream file(filename);
    if (!file)
        return;

    // Read entire file into string
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());

    // Parse JSON from string
    auto jsonOrError = llvm::json::parse(content);
    if (!jsonOrError) {
        llvm::errs() << "Failed to parse JSON: "
                     << toString(jsonOrError.takeError()) << "\n";
        return;
    }

    if (auto obj = jsonOrError->getAsObject()) {
        for (auto &pair : *obj) {
            identifierMap[pair.getFirst().str()] =
                pair.getSecond().getAsString().value().str();
        }
    }
}

void Renamer::saveMappings(const std::string &filename) {
    llvm::json::Object jsonMap;
    for (const auto &pair : identifierMap) {
        jsonMap[pair.first] = pair.second;
    }

    std::error_code ec;
    llvm::raw_fd_ostream out(filename, ec);
    if (!ec) {
        out << llvm::json::Value(std::move(jsonMap));
    } else {
        llvm::errs() << "Failed to write mappings: " << ec.message() << "\n";
    }
}

std::string Renamer::getShortName(const std::string &qualifiedName,
                                  bool isMacro) {
    std::lock_guard<std::mutex> lock(mtx);

    static const std::set<std::string> preserved{"main", "ptrdiff_t", "size_t",
                                                 "nullptr_t", "max_align_t"};

    if (preserved.count(qualifiedName) || qualifiedName.starts_with("std::")) {
        return qualifiedName;
    }

    auto &map = isMacro ? macroMap : identifierMap;
    if (auto it = map.find(qualifiedName); it != map.end()) {
        return it->second;
    }

    std::string newName;
    do {
        newName = generateName(currentIndex++);
    } while (reservedKeywords.count(newName));

    map[qualifiedName] = newName;
    return newName;
}

class CustomPPCallbacks : public PPCallbacks {
    Renamer &renamer;
    SourceManager &sm;
    Rewriter &rewriter;
    std::set<std::string> processedMacros;

public:
    CustomPPCallbacks(Renamer &r, SourceManager &sm, Rewriter &rw)
        : renamer(r), sm(sm), rewriter(rw) {}

    void MacroDefined(const Token &MacroNameTok,
                      const MacroDirective *MD) override {
        SourceLocation loc = MacroNameTok.getLocation();
        if (!sm.isInMainFile(loc) || sm.isInSystemHeader(loc)) {
            return;
        }

        // check against a list of known identifiers
        std::string macroName =
            MacroNameTok.getIdentifierInfo()->getName().str();
        if (processedMacros.count(macroName)) {
            return;
        } // already processed
        processedMacros.insert(macroName);

        // now actually process the macro
        bool isInvalid = loc.isInvalid();
        bool isInMainFile = sm.isInMainFile(loc);
        bool isInSystemHeader = sm.isInSystemHeader(loc);
        std::string filename = sm.getFilename(loc).str();

        llvm::errs() << "MacroDefined: "
                     << MacroNameTok.getIdentifierInfo()->getName()
                     << "\n macroName: " << macroName
                     << "\n isInvalid: " << isInvalid
                     << "\n isInMainFile: " << isInMainFile
                     << "\n isInSystemHeader: " << isInSystemHeader
                     << "\n Location: " << loc.printToString(sm)
                     << "\n Filename: " << filename << "\n";
        // sm.dump();

        std::string shortName = renamer.getShortName(macroName, true);

        rewriter.ReplaceText(MacroNameTok.getLocation(), macroName.length(),
                             shortName);
    }

    void MacroExpands(const Token &MacroNameTok, const MacroDefinition &MD,
                      SourceRange Range, const MacroArgs *Args) override {
        SourceLocation loc = MacroNameTok.getLocation();
        if (!sm.isInMainFile(loc) || sm.isInSystemHeader(loc)) {
            return;
        }

        bool isInvalid = loc.isInvalid();
        bool isInMainFile = sm.isInMainFile(loc);
        bool isInSystemHeader = sm.isInSystemHeader(loc);
        std::string filename = sm.getFilename(loc).str();

        llvm::errs() << "MacroExpands: "
                     << MacroNameTok.getIdentifierInfo()->getName()
                     << "\n  Location: " << loc.printToString(sm)
                     << "\n  Filename: " << filename
                     << "\n  isInvalid: " << isInvalid
                     << "\n  isInMainFile: " << isInMainFile
                     << "\n  isInSystemHeader: " << isInSystemHeader << "\n";
        // sm.dump();

        std::string macroName =
            MacroNameTok.getIdentifierInfo()->getName().str();
        std::string shortName = renamer.getMacroShortName(macroName);

        if (!shortName.empty()) {
            rewriter.ReplaceText(MacroNameTok.getLocation(), macroName.length(),
                                 shortName);
        }
    }
};

class CustomASTVisitor : public RecursiveASTVisitor<CustomASTVisitor> {
    ASTContext &context;
    Renamer &renamer;
    SourceManager &sm;
    Rewriter &rewriter;
    std::set<Decl *> processedDecls;

public:
    CustomASTVisitor(ASTContext &ctx, Renamer &r, Rewriter &rw)
        : context(ctx), renamer(r), sm(ctx.getSourceManager()), rewriter(rw) {}

    bool VisitNamedDecl(NamedDecl *decl) {
        if (!decl || processedDecls.count(decl))
            return true;
        processedDecls.insert(decl);

        // Rest of your processing logic...
        llvm::errs() << "START Processing: " << decl->getNameAsString() << "\n";

        if (decl->getIdentifier() && decl->getName().empty()) {
            return true; // Skip anonymous declarations
        }

        SourceLocation loc = decl->getLocation();
        llvm::errs() << "Processing declaration: "
                     << decl->getQualifiedNameAsString() << " at "
                     << loc.printToString(
                            decl->getASTContext().getSourceManager())
                     << "\n";
        llvm::errs().flush(); // Ensure immediate output

        if (shouldSkip(decl))
            return true;

        // llvm::errs() << "Processing declaration: " << decl->getName() <<
        // "\n";
        std::string qualifiedName = decl->getQualifiedNameAsString();
        std::string shortName = renamer.getShortName(qualifiedName);
        llvm::errs() << "VisitNamedDecl, qualifiedName: " << qualifiedName
                     << " shortName: " << shortName << "\n";
        rewriter.ReplaceText(decl->getLocation(), decl->getName().size(),
                             shortName);

        llvm::errs() << "Rewrote " << decl->getName() << " to " << shortName
                     << " at " << loc.printToString(sm) << "\n";
        llvm::errs() << "END Processing: " << decl->getNameAsString() << "\n";
        return true;
    }

    bool VisitDeclRefExpr(DeclRefExpr *expr) {
        if (NamedDecl *decl = expr->getDecl()) {
            llvm::errs() << "Processing declaration reference expression: "
                         << decl << "\n";
            if (shouldSkip(decl))
                return true;

            std::string qualifiedName = decl->getQualifiedNameAsString();
            std::string shortName =
                renamer.getIdentifierShortName(qualifiedName);
            llvm::errs() << "VisitDeclRefExpr, qualifiedName: " << qualifiedName
                         << " shortName: " << shortName << "\n";
            if (!shortName.empty()) {
                rewriter.ReplaceText(expr->getLocation(),
                                     decl->getName().size(), shortName);
            }
        }
        return true;
    }

    bool TraverseDecl(Decl *D) {
        if (!D)
            return true;

        SourceLocation loc = D->getLocation();
        if (loc.isInvalid())
            return true;

        // Get precise file information
        bool isMainFile = sm.isInMainFile(loc);
        bool isSystemHeader = sm.isInSystemHeader(loc);
        std::string filename = sm.getFilename(loc).str();

        // Skip anything not in main file or in system headers
        if (!isMainFile || isSystemHeader) {
            return true;
        }

        return RecursiveASTVisitor<CustomASTVisitor>::TraverseDecl(D);
    }

private:
    bool shouldSkip(NamedDecl *decl) {
        // Simplified since TraverseDecl already handles filtering
        SourceLocation loc = decl->getLocation();
        return loc.isInvalid() || decl->isImplicit();
    }
};

class CustomASTConsumer : public clang::ASTConsumer {
    std::unique_ptr<CustomASTVisitor> visitor;

public:
    CustomASTConsumer(clang::ASTContext &ctx, Renamer &r, clang::Rewriter &rw)
        : visitor(std::make_unique<CustomASTVisitor>(ctx, r, rw)) {}

    void HandleTranslationUnit(clang::ASTContext &context) override {
        visitor->TraverseDecl(context.getTranslationUnitDecl());
    }
};

class CustomFrontendAction : public clang::ASTFrontendAction {
    Renamer &renamer;
    std::unique_ptr<Rewriter> rewriter;

public:
    CustomFrontendAction(Renamer &r)
        : renamer(r), rewriter(std::make_unique<Rewriter>()) {}

    std::unique_ptr<clang::ASTConsumer>
    CreateASTConsumer(clang::CompilerInstance &ci, llvm::StringRef) override {
        rewriter->setSourceMgr(ci.getSourceManager(), ci.getLangOpts());
        return std::make_unique<CustomASTConsumer>(ci.getASTContext(), renamer,
                                                   *rewriter);
    }

    void ExecuteAction() override {
        clang::CompilerInstance &ci = getCompilerInstance();
        ci.getPreprocessor().addPPCallbacks(std::make_unique<CustomPPCallbacks>(
            renamer, ci.getSourceManager(), *rewriter));
        clang::ASTFrontendAction::ExecuteAction();

        // careful: we're overwriting files
        if (rewriter->overwriteChangedFiles()) {
            llvm::errs() << "Successfully wrote modified files\n";
        } else {
            llvm::errs() << "No files modified\n";
        }

        // llvm::errs() << "Total buffers modified: " <<
        //     rewriter.getBufferRanges().size() << "\n";
        rewriter.reset();
    }
};

class CustomActionFactory : public FrontendActionFactory {
    Renamer &renamer;

public:
    CustomActionFactory(Renamer &r) : renamer(r) {}

    std::unique_ptr<FrontendAction> create() override {
        return std::make_unique<CustomFrontendAction>(renamer);
    }
};

int main(int argc, const char **argv) {
    llvm::InitLLVM init(argc, argv);

    auto ExpectedParser =
        CommonOptionsParser::create(argc, argv, MyToolCategory);
    if (!ExpectedParser) {
        llvm::errs() << "Error creating options parser: "
                     << llvm::toString(ExpectedParser.takeError()) << "\n";
        return 1;
    }

    CommonOptionsParser &opts = ExpectedParser.get();
    ClangTool tool(opts.getCompilations(), opts.getSourcePathList());

    Renamer renamer;
    if (!MappingFile.empty())
        renamer.loadMappings(MappingFile);

    CustomActionFactory factory(renamer);

    int result = tool.run(&factory);

    // TODO: if renamer.identifierMap is empty, don't try to save the mapping
    renamer.saveMappings(MappingFile);

    return result;
}
