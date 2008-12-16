/*
 * Clutter.
 *
 * An OpenGL based 'interactive canvas' library.
 *
 * Authored By Matthew Allum  <mallum@openedhand.com>
 *
 * Copyright (C) 2006 OpenedHand
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
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef _HAVE_CLUTTER_ROUND_RECTANGLE_H
#define _HAVE_CLUTTER_ROUND_RECTANGLE_H

#include <glib-object.h>
#include <clutter/clutter-actor.h>
#include <clutter/clutter-color.h>

G_BEGIN_DECLS

#define CLUTTER_TYPE_ROUND_RECTANGLE clutter_round_rectangle_get_type()

#define CLUTTER_ROUND_RECTANGLE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  CLUTTER_TYPE_ROUND_RECTANGLE, ClutterRoundRectangle))

#define CLUTTER_ROUND_RECTANGLE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  CLUTTER_TYPE_ROUND_RECTANGLE, ClutterRoundRectangleClass))

#define CLUTTER_IS_ROUND_RECTANGLE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  CLUTTER_TYPE_ROUND_RECTANGLE))

#define CLUTTER_IS_ROUND_RECTANGLE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  CLUTTER_TYPE_ROUND_RECTANGLE))

#define CLUTTER_ROUND_RECTANGLE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  CLUTTER_TYPE_ROUND_RECTANGLE, ClutterRoundRectangleClass))

typedef struct _ClutterRoundRectangle        ClutterRoundRectangle;
typedef struct _ClutterRoundRectangleClass   ClutterRoundRectangleClass;
typedef struct _ClutterRoundRectanglePrivate ClutterRoundRectanglePrivate;

struct _ClutterRoundRectangle
{
  ClutterActor           parent;

  /*< private >*/
  ClutterRoundRectanglePrivate *priv;
}; 

struct _ClutterRoundRectangleClass 
{
  ClutterActorClass parent_class;

  /* padding for future expansion */
  void (*_clutter_round_rectangle1) (void);
  void (*_clutter_round_rectangle2) (void);
  void (*_clutter_round_rectangle3) (void);
  void (*_clutter_round_rectangle4) (void);
};

GType clutter_round_rectangle_get_type (void) G_GNUC_CONST;

ClutterActor *clutter_round_rectangle_new              (void);
ClutterActor *clutter_round_rectangle_new_with_color   (const ClutterColor *color);

void          clutter_round_rectangle_get_color        (ClutterRoundRectangle   *rectangle,
                                                  ClutterColor       *color);
void          clutter_round_rectangle_set_color        (ClutterRoundRectangle   *rectangle,
						  const ClutterColor *color);
guint         clutter_round_rectangle_get_border_width (ClutterRoundRectangle   *rectangle);
void          clutter_round_rectangle_set_border_width (ClutterRoundRectangle   *rectangle,
                                                  guint               width);
void          clutter_round_rectangle_get_border_color (ClutterRoundRectangle   *rectangle,
                                                  ClutterColor       *color);
void          clutter_round_rectangle_set_border_color (ClutterRoundRectangle   *rectangle,
                                                  const ClutterColor *color);

G_END_DECLS

#endif
