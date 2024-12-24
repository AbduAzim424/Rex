/*
 * Copyright (c) 2024, shAdE424
 * All rights reserved.
 *
 * This file is part of Rex, licensed under the BSD 3-Clause License.
 * See the LICENSE file at the root of this repository for full details.
 */

#pragma once

#include "types.hpp"

class ExecutionEngine final
{
public:
    ExecutionEngine()
    {
    }

    ~ExecutionEngine()
    {
    }

    void executeApplicationAndExit(const std::string& application, const std::vector<std::string>& args = {});
private:
    bool executeApplication(const std::string& application, const std::vector<std::string>& args);

    void handleError(const std::string& message);
};