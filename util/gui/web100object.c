#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#ifdef HAVE_MALLOC_H
# include <malloc.h>
#endif
#include <gtk/gtk.h> 
#include "web100.h" 
#include "web100poll.h"
#include "web100object.h"
#include "connectioninfo.h"

enum { 
  CONNECTION_INIT,
  CONNECTION_CLOSED,
  SNAP_UPDATE,
  LAST_SIGNAL
};

static void web100_object_class_init (Web100ObjectClass *class);
static void web100_object_init (Web100Object *web100_object);
static void web100_object_construct (Web100Object *web100_object, struct web100_agent *agent);
static void destroy_notify (gpointer data);
static void web100_object_destroy (GtkObject *object);

static guint web100_object_signals[LAST_SIGNAL] = { 0 };

static GtkObjectClass *parent_class = NULL;


#ifdef GTK2
GType
web100_object_get_type (void)
{
  static GType web100_object_type = 0;

  if (!web100_object_type)
  {
    static const GTypeInfo web100_object_info =
    {
      sizeof (Web100ObjectClass),
      NULL,
      NULL,
      (GClassInitFunc) web100_object_class_init,
      NULL,
      NULL,
      sizeof (Web100Object),
      0,
      (GInstanceInitFunc) web100_object_init,
    };

    web100_object_type = g_type_register_static (GTK_TYPE_OBJECT, "Web100Object", &web100_object_info, 0); 
  }

  return web100_object_type;
}
#else
GtkType
web100_object_get_type (void)
{
  static GtkType web100_object_type = 0;

  if(!web100_object_type)
    {
      static const GtkTypeInfo web100_object_info =
      {
	"Web100Object",
	sizeof (Web100Object),
	sizeof (Web100ObjectClass),
	(GtkClassInitFunc) web100_object_class_init,
	(GtkObjectInitFunc) web100_object_init,
        NULL,
        NULL,
	(GtkClassInitFunc) NULL,
      };

      web100_object_type = gtk_type_unique (GTK_TYPE_OBJECT, &web100_object_info);
    }

  return web100_object_type;
}
#endif

static void
web100_object_class_init (Web100ObjectClass *class)
{
  GtkObjectClass *object_class;

  object_class = (GtkObjectClass*) class;

#ifdef GTK2
  parent_class = g_type_class_peek_parent (class);
#else
  parent_class = gtk_type_class (gtk_object_get_type ());
#endif

  object_class->destroy = web100_object_destroy;

  class->connection_closed = NULL;
  class->snap_update = NULL;

#ifdef GTK2
  web100_object_signals[CONNECTION_INIT] =
    g_signal_new ("connection_init",
       	G_TYPE_FROM_CLASS(object_class),
       	G_SIGNAL_RUN_FIRST,
       	G_STRUCT_OFFSET(Web100ObjectClass, connection_init),
       	NULL,
       	NULL,
       	g_cclosure_marshal_VOID__VOID,
       	G_TYPE_NONE, 0, NULL);

  web100_object_signals[CONNECTION_CLOSED] =
    g_signal_new ("connection_closed",
       	G_TYPE_FROM_CLASS(object_class),
       	G_SIGNAL_RUN_FIRST,
       	G_STRUCT_OFFSET(Web100ObjectClass, connection_closed),
       	NULL,
       	NULL,
       	g_cclosure_marshal_VOID__VOID,
       	G_TYPE_NONE, 0, NULL);

  web100_object_signals[SNAP_UPDATE] =
    g_signal_new ("snap_update",
       	G_TYPE_FROM_CLASS(object_class),
       	G_SIGNAL_RUN_FIRST, 
	G_STRUCT_OFFSET(Web100ObjectClass, snap_update),
       	NULL,
       	NULL,
       	g_cclosure_marshal_VOID__VOID, 
	G_TYPE_NONE, 0, NULL);
#else
  web100_object_signals[CONNECTION_INIT] =
    gtk_signal_new ("connection_init",
       	GTK_RUN_FIRST | GTK_RUN_NO_RECURSE, 
	object_class->type,
       	GTK_SIGNAL_OFFSET (Web100ObjectClass, connection_init),
       	gtk_marshal_NONE__NONE,
       	GTK_TYPE_NONE, 0); 

  web100_object_signals[CONNECTION_CLOSED] =
    gtk_signal_new ("connection_closed",
       	GTK_RUN_FIRST | GTK_RUN_NO_RECURSE, 
	object_class->type,
       	GTK_SIGNAL_OFFSET (Web100ObjectClass, connection_closed),
       	gtk_marshal_NONE__NONE,
       	GTK_TYPE_NONE, 0); 

  web100_object_signals[SNAP_UPDATE] = 
    gtk_signal_new ("snap_update",
       	GTK_RUN_FIRST | GTK_RUN_NO_RECURSE, 
	object_class->type, 
	GTK_SIGNAL_OFFSET (Web100ObjectClass, snap_update),
       	gtk_marshal_NONE__NONE,
       	GTK_TYPE_NONE, 0);

  gtk_object_class_add_signals (object_class, web100_object_signals, LAST_SIGNAL);
#endif
}

static void web100_object_init (Web100Object *web100object)
{ 
  struct snapshot_list *snap;

  web100object->cid = WEB100_OBJECT_CONNECTION_TYPE_NONE;
  web100object->addrtype = WEB100_ADDRTYPE_UNKNOWN; 
  web100object->snapshot_head = NULL;
  web100object->widgets = NULL;
}

