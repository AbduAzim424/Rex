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

#include <xcb/xcb.h>

/*
 * Top-level application object. Connects to the X server, wires the input
 * handler to the UI and runs the event loop.
 */
class Rex final
{
public:
    // Connects to the X server and creates the window. Exits on failure.
    Rex();

    // Blocks processing X events until the connection closes or the user
    // launches a program / quits.
    void run();

private:
    xcb_connection_t*  m_connection;
    InputHandler       m_inputHandler;
    UI                 m_ui;
};
