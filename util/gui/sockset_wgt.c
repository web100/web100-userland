
#include <stdlib.h>
#include <stdio.h> 
#include <gtk/gtk.h>

#include "web100.h"
#include "web100sync.h"
#include "web100obj.h" 
#include "cnlst_wgt.h"
#include "sockset_wgt.h"


#define SOCKSET_DEFAULT_SIZE 300

enum {
  WEB100OBJ_CHANGED,
  LAST_SIGNAL
};

static void sockset_class_init (SocksetClass *class);
static void sockset_init (Sockset *sockset);
static void sockset_destroy (GtkObject *object);
static void sockset_set_connection_info (Sockset *sockset, Web100Obj *web100obj);
static void show_socklist (GtkWidget*, gpointer);
static void select_socket(GtkWidget *parentclist, gint row, gint column,
                   GdkEventAny *event, gpointer data);

static guint sockset_signals[LAST_SIGNAL] = { 0 };

static GtkVBoxClass *parent_class = NULL;

//static GtkWidget *sessentry, *cidentry;

GtkType sockset_get_type ()
{
  static guint sockset_type = 0;

  if(!sockset_type)
    {
      GtkTypeInfo sockset_info =
      {
	"Sockset",
	sizeof (Sockset),
	sizeof (SocksetClass),
	(GtkClassInitFunc) sockset_class_init,
	(GtkObjectInitFunc) sockset_init,
        (GtkArgSetFunc) NULL,
        (GtkArgGetFunc) NULL
      };

      sockset_type = gtk_type_unique (gtk_vbox_get_type (), &sockset_info);
    }

  return sockset_type;
}

static void sockset_class_init (SocksetClass *class)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;

  object_class = (GtkObjectClass*) class;
  widget_class = (GtkWidgetClass*) class;

  parent_class = gtk_type_class (gtk_vbox_get_type ());

  object_class->destroy = sockset_destroy;

  sockset_signals[WEB100OBJ_CHANGED] =
    gtk_signal_new ("web100obj_changed",
                    GTK_RUN_FIRST | GTK_RUN_NO_RECURSE,
                    object_class->type,
                    GTK_SIGNAL_OFFSET (SocksetClass, web100obj_changed),
                    gtk_marshal_NONE__NONE,
                    GTK_TYPE_NONE, 0);

  gtk_object_class_add_signals (object_class, sockset_signals, LAST_SIGNAL);

  class->web100obj_changed = NULL;
}

static void sockset_init (Sockset *sockset)
{ 
  GTK_WIDGET_SET_FLAGS (sockset, GTK_NO_WINDOW);
  sockset->web100obj = NULL;
}
  
static void sockset_construct (Sockset *sockset,
                               Web100Obj *web100obj,
                               gboolean malleable)
{
  GtkWidget *hbox, *align, *frame, *button;

  align = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_box_pack_start (GTK_BOX (sockset), align, FALSE, FALSE, 0);
  gtk_widget_show (align);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (align), hbox);
  gtk_widget_show (hbox);

  frame = gtk_frame_new ("TCP session name");
//  gtk_container_add (GTK_CONTAINER (align), frame);
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  sockset->sessentry = gtk_entry_new();
  gtk_widget_set_usize(sockset->sessentry, 200, 20);
//  gtk_signal_connect(GTK_OBJECT(sessentry), "activate",
//                     GTK_SIGNAL_FUNC(sessentry_callback), NULL);
  gtk_container_add(GTK_CONTAINER(frame), sockset->sessentry);
  gtk_widget_show(sockset->sessentry);

  frame = gtk_frame_new ("CID");
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  sockset->cidentry = gtk_entry_new();
  gtk_widget_set_usize(sockset->cidentry, 30, 20);
//  gtk_signal_connect(GTK_OBJECT(cidentry), "activate",
//                     GTK_SIGNAL_FUNC(cidentry_callback), NULL);
  gtk_container_add(GTK_CONTAINER(frame), sockset->cidentry);
  gtk_widget_show(sockset->cidentry);

  align = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_box_pack_start (GTK_BOX (sockset), align, FALSE, FALSE, 5);
  gtk_widget_show (align);

  if (malleable) {
    button = gtk_button_new_with_label("Select TCP session");
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
                       GTK_SIGNAL_FUNC(show_socklist), (gpointer) sockset);
    gtk_container_add (GTK_CONTAINER (align), button);
    gtk_widget_show(button);
  }

  sockset_set_web100obj (sockset, web100obj); 
}

GtkWidget* sockset_new (Web100Obj *web100obj, gboolean malleable)
{
  Sockset *sockset = gtk_type_new (sockset_get_type ()); 

  sockset_construct (sockset, web100obj, malleable);

  return GTK_WIDGET (sockset); 
}

void sockset_update (GtkObject *object, gpointer *data)
{
// do something
}

