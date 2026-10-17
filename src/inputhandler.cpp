/*
 * Copyright (c) 2024, shAdE424
 * All rights reserved.
 *
 * This file is part of Rex, licensed under the BSD 3-Clause License.
 * See the LICENSE file at the root of this repository for full details.
 */

#include "inputhandler.hpp"

#include <cstdlib>
#include <iostream>

#include <X11/keysym.h>

bool InputHandler::init(xcb_connection_t* connection, size_t maxSuggestions)
{
    if (xcb_connection_has_error(connection))
    {
        logError("Connection has an error.");
        return false;
    }
    m_connection = connection;

    m_key_symbols = xcb_key_symbols_alloc(m_connection);
    if (!m_key_symbols)
    {
        logError("Failed to allocate key symbols.");
        return false;
    }

    m_suggestions.populateFromPath();
    m_max_suggestion = maxSuggestions;

    return true;
}

std::string_view InputHandler::processEvent(xcb_generic_event_t* event)
{
    if (((event->response_type) & ~0x80) == XCB_KEY_PRESS)
    {
        xcb_key_press_event_t* key_event = reinterpret_cast<xcb_key_press_event_t*>(event);
        processKeyPress(key_event);
    }

    // Return the current state of the input buffer
    return std::string_view(m_inputBuffer);
}

char InputHandler::mapKeysymToChar(xcb_keysym_t keysym)
{
    // Handle printable ASCII characters
    if (keysym >= 0x20 && keysym <= 0x7E)
    {
        return static_cast<char>(keysym);
    }

    return '\0';
}

void InputHandler::processKeyPress(xcb_key_press_event_t* k_event)
{
    xcb_keycode_t keycode = k_event->detail;

    // Column 0 of the keysym table is the unshifted symbol, column 1 shifted
    bool shift_active = k_event->state & XCB_MOD_MASK_SHIFT;

    int column = shift_active ? 1 : 0;
    xcb_keysym_t keysym = xcb_key_symbols_get_keysym(m_key_symbols, keycode, column);

    switch (keysym)
    {
        case XK_Return:
        {
            if (m_suggestion_index < m_text_suggestions.size())
            {
                m_quit_requested = m_exec_engine.launch(m_text_suggestions[m_suggestion_index]);
            }
            break;
        }
        case XK_BackSpace:
        {
            if (!m_inputBuffer.empty())
            {
                m_inputBuffer.pop_back();
                updateSuggestions();
            }
            break;
        }
        case XK_Escape:
        {
            m_quit_requested = true;
            break;
        }
        case XK_Up:
        {
            if (!m_text_suggestions.empty())
            {
                const size_t count = m_text_suggestions.size();
                m_suggestion_index = (m_suggestion_index + count - 1) % count;
            }
            break;
        }
        case XK_Down:
        {
            if (!m_text_suggestions.empty())
            {
                m_suggestion_index = (m_suggestion_index + 1) % m_text_suggestions.size();
            }
            break;
        }
        default:
        {
            // Convert the keysym to a character and append it
            char ch = mapKeysymToChar(keysym);
            if (ch != '\0')
            {
                m_inputBuffer += ch;
                updateSuggestions();
            }
            break;
        }
    }
}

void InputHandler::logError(const std::string& error_message)
{
    std::cerr << "InputHandler Error: " << error_message << std::endl;
}

void InputHandler::updateSuggestions()
{
    m_text_suggestions = m_suggestions.getMatches(m_inputBuffer, m_max_suggestion);
    m_suggestion_index = 0;
}

const std::vector<std::string>& InputHandler::getSuggestions() const
{
    return m_text_suggestions;
}

bool InputHandler::quitRequested() const
{
    return m_quit_requested;
}

size_t InputHandler::getSelectedIndex() const
{
    return m_suggestion_index;
}