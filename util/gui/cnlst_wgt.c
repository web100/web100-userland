#include <stdlib.h>
#include <stdio.h> 
#include <gtk/gtk.h>

#include "web100.h"
#include "web100obj.h"
#include "connection_info.h"
#include "cnlst_wgt.h"


static void cnlst_class_init (CnlstClass *class);
static void cnlst_init (Cnlst *cnlst);
static void cnlst_destroy (GtkObject *object);
static void choose_sort(GtkWidget *, gpointer);
static void row_destroy (gpointer data);

static GtkVBoxClass *parent_class = NULL;

static char *states[12] = {
    "ESTBLSH",   "SYNSENT",   "SYNRECV",   "FWAIT1",   "FWAIT2", "TMEWAIT",
    "CLOSED",    "CLSWAIT",   "LASTACK",   "LISTEN", "CLOSING" }; 
static char *text[8];


GtkType cnlst_get_type ()
{
  static guint cnlst_type = 0;

  if(!cnlst_type)
    {
      GtkTypeInfo cnlst_info =
      {
	"Cnlst",
	sizeof (Cnlst),
	sizeof (CnlstClass),
	(GtkClassInitFunc) cnlst_class_init,
	(GtkObjectInitFunc) cnlst_init,
        (GtkArgSetFunc) NULL,
        (GtkArgGetFunc) NULL
      };

      cnlst_type = gtk_type_unique (gtk_vbox_get_type (), &cnlst_info);
    }

  return cnlst_type;
}

static void cnlst_class_init (CnlstClass *class)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;

  object_class = (GtkObjectClass*) class;
  widget_class = (GtkWidgetClass*) class;

  parent_class = gtk_type_class (gtk_vbox_get_type ());

  object_class->destroy = cnlst_destroy;
}

static void cnlst_init (Cnlst *cnlst)
{
    GTK_WIDGET_SET_FLAGS (cnlst, GTK_NO_WINDOW);
    cnlst->web100obj = NULL;
}

static void cnlst_construct (Cnlst *cnlst, Web100Obj *web100obj)
{ 
  gchar *titles[8] = { "cmdline", "pid", "local add", "local port",
                       "remote add", "remote port", "cid", "state" };
  GtkWidget *scrolladj, *scrollwin, *hbox, *label, *button[5];      
  char *labels[5] = { "cmdline", "local port", "remote add", "remote port", "cid"};
  int ii;

  GTK_WIDGET_SET_FLAGS (cnlst, GTK_NO_WINDOW); 

  for(ii=0;ii<8;ii++) if( (text[ii] = malloc(64)) == NULL) {
    fprintf(stderr, "Out of memory!\n");
    exit(1);
  }

  scrollwin = gtk_scrolled_window_new(NULL, NULL); 
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrollwin),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS); 
  gtk_box_pack_start(GTK_BOX(cnlst), scrollwin, TRUE, TRUE, 0);
  gtk_widget_show(scrollwin);

  cnlst->clist = gtk_clist_new_with_titles(8, titles);

  gtk_widget_set_usize (GTK_WIDGET( cnlst->clist), 500, 250);
  gtk_clist_set_column_width(GTK_CLIST(cnlst->clist), 0, 50);
  gtk_clist_set_column_width(GTK_CLIST(cnlst->clist), 1, 30);
  gtk_clist_set_column_width(GTK_CLIST(cnlst->clist), 2, 80);
  gtk_clist_set_column_width(GTK_CLIST(cnlst->clist), 3, 50);
  gtk_clist_set_column_width(GTK_CLIST(cnlst->clist), 4, 80);
  gtk_clist_set_column_width(GTK_CLIST(cnlst->clist), 5, 50);
  gtk_clist_set_column_width(GTK_CLIST(cnlst->clist), 6, 30);
  gtk_clist_set_column_width(GTK_CLIST(cnlst->clist), 7, 60);
  gtk_container_add(GTK_CONTAINER(scrollwin), cnlst->clist);
  gtk_widget_show(cnlst->clist);

  cnlst_set_web100obj (cnlst, web100obj);
  cnlst_update((gpointer) cnlst); 
}

GtkWidget* cnlst_new (Web100Obj *web100obj)
{
  Cnlst *cnlst = gtk_type_new (cnlst_get_type ()); 

  cnlst_construct (cnlst, web100obj);

  return GTK_WIDGET (cnlst); 
}

