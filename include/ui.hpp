/*
 * Copyright (c) 2024, shAdE424
 * All rights reserved.
 *
 * This file is part of Rex, licensed under the BSD 3-Clause License.
 * See the LICENSE file at the root of this repository for full details.
 */

#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include <cairo/cairo.h>
#include <pango/pangocairo.h>
#include <xcb/xcb.h>

/*
 * The launcher window: a borderless, always-on-top popup drawn with Cairo,
 * showing the search query and a list of suggestions.
 */
class UI final
{
public:
    UI() : m_connection(nullptr), m_screen(nullptr), m_window_id(XCB_NONE),
        m_window_width(0), m_window_height(0), m_x(0), m_y(0), m_font("Roboto 12"),
        m_bgColor(0xFFFFFF), m_textColor(0x000000), m_highlightColor(0xFFAA00), m_boxColor(0x80B3FF),
        m_cairoSurface(nullptr), m_cairoContext(nullptr), m_pangoLayout(nullptr)
    {
    }

    ~UI()
    {
        if (m_pangoLayout)
        {
            g_object_unref(m_pangoLayout);
        }
        if (m_cairoContext)
        {
            cairo_destroy(m_cairoContext);
        }
        if (m_cairoSurface)
        {
            cairo_surface_destroy(m_cairoSurface);
        }
        if (m_window_id != XCB_NONE)
        {
            xcb_destroy_window(m_connection, m_window_id);
        }
    }

    // Creates and maps the window and sets up the Cairo/Pango drawing state.
    // Throws std::runtime_error on failure.
    void init(xcb_connection_t* connection);

    // Redraws the whole window. highlightedIndex is the selected suggestion.
    void render(std::string_view query, const std::vector<std::string>& suggestions, size_t highlightedIndex);

private:
    void createWindow();
    xcb_visualtype_t* getVisualType() const;

    void clear();
    void drawSearchBar(std::string_view query);
    void drawSuggestions(const std::vector<std::string>& suggestions, size_t highlightedIndex);
    void drawText(int x, int y, const std::string& text, bool highlighted);
    void drawRoundedRect(double x, double y, double width, double height, double radius);

    void setFont(const std::string& fontDescription);
    void setSourceColor(uint32_t color, double alpha = 1.0);

    xcb_connection_t* m_connection;
    xcb_screen_t*     m_screen;
    xcb_window_t      m_window_id;

    uint16_t          m_window_width;
    uint16_t          m_window_height;
    uint16_t          m_x;
    uint16_t          m_y;

    std::string       m_font;
    uint32_t          m_bgColor;
    uint32_t          m_textColor;
    uint32_t          m_highlightColor;
    uint32_t          m_boxColor;

    cairo_surface_t*  m_cairoSurface;
    cairo_t*          m_cairoContext;
    PangoLayout*      m_pangoLayout;
};
