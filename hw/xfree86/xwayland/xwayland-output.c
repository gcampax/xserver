/*
 * Copyright © 2011 Intel Corporation
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of the
 * copyright holders not be used in advertising or publicity
 * pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied
 * warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */

#ifdef HAVE_XORG_CONFIG_H
#include "xorg-config.h"
#endif

#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>
#include <wayland-client.h>

#include <xorg-server.h>
#include <cursorstr.h>
#include <xf86Crtc.h>
#include <mipointrst.h>
#include <randrstr.h>

#include "xwayland.h"
#include "xwayland-private.h"
#include "xserver-client-protocol.h"

static void
crtc_dpms(xf86CrtcPtr drmmode_crtc, int mode)
{
}

static Bool
crtc_set_mode_major(xf86CrtcPtr crtc, DisplayModePtr mode,
		    Rotation rotation, int x, int y)
{
	return TRUE;
}

static void
crtc_set_cursor_colors (xf86CrtcPtr crtc, int bg, int fg)
{
}

static void
crtc_set_cursor_position (xf86CrtcPtr crtc, int x, int y)
{
}

static void
crtc_show_cursor (xf86CrtcPtr crtc)
{
}

static void
crtc_hide_cursor (xf86CrtcPtr crtc)
{
}

static void
crtc_load_cursor_argb (xf86CrtcPtr crtc, CARD32 *image)
{
}

static PixmapPtr
crtc_shadow_create(xf86CrtcPtr crtc, void *data, int width, int height)
{
	return NULL;
}

static void *
crtc_shadow_allocate(xf86CrtcPtr crtc, int width, int height)
{
	return NULL;
}

static void
crtc_shadow_destroy(xf86CrtcPtr crtc, PixmapPtr rotate_pixmap, void *data)
{
}

static void
crtc_destroy(xf86CrtcPtr crtc)
{
    /* Nothing to do here, we only destroy CRTCs when instructed to do
       so by wl_output changes
    */
}

static const xf86CrtcFuncsRec crtc_funcs = {
    .dpms                = crtc_dpms,
    .set_mode_major      = crtc_set_mode_major,
    .set_cursor_colors   = crtc_set_cursor_colors,
    .set_cursor_position = crtc_set_cursor_position,
    .show_cursor         = crtc_show_cursor,
    .hide_cursor         = crtc_hide_cursor,
    .load_cursor_argb    = crtc_load_cursor_argb,
    .shadow_create       = crtc_shadow_create,
    .shadow_allocate     = crtc_shadow_allocate,
    .shadow_destroy      = crtc_shadow_destroy,
    .destroy		 = crtc_destroy,
};

static void
output_dpms(xf86OutputPtr output, int mode)
{
	return;
}

static xf86OutputStatus
output_detect(xf86OutputPtr output)
{
	return XF86OutputStatusConnected;
}

static Bool
output_mode_valid(xf86OutputPtr output, DisplayModePtr pModes)
{
	return MODE_OK;
}

static DisplayModePtr
output_get_modes(xf86OutputPtr xf86output)
{
    struct xwl_output *output = xf86output->driver_private;
    struct monitor_ranges *ranges;
    DisplayModePtr modes;

    modes = xf86CVTMode(output->width, output->height, 60, TRUE, FALSE);
    output->xf86monitor.det_mon[0].type = DS_RANGES;
    ranges = &output->xf86monitor.det_mon[0].section.ranges;
    ranges->min_h = modes->HSync - 10;
    ranges->max_h = modes->HSync + 10;
    ranges->min_v = modes->VRefresh - 10;
    ranges->max_v = modes->VRefresh + 10;
    ranges->max_clock = modes->Clock + 100;
    output->xf86monitor.det_mon[1].type = DT;
    output->xf86monitor.det_mon[2].type = DT;
    output->xf86monitor.det_mon[3].type = DT;
    output->xf86monitor.no_sections = 0;

    xf86output->MonInfo = &output->xf86monitor;

    return modes;
}

static void
output_destroy(xf86OutputPtr xf86output)
{
    struct xwl_output *output = xf86output->driver_private;

    free(output);
}

