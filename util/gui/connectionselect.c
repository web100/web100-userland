#include <stdlib.h>
#include <stdio.h> 
#include <gtk/gtk.h>
#include "connectionselect.h"


static void connection_select_class_init (ConnectionSelectClass *klass);
static void connection_select_init (ConnectionSelect *connection_select);
static void connection_select_destroy (GtkObject *object);
static void connection_select_construct (ConnectionSelect *connection_select, Web100Object *web100obj, int selectable);

static void connection_select_snap_update (GtkObject *object, gpointer data);
static void connection_select_set_object (Web100Widget *widget, Web100Object *object);
static void connection_select_connection_closed (GtkObject *object, gpointer data);
static void connection_select_connection_init (GtkObject *object, gpointer data);
static void connection_select_set_addr_entry (ConnectionSelect *);
static void connection_select_set_cid_entry  (ConnectionSelect *); 
#ifdef GTK2
static void connection_list_select_callback (GtkTreeSelection *, ConnectionSelect *); 
#else
static void connection_list_select_callback (GtkCList *clist, gint row, gint column, GdkEventButton *event, ConnectionSelect *);
#endif
static gint connection_select_close_callback (GtkWidget *, GdkEventButton *, gpointer); 
static void select_button_callback (GtkButton *, gpointer);
static void clear_button_callback (GtkButton *, gpointer);
static void cs_cid_entry_cb (GtkEntry *, gpointer);

static Web100WidgetClass *parent_class = NULL;


#ifdef GTK2
GType
connection_select_get_type (void)
{
  static GType connection_select_type = 0;

  if (!connection_select_type)
  {
    static const GTypeInfo connection_select_info =
    {
      sizeof (ConnectionSelectClass),
      NULL,
      NULL,
      (GClassInitFunc) connection_select_class_init,
      NULL,
      NULL,
      sizeof (ConnectionSelect),
      0,
      (GInstanceInitFunc) connection_select_init,
    };

    connection_select_type = g_type_register_static (TYPE_WEB100_WIDGET, "ConnectionSelect", &connection_select_info, 0);
  }

  return connection_select_type;
}
#else
GtkType
connection_select_get_type (void)
{
  static GtkType connection_select_type = 0;

  if(!connection_select_type)
    {
      GtkTypeInfo connection_select_info =
      {
	"ConnectionSelect",
	sizeof (ConnectionSelect),
	sizeof (ConnectionSelectClass),
	(GtkClassInitFunc) connection_select_class_init,
	(GtkObjectInitFunc) connection_select_init,
        NULL,
        NULL
      };

      connection_select_type = gtk_type_unique (web100_widget_get_type (), &connection_select_info);
    }

  return connection_select_type;
}
#endif

static void
connection_select_class_init (ConnectionSelectClass *class)
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

  object_class->destroy = connection_select_destroy;

  web100widget_class->connection_init = connection_select_connection_init;
  web100widget_class->connection_closed = connection_select_connection_closed;
  web100widget_class->set_object = connection_select_set_object;
}

static void
connection_select_init (ConnectionSelect *connselect)
{
  GTK_WIDGET_SET_FLAGS (connselect, GTK_NO_WINDOW);

  connselect->connection_list = NULL;
}

GtkWidget*
connection_select_new (Web100Object *web100obj, int selectable)
{ 
  ConnectionSelect *connection_select;
#ifdef GTK2
  connection_select = g_object_new (TYPE_CONNECTION_SELECT, NULL);
#else
  connection_select = gtk_type_new (connection_select_get_type ()); 
#endif
 
  web100_widget_set_object (WEB100_WIDGET (connection_select), web100obj);

  connection_select_construct (connection_select, web100obj, selectable);

  connection_select_connection_init (NULL, connection_select);

  connection_select->selectable = selectable;

  return GTK_WIDGET (connection_select); 
}

