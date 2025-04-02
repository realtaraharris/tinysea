#include "stdafx.h"
#include "clang/Tooling/CommonOptionsParser.h"

using namespace clang;
using namespace clang::tooling;

void processCMakeProject(const std::string &projectDir,
                         const std::string &outputFile, Renamer &renamer,
                         llvm::cl::OptionCategory &category) {
    std::unique_ptr<clang::tooling::CompilationDatabase> db;
    std::string error;

    db = clang::tooling::CompilationDatabase::loadFromDirectory(projectDir,
                                                                error);

    if (!db) {
        llvm::errs() << "Failed to find compilation database. Tried:" << "  "
                     << projectDir << "/build\n"
                     << "Error: " << error << "\n";

        return;
    }

    // Build arguments with Haiku-specific includes
    std::vector<std::string> args = {
        "tinysea", "--",
        "-std=c++20",  // Add C++ standard used by SolveSpace
        "-nostdinc++", // Prevent conflicting standard library paths
        "-msse", "-msse2", "-mavx", "-mfma", "-march=x86-64",
        // "-D__SSE__=1",
        // "-D__SSE2__=1",
        // "-D__SSE3__=1",
        "-DEIGEN_MPL2_ONLY",
        // "-DEIGEN_NO_DEBUG", // already defined
        "-DEIGEN_DONT_VECTORIZE",
        // "-target x86_64-unknown-haiku", // ???
        "-isystem/boot/system/develop/headers",
        "-isystem/boot/system/develop/headers/Eigen",
        "-isystem/boot/system/develop/headers/c++",
        "-isystem/boot/system/develop/headers/c++/x86_64-unknown-haiku",
        "-isystem/boot/system/develop/headers/posix",
        "-isystem/boot/system/develop/headers/private",
        // "-isystem/boot/system/develop/headers/gcc/include",
        "-isystem/boot/system/lib/clang/18/include",
        "-isystem/boot/system/develop/headers/agg2",
        "-isystem/boot/home/cppfront/include", "-I/boot/home/solvespace/src"};

    // Insert source files BEFORE the "--" separator
    std::vector<std::string> sourceFiles = db->getAllFiles();
    args.insert(args.begin() + 1, sourceFiles.begin(), sourceFiles.end());

    // Convert to C-style arguments
    std::vector<const char *> argv;
    for (const auto &arg : args) {
        argv.push_back(arg.c_str());
    }
    int argc = argv.size();

    // Create options parser with error handling
    auto OptionsParser = clang::tooling::CommonOptionsParser::create(
        argc, argv.data(), category, llvm::cl::ZeroOrMore);

    if (!OptionsParser) {
        llvm::errs() << "Failed to create options parser: "
                     << llvm::toString(OptionsParser.takeError()) << "\n";
        return;
    }

    // Run tool with proper error handling
    clang::tooling::ClangTool tool(OptionsParser->getCompilations(),
                                   OptionsParser->getSourcePathList());

    auto factory = std::make_unique<CustomActionFactory>(renamer);
    if (int result = tool.run(factory.get())) {
        llvm::errs() << "Tool failed with code: " << result << "\n";
        return;
    }

    std::ofstream out(outputFile);
    out << renamer.getCombinedOutput();
}

int main(int argc, const char **argv) {
    llvm::InitLLVM init(argc, argv);

    llvm::cl::OptionCategory category("tinysea Options");
    llvm::cl::opt<std::string> cmakeProject(
        "cmake-project", llvm::cl::desc("Process entire CMake project"),
        llvm::cl::cat(category));
    llvm::cl::opt<std::string> outputFile(
        "output", llvm::cl::desc("Output file for transformed code"),
        llvm::cl::cat(category));
    llvm::cl::opt<std::string> MappingFile(
        "mapping", llvm::cl::desc("Specify mapping file"),
        llvm::cl::value_desc("filename"), llvm::cl::cat(category));

    llvm::cl::HideUnrelatedOptions(category);
    llvm::cl::ParseCommandLineOptions(argc, argv, "tinysea\n");

    Renamer renamer;

    if (!MappingFile.empty())
        renamer.loadMappings(MappingFile);

    if (cmakeProject.empty())
        return 1;

    processCMakeProject(cmakeProject, outputFile, renamer, category);

    std::cout << "in here" << std::endl;

    // Only save if there are mappings and a filename was specified
    if (!MappingFile.empty() && renamer.hasMappings()) {
        renamer.saveMappings(MappingFile);
    }

    return 0;
}
/*
    auto ExpectedParser =
        CommonOptionsParser::create(argc, argv, MyToolCategory);
    if (!ExpectedParser) {
        llvm::errs() << "Error creating options parser: "
                     << llvm::toString(ExpectedParser.takeError()) << "\n";
        return 1;
    }

    CommonOptionsParser &opts = ExpectedParser.get();
    ClangTool tool(opts.getCompilations(), opts.getSourcePathList());

    CustomActionFactory factory(renamer);
    int result = tool.run(&factory);

    return result;
*/
