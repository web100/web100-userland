#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <malloc.h>
#include <gtk/gtk.h>

#include "web100-int.h"
#include "web100.h" 
#include "web100sync.h"
#include "web100obj.h"

enum { 
  CONNECTION_CHANGED, // XXX
  SNAP_UPDATE,
  LAST_SIGNAL
};

static void web100obj_class_init (Web100ObjClass *class);
static void web100obj_init (Web100Obj *web100obj);
static void web100obj_construct (Web100Obj *web100obj,
                                 struct web100_agent *agent);
void destroy_notify (gpointer data);
//static void web100obj_set_connection (Web100Obj *web100obj, int cid);
//static void web100obj_refresh (Web100Obj *web100obj);
static void web100obj_destroy (GtkObject *object);

static guint web100obj_signals[LAST_SIGNAL] = { 0 };

static GtkObjectClass *parent_class = NULL;


GtkType web100obj_get_type ()
{
  static guint web100obj_type = 0;

  if(!web100obj_type)
    {
      GtkTypeInfo web100obj_info =
      {
	"Web100Obj",
	sizeof (Web100Obj),
	sizeof (Web100ObjClass),
	(GtkClassInitFunc) web100obj_class_init,
	(GtkObjectInitFunc) web100obj_init,
        (GtkArgSetFunc) NULL,
        (GtkArgGetFunc) NULL
      };

      web100obj_type = gtk_type_unique (gtk_object_get_type (),
                                         &web100obj_info);
    }

  return web100obj_type;
}

static void web100obj_class_init (Web100ObjClass *class)
{
  GtkObjectClass *object_class;

  object_class = (GtkObjectClass*) class;

  parent_class = gtk_type_class (gtk_object_get_type ());

  object_class->destroy = web100obj_destroy;

  web100obj_signals[CONNECTION_CHANGED] =
    gtk_signal_new ("connection_changed",
                    GTK_RUN_FIRST | GTK_RUN_NO_RECURSE,
                    object_class->type,
                    GTK_SIGNAL_OFFSET (Web100ObjClass, connection_changed),
                    gtk_marshal_NONE__NONE,
                    GTK_TYPE_NONE, 0); 
  web100obj_signals[SNAP_UPDATE] = 
    gtk_signal_new ("snap_update",
                    GTK_RUN_FIRST | GTK_RUN_NO_RECURSE,
                    object_class->type,
                    GTK_SIGNAL_OFFSET (Web100ObjClass, snap_update),
                    gtk_marshal_NONE__NONE,
                    GTK_TYPE_NONE, 0);

  gtk_object_class_add_signals (object_class, web100obj_signals, LAST_SIGNAL);

  class->connection_changed = NULL;
  class->snap_update = NULL; 
}

static void web100obj_init (Web100Obj *web100obj)
{ 

}

GtkObject* web100obj_new (Web100Sync *web100sync, char *ascid)
{ 
  Web100Obj *web100obj = gtk_type_new (web100obj_get_type ());
  int cid;

  web100obj_construct (web100obj, web100sync->agent);

  web100obj->web100sync = web100sync;
  gtk_object_ref (GTK_OBJECT (web100sync));
  gtk_object_sink (GTK_OBJECT (web100sync));

  if (ascid) { 
    cid = atoi (ascid);
    web100obj_set_connection (web100obj, cid);

    web100sync->objects = g_list_prepend (web100sync->objects, web100obj);
    gtk_object_weakref (GTK_OBJECT (web100obj), &destroy_notify, web100obj);
//    printf("length = %d\n", g_list_length (web100sync->objects));
  }
  return GTK_OBJECT (web100obj); 
}

static void web100obj_construct (Web100Obj *web100obj,
                          struct web100_agent *agent)
{ 
  web100_group *gp;
  struct snapshot_data *snap;

  if ((gp = web100_group_head(agent)) == NULL &&
      web100_errno != WEB100_ERR_SUCCESS) {
    web100_perror("web100_group_head");
    return;
  } 

  while (gp) { 
    snap = malloc (sizeof(struct snapshot_data));

    snap->group = gp;
    strcpy(snap->name, gp->name);

    snap->next = web100obj->snapshot_head;
    web100obj->snapshot_head = snap;

    if ((gp = web100_group_next(gp)) == NULL &&
       	web100_errno != WEB100_ERR_SUCCESS) {
      web100_perror("web100_group_next");
      return;
    }
  } 

  web100obj->agent = agent; 
} 

