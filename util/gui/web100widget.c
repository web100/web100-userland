#include <stdlib.h>
#include <stdio.h> 
#include <gtk/gtk.h>

#include "web100widget.h"


enum {
  OBJECT_CHANGED,
  LAST_SIGNAL
}; 

static void web100_widget_class_init (Web100WidgetClass *klass);
static void web100_widget_init (Web100Widget *web100widget);
static void web100_widget_destroy (GtkObject *object);

static GtkVBoxClass *parent_class = NULL;
static guint web100widget_signals[LAST_SIGNAL] = { 0 };

#ifdef GTK2
GType
web100_widget_get_type (void)
{
  static GType web100_widget_type = 0;

  if (!web100_widget_type)
  {
    static const GTypeInfo web100_widget_info =
    {
      sizeof (Web100WidgetClass),
      NULL,
      NULL,
      (GClassInitFunc) web100_widget_class_init,
      NULL,
      NULL,
      sizeof (Web100Widget),
      0,
      (GInstanceInitFunc) web100_widget_init,
    };

    web100_widget_type = g_type_register_static (GTK_TYPE_VBOX, "Web100Widget", &web100_widget_info, 0); 
  }

  return web100_widget_type;
}
#else
GtkType
web100_widget_get_type (void)
{
  static GtkType web100_widget_type = 0;

  if(!web100_widget_type)
    {
      GtkTypeInfo web100_widget_info =
      {
	"Web100Widget",
	sizeof (Web100Widget),
	sizeof (Web100WidgetClass),
	(GtkClassInitFunc) web100_widget_class_init,
	(GtkObjectInitFunc) web100_widget_init,
        NULL,
        NULL
      };

      web100_widget_type = gtk_type_unique (gtk_vbox_get_type (), &web100_widget_info);
    }

  return web100_widget_type;
}
#endif

static void
web100_widget_class_init (Web100WidgetClass *class)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;

  object_class = (GtkObjectClass*) class;
  widget_class = (GtkWidgetClass*) class;

#ifdef GTK2
  parent_class = g_type_class_peek_parent (class);
#else
  parent_class = gtk_type_class (GTK_TYPE_VBOX);
#endif

#ifdef GTK2
  web100widget_signals[OBJECT_CHANGED] =
    g_signal_new ("object_changed",
       	G_TYPE_FROM_CLASS(object_class),
       	G_SIGNAL_RUN_FIRST | G_SIGNAL_NO_RECURSE,
       	G_STRUCT_OFFSET(Web100WidgetClass, object_changed),
       	NULL,
       	NULL,
       	g_cclosure_marshal_VOID__VOID,
       	G_TYPE_NONE, 0, NULL);
#else
  web100widget_signals[OBJECT_CHANGED] =
    gtk_signal_new ("object_changed",
       	GTK_RUN_FIRST | GTK_RUN_NO_RECURSE, 
	object_class->type,
       	GTK_SIGNAL_OFFSET (Web100WidgetClass, object_changed),
       	gtk_marshal_NONE__NONE,
       	GTK_TYPE_NONE, 0); 

  gtk_object_class_add_signals (object_class, web100widget_signals, LAST_SIGNAL);
#endif

  object_class->destroy = web100_widget_destroy;

  class->connection_closed = web100_widget_connection_closed;
  class->connection_init = NULL;
  class->snap_update = NULL;

  class->object_changed = NULL;

  class->set_object = web100_widget_set_object;
}

static void
web100_widget_init (Web100Widget *web100widget)
{
  GTK_WIDGET_SET_FLAGS (web100widget, GTK_NO_WINDOW);

  web100widget->web100object = NULL; 

  web100widget->widgets = NULL;

  web100widget->update_widget = NULL;
  web100widget->update_toggle = 0;
}

