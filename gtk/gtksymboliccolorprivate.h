/* GTK - The GIMP Toolkit
 * Copyright (C) 2011 Benjamin Otte <otte@gnome.org>
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

#ifndef __GTK_SYMBOLIC_COLOR_PRIVATE_H__
#define __GTK_SYMBOLIC_COLOR_PRIVATE_H__

#include "gtk/gtksymboliccolor.h"
#include "gtk/gtkcssvalueprivate.h"

G_BEGIN_DECLS

typedef GtkSymbolicColor * (* GtkSymbolicColorLookupFunc) (gpointer data, const char *name);

GtkCssValue *      _gtk_symbolic_color_resolve_full       (GtkSymbolicColor           *color,
                                                           GtkSymbolicColorLookupFunc  func,
                                                           gpointer                    data);

GtkSymbolicColor * _gtk_symbolic_color_get_current_color  (void);

G_END_DECLS

#endif /* __GTK_SYMBOLIC_COLOR_PRIVATE_H__ */
