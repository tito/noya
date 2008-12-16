#include <clutter/clutter.h>
#include "clutter-round-rectangle.h"

#ifndef CLUTTER_PARAM_READWRITE
#define CLUTTER_PARAM_READWRITE \
         G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK |G_PARAM_STATIC_BLURB
#endif

G_DEFINE_TYPE (ClutterRoundRectangle, clutter_round_rectangle, CLUTTER_TYPE_ACTOR);

enum
{
  PROP_0,

  PROP_COLOR,
  PROP_BORDER_COLOR,
  PROP_BORDER_WIDTH,
  PROP_HAS_BORDER

  /* FIXME: Add gradient, rounded corner props etc */
};

#define CLUTTER_ROUND_RECTANGLE_GET_PRIVATE(obj) \
(G_TYPE_INSTANCE_GET_PRIVATE ((obj), CLUTTER_TYPE_ROUND_RECTANGLE, ClutterRoundRectanglePrivate))

struct _ClutterRoundRectanglePrivate
{
  ClutterColor color;
  ClutterColor border_color;

  guint border_width;

  guint has_border : 1;
};

static void
clutter_round_rectangle_paint (ClutterActor *self)
{
  ClutterRoundRectangle        *rectangle = CLUTTER_ROUND_RECTANGLE(self);
  ClutterRoundRectanglePrivate *priv;
  ClutterGeometry          geom;
  ClutterColor             tmp_col;

  rectangle = CLUTTER_ROUND_RECTANGLE(self);
  priv = rectangle->priv;

  clutter_actor_get_allocation_geometry (self, &geom);

  /* parent paint call will have translated us into position so
   * paint from 0, 0
   */
  if (priv->has_border)
    {
      tmp_col.red   = priv->color.red;
      tmp_col.green = priv->color.green;
      tmp_col.blue  = priv->color.blue;
      tmp_col.alpha = clutter_actor_get_paint_opacity (self)
                      * priv->color.alpha
                      / 255;


      cogl_color (&tmp_col);

	  cogl_path_round_rectangle (
		  0, 0,
		  CLUTTER_INT_TO_FIXED(geom.width),
		  CLUTTER_INT_TO_FIXED(geom.height),
		  CLUTTER_INT_TO_FIXED(4), 1
	  );

	  cogl_path_fill();

      tmp_col.red   = priv->border_color.red;
      tmp_col.green = priv->border_color.green;
      tmp_col.blue  = priv->border_color.blue;
      tmp_col.alpha = clutter_actor_get_paint_opacity (self)
                      * priv->border_color.alpha
                      / 255;

      cogl_color (&tmp_col);

	  glEnable(GL_LINE_SMOOTH);
	  glHint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);
	  glLineWidth(1.0);

	  cogl_path_stroke();

	  glDisable(GL_LINE_SMOOTH);
    }
  else
    {
      tmp_col.red   = priv->color.red;
      tmp_col.green = priv->color.green;
      tmp_col.blue  = priv->color.blue;
      tmp_col.alpha = clutter_actor_get_paint_opacity (self)
                      * priv->color.alpha
                      / 255;

      cogl_color (&tmp_col);

      cogl_rectangle (0, 0, geom.width, geom.height);
    }
}

