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

#include "config.h"

#include "gtkcsstypesprivate.h"
#include "gtkthemingbackgroundprivate.h"
#include "gtkthemingengineprivate.h"

#include <math.h>

#include <gdk/gdk.h>

/* this is in case round() is not provided by the compiler, 
 * such as in the case of C89 compilers, like MSVC
 */
#include "fallback-c89.c"

static void
_gtk_theming_background_apply_window_background (GtkThemingBackground *bg,
                                                 cairo_t              *cr)
{
  if (gtk_style_context_has_class (bg->context, "background"))
    {
      cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 0.0); /* transparent */
      cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
      cairo_paint (cr);
    }
}

static void
_gtk_theming_background_apply_origin (GtkThemingBackground *bg)
{
  GtkCssArea origin;
  cairo_rectangle_t image_rect;

  gtk_style_context_get (bg->context, bg->flags,
                         "background-origin", &origin,
                         NULL);

  /* The default size of the background image depends on the
     background-origin value as this affects the top left
     and the bottom right corners. */
  switch (origin) {
  case GTK_CSS_AREA_BORDER_BOX:
    image_rect.x = 0;
    image_rect.y = 0;
    image_rect.width = bg->paint_area.width;
    image_rect.height = bg->paint_area.height;
    break;
  case GTK_CSS_AREA_CONTENT_BOX:
    image_rect.x = bg->border.left + bg->padding.left;
    image_rect.y = bg->border.top + bg->padding.top;
    image_rect.width = bg->paint_area.width - bg->border.left - bg->border.right - bg->padding.left - bg->padding.right;
    image_rect.height = bg->paint_area.height - bg->border.top - bg->border.bottom - bg->padding.top - bg->padding.bottom;
    break;
  case GTK_CSS_AREA_PADDING_BOX:
  default:
    image_rect.x = bg->border.left;
    image_rect.y = bg->border.top;
    image_rect.width = bg->paint_area.width - bg->border.left - bg->border.right;
    image_rect.height = bg->paint_area.height - bg->border.top - bg->border.bottom;
    break;
  }

  /* XXX: image_rect might have negative width/height here.
   * Do we need to do something about it? */
  bg->image_rect = image_rect;
}

static void
_gtk_theming_background_apply_clip (GtkThemingBackground *bg)
{
  GtkCssArea clip;

  gtk_style_context_get (bg->context, bg->flags,
                         "background-clip", &clip,
                         NULL);

  if (clip == GTK_CSS_AREA_PADDING_BOX)
    {
      _gtk_rounded_box_shrink (&bg->clip_box,
			       bg->border.top, bg->border.right,
			       bg->border.bottom, bg->border.left);
    }
  else if (clip == GTK_CSS_AREA_CONTENT_BOX)
    {
      _gtk_rounded_box_shrink (&bg->clip_box,
			       bg->border.top + bg->padding.top,
			       bg->border.right + bg->padding.right,
			       bg->border.bottom + bg->padding.bottom,
			       bg->border.left + bg->padding.left);
    }
}

static void
_gtk_theming_background_get_cover_contain (GtkCssImage *image,
                                           gboolean     cover,
                                           double       width,
                                           double       height,
                                           double      *concrete_width,
                                           double      *concrete_height)
{
  double aspect, image_aspect;
  
  image_aspect = _gtk_css_image_get_aspect_ratio (image);
  if (image_aspect == 0.0)
    {
      *concrete_width = width;
      *concrete_height = height;
      return;
    }

  aspect = width / height;

  if ((aspect >= image_aspect && cover) ||
      (aspect < image_aspect && !cover))
    {
      *concrete_width = width;
      *concrete_height = width / image_aspect;
    }
  else
    {
      *concrete_height = height;
      *concrete_width = height * image_aspect;
    }
}

