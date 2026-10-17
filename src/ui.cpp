/*
 * Copyright (c) 2024, shAdE424
 * All rights reserved.
 *
 * This file is part of Rex, licensed under the BSD 3-Clause License.
 * See the LICENSE file at the root of this repository for full details.
 */

#include "ui.hpp"

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

#include <cairo/cairo-xcb.h>

namespace
{
    // Window size as a fraction of the screen, and horizontal placement.
    constexpr double kWidthRatio = 0.17;
    constexpr double kHeightRatio = 0.3;
    constexpr double kHorizontalOffsetDivisor = 4.3;
}

void UI::init(xcb_connection_t* connection)
{
    if (xcb_connection_has_error(connection))
    {
        exit(-1);
    }
    m_connection = connection;
    m_screen = xcb_setup_roots_iterator(xcb_get_setup(m_connection)).data;
    m_window_id = xcb_generate_id(m_connection);

    m_window_width = m_screen->width_in_pixels * kWidthRatio;
    m_window_height = m_screen->height_in_pixels * kHeightRatio;

    m_x = (m_screen->width_in_pixels - m_window_width) / kHorizontalOffsetDivisor;
    m_y = (m_screen->height_in_pixels - m_window_height) / 2;

    createWindow();

    xcb_visualtype_t* visual = getVisualType();
    if (!visual)
    {
        throw std::runtime_error("Failed to get XCB visual type.");
    }

    m_cairoSurface = cairo_xcb_surface_create(m_connection, m_window_id, visual, m_window_width, m_window_height);
    if (!m_cairoSurface)
    {
        throw std::runtime_error("Failed to create Cairo surface.");
    }

    m_cairoContext = cairo_create(m_cairoSurface);
    if (!m_cairoContext)
    {
        throw std::runtime_error("Failed to create Cairo context.");
    }

    m_pangoLayout = pango_cairo_create_layout(m_cairoContext);
    if (!m_pangoLayout)
    {
        throw std::runtime_error("Failed to create Pango layout.");
    }
    setFont(m_font);
}