static void
clutter_round_rectangle_set_property (GObject      *object,
				guint         prop_id,
				const GValue *value,
				GParamSpec   *pspec)
{
  ClutterRoundRectangle *rectangle = CLUTTER_ROUND_RECTANGLE(object);

  switch (prop_id)
    {
    case PROP_COLOR:
      clutter_round_rectangle_set_color (rectangle, g_value_get_boxed (value));
      break;
    case PROP_BORDER_COLOR:
      clutter_round_rectangle_set_border_color (rectangle,
                                          g_value_get_boxed (value));
      break;
    case PROP_BORDER_WIDTH:
      clutter_round_rectangle_set_border_width (rectangle,
                                          g_value_get_uint (value));
      break;
    case PROP_HAS_BORDER:
      rectangle->priv->has_border = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
clutter_round_rectangle_get_property (GObject    *object,
				guint       prop_id,
				GValue     *value,
				GParamSpec *pspec)
{
  ClutterRoundRectangle *rectangle = CLUTTER_ROUND_RECTANGLE(object);
  ClutterColor      color;

  switch (prop_id)
    {
    case PROP_COLOR:
      clutter_round_rectangle_get_color (rectangle, &color);
      g_value_set_boxed (value, &color);
      break;
    case PROP_BORDER_COLOR:
      clutter_round_rectangle_get_border_color (rectangle, &color);
      g_value_set_boxed (value, &color);
      break;
    case PROP_BORDER_WIDTH:
      g_value_set_uint (value, rectangle->priv->border_width);
      break;
    case PROP_HAS_BORDER:
      g_value_set_boolean (value, rectangle->priv->has_border);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}


static void
clutter_round_rectangle_finalize (GObject *object)
{
  G_OBJECT_CLASS (clutter_round_rectangle_parent_class)->finalize (object);
}

static void
clutter_round_rectangle_dispose (GObject *object)
{
  G_OBJECT_CLASS (clutter_round_rectangle_parent_class)->dispose (object);
}


static void
clutter_round_rectangle_class_init (ClutterRoundRectangleClass *klass)
{
  GObjectClass        *gobject_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);

  actor_class->paint        = clutter_round_rectangle_paint;

  gobject_class->finalize     = clutter_round_rectangle_finalize;
  gobject_class->dispose      = clutter_round_rectangle_dispose;
  gobject_class->set_property = clutter_round_rectangle_set_property;
  gobject_class->get_property = clutter_round_rectangle_get_property;

  /**
   * ClutterRoundRectangle:color:
   *
   * The color of the rectangle.
   */
  g_object_class_install_property (gobject_class,
                                   PROP_COLOR,
                                   g_param_spec_boxed ("color",
                                                       "Color",
                                                       "The color of the rectangle",
                                                       CLUTTER_TYPE_COLOR,
                                                       CLUTTER_PARAM_READWRITE));
  /**
   * ClutterRoundRectangle:border-color:
   *
   * The color of the border of the rectangle.
   *
   * Since: 0.2
   */
  g_object_class_install_property (gobject_class,
                                   PROP_BORDER_COLOR,
                                   g_param_spec_boxed ("border-color",
                                                       "Border Color",
                                                       "The color of the border of the rectangle",
                                                       CLUTTER_TYPE_COLOR,
                                                       CLUTTER_PARAM_READWRITE));
  /**
   * ClutterRoundRectangle:border-width:
   *
   * The width of the border of the rectangle, in pixels.
   *
   * Since: 0.2
   */
  g_object_class_install_property (gobject_class,
                                   PROP_BORDER_WIDTH,
                                   g_param_spec_uint ("border-width",
                                                      "Border Width",
                                                      "The width of the border of the rectangle",
                                                      0, G_MAXUINT,
                                                      0,
                                                      CLUTTER_PARAM_READWRITE));
  /**
   * ClutterRoundRectangle:has-border:
   *
   * Whether the #ClutterRoundRectangle should be displayed with a border.
   *
   * Since: 0.2
   */
  g_object_class_install_property (gobject_class,
                                   PROP_HAS_BORDER,
                                   g_param_spec_boolean ("has-border",
                                                         "Has Border",
                                                         "Whether the rectangle should have a border",
                                                         FALSE,
                                                         CLUTTER_PARAM_READWRITE));

  g_type_class_add_private (gobject_class, sizeof (ClutterRoundRectanglePrivate));
}

static void
clutter_round_rectangle_init (ClutterRoundRectangle *self)
{
  ClutterRoundRectanglePrivate *priv;

  self->priv = priv = CLUTTER_ROUND_RECTANGLE_GET_PRIVATE (self);

  priv->color.red = 0xff;
  priv->color.green = 0xff;
  priv->color.blue = 0xff;
  priv->color.alpha = 0xff;

  priv->border_color.red = 0x00;
  priv->border_color.green = 0x00;
  priv->border_color.blue = 0x00;
  priv->border_color.alpha = 0xff;

  priv->border_width = 0;

  priv->has_border = FALSE;
}

/**
 * clutter_round_rectangle_new:
 *
 * Creates a new #ClutterActor with a rectangular shape.
 *
 * Return value: a new #ClutterActor
 */
ClutterActor*
clutter_round_rectangle_new (void)
{
  return g_object_new (CLUTTER_TYPE_ROUND_RECTANGLE, NULL);
}

/**
 * clutter_round_rectangle_new_with_color:
 * @color: a #ClutterColor
 *
 * Creates a new #ClutterActor with a rectangular shape
 * and of the given @color.
 *
 * Return value: a new #ClutterActor
 */
ClutterActor *
clutter_round_rectangle_new_with_color (const ClutterColor *color)
{
  return g_object_new (CLUTTER_TYPE_ROUND_RECTANGLE,
		       "color", color,
		       NULL);
}

/**
 * clutter_round_rectangle_get_color:
 * @rectangle: a #ClutterRoundRectangle
 * @color: return location for a #ClutterColor
 *
 * Retrieves the color of @rectangle.
 */
void
clutter_round_rectangle_get_color (ClutterRoundRectangle *rectangle,
			     ClutterColor     *color)
{
  ClutterRoundRectanglePrivate *priv;

  g_return_if_fail (CLUTTER_IS_ROUND_RECTANGLE (rectangle));
  g_return_if_fail (color != NULL);

  priv = rectangle->priv;

  color->red = priv->color.red;
  color->green = priv->color.green;
  color->blue = priv->color.blue;
  color->alpha = priv->color.alpha;
}

/**
 * clutter_round_rectangle_set_color:
 * @rectangle: a #ClutterRoundRectangle
 * @color: a #ClutterColor
 *
 * Sets the color of @rectangle.
 */
void
clutter_round_rectangle_set_color (ClutterRoundRectangle   *rectangle,
			     const ClutterColor *color)
{
  ClutterRoundRectanglePrivate *priv;

  g_return_if_fail (CLUTTER_IS_ROUND_RECTANGLE (rectangle));
  g_return_if_fail (color != NULL);

  g_object_ref (rectangle);

  priv = rectangle->priv;

  priv->color.red = color->red;
  priv->color.green = color->green;
  priv->color.blue = color->blue;
  priv->color.alpha = color->alpha;

#if 0
  /* FIXME - appears to be causing border to always get drawn */
  if (clutter_color_equal (&priv->color, &priv->border_color))
    priv->has_border = FALSE;
  else
    priv->has_border = TRUE;
#endif

  if (CLUTTER_ACTOR_IS_VISIBLE (rectangle))
    clutter_actor_queue_redraw (CLUTTER_ACTOR (rectangle));

  g_object_notify (G_OBJECT (rectangle), "color");
  g_object_notify (G_OBJECT (rectangle), "has-border");
  g_object_unref (rectangle);
}

/**
 * clutter_round_rectangle_get_border_width:
 * @rectangle: a #ClutterRoundRectangle
 *
 * Gets the width (in pixels) of the border used by @rectangle
 *
 * Return value: the border's width
 *
 * Since: 0.2
 */
guint
clutter_round_rectangle_get_border_width (ClutterRoundRectangle *rectangle)
{
  g_return_val_if_fail (CLUTTER_IS_ROUND_RECTANGLE (rectangle), 0);

  return rectangle->priv->border_width;
}

/**
 * clutter_round_rectangle_set_border_width:
 * @rectangle: a #ClutterRoundRectangle
 * @width: the width of the border
 *
 * Sets the width (in pixel) of the border used by @rectangle.
 * A @width of 0 will unset the border.
 *
 * Since: 0.2
 */
void
clutter_round_rectangle_set_border_width (ClutterRoundRectangle *rectangle,
                                    guint             width)
{
  ClutterRoundRectanglePrivate *priv;

  g_return_if_fail (CLUTTER_IS_ROUND_RECTANGLE (rectangle));
  priv = rectangle->priv;

  if (priv->border_width != width)
    {
      g_object_ref (rectangle);

      priv->border_width = width;

      if (priv->border_width != 0)
        priv->has_border = TRUE;
      else
        priv->has_border = FALSE;

      if (CLUTTER_ACTOR_IS_VISIBLE (CLUTTER_ACTOR (rectangle)))
        clutter_actor_queue_redraw (CLUTTER_ACTOR (rectangle));

      g_object_notify (G_OBJECT (rectangle), "border-width");
      g_object_notify (G_OBJECT (rectangle), "has-border");
      g_object_unref (rectangle);
    }
}

/**
 * clutter_round_rectangle_get_border_color:
 * @rectangle: a #ClutterRoundRectangle
 * @color: return location for a #ClutterColor
 *
 * Gets the color of the border used by @rectangle and places
 * it into @color.
 *
 * Since: 0.2
 */
void
clutter_round_rectangle_get_border_color (ClutterRoundRectangle *rectangle,
                                    ClutterColor     *color)
{
  ClutterRoundRectanglePrivate *priv;

  g_return_if_fail (CLUTTER_IS_ROUND_RECTANGLE (rectangle));
  g_return_if_fail (color != NULL);

  priv = rectangle->priv;

  color->red = priv->border_color.red;
  color->green = priv->border_color.green;
  color->blue = priv->border_color.blue;
  color->alpha = priv->border_color.alpha;
}

/**
 * clutter_round_rectangle_set_border_color:
 * @rectangle: a #ClutterRoundRectangle
 * @color: the color of the border
 *
 * Sets the color of the border used by @rectangle using @color
 */
void
clutter_round_rectangle_set_border_color (ClutterRoundRectangle   *rectangle,
                                    const ClutterColor *color)
{
  ClutterRoundRectanglePrivate *priv;

  g_return_if_fail (CLUTTER_IS_ROUND_RECTANGLE (rectangle));
  g_return_if_fail (color != NULL);

  priv = rectangle->priv;

  if (priv->border_color.red != color->red ||
      priv->border_color.green != color->green ||
      priv->border_color.blue != color->blue ||
      priv->border_color.alpha != color->alpha)
    {
      g_object_ref (rectangle);

      priv->border_color.red = color->red;
      priv->border_color.green = color->green;
      priv->border_color.blue = color->blue;
      priv->border_color.alpha = color->alpha;

      if (clutter_color_equal (&priv->color, &priv->border_color))
        priv->has_border = FALSE;
      else
        priv->has_border = TRUE;

      if (CLUTTER_ACTOR_IS_VISIBLE (CLUTTER_ACTOR (rectangle)))
        clutter_actor_queue_redraw (CLUTTER_ACTOR (rectangle));

      g_object_notify (G_OBJECT (rectangle), "border-color");
      g_object_notify (G_OBJECT (rectangle), "has-border");
      g_object_unref (rectangle);
    }
}
