/*
 * (C) Copyright 2014 Weng Xuetian <wengxt@gmail.com>
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the GNU Lesser General Public License
 * (LGPL) version 2.1 which accompanies this distribution, and is available at
 * http://www.gnu.org/licenses/lgpl-2.1.html
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 */
#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include "imclient.h"
#include "ximproto.h"

xcb_connection_t *connection;
xcb_window_t w;
xcb_screen_t* screen;
bool end = false;

void destroy_ic_callback(xcb_xim_t* im, xcb_xic_t ic, void* user_data)
{
    fprintf(stderr, "ic %d destroyed\n", ic);
    // end = true;
}

void get_ic_values_callback(xcb_xim_t* im, xcb_xic_t ic, xcb_im_get_ic_values_reply_fr_t* reply, void* user_data)
{
    fprintf(stderr, "get ic %d done\n", ic);
    xcb_xim_destroy_ic(im, ic, destroy_ic_callback, NULL);
}

void set_ic_values_callback(xcb_xim_t* im, xcb_xic_t ic, void* user_data)
{
    fprintf(stderr, "set ic %d done\n", ic);
    xcb_xim_get_ic_values(im, ic, get_ic_values_callback, NULL,
                          XNPreeditAttributes,
                          NULL);
}

void create_ic_callback(xcb_xim_t* im, xcb_xic_t ic, void* user_data)
{
    if (ic) {
        fprintf(stderr, "icid:%u\n", ic);
        xcb_point_t spot;
        spot.x = 0;
        spot.y = 0;
        xcb_xim_nested_list nested = xcb_xim_create_nested_list(
            im,
            XNSpotLocation, &spot, NULL
        );
        xcb_xim_set_ic_values(im, ic, set_ic_values_callback, NULL,
                              XNPreeditAttributes, &nested,
                              NULL);
        free(nested.data);
    } else {
        fprintf(stderr, "failed.\n");
    }
}

void get_im_values_callback(xcb_xim_t* im, xcb_im_get_im_values_reply_fr_t* reply, void* user_data)
{
    w = xcb_generate_id (connection);
    xcb_create_window (connection, XCB_COPY_FROM_PARENT, w, screen->root,
                       0, 0, 1, 1, 1,
                       XCB_WINDOW_CLASS_INPUT_OUTPUT,
                       screen->root_visual,
                       0, NULL);
    uint32_t input_style = XCB_IM_PreeditPosition | XCB_IM_StatusArea;
    xcb_point_t spot;
    spot.x = 0;
    spot.y = 0;
    xcb_xim_nested_list nested = xcb_xim_create_nested_list(
        im,
        XNSpotLocation, &spot, NULL
    );
    xcb_xim_create_ic(im,
                      create_ic_callback, NULL,
                      XNInputStyle, &input_style,
                      XNClientWindow, &w,
                      XNFocusWindow, &w,
                      XNPreeditAttributes, &nested,
                      NULL
                     );
    free(nested.data);
}

void open_callback(xcb_xim_t* im, void* user_data)
{
    xcb_xim_get_im_values(im, get_im_values_callback, NULL, XNQueryInputStyle, NULL);
}

int main(int argc, char* argv[])
{
    /* Open the connection to the X server */

    int screen_default_nbr;
    connection = xcb_connect (NULL, &screen_default_nbr);
    screen = xcb_aux_get_screen(connection, screen_default_nbr);

    if (!screen) {
        return false;
    }
    xcb_xim_t* im = xcb_xim_create(connection, screen_default_nbr, "@im=test_server");
    assert(xcb_xim_open(im, open_callback, true, NULL));

    xcb_generic_event_t *event;
    while ( (event = xcb_wait_for_event (connection)) ) {
        xcb_xim_filter_event(im, event);
        free(event);
        if (end) {
            break;
        }
    }

    xcb_xim_close(im);
    xcb_xim_destroy(im);

    xcb_disconnect(connection);

    return 0;
}