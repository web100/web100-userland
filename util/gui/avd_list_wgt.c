#include <stdlib.h>
#include <stdio.h> 
#include <sys/types.h> 
#include <unistd.h>
#include <string.h>

#include <gtk/gtk.h>
#include "web100.h"
#include "web100obj.h"
#include "avd_list_wgt.h"
#include "sockset_wgt.h"

#define VARNAME_ARRAY_SIZE_INIT 256

static void avd_list_class_init (Avd_listClass *class);
static void avd_list_init (Avd_list *avd_list);
static void avd_list_destroy (GtkObject *object);
static void avd_list_list_vars (Avd_list *);
static void avd_list_web100obj_snap_update (GtkObject *object, gpointer *data); 
static void sort_varlist (GtkWidget *button, gpointer data);
static void reset (GtkWidget *button, gpointer data);
static int  compar (const void *, const void *);
static void row_destroy (gpointer);

static GtkVBoxClass *parent_class = NULL;

char       *varname;
static int  varname_array_length;
static int  varname_array_size = VARNAME_ARRAY_SIZE_INIT;
static int  varlistsize = 0;
char        vname[256][WEB100_VARNAME_LEN_MAX];
static char valtext[50]; 
static int  sortyes = 0;


GtkType avd_list_get_type ()
{
  static guint avd_list_type = 0;

  if(!avd_list_type)
    {
      GtkTypeInfo avd_list_info =
      {
	"Avd_list",
	sizeof (Avd_list),
	sizeof (Avd_listClass),
	(GtkClassInitFunc) avd_list_class_init,
	(GtkObjectInitFunc) avd_list_init,
        (GtkArgSetFunc) NULL,
        (GtkArgGetFunc) NULL
      };

      avd_list_type = gtk_type_unique (gtk_vbox_get_type (), &avd_list_info);
    }

  return avd_list_type;
}

static void avd_list_class_init (Avd_listClass *class)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;

  object_class = (GtkObjectClass*) class;
  widget_class = (GtkWidgetClass*) class;

  parent_class = gtk_type_class (gtk_vbox_get_type ());

  object_class->destroy = avd_list_destroy;
}

GtkWidget* avd_list_new (Web100Obj *web100obj)
{
  Avd_list *avd_list = gtk_type_new (avd_list_get_type ()); 

  avd_list_set_web100obj (avd_list, web100obj); 
  
  gtk_widget_set_usize (GTK_WIDGET (avd_list), 350, 300);

  return GTK_WIDGET (avd_list); 
}

void avd_list_set_web100obj (Avd_list *avd_list, Web100Obj *web100obj)
{ 
  web100_group *gp;
  web100_var *var;
  struct snapshot_data *snap; 

  gchar *ntext[4] = { NULL, NULL, NULL, NULL };
  int ii=0, jj=0;

  g_return_if_fail (avd_list != NULL);
  g_return_if_fail (IS_AVD_LIST (avd_list)); 

  g_return_if_fail (web100obj != NULL);
  g_return_if_fail (IS_WEB100_OBJ (web100obj));

  if (avd_list->web100obj) { 
    gtk_signal_disconnect_by_data (GTK_OBJECT (avd_list->web100obj),
                                   (gpointer) avd_list); 
    gtk_object_unref (GTK_OBJECT (avd_list->web100obj)); 
  }

  avd_list->web100obj = web100obj; 
  gtk_object_ref (GTK_OBJECT (web100obj)); 
  gtk_object_sink (GTK_OBJECT (web100obj)); 

  gtk_clist_clear (GTK_CLIST (avd_list->varlist));

  gp = web100_group_find(web100obj->agent, "read");
  varname_array_length = 0;
//  snap = web100obj->snapshot_head; 
//  while (snap) { 
    if ((var = web100_var_head (gp)) == NULL &&
       	web100_errno != WEB100_ERR_SUCCESS) {
      web100_perror("web100_var_head");
      return;
    } 

    while (var) { 
      if (varname_array_length == varname_array_size) { 
       	varname_array_size *= 2;
       	varname = (char *) realloc(varname, WEB100_VARNAME_LEN_MAX*varname_array_size); 
      } 
      if (strncmp(web100_get_var_name(var), "_", 1)) { 
	strncpy ((char *) &varname[jj], web100_get_var_name(var), WEB100_VARNAME_LEN_MAX); 
	strncpy ((char *) vname[jj], web100_get_var_name(var), WEB100_VARNAME_LEN_MAX);

       	gtk_clist_insert (GTK_CLIST (avd_list->varlist), jj, ntext); 
	gtk_clist_set_text (GTK_CLIST (avd_list->varlist), jj, 0, &varname[jj]);

	gtk_clist_set_row_data_full (GTK_CLIST (avd_list->varlist), jj, var,
	    (GtkDestroyNotify) row_destroy); 

	varname_array_length++;
	jj++;
      }

      if ((var = web100_var_next(var)) == NULL &&
	  web100_errno != WEB100_ERR_SUCCESS) {
       	web100_perror("web100_var_next");
       	return;
      } 
    }
//    snap = snap->next;
//  }

  varlistsize = jj;

  gtk_signal_connect (GTK_OBJECT(web100obj), "snap_update", GTK_SIGNAL_FUNC(avd_list_web100obj_snap_update), avd_list); 
}

