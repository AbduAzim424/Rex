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

class Rex final
{
public:
    Rex();

    void run();

private:
    xcb_connection_t*  m_connection;
    InputHandler       m_inputHandler;
    UI                 m_ui;
};
