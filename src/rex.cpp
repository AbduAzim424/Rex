/*
 * Copyright (c) 2024, shAdE424
 * All rights reserved.
 *
 * This file is part of Rex, licensed under the BSD 3-Clause License.
 * See the LICENSE file at the root of this repository for full details.
 */

#include "../include/rex.hpp"

void Rex::init()
{
    m_connection = xcb_connect(nullptr, nullptr);

    if(xcb_connection_has_error(m_connection))
    {
        std::cerr << "Could not connect to X server\nProcess Aborted\n";
        exit(-1);
    }
}

void Rex::runEventLoop()
{
    xcb_generic_event_t *event;

    m_ui.drawUI("", {}, 0);
    while ((event = xcb_wait_for_event(m_connection))) 
    {
        m_renderTextBuffer = m_inputHandler.processEvents(event);
        m_renderSuggestions = m_inputHandler.getSuggestions();
        m_index_suggestion = m_inputHandler.getIndexSuggestion();

        m_ui.updateUI(m_renderTextBuffer, m_renderSuggestions, m_index_suggestion);
    }
}