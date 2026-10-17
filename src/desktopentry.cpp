/*
 * Copyright (c) 2024, shAdE424
 * All rights reserved.
 *
 * This file is part of Rex, licensed under the BSD 3-Clause License.
 * See the LICENSE file at the root of this repository for full details.
 */

#include "desktopentry.hpp"
#include "executionengine.hpp"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>
#include <optional>
#include <unordered_set>

namespace fs = std::filesystem;

namespace
{
    std::vector<fs::path> applicationDirectories()
    {
        std::vector<fs::path> dirs;

        const char* dataHome = std::getenv("XDG_DATA_HOME");
        const char* home = std::getenv("HOME");
        if (dataHome && *dataHome)
        {
            dirs.emplace_back(fs::path(dataHome) / "applications");
        }
        else if (home && *home)
        {
            dirs.emplace_back(fs::path(home) / ".local/share/applications");
        }

        const char* dataDirs = std::getenv("XDG_DATA_DIRS");
        std::string list = (dataDirs && *dataDirs) ? dataDirs : "/usr/local/share:/usr/share";

        size_t start = 0;
        while (start <= list.size())
        {
            size_t end = list.find(':', start);
            if (end == std::string::npos)
            {
                end = list.size();
            }
            if (end > start)
            {
                dirs.emplace_back(fs::path(list.substr(start, end - start)) / "applications");
            }
            start = end + 1;
        }

        return dirs;
    }

    // Decodes the escape sequences allowed in .desktop string values.
    std::string unescapeValue(const std::string& value)
    {
        std::string out;
        for (size_t i = 0; i < value.size(); ++i)
        {
            if (value[i] != '\\' || i + 1 == value.size())
            {
                out += value[i];
                continue;
            }

            switch (value[++i])
            {
                case 's':  out += ' ';  break;
                case 'n':  out += '\n'; break;
                case 't':  out += '\t'; break;
                case 'r':  out += '\r'; break;
                case '\\': out += '\\'; break;
                default:   out += '\\'; out += value[i]; break;
            }
        }
        return out;
    }

    // Removes Exec field codes such as %f, %U or %i, which only make sense
    // when opening files, and turns %% into a literal %.
    std::string stripFieldCodes(const std::string& exec)
    {
        std::string out;
        for (size_t i = 0; i < exec.size(); ++i)
        {
            if (exec[i] != '%' || i + 1 == exec.size())
            {
                out += exec[i];
                continue;
            }

            if (exec[++i] == '%')
            {
                out += '%';
            }
        }
        return out;
    }

    // Reads the key/value pairs of the [Desktop Entry] group. Localised keys
    // such as Name[de] are ignored.
    std::map<std::string, std::string> readDesktopGroup(const fs::path& file)
    {
        std::map<std::string, std::string> values;
        std::ifstream in(file);
        std::string line;
        bool inGroup = false;

        while (std::getline(in, line))
        {
            if (line.empty() || line[0] == '#')
            {
                continue;
            }
            if (line[0] == '[')
            {
                inGroup = (line == "[Desktop Entry]");
                continue;
            }
            if (!inGroup)
            {
                continue;
            }

            const size_t eq = line.find('=');
            if (eq == std::string::npos)
            {
                continue;
            }

            std::string key = line.substr(0, eq);
            while (!key.empty() && key.back() == ' ')
            {
                key.pop_back();
            }
            if (key.find('[') != std::string::npos)
            {
                continue;
            }

            size_t valueStart = line.find_first_not_of(' ', eq + 1);
            values.emplace(key, valueStart == std::string::npos ? "" : line.substr(valueStart));
        }

        return values;
    }

    std::optional<DesktopEntry> parseDesktopFile(const fs::path& file)
    {
        const std::map<std::string, std::string> values = readDesktopGroup(file);

        auto get = [&values](const std::string& key) -> std::string
        {
            auto it = values.find(key);
            return it == values.end() ? std::string() : it->second;
        };

        if (get("Type") != "Application" || get("NoDisplay") == "true" || get("Hidden") == "true")
        {
            return std::nullopt;
        }

        DesktopEntry entry;
        entry.name = unescapeValue(get("Name"));
        std::optional<std::vector<std::string>> command = splitCommandLine(stripFieldCodes(unescapeValue(get("Exec"))));

        if (entry.name.empty() || !command || command->empty())
        {
            return std::nullopt;
        }

        entry.command = std::move(*command);
        return entry;
    }
}

std::vector<DesktopEntry> loadDesktopEntries()
{
    std::vector<DesktopEntry> entries;
    std::unordered_set<std::string> seenIds;

    for (const fs::path& dir : applicationDirectories())
    {
        std::error_code ec;
        if (!fs::is_directory(dir, ec))
        {
            continue;
        }

        for (fs::recursive_directory_iterator it(dir, fs::directory_options::skip_permission_denied, ec), end;
             !ec && it != end; it.increment(ec))
        {
            if (it->path().extension() != ".desktop" || !it->is_regular_file(ec))
            {
                continue;
            }

            // The desktop file ID is the path relative to the applications
            // directory with '/' replaced by '-'. An earlier directory's file
            // shadows later ones even if it is hidden, which is how users
            // override or remove system entries.
            std::string id = fs::relative(it->path(), dir, ec).string();
            std::replace(id.begin(), id.end(), '/', '-');
            if (!seenIds.insert(id).second)
            {
                continue;
            }

            if (std::optional<DesktopEntry> entry = parseDesktopFile(it->path()))
            {
                entries.push_back(std::move(*entry));
            }
        }
    }

    return entries;
}
