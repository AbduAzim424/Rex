/*
 * Copyright (c) 2024, shAdE424
 * All rights reserved.
 *
 * This file is part of Rex, licensed under the BSD 3-Clause License.
 * See the LICENSE file at the root of this repository for full details.
 */

#pragma once

#include <string>
#include <unordered_set>
#include <vector>

/*
 * Something Rex can launch: an installed application or a $PATH executable.
 */
struct Suggestion
{
    std::string               label;          // shown in the list and matched against
    std::vector<std::string>  command;        // argv to run
    bool                      isApplication;  // from a .desktop file
};

/*
 * The set of launchable programs and the matching logic over them.
 */
class Suggestions final
{
public:
    // Adds applications from .desktop files. Call before populateFromPath()
    // so an application wins over an executable with the same name.
    void populateFromDesktopFiles();

    // Adds every executable regular file found in the $PATH directories.
    void populateFromPath();

    // Returns up to maxResults suggestions whose label matches input, ignoring
    // case. Exact matches rank first, then prefix, substring and subsequence
    // matches; within each group shorter labels come first, applications
    // before executables, then alphabetical order. Returns nothing for empty
    // input. With allowFuzzy false, subsequence matches are left out.
    std::vector<Suggestion> getMatches(const std::string& input, size_t maxResults, bool allowFuzzy = true) const;

private:
    // How well a word matches the input; lower is better.
    enum class MatchKind { Exact, Prefix, Substring, Subsequence, None };

    static MatchKind classify(const std::string& input, const std::string& word);
    static std::vector<std::string> split(const std::string& str, char delimiter);
    static bool isSubsequence(const std::string& input, const std::string& word);
    static std::string toLower(std::string str);

    void add(Suggestion suggestion);
    void addExecutable(const std::string& name);

    std::vector<Suggestion>          m_entries;
    std::vector<std::string>         m_lowerLabels;  // m_entries' labels lowercased, same order
    std::unordered_set<std::string>  m_seen;
};
