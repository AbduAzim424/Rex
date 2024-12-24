/*
 * Copyright (c) 2024, shAdE424
 * All rights reserved.
 *
 * This file is part of Rex, licensed under the BSD 3-Clause License.
 * See the LICENSE file at the root of this repository for full details.
 */

#pragma once

#include "types.hpp"

#define M_PI 3.14159265358979323846
#define M_PI_2 1.57079632679489661923

class UI final
{
public:
    UI() : m_connection(nullptr), m_screen(nullptr), m_font("Roboto 12"), 
        m_bgColor(0xFFFFFF), m_textColor(0x000000), m_highlightColor(0xFFAA00), 
        m_draw_searbar_count(0), m_draw_suggestions_count(0)
    {
    }

    ~UI()
    {
        g_object_unref(m_pangoLayout);
        cairo_destroy(m_cairoContext);
        cairo_surface_destroy(m_cairoSurface);
        xcb_destroy_window(m_connection, m_window_id);
    }

    void init(xcb_connection_t* connection, xcb_window_t window_id);

    void drawUI(const std::string& query, const std::vector<std::string>& suggestions, size_t highlightedIndex);
    void updateUI(std::string_view typedText, std::vector<std::string> suggestions, ssize_t highlightedIndex);
    void clearUI();

    void setFont(const std::string& fontDescription);
    void setSourceColor(cairo_t* cr, uint32_t color);
private:
    void drawSearchBar(const std::string& query);
    void drawSuggestions(const std::vector<std::string>& suggestions, size_t highlightedIndex);
    void drawText(cairo_t* cr, int x, int y, const std::string& text, bool highlighted);

    xcb_visualtype_t* getVisualType(xcb_screen_t* screen);

    void createWindow();
private:
    xcb_connection_t* m_connection;
    xcb_screen_t*     m_screen;
    xcb_window_t      m_window_id;

    uint16_t          m_window_width;
    uint16_t          m_window_height;
    uint16_t          m_x;
    uint16_t          m_y;

    std::string m_font;
    uint32_t m_bgColor;
    uint32_t m_textColor;
    uint32_t m_highlightColor;
    
    cairo_surface_t* m_cairoSurface;
    cairo_t* m_cairoContext;

    PangoLayout* m_pangoLayout;

    int m_draw_searbar_count;
    int m_draw_suggestions_count;
};