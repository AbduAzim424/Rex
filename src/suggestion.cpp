/*
 * Copyright (c) 2024, shAdE424
 * All rights reserved.
 *
 * This file is part of Rex, licensed under the BSD 3-Clause License.
 * See the LICENSE file at the root of this repository for full details.
 */

#include "suggestion.hpp"
#include "desktopentry.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

void Suggestions::populateFromDesktopFiles()
{
    for (DesktopEntry& entry : loadDesktopEntries())
    {
        add({std::move(entry.name), std::move(entry.command), true});
    }
}

void Suggestions::populateFromPath()
{
    const char* path_env = std::getenv("PATH");
    if (!path_env)
    {
        std::cerr << "Error: PATH environment variable not found.\n";
        return;
    }

    for (const auto& path : split(path_env, ':'))
    {
        if (!fs::exists(path) || !fs::is_directory(path))
        {
            std::cerr << "Skipping invalid path: " << path << "\n";
            continue;
        }

        try
        {
            for (const auto& entry : fs::directory_iterator(path))
            {
                if (!entry.is_regular_file())
                {
                    continue;
                }

                const auto exec_bits = fs::perms::owner_exec | fs::perms::group_exec | fs::perms::others_exec;
                if ((entry.status().permissions() & exec_bits) != fs::perms::none)
                {
                    addExecutable(entry.path().filename().string());
                }
            }
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error accessing directory '" << path << "': " << e.what() << "\n";
        }
    }
}

void Suggestions::addExecutable(const std::string& name)
{
    // Ignore names with only non-alphanumeric characters or single-character filenames
    if (name.size() <= 1 || std::none_of(name.begin(), name.end(), ::isalnum))
    {
        return;
    }

    add({name, {name}, false});
}

void Suggestions::add(Suggestion suggestion)
{
    // The same name can appear in several $PATH directories (or the same
    // directory can be listed twice), and an application can share its name
    // with an executable. Keep the first; applications are added first.
    if (m_seen.insert(suggestion.label).second)
    {
        m_lowerLabels.push_back(toLower(suggestion.label));
        m_entries.push_back(std::move(suggestion));
    }
}

std::vector<Suggestion> Suggestions::getMatches(const std::string& input, size_t maxResults, bool allowFuzzy) const
{
    if (input.empty())
    {
        return {};
    }

    const std::string needle = toLower(input);

    struct Ranked
    {
        MatchKind kind;
        const Suggestion* entry;
    };

    std::vector<Ranked> ranked;
    for (size_t i = 0; i < m_entries.size(); ++i)
    {
        MatchKind kind = classify(needle, m_lowerLabels[i]);
        if (kind != MatchKind::None && (allowFuzzy || kind != MatchKind::Subsequence))
        {
            ranked.push_back({kind, &m_entries[i]});
        }
    }

    std::sort(ranked.begin(), ranked.end(), [](const Ranked& a, const Ranked& b)
    {
        if (a.kind != b.kind)
        {
            return a.kind < b.kind;
        }
        if (a.entry->label.size() != b.entry->label.size())
        {
            return a.entry->label.size() < b.entry->label.size();
        }
        if (a.entry->isApplication != b.entry->isApplication)
        {
            return a.entry->isApplication;
        }
        return a.entry->label < b.entry->label;
    });

    std::vector<Suggestion> matches;
    for (size_t i = 0; i < ranked.size() && i < maxResults; ++i)
    {
        matches.push_back(*ranked[i].entry);
    }

    return matches;
}

// Both arguments are expected to be lowercase already.
Suggestions::MatchKind Suggestions::classify(const std::string& input, const std::string& word)
{
    if (word == input)
    {
        return MatchKind::Exact;
    }

    const size_t pos = word.find(input);
    if (pos == 0)
    {
        return MatchKind::Prefix;
    }
    if (pos != std::string::npos)
    {
        return MatchKind::Substring;
    }
    if (isSubsequence(input, word))
    {
        return MatchKind::Subsequence;
    }
    return MatchKind::None;
}

std::vector<std::string> Suggestions::split(const std::string& str, char delimiter)
{
    std::vector<std::string> tokens;
    size_t start = 0, end = 0;
    while ((end = str.find(delimiter, start)) != std::string::npos)
    {
        tokens.push_back(str.substr(start, end - start));
        start = end + 1;
    }
    tokens.push_back(str.substr(start));
    return tokens;
}

// True if all characters of input appear in word in the same order,
// e.g. "ffx" is a subsequence of "firefox".
bool Suggestions::isSubsequence(const std::string& input, const std::string& word)
{
    size_t i = 0, j = 0;
    while (i < input.size() && j < word.size())
    {
        if (input[i] == word[j])
        {
            i++;
        }
        j++;
    }
    return i == input.size();
}

std::string Suggestions::toLower(std::string str)
{
    std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c)
    {
        return static_cast<char>(std::tolower(c));
    });
    return str;
}