void UI::createWindow()
{
    uint32_t mask = XCB_CW_BORDER_PIXEL | XCB_CW_EVENT_MASK | XCB_CW_COLORMAP;
    uint32_t values[] =
    {
        0, // Border pixel (no border)
        XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_KEY_PRESS,
        m_screen->default_colormap
    };

    xcb_create_window(
        m_connection,
        XCB_COPY_FROM_PARENT,
        m_window_id, m_screen->root,
        m_x, m_y,
        m_window_width, m_window_height,
        0, // Border width
        XCB_WINDOW_CLASS_INPUT_OUTPUT,
        m_screen->root_visual,
        mask, values
    );

    // Looks up an X atom by name, returning XCB_ATOM_NONE if unavailable.
    auto intern_atom = [this](const char* atom_name)
    {
        xcb_intern_atom_cookie_t cookie = xcb_intern_atom(m_connection, 0, strlen(atom_name), atom_name);
        xcb_intern_atom_reply_t* reply = xcb_intern_atom_reply(m_connection, cookie, nullptr);
        if (reply)
        {
            xcb_atom_t atom = reply->atom;
            free(reply);
            return atom;
        }
        return static_cast<xcb_atom_t>(XCB_ATOM_NONE);
    };

    xcb_atom_t net_wm_window_type = intern_atom("_NET_WM_WINDOW_TYPE");
    xcb_atom_t net_wm_window_type_utility = intern_atom("_NET_WM_WINDOW_TYPE_UTILITY");
    xcb_atom_t net_active_window = intern_atom("_NET_ACTIVE_WINDOW");
    xcb_atom_t net_wm_state = intern_atom("_NET_WM_STATE");
    xcb_atom_t net_wm_state_skip_taskbar = intern_atom("_NET_WM_STATE_SKIP_TASKBAR");
    xcb_atom_t net_wm_state_skip_pager = intern_atom("_NET_WM_STATE_SKIP_PAGER");
    xcb_atom_t net_wm_state_above = intern_atom("_NET_WM_STATE_ABOVE");
    xcb_atom_t net_wm_window_opacity = intern_atom("_NET_WM_WINDOW_OPACITY");
    xcb_atom_t motif_hints = intern_atom("_MOTIF_WM_HINTS");

    // EWMH hints: mark the window as a utility popup so window managers keep
    // it out of the taskbar/pager and above other windows.
    if (net_wm_window_type != XCB_ATOM_NONE && net_wm_window_type_utility != XCB_ATOM_NONE)
    {
        xcb_change_property(m_connection, XCB_PROP_MODE_REPLACE, m_window_id, net_wm_window_type, XCB_ATOM_ATOM, 32, 1, &net_wm_window_type_utility);
    }

    // Set window states to skip taskbar and pager
    if (net_wm_state != XCB_ATOM_NONE && net_wm_state_skip_taskbar != XCB_ATOM_NONE && net_wm_state_skip_pager != XCB_ATOM_NONE)
    {
        xcb_atom_t states[] = {net_wm_state_skip_taskbar, net_wm_state_skip_pager};
        xcb_change_property(m_connection, XCB_PROP_MODE_APPEND, m_window_id, net_wm_state, XCB_ATOM_ATOM, 32, 2, states);
    }

    // Set window state to always be above other windows
    if (net_wm_state != XCB_ATOM_NONE && net_wm_state_above != XCB_ATOM_NONE)
    {
        xcb_atom_t states[] = {net_wm_state_above};
        xcb_change_property(m_connection, XCB_PROP_MODE_APPEND, m_window_id, net_wm_state, XCB_ATOM_ATOM, 32, 1, states);
    }

    // _NET_WM_WINDOW_OPACITY is a 32-bit fraction of 0xFFFFFFFF; 0xF2... is ~95%.
    // Only takes effect with a compositor.
    uint32_t opacity_value = 0xF2FFFFFF;
    if (net_wm_window_opacity != XCB_ATOM_NONE)
    {
        xcb_change_property(m_connection, XCB_PROP_MODE_REPLACE, m_window_id, net_wm_window_opacity, XCB_ATOM_CARDINAL, 32, 1, &opacity_value);
    }

    // Legacy Motif hints are still the most widely supported way to ask the
    // window manager for no title bar or borders (flags=2: decorations field set).
    struct MotifHints
    {
        uint32_t flags;
        uint32_t functions;
        uint32_t decorations;
        int32_t input_mode;
        uint32_t status;
    };

    MotifHints hints = {2, 0, 0, 0, 0};
    if (motif_hints != XCB_ATOM_NONE)
    {
        xcb_change_property(m_connection, XCB_PROP_MODE_REPLACE, m_window_id, motif_hints, motif_hints, 32, sizeof(MotifHints) / 4, &hints);
    }

    xcb_map_window(m_connection, m_window_id);

    // Ask the window manager to activate the window, then take keyboard focus.
    if (net_active_window != XCB_ATOM_NONE)
    {
        xcb_client_message_event_t event = {};
        event.response_type = XCB_CLIENT_MESSAGE;
        event.window = m_window_id;
        event.type = net_active_window;
        event.format = 32;
        event.data.data32[0] = 1; // Source indication (1 = application)
        event.data.data32[1] = XCB_CURRENT_TIME;
        event.data.data32[2] = m_window_id;

        xcb_send_event(
            m_connection,
            false,
            m_screen->root,
            XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY | XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT,
            reinterpret_cast<const char*>(&event)
        );
    }

    xcb_set_input_focus(
        m_connection,
        XCB_INPUT_FOCUS_POINTER_ROOT,
        m_window_id,
        XCB_CURRENT_TIME
    );

    xcb_flush(m_connection);
}

void UI::render(std::string_view query, const std::vector<std::string>& suggestions, size_t highlightedIndex)
{
    clear();
    drawSearchBar(query);
    drawSuggestions(suggestions, highlightedIndex);

    cairo_surface_flush(m_cairoSurface);
    xcb_flush(m_connection);
}