Web100Obj* avd_list_get_web100obj (Avd_list *avd_list)
{
  g_return_val_if_fail (avd_list != NULL, NULL);
  g_return_val_if_fail (IS_AVD_LIST (avd_list), NULL);

  return avd_list->web100obj;
}

static void avd_list_list_vars (Avd_list *avd_list)
{ 
  web100_var *var; 
  web100_group *gp;
  Web100Obj *web100obj;
  struct snapshot_data *snap;
  char buf[256]; 
  char *text;
  int ii;
  
  web100obj = avd_list->web100obj; 

  gtk_clist_freeze(GTK_CLIST(avd_list->varlist));
  
  gp = web100_group_find(web100obj->agent, "read");
  for (ii=0;ii<varlistsize;ii++) {

    var = web100_var_find(gp, vname[ii]); 

    snap = web100obj->snapshot_head;

    while(snap) {
      if(!strcmp(web100_get_group_name(snap->group), "read"))
	break;
      snap = snap->next;
    }

    web100_snap_read (var, (snap->last), buf);
    strcpy(valtext, web100_value_to_text(web100_get_var_type(var), buf)); 

    gtk_clist_set_text (GTK_CLIST (avd_list->varlist), ii, 1, valtext); 

    if ((web100_get_var_type(var) == WEB100_TYPE_COUNTER32) ||
        (web100_get_var_type(var) == WEB100_TYPE_COUNTER64)) {

      if (snap->prior) { 
       	if (web100_delta_any(var, snap->last, snap->prior, buf) < 0) {
	  web100_perror ("web100_delta_any");
	  continue;
	}
	
       	strcpy(valtext, web100_value_to_text(web100_get_var_type(var), buf));
       	gtk_clist_set_text (GTK_CLIST (avd_list->varlist), ii, 2, valtext);
      }

      if (snap->set) {
       	if (web100_delta_any(var, snap->last, snap->set, buf) < 0) {
	  web100_perror ("web100_delta_any");
	  continue;
       	}

        strcpy(valtext, web100_value_to_text(web100_get_var_type(var), buf));
       	gtk_clist_set_text (GTK_CLIST (avd_list->varlist), ii, 3, valtext);
      }
    } 
  } 

  gtk_clist_thaw(GTK_CLIST(avd_list->varlist));  
}

void avd_list_web100obj_snap_update (GtkObject *object, gpointer *data)
{ 
  Web100Obj *web100obj; 

  web100obj = WEB100_OBJ (object);

  if(WEB100_OBJ (object)->connection) { 

    avd_list_list_vars (AVD_LIST (data)); 
  }
}

