#include <stdlib.h>
#include <stdio.h> 
#include <sys/types.h>
#include <gtk/gtk.h>
#include <unistd.h>

#include "je-web100.h"
#include "je-web100-int.h"
#include "web100obj.h"
#include "avd_wgt.h"

#ifdef TABLE_DRIVEN
#include "table.h" 
#endif


static void avd_class_init (AvdClass *class);
static void avd_init (Avd *avd);
static void avd_destroy (GtkObject *object);
static void avd_list_vars (web100_snapshot_t *ns, web100_snapshot_t *os, Avd *);
static void sort_varlist (GtkWidget *button, gpointer data);
static void reset (GtkWidget *button, gpointer data);
static int  compar (const void *, const void *);
static void row_destroy (gpointer);

static GtkVBoxClass *parent_class = NULL;

static char   varname[200][WEB100_VARNAME_LEN_MAX];
static int    varlistsize = 0;
static char   valtext[50]; 
static int    sortyes = 0;


GtkType avd_get_type ()
{
  static guint avd_type = 0;

  if(!avd_type)
    {
      GtkTypeInfo avd_info =
      {
	"Avd",
	sizeof (Avd),
	sizeof (AvdClass),
	(GtkClassInitFunc) avd_class_init,
	(GtkObjectInitFunc) avd_init,
        (GtkArgSetFunc) NULL,
        (GtkArgGetFunc) NULL
      };

      avd_type = gtk_type_unique (gtk_vbox_get_type (), &avd_info);
    }

  return avd_type;
}

static void avd_class_init (AvdClass *class)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;

  object_class = (GtkObjectClass*) class;
  widget_class = (GtkWidgetClass*) class;

  parent_class = gtk_type_class (gtk_vbox_get_type ());

  object_class->destroy = avd_destroy;
}

static void avd_init (Avd *avd)
{ 
  GtkWidget *hbox, *sortbutton, *resetbutton;
  gchar *titles[4] = { "Var name", "value", "delta", "reset" };


  GTK_WIDGET_SET_FLAGS (avd, GTK_NO_WINDOW); 
  avd->web100obj = NULL;

  avd->scrollwin = gtk_scrolled_window_new (NULL, NULL);

  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (avd->scrollwin),
                                  GTK_POLICY_NEVER,
                                  GTK_POLICY_AUTOMATIC);

  gtk_box_pack_start (GTK_BOX (avd), avd->scrollwin, TRUE, TRUE, 0);
  gtk_widget_show (avd->scrollwin);

  avd->varlist = gtk_clist_new_with_titles (4, titles);
  gtk_clist_column_titles_passive (GTK_CLIST (avd->varlist)); 
  gtk_clist_set_column_width (GTK_CLIST (avd->varlist), 0, 90);
  gtk_clist_set_column_width (GTK_CLIST (avd->varlist), 1, 70);
  gtk_clist_set_column_width (GTK_CLIST (avd->varlist), 2, 70);
  gtk_clist_set_column_width (GTK_CLIST (avd->varlist), 3, 70);

  gtk_container_add(GTK_CONTAINER(avd->scrollwin), avd->varlist); 
  gtk_widget_show(avd->varlist); 

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (avd), hbox, FALSE, FALSE, 5);
  gtk_widget_show (hbox);

  sortbutton = gtk_button_new_with_label ("Sort");
  gtk_signal_connect(GTK_OBJECT(sortbutton), "clicked",
                     GTK_SIGNAL_FUNC(sort_varlist), (gpointer) avd);
  gtk_box_pack_start (GTK_BOX (hbox), sortbutton, FALSE, FALSE, 0);
  gtk_widget_show(sortbutton);

  resetbutton = gtk_button_new_with_label ("Reset");
  gtk_signal_connect(GTK_OBJECT(resetbutton), "clicked",
                     GTK_SIGNAL_FUNC(reset), (gpointer) avd);
  gtk_box_pack_end (GTK_BOX (hbox), resetbutton, FALSE, FALSE, 0);
  gtk_widget_show(resetbutton);
}

GtkWidget* avd_new (Web100Obj *web100obj)
{
  Avd *avd = gtk_type_new (avd_get_type ()); 

  avd_set_web100obj (avd, web100obj); 
  gtk_widget_set_usize (GTK_WIDGET (avd), 350, 300);

  return GTK_WIDGET (avd); 
}

void sort_varlist (GtkWidget *button, gpointer data)
{
  sortyes = 1 - sortyes;
//  avd_set_web100obj (AVD (data), AVD (data)->web100obj);
}

void reset (GtkWidget *button, gpointer data)
{
  Avd *avd;

  avd = AVD (data);

  web100_snap_data_copy (avd->web100obj->set_snap_read, avd->web100obj->snap_read_last);
}

