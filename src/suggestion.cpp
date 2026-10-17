/*
 * Copyright (c) 2024, shAdE424
 * All rights reserved.
 *
 * This file is part of Rex, licensed under the BSD 3-Clause License.
 * See the LICENSE file at the root of this repository for full details.
 */

#include "suggestion.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

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
                    addWord(entry.path().filename().string());
                }
            }
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error accessing directory '" << path << "': " << e.what() << "\n";
        }
    }
}

void Suggestions::addWord(const std::string& word)
{
    // Ignore words with only non-alphanumeric characters or single-character filenames
    if (word.size() <= 1 || std::none_of(word.begin(), word.end(), ::isalnum))
    {
        return;
    }

    // The same name can appear in several $PATH directories (or the same
    // directory can be listed twice); only the first one is reachable anyway.
    if (m_seen.insert(word).second)
    {
        m_words.push_back(word);
    }
}

std::vector<std::string> Suggestions::getMatches(const std::string& input, size_t maxResults) const
{
    if (input.empty())
    {
        return {};
    }

    std::vector<std::string> matches;
    for (const auto& word : m_words)
    {
        if (isSubsequence(input, word))
        {
            matches.push_back(word);
        }
    }

    std::sort(matches.begin(), matches.end(), [](const std::string& a, const std::string& b)
    {
        if (a.size() != b.size())
        {
            return a.size() < b.size();
        }
        return a < b;
    });

    if (matches.size() > maxResults)
    {
        matches.resize(maxResults);
    }

    return matches;
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