void connection_select_set_addr_entry (ConnectionSelect *connselect)
{
  Web100Object *web100obj = WEB100_WIDGET (connselect)->web100object;
  char buf[128];

  if (web100obj->addrtype) { 
    if (web100obj->addrtype == WEB100_ADDRTYPE_IPV4) {
      strncpy (buf, web100_value_to_text (WEB100_TYPE_INET_ADDRESS_IPV4, &web100obj->spec.dst_addr), 128);
      strcat (buf, ": ");
      strcat (buf, web100_value_to_text (WEB100_TYPE_INET_PORT_NUMBER, &web100obj->spec.dst_port)); 
    }
    if (web100obj->addrtype == WEB100_ADDRTYPE_IPV6) {
      strncpy (buf, web100_value_to_text (WEB100_TYPE_INET_ADDRESS_IPV6, &web100obj->spec_v6.dst_addr), 128);
      strcat (buf, ". ");
      strcat (buf, web100_value_to_text (WEB100_TYPE_INET_PORT_NUMBER, &web100obj->spec_v6.dst_port)); 
    }

    gtk_entry_set_text (connselect->addr_entry, buf);
  }
  else
    gtk_entry_set_text (connselect->addr_entry, "");
}

void connection_select_set_cid_entry (ConnectionSelect *connselect)
{
  Web100Object *web100obj = WEB100_WIDGET (connselect)->web100object;
  char strcid[64];

  g_return_if_fail (connselect != NULL);
  g_return_if_fail (IS_CONNECTION_SELECT (connselect));

  if (web100obj->cid >= 0) {
    snprintf (strcid, 64, "%d", web100obj->cid); 
    gtk_entry_set_text (connselect->cid_entry, strcid); 
  }
  else gtk_entry_set_text (connselect->cid_entry, ""); 
}

