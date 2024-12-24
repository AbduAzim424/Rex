/*
 * Copyright (c) 2024, shAdE424
 * All rights reserved.
 *
 * This file is part of Rex, licensed under the BSD 3-Clause License.
 * See the LICENSE file at the root of this repository for full details.
 */

#pragma once

#include "types.hpp"
#include "inputhandler.hpp"
#include "ui.hpp"

class Rex final
{
public:
    Rex() : m_index_suggestion(0)
    {
        init();

        xcb_window_t id = xcb_generate_id(m_connection);
        m_inputHandler.init(m_connection, id, 6);
        m_ui.init(m_connection, id);
    }

    ~Rex()
    {
    }

    void init();
    void runEventLoop();
public:
    xcb_connection_t*           m_connection;
    std::string_view            m_renderTextBuffer;
    std::vector<std::string>    m_renderSuggestions;

private:
    InputHandler   m_inputHandler;
    UI             m_ui;

    ssize_t        m_index_suggestion;
};