static const xf86OutputFuncsRec output_funcs = {
    .dpms	= output_dpms,
    .detect	= output_detect,
    .mode_valid	= output_mode_valid,
    .get_modes	= output_get_modes,
    .destroy	= output_destroy
};

struct xwl_output *
xwl_output_create(struct xwl_screen *xwl_screen)
{
    struct xwl_output *xwl_output;
    xf86OutputPtr xf86output;
    xf86CrtcPtr xf86crtc;

    xwl_output = calloc(sizeof *xwl_output, 1);
    if (xwl_output == NULL) {
	ErrorF("create_output ENOMEM");
	return NULL;
    }

    xwl_output->xwl_screen = xwl_screen;

    xf86output = xf86OutputCreate(xwl_screen->scrninfo,
				  &output_funcs, "XWAYLAND-1");
    xf86output->driver_private = xwl_output;
    xf86output->possible_crtcs = 1;
    xf86output->possible_clones = 1;

    xf86crtc = xf86CrtcCreate(xwl_screen->scrninfo, &crtc_funcs);
    xf86crtc->enabled = TRUE;
    xf86crtc->driver_private = xwl_output;

    xwl_output->xf86output = xf86output;
    xwl_output->xf86crtc = xf86crtc;

    return xwl_output;
}

static Bool
resize(ScrnInfoPtr scrn, int width, int height)
{
    if (scrn->virtualX == width && scrn->virtualY == height)
	return TRUE;
    /* We don't handle resize at all, we must match the compositor size */
    return FALSE;
}

static const xf86CrtcConfigFuncsRec config_funcs = {
    resize
};

static void
display_handle_geometry(void *data, struct wl_output *wl_output, int x, int y,
			int physical_width, int physical_height, int subpixel,
			const char *make, const char *model, int transform)
{
    struct xwl_output *xwl_output = data;
    struct xwl_screen *xwl_screen = xwl_output->xwl_screen;

    xwl_output->xf86output->mm_width = physical_width;
    xwl_output->xf86output->mm_height = physical_height;

    switch (subpixel) {
    case WL_OUTPUT_SUBPIXEL_UNKNOWN:
	xwl_output->xf86output->subpixel_order = SubPixelUnknown;
	break;
    case WL_OUTPUT_SUBPIXEL_NONE:
	xwl_output->xf86output->subpixel_order = SubPixelNone;
	break;
    case WL_OUTPUT_SUBPIXEL_HORIZONTAL_RGB:
	xwl_output->xf86output->subpixel_order = SubPixelHorizontalRGB;
	break;
    case WL_OUTPUT_SUBPIXEL_HORIZONTAL_BGR:
	xwl_output->xf86output->subpixel_order = SubPixelHorizontalBGR;
	break;
    case WL_OUTPUT_SUBPIXEL_VERTICAL_RGB:
	xwl_output->xf86output->subpixel_order = SubPixelVerticalRGB;
	break;
    case WL_OUTPUT_SUBPIXEL_VERTICAL_BGR:
	xwl_output->xf86output->subpixel_order = SubPixelVerticalBGR;
	break;
    }

    xwl_output->x = x;
    xwl_output->y = y;

    xorg_list_append (&xwl_output->link, &xwl_screen->output_list);
}