static void
_gtk_theming_background_paint (GtkThemingBackground *bg,
                               cairo_t              *cr)
{
  cairo_save (cr);

  _gtk_rounded_box_path (&bg->clip_box, cr);
  cairo_clip (cr);

  gdk_cairo_set_source_rgba (cr, &bg->bg_color);
  cairo_paint (cr);

  if (bg->image
      && bg->image_rect.width > 0
      && bg->image_rect.height > 0)
    {
      GtkCssBackgroundRepeat hrepeat, vrepeat;
      GtkCssBackgroundSize *size;
      GtkCssBackgroundPosition *pos;
      double image_width, image_height;
      double width, height;

      size = _gtk_css_value_get_background_size (_gtk_style_context_peek_property (bg->context, "background-size"));
      pos = _gtk_css_value_get_background_position (_gtk_style_context_peek_property (bg->context, "background-position"));
      gtk_style_context_get (bg->context, bg->flags,
                             "background-repeat", &hrepeat,
                             NULL);
      vrepeat = GTK_CSS_BACKGROUND_VERTICAL (hrepeat);
      hrepeat = GTK_CSS_BACKGROUND_HORIZONTAL (hrepeat);
      width = bg->image_rect.width;
      height = bg->image_rect.height;

      if (size->contain || size->cover)
        _gtk_theming_background_get_cover_contain (bg->image,
                                                   size->cover,
                                                   width,
                                                   height,
                                                   &image_width,
                                                   &image_height);
      else
        _gtk_css_image_get_concrete_size (bg->image,
                                          /* note: 0 does the right thing here for 'auto' */
                                          _gtk_css_number_get (&size->width, width),
                                          _gtk_css_number_get (&size->height, height),
                                          width, height,
                                          &image_width, &image_height);

      /* optimization */
      if (image_width == width)
        hrepeat = GTK_CSS_BACKGROUND_NO_REPEAT;
      if (image_height == height)
        vrepeat = GTK_CSS_BACKGROUND_NO_REPEAT;

      cairo_translate (cr, bg->image_rect.x, bg->image_rect.y);

      if (hrepeat == GTK_CSS_BACKGROUND_NO_REPEAT && vrepeat == GTK_CSS_BACKGROUND_NO_REPEAT)
        {
	  cairo_translate (cr,
			   _gtk_css_number_get (&pos->x, bg->image_rect.width - image_width),
			   _gtk_css_number_get (&pos->y, bg->image_rect.height - image_height));
          /* shortcut for normal case */
          _gtk_css_image_draw (bg->image, cr, image_width, image_height);
        }
      else
        {
          int surface_width, surface_height;
          cairo_surface_t *surface;
          cairo_t *cr2;

          /* If ‘background-repeat’ is ‘round’ for one (or both) dimensions,
           * there is a second step. The UA must scale the image in that
           * dimension (or both dimensions) so that it fits a whole number of
           * times in the background positioning area. In the case of the width
           * (height is analogous):
           *
           * If X ≠ 0 is the width of the image after step one and W is the width
           * of the background positioning area, then the rounded width
           * X' = W / round(W / X) where round() is a function that returns the
           * nearest natural number (integer greater than zero). 
           *
           * If ‘background-repeat’ is ‘round’ for one dimension only and if
           * ‘background-size’ is ‘auto’ for the other dimension, then there is
           * a third step: that other dimension is scaled so that the original
           * aspect ratio is restored. 
           */
          if (hrepeat == GTK_CSS_BACKGROUND_ROUND)
            {
              double n = round (width / image_width);

              n = MAX (1, n);

              if (vrepeat != GTK_CSS_BACKGROUND_ROUND
                  /* && vsize == auto (it is by default) */)
                image_height *= width / (image_width * n);
              image_width = width / n;
            }
          if (vrepeat == GTK_CSS_BACKGROUND_ROUND)
            {
              double n = round (height / image_height);

              n = MAX (1, n);

              if (hrepeat != GTK_CSS_BACKGROUND_ROUND
                  /* && hsize == auto (it is by default) */)
                image_width *= height / (image_height * n);
              image_height = height / n;
            }

          /* if hrepeat or vrepeat is 'space', we create a somewhat larger surface
           * to store the extra space. */
          if (hrepeat == GTK_CSS_BACKGROUND_SPACE)
            {
              double n = floor (width / image_width);
              surface_width = n ? round (width / n) : 0;
            }
          else
            surface_width = round (image_width);

          if (vrepeat == GTK_CSS_BACKGROUND_SPACE)
            {
              double n = floor (height / image_height);
              surface_height = n ? round (height / n) : 0;
            }
          else
            surface_height = round (image_height);

          surface = cairo_surface_create_similar (cairo_get_target (cr),
                                                  CAIRO_CONTENT_COLOR_ALPHA,
                                                  surface_width, surface_height);
          cr2 = cairo_create (surface);
          cairo_translate (cr2,
                           0.5 * (surface_width - image_width),
                           0.5 * (surface_height - image_height));
          _gtk_css_image_draw (bg->image, cr2, image_width, image_height);
          cairo_destroy (cr2);

          cairo_set_source_surface (cr, surface,
				    _gtk_css_number_get (&pos->x, bg->image_rect.width - image_width),
				    _gtk_css_number_get (&pos->y, bg->image_rect.height - image_height));
          cairo_pattern_set_extend (cairo_get_source (cr), CAIRO_EXTEND_REPEAT);
          cairo_surface_destroy (surface);

          cairo_rectangle (cr,
                           0, 0,
                           hrepeat == GTK_CSS_BACKGROUND_NO_REPEAT ? image_width : width,
                           vrepeat == GTK_CSS_BACKGROUND_NO_REPEAT ? image_height : height);
          cairo_fill (cr);
        }
    }

  cairo_restore (cr);
}