GtkObject* web100_object_new (Web100Poll *wp, int cid)
{ 
  Web100Object *web100_object = gtk_type_new (web100_object_get_type ());

  web100_object_construct (web100_object, wp->agent);

  web100_object->web100poll = wp;
  gtk_object_ref (GTK_OBJECT (wp));
  gtk_object_sink (GTK_OBJECT (wp));

  if (cid >= 0 ) {
    web100_object_set_connection (web100_object, cid); 
  }
  return GTK_OBJECT (web100_object); 
}

static void
web100_object_construct (Web100Object *web100_object, struct web100_agent *agent)
{ 
  web100_group *gp;
  struct snapshot_list *snap;

  gp = web100_group_head(agent);
  web100_object->snapshot_head = NULL;
  while (gp) { 
    snap = malloc (sizeof(struct snapshot_list));

    snap->group = gp;
    strcpy(snap->name, web100_get_group_name(gp));
    snap->last = NULL;
    snap->prior = NULL;
    snap->set = NULL;

    snap->next = web100_object->snapshot_head;
    web100_object->snapshot_head = snap;

    gp = web100_group_next(gp);
  } 

  web100_object->agent = agent; 
} 

int
web100_object_set_connection (Web100Object *web100obj, int cid)
{ 
  struct connection_info *ci;
  web100_connection *cp; 
  struct snapshot_list *snap;

  if (web100obj->cid != WEB100_OBJECT_CONNECTION_TYPE_NONE) return;
                                    // a connection may only be set once
                                    // per object

  if ((cp = web100_connection_lookup (web100obj->agent, cid)) == NULL) {
    web100_perror ("web100_object_connection_lookup"); 
    return web100_errno;
  }

  web100obj->cid = cid; 
  web100obj->addrtype = web100_get_connection_addrtype (cp);

  if (web100obj->addrtype == WEB100_ADDRTYPE_IPV4)
    web100_get_connection_spec (cp, &web100obj->spec);
  if (web100obj->addrtype == WEB100_ADDRTYPE_IPV6)
    web100_get_connection_spec_v6 (cp, &web100obj->spec_v6);

  snap = web100obj->snapshot_head; 

  web100obj->web100poll->objects = g_list_prepend (web100obj->web100poll->objects, web100obj);
  gtk_object_weakref (GTK_OBJECT (web100obj), &destroy_notify, web100obj);

  web100_object_connection_init (web100obj);

  return WEB100_ERR_SUCCESS;
}

void web100_object_refresh (Web100Object *web100_object)
{
  struct snapshot_list *snap; 
  web100_snapshot *tmp;
  web100_connection *cp;

  g_return_if_fail (web100_object != NULL);
  g_return_if_fail (IS_WEB100_OBJECT (web100_object));

  if (web100_object->cid == WEB100_OBJECT_CONNECTION_TYPE_NONE) return;

  if ((cp = web100_connection_lookup (web100_object->agent, web100_object->cid)) == NULL) {
    web100_object_connection_closed (web100_object);
    return;
  }

  snap = web100_object->snapshot_head;
  while (snap) { 
    tmp = snap->prior; 
    snap->prior = snap->last;
    snap->last = web100_snapshot_alloc (snap->group, cp);
    if (tmp) 
      free (tmp); 

    if (web100_snap (snap->last) != 0) { 
	web100_object_connection_closed (web100_object);
	return;
    } 
    snap = snap->next;
  }

  web100_object_snap_update (web100_object); 
}

void web100_object_connection_init (Web100Object *web100_object)
{
  g_return_if_fail (web100_object != NULL);
  g_return_if_fail (IS_WEB100_OBJECT (web100_object));

#ifdef GTK2
  g_signal_emit_by_name (web100_object, "connection_init");
#else 
  gtk_signal_emit_by_name (GTK_OBJECT (web100_object), "connection_init");
#endif
}

void web100_object_connection_closed (Web100Object *web100_object)
{
  g_return_if_fail (web100_object != NULL);
  g_return_if_fail (IS_WEB100_OBJECT (web100_object));

#ifdef GTK2
  g_signal_emit_by_name (web100_object, "connection_closed");
#else 
  gtk_signal_emit_by_name (GTK_OBJECT (web100_object), "connection_closed");
#endif
}

void web100_object_snap_update (Web100Object *web100_object)
{ 
  g_return_if_fail (web100_object != NULL);
  g_return_if_fail (IS_WEB100_OBJECT (web100_object));
  
#ifdef GTK2
  g_signal_emit_by_name (web100_object, "snap_update");
#else 
  gtk_signal_emit_by_name (GTK_OBJECT (web100_object), "snap_update");
#endif
} 

static void destroy_notify (gpointer data)
{
  Web100Poll *wp;
  Web100Object *web100_object;

  web100_object = WEB100_OBJECT (data);
  wp = web100_object->web100poll;

  wp->objects = g_list_remove (wp->objects, web100_object);
}

static void web100_object_destroy (GtkObject *object)
{ 
  struct snapshot_list *snap;

  g_return_if_fail (object != NULL);
  g_return_if_fail (IS_WEB100_OBJECT (object));

  snap = WEB100_OBJECT (object)->snapshot_head;
  while (snap) {
    if (snap->last) web100_snapshot_free (snap->last);
    if (snap->prior) web100_snapshot_free (snap->prior);
    if (snap->set) web100_snapshot_free (snap->set);

    snap = snap->next;
  } 

  GTK_OBJECT_CLASS (parent_class)->destroy (object);
} 