static void
display_handle_mode(void *data, struct wl_output *wl_output, uint32_t flags,
		    int width, int height, int refresh)
{
    struct xwl_output *xwl_output = data;
    struct xwl_screen *xwl_screen = xwl_output->xwl_screen;
    ScreenPtr pScreen = xwl_screen->screen;
    ScrnInfoPtr scrn = xwl_screen->scrninfo;
    CARD16 width_mm, height_mm;
    DisplayModePtr mode;
    rrScrPrivPtr rp;

    if (!(flags & WL_OUTPUT_MODE_CURRENT))
	return;

    xwl_output->width = width;
    xwl_output->height = height;

    if (xwl_output->x + xwl_output->width > scrn->virtualX ||
	xwl_output->y + xwl_output->height > scrn->virtualY) {
	/* Fake a RandR request to resize the screen. It will bounce
	   back to our crtc_resize, which does nothing.
	*/
	/* Preupdate virtualX / virtualY, so that crtc_resize returns TRUE */
	scrn->virtualX = xwl_output->x + xwl_output->width;
	scrn->virtualY = xwl_output->y + xwl_output->height;

	/* Ignore the compositor provided values for mm_width/mm_height,
	   as it doesn't make sense to sum the values of different outputs.
	   Just make the DPI 96 */
	width_mm = (scrn->virtualX / 96.0) * 25.4 + 0.5;
	height_mm = (scrn->virtualY / 96.0) * 25.4 + 0.5;

	/* But! When the server starts, the RandR stuff is not initialized,
	   so we can't call rrGetScrPriv. We updated virtualX/Y anyway, let's
	   hope it's enough.
	*/
	if (xwl_screen->outputs_initialized) {
	    rp = rrGetScrPriv(pScreen);
	    if (rp->rrScreenSetSize)
		rp->rrScreenSetSize(pScreen, scrn->virtualX, scrn->virtualY, width_mm, height_mm);
	}
    }

    xwl_output->xf86crtc->enabled = TRUE;
    mode = xf86CVTMode(width, height, 60, TRUE, FALSE);
    xf86CrtcSetModeTransform(xwl_output->xf86crtc, mode, RR_Rotate_0, NULL, xwl_output->x, xwl_output->y);
}

static const struct wl_output_listener output_listener = {
    display_handle_geometry,
    display_handle_mode
};

static void
global_handler(void *data, struct wl_registry *registry, uint32_t id,
	       const char *interface, uint32_t version)
{
    struct xwl_screen *xwl_screen = data;
    struct xwl_output *xwl_output;

    if (strcmp (interface, "wl_output") == 0) {
	xwl_output = xwl_output_create(xwl_screen);
	xwl_output->output = wl_registry_bind(registry, id,
	                                      &wl_output_interface, 1);
	xwl_output->name = id;
	wl_output_add_listener(xwl_output->output,
			       &output_listener, xwl_output);
    }
}

void
xwl_output_remove(struct xwl_output *xwl_output)
{
    xorg_list_del (&xwl_output->link);
    xf86OutputDestroy (xwl_output->xf86output);
    xf86CrtcDestroy (xwl_output->xf86crtc);

    wl_output_destroy (xwl_output->output);
}

static void
global_remove(void *data, struct wl_registry *registry, uint32_t name)
{
    struct xwl_screen *xwl_screen = data;
    struct xwl_output *xwl_output, *tmp;

    xorg_list_for_each_entry_safe (xwl_output, tmp, &xwl_screen->output_list, link) {
	if (xwl_output->name == name) {
	    xwl_output_remove(xwl_output);
	    break;
	}
    }
}  

static const struct wl_registry_listener global_listener = {
    global_handler,
    global_remove
};

void
xwayland_screen_preinit_output(struct xwl_screen *xwl_screen, ScrnInfoPtr scrninfo)
{
    int ret;
    DisplayModePtr last, mode = NULL;

    xf86CrtcConfigInit(scrninfo, &config_funcs);

    xf86CrtcSetSizeRange(scrninfo, 320, 200, 8192, 8192);

    xwl_screen->output_registry = wl_display_get_registry(xwl_screen->display);
    wl_registry_add_listener(xwl_screen->output_registry, &global_listener,
                             xwl_screen);

    while (xwl_screen->output_list.next == &xwl_screen->output_list) {
        ret = wl_display_roundtrip(xwl_screen->display);
        if (ret == -1)
            FatalError("failed to dispatch Wayland events: %s\n", strerror(errno));
    }

    xwl_screen->outputs_initialized = TRUE;

    if (!scrninfo->modes) {
        scrninfo->modes = xf86ModesAdd(scrninfo->modes,
				       xf86CVTMode(scrninfo->display->virtualX,
						   scrninfo->display->virtualY,
						   60, 0, 0));
    }

    /* For some reason, scrninfo->modes is circular, unlike the other mode
     * lists.  How great is that?
     */
    for (last = scrninfo->modes; last && last->next; last = last->next);
    last->next = scrninfo->modes;
    scrninfo->modes->prev = last;
    if (mode) {
        while (scrninfo->modes != mode)
            scrninfo->modes = scrninfo->modes->next;
    }

    scrninfo->currentMode = scrninfo->modes;
}