static void
connection_select_destroy (GtkObject *object)
{ 
  g_return_if_fail (object != NULL);
  g_return_if_fail (IS_CONNECTION_SELECT (object));

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
connection_select_set_object (Web100Widget *widget, Web100Object *object)
{
  web100_widget_set_object (widget, object);

  connection_select_connection_init (NULL, widget);
}

static void
connection_select_connection_init (GtkObject *object, gpointer data)
{
  connection_select_set_cid_entry (CONNECTION_SELECT (data));

  connection_select_set_addr_entry (CONNECTION_SELECT (data));
}

static void
connection_select_connection_closed (GtkObject *object, gpointer data)
{
  ConnectionSelect *cs = CONNECTION_SELECT (data);

  web100_widget_connection_closed (object, data);

  if (!cs->selectable)
    gtk_widget_set_sensitive (GTK_WIDGET (data), FALSE);
}

#ifdef GTK2
static void
connection_list_select_callback (GtkTreeSelection *listselect, ConnectionSelect *connselect)
{
  Web100Object *object = NULL; 
  GList *objects;
  GtkTreeModel *model;
  GtkTreeIter iter; 
  int cid;

  g_return_if_fail (listselect != NULL);
  g_return_if_fail (GTK_IS_TREE_SELECTION (listselect));
  g_return_if_fail (connselect != NULL);
  g_return_if_fail (IS_CONNECTION_SELECT (connselect));

  if (!gtk_tree_selection_get_selected (listselect, &model, &iter))
    return;
  
  gtk_tree_model_get (model, &iter, 7, &cid, -1);

  if (WEB100_WIDGET (connselect)->web100object->cid == -1) {
    object = WEB100_WIDGET (connselect)->web100object;
    if ((web100_object_set_connection (object, cid)) != 0)
      web100_perror ("wubba\n");
  }
  else 
    for (objects = WEB100_WIDGET (connselect)->web100object->web100poll->objects; objects != NULL; objects = g_list_next (objects)) { 
      if (WEB100_OBJECT (objects->data)->cid == cid) { 
	object = WEB100_OBJECT (objects->data); 
	break;
      }
    }

  if (!object)
    object = WEB100_OBJECT (web100_object_new (WEB100_WIDGET (connselect)->web100object->web100poll, cid));

  connection_select_set_object (WEB100_WIDGET (connselect), object); 
}
#else
static void
connection_list_select_callback (GtkCList *clist, gint row, gint column, GdkEventButton *event, ConnectionSelect *connselect)
{
  Web100Object *object = NULL; 
  GList *objects;
  int cid;

  g_return_if_fail (clist != NULL);
  g_return_if_fail (GTK_IS_CLIST (clist));
  g_return_if_fail (connselect != NULL);
  g_return_if_fail (IS_CONNECTION_SELECT (connselect));

  cid = GPOINTER_TO_INT (gtk_clist_get_row_data (GTK_CLIST (clist), row));

  if (WEB100_WIDGET (connselect)->web100object->cid == -1) {
    object = WEB100_WIDGET (connselect)->web100object;
    if ((web100_object_set_connection (object, cid)) != 0)
      return;
  }
  else 
    for (objects = WEB100_WIDGET (connselect)->web100object->web100poll->objects; objects != NULL; objects = g_list_next (objects)) { 
      if (WEB100_OBJECT (objects->data)->cid == cid) { 
	object = WEB100_OBJECT (objects->data); 
	break;
      }
    }

  if (!object)
    object = WEB100_OBJECT (web100_object_new (WEB100_WIDGET (connselect)->web100object->web100poll, cid));

  connection_select_set_object (WEB100_WIDGET (connselect), object); 
}
#endif

static gint
connection_select_close_callback (GtkWidget *widget, GdkEventButton *event, gpointer data)
{ 
  if ((event->type==GDK_2BUTTON_PRESS ||
       event->type==GDK_3BUTTON_PRESS) ) { 
    gtk_widget_destroy (GTK_WIDGET (data)); 
  }

  return FALSE;
}

static void
select_button_callback (GtkButton *button, gpointer data)
{
  GtkWidget *window;
  ConnectionSelect *connselect;
  ConnectionList   *connlist;
#ifdef GTK2
  GtkTreeSelection *listselect;
#else
#endif
  static char hostname[64];
  size_t len;
  char titlebar[128];

  connselect = CONNECTION_SELECT (data);

  connlist = CONNECTION_LIST (connection_list_new (WEB100_WIDGET(connselect)->web100object));

  connselect->connection_list = connlist;

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gethostname (hostname, len);
  strcpy (titlebar, "Connection list");
  strcat (titlebar, "@");
  strncat (titlebar, hostname, 64);
  gtk_window_set_title (GTK_WINDOW (window), titlebar);

  gtk_window_set_default_size (GTK_WINDOW (window), 400, 200);

  gtk_container_add (GTK_CONTAINER (window), GTK_WIDGET (connselect->connection_list));
  gtk_widget_show (GTK_WIDGET (connselect->connection_list));

  gtk_widget_show (window); 

#ifdef GTK2
  listselect = gtk_tree_view_get_selection (GTK_TREE_VIEW (connlist->view));

  gtk_tree_selection_set_mode (listselect, GTK_SELECTION_SINGLE);
  gtk_tree_selection_unselect_all (listselect);

  g_signal_connect (G_OBJECT (listselect), "changed", G_CALLBACK (connection_list_select_callback), connselect);

  g_signal_connect (G_OBJECT (connlist->view), "button-press-event", G_CALLBACK (connection_select_close_callback), window);
#else
  gtk_signal_connect (GTK_OBJECT (connlist->clist), "select-row", GTK_SIGNAL_FUNC (connection_list_select_callback), connselect);
  gtk_signal_connect (GTK_OBJECT (connlist), "button-press-event", GTK_SIGNAL_FUNC (connection_select_close_callback), window);
#endif
}

static void clear_button_callback (GtkButton *button, gpointer data)
{
  ConnectionSelect *cs = CONNECTION_SELECT (data);
  Web100Poll *wp;
  Web100Object *wo;
  
  wp = WEB100_WIDGET (data)->web100object->web100poll;
  wo = WEB100_OBJECT (web100_object_new (wp, -1));

  connection_select_set_object (WEB100_WIDGET (data), wo);
}

static void
cs_cid_entry_cb (GtkEntry *entry, gpointer data)
{
  int cid;
  web100_connection *cp;
  Web100Object *object;
  Web100Widget *widget = WEB100_WIDGET (data);

  cid = atoi (gtk_entry_get_text (entry));

  if (cp = web100_connection_lookup (widget->web100object->agent, cid)) {
    object = WEB100_OBJECT (web100_object_new (widget->web100object->web100poll, cid));
    connection_select_set_object (widget, object);
  }
}

static void
connection_select_construct (ConnectionSelect *connselect, Web100Object *web100obj, int selectable)
{
  GtkWidget *hbox, *vbox, *frame;
  GtkWidget *select_button, *clear_button;

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (connselect), hbox);
  gtk_widget_show (hbox);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 20);
  gtk_widget_show (vbox);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
  gtk_widget_show (hbox);

  frame = gtk_frame_new ("IP address"); 
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);
  
  connselect->addr_entry = GTK_ENTRY (gtk_entry_new());
