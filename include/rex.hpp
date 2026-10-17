/*
 * Copyright (c) 2024, shAdE424
 * All rights reserved.
 *
 * This file is part of Rex, licensed under the BSD 3-Clause License.
 * See the LICENSE file at the root of this repository for full details.
 */

#pragma once

#include "inputhandler.hpp"
#include "ui.hpp"

#include <memory>

#include <xcb/xcb.h>

/*
 * Top-level application object. Connects to the X server, wires the input
 * handler to the UI and runs the event loop.
 */
class Rex final
{
public:
    // Connects to the X server and creates the window.
    // Throws std::runtime_error on failure.
    Rex();

    // Processes X events until the user launches a program or quits, or the
    // connection closes.
    void run();

private:
    struct ConnectionDeleter
    {
        void operator()(xcb_connection_t* connection) const { xcb_disconnect(connection); }
    };

    // Declared first so it is destroyed last, after the window and key
    // symbols that depend on it.
    std::unique_ptr<xcb_connection_t, ConnectionDeleter>  m_connection;
    InputHandler                                          m_inputHandler;
    UI                                                    m_ui;
};