static void
_gtk_theming_background_apply_shadow (GtkThemingBackground *bg,
                                      cairo_t              *cr)
{
  GtkShadow *box_shadow;

  gtk_style_context_get (bg->context, bg->flags,
                         "box-shadow", &box_shadow,
                         NULL);

  if (box_shadow != NULL)
    {
      _gtk_box_shadow_render (box_shadow, cr, &bg->padding_box);
      _gtk_shadow_unref (box_shadow);
    }
}

static void
_gtk_theming_background_init_context (GtkThemingBackground *bg)
{
  bg->flags = gtk_style_context_get_state (bg->context);

  gtk_style_context_get_border (bg->context, bg->flags, &bg->border);
  gtk_style_context_get_padding (bg->context, bg->flags, &bg->padding);
  gtk_style_context_get_background_color (bg->context, bg->flags, &bg->bg_color);

  /* In the CSS box model, by default the background positioning area is
   * the padding-box, i.e. all the border-box minus the borders themselves,
   * which determines also its default size, see
   * http://dev.w3.org/csswg/css3-background/#background-origin
   *
   * In the future we might want to support different origins or clips, but
   * right now we just shrink to the default.
   */
  _gtk_rounded_box_init_rect (&bg->padding_box, 0, 0, bg->paint_area.width, bg->paint_area.height);

  _gtk_rounded_box_apply_border_radius_for_context (&bg->padding_box, bg->context, bg->flags, bg->junction);

  bg->clip_box = bg->padding_box;
  _gtk_rounded_box_shrink (&bg->padding_box,
			   bg->border.top, bg->border.right,
			   bg->border.bottom, bg->border.left);

  _gtk_theming_background_apply_clip (bg);
  _gtk_theming_background_apply_origin (bg);

  bg->image = _gtk_css_value_get_image (_gtk_style_context_peek_property (bg->context, "background-image"));
}

void
_gtk_theming_background_init (GtkThemingBackground *bg,
                              GtkThemingEngine     *engine,
                              gdouble               x,
                              gdouble               y,
                              gdouble               width,
                              gdouble               height,
                              GtkJunctionSides      junction)
{
  GtkStyleContext *context;

  g_assert (bg != NULL);

  context = _gtk_theming_engine_get_context (engine);
  _gtk_theming_background_init_from_context (bg, context,
                                             x, y, width, height,
                                             junction);
}

void
_gtk_theming_background_init_from_context (GtkThemingBackground *bg,
                                           GtkStyleContext      *context,
                                           gdouble               x,
                                           gdouble               y,
                                           gdouble               width,
                                           gdouble               height,
                                           GtkJunctionSides      junction)
{
  g_assert (bg != NULL);

  bg->context = context;

  bg->paint_area.x = x;
  bg->paint_area.y = y;
  bg->paint_area.width = width;
  bg->paint_area.height = height;

  bg->image = NULL;
  bg->junction = junction;

  _gtk_theming_background_init_context (bg);
}

void
_gtk_theming_background_render (GtkThemingBackground *bg,
                                cairo_t              *cr)
{
  cairo_save (cr);
  cairo_translate (cr, bg->paint_area.x, bg->paint_area.y);

  _gtk_theming_background_apply_window_background (bg, cr);
  _gtk_theming_background_paint (bg, cr);
  _gtk_theming_background_apply_shadow (bg, cr);

  cairo_restore (cr);
}

gboolean
_gtk_theming_background_has_background_image (GtkThemingBackground *bg)
{
  return (bg->image != NULL) ? TRUE : FALSE;
}
