#include <stdlib.h>
#include <stdio.h> 
#include <string.h>
#include <gtk/gtk.h>
#include "connectioninfo.h"
#include "connectionlist.h"


static void connection_list_class_init (ConnectionListClass *klass);
static void connection_list_init (ConnectionList *connection_list);
static void connection_list_destroy (GtkObject *object);
static void connection_list_construct (ConnectionList *connection_list, Web100Object *web100obj);

static void connection_list_snap_update (GtkObject *object, gpointer data);
static void list_connections (ConnectionList *);
static void close_button_callback (GtkButton *, gpointer);
static void refresh_button_callback (GtkButton *, gpointer);

static Web100WidgetClass *parent_class = NULL;

enum {
  CMDLINE,
  PID,
  UID,
  LOCAL_ADDR,
  LOCAL_PORT,
  REMOTE_ADDR,
  REMOTE_PORT,
  CID,
  STATE,
  N_COLUMNS
};

#define LIST_ENTRY_LEN_MAX 64
static char *text[9];

#ifdef GTK2
GType
connection_list_get_type (void)
{
  static GType connection_list_type = 0;

  if (!connection_list_type)
  {
    static const GTypeInfo connection_list_info =
    {
      sizeof (ConnectionListClass),
      NULL,
      NULL,
      (GClassInitFunc) connection_list_class_init,
      NULL,
      NULL,
      sizeof (ConnectionList),
      0,
      (GInstanceInitFunc) connection_list_init,
    };

    connection_list_type = g_type_register_static (TYPE_WEB100_WIDGET, "ConnectionList", &connection_list_info, 0);
  }

  return connection_list_type;
}
#else
GtkType
connection_list_get_type (void)
{
  static GtkType connection_list_type = 0;

  if(!connection_list_type)
    {
      GtkTypeInfo connection_list_info =
      {
	"ConnectionList",
	sizeof (ConnectionList),
	sizeof (ConnectionListClass),
	(GtkClassInitFunc) connection_list_class_init,
	(GtkObjectInitFunc) connection_list_init,
        NULL,
        NULL
      };

      connection_list_type = gtk_type_unique (web100_widget_get_type (), &connection_list_info);
    }

  return connection_list_type;
}
#endif

static void
connection_list_class_init (ConnectionListClass *class)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;
  Web100WidgetClass *web100widget_class;

  object_class = (GtkObjectClass*) class;
  widget_class = (GtkWidgetClass*) class;
  web100widget_class = (Web100WidgetClass*) class;

#ifdef GTK2
  parent_class = g_type_class_peek_parent (class);
#else
  parent_class = gtk_type_class (TYPE_WEB100_WIDGET);
#endif

  object_class->destroy = connection_list_destroy;

  web100widget_class->snap_update = connection_list_snap_update;
}

static void
connection_list_init (ConnectionList *connection_list)
{
  int ii;

  GTK_WIDGET_SET_FLAGS (connection_list, GTK_NO_WINDOW);

#ifndef GTK2
  for (ii=0; ii<9; ii++) {
    text[ii] = malloc (LIST_ENTRY_LEN_MAX);
  }
#endif
}

GtkWidget*
connection_list_new (Web100Object *web100obj)
{ 
  ConnectionList *connlist;
#ifdef GTK2
  connlist = g_object_new (TYPE_CONNECTION_LIST, NULL);
#else
  connlist = gtk_type_new (connection_list_get_type ()); 
#endif
 
  web100_widget_set_object (WEB100_WIDGET (connlist), web100obj);
  
  connection_list_construct (connlist, web100obj);

  return GTK_WIDGET (connlist); 
}

