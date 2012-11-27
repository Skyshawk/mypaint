# This file is part of MyPaint.
# Copyright (C) 2012 by Ali Lown <ali@lown.me.uk>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.

from gettext import gettext as _
import gtk, gobject, pango
gdk = gtk.gdk
import dialogs
import tileddrawwidget

# import bisect


class ToolWidget(gtk.VBox):
    """Tool widget for previewing the whole canvas"""

    stock_id = "mypaint-tool-preview-window"
    tool_widget_title = _("Preview")


    def __init__(self, app):
        gtk.VBox.__init__(self)
        self.app = app
        self.set_size_request(250, 250)

        self.doc = app.doc.model
        self.tdw = tileddrawwidget.TiledDrawWidget(app, self.doc)
        self.tdw.zoom_min = 1/50.0
        self.tdw.set_size_request(250, 250)
        self.tdw.set_sensitive(False)
        self.add(self.tdw)

        ## Supported zoom levels
        ## Not sure if this looks better
        #self.zoomlevel_values = [
        #    1.0/128, 1.5/128,
        #    1.0/64, 1.5/64,
        #    1.0/32, 1.5/32,
        #    1.0/16, 1.0/8, 2.0/11, 0.25, 1.0/3, 0.50, 2.0/3,
        #    1.0 ]

        self.tdw.zoom_min = 1.0 / 128
        self.tdw.zoom_max = 1.0 / 2
        self.tdw.scale = self.app.preferences['view.default_zoom']

        # Used for detection of potential effective bbox changes during
        # canvas modify events
        self.x_min = self.x_max = None
        self.y_min = self.y_max = None

        # Used for determining if a potential bbox change is a real one
        self.last_resize_bbox = None

        # Document observers for scale and zoom
        self.doc.canvas_observers.append(self.canvas_modified_cb)
        self.doc.doc_observers.append(self.doc_structure_modified_cb)
        self.doc.frame_observers.append(self.frame_modified_cb)
        self.connect("size-allocate", self.size_alloc_cb)


    def limit_scale(self, scale):
        scale = min(scale, self.tdw.zoom_max)
        scale = max(scale, self.tdw.zoom_min)

        ## Limit to a supported zoom level
        #scale_i = bisect.bisect_left(self.zoomlevel_values, scale)
        #if scale_i >= len(self.zoomlevel_values):
        #    scale_i = len(self.zoomlevel_values) - 1
        #scale = self.zoomlevel_values[max(0, scale_i-1)]

        return scale


    def force_resize(self):
        self.x_min = None
        self.x_max = None
        self.y_min = None
        self.y_max = None
        self.last_resize_bbox = None
        self.resize_to_effective_bbox()


    def size_alloc_cb(self, widget, alloc):
        # Rescaling is always needed when the widget size changes.
        self.force_resize()


    def frame_modified_cb(self, *args):
        # Effective bbox change due to frame is adjustment or toggle.
        self.force_resize()


    def doc_structure_modified_cb(self, *args):
        # Potentially a layer clear, which could affect the bbox.
        self.force_resize()


    def canvas_modified_cb(self, x, y, w, h):
        # Called when layer contents change and redraw is required.
        # Try to avoid unnecessary updates, e.g. drawing iside the canvas.
        resize_needed = False

        if x == 0 and y == 0 and w == 0 and h == 0:
            # Redraw-all notifications.
            # Don't track the zeros.
            resize_needed = True
        else:
            # Real update rectangle: track size, resize if grown.
            if self.x_min is None or x < self.x_min:
                self.x_min = x
                resize_needed = True
            if self.x_max is None or x+w > self.x_max:
                self.x_max = x+w
                resize_needed = True
            if self.y_min is None or y < self.y_min:
                self.y_min = y
                resize_needed = True
            if self.y_max is None or y+h > self.y_max:
                self.y_max = y+h
                resize_needed = True

        if resize_needed:
            self.resize_to_effective_bbox()


    def resize_to_effective_bbox(self):
        """Resize and center appropriately."""

        # Calculate new zoom level and centering based on the effective bbox
        alloc = self.get_allocation()

        # Don't resize unless the bbox has actually changed.
        # Avoids juddering.
        bbox = tuple(self.doc.get_effective_bbox())
        if self.last_resize_bbox == bbox:
            return
        self.last_resize_bbox = bbox

        # Effective bbox dimensions.
        fx, fy, fw, fh = bbox

        # Avoid a division by zero
        if fw == 0:
            fw = 64
        if fh == 0:
            fh = 64

        # Tracking vars may have been reset.
        # The bbox is a pretty good seed value for them...
        if fx < self.x_min:
            self.x_min = fx
        if fx+fw > self.x_max:
            self.x_max = fx
        if fy < self.y_min:
            self.y_min = fy+fw
        if fy+fw > self.y_max:
            self.y_max = fy+fw

        # Scale to fit within a rectangle slightly smaller than the widget.
        # Slight borders are nice.
        zoom_x = (float(alloc.width) - 12) / fw
        zoom_y = (float(alloc.height) - 12) / fh

        scale = self.limit_scale(min(zoom_x, zoom_y))

        self.tdw.scale = scale
        self.tdw.recenter_document()
