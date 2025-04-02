#pragma once

class Renamer {
    std::unordered_map<std::string, std::string> identifierMap;
    std::unordered_map<std::string, std::string> macroMap;
    std::set<std::string> reservedKeywords;
    std::stringstream combinedOutput;
    unsigned currentIndex = 0;

    std::string generateName(unsigned index);
    void initKeywords();
    unsigned shortNameToIndex(const std::string &name);

public:
    Renamer();
    void loadMappings(const std::string &filename);
    void saveMappings(const std::string &filename);

    std::string getShortName(const std::string &qualifiedName,
                             bool isMacro = false);

    bool hasMappings() const;
    void collectTransformedCode(const std::string &filename,
                                const std::string &content);
    std::string getCombinedOutput() const;
};