void cnlst_set_web100obj (Cnlst *cnlst, Web100Obj *web100obj)
{
  g_return_if_fail (cnlst != NULL);
  g_return_if_fail (IS_CNLST (cnlst)); 

  g_return_if_fail (web100obj != NULL);
  g_return_if_fail (IS_WEB100_OBJ (web100obj));

  if (cnlst->web100obj) { 
    gtk_signal_disconnect_by_data (GTK_OBJECT (cnlst->web100obj),
                                   (gpointer) cnlst);
    gtk_object_unref (GTK_OBJECT (cnlst->web100obj));
  }

  cnlst->web100obj = web100obj; 
  gtk_object_ref (GTK_OBJECT (web100obj)); 
  gtk_object_sink (GTK_OBJECT (web100obj));
}

Web100Obj* cnlst_get_web100obj (Cnlst *cnlst)
{
    g_return_val_if_fail (cnlst != NULL, NULL);
    g_return_val_if_fail (IS_CNLST (cnlst), NULL);

    return cnlst->web100obj;
}

gint cnlst_update (gpointer data)
{ 
  Cnlst *cnlst = (Cnlst *) data;
  int ii=0; 
  GtkCList *clist;
  float vadjust;
  struct connection_info *res;
  struct web100_connection_spec spec;
  struct web100_connection_spec_v6 spec_v6;

  
  clist = GTK_CLIST(cnlst->clist);

  gtk_clist_freeze(GTK_CLIST(clist));

  vadjust = GTK_CLIST(clist)->vadjustment->value;

  gtk_clist_clear(GTK_CLIST(clist) );

  res = connection_info_head(cnlst->web100obj->agent);

  while(res) { 
    strcpy(text[0], connection_info_get_cmdline(res)); 
    sprintf(text[1], "%d", connection_info_get_pid(res)); 

    if(connection_info_get_addrtype(res) == WEB100_ADDRTYPE_IPV4) {
      connection_info_get_spec(res, &spec);
      strcpy(text[2], web100_value_to_text(WEB100_TYPE_INET_ADDRESS_IPV4, &spec.src_addr));
      strcpy(text[3], web100_value_to_text(WEB100_TYPE_INET_PORT_NUMBER, &spec.src_port));
      strcpy(text[4], web100_value_to_text(WEB100_TYPE_INET_ADDRESS_IPV4, &spec.dst_addr));
      strcpy(text[5], web100_value_to_text(WEB100_TYPE_INET_PORT_NUMBER, &spec.dst_port));
    }

    if(connection_info_get_addrtype(res) == WEB100_ADDRTYPE_IPV6) {
      connection_info_get_spec_v6(res, &spec_v6);
      strcpy(text[2], web100_value_to_text(WEB100_TYPE_INET_ADDRESS_IPV6, &spec_v6.src_addr));
      strcpy(text[3], web100_value_to_text(WEB100_TYPE_INET_PORT_NUMBER, &spec_v6.src_port));
      strcpy(text[4], web100_value_to_text(WEB100_TYPE_INET_ADDRESS_IPV6, &spec_v6.dst_addr));
      strcpy(text[5], web100_value_to_text(WEB100_TYPE_INET_PORT_NUMBER, &spec_v6.dst_port));
    }

    sprintf(text[6], "%d", connection_info_get_cid(res)); 
    if(connection_info_get_state(res))
      strcpy(text[7], states[connection_info_get_state(res)-1]); 

    gtk_clist_insert (GTK_CLIST (cnlst->clist), ii, text);
    gtk_clist_set_row_data (GTK_CLIST (cnlst->clist), ii, GINT_TO_POINTER(connection_info_get_cid(res)));
	                  
    res = connection_info_next(res);
    ii++; 
  }

  gtk_adjustment_set_value( GTK_ADJUSTMENT(GTK_CLIST(clist)->vadjustment), vadjust);

  gtk_clist_thaw(GTK_CLIST(clist)); 

  return TRUE;
}

static void cnlst_destroy (GtkObject *object)
{
  Cnlst *cnlst;

  g_return_if_fail (object != NULL);
  g_return_if_fail (IS_CNLST(object)); 

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void row_destroy (gpointer data)
{
//  free (data);
}