static void
connection_list_destroy (GtkObject *object)
{ 
  ConnectionList *connlist;

  g_return_if_fail (object != NULL);
  g_return_if_fail (IS_CONNECTION_LIST (object));

  connlist = CONNECTION_LIST (object);

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
connection_list_snap_update (GtkObject *object, gpointer data)
{
  // no automatic update
}

static void
list_connections (ConnectionList *connlist)
{
  web100_agent *agent;
  struct web100_connection_spec spec;
  struct web100_connection_spec_v6 spec_v6;
  struct connection_info *conninfo;

#ifdef GTK2
  GtkListStore *liststore;
  GtkTreeIter iter;
#else
  GtkCList *clist;
  gfloat vadjust;
  int ii = 0;
#endif

  const char *states[] = { "UNKNOWN", "ESTBLSH", "SYNSENT", "SYNRECV", "FWAIT1", "FWAIT2", "TMEWAIT", "CLOSED", "CLSWAIT", "LASTACK", "LISTEN", "CLOSING" };

#ifdef GTK2
  liststore = connlist->list_store;

  gtk_list_store_clear (liststore);
#else
  clist = GTK_CLIST(connlist->clist);

  gtk_clist_freeze(GTK_CLIST(clist));

  vadjust = GTK_CLIST(clist)->vadjustment->value;

  gtk_clist_clear(GTK_CLIST(clist) );
#endif

  agent = WEB100_WIDGET (connlist)->web100object->agent;

  if ((conninfo = connection_info_head (agent)) == NULL)
    web100_perror ("conection_info_head"); 

  while (conninfo) {
    if (connection_info_get_addrtype (conninfo) == WEB100_ADDRTYPE_IPV4)
      connection_info_get_spec (conninfo, &spec);
    if (connection_info_get_addrtype (conninfo) == WEB100_ADDRTYPE_IPV6)
      connection_info_get_spec_v6 (conninfo, &spec_v6);

#ifdef GTK2
    gtk_list_store_append (liststore, &iter);

    gtk_list_store_set (liststore, &iter, CMDLINE, connection_info_get_cmdline (conninfo), -1);

    gtk_list_store_set (liststore, &iter, PID, connection_info_get_pid (conninfo), -1);

    gtk_list_store_set (liststore, &iter, UID, connection_info_get_uid (conninfo), -1);

    if (connection_info_get_addrtype (conninfo) == WEB100_ADDRTYPE_IPV4) {
      gtk_list_store_set (liststore, &iter, LOCAL_ADDR, web100_value_to_text(WEB100_TYPE_INET_ADDRESS_IPV4, &spec.src_addr), -1);
      gtk_list_store_set (liststore, &iter, LOCAL_PORT, web100_value_to_text(WEB100_TYPE_INET_PORT_NUMBER, &spec.src_port), -1);
    }
    if (connection_info_get_addrtype (conninfo) == WEB100_ADDRTYPE_IPV6) { 
      gtk_list_store_set (liststore, &iter, LOCAL_ADDR, web100_value_to_text(WEB100_TYPE_INET_ADDRESS_IPV6, &spec_v6.src_addr), -1); 
      gtk_list_store_set (liststore, &iter, LOCAL_PORT, web100_value_to_text(WEB100_TYPE_INET_PORT_NUMBER, &spec_v6.src_port), -1);
    }

    if (connection_info_get_addrtype (conninfo) == WEB100_ADDRTYPE_IPV4) {
      gtk_list_store_set (liststore, &iter, REMOTE_ADDR, web100_value_to_text(WEB100_TYPE_INET_ADDRESS_IPV4, &spec.dst_addr), -1);
      gtk_list_store_set (liststore, &iter, REMOTE_PORT, web100_value_to_text(WEB100_TYPE_INET_PORT_NUMBER, &spec.dst_port), -1);
    }
    if (connection_info_get_addrtype (conninfo) == WEB100_ADDRTYPE_IPV6) {
      gtk_list_store_set (liststore, &iter, REMOTE_ADDR, web100_value_to_text(WEB100_TYPE_INET_ADDRESS_IPV6, &spec_v6.dst_addr), -1);
      gtk_list_store_set (liststore, &iter, REMOTE_PORT, web100_value_to_text(WEB100_TYPE_INET_PORT_NUMBER, &spec_v6.dst_port), -1);
    }

    gtk_list_store_set (liststore, &iter, CID, connection_info_get_cid (conninfo), -1);

    gtk_list_store_set (liststore, &iter, STATE, states[connection_info_get_state(conninfo)], -1);

#else
    strncpy(text[0], connection_info_get_cmdline(conninfo), LIST_ENTRY_LEN_MAX); 

    sprintf(text[1], "%d", connection_info_get_pid(conninfo)); 

    sprintf(text[2], "%d", connection_info_get_uid(conninfo)); 

    if(connection_info_get_addrtype(conninfo) == WEB100_ADDRTYPE_IPV4) {
      connection_info_get_spec(conninfo, &spec);
      strcpy(text[3], web100_value_to_text(WEB100_TYPE_INET_ADDRESS_IPV4, &spec.src_addr));
      strcpy(text[4], web100_value_to_text(WEB100_TYPE_INET_PORT_NUMBER, &spec.src_port));
      strcpy(text[5], web100_value_to_text(WEB100_TYPE_INET_ADDRESS_IPV4, &spec.dst_addr));
      strcpy(text[6], web100_value_to_text(WEB100_TYPE_INET_PORT_NUMBER, &spec.dst_port));
    }

    if(connection_info_get_addrtype(conninfo) == WEB100_ADDRTYPE_IPV6) {
      connection_info_get_spec_v6(conninfo, &spec_v6);
      strcpy(text[3], web100_value_to_text(WEB100_TYPE_INET_ADDRESS_IPV6, &spec_v6.src_addr));
      strcpy(text[4], web100_value_to_text(WEB100_TYPE_INET_PORT_NUMBER, &spec_v6.src_port));
      strcpy(text[5], web100_value_to_text(WEB100_TYPE_INET_ADDRESS_IPV6, &spec_v6.dst_addr));
      strcpy(text[6], web100_value_to_text(WEB100_TYPE_INET_PORT_NUMBER, &spec_v6.dst_port));
    }

    sprintf(text[7], "%d", connection_info_get_cid(conninfo)); 

    strcpy(text[8], states[connection_info_get_state(conninfo)]); 

    gtk_clist_insert (GTK_CLIST (clist), ii, text);
    gtk_clist_set_row_data (GTK_CLIST (clist), ii, GINT_TO_POINTER(connection_info_get_cid(conninfo)));
    ii++;
#endif

    conninfo = connection_info_next (conninfo); 
  } 

#ifndef GTK2
  gtk_adjustment_set_value( GTK_ADJUSTMENT(GTK_CLIST(clist)->vadjustment), vadjust);

  gtk_clist_thaw(GTK_CLIST(clist));
#endif
}

static void
close_button_callback (GtkButton *button, gpointer data)
{
  ConnectionList *connlist = CONNECTION_LIST (data);
  GtkWidget *window;

  window = gtk_widget_get_toplevel (GTK_WIDGET (connlist));

  if (window) gtk_widget_destroy (window);
}

static void
refresh_button_callback (GtkButton *button, gpointer data)
{
  ConnectionList *connlist = CONNECTION_LIST (data);

  list_connections (connlist);
}

static void
connection_list_construct (ConnectionList *connlist, Web100Object *web100obj)
{
  GtkWidget *scrolledwindow, *hbox, *close_button, *refresh_button;
#ifdef GTK2 
  GtkTreeViewColumn *column; 
  GtkCellRenderer   *cell_renderer;
#else
  gchar *titles[9] = { "cmdline", "pid", "uid", "local add", "local port", "remote add", "remote port", "cid", "state" };
#endif

  scrolledwindow = gtk_scrolled_window_new (NULL, NULL); 

  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

  gtk_box_pack_start (GTK_BOX (connlist), scrolledwindow, TRUE, TRUE, 0);
  gtk_widget_show (scrolledwindow);

#ifdef GTK2
  connlist->list_store = gtk_list_store_new (N_COLUMNS, G_TYPE_STRING, G_TYPE_INT, G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT, G_TYPE_STRING);

  connlist->view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (connlist->list_store));
  g_object_unref (connlist->list_store); // since view now holds a reference,
                                         // remove the (local, non-floating)
                                         // g_object ref

  cell_renderer = gtk_cell_renderer_text_new ();

  column = gtk_tree_view_column_new_with_attributes ("Cmdline", cell_renderer, "text", CMDLINE, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (connlist->view), column);

  column = gtk_tree_view_column_new_with_attributes ("PID", cell_renderer, "text", PID, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (connlist->view), column);

  column = gtk_tree_view_column_new_with_attributes ("UID", cell_renderer, "text", UID, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (connlist->view), column);

  column = gtk_tree_view_column_new_with_attributes ("Local addr", cell_renderer, "text", LOCAL_ADDR, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (connlist->view), column);

  column = gtk_tree_view_column_new_with_attributes ("Port", cell_renderer, "text", LOCAL_PORT, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (connlist->view), column);

  column = gtk_tree_view_column_new_with_attributes ("Remote addr", cell_renderer, "text", REMOTE_ADDR, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (connlist->view), column);

  column = gtk_tree_view_column_new_with_attributes ("Port", cell_renderer, "text", REMOTE_PORT, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (connlist->view), column);

  column = gtk_tree_view_column_new_with_attributes ("CID", cell_renderer, "text", CID, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (connlist->view), column);

  column = gtk_tree_view_column_new_with_attributes ("State", cell_renderer, "text", STATE, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (connlist->view), column);

  gtk_container_add (GTK_CONTAINER (scrolledwindow), connlist->view);

  gtk_widget_show (connlist->view); 

#else 
  connlist->clist = GTK_CLIST (gtk_clist_new_with_titles(9, titles));

  gtk_widget_set_usize (GTK_WIDGET (connlist->clist), -1, 250);

  gtk_clist_column_titles_passive (GTK_CLIST (connlist->clist)); 
  gtk_clist_set_column_auto_resize (GTK_CLIST (connlist->clist), 0, TRUE);
  gtk_clist_set_column_auto_resize (GTK_CLIST (connlist->clist), 1, TRUE);
  gtk_clist_set_column_auto_resize (GTK_CLIST (connlist->clist), 2, TRUE);
  gtk_clist_set_column_auto_resize (GTK_CLIST (connlist->clist), 3, TRUE);
  gtk_clist_set_column_auto_resize (GTK_CLIST (connlist->clist), 4, TRUE);
  gtk_clist_set_column_auto_resize (GTK_CLIST (connlist->clist), 5, TRUE);
  gtk_clist_set_column_auto_resize (GTK_CLIST (connlist->clist), 6, TRUE);
  gtk_clist_set_column_auto_resize (GTK_CLIST (connlist->clist), 7, TRUE);
  gtk_clist_set_column_auto_resize (GTK_CLIST (connlist->clist), 8, TRUE);

  gtk_clist_set_button_actions (connlist->clist, 2, GTK_BUTTON_SELECTS);

  gtk_container_add(GTK_CONTAINER(scrolledwindow), GTK_WIDGET (connlist->clist));

  gtk_widget_show(GTK_WIDGET (connlist->clist));
#endif  

  list_connections (connlist);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_end (GTK_BOX (connlist), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);
 
#ifdef GTK2
  close_button = gtk_button_new_with_mnemonic ("_Close"); 

  g_signal_connect (G_OBJECT (close_button), "clicked", G_CALLBACK (close_button_callback), connlist);
#else
  close_button = gtk_button_new_with_label ("Close");

  gtk_signal_connect (GTK_OBJECT (close_button), "clicked", GTK_SIGNAL_FUNC (close_button_callback), connlist);
#endif

  gtk_box_pack_start (GTK_BOX (hbox), close_button, FALSE, FALSE, 2);
  gtk_widget_show (close_button);

#ifdef GTK2
  refresh_button = gtk_button_new_with_mnemonic ("_Refresh"); 

  g_signal_connect (G_OBJECT (refresh_button), "clicked", G_CALLBACK (refresh_button_callback), connlist);
#else
  refresh_button = gtk_button_new_with_label ("Refresh"); 

  gtk_signal_connect (GTK_OBJECT (refresh_button), "clicked", GTK_SIGNAL_FUNC (refresh_button_callback), connlist);
#endif

  gtk_box_pack_end (GTK_BOX (hbox), refresh_button, FALSE, FALSE, 0);
  gtk_widget_show (refresh_button);
}
