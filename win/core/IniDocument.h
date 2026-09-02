#pragma once

#include <string>
#include <vector>


namespace core {

class IniDocument {
public:
    bool load(const std::string& filePath);
    bool save(const std::string& filePath) const;

    bool hasSection(const std::string& section) const;
    std::vector<std::string> sections() const;

    bool hasKey(const std::string& section, const std::string& key) const;
    std::string get(const std::string& section, const std::string& key,
        const std::string& defaultValue = "") const;

    void set(const std::string& section, const std::string& key, const std::string& value);
    void addSection(const std::string& section);
    void removeSection(const std::string& section);
    void removeKey(const std::string& section, const std::string& key);

    static std::string trim(const std::string& text);
    static std::string toLower(const std::string& text);

private:
    enum class Kind {
        Text,
        Section,
        KeyValue
    };

    struct Entry {
        Kind kind = Kind::Text;
        std::string section;
        std::string key;
        std::string value;
        std::string raw;
        bool dirty = false;
    };

    std::size_t findKey(const std::string& section, const std::string& key) const;
    std::size_t findSection(const std::string& section) const;
    std::size_t insertionPoint(const std::string& section) const;

    std::vector<Entry> entries;
};

}
