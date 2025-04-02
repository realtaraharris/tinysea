#include "stdafx.h"

std::string Renamer::generateName(unsigned index) {
    std::string name;
    unsigned n = index;

    do {
        unsigned remainder = n % 26;
        name.push_back('a' + remainder);
        n = n / 26;
    } while (n > 0);

    std::reverse(name.begin(), name.end());
    return name;
}

void Renamer::initKeywords() {
    const std::vector<std::string> keywords = {
        "alignas",      "alignof",      "and",           "and_eq",
        "asm",          "auto",         "bitand",        "bitor",
        "bool",         "break",        "case",          "catch",
        "char",         "char8_t",      "char16_t",      "char32_t",
        "class",        "compl",        "concept",       "const",
        "consteval",    "constexpr",    "const_cast",    "continue",
        "co_await",     "co_return",    "co_yield",      "decltype",
        "default",      "define",       "delete",        "do",
        "double",       "dynamic_cast", "else",          "enum",
        "explicit",     "export",       "extern",        "false",
        "float",        "for",          "friend",        "goto",
        "if",           "inline",       "include",       "int",
        "long",         "mutable",      "namespace",     "new",
        "noexcept",     "not",          "not_eq",        "nullptr",
        "operator",     "or",           "or_eq",         "private",
        "protected",    "public",       "register",      "reinterpret_cast",
        "requires",     "return",       "short",         "signed",
        "sizeof",       "static",       "static_assert", "static_cast",
        "struct",       "switch",       "template",      "this",
        "thread_local", "throw",        "true",          "try",
        "typedef",      "typeid",       "typename",      "union",
        "unsigned",     "using",        "virtual",       "void",
        "volatile",     "wchar_t",      "while",         "xor",
        "xor_eq"};
    reservedKeywords.insert(keywords.begin(), keywords.end());
}

unsigned Renamer::shortNameToIndex(const std::string &name) {
    unsigned value = 0;
    for (auto it = name.rbegin(); it != name.rend(); ++it) {
        char c = *it;
        if (c < 'a' || c > 'z')
            return UINT_MAX; // Invalid character
        value = value * 26 + (c - 'a' + 1);
    }
    return value - 1; // Account for +1 offset in generation
}

Renamer::Renamer() {
    initKeywords();
}

void Renamer::loadMappings(const std::string &filename) {
    std::ifstream file(filename);
    if (!file)
        return;

    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());

    auto jsonOrError = llvm::json::parse(content);
    if (!jsonOrError) {
        llvm::errs() << "Failed to parse JSON: "
                     << toString(jsonOrError.takeError()) << "\n";
        return;
    }

    unsigned maxIndex = 0;
    if (auto obj = jsonOrError->getAsObject()) {
        for (auto &pair : *obj) {
            std::string original = pair.getFirst().str();
            std::string shortName =
                pair.getSecond().getAsString().value().str();

            // Validate short name format
            bool valid =
                std::all_of(shortName.begin(), shortName.end(),
                            [](char c) { return c >= 'a' && c <= 'z'; });

            if (!valid) {
                llvm::errs()
                    << "Skipping invalid short name: " << shortName << "\n";
                continue;
            }

            // Track maximum index
            unsigned index = shortNameToIndex(shortName);
            if (index != UINT_MAX && index >= maxIndex) {
                maxIndex = index + 1; // Set to next available index
            }

            identifierMap[original] = shortName;
        }
    }

    currentIndex = maxIndex;
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

std::string Renamer::getShortName(const std::string &qualifiedName) {
    static const std::set<std::string> preservedTypes = {
        "int",  "char",      "void",   "bool",      "float",       "double",
        "main", "ptrdiff_t", "size_t", "nullptr_t", "max_align_t", "NULL"};

    // if we find that the qualified name is part of a library, pass it through
    if (preservedTypes.count(qualifiedName) ||
        qualifiedName.starts_with("std::")) {
        return qualifiedName;
    }

    // if we've already seen the thing before, return the associated shortname
    if (auto it = identifierMap.find(qualifiedName);
        it != identifierMap.end()) {
        return it->second;
    }

    // we didn't find the name, so let's make a new name
    std::string newName;

    do { // generate new names until we get one that's not a reserved keyword
        newName = generateName(currentIndex++);
    } while (reservedKeywords.count(newName));

    std::cout << "newName: " << newName << std::endl;

    identifierMap[qualifiedName] = newName;
    return newName;
}

bool Renamer::hasMappings() const {
    return !identifierMap.empty();
}

void Renamer::collectTransformedCode(const std::string &filename,
                                     const std::string &content) {
    combinedOutput << "// ======== " << filename << " ========\n"
                   << content << "\n\n";
}

std::string Renamer::getCombinedOutput() const {
    return combinedOutput.str();
}
