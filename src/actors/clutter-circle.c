#include <clutter/clutter.h>
#include "clutter-circle.h"

#ifndef CLUTTER_PARAM_READWRITE
#define CLUTTER_PARAM_READWRITE \
         G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK |G_PARAM_STATIC_BLURB
#endif


G_DEFINE_TYPE (ClutterCircle, clutter_circle, CLUTTER_TYPE_ACTOR);

enum
{
  PROP_0,
  PROP_COLOR,
  PROP_ANGLE_START,
  PROP_ANGLE_STOP,
  PROP_WIDTH,
  PROP_RADIUS,
};

#define CLUTTER_CIRCLE_GET_PRIVATE(obj) \
(G_TYPE_INSTANCE_GET_PRIVATE ((obj), CLUTTER_TYPE_CIRCLE, ClutterCirclePrivate))

struct _ClutterCirclePrivate
{
  ClutterColor color;
  ClutterFixed angle_start;
  ClutterFixed angle_stop;
  ClutterFixed width;
  ClutterFixed radius;
};

static void
clutter_circle_paint (ClutterActor *self)
{
	ClutterFixed				dw, dh;
	ClutterCircle				*circle = CLUTTER_CIRCLE(self);
	ClutterCirclePrivate		*priv;
	ClutterGeometry				geom;
	ClutterColor				tmp_col;
	ClutterFixed				precision = 1;

	circle = CLUTTER_CIRCLE(self);
	priv = circle->priv;

	clutter_actor_get_allocation_geometry (self, &geom);

	tmp_col.red   = priv->color.red;
	tmp_col.green = priv->color.green;
	tmp_col.blue  = priv->color.blue;
	tmp_col.alpha = clutter_actor_get_paint_opacity (self)
		* priv->color.alpha
		/ 255;

	cogl_color (&tmp_col);

	if ( priv->radius == 0 )
		clutter_circle_set_radius(circle, geom.width);

	dw = CLUTTER_INT_TO_FIXED(geom.width) >> 1;
	dh = CLUTTER_INT_TO_FIXED(geom.height) >> 1;
	/**
	cogl_path_ellipse (dw, dh,
		CLUTTER_INT_TO_FIXED(geom.width),
		CLUTTER_INT_TO_FIXED(geom.height),
		);
	**/
	cogl_path_move_to(dw, dh);
	cogl_path_arc_rel(0, 0,
		CLUTTER_INT_TO_FIXED(priv->radius),
		CLUTTER_INT_TO_FIXED(priv->radius),
		CLUTTER_ANGLE_FROM_DEG(priv->angle_start + 270),
		CLUTTER_ANGLE_FROM_DEG(priv->angle_stop + 270),
		precision
	);

	if ( priv->width != 0 )
	{
		cogl_path_line_to(dw, dh);
		cogl_path_arc_rel(0, 0,
			CLUTTER_INT_TO_FIXED(priv->radius + priv->width),
			CLUTTER_INT_TO_FIXED(priv->radius + priv->width),
			CLUTTER_ANGLE_FROM_DEG(priv->angle_stop + 270),
			CLUTTER_ANGLE_FROM_DEG(priv->angle_start + 270),
			precision
		);
	}
	cogl_path_close();
	cogl_path_fill();

}

void
clutter_circle_set_angle_start (ClutterCircle *circle,
                                guint          angle)
{
	ClutterCirclePrivate *priv;

	g_return_if_fail (CLUTTER_IS_CIRCLE (circle));
	priv = circle->priv;

	if (priv->angle_start != angle)
	{
		g_object_ref (circle);

		priv->angle_start = angle;

		if (CLUTTER_ACTOR_IS_VISIBLE (CLUTTER_ACTOR (circle)))
			clutter_actor_queue_redraw (CLUTTER_ACTOR (circle));

		g_object_notify (G_OBJECT (circle), "angle-start");
		g_object_unref (circle);
	}
}

void
clutter_circle_set_angle_stop (ClutterCircle *circle,
                                guint          angle)
{
	ClutterCirclePrivate *priv;

	g_return_if_fail (CLUTTER_IS_CIRCLE (circle));
	priv = circle->priv;

	if (priv->angle_stop != angle)
	{
		g_object_ref (circle);

		priv->angle_stop = angle;

		if (CLUTTER_ACTOR_IS_VISIBLE (CLUTTER_ACTOR (circle)))
			clutter_actor_queue_redraw (CLUTTER_ACTOR (circle));

		g_object_notify (G_OBJECT (circle), "angle-stop");
		g_object_unref (circle);
	}
}

