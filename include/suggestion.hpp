/*
 * Copyright (c) 2024, shAdE424
 * All rights reserved.
 *
 * This file is part of Rex, licensed under the BSD 3-Clause License.
 * See the LICENSE file at the root of this repository for full details.
 */

#pragma once

#include <string>
#include <vector>

class Suggestions final
{
public:
    void populateFromPath();
    void addWord(const std::string& word);

    std::vector<std::string> getMatches(const std::string& input, size_t maxResults) const;

private:
    static std::vector<std::string> split(const std::string& str, char delimiter);
    static bool isSubsequence(const std::string& input, const std::string& word);

    std::vector<std::string> m_words;
};
