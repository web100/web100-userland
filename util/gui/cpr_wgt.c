
#include <stdlib.h>
#include <stdio.h> 
#include <string.h>
#include <gtk/gtk.h>

#include "web100.h"
#include "web100obj.h"
#include "sockset_wgt.h"
#include "cpr_wgt.h"


#define MAXVARNAME 30
#define CPR_DEFAULT_SIZE 300

static void cpr_class_init (CprClass *class);
static void cpr_init (Cpr *cpr);
void cpr_web100obj_snap_update (GtkObject *object, gpointer *data); 
static void cpr_list_vars (Cpr *, Web100Obj *);
static void cpr_destroy (GtkObject *object);

static GtkVBoxClass *parent_class = NULL;

web100_var *wv;

static char varname[9][MAXVARNAME] = {
  "State", "SACKEnabled", "TimestampsEnabled", "CurMSS",
  "MaxMSS", "MinMSS", "WinScaleRcvd", "WinScaleSent", "NagleEnabled" };
char valtext[21];


GtkType cpr_get_type ()
{
  static guint cpr_type = 0;

  if(!cpr_type)
    {
      GtkTypeInfo cpr_info =
      {
	"Cpr",
	sizeof (Cpr),
	sizeof (CprClass),
	(GtkClassInitFunc) cpr_class_init,
	(GtkObjectInitFunc) cpr_init,
        (GtkArgSetFunc) NULL,
        (GtkArgGetFunc) NULL
      };

      cpr_type = gtk_type_unique (gtk_vbox_get_type (), &cpr_info);
    }

  return cpr_type;
}

static void cpr_class_init (CprClass *class)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;

  object_class = (GtkObjectClass*) class;
  widget_class = (GtkWidgetClass*) class;

  parent_class = gtk_type_class (gtk_vbox_get_type ());

  object_class->destroy = cpr_destroy;
} 

GtkWidget* cpr_new (Web100Obj *web100obj)
{
  Cpr *cpr = gtk_type_new (cpr_get_type ()); 

  cpr_set_web100obj (cpr, web100obj);

  return GTK_WIDGET (cpr); 
}

void cpr_set_web100obj (Cpr *cpr, Web100Obj *web100obj)
{
  struct snapshot_data *snap;
  gchar *ntext[3] = { NULL, NULL, NULL };
  int ii;

  g_return_if_fail (cpr != NULL);
  g_return_if_fail (IS_CPR (cpr)); 

  if (!web100obj) {
    cpr->web100obj = NULL;
    return;
  }
  else
    g_return_if_fail (IS_WEB100_OBJ (web100obj));

  if (cpr->web100obj) { 
    gtk_signal_disconnect_by_data (GTK_OBJECT (cpr->web100obj),
                                   (gpointer) cpr);
    gtk_object_unref (GTK_OBJECT (cpr->web100obj));
  }

  cpr->web100obj = web100obj; 
  gtk_object_ref (GTK_OBJECT (web100obj)); 
  gtk_object_sink (GTK_OBJECT (web100obj)); 

  gtk_clist_clear (GTK_CLIST (cpr->varlist));

  for(ii=0;ii<9;ii++) {
    gtk_clist_insert (GTK_CLIST (cpr->varlist), ii, ntext);

    snap = web100obj->snapshot_head;
    while (snap) {
      if ((wv = web100_var_find (snap->group, varname[ii])) == NULL) {
	snap = snap->next;
	continue; 
      }
      gtk_clist_set_text (GTK_CLIST (cpr->varlist), ii, 0, varname[ii]);
      gtk_clist_set_row_data (GTK_CLIST (cpr->varlist), ii, wv);
      snap = snap->next;
    }
  }

  gtk_signal_connect (GTK_OBJECT(web100obj),
                      "snap_update",
                      GTK_SIGNAL_FUNC(cpr_web100obj_snap_update),
                      cpr);
}

static void cpr_list_vars (Cpr *cpr, Web100Obj *web100obj)
{ 
  int ii;
  char buf[8];

  g_return_if_fail (cpr != NULL);
  g_return_if_fail (IS_CPR (cpr));
  
  if (!cpr->web100obj->connection) return;

  gtk_clist_freeze (GTK_CLIST (cpr->varlist)); 

  for(ii=0;ii<9;ii++) { 
    if ((wv = gtk_clist_get_row_data (GTK_CLIST (cpr->varlist), ii)) == NULL)
      continue; 
    if (web100_raw_read (wv, cpr->web100obj->connection, buf) < 0) {
      web100_perror ("cpr_list_vars: raw_read");
      continue;
    } 
    strcpy(valtext, web100_value_to_text(web100_get_var_type (wv), buf));
    gtk_clist_set_text(GTK_CLIST(cpr->varlist), ii, 1, valtext); 
  }

  gtk_clist_thaw(GTK_CLIST(cpr->varlist));  
}

void cpr_web100obj_snap_update (GtkObject *object, gpointer *data)
{
  if (WEB100_OBJ (object)->connection) 
    cpr_list_vars (CPR (data), WEB100_OBJ (object));
}

void cpr_sockset_listen (GtkObject *object, gpointer data)
{ 
  g_return_if_fail (object != NULL);
  g_return_if_fail (GTK_IS_OBJECT (object));
  g_return_if_fail (data != NULL);

  cpr_set_web100obj (CPR (data), SOCKSET (object)->web100obj);
}

static void cpr_init (Cpr *cpr)
{
  gchar *titles[2] = { "Var name", "value" };
  gchar *ntext[3] = { NULL, NULL, NULL };
  int ii;

  GTK_WIDGET_SET_FLAGS (cpr, GTK_NO_WINDOW);
  cpr->web100obj = NULL;
  cpr->scrollwin = NULL;
  cpr->varlist = NULL; 

  cpr->scrollwin = gtk_scrolled_window_new (NULL, NULL); 
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (cpr->scrollwin),
                                  GTK_POLICY_NEVER,
                                  GTK_POLICY_AUTOMATIC); 

  gtk_box_pack_start (GTK_BOX (cpr), cpr->scrollwin, TRUE, TRUE, 0); 
  gtk_widget_show (cpr->scrollwin);

  cpr->varlist = gtk_clist_new_with_titles ( 2, titles);

  gtk_clist_column_titles_passive (GTK_CLIST (cpr->varlist)); 
  gtk_widget_set_usize (GTK_WIDGET(cpr->varlist), 220, 200);
  gtk_clist_set_column_width (GTK_CLIST (cpr->varlist), 0, 100);
  gtk_clist_set_column_width (GTK_CLIST (cpr->varlist), 1, 100);

  gtk_container_add (GTK_CONTAINER (cpr->scrollwin), cpr->varlist);

  gtk_widget_show (cpr->varlist);
}

static void cpr_destroy (GtkObject *object)
{
  g_return_if_fail (object != NULL);
  g_return_if_fail (IS_CPR(object));

  if (CPR (object)->web100obj != NULL) { 
    gtk_signal_disconnect_by_data (GTK_OBJECT (CPR (object)->web100obj),
                                   (gpointer) object);
    gtk_object_unref (GTK_OBJECT (CPR (object)->web100obj));
  }

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}



