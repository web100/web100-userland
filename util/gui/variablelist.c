#include <stdlib.h>
#include <stdio.h> 
#include <string.h>
#include <gtk/gtk.h> 
#include "web100.h"
#include "variablelist.h"
#include "utillaunch.h"


enum {
  VARNAME,
  VALUE,
  DELTA,
  RESET,
  N_COLUMNS
};

#define VARNAME_ARRAY_SIZE_INIT 32

static void variable_list_class_init (VariableListClass *klass);
static void variable_list_init (VariableList *variable_list);
static void variable_list_destroy (GtkObject *object);
static void variable_list_snap_update (GtkObject *object, gpointer data); 
static void variable_list_connection_closed (GtkObject *object, gpointer data);
static void list_values (VariableList *variable_list);
#ifdef GTK2
static void list_select_callback (GtkTreeSelection *, gpointer);
#else
static void list_select_callback (GtkCList *clist, gint row, gint column, GdkEventButton *event, VariableList *varlist);
#endif
static gint display_launch_double_click (GtkWidget *widget, GdkEventButton *event, gpointer data);
static gint display_launch_right_click (GtkWidget *widget, GdkEventButton *event, gpointer data);
static void variable_list_construct (VariableList *variable_list, Web100Object *web100obj);


static Web100WidgetClass *parent_class = NULL;


#ifdef GTK2
GType
variable_list_get_type (void)
{
  static GType variable_list_type = 0;

  if (!variable_list_type)
  {
    static const GTypeInfo variable_list_info =
    {
      sizeof (VariableListClass),
      NULL,
      NULL,
      (GClassInitFunc) variable_list_class_init,
      NULL,
      NULL,
      sizeof (VariableList),
      0,
      (GInstanceInitFunc) variable_list_init,
    };

    variable_list_type = g_type_register_static (TYPE_WEB100_WIDGET, "VariableList", &variable_list_info, 0);
  }

  return variable_list_type;
}
#else
GtkType
variable_list_get_type (void)
{
  static GtkType variable_list_type = 0;

  if(!variable_list_type)
    {
      GtkTypeInfo variable_list_info =
      {
	"VariableList",
	sizeof (VariableList),
	sizeof (VariableListClass),
	(GtkClassInitFunc) variable_list_class_init,
	(GtkObjectInitFunc) variable_list_init,
        NULL,
        NULL
      };

      variable_list_type = gtk_type_unique (web100_widget_get_type (), &variable_list_info);
    }

  return variable_list_type;
}
#endif

static void
variable_list_class_init (VariableListClass *class)
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

  object_class->destroy = variable_list_destroy;

  web100widget_class->snap_update = variable_list_snap_update;
  web100widget_class->connection_closed = variable_list_connection_closed;
}

static void
variable_list_init (VariableList *variablelist)
{
  GTK_WIDGET_SET_FLAGS (variablelist, GTK_NO_WINDOW); 

  variablelist->variable = NULL;
}

GtkWidget*
variable_list_new (Web100Object *web100obj)
{ 
  VariableList *variablelist;
#ifdef GTK2
  variablelist = g_object_new (TYPE_VARIABLE_LIST, NULL);
#else
  variablelist = gtk_type_new (variable_list_get_type ()); 
#endif

  web100_widget_set_object (WEB100_WIDGET (variablelist), web100obj);
  
  variable_list_construct (variablelist, web100obj);

  return GTK_WIDGET (variablelist); 
}

