/*
 * Copyright (c) 2024, shAdE424
 * All rights reserved.
 *
 * This file is part of Rex, licensed under the BSD 3-Clause License.
 * See the LICENSE file at the root of this repository for full details.
 */

#pragma once

#include "types.hpp"

#pragma once

namespace fs = std::filesystem;

struct TrieNode final
{
    bool m_is_end_of_word;
    std::unordered_map<char, std::unique_ptr<TrieNode>> m_children;

    TrieNode() : m_is_end_of_word(false) {}
};

struct Trie final
{
    Trie() : m_root(std::make_unique<TrieNode>()) 
    {}

    void insert(const std::string& word)
    {
        TrieNode* node = m_root.get();
        for (char ch : word)
        {
            if (node->m_children.find(ch) == node->m_children.end())
            {
                node->m_children[ch] = std::make_unique<TrieNode>();
            }
            node = node->m_children[ch].get();
        }
        node->m_is_end_of_word = true;
    }

    std::vector<std::string> get_matches(const std::string& prefix) const
    {
        TrieNode* node = m_root.get();
        std::vector<std::string> matches;

        for (char ch : prefix)
        {
            if (node->m_children.find(ch) == node->m_children.end())
            {
                return matches;
            }
            node = node->m_children[ch].get();
        }

        collect_matches(node, prefix, matches);
        return matches;
    }

private:
    void collect_matches(TrieNode* node, const std::string& current, std::vector<std::string>& matches) const
    {
        if (!node)
            return;

        if (node->m_is_end_of_word)
        {
            matches.push_back(current);
        }

        for (const auto& [ch, child] : node->m_children)
        {
            collect_matches(child.get(), current + ch, matches);
        }
    }

private:
    std::unique_ptr<TrieNode> m_root;
};

class Suggestions final
{
public:
    Suggestions() : m_trie(std::make_unique<Trie>()) {}

    void populate_from_path()
    {
        const char* path_env = std::getenv("PATH");
        if (!path_env)
        {
            std::cerr << "Error: PATH environment variable not found.\n";
            return;
        }

        std::string path_var(path_env);
        std::vector<std::string> paths = split(path_var, ':');

        for (const auto& path : paths)
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
                    if (entry.is_regular_file())
                    {
                        auto perms = entry.status().permissions();

                        if ((perms & fs::perms::owner_exec) != fs::perms::none ||
                            (perms & fs::perms::group_exec) != fs::perms::none ||
                            (perms & fs::perms::others_exec) != fs::perms::none)
                        {
                            std::string filename = entry.path().filename().string();

                            add_word(filename);
                        }
                    }
                }
            }
            catch (const std::exception& e)
            {
                std::cerr << "Error accessing directory '" << path << "': " << e.what() << "\n";
            }
        }
    }

    void add_word(const std::string& word)
    {
        // Ignore words with only non-alphanumeric characters or single-character filenames
        if (word.empty() || word.size() == 1 || std::none_of(word.begin(), word.end(), ::isalnum))
            return;

        m_trie->insert(word);
        m_all_words.push_back(word);
    }

    std::vector<std::string> get_exact_matches(const std::string& prefix) const
    {
        return m_trie->get_matches(prefix);
    }

    std::vector<std::string> get_fuzzy_matches(const std::string& input, int max_distance = 2) const
    {
        if (input.empty())
        {
            return {};
        }

        std::vector<std::string> matches;

        // Collect all matches
        for (const auto& word : m_all_words) 
        {
            if (is_subsequence(input, word)) 
            {
                matches.push_back(word);
            }
        }

        // Sort matches by length (shorter matches first)
        std::sort(matches.begin(), matches.end(), [](const std::string& a, const std::string& b) 
        {
            return a.size() < b.size();
        });

        if (matches.size() > max_distance)
        {
            matches.resize(max_distance); 
        }

        return matches;
    }

    std::vector<std::string> get_best_matches(const std::string& input, int max_distance = 2) const
    {
        std::vector<std::string> matches = get_fuzzy_matches(input, max_distance);

        // Additional filter: Ignore non-alphanumeric or single-character results
        matches.erase(std::remove_if(matches.begin(), matches.end(),
                                    [](const std::string& word)
                                    {
                                        return word.empty() || word.size() == 1 ||
                                                std::none_of(word.begin(), word.end(), ::isalnum);
                                    }),
                    matches.end());

        return matches;
    }

private:
    static std::vector<std::string> split(const std::string& str, char delimiter)
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

    bool is_subsequence(const std::string& input, const std::string& word) const
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

    std::unique_ptr<Trie> m_trie;
    std::vector<std::string> m_all_words;
};