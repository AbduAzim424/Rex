/*
 * Copyright (c) 2024, shAdE424
 * All rights reserved.
 *
 * This file is part of Rex, licensed under the BSD 3-Clause License.
 * See the LICENSE file at the root of this repository for full details.
 */

#include "rex.hpp"

#include <cstdlib>
#include <stdexcept>

namespace
{
    constexpr size_t kMaxSuggestions = 6;
}

Rex::Rex() : m_connection(xcb_connect(nullptr, nullptr))
{
    if (xcb_connection_has_error(m_connection.get()))
    {
        throw std::runtime_error("Could not connect to X server.");
    }

    if (!m_inputHandler.init(m_connection.get(), kMaxSuggestions))
    {
        throw std::runtime_error("Failed to initialise keyboard input.");
    }

    m_ui.init(m_connection.get());
}

void Rex::run()
{
    m_ui.render("", {}, 0);

    while (xcb_generic_event_t* event = xcb_wait_for_event(m_connection.get()))
    {
        std::string_view query = m_inputHandler.processEvent(event);
        free(event);

        if (m_inputHandler.quitRequested())
        {
            break;
        }

        m_ui.render(query, m_inputHandler.getSuggestions(), m_inputHandler.getSelectedIndex());
    }
}