void
web100_widget_set_object (Web100Widget *web100widget, Web100Object *web100object)
{
  Web100WidgetClass *WW;

  g_return_if_fail (web100widget != NULL);
  g_return_if_fail (IS_WEB100_WIDGET (web100widget)); 
  g_return_if_fail (web100object != NULL);
  g_return_if_fail (IS_WEB100_OBJECT (web100object)); 

  WW = WEB100_WIDGET_GET_CLASS (web100widget);

  if (web100widget->web100object) { 
#ifdef GTK2 
    g_object_unref (G_OBJECT (web100widget->web100object));
#else 
    gtk_object_unref (GTK_OBJECT (web100widget->web100object));
#endif
  }

  web100widget->web100object = web100object; 

#ifdef GTK2
  g_object_ref (G_OBJECT (web100object));

  if (WW->connection_closed != NULL)
    g_signal_connect (G_OBJECT (web100object), "connection_closed", G_CALLBACK (WW->connection_closed), web100widget);

  if (WW->connection_init != NULL)
    g_signal_connect (G_OBJECT (web100object), "connection_init", G_CALLBACK (WW->connection_init), web100widget);

  if (WW->snap_update != NULL)
    g_signal_connect (G_OBJECT (web100object), "snap_update", G_CALLBACK (WW->snap_update), (gpointer) web100widget); 
#else
  gtk_object_ref (GTK_OBJECT (web100object));

  if (WW->connection_closed != NULL)
    gtk_signal_connect (GTK_OBJECT (web100object), "connection_closed", GTK_SIGNAL_FUNC (WW->connection_closed), web100widget);

  if (WW->connection_init != NULL)
    gtk_signal_connect (GTK_OBJECT (web100object), "connection_init", GTK_SIGNAL_FUNC (WW->connection_init), web100widget);

  if (WW->snap_update != NULL)
    gtk_signal_connect (GTK_OBJECT (web100object), "snap_update", GTK_SIGNAL_FUNC (WW->snap_update), web100widget);
#endif

  gtk_object_sink (GTK_OBJECT (web100object)); // since web100obj is a
                                               // GtkObject, not a GObject
#ifdef GTK2
  g_signal_emit_by_name (GTK_OBJECT (web100widget), "object_changed");
#else
  gtk_signal_emit_by_name (GTK_OBJECT (web100widget), "object_changed");
#endif
}

void
web100_widget_set_object_from_widget (Web100Widget *dst, Web100Widget *src)
{
  g_return_if_fail (dst != NULL);
  g_return_if_fail (IS_WEB100_WIDGET (dst));

  g_return_if_fail (src != NULL);
  g_return_if_fail (IS_WEB100_WIDGET (src));

  WEB100_WIDGET_GET_CLASS (dst)->set_object (dst, web100_widget_get_object (src));
}

void
web100_widget_set_object_from_widget_reversed (Web100Widget *src, Web100Widget *dst)
{
  web100_widget_set_object_from_widget (dst, src); 
}

Web100Object*
web100_widget_get_object (Web100Widget *web100widget)
{
  g_return_val_if_fail (web100widget != NULL, NULL);
  g_return_val_if_fail (IS_WEB100_WIDGET (web100widget), NULL);

  return web100widget->web100object;
}

void
web100_widget_share_object (Web100Widget *dst, Web100Widget *src)
{
  g_return_if_fail (dst != NULL);
  g_return_if_fail (IS_WEB100_WIDGET (dst));
  g_return_if_fail (src != NULL);
  g_return_if_fail (IS_WEB100_WIDGET (src));

#ifdef GTK2
  g_signal_connect_object (src, "object_changed", G_CALLBACK (web100_widget_set_object_from_widget_reversed), dst, 0);
#else
  gtk_signal_connect_while_alive (GTK_OBJECT (src), "object_changed", GTK_SIGNAL_FUNC (web100_widget_set_object_from_widget_reversed), dst, GTK_OBJECT (dst));
#endif
}

void
web100_widget_connection_closed (GtkObject *object, gpointer data)
{
  Web100Widget *ww = WEB100_WIDGET (data);
  Web100WidgetClass *WW = WEB100_WIDGET_GET_CLASS (ww);

#ifdef GTK2
    if (WW->snap_update != NULL) {
      g_signal_handlers_disconnect_by_func (G_OBJECT (ww->web100object), G_CALLBACK (WW->snap_update), ww); 

      WW->snap_update = NULL;
    }
#else
    if (WW->snap_update != NULL) {
      gtk_signal_disconnect_by_func (GTK_OBJECT (ww->web100object), GTK_SIGNAL_FUNC (WW->snap_update), (gpointer) ww);

      WW->snap_update = NULL;
    }
#endif
}

static void
web100_widget_destroy (GtkObject *object)
{
  Web100Widget *ww;
  Web100WidgetClass *WW;

  g_return_if_fail (object != NULL);
  g_return_if_fail (IS_WEB100_WIDGET (object));

  ww = WEB100_WIDGET (object);
  WW = WEB100_WIDGET_GET_CLASS (ww);

  if (ww->web100object != NULL) {
#ifdef GTK2
    g_signal_handlers_disconnect_matched (ww->web100object, G_SIGNAL_MATCH_DATA, 0, 0, 0, 0, ww);

    g_object_unref (G_OBJECT (ww->web100object)); 
#else 
    gtk_signal_disconnect_by_data (GTK_OBJECT (ww->web100object), ww);

    gtk_object_unref (GTK_OBJECT (ww->web100object));
#endif

    ww->web100object = NULL; 
  }

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}
