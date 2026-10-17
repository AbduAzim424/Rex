/*
 * Copyright (c) 2024, shAdE424
 * All rights reserved.
 *
 * This file is part of Rex, licensed under the BSD 3-Clause License.
 * See the LICENSE file at the root of this repository for full details.
 */

#include "rex.hpp"

#include <cstdlib>
#include <iostream>

namespace
{
    constexpr ssize_t kMaxSuggestions = 6;
}

Rex::Rex()
{
    m_connection = xcb_connect(nullptr, nullptr);

    if (xcb_connection_has_error(m_connection))
    {
        std::cerr << "Could not connect to X server\nProcess Aborted\n";
        exit(-1);
    }

    m_inputHandler.init(m_connection, kMaxSuggestions);
    m_ui.init(m_connection);
}

void Rex::run()
{
    m_ui.render("", {}, 0);

    while (xcb_generic_event_t* event = xcb_wait_for_event(m_connection))
    {
        std::string_view query = m_inputHandler.processEvent(event);
        m_ui.render(query, m_inputHandler.getSuggestions(), m_inputHandler.getSelectedIndex());
    }
}
