#include <stdlib.h>
#include <stdio.h> 
#include <sys/types.h> 
#include <unistd.h>
#include <string.h>

#include <gtk/gtk.h>
#include "web100.h"
#include "web100obj.h"
#include "avd_table_wgt.h"
#include "sockset_wgt.h"

#include "table.h"

#define VARNAME_ARRAY_SIZE_INIT 256

static void avd_table_class_init (Avd_tableClass *class);
static void avd_table_init (Avd_table *avd_table);
static void avd_table_destroy (GtkObject *object);
static void avd_table_list_vars (Avd_table *);
static void avd_table_web100obj_snap_update (GtkObject *object, gpointer *data); 
static void sort_varlist (GtkWidget *button, gpointer data);
static void reset (GtkWidget *button, gpointer data);
static int  compar (const void *, const void *);
static void row_destroy (gpointer);

static GtkVBoxClass *parent_class = NULL;

char       *varname;
static int  varname_array_length, varname_array_size;
static int  varlistsize = 0;
static char valtext[50]; 
static int  sortyes = 0;


GtkType avd_table_get_type ()
{
  static guint avd_table_type = 0;

  if(!avd_table_type)
    {
      GtkTypeInfo avd_table_info =
      {
	"Avd_table",
	sizeof (Avd_table),
	sizeof (Avd_tableClass),
	(GtkClassInitFunc) avd_table_class_init,
	(GtkObjectInitFunc) avd_table_init,
        (GtkArgSetFunc) NULL,
        (GtkArgGetFunc) NULL
      };

      avd_table_type = gtk_type_unique (gtk_vbox_get_type (), &avd_table_info);
    }

  return avd_table_type;
}

static void avd_table_class_init (Avd_tableClass *class)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;

  object_class = (GtkObjectClass*) class;
  widget_class = (GtkWidgetClass*) class;

  parent_class = gtk_type_class (gtk_vbox_get_type ());

  object_class->destroy = avd_table_destroy;
}

GtkWidget* avd_table_new (Web100Obj *web100obj)
{
  Avd_table *avd_table = gtk_type_new (avd_table_get_type ()); 

  avd_table_set_web100obj (avd_table, web100obj); 
  
  gtk_widget_set_usize (GTK_WIDGET (avd_table), 350, 300);

  return GTK_WIDGET (avd_table); 
}