#ifdef GTK2 
  gtk_editable_set_editable (GTK_EDITABLE (connselect->addr_entry), FALSE); 
  gtk_widget_set_size_request (GTK_WIDGET (connselect->addr_entry), 200, -1);
#else 
  gtk_entry_set_editable (connselect->addr_entry, FALSE); 
  gtk_widget_set_usize(GTK_WIDGET (connselect->addr_entry), 200, 20);
#endif 
  gtk_container_add(GTK_CONTAINER(frame), GTK_WIDGET (connselect->addr_entry));
  gtk_widget_show(GTK_WIDGET (connselect->addr_entry));

  frame = gtk_frame_new ("CID");
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  connselect->cid_entry = GTK_ENTRY (gtk_entry_new());
#ifdef GTK2
  g_signal_connect (G_OBJECT (connselect->cid_entry), "activate", G_CALLBACK (cs_cid_entry_cb), connselect);

  if (!selectable)
    gtk_editable_set_editable (GTK_EDITABLE (connselect->cid_entry), FALSE); 
  gtk_widget_set_size_request (GTK_WIDGET (connselect->cid_entry), 30, -1);
#else
  gtk_signal_connect (GTK_OBJECT (connselect->cid_entry), "activate", GTK_SIGNAL_FUNC (cs_cid_entry_cb), connselect);

  if (!selectable)
    gtk_entry_set_editable (connselect->cid_entry, FALSE); 
  gtk_widget_set_usize(GTK_WIDGET (connselect->cid_entry), 30, 20);
#endif 
  gtk_container_add(GTK_CONTAINER(frame), GTK_WIDGET (connselect->cid_entry));
  gtk_widget_show(GTK_WIDGET (connselect->cid_entry));

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  if (selectable) {
#ifdef GTK2
    select_button = gtk_button_new_with_mnemonic ("_Select");

    g_signal_connect (G_OBJECT (select_button), "clicked", G_CALLBACK (select_button_callback), connselect);
#else
    select_button = gtk_button_new_with_label ("Select");

    gtk_signal_connect (GTK_OBJECT (select_button), "clicked", GTK_SIGNAL_FUNC (select_button_callback), connselect);
#endif 
    gtk_box_pack_start (GTK_BOX (hbox), select_button, FALSE, FALSE, 0);
    gtk_widget_show (select_button); 
#ifdef GTK2
    clear_button = gtk_button_new_with_mnemonic ("_Clear");

    g_signal_connect(G_OBJECT (clear_button), "clicked", G_CALLBACK (clear_button_callback), connselect);
#else
    clear_button = gtk_button_new_with_label ("Clear");

    gtk_signal_connect (GTK_OBJECT (clear_button), "clicked", GTK_SIGNAL_FUNC (clear_button_callback), connselect);
#endif
    gtk_box_pack_end (GTK_BOX (hbox), clear_button, FALSE, FALSE, 0);
    gtk_widget_show (clear_button);
  }
}
