void web100obj_set_connection (Web100Obj *web100obj, int cid)
{ 
  web100_group *gp; 
  web100_connection *connection;
  struct snapshot_data *snap;

  if (web100obj->connection) return; 

  if ((connection = web100_connection_lookup (web100obj->agent, cid)) == NULL) {
    web100_perror ("web100obj_connection_lookup"); 
    return;
  }

// make a local copy, as connection pointers are ephemeral at the
// library level
  if ((web100obj->connection = (web100_connection *) malloc (sizeof(web100_connection))) == NULL) {
    fprintf(stderr, "Out of memory\n");
    exit(1);
  }

  web100_connection_data_copy (web100obj->connection, connection);

//redundancy for convenience:
  web100obj->cid = cid;
  web100_get_connection_spec(web100obj->connection, &web100obj->spec); 

  snap = web100obj->snapshot_head; 
  while (snap) { 
    snap->last = web100_snapshot_alloc (snap->group, web100obj->connection);
    snap->prior = web100_snapshot_alloc (snap->group, web100obj->connection);
    snap->set = web100_snapshot_alloc (snap->group, web100obj->connection); 

    snap = snap->next;
  }

  web100obj_refresh (web100obj); 
}

void web100obj_refresh (Web100Obj *web100obj)
{
  struct snapshot_data *snap; 

  g_return_if_fail (web100obj != NULL);
  g_return_if_fail (IS_WEB100_OBJ (web100obj));

  if (!web100obj->connection) return;

  snap = web100obj->snapshot_head;
  while (snap) { 
    web100_snap_data_copy (snap->prior, snap->last);

    if ((web100_snap ((snap->last)) < 0)) { 
      if (web100_errno == WEB100_ERR_NOCONNECTION) { 
      }
      else
       	web100_perror ("web100obj_refresh");

      return;
    } 
    snap = snap->next;
  }

  web100obj_snap_update (web100obj); 
}

void web100obj_connection_changed (Web100Obj *web100obj)
{
  g_return_if_fail (web100obj != NULL);
  g_return_if_fail (IS_WEB100_OBJ (web100obj));

  gtk_signal_emit (GTK_OBJECT (web100obj),
                   web100obj_signals[CONNECTION_CHANGED]);
}

void web100obj_snap_update (Web100Obj *web100obj)
{ 
  g_return_if_fail (web100obj != NULL);
  g_return_if_fail (IS_WEB100_OBJ (web100obj));

  gtk_signal_emit (GTK_OBJECT (web100obj), web100obj_signals[SNAP_UPDATE]); 
} 

void destroy_notify (gpointer data)
{
  Web100Sync *web100sync;
  Web100Obj *web100obj;
  
  web100obj = WEB100_OBJ (data);
  web100sync = web100obj->web100sync;

  web100sync->objects = g_list_remove (web100sync->objects, web100obj);
//  printf("length = %d\n", g_list_length (web100sync->objects));
}

static void web100obj_destroy (GtkObject *object)
{ 
  struct snapshot_data *snap;

  g_return_if_fail (object != NULL);
  g_return_if_fail (IS_WEB100_OBJ (object));

  if (WEB100_OBJ(object)->connection) {
    free (WEB100_OBJ(object)->connection);

    snap = WEB100_OBJ (object)->snapshot_head;
    while (snap) {
      web100_snapshot_free (snap->last);
      web100_snapshot_free (snap->prior);
      web100_snapshot_free (snap->set);
      //    web100_snapshot_free (snap->alt); 
      snap = snap->next;
    } 
  }

  GTK_OBJECT_CLASS (parent_class)->destroy (object);
} 

//utility functions;

// compare web100objects by connection info
gint web100obj_compar (web100_connection *aa, web100_connection *bb)
{ 
  g_return_if_fail (aa != NULL); 
  g_return_if_fail (bb != NULL);

  if (aa->agent == bb->agent &&
      aa->cid == bb->cid &&
      aa->spec.dst_port == bb->spec.dst_port &&
      aa->spec.dst_addr == bb->spec.dst_addr &&
      aa->spec.src_port == bb->spec.src_port &&
      aa->spec.src_addr == bb->spec.src_addr)
    return 0; 

  return 1;
}

void sprinttype(char *text, int type, void *buf)
{
  switch(type) {
    case WEB100_TYPE_IP_ADDRESS:
      {
        unsigned char *addr = (unsigned char *) buf; 
        sprintf(text, "%u.%u.%u.%u", addr[0], addr[1], addr[2], addr[3]);
        break;
      }
    case WEB100_TYPE_INTEGER:
    case WEB100_TYPE_INTEGER32:
    case WEB100_TYPE_COUNTER32: 
    case WEB100_TYPE_GAUGE32: 
    case WEB100_TYPE_UNSIGNED32:
    case WEB100_TYPE_TIME_TICKS:
      sprintf(text, "%lu", *(u_int32_t *) buf);
      break;
    case WEB100_TYPE_COUNTER64:
      sprintf(text, "%llu", *(u_int64_t *) buf);
      break;
    case WEB100_TYPE_UNSIGNED16:
      sprintf(text, "%u", *(u_int16_t *) buf);
      break;
    default:
      sprintf(text, "%s", "unknown type");
  }
}