void UI::clear()
{
    setSourceColor(m_bgColor);
    cairo_paint(m_cairoContext);
}

void UI::drawSearchBar(std::string_view query)
{
    const int padding = 5;
    const int height = 30;
    const int right = 55;
    const int cornerRadius = 5;
    const double alpha = 0.5;

    setSourceColor(m_boxColor, alpha);
    drawRoundedRect(padding, 0, right - padding, height, cornerRadius);
    cairo_fill(m_cairoContext);

    drawText(5, 3, "Search: " + std::string(query), false);

    // Underline separating the query from the suggestions
    cairo_set_source_rgb(m_cairoContext, 0, 0, 0);
    cairo_set_line_width(m_cairoContext, 2);
    cairo_move_to(m_cairoContext, 5, 30);
    cairo_line_to(m_cairoContext, m_window_width - 5, 25);
    cairo_stroke(m_cairoContext);
}

void UI::drawSuggestions(const std::vector<std::string>& suggestions, size_t highlightedIndex)
{
    const int padding = 10;
    const int height = 40;
    const int right = m_window_width - 1.2 * padding;
    const int cornerRadius = 6;
    const int verticalSpacing = 5;
    const double alpha = 0.3;

    int y = 40;
    for (size_t i = 0; i < suggestions.size(); ++i)
    {
        setSourceColor(m_boxColor, alpha);
        drawRoundedRect(padding, y, right - padding, height, cornerRadius);
        cairo_fill(m_cairoContext);

        drawText(padding + cornerRadius, y + padding, suggestions[i], highlightedIndex == i);

        y += height + verticalSpacing;
    }
}

void UI::drawText(int x, int y, const std::string& text, bool highlighted)
{
    setSourceColor(highlighted ? m_highlightColor : m_textColor);
    cairo_move_to(m_cairoContext, x, y);
    pango_layout_set_text(m_pangoLayout, text.c_str(), -1);
    pango_cairo_update_layout(m_cairoContext, m_pangoLayout);
    pango_cairo_show_layout(m_cairoContext, m_pangoLayout);
}

// Builds a rounded-rectangle path; the caller fills or strokes it.
void UI::drawRoundedRect(double x, double y, double width, double height, double radius)
{
    cairo_new_path(m_cairoContext);
    cairo_arc(m_cairoContext, x + width - radius, y + radius,          radius, -M_PI_2, 0);
    cairo_arc(m_cairoContext, x + width - radius, y + height - radius, radius, 0, M_PI_2);
    cairo_arc(m_cairoContext, x + radius,         y + height - radius, radius, M_PI_2, M_PI);
    cairo_arc(m_cairoContext, x + radius,         y + radius,          radius, M_PI, 3 * M_PI_2);
    cairo_close_path(m_cairoContext);
}

void UI::setFont(const std::string& fontDescription)
{
    m_font = fontDescription;
    PangoFontDescription* font_desc = pango_font_description_from_string(m_font.c_str());
    pango_layout_set_font_description(m_pangoLayout, font_desc);
    pango_font_description_free(font_desc);
}

void UI::setSourceColor(uint32_t color, double alpha)
{
    cairo_set_source_rgba(m_cairoContext,
        ((color >> 16) & 0xFF) / 255.0,
        ((color >> 8) & 0xFF) / 255.0,
        (color & 0xFF) / 255.0,
        alpha);
}

// Finds the visual type of the root window, which Cairo needs to create a surface.
xcb_visualtype_t* UI::getVisualType() const
{
    xcb_depth_iterator_t depth_iter = xcb_screen_allowed_depths_iterator(m_screen);

    for (; depth_iter.rem; xcb_depth_next(&depth_iter))
    {
        xcb_visualtype_iterator_t visual_iter = xcb_depth_visuals_iterator(depth_iter.data);
        for (; visual_iter.rem; xcb_visualtype_next(&visual_iter))
        {
            if (m_screen->root_visual == visual_iter.data->visual_id)
            {
                return visual_iter.data;
            }
        }
    }
    return nullptr;
}