void avd_web100obj_connection_changed (GtkObject *object, gpointer *data)
{
  Web100Obj *web100obj;
  web100_var_t *var;
  Avd *avd;
  int jj; 

  web100obj = WEB100_OBJ (object);
  avd = AVD (data);

  for (jj=0;jj<varlistsize;jj++) {
    var = web100_var_find (web100_group_find (web100obj->agent, "read"),
                           &varname[jj][0]);
    gtk_clist_set_row_data_full (GTK_CLIST (avd->varlist), jj, var,
                                 (GtkDestroyNotify) row_destroy);
  } 
}

void avd_web100obj_snap_update (GtkObject *object, gpointer *data)
{ 
  Web100Obj *web100obj; 

  web100obj = WEB100_OBJ (object);

  if(WEB100_OBJ (object)->connection) { 
    avd_list_vars (web100obj->snap_read_last, web100obj->snap_read_prior, AVD (data)); 


    avd_list_vars (web100obj->snap_tune_last, web100obj->snap_tune_prior, AVD (data)); 

  }
}

static void avd_destroy (GtkObject *object)
{
  g_return_if_fail (object != NULL);
  g_return_if_fail (IS_AVD(object));

  if (AVD (object)->web100obj != NULL) { 
    gtk_signal_disconnect_by_data (GTK_OBJECT (AVD (object)->web100obj),
                                   (gpointer) object);
    gtk_object_unref (GTK_OBJECT (AVD (object)->web100obj));
  }

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

void avd_set_web100obj (Avd *avd, Web100Obj *web100obj)
{ 
  web100_group_t *gp;
  web100_var_t *var;
  gchar *ntext[4] = { NULL, NULL, NULL, NULL };
  int ii=0, jj=0;

  g_return_if_fail (avd != NULL);
  g_return_if_fail (IS_AVD (avd)); 

  if (!web100obj) { 
    web100obj = (Web100Obj *) web100obj_new (NULL, NULL); 
    return;
  }
  else
    g_return_if_fail (IS_WEB100_OBJ (web100obj));

  if (avd->web100obj) { 
    gtk_signal_disconnect_by_data (GTK_OBJECT (avd->web100obj),
                                   (gpointer) avd);
    gtk_object_unref (GTK_OBJECT (avd->web100obj));
  }

  avd->web100obj = web100obj; 
  gtk_object_ref (GTK_OBJECT (web100obj)); 
  gtk_object_sink (GTK_OBJECT (web100obj)); 

  avd->web100obj->snap_read_prior = avd->web100obj->snap_tune_prior = NULL;
  

  if(avd->varlist) gtk_clist_clear (GTK_CLIST (avd->varlist));

#ifdef TABLE_DRIVEN

  for (jj=0; autotable[jj].tname; jj++) { 

    gtk_clist_insert(GTK_CLIST(avd->varlist), jj, ntext);
    gtk_clist_set_text(GTK_CLIST(avd->varlist), jj, 0, autotable[jj].tname);


    if (autotable[jj].type >= 0) {
      gp = web100_group_head (web100obj->agent);
      while (gp) {
       	if ((var = web100_var_find(gp, autotable[jj].tname)) != NULL)
	  break;
       	gp = web100_group_next (gp);
      }
      gtk_clist_set_row_data_full(GTK_CLIST(avd->varlist), jj, var,
	                          (GtkDestroyNotify) row_destroy);
    }

  }

#else // list from header

  if ((gp = web100_group_find (web100obj->agent, "read")) == NULL) {
    web100_perror("web100_group_find");
    return;
  } 

  if ((var = web100_var_head (gp)) == NULL &&
      web100_errno != WEB100_ERR_SUCCESS) {
    web100_perror("web100_var_head");
    return;
  }

  while (var) {

    strncpy (varname[jj], web100_get_var_name(var), WEB100_VARNAME_LEN_MAX);
    gtk_clist_insert (GTK_CLIST (avd->varlist), jj, ntext); 
    gtk_clist_set_text (GTK_CLIST (avd->varlist), jj, 0, varname[jj]);
   
    gtk_clist_set_row_data_full (GTK_CLIST (avd->varlist), jj, var,
                                 (GtkDestroyNotify) row_destroy); 
    jj++;

    if ((var = web100_var_next(var)) == NULL &&
        web100_errno != WEB100_ERR_SUCCESS) {
      web100_perror("web100_var_next");
      return;
    } 
  } 

#endif // TABLE_DRIVEN 

  varlistsize = jj;

  gtk_signal_connect (GTK_OBJECT(web100obj), 
                      "connection_changed",
                      GTK_SIGNAL_FUNC(avd_web100obj_connection_changed),
                      avd);

//  avd_web100obj_connection_changed (GTK_OBJECT(web100obj), (gpointer) avd);

  gtk_signal_connect (GTK_OBJECT(web100obj),
                      "snap_update",
                      GTK_SIGNAL_FUNC(avd_web100obj_snap_update),
                      avd); 
}

Web100Obj* avd_get_web100obj (Avd *avd)
{
  g_return_val_if_fail (avd != NULL, NULL);
  g_return_val_if_fail (IS_AVD (avd), NULL);

  return avd->web100obj;
}

static void avd_list_vars (web100_snapshot_t *ns, web100_snapshot_t *os, Avd *avd)
{ 
  int ii=0;
  web100_var_t *var;
  web100_snapshot_t *sp;
  char buf[8];
  u_int32_t val32, del32;
//  u_int64_t val;
  unsigned long long val;

  char *text;

  gtk_clist_freeze(GTK_CLIST(avd->varlist));
  
#ifdef TABLE_DRIVEN

  for (ii=0;ii<varlistsize;ii++) {

    if (autotable[ii].type < 0) {
      /* titles, etc */
      continue;
    }
   
    if ((var = (web100_var_t *) gtk_clist_get_row_data (GTK_CLIST (avd->varlist), ii)) == NULL) {
      gtk_clist_set_text(GTK_CLIST(avd->varlist), ii, 1, "Unsupported");
      continue;
    }

    if (var->group != ns->group) {
      if (!gtk_clist_get_text (GTK_CLIST(avd->varlist), ii, 1, &text))
       	gtk_clist_set_text(GTK_CLIST(avd->varlist), ii, 1, "Elsewhere");
      continue;
    }

    switch (autotable[ii].type) {
      case WEB100_TYPE_IP_ADDRESS:
        break;
      case WEB100_TYPE_COUNTER32:
      case WEB100_TYPE_COUNTER64:
        if (os) {
          val = autotable[ii].deltafn(ns, os);
          sprinttype(valtext, autotable[ii].type, &val);
          gtk_clist_set_text(GTK_CLIST(avd->varlist), ii, 2, valtext);
        }
      case WEB100_TYPE_INTEGER:
      case WEB100_TYPE_INTEGER32:
      case WEB100_TYPE_GAUGE32:
      case WEB100_TYPE_UNSIGNED32:
      case WEB100_TYPE_TIME_TICKS:
      case WEB100_TYPE_UNSIGNED16: 
        val = autotable[ii].getfn(ns);
        sprinttype (valtext, autotable[ii].type, &val);
        gtk_clist_set_text(GTK_CLIST(avd->varlist), ii, 1, valtext);
        break;
      default:
        gtk_clist_set_text(GTK_CLIST(avd->varlist), ii, 1, "Unknown type");
        break;
    }
  }

#else 

  for (ii=0;ii<varlistsize;ii++) {
    if ((var = (web100_var_t *) gtk_clist_get_row_data (GTK_CLIST (avd->varlist), ii)) == NULL)
      continue; 

    if (web100_snap_read (var, ns, &buf) < 0) {
//      web100_perror ("web100_snap_read");
      continue;
    } 
    sprinttype (valtext, web100_get_var_type(var), buf); 

    gtk_clist_set_text (GTK_CLIST (avd->varlist), ii, 1, valtext); 

    if ((web100_get_var_type(var) == WEB100_TYPE_COUNTER32) ||
        (web100_get_var_type(var) == WEB100_TYPE_COUNTER64))
      if (os) { 
       	if (web100_delta_any(var, ns, os, buf) < 0) {
	  web100_perror ("web100_delta_any");
	  continue;
	}
	
       	sprinttype (valtext, web100_get_var_type(var), buf);
       	gtk_clist_set_text (GTK_CLIST (avd->varlist), ii, 2, valtext);
      }

/*
    if ((web100_get_var_type(var) == WEB100_TYPE_COUNTER32) ||
        (web100_get_var_type(var) == WEB100_TYPE_COUNTER64)) {
      web100_delta_any(var, , buf);
      sprintval (valtext, var, buf);
      gtk_clist_set_text (GTK_CLIST (avd->varlist), ii, 3, valtext);
    }  
*/    
  } 

#endif //TABLE_DRIVEN

//  if (first) web100_snap_data_copy (web100obj->set_snap_read, web100obj->snap_read_last);
//  first = 0; 

  gtk_clist_thaw(GTK_CLIST(avd->varlist));  
}

int compar(const void *data1, const void *data2)
{ 
    return(strcmp((char *) data1++, (char *) data2++)); 
}

static void row_destroy(gpointer data)
{
//  free(data), if allocated;
}
