/*
 * Copyright (c) 2024, shAdE424
 * All rights reserved.
 *
 * This file is part of Rex, licensed under the BSD 3-Clause License.
 * See the LICENSE file at the root of this repository for full details.
 */

#include "inputhandler.hpp"

#include <cctype>
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

    m_suggestions.populateFromDesktopFiles();
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

    // Caps Lock inverts the case of letters only, so Shift+Caps gives lowercase
    if ((k_event->state & XCB_MOD_MASK_LOCK) && keysym < 0x80 && std::isalpha(static_cast<int>(keysym)))
    {
        int ch = static_cast<int>(keysym);
        keysym = std::isupper(ch) ? std::tolower(ch) : std::toupper(ch);
    }

    switch (keysym)
    {
        case XK_Return:
        {
            launchSelection();
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
    // Multi-word input may be a name with spaces ("visual studio"). Accept
    // only exact, prefix or substring hits for it, so that a command followed
    // by arguments doesn't fuzzy-match some unrelated long name.
    std::string query = trimmedInput();
    std::vector<Suggestion> matches;
    bool whole = false;
    if (query.find(' ') != std::string::npos)
    {
        matches = m_suggestions.getMatches(query, m_max_suggestion, false);
        whole = !matches.empty();
    }

    // Otherwise only the command word is matched, so typing arguments keeps
    // the current list and selection.
    if (!whole)
    {
        query = commandWord();
        if (!m_matched_whole && query == m_matched_query)
        {
            return;
        }
        matches = m_suggestions.getMatches(query, m_max_suggestion);
    }

    m_matched_query = std::move(query);
    m_matched_whole = whole;
    m_matches = std::move(matches);
    m_text_suggestions.clear();
    for (const Suggestion& match : m_matches)
    {
        m_text_suggestions.push_back(match.label);
    }
    m_suggestion_index = 0;
}

// Runs the selected suggestion, or the typed command if nothing matched. When
// the suggestions came from the command word, the rest of the input is
// appended as arguments.
void InputHandler::launchSelection()
{
    std::vector<std::string> command = m_suggestion_index < m_matches.size()
        ? m_matches[m_suggestion_index].command
        : std::vector<std::string>{commandWord()};
    if (command.empty() || command[0].empty())
    {
        return;
    }

    const std::string argText = m_matched_whole ? std::string() : argumentText();
    std::optional<std::vector<std::string>> args = splitCommandLine(argText);
    if (!args)
    {
        logError("Could not parse arguments: " + argText);
        return;
    }
    command.insert(command.end(), args->begin(), args->end());

    const std::string program = command.front();
    command.erase(command.begin());
    m_quit_requested = m_exec_engine.launch(program, command);
}

// The input without leading or trailing spaces.
std::string InputHandler::trimmedInput() const
{
    const size_t start = m_inputBuffer.find_first_not_of(' ');
    if (start == std::string::npos)
    {
        return {};
    }
    const size_t end = m_inputBuffer.find_last_not_of(' ');
    return m_inputBuffer.substr(start, end - start + 1);
}

// The first space-separated word of the input.
std::string InputHandler::commandWord() const
{
    const size_t start = m_inputBuffer.find_first_not_of(' ');
    if (start == std::string::npos)
    {
        return {};
    }
    const size_t end = m_inputBuffer.find(' ', start);
    return m_inputBuffer.substr(start, end == std::string::npos ? std::string::npos : end - start);
}

// Everything after the command word.
std::string InputHandler::argumentText() const
{
    const size_t start = m_inputBuffer.find_first_not_of(' ');
    if (start == std::string::npos)
    {
        return {};
    }
    const size_t end = m_inputBuffer.find(' ', start);
    return end == std::string::npos ? std::string() : m_inputBuffer.substr(end + 1);
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