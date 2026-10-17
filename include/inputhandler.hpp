/*
 * Copyright (c) 2024, shAdE424
 * All rights reserved.
 *
 * This file is part of Rex, licensed under the BSD 3-Clause License.
 * See the LICENSE file at the root of this repository for full details.
 */

#pragma once

#include "executionengine.hpp"
#include "suggestion.hpp"

#include <string>
#include <string_view>
#include <vector>

#include <sys/types.h>
#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>

/*
 * Turns key presses into an input string and a selected suggestion, and
 * launches the selection on Enter.
 */
class InputHandler final
{
public:
    InputHandler() : m_connection(nullptr), m_suggestion_index(0)
    {
    }

    ~InputHandler()
    {
        if (m_key_symbols)
        {
            xcb_key_symbols_free(m_key_symbols);
        }
    }

    // Loads key symbols and scans $PATH. maxSuggestions caps the result list.
    bool              init(xcb_connection_t* connection, ssize_t maxSuggestions);

    // Handles one X event and returns the current query text.
    std::string_view  processEvent(xcb_generic_event_t* event);

    const std::vector<std::string>&  getSuggestions() const;
    ssize_t                          getSelectedIndex() const;

private:
    void         processKeyPress(xcb_key_press_event_t* k_event);
    void         updateSuggestions();
    static char  mapKeysymToChar(xcb_keysym_t keysym);
    static void  logError(const std::string& error_message);

    xcb_connection_t*         m_connection;
    xcb_key_symbols_t*        m_key_symbols;

    ExecutionEngine           m_exec_engine;
    Suggestions               m_suggestions;

    std::string               m_inputBuffer;
    std::vector<std::string>  m_text_suggestions;
    ssize_t                   m_suggestion_index;
    ssize_t                   m_max_suggestion;
};
