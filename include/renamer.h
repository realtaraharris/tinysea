#pragma once

class Renamer {
    std::mutex mtx;
    std::unordered_map<std::string, std::string> identifierMap;
    std::unordered_map<std::string, std::string> macroMap;
    std::vector<std::string> nameSequence;
    std::set<std::string> reservedKeywords;
    std::unordered_set<std::string> usedShortNames;
    unsigned currentIndex = 0;

    std::string generateName(unsigned index);
    void initKeywords();
    unsigned shortNameToIndex(const std::string &name);

public:
    Renamer();
    bool hasMacro(const std::string &name) const;
    std::string getMacroShortName(const std::string &name) const;
    void loadMappings(const std::string &filename);
    void saveMappings(const std::string &filename);
    std::string getShortName(const std::string &qualifiedName,
                             bool isMacro = false);
    bool hasIdentifier(const std::string &name) const;
    std::string getIdentifierShortName(const std::string &name) const;
    bool hasIdentifierMappings() const;
};
