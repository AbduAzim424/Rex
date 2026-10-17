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

/*
 * An installed application described by a freedesktop.org .desktop file.
 */
struct DesktopEntry
{
    std::string               name;     // Name= value
    std::vector<std::string>  command;  // Exec= value split into argv, field codes removed
};

// Loads visible applications from the "applications" subdirectory of
// $XDG_DATA_HOME and each $XDG_DATA_DIRS entry, in that order. When several
// directories contain the same desktop file ID, only the first is used.
std::vector<DesktopEntry> loadDesktopEntries();
