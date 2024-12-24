/*
 * Copyright (c) 2024, shAdE424
 * All rights reserved.
 *
 * This file is part of Rex, licensed under the BSD 3-Clause License.
 * See the LICENSE file at the root of this repository for full details.
 */

#pragma once

#include "types.hpp"
#include "executionengine.hpp"
#include "suggestion.hpp"

class InputHandler final 
{
public:
    InputHandler() : m_connection(nullptr), m_suggestion_index(0)
    {
    }

    ~InputHandler()
    {
        if (m_keysyms_loaded) 
        {
            free(m_reply);
        }

        if (m_key_symbols) 
        {
            xcb_key_symbols_free(m_key_symbols);
        }
    }

    bool                       init(xcb_connection_t* connection, xcb_window_t window_id, ssize_t max_suggeestions);
    std::string_view           processEvents(xcb_generic_event_t* event);
    char                       mapKeysymToChar(xcb_keysym_t keysym);

    std::vector<std::string>&  getSuggestions();
    ssize_t                    getIndexSuggestion() const;
private:
    void  processKeyPress(xcb_key_press_event_t* k_event);
    bool  loadKeyMapping();

    void  logError(const std::string& error_message);
private:
    xcb_connection_t*   m_connection;
    const xcb_setup_t*  m_setup;
    xcb_window_t        m_window_id;

    ExecutionEngine     m_exec_engine;
    Suggestions         m_suggestions;

    std::string         m_inputBuffer;

    xcb_keycode_t       m_first_keycode; 
    xcb_keycode_t       m_last_keycode;
    int                 m_num_keycodes;
    int                 m_keysyms_per_keycode;

    std::vector<std::vector<xcb_keysym_t>>  m_keysyms;

    bool                                    m_keysyms_loaded;
    xcb_get_keyboard_mapping_reply_t*       m_reply;
    xcb_key_symbols_t*                      m_key_symbols;

    ssize_t                                 m_suggestion_index;
    ssize_t                                 m_max_suggestion;
    std::vector<std::string>                m_text_suggestions;
};