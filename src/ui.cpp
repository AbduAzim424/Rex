/*
 * Copyright (c) 2024, shAdE424
 * All rights reserved.
 *
 * This file is part of Rex, licensed under the BSD 3-Clause License.
 * See the LICENSE file at the root of this repository for full details.
 */

#include "../include/ui.hpp"

void UI::init(xcb_connection_t* connection, xcb_window_t window_id)
{
    if (xcb_connection_has_error(connection))
    {
        exit(-1);
    }   
    m_connection = connection;

    m_screen = xcb_setup_roots_iterator(xcb_get_setup(m_connection)).data;

    m_window_id = window_id;

    m_window_width = m_screen->width_in_pixels * 0.17;
    m_window_height = m_screen->height_in_pixels * 0.3;

    m_x = (m_screen->width_in_pixels - m_window_width) / 4.3;
    m_y = (m_screen->height_in_pixels - m_window_height) / 2;

    createWindow();

    xcb_visualtype_t* visual = getVisualType(m_screen);
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
        XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_BUTTON_PRESS |
        XCB_EVENT_MASK_FOCUS_CHANGE | XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_LEAVE_WINDOW | XCB_STACK_MODE_ABOVE,
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

    // Set window type to utility
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

    // Set opacity to 95%
    uint32_t opacity_value = 0xF2FFFFFF;
    if (net_wm_window_opacity != XCB_ATOM_NONE) 
    {
        xcb_change_property(m_connection, XCB_PROP_MODE_REPLACE, m_window_id, net_wm_window_opacity, XCB_ATOM_CARDINAL, 32, 1, &opacity_value);
    }

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

void UI::drawUI(const std::string& query, const std::vector<std::string>& suggestions, size_t highlightedIndex)
{
    clearUI();
    drawSearchBar(query);
    drawSuggestions(suggestions, highlightedIndex);
    cairo_surface_flush(m_cairoSurface);
    xcb_flush(m_connection);
}

void UI::drawSearchBar(const std::string& query)
{
    const int padding = 5;  // Padding inside the rectangle
    const int rectHeight = 30;  // Height of the rectangle
    const int rectWidth = 10 + 45; // Adjust width based on the query length (a rough estimate)
    const int cornerRadius = 5;  // Rounded corners radius
    const float alpha = 0.5f;  // Transparency for the rectangle

    cairo_set_source_rgba(m_cairoContext, 0.5, 0.7, 1.0, alpha); // Light blue with transparency

    cairo_new_path(m_cairoContext);
    cairo_move_to(m_cairoContext, padding + cornerRadius, 0); 

    cairo_line_to(m_cairoContext, rectWidth - cornerRadius * 2, 0);  // Top line
    cairo_arc(m_cairoContext, rectWidth - cornerRadius, 0 + cornerRadius, cornerRadius, -M_PI_2, 0);  // Top-right corner

    cairo_line_to(m_cairoContext, rectWidth, 0 + rectHeight - cornerRadius);  // Right side
    cairo_arc(m_cairoContext, rectWidth - cornerRadius, 0 + rectHeight - cornerRadius, cornerRadius, 0, M_PI_2);  // Bottom-right corner

    cairo_line_to(m_cairoContext, padding + cornerRadius, 0 + rectHeight);  // Bottom side
    cairo_arc(m_cairoContext, padding + cornerRadius, 0 + rectHeight - cornerRadius, cornerRadius, M_PI_2, M_PI);  // Bottom-left corner

    cairo_line_to(m_cairoContext, padding, 0 + cornerRadius);  // Left side
    cairo_arc(m_cairoContext, padding + cornerRadius, 0 + cornerRadius, cornerRadius, M_PI, -M_PI_2);  // Top-left corner

    cairo_close_path(m_cairoContext);
    cairo_fill(m_cairoContext); 

    drawText(m_cairoContext, 5, 3, "Search: " + query, false);

    // Draw black line under the search prompt
    cairo_set_source_rgb(m_cairoContext, 0, 0, 0);
    cairo_set_line_width(m_cairoContext, 2);
    cairo_move_to(m_cairoContext, 5, 30); 
    cairo_line_to(m_cairoContext, m_window_width - 5, 25);  
    cairo_stroke(m_cairoContext);
}

void UI::drawSuggestions(const std::vector<std::string>& suggestions, size_t highlightedIndex)
{
    ++m_draw_suggestions_count;
    int y = 40;
    const int padding = 10;  // Padding around text inside the rectangle
    const int rectHeight = 40; // Height for each rectangle
    const int rectWidth = m_window_width - 1.2 * padding; // Width for each rectangle with padding
    const float alpha = 0.3f; // Transparency level for the rectangle (range 0.0 to 1.0)
    const int cornerRadius = 6; // Radius for rounded corners
    const int verticalSpacing = 5; // Vertical spacing between rectangles

    for (size_t i = 0; i < suggestions.size(); ++i) 
    {
        cairo_set_source_rgba(m_cairoContext, 0.5, 0.7, 1.0, alpha); // Light blue with higher transparency
        
        // Set rounded corners for the rectangle
        cairo_new_path(m_cairoContext);  // Start a new path for the rectangle
        cairo_move_to(m_cairoContext, padding + cornerRadius, y);  // Move to the start position, adjusted for corner radius

        // Draw the top line
        cairo_line_to(m_cairoContext, rectWidth - cornerRadius * 2, y);  // Top line
        cairo_arc(m_cairoContext, rectWidth - cornerRadius, y + cornerRadius, cornerRadius, -M_PI_2, 0);  // Top-right corner

        // Draw the right line
        cairo_line_to(m_cairoContext, rectWidth, y + rectHeight - cornerRadius);  // Right side
        cairo_arc(m_cairoContext, rectWidth - cornerRadius, y + rectHeight - cornerRadius, cornerRadius, 0, M_PI_2);  // Bottom-right corner

        // Draw the bottom line
        cairo_line_to(m_cairoContext, padding + cornerRadius, y + rectHeight);  // Bottom side
        cairo_arc(m_cairoContext, padding + cornerRadius, y + rectHeight - cornerRadius, cornerRadius, M_PI_2, M_PI);  // Bottom-left corner

        // Draw the left line
        cairo_line_to(m_cairoContext, padding, y + cornerRadius);  // Left side
        cairo_arc(m_cairoContext, padding + cornerRadius, y + cornerRadius, cornerRadius, M_PI, -M_PI_2);  // Top-left corner

        cairo_close_path(m_cairoContext); 
        cairo_fill(m_cairoContext); 

        if (highlightedIndex == i) 
        {
            cairo_set_source_rgb(m_cairoContext, (m_highlightColor >> 16 & 0xFF) / 255.0, 
                                 (m_highlightColor >> 8 & 0xFF) / 255.0, 
                                 (m_highlightColor & 0xFF) / 255.0);  // Highlight color
        } 
        else 
        {
            cairo_set_source_rgb(m_cairoContext, (m_textColor >> 16 & 0xFF) / 255.0, 
                                 (m_textColor >> 8 & 0xFF) / 255.0, 
                                 (m_textColor & 0xFF) / 255.0);  // Regular text color
        }

        // Draw the suggestion text on top of the rectangle
        drawText(m_cairoContext, padding + cornerRadius, y + padding, suggestions[i], highlightedIndex == i);
        
        y += rectHeight + verticalSpacing;  // Move to the next suggestion (with vertical spacing)
    }
}

void UI::setFont(const std::string& fontDescription)
{
    m_font = fontDescription;
    PangoFontDescription* font_desc = pango_font_description_from_string(m_font.c_str());
    pango_layout_set_font_description(m_pangoLayout, font_desc);
    pango_font_description_free(font_desc);
}

void UI::setSourceColor(cairo_t* cr, uint32_t color) 
{
    cairo_set_source_rgb(cr, 
        (color >> 16 & 0xFF) / 255.0, 
        (color >> 8 & 0xFF) / 255.0, 
        (color & 0xFF) / 255.0
    );
}

void UI::drawText(cairo_t* cr, int x, int y, const std::string& text, bool highlighted)
{
    if (highlighted) 
    {
        cairo_set_source_rgb(cr, (m_highlightColor >> 16 & 0xFF) / 255.0, 
                             (m_highlightColor >> 8 & 0xFF) / 255.0, 
                             (m_highlightColor & 0xFF) / 255.0);
    } 
    else 
    {
        cairo_set_source_rgb(cr, (m_textColor >> 16 & 0xFF) / 255.0, 
                             (m_textColor >> 8 & 0xFF) / 255.0, 
                             (m_textColor & 0xFF) / 255.0);
    }
    cairo_move_to(cr, x, y);
    pango_layout_set_text(m_pangoLayout, text.c_str(), -1);
    pango_cairo_update_layout(cr, m_pangoLayout);
    pango_cairo_show_layout(cr, m_pangoLayout);
}

xcb_visualtype_t* UI::getVisualType(xcb_screen_t* screen) 
{
    xcb_depth_iterator_t depth_iter = xcb_screen_allowed_depths_iterator(screen);
    
    for (; depth_iter.rem; xcb_depth_next(&depth_iter)) 
    {
        xcb_visualtype_iterator_t visual_iter = xcb_depth_visuals_iterator(depth_iter.data);
        for (; visual_iter.rem; xcb_visualtype_next(&visual_iter)) 
        {
            if (screen->root_visual == visual_iter.data->visual_id) 
            {
                return visual_iter.data;
            }
        }
    }
    return nullptr;
}

void UI::updateUI(std::string_view typedText, std::vector<std::string> suggestions, ssize_t highlightedIndex)
{
    clearUI();
    drawSearchBar(std::string(typedText));

    drawSuggestions(suggestions, highlightedIndex);
    
    cairo_surface_flush(m_cairoSurface);
    xcb_flush(m_connection);
}

void UI::clearUI()
{
    cairo_set_source_rgb(m_cairoContext, (m_bgColor >> 16 & 0xFF) / 255.0, 
                         (m_bgColor >> 8 & 0xFF) / 255.0, 
                         (m_bgColor & 0xFF) / 255.0);
    cairo_paint(m_cairoContext);
}