#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#ifdef HAVE_MALLOC_H
# include <malloc.h>
#endif
#include <gtk/gtk.h> 
#include "web100.h" 
#include "web100obj.h"
#include "web100sync.h"

enum { 
  LAST_SIGNAL
};

static void web100sync_class_init (Web100SyncClass *class);
static void web100sync_init (Web100Sync *web100sync);
static void web100sync_construct (Web100Sync *web100sync,
                                 struct web100_agent *agent);
static guint web100sync_update (gpointer data);
static void web100sync_destroy (GtkObject *object);

static guint web100sync_signals[LAST_SIGNAL] = { 0 };

static GtkObjectClass *parent_class = NULL;


GtkType web100sync_get_type ()
{
  static guint web100sync_type = 0;

  if(!web100sync_type)
    {
      GtkTypeInfo web100sync_info =
      {
	"Web100Sync",
	sizeof (Web100Sync),
	sizeof (Web100SyncClass),
	(GtkClassInitFunc) web100sync_class_init,
	(GtkObjectInitFunc) web100sync_init,
        (GtkArgSetFunc) NULL,
        (GtkArgGetFunc) NULL
      };

      web100sync_type = gtk_type_unique (gtk_object_get_type (),
                                         &web100sync_info);
    }

  return web100sync_type;
}

static void web100sync_class_init (Web100SyncClass *class)
{
  GtkObjectClass *object_class;

  object_class = (GtkObjectClass*) class;

  parent_class = gtk_type_class (gtk_object_get_type ());

  object_class->destroy = web100sync_destroy;
  gtk_object_class_add_signals (object_class, web100sync_signals, LAST_SIGNAL);
}

static void web100sync_init (Web100Sync *web100sync)
{ 
  web100sync->objects = NULL; //

  web100sync->adjustment = GTK_ADJUSTMENT (gtk_adjustment_new (1.0,0.0,10.0,0.1,0.1,0.1));
} 

GtkObject* web100sync_new (web100_agent *agent)
{ 
  Web100Sync *web100sync = gtk_type_new (web100sync_get_type ()); 

  if (!agent)
    if ((agent = web100_attach(WEB100_AGENT_TYPE_LOCAL, NULL)) == NULL) {
      web100_perror("web100_attach");
      fprintf (stderr, "Is this a web100 kernel?\n");
      return NULL;
    } 

  web100sync->agent = agent; 
  gtk_timeout_add (1000, (GtkFunction) web100sync_update, web100sync);
  web100sync_update (web100sync);

  return GTK_OBJECT (web100sync); 
}

void web100sync_refresh (gpointer data, gpointer user_data)
{
  web100obj_refresh (WEB100_OBJ (data)); 
}
// rename _timeout
static guint web100sync_update (gpointer data)
{
  GList *objects;
  Web100Obj *web100obj;
  static int kk=0;

  g_list_foreach (WEB100_SYNC (data)->objects, &web100sync_refresh, NULL);

  return TRUE;
}

static void web100sync_destroy (GtkObject *object)
{ 
  g_return_if_fail (object != NULL);
  g_return_if_fail (IS_WEB100_SYNC (object));

  web100_detach (WEB100_SYNC (object)->agent);

  GTK_OBJECT_CLASS (parent_class)->destroy (object);
} 
