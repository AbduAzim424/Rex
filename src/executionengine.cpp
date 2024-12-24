/*
 * Copyright (c) 2024, shAdE424
 * All rights reserved.
 *
 * This file is part of Rex, licensed under the BSD 3-Clause License.
 * See the LICENSE file at the root of this repository for full details.
 */

#include "../include/executionengine.hpp"

void ExecutionEngine::executeApplicationAndExit(const std::string& application, const std::vector<std::string>& args)
{
    if (application.empty()) return;

    if (executeApplication(application, args)) 
    {
        exit(0); 
    }
}

bool ExecutionEngine::executeApplication(const std::string& application, const std::vector<std::string>& args)
{
    pid_t pid = fork();
    if (pid < 0) 
    {
        handleError("Fork failed!");
        return false;
    }

    if (pid == 0) 
    {
        std::vector<const char*> execArgs;
        execArgs.push_back(application.c_str());
        for (const auto& arg : args) 
        {
            execArgs.push_back(arg.c_str());
        }
        execArgs.push_back(nullptr);

        execvp(application.c_str(), const_cast<char**>(execArgs.data()));

        // If execvp fails
        handleError("Failed to execute application: " + application);
        exit(-1);
    }

    return true;
}

void ExecutionEngine::handleError(const std::string& message)
{
    std::cerr << "Execution Error: " << message << std::endl;
}