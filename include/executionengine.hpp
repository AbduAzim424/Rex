/*
 * Copyright (c) 2024, shAdE424
 * All rights reserved.
 *
 * This file is part of Rex, licensed under the BSD 3-Clause License.
 * See the LICENSE file at the root of this repository for full details.
 */

#pragma once

#include <optional>
#include <string>
#include <vector>

// Splits a command line into words like a shell would (quotes, ~ and $VAR
// are handled; command substitution is not allowed). Returns std::nullopt
// if the text can't be parsed, e.g. an unbalanced quote or a character such
// as | or ; that only makes sense to a real shell.
std::optional<std::vector<std::string>> splitCommandLine(const std::string& text);

/*
 * Launches programs as detached child processes.
 */
class ExecutionEngine final
{
public:
    // Starts application (looked up in $PATH). Returns true if it was started.
    bool launch(const std::string& application, const std::vector<std::string>& args = {});

private:
    void handleError(const std::string& message);
};
