#include "IniDocument.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <system_error>


namespace fs = std::filesystem;

namespace core {

namespace {

constexpr std::size_t notFound = static_cast<std::size_t>(-1);

}

std::string IniDocument::trim(const std::string& text) {
    const auto start = text.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) {
        return "";
    }
    const auto end = text.find_last_not_of(" \t\r\n");
    return text.substr(start, end - start + 1);
}

std::string IniDocument::toLower(const std::string& text) {
    std::string result = text;
    std::transform(result.begin(), result.end(), result.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return result;
}

bool IniDocument::load(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    entries.clear();
    std::string currentSection;
    std::string line;

    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        Entry entry;
        entry.raw = line;
        entry.section = currentSection;

        const std::string trimmed = trim(line);

        if (trimmed.empty() || trimmed[0] == ';' || trimmed[0] == '#') {
            entry.kind = Kind::Text;
            entries.push_back(std::move(entry));
            continue;
        }

        if (trimmed.front() == '[' && trimmed.back() == ']') {
            currentSection = trim(trimmed.substr(1, trimmed.size() - 2));
            entry.kind = Kind::Section;
            entry.section = currentSection;
            entries.push_back(std::move(entry));
            continue;
        }

        const auto separator = trimmed.find('=');
        if (separator == std::string::npos || currentSection.empty()) {
            entry.kind = Kind::Text;
            entries.push_back(std::move(entry));
            continue;
        }

        entry.kind = Kind::KeyValue;
        entry.key = trim(trimmed.substr(0, separator));
        entry.value = trim(trimmed.substr(separator + 1));
        entries.push_back(std::move(entry));
    }

    return true;
}

bool IniDocument::save(const std::string& filePath) const {
    const std::string temporaryPath = filePath + ".tmp";

    {
        std::ofstream file(temporaryPath, std::ios::out | std::ios::binary | std::ios::trunc);
        if (!file.is_open()) {
            return false;
        }

        for (const auto& entry : entries) {
            if (entry.kind == Kind::KeyValue) {
                if (entry.dirty || entry.raw.empty()) {
                    file << entry.key << " = " << entry.value << "\n";
                }
                else {
                    file << entry.raw << "\n";
                }
            }
            else if (entry.kind == Kind::Section) {
                file << (entry.raw.empty() ? "[" + entry.section + "]" : entry.raw) << "\n";
            }
            else {
                file << entry.raw << "\n";
            }
        }

        if (!file.good()) {
            return false;
        }
    }

    std::error_code error;
    fs::rename(temporaryPath, filePath, error);
    if (error) {
        fs::remove(temporaryPath, error);
        return false;
    }
    return true;
}

std::size_t IniDocument::findSection(const std::string& section) const {
    const std::string wanted = toLower(section);
    for (std::size_t i = 0; i < entries.size(); i++) {
        if (entries[i].kind == Kind::Section && toLower(entries[i].section) == wanted) {
            return i;
        }
    }
    return notFound;
}

std::size_t IniDocument::findKey(const std::string& section, const std::string& key) const {
    const std::string wantedSection = toLower(section);
    const std::string wantedKey = toLower(key);

    for (std::size_t i = 0; i < entries.size(); i++) {
        if (entries[i].kind == Kind::KeyValue &&
            toLower(entries[i].section) == wantedSection &&
            toLower(entries[i].key) == wantedKey) {
            return i;
        }
    }
    return notFound;
}

std::size_t IniDocument::insertionPoint(const std::string& section) const {
    const std::size_t start = findSection(section);
    if (start == notFound) {
        return notFound;
    }

    std::size_t end = start + 1;
    while (end < entries.size() && entries[end].kind != Kind::Section) {
        end++;
    }

    while (end > start + 1 && entries[end - 1].kind == Kind::Text &&
        trim(entries[end - 1].raw).empty()) {
        end--;
    }

    return end;
}

bool IniDocument::hasSection(const std::string& section) const {
    return findSection(section) != notFound;
}

std::vector<std::string> IniDocument::sections() const {
    std::vector<std::string> result;
    for (const auto& entry : entries) {
        if (entry.kind == Kind::Section) {
            result.push_back(entry.section);
        }
    }
    return result;
}

bool IniDocument::hasKey(const std::string& section, const std::string& key) const {
    return findKey(section, key) != notFound;
}

std::string IniDocument::get(const std::string& section, const std::string& key,
        const std::string& defaultValue) const {
    const std::size_t index = findKey(section, key);
    return index == notFound ? defaultValue : entries[index].value;
}

void IniDocument::addSection(const std::string& section) {
    if (hasSection(section)) {
        return;
    }

    if (!entries.empty()) {
        Entry blank;
        blank.kind = Kind::Text;
        blank.raw = "";
        entries.push_back(std::move(blank));
    }

    Entry header;
    header.kind = Kind::Section;
    header.section = section;
    header.raw = "[" + section + "]";
    entries.push_back(std::move(header));
}

void IniDocument::set(const std::string& section, const std::string& key, const std::string& value) {
    const std::size_t existing = findKey(section, key);
    if (existing != notFound) {
        if (entries[existing].value != value) {
            entries[existing].value = value;
            entries[existing].dirty = true;
        }
        return;
    }

    addSection(section);

    Entry entry;
    entry.kind = Kind::KeyValue;
    entry.section = section;
    entry.key = key;
    entry.value = value;
    entry.dirty = true;

    const std::size_t position = insertionPoint(section);
    if (position == notFound || position >= entries.size()) {
        entries.push_back(std::move(entry));
    }
    else {
        entries.insert(entries.begin() + static_cast<std::ptrdiff_t>(position), std::move(entry));
    }
}

void IniDocument::removeKey(const std::string& section, const std::string& key) {
    const std::size_t index = findKey(section, key);
    if (index != notFound) {
        entries.erase(entries.begin() + static_cast<std::ptrdiff_t>(index));
    }
}

void IniDocument::removeSection(const std::string& section) {
    const std::size_t start = findSection(section);
    if (start == notFound) {
        return;
    }

    std::size_t end = start + 1;
    while (end < entries.size() && entries[end].kind != Kind::Section) {
        end++;
    }

    entries.erase(entries.begin() + static_cast<std::ptrdiff_t>(start),
        entries.begin() + static_cast<std::ptrdiff_t>(end));
}

}