void avd_table_set_web100obj (Avd_table *avd_table, Web100Obj *web100obj)
{ 
  web100_group *gp;
  web100_var *var;
  struct snapshot_data *snap;
  static int varname_array_length, varname_array_size = VARNAME_ARRAY_SIZE_INIT;

  gchar *ntext[4] = { NULL, NULL, NULL, NULL };
  int ii=0, jj=0;

  g_return_if_fail (avd_table != NULL);
  g_return_if_fail (IS_AVD_TABLE (avd_table)); 

  g_return_if_fail (web100obj != NULL);
  g_return_if_fail (IS_WEB100_OBJ (web100obj));

  if (avd_table->web100obj) { 
    gtk_signal_disconnect_by_data (GTK_OBJECT (avd_table->web100obj),
                                   (gpointer) avd_table); 
    gtk_object_unref (GTK_OBJECT (avd_table->web100obj)); 
  }

  avd_table->web100obj = web100obj; 
  gtk_object_ref (GTK_OBJECT (web100obj)); 
  gtk_object_sink (GTK_OBJECT (web100obj)); 

  gtk_clist_clear (GTK_CLIST (avd_table->varlist));

  varname_array_length = 0;
  snap = web100obj->snapshot_head; 
  while (snap) { 
    if ((var = web100_var_head (snap->group)) == NULL &&
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

       	gtk_clist_insert (GTK_CLIST (avd_table->varlist), jj, ntext); 
	gtk_clist_set_text (GTK_CLIST (avd_table->varlist), jj, 0, &varname[jj]);

	gtk_clist_set_row_data_full (GTK_CLIST (avd_table->varlist), jj, var,
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
    snap = snap->next;
  }

  varlistsize = jj;

  gtk_signal_connect (GTK_OBJECT(web100obj),
      "snap_update",
      GTK_SIGNAL_FUNC(avd_table_web100obj_snap_update),
      avd_table); 
}

Web100Obj* avd_table_get_web100obj (Avd_table *avd_table)
{
  g_return_val_if_fail (avd_table != NULL, NULL);
  g_return_val_if_fail (IS_AVD_TABLE (avd_table), NULL);

  return avd_table->web100obj;
}

static void avd_table_list_vars (Avd_table *avd_table)
{ 
  Web100Obj *web100obj;
  struct snapshot_data *snap;
  char buf[8];
  int ii;

  web100obj = avd_table->web100obj;

  gtk_clist_freeze(GTK_CLIST(avd_table->varlist)); 

  for (ii=0;ii<varlistsize;ii++) {
     for (snap = web100obj->snapshot_head; snap != NULL; snap = snap->next) {

    if (autotable[ii].type < 0) {
      /* titles, etc */
      continue;
    }

    switch (autotable[ii].type) {
      case WEB100_TYPE_IP_ADDRESS:
        break; 
      case WEB100_TYPE_COUNTER32:
      case WEB100_TYPE_COUNTER64: 
        if (snap->prior) {
          if (autotable[ii].deltafn(snap->last, snap->prior, buf) < 0)
            break;
          sprinttype (valtext, autotable[ii].type, buf);
          gtk_clist_set_text(GTK_CLIST(avd_table->varlist), ii, 2, valtext);
        } 
      case WEB100_TYPE_INTEGER:
      case WEB100_TYPE_INTEGER32:
      case WEB100_TYPE_GAUGE32:
      case WEB100_TYPE_UNSIGNED32:
      case WEB100_TYPE_TIME_TICKS:
      case WEB100_TYPE_UNSIGNED16: 
        if (autotable[ii].getfn(snap->last, buf) < 0)
          break;
        sprinttype (valtext, autotable[ii].type, buf);
        gtk_clist_set_text(GTK_CLIST(avd_table->varlist), ii, 1, valtext);
        break;
      default:
        gtk_clist_set_text(GTK_CLIST(avd_table->varlist), ii, 1, "Unknown Type");
        break;
    }
     }
  }

 gtk_clist_thaw(GTK_CLIST(avd_table->varlist));  
}

void avd_table_web100obj_snap_update (GtkObject *object, gpointer *data)
{ 
  Web100Obj *web100obj; 
  struct snapshot_data *snap;

  web100obj = WEB100_OBJ (object);

  if(WEB100_OBJ (object)->connection) { 
    avd_table_list_vars (AVD_TABLE (data)); 
  }
}

void avd_table_sockset_listen (GtkObject *object, gpointer data)
{
  Sockset *sockset;
  Avd_table *avd_table;
  Web100Obj *web100obj;

  g_return_if_fail (object != NULL);
  g_return_if_fail (GTK_IS_OBJECT (object));
  g_return_if_fail (data != NULL);

//  printf("heard\n");
  sockset = SOCKSET (object);
  avd_table = AVD_TABLE (data);
  web100obj = sockset->web100obj;
  g_return_if_fail (IS_WEB100_OBJ (web100obj));
//  printf("this far\n");
  avd_table_set_web100obj (AVD_TABLE (avd_table), SOCKSET (sockset)->web100obj);

//  gtk_timeout_add(1000, (GtkFunction) web100obj_refresh, web100obj);
//  web100obj_refresh (WEB100_OBJ (web100obj));
}

static void avd_table_destroy (GtkObject *object)
{
  g_return_if_fail (object != NULL);
  g_return_if_fail (IS_AVD_TABLE(object));

  free (varname);

  if (AVD_TABLE (object)->web100obj != NULL) { 
    gtk_signal_disconnect_by_data (GTK_OBJECT (AVD_TABLE (object)->web100obj),
                                   (gpointer) object);
    gtk_object_unref (GTK_OBJECT (AVD_TABLE (object)->web100obj));
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
  avd_table_set_web100obj (AVD_TABLE (data), AVD_TABLE (data)->web100obj);
}

static void reset (GtkWidget *button, gpointer data)
{
  Avd_table *avd_table;
  struct snapshot_data *snap;

  avd_table = AVD_TABLE (data);
// protect this or set flag 'set_yes'
  snap = avd_table->web100obj->snapshot_head;
  while (snap) {
    web100_snap_data_copy (snap->set, snap->last);
    snap = snap->next;
  } 
} 

static void avd_table_init (Avd_table *avd_table)
{ 
  GtkWidget *hbox, *sortbutton, *reset_button;
  gchar *titles[4] = { "Var name", "value", "delta", "reset" };


  GTK_WIDGET_SET_FLAGS (avd_table, GTK_NO_WINDOW); 

  varname = (char *) calloc(VARNAME_ARRAY_SIZE_INIT,  sizeof(char)*WEB100_VARNAME_LEN_MAX); 
  
  avd_table->web100obj = NULL; //XXX

  avd_table->scrollwin = gtk_scrolled_window_new (NULL, NULL);

  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (avd_table->scrollwin),
                                  GTK_POLICY_NEVER,
                                  GTK_POLICY_AUTOMATIC);

  gtk_box_pack_start (GTK_BOX (avd_table), avd_table->scrollwin, TRUE, TRUE, 0);
  gtk_widget_show (avd_table->scrollwin);

  avd_table->varlist = gtk_clist_new_with_titles (4, titles);
  gtk_clist_column_titles_passive (GTK_CLIST (avd_table->varlist)); 
  gtk_clist_set_column_width (GTK_CLIST (avd_table->varlist), 0, 90);
  gtk_clist_set_column_width (GTK_CLIST (avd_table->varlist), 1, 70);
  gtk_clist_set_column_width (GTK_CLIST (avd_table->varlist), 2, 70);
  gtk_clist_set_column_width (GTK_CLIST (avd_table->varlist), 3, 70);

  gtk_container_add(GTK_CONTAINER(avd_table->scrollwin), avd_table->varlist); 
  gtk_widget_show(avd_table->varlist); 

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (avd_table), hbox, FALSE, FALSE, 5);
  gtk_widget_show (hbox);

  sortbutton = gtk_button_new_with_label ("Sort");
  gtk_signal_connect(GTK_OBJECT(sortbutton), "clicked",
                     GTK_SIGNAL_FUNC(sort_varlist), (gpointer) avd_table);
  gtk_box_pack_start (GTK_BOX (hbox), sortbutton, FALSE, FALSE, 0);
  gtk_widget_show(sortbutton);

  reset_button = gtk_button_new_with_label ("Reset");
  gtk_signal_connect(GTK_OBJECT(reset_button), "clicked",
                     GTK_SIGNAL_FUNC(reset), (gpointer) avd_table);
  gtk_box_pack_end (GTK_BOX (hbox), reset_button, FALSE, FALSE, 0);
  gtk_widget_show(reset_button);
}