static void sockset_destroy (GtkObject *object)
{
  g_return_if_fail (object != NULL);
  g_return_if_fail (IS_SOCKSET(object));

  if (SOCKSET (object)->web100obj != NULL) { 
//    gtk_signal_disconnect_by_data (GTK_OBJECT (SOCKSET (object)->web100obj),
//                                   (gpointer) object);
    gtk_object_unref (GTK_OBJECT (SOCKSET (object)->web100obj));
  }

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

void sockset_changed (Sockset *sockset)
{
  g_return_if_fail (sockset != NULL);
  g_return_if_fail (IS_SOCKSET (sockset));

  gtk_signal_emit (GTK_OBJECT (sockset), sockset_signals[WEB100OBJ_CHANGED]);
}

void sockset_set_web100obj (Sockset *sockset, Web100Obj *web100obj)
{
  Web100Obj *old_web100obj;

  g_return_if_fail (sockset != NULL);
  g_return_if_fail (IS_SOCKSET (sockset)); 

  if (!web100obj) { 
    web100obj = (Web100Obj *) web100obj_new (NULL, NULL);
  }
  else
    g_return_if_fail (IS_WEB100_OBJ (web100obj));

  if (sockset->web100obj) { 
//    gtk_signal_disconnect_by_data (GTK_OBJECT (sockset->web100obj),
//                                   (gpointer) sockset);
    gtk_object_unref (GTK_OBJECT (sockset->web100obj));
  }
  
  sockset->web100obj = web100obj; 
  gtk_object_ref (GTK_OBJECT (web100obj)); 
  gtk_object_sink (GTK_OBJECT (web100obj)); 

  if (web100obj->connection)
    sockset_set_connection_info (sockset, web100obj);

//  sockset_changed (sockset);
/*
  gtk_signal_connect (GTK_OBJECT(web100obj),
                      "changed",
                      GTK_SIGNAL_FUNC(sockset_web100obj_changed),
                      sockset);

  gtk_signal_connect (GTK_OBJECT(web100obj),
                      "snap_update",
                      GTK_SIGNAL_FUNC(sockset_web100obj_snap_update),
                      sockset);
*/
}

Web100Obj* sockset_get_web100obj (Sockset *sockset)
{
  g_return_val_if_fail (sockset != NULL, NULL);
  g_return_val_if_fail (IS_SOCKSET (sockset), NULL);

  return sockset->web100obj;
}

static void show_socklist (GtkWidget *button, gpointer data)
{
  GtkWidget *window;
  char titlebar[65];
  char hostname[45];

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  strcpy(titlebar, "TCP sockets@");
  gethostname(hostname, (size_t) 45);
  strcat(titlebar, hostname);
  gtk_window_set_title (GTK_WINDOW (window), titlebar);

  gtk_signal_connect (GTK_OBJECT (window), "destroy",
                      GTK_SIGNAL_FUNC (gtk_widget_destroy), window);

  SOCKSET (data)->cnlst = CNLST (cnlst_new (sockset_get_web100obj(SOCKSET(data))));
  gtk_container_add (GTK_CONTAINER (window),
                     GTK_WIDGET (SOCKSET (data)->cnlst));
  gtk_widget_show (GTK_WIDGET (SOCKSET (data)->cnlst));

  gtk_signal_connect (GTK_OBJECT (SOCKSET (data)->cnlst->clist), "select-row",
                      GTK_SIGNAL_FUNC (select_socket), data);

  gtk_widget_show (window); 
}

static void select_socket(GtkWidget *parentclist, gint row, gint column,
                GdkEventAny *event, gpointer data)
{
  Sockset *sockset;
  Web100Obj *web100obj = NULL;
  web100_connection *conn;
  int cid;
  char ascid[64];
  GList *objects;

  sockset = SOCKSET (data);

  cid = GPOINTER_TO_INT(gtk_clist_get_row_data (GTK_CLIST(parentclist), row));
  sprintf(ascid, "%d", cid);

// check if the object already exists: 

  for (objects = sockset->web100obj->web100sync->objects;
      objects != NULL;
      objects = g_list_next (objects)) { 
        if (WEB100_OBJ (objects->data)->cid == atoi(ascid)) { 
	  web100obj = WEB100_OBJ (objects->data); 
	  break;
	}
  } 
  if (!web100obj)
    web100obj = WEB100_OBJ (web100obj_new (sockset->web100obj->web100sync, ascid));

  sockset_set_web100obj (sockset, web100obj);
  sockset_changed (sockset);
}

static void sockset_set_connection_info (Sockset *sockset, Web100Obj *web100obj)
{ 
  unsigned char *src, *dst;
  char ascid[32], ftuple[128]; 

  sprintf (ascid, "%d", web100obj->cid);
  gtk_entry_set_text(GTK_ENTRY(sockset->cidentry), ascid);

  src = (unsigned char *)&(web100obj->spec).src_addr;
  dst = (unsigned char *)&(web100obj->spec).dst_addr;

  sprintf (ftuple, "%u.%u.%u.%u:%u %u.%u.%u.%u:%u",
                 src[0], src[1], src[2], src[3], (web100obj->spec).src_port,
		 dst[0], dst[1], dst[2], dst[3], (web100obj->spec).dst_port);

  gtk_entry_set_text (GTK_ENTRY (sockset->sessentry), ftuple);
		 
}








