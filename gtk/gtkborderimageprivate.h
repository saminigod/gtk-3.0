/* GTK - The GIMP Toolkit
 * Copyright (C) 2010 Carlos Garnacho <carlosg@gnome.org>
 * Copyright (C) 2011 Red Hat, Inc.
 *
 * Authors: Carlos Garnacho <carlosg@gnome.org>
 *          Cosimo Cecchi <cosimoc@gnome.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GTK_BORDER_IMAGE_H__
#define __GTK_BORDER_IMAGE_H__

#include "gtkborder.h"
#include "gtkcssimageprivate.h"
#include "gtkthemingengine.h"
#include "gtkcsstypesprivate.h"

G_BEGIN_DECLS

typedef struct _GtkBorderImage GtkBorderImage;

struct _GtkBorderImage {
  GtkCssImage *source;

  GtkBorder slice;
  gboolean has_width;
  GtkBorder width;
  GtkCssBorderImageRepeat repeat;
};

gboolean          _gtk_border_image_init             (GtkBorderImage       *image,
                                                      GtkThemingEngine     *engine);

void              _gtk_border_image_render           (GtkBorderImage       *image,
                                                      GtkBorder            *border_width,
                                                      cairo_t              *cr,
                                                      gdouble               x,
                                                      gdouble               y,
                                                      gdouble               width,
                                                      gdouble               height);

G_END_DECLS

#endif /* __GTK_BORDER_IMAGE_H__ */