void
clutter_circle_set_width (ClutterCircle *circle,
                                guint          width)
{
	ClutterCirclePrivate *priv;

	g_return_if_fail (CLUTTER_IS_CIRCLE (circle));
	priv = circle->priv;

	if (priv->width != width)
	{
		g_object_ref (circle);

		priv->width = width;

		if (CLUTTER_ACTOR_IS_VISIBLE (CLUTTER_ACTOR (circle)))
			clutter_actor_queue_redraw (CLUTTER_ACTOR (circle));

		g_object_notify (G_OBJECT (circle), "width");
		g_object_unref (circle);
	}
}

void
clutter_circle_set_radius (ClutterCircle *circle,
                                guint          radius)
{
	ClutterCirclePrivate *priv;

	g_return_if_fail (CLUTTER_IS_CIRCLE (circle));
	priv = circle->priv;

	if (priv->radius != radius)
	{
		g_object_ref (circle);

		priv->radius = radius;

		if (CLUTTER_ACTOR_IS_VISIBLE (CLUTTER_ACTOR (circle)))
			clutter_actor_queue_redraw (CLUTTER_ACTOR (circle));

		g_object_notify (G_OBJECT (circle), "radius");
		g_object_unref (circle);
	}
}

static void
clutter_circle_set_property (GObject      *object,
				guint         prop_id,
				const GValue *value,
				GParamSpec   *pspec)
{
  ClutterCircle *circle = CLUTTER_CIRCLE(object);

  switch (prop_id)
    {
    case PROP_COLOR:
      clutter_circle_set_color (circle, g_value_get_boxed (value));
      break;
	case PROP_ANGLE_START:
	  clutter_circle_set_angle_start (circle, g_value_get_uint (value));
	  break;
	case PROP_ANGLE_STOP:
	  clutter_circle_set_angle_stop (circle, g_value_get_uint (value));
	  break;
	case PROP_WIDTH:
	  clutter_circle_set_width (circle, g_value_get_uint (value));
	  break;
	case PROP_RADIUS:
	  clutter_circle_set_radius (circle, g_value_get_uint (value));
	  break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
clutter_circle_get_property (GObject    *object,
				guint       prop_id,
				GValue     *value,
				GParamSpec *pspec)
{
  ClutterCircle *circle = CLUTTER_CIRCLE(object);
  ClutterColor      color;

  switch (prop_id)
    {
    case PROP_COLOR:
      clutter_circle_get_color (circle, &color);
      g_value_set_boxed (value, &color);
      break;
    case PROP_ANGLE_START:
      g_value_set_uint (value, circle->priv->angle_start);
	  break;
    case PROP_ANGLE_STOP:
      g_value_set_uint (value, circle->priv->angle_stop);
	  break;
    case PROP_WIDTH:
      g_value_set_uint (value, circle->priv->width);
	  break;
    case PROP_RADIUS:
      g_value_set_uint (value, circle->priv->radius);
	  break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}


static void
clutter_circle_finalize (GObject *object)
{
  G_OBJECT_CLASS (clutter_circle_parent_class)->finalize (object);
}

static void
clutter_circle_dispose (GObject *object)
{
  G_OBJECT_CLASS (clutter_circle_parent_class)->dispose (object);
}


static void
clutter_circle_class_init (ClutterCircleClass *klass)
{
  GObjectClass        *gobject_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);

  actor_class->paint        = clutter_circle_paint;

  gobject_class->finalize     = clutter_circle_finalize;
  gobject_class->dispose      = clutter_circle_dispose;
  gobject_class->set_property = clutter_circle_set_property;
  gobject_class->get_property = clutter_circle_get_property;

  /**
   * ClutterCircle:color:
   *
   * The color of the circle.
   */
  g_object_class_install_property (gobject_class,
                                   PROP_COLOR,
                                   g_param_spec_boxed ("color",
                                                       "Color",
                                                       "The color of the circle",
                                                       CLUTTER_TYPE_COLOR,
                                                       CLUTTER_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_ANGLE_START,
                                   g_param_spec_uint ("angle-start",
                                                       "Start Angle",
                                                       "The start of angle",
													   0, G_MAXUINT,
													   0,
                                                       CLUTTER_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_ANGLE_STOP,
                                   g_param_spec_uint ("angle-stop",
                                                       "End Angle",
                                                       "The end of angle",
													   0, G_MAXUINT,
													   0,
                                                       CLUTTER_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_WIDTH,
                                   g_param_spec_uint ("width",
                                                       "Width",
                                                       "Width",
													   0, G_MAXUINT,
													   0,
                                                       CLUTTER_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_RADIUS,
                                   g_param_spec_uint ("radius",
                                                       "Radius",
                                                       "Radius",
													   0, G_MAXUINT,
													   0,
                                                       CLUTTER_PARAM_READWRITE));
#if 0
  /**
   * ClutterCircle:border-color:
   *
   * The color of the border of the circle.
   *
   * Since: 0.2
   */
  g_object_class_install_property (gobject_class,
                                   PROP_BORDER_COLOR,
                                   g_param_spec_boxed ("border-color",
                                                       "Border Color",
                                                       "The color of the border of the circle",
                                                       CLUTTER_TYPE_COLOR,
                                                       CLUTTER_PARAM_READWRITE));
  /**
   * ClutterCircle:border-width:
   *
   * The width of the border of the circle, in pixels.
   *
   * Since: 0.2
   */
  g_object_class_install_property (gobject_class,
                                   PROP_BORDER_WIDTH,
                                   g_param_spec_uint ("border-width",
                                                      "Border Width",
                                                      "The width of the border of the circle",
                                                      0, G_MAXUINT,
                                                      0,
                                                      CLUTTER_PARAM_READWRITE));
  /**
   * ClutterCircle:has-border:
   *
   * Whether the #ClutterCircle should be displayed with a border.
   *
   * Since: 0.2
   */
  g_object_class_install_property (gobject_class,
                                   PROP_HAS_BORDER,
                                   g_param_spec_boolean ("has-border",
                                                         "Has Border",
                                                         "Whether the circle should have a border",
                                                         FALSE,
                                                         CLUTTER_PARAM_READWRITE));

#endif
  g_type_class_add_private (gobject_class, sizeof (ClutterCirclePrivate));
}

static void
clutter_circle_init (ClutterCircle *self)
{
  ClutterCirclePrivate *priv;

  self->priv = priv = CLUTTER_CIRCLE_GET_PRIVATE (self);

  priv->color.red = 0xff;
  priv->color.green = 0xff;
  priv->color.blue = 0xff;
  priv->color.alpha = 0xff;

  priv->angle_start = 0;
  priv->angle_stop = 0;
  priv->width = 0;
  priv->radius = 0;
}

/**
 * clutter_circle_new:
 *
 * Creates a new #ClutterActor with a rectangular shape.
 *
 * Return value: a new #ClutterActor
 */
ClutterActor*
clutter_circle_new (void)
{
  return g_object_new (CLUTTER_TYPE_CIRCLE, NULL);
}

/**
 * clutter_circle_new_with_color:
 * @color: a #ClutterColor
 *
 * Creates a new #ClutterActor with a rectangular shape
 * and of the given @color.
 *
 * Return value: a new #ClutterActor
 */
ClutterActor *
clutter_circle_new_with_color (const ClutterColor *color)
{
  return g_object_new (CLUTTER_TYPE_CIRCLE,
		       "color", color,
		       NULL);
}

/**
 * clutter_circle_get_color:
 * @circle: a #ClutterCircle
 * @color: return location for a #ClutterColor
 *
 * Retrieves the color of @circle.
 */
void
clutter_circle_get_color (ClutterCircle *circle,
			     ClutterColor     *color)
{
  ClutterCirclePrivate *priv;

  g_return_if_fail (CLUTTER_IS_CIRCLE (circle));
  g_return_if_fail (color != NULL);

  priv = circle->priv;

  color->red = priv->color.red;
  color->green = priv->color.green;
  color->blue = priv->color.blue;
  color->alpha = priv->color.alpha;
}

/**
 * clutter_circle_set_color:
 * @circle: a #ClutterCircle
 * @color: a #ClutterColor
 *
 * Sets the color of @circle.
 */
void
clutter_circle_set_color (ClutterCircle   *circle,
			     const ClutterColor *color)
{
  ClutterCirclePrivate *priv;

  g_return_if_fail (CLUTTER_IS_CIRCLE (circle));
  g_return_if_fail (color != NULL);

  g_object_ref (circle);

  priv = circle->priv;

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

  if (CLUTTER_ACTOR_IS_VISIBLE (circle))
    clutter_actor_queue_redraw (CLUTTER_ACTOR (circle));

  g_object_notify (G_OBJECT (circle), "color");
  g_object_unref (circle);
}

