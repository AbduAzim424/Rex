/*
 * Copyright (c) 2024, shAdE424
 * All rights reserved.
 *
 * This file is part of Rex, licensed under the BSD 3-Clause License.
 * See the LICENSE file at the root of this repository for full details.
 */

#include "../include/inputhandler.hpp"

bool InputHandler::init(xcb_connection_t* connection, ssize_t max_suggeestions)
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

    m_suggestions.populate_from_path();
    m_max_suggestion = max_suggeestions;

    return true;
}

std::string_view InputHandler::processEvents(xcb_generic_event_t* event)
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
    
    // Determine active modifiers
    bool shift_active = k_event->state & XCB_MOD_MASK_SHIFT;

    int column = shift_active ? 1 : 0;
    xcb_keysym_t keysym = xcb_key_symbols_get_keysym(m_key_symbols, keycode, column);

    switch (keysym)
    {
        case XK_Return:
        {
            m_exec_engine.executeApplicationAndExit(m_text_suggestions[m_suggestion_index], {});
            break;
        }
        case XK_BackSpace:
        {
            if (!m_inputBuffer.empty()) 
            {
                m_inputBuffer.pop_back();
                m_text_suggestions = m_suggestions.get_fuzzy_matches(m_inputBuffer, m_max_suggestion);
                m_suggestion_index = 0;
            }
            break;
        }
        case XK_Escape:
        {
            exit(0);
            break;
        }
        case XK_Up:
        {
            if (m_suggestion_index == 0)
            {
                if (m_max_suggestion > m_text_suggestions.size())
                {
                    m_suggestion_index = (m_text_suggestions.size() - 1);
                }
                else
                {
                    m_suggestion_index = (m_max_suggestion - 1);
                }
            }
            else if (m_suggestion_index <= (m_max_suggestion - 1) && m_suggestion_index > 0)
            {
                --m_suggestion_index;
            }
            break;
        }
        case XK_Down:
        {
            if (m_suggestion_index == (m_max_suggestion - 1))
            {
                m_suggestion_index = 0;
            }
            else if (m_suggestion_index == (m_text_suggestions.size() - 1))
            {
                m_suggestion_index = 0;
            }
            else if (m_suggestion_index >= 0 && m_suggestion_index < m_max_suggestion)
            {
                ++m_suggestion_index;
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
            }

            m_text_suggestions = m_suggestions.get_fuzzy_matches(m_inputBuffer, m_max_suggestion);
            m_suggestion_index = 0;
            break;
        }
    }
}

void InputHandler::logError(const std::string& error_message)
{
    std::cerr << "InputHandler Error: " << error_message << std::endl;
}

std::vector<std::string>& InputHandler::getSuggestions()
{
    return m_text_suggestions;
}

ssize_t InputHandler::getIndexSuggestion() const
{
    return m_suggestion_index;
}