void avd_list_sockset_listen (GtkObject *object, gpointer data)
{
  Sockset *sockset;
  Avd_list *avd_list;
  Web100Obj *web100obj;

  g_return_if_fail (object != NULL);
  g_return_if_fail (GTK_IS_OBJECT (object));
  g_return_if_fail (data != NULL);

  sockset = SOCKSET (object);
  avd_list = AVD_LIST (data);
  web100obj = sockset->web100obj;
  g_return_if_fail (IS_WEB100_OBJ (web100obj));

  avd_list_set_web100obj (AVD_LIST (avd_list), SOCKSET (sockset)->web100obj);
}

static void avd_list_destroy (GtkObject *object)
{
  g_return_if_fail (object != NULL);
  g_return_if_fail (IS_AVD_LIST(object));

  free (varname);

  if (AVD_LIST (object)->web100obj != NULL) { 
    gtk_signal_disconnect_by_data (GTK_OBJECT (AVD_LIST (object)->web100obj),
                                   (gpointer) object);
    gtk_object_unref (GTK_OBJECT (AVD_LIST (object)->web100obj));
  }

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

int compar(const void *data1, const void *data2)
{ 
    return(strcmp((char *) data1++, (char *) data2++)); 
}

static void row_destroy(gpointer data)
{
//  free(data), if allocated;
}

void sort_varlist (GtkWidget *button, gpointer data)
{
  sortyes = 1 - sortyes;
  avd_list_set_web100obj (AVD_LIST (data), AVD_LIST (data)->web100obj);
}

static void reset (GtkWidget *button, gpointer data)
{
  Avd_list *avd_list;
  struct snapshot_data *snap;

  avd_list = AVD_LIST (data);

  snap = avd_list->web100obj->snapshot_head;
  while (snap) {
    web100_snap_data_copy (snap->set, snap->last);
    snap = snap->next;
  } 
} 

static void avd_list_init (Avd_list *avd_list)
{ 
  GtkWidget *hbox, *sortbutton, *reset_button;
  gchar *titles[4] = { "Var name", "value", "delta", "reset" };


  GTK_WIDGET_SET_FLAGS (avd_list, GTK_NO_WINDOW); 

  varname = (char *) calloc(VARNAME_ARRAY_SIZE_INIT,  sizeof(char)*WEB100_VARNAME_LEN_MAX); 
  
  avd_list->web100obj = NULL;

  avd_list->scrollwin = gtk_scrolled_window_new (NULL, NULL);

  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (avd_list->scrollwin),
                                  GTK_POLICY_NEVER,
                                  GTK_POLICY_AUTOMATIC);

  gtk_box_pack_start (GTK_BOX (avd_list), avd_list->scrollwin, TRUE, TRUE, 0);
  gtk_widget_show (avd_list->scrollwin);

  avd_list->varlist = gtk_clist_new_with_titles (4, titles);
  gtk_clist_column_titles_passive (GTK_CLIST (avd_list->varlist)); 
  gtk_clist_set_column_width (GTK_CLIST (avd_list->varlist), 0, 90);
  gtk_clist_set_column_width (GTK_CLIST (avd_list->varlist), 1, 70);
  gtk_clist_set_column_width (GTK_CLIST (avd_list->varlist), 2, 70);
  gtk_clist_set_column_width (GTK_CLIST (avd_list->varlist), 3, 70);

  gtk_container_add(GTK_CONTAINER(avd_list->scrollwin), avd_list->varlist); 
  gtk_widget_show(avd_list->varlist); 

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (avd_list), hbox, FALSE, FALSE, 5);
  gtk_widget_show (hbox);

  sortbutton = gtk_button_new_with_label ("Sort");
  gtk_signal_connect(GTK_OBJECT(sortbutton), "clicked",
                     GTK_SIGNAL_FUNC(sort_varlist), (gpointer) avd_list);
  gtk_box_pack_start (GTK_BOX (hbox), sortbutton, FALSE, FALSE, 0);
  gtk_widget_show(sortbutton);

  reset_button = gtk_button_new_with_label ("Reset");
  gtk_signal_connect(GTK_OBJECT(reset_button), "clicked",
                     GTK_SIGNAL_FUNC(reset), (gpointer) avd_list);
  gtk_box_pack_end (GTK_BOX (hbox), reset_button, FALSE, FALSE, 0);
  gtk_widget_show(reset_button);
}