static void
variable_list_destroy (GtkObject *object)
{ 
  VariableList *varlist;

  g_return_if_fail (object != NULL);
  g_return_if_fail (IS_VARIABLE_LIST (object));

  varlist = VARIABLE_LIST (object);

  if (varlist->variable) {
    free (varlist->variable);
    varlist->variable = NULL;
  }

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
variable_list_snap_update (GtkObject *object, gpointer data)
{
  VariableList *varlist = VARIABLE_LIST (data); 

  list_values (varlist);
}

static void
variable_list_connection_closed (GtkObject *object, gpointer data)
{
} 

static void
list_values (VariableList *varlist)
{
  web100_agent *agent;
  web100_group *group;
  web100_var **var;
  struct web100_connection_spec spec;
  struct web100_connection_spec_v6 spec_v6; 
  struct snapshot_list *snap, *head; 
  char buf[256];
  int ii = 0;
  char temp[WEB100_VARNAME_LEN_MAX];

  const char *states[] = { "unknown", "Closed", "Listen", "SynSent", "SynReceived", "Established", "FinWait1", "FinWait2", "CloseWait", "LastAck", "Closing", "TimeWait", "DeleteTcb" };
#ifdef GTK2
  GtkTreeIter iter;
#else
#endif

#ifdef GTK2
  if (WEB100_WIDGET (varlist)->web100object->cid == -1) return;

  gtk_tree_model_get_iter_first (GTK_TREE_MODEL (varlist->list_store), &iter);

  head = WEB100_WIDGET (varlist)->web100object->snapshot_head;

  ii = 0;
  while (varlist->variable[ii]) { 
    snap = head;
    while (snap) { 
      if (!web100_snap_read ((web100_var *) varlist->variable[ii], (snap->last), buf)) {
       	gtk_list_store_set (varlist->list_store, &iter, VALUE, web100_value_to_text(web100_get_var_type(varlist->variable[ii]), buf), -1);

	if (!strcmp (web100_get_var_name (varlist->variable[ii]), "State"))
	  gtk_list_store_set (varlist->list_store, &iter, VALUE, states[atoi(web100_value_to_text(web100_get_var_type(varlist->variable[ii]), buf))], -1); 

	if ((web100_get_var_type(varlist->variable[ii]) == WEB100_TYPE_COUNTER32) ||
	    (web100_get_var_type(varlist->variable[ii]) == WEB100_TYPE_COUNTER64)) {
	  if (snap->prior) { 
	    if (!web100_delta_any(varlist->variable[ii], snap->last, snap->prior, buf))
	      gtk_list_store_set (varlist->list_store, &iter, DELTA, web100_value_to_text(web100_get_var_type(varlist->variable[ii]), buf), -1); 
	  }
       	} 
      }
      snap = snap->next;
    }
    gtk_tree_model_iter_next (GTK_TREE_MODEL (varlist->list_store), &iter);
    ii++; 
  }

#else

  if (WEB100_WIDGET (varlist)->web100object->cid == -1) return;

  head = WEB100_WIDGET (varlist)->web100object->snapshot_head;

  ii = 0;
  while (varlist->variable[ii]) {
    snap = head;
    while (snap) { 
      if (!web100_snap_read ((web100_var *) varlist->variable[ii], (snap->last), buf)) { 
	gtk_clist_set_text (GTK_CLIST (varlist->clist), ii, 1, web100_value_to_text(web100_get_var_type(varlist->variable[ii]), buf));

	if (!strcmp (web100_get_var_name (varlist->variable[ii]), "State")) 
	  gtk_clist_set_text (GTK_CLIST (varlist->clist), ii, 1, states[atoi(web100_value_to_text(web100_get_var_type(varlist->variable[ii]), buf))]);

	if ((web100_get_var_type(varlist->variable[ii]) == WEB100_TYPE_COUNTER32) ||
	    (web100_get_var_type(varlist->variable[ii]) == WEB100_TYPE_COUNTER64)) {
	  if (snap->prior) { 
	    if (!web100_delta_any(varlist->variable[ii], snap->last, snap->prior, buf)) 
	      gtk_clist_set_text (GTK_CLIST (varlist->clist), ii, 2, web100_value_to_text(web100_get_var_type(varlist->variable[ii]), buf));

	  }
       	} 
      }
      snap = snap->next;
    }
    ii++; 
  }
#endif
}

#ifdef GTK2
static void
list_select_callback (GtkTreeSelection *listselect, gpointer data)
{

  GtkTreeIter iter;
  GtkTreeModel *model; 
  char *varname;
  static int virgin = 1; // kludge: GtkTreeSelect emits "changed" upon first focus

  Web100Widget *ww = WEB100_WIDGET (data);
  
  if (gtk_tree_selection_get_selected (listselect, &model, &iter))
  { 
    if (virgin) {
      virgin = 0;
      return;
    }

    gtk_tree_model_get (model, &iter, VARNAME, &varname, -1);

    util_launch ("display", ww->web100object, FALSE, ww->local, varname);
  }
}

#else
static void
list_select_callback (GtkCList *clist, gint row, gint column, GdkEventButton *event, VariableList *varlist)
{ 
  web100_var *var;
  char varname[WEB100_VARNAME_LEN_MAX];

  Web100Widget *ww = WEB100_WIDGET (varlist);

  if ((event->type == GDK_2BUTTON_PRESS ||
       	event->type == GDK_3BUTTON_PRESS) || (event->button == 3)) { 

    var = (web100_var *) gtk_clist_get_row_data (varlist->clist, row);
    strcpy (varname, web100_get_var_name (var));

    util_launch ("display", ww->web100object, FALSE, ww->local, varname);
  }
}
#endif

static gint display_launch_double_click (GtkWidget *widget, GdkEventButton *event, gpointer data)
{
#ifdef GTK2
  GtkTreeModel *model;
  GtkTreeIter iter; 
  gchar *varname;
#else
#endif

  if ((event->type==GDK_2BUTTON_PRESS ||
       	event->type==GDK_3BUTTON_PRESS)) {
  
#ifdef GTK2
    if (gtk_tree_selection_get_selected (VARIABLE_LIST (data)->list_select, &model, &iter))
    {
      gtk_tree_model_get (model, &iter, VARNAME, &varname, -1);

      util_launch ("display", WEB100_WIDGET (data)->web100object, FALSE, WEB100_WIDGET (data)->local, varname); 
    }
#else
#endif
  } 

  return FALSE;
}

static gint display_launch_right_click (GtkWidget *widget, GdkEventButton *event, gpointer data)
{
#ifdef GTK2
  GtkTreeModel *model;
  GtkTreeIter iter;
  gchar *varname;
#else
#endif

  if (event->button == 3) { 

#ifdef GTK2
    if (gtk_tree_selection_get_selected (VARIABLE_LIST (data)->list_select, &model, &iter))
    {
      gtk_tree_model_get (model, &iter, VARNAME, &varname, -1);

      util_launch ("display", WEB100_WIDGET (data)->web100object, FALSE, WEB100_WIDGET (data)->local, varname); 
    } 
#else
#endif
  }

  return FALSE;
}

static void
variable_list_construct (VariableList *varlist, Web100Object *web100obj)
{
  web100_agent *agent;
  web100_group *group;
  web100_var *var;
  struct snapshot_list *snap;

  int array_length = 0;
  int array_size = VARNAME_ARRAY_SIZE_INIT;
  int ii = 0;

  GtkWidget *scrolledwindow;

#ifdef GTK2
  GtkWidget *view;
  GtkTreeViewColumn *column;
  GtkTreeSelection  *list_select; 
  GtkCellRenderer   *cell_renderer;
  GtkTreeIter iter;
#else
  GtkCList *clist;
  gchar *titles[] = { "Var name", "value", "delta" };
  gchar *nulltext[3] = { NULL, NULL, NULL };
#endif

// Populate var name list

  varlist->variable = (web100_var **) calloc(VARNAME_ARRAY_SIZE_INIT, sizeof(void *));
 
  agent = WEB100_WIDGET (varlist)->web100object->agent;

  snap = WEB100_WIDGET (varlist)->web100object->snapshot_head;

  while (snap) {
    if (group = web100_group_find (agent, snap->name)) {

      var = web100_var_head (group);

      while (var) { 
	if (array_length == array_size) {
	  array_size *= 2;

	  varlist->variable = (web100_var **) realloc(varlist->variable, array_size*sizeof(void *));
	}

	varlist->variable[array_length++] = var; 
	var = web100_var_next (var);
      }
    }
    snap = snap->next;
  }
  varlist->variable[array_length] = NULL;

// Markup list/tree

#ifdef GTK2
  gtk_widget_set_size_request (GTK_WIDGET (varlist), -1, 300);

  varlist->list_store = gtk_list_store_new (N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);

  view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (varlist->list_store));
  g_object_unref (varlist->list_store); // since view adds a ref

  g_signal_connect (G_OBJECT (view), "button-press-event", G_CALLBACK (display_launch_double_click), varlist);
  g_signal_connect (G_OBJECT (view), "button-release-event", G_CALLBACK (display_launch_right_click), varlist);

  gtk_tree_view_set_search_column (GTK_TREE_VIEW (view), 0);

  varlist->list_select = gtk_tree_view_get_selection (GTK_TREE_VIEW (view));
  gtk_tree_selection_set_mode (varlist->list_select, GTK_SELECTION_SINGLE);
  gtk_tree_selection_unselect_all (varlist->list_select);
  
  cell_renderer = gtk_cell_renderer_text_new ();

  column = gtk_tree_view_column_new_with_attributes ("Variable name", cell_renderer, "text", VARNAME, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (view), column);

  column = gtk_tree_view_column_new_with_attributes ("Value", cell_renderer, "text", VALUE, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (view), column);

  column = gtk_tree_view_column_new_with_attributes ("Delta", cell_renderer, "text", DELTA, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (view), column);

  ii = 0;
  while (varlist->variable[ii]) { 
    gtk_list_store_append (varlist->list_store, &iter);

    gtk_list_store_set (varlist->list_store, &iter, VARNAME, web100_get_var_name (varlist->variable[ii]), -1);

    ii++;
  }

#else
  gtk_widget_set_usize (GTK_WIDGET (varlist), -1, 200);

  varlist->clist = GTK_CLIST (gtk_clist_new_with_titles (3, titles));

  gtk_signal_connect (GTK_OBJECT (varlist->clist), "select-row", GTK_SIGNAL_FUNC (list_select_callback), varlist);

  gtk_clist_column_titles_passive (GTK_CLIST (varlist->clist));
  gtk_clist_set_column_auto_resize (GTK_CLIST (varlist->clist), 0, TRUE);
  gtk_clist_set_column_auto_resize (GTK_CLIST (varlist->clist), 1, TRUE);
  gtk_clist_set_column_auto_resize (GTK_CLIST (varlist->clist), 2, TRUE);

  gtk_clist_set_button_actions (varlist->clist, 2, GTK_BUTTON_SELECTS);

  ii = 0;
  while (varlist->variable[ii]) {
    gtk_clist_insert (GTK_CLIST (varlist->clist), ii, nulltext);

    gtk_clist_set_text (GTK_CLIST (varlist->clist), ii, 0, web100_get_var_name (varlist->variable[ii]));

    gtk_clist_set_row_data (varlist->clist, ii, varlist->variable[ii]);
    ii++;
  }

#endif

  list_values (varlist);

  scrolledwindow = gtk_scrolled_window_new (NULL, NULL); 
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

  gtk_box_pack_start (GTK_BOX (varlist), scrolledwindow, TRUE, TRUE, 0);

  gtk_widget_show (scrolledwindow);

#ifdef GTK2
  gtk_container_add (GTK_CONTAINER (scrolledwindow), view);
  gtk_widget_show (view);

#else
  gtk_container_add (GTK_CONTAINER (scrolledwindow), GTK_WIDGET (varlist->clist));
  gtk_widget_show (GTK_WIDGET (varlist->clist));

#endif 
} 
