#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <math.h>
#ifdef HAVE_MALLOC_H
# include <malloc.h>
#endif
#include <gtk/gtk.h>
#include "web100object.h"
#include "web100poll.h"


static void web100_poll_class_init (Web100PollClass *class);
static void web100_poll_init (Web100Poll *web100poll);
static void web100_poll_destroy (GtkObject *object);
static void web100_poll_construct (Web100Poll *web100poll, struct web100_agent *agent);

static guint web100_poll_update (gpointer data);
static void timeout_callback (GtkAdjustment *adjustment, Web100Poll *web100poll);

static GtkObjectClass *parent_class = NULL;


#ifdef GTK2
GType
web100_poll_get_type (void)
{
  static GType web100_poll_type = 0;

  if (!web100_poll_type)
  {
    static const GTypeInfo web100_poll_info =
    {
      sizeof (Web100PollClass),
      NULL,
      NULL,
      (GClassInitFunc) web100_poll_class_init,
      NULL,
      NULL,
      sizeof (Web100Poll),
      0,
      (GInstanceInitFunc) web100_poll_init,
    };

    web100_poll_type = g_type_register_static (GTK_TYPE_OBJECT, "Web100Poll", &web100_poll_info, 0); 
  }

  return web100_poll_type;
}
#else
GtkType
web100_poll_get_type ()
{
  static guint web100_poll_type = 0;

  if(!web100_poll_type)
    {
      GtkTypeInfo web100_poll_info =
      {
	"Web100Poll",
	sizeof (Web100Poll),
	sizeof (Web100PollClass),
	(GtkClassInitFunc) web100_poll_class_init,
	(GtkObjectInitFunc) web100_poll_init,
        NULL,
        NULL
      };

      web100_poll_type = gtk_type_unique (GTK_TYPE_OBJECT, &web100_poll_info);
    }

  return web100_poll_type;
}
#endif

static void web100_poll_class_init (Web100PollClass *class)
{
  GtkObjectClass *object_class;

  object_class = (GtkObjectClass*) class;

#ifdef GTK2
    parent_class = g_type_class_peek_parent (class);
#else 
    parent_class = gtk_type_class (gtk_object_get_type ());
#endif

  object_class->destroy = web100_poll_destroy;
}

static void web100_poll_init (Web100Poll *web100poll)
{ 
  web100poll->objects = NULL;

  web100poll->adjustment = GTK_ADJUSTMENT (gtk_adjustment_new (1.0,0.1,3.1,0.1,0.1,0.1));

#ifdef GTK2
  g_object_ref (G_OBJECT (web100poll->adjustment));
#else
  gtk_object_ref (GTK_OBJECT (web100poll->adjustment));
#endif
  gtk_object_sink (GTK_OBJECT (web100poll->adjustment));
} 

GtkObject* web100_poll_new (web100_agent *agent)
{ 
  Web100Poll *web100poll = gtk_type_new (web100_poll_get_type ()); 

  if (!agent)
    if ((agent = web100_attach(WEB100_AGENT_TYPE_LOCAL, NULL)) == NULL) {
      web100_perror("web100_attach");
      fprintf (stderr, "Is this a web100 kernel?\n");
      return NULL;
    } 

  web100poll->agent = agent; 
  web100poll->timeout_id = gtk_timeout_add (1000, (GtkFunction) web100_poll_update, web100poll);

#ifdef GTK2
  g_signal_connect (G_OBJECT (web100poll->adjustment), "value-changed", G_CALLBACK (timeout_callback), web100poll);
#else
  gtk_signal_connect (GTK_OBJECT (web100poll->adjustment), "value-changed", GTK_SIGNAL_FUNC (timeout_callback), web100poll); 
#endif
  web100_poll_update (web100poll);

  return GTK_OBJECT (web100poll); 
}

void web100_poll_refresh (gpointer data, gpointer user_data)
{
  web100_object_refresh (WEB100_OBJECT (data)); 
}

static guint web100_poll_update (gpointer data)
{
  GList *objects;
  Web100Object *web100obj;
  static int kk=0;

  g_list_foreach (WEB100_POLL (data)->objects, &web100_poll_refresh, NULL);

  return TRUE;
}

static void
timeout_callback (GtkAdjustment *adjustment, Web100Poll *wpoll)
{
  int timeout;
  gdouble value;

#ifdef GTK2
  value = gtk_adjustment_get_value (adjustment);
#else
  value = adjustment->value;
#endif

  timeout = floor (1000*value); 

  gtk_timeout_remove (wpoll->timeout_id);

  wpoll->timeout_id = gtk_timeout_add (timeout, (GtkFunction) web100_poll_update, wpoll);
}

static void web100_poll_destroy (GtkObject *object)
{ 
  g_return_if_fail (object != NULL);
  g_return_if_fail (IS_WEB100_POLL (object));

  if (WEB100_POLL (object)->adjustment) {
#ifdef GTK2
    g_object_unref (G_OBJECT (WEB100_POLL (object)->adjustment));
#else
    gtk_object_unref (GTK_OBJECT (WEB100_POLL (object)->adjustment));
#endif
    WEB100_POLL (object)->adjustment = NULL;
  }

  if (WEB100_POLL (object)->agent) {
      web100_detach (WEB100_POLL (object)->agent);
      WEB100_POLL (object)->agent = NULL;
  }

  GTK_OBJECT_CLASS (parent_class)->destroy (object);
} 
