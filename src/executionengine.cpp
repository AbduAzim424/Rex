/*
 * Copyright (c) 2024, shAdE424
 * All rights reserved.
 *
 * This file is part of Rex, licensed under the BSD 3-Clause License.
 * See the LICENSE file at the root of this repository for full details.
 */

#include "executionengine.hpp"

#include <cerrno>
#include <cstring>
#include <iostream>

#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>

bool ExecutionEngine::launch(const std::string& application, const std::vector<std::string>& args)
{
    if (application.empty())
    {
        return false;
    }

    // Build argv before forking so the child does no allocation.
    std::vector<const char*> execArgs;
    execArgs.push_back(application.c_str());
    for (const auto& arg : args)
    {
        execArgs.push_back(arg.c_str());
    }
    execArgs.push_back(nullptr);

    // If exec fails the child writes errno into this pipe. The write end is
    // close-on-exec, so a successful exec closes it and the parent reads EOF.
    int statusPipe[2];
    if (pipe2(statusPipe, O_CLOEXEC) < 0)
    {
        handleError(std::string("pipe failed: ") + std::strerror(errno));
        return false;
    }

    pid_t pid = fork();
    if (pid < 0)
    {
        handleError(std::string("fork failed: ") + std::strerror(errno));
        close(statusPipe[0]);
        close(statusPipe[1]);
        return false;
    }

    if (pid == 0)
    {
        // Child: replace this process with the requested program.
        close(statusPipe[0]);
        execvp(execArgs[0], const_cast<char**>(execArgs.data()));

        int err = errno;
        ssize_t written = write(statusPipe[1], &err, sizeof(err));
        (void)written;
        _exit(127);
    }

    close(statusPipe[1]);

    int childErrno = 0;
    ssize_t bytesRead;
    do
    {
        bytesRead = read(statusPipe[0], &childErrno, sizeof(childErrno));
    } while (bytesRead < 0 && errno == EINTR);
    close(statusPipe[0]);

    if (bytesRead > 0)
    {
        waitpid(pid, nullptr, 0);
        handleError("Failed to execute " + application + ": " + std::strerror(childErrno));
        return false;
    }

    return true;
}

void ExecutionEngine::handleError(const std::string& message)
{
    std::cerr << "Execution Error: " << message << std::endl;
}
