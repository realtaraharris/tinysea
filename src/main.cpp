#include "stdafx.h"

using namespace clang;
using namespace clang::tooling;

static llvm::cl::OptionCategory MyToolCategory("Identifier Shortener Options");
static llvm::cl::opt<std::string>
    MappingFile("mapping", llvm::cl::desc("Specify mapping file"),
                llvm::cl::value_desc("filename"),
                llvm::cl::cat(MyToolCategory));

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

    // Only save if there are mappings and a filename was specified
    if (!MappingFile.empty() && renamer.hasIdentifierMappings()) {
        renamer.saveMappings(MappingFile);
    }

    return result;
}
