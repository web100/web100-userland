// 
#include <stdlib.h>
#include <stdio.h> 
#include <gtk/gtk.h>

#include "web100.h"
#include "web100sync.h"
#include "web100obj.h" 
#include "ur_launch.h"
#include "sockset_wgt.h"
#include "dtb_wgt.h"


//#define DTB_DEFAULT_SIZE 300
#define NUM_TOOLNAME 6
#define NUM_VARNAME 15


char toolname[NUM_TOOLNAME][16] = {
  "vdt", "avd_list", "cpr", "triage", "stuner", "rtuner" };

static void dtb_class_init (DtbClass *class);
static void dtb_init (Dtb *dtb);
//static void dtb_construct (Dtb *dtb, Web100Obj *web100obj);
//static void dtb_web100obj_snap_update (GtkObject *object, gpointer *data);
static guint dtb_timeout (gpointer data);
static void dtb_destroy (GtkObject *object);

static GtkVBoxClass *parent_class = NULL;


GtkType dtb_get_type ()
{
  static guint dtb_type = 0;

  if(!dtb_type)
    {
      GtkTypeInfo dtb_info =
      {
	"Dtb",
	sizeof (Dtb),
	sizeof (DtbClass),
	(GtkClassInitFunc) dtb_class_init,
	(GtkObjectInitFunc) dtb_init,
        (GtkArgSetFunc) NULL,
        (GtkArgGetFunc) NULL
      };

      dtb_type = gtk_type_unique (gtk_vbox_get_type (), &dtb_info);
    }

  return dtb_type;
}

static void dtb_class_init (DtbClass *class)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;

  object_class = (GtkObjectClass*) class;
  widget_class = (GtkWidgetClass*) class;

  parent_class = gtk_type_class (gtk_vbox_get_type ());

  object_class->destroy = dtb_destroy;
}

GtkWidget* dtb_new (Web100Obj *web100obj)
{
  Dtb *dtb = gtk_type_new (dtb_get_type ()); 

  dtb_set_web100obj (dtb, web100obj);

  gtk_timeout_add (1000, (GtkFunction) dtb_timeout, dtb);

  return GTK_WIDGET (dtb); 
}

void dtb_set_web100obj (Dtb *dtb, Web100Obj *web100obj)
{
  g_return_if_fail (dtb != NULL);
  g_return_if_fail (IS_DTB (dtb)); 

// XXX
  if (!web100obj) {
    dtb->web100obj = NULL;
    return;
  }
  else
    g_return_if_fail (IS_WEB100_OBJ (web100obj));

  if (dtb->web100obj) { 
//    gtk_signal_disconnect_by_data (GTK_OBJECT (dtb->web100obj),
//                                   (gpointer) dtb);
    gtk_object_unref (GTK_OBJECT (dtb->web100obj));
  }

  dtb->web100obj = web100obj; 
  gtk_object_ref (GTK_OBJECT (web100obj)); 
  gtk_object_sink (GTK_OBJECT (web100obj)); 

/*
  gtk_signal_connect (GTK_OBJECT(web100obj),
                      "changed",
                      GTK_SIGNAL_FUNC(dtb_web100obj_changed),
                      dtb);

  gtk_signal_connect (GTK_OBJECT(web100obj),
                      "snap_update",
                      GTK_SIGNAL_FUNC(dtb_web100obj_snap_update),
                      dtb);
*/
}

Web100Obj* dtb_get_web100obj (Dtb *dtb)
{
  g_return_val_if_fail (dtb != NULL, NULL);
  g_return_val_if_fail (IS_DTB (dtb), NULL);

  return dtb->web100obj;
}

void dtb_sockset_listen (GtkObject *object, gpointer data)
{
  g_return_if_fail (object != NULL);
  g_return_if_fail (GTK_IS_OBJECT (object));
  g_return_if_fail (data != NULL);

  dtb_set_web100obj (DTB (data), SOCKSET (object)->web100obj); 
}

void dtb_update (GtkObject *object, gpointer *data) 
{
// do something
}

static guint dtb_timeout (gpointer data)
{
//  printf ("dtb's object cid: %d\n", DTB (data)->web100obj->cid);
}

void choose_tool(GtkWidget *parentclist, gint row, gint column,
                 GdkEventButton *event, gpointer data)
{
  char *name;


  name = (char *) gtk_clist_get_row_data(GTK_CLIST(parentclist), row);
//  printf("row #%d; name: %s\n", row, name);
  ur_launch_util (name, DTB (data)->web100obj, FALSE, FALSE);
}

static void dtb_destroy (GtkObject *object)
{
  g_return_if_fail (object != NULL);
  g_return_if_fail (IS_DTB(object));

  if (DTB (object)->web100obj != NULL) { 
//    gtk_signal_disconnect_by_data (GTK_OBJECT (DTB (object)->web100obj),
//                                   (gpointer) object);
    gtk_object_unref (GTK_OBJECT (DTB (object)->web100obj));
  }

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void dtb_init (Dtb *dtb)
{
  GtkWidget *hbox, *align, *frame, *button, *clist;
  gint hsize;
  int ii;

  char toollongname[NUM_TOOLNAME][128] = {
    "One counter/gauge display", "All variable display",
    "Connection properties", "Congestion pie chart",
    "Send tuning controls", "Receive tuning controls" };

  char varlongname[NUM_VARNAME][128] = {
    "Total Packets Received", "Data Packets Received",
    "Ack Packets Received", "Data Bytes Received",
    "Total Packets Transmitted", "Data Packets Transmitted",
    "Ack Packets Transmitted", "Data Bytes Transmitted",
    "Packets Retransmitted", "Bytes Retransmitted",
    "Duplicate Acks Received", "CurrentCwnd",
    "CurrentSsthresh", "SmoothedRTT", "CurrentRTO" };

//  char varname[NUM_VARNAME][128] = {
//  };

  gchar *ltext[1];

  if((ltext[0] = malloc(128)) == NULL){
    fprintf(stderr, "Out of memory!\n");
    exit(1);
  }

  GTK_WIDGET_SET_FLAGS (dtb, GTK_NO_WINDOW);
  dtb->web100obj = NULL; //XXX

//markup from here on  
  hbox = gtk_hbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(dtb), hbox, FALSE, FALSE, 10);
  gtk_widget_show(hbox);

  align = gtk_alignment_new(0.2,0.5,0.0,0.0);
  gtk_box_pack_start(GTK_BOX(hbox), align, FALSE, FALSE, 10);
  gtk_widget_show(align);

  frame = gtk_frame_new("Tool box");
  gtk_frame_set_label_align(GTK_FRAME(frame), 0.5, 0.0);
  gtk_container_add(GTK_CONTAINER(align), frame);
  gtk_widget_show(frame);

  dtb->toollist = gtk_clist_new(1);

//  hsize = gdk_string_width(font, "One counter/gauge display");
//  hsize += gdk_string_width(font, "w");
  gtk_clist_set_column_width(GTK_CLIST(dtb->toollist), 0, 120);
  gtk_clist_set_column_justification(GTK_CLIST(dtb->toollist), 0,
                                     GTK_JUSTIFY_CENTER);

  clist = dtb->toollist;

  for (ii=0; ii<NUM_TOOLNAME; ii++) {
    strcpy(ltext[0], toollongname[ii]);
    gtk_clist_append(GTK_CLIST(clist), ltext); 
    gtk_clist_set_row_data (GTK_CLIST (clist), ii, toolname[ii]);
  } 

  gtk_signal_connect(GTK_OBJECT(clist), "select-row",
                     GTK_SIGNAL_FUNC(choose_tool), dtb); 

  gtk_container_add(GTK_CONTAINER(frame),  dtb->toollist);
  gtk_widget_show(dtb->toollist);

  align = gtk_alignment_new(0.7,0.5,0.0,0.0);
  gtk_box_pack_end(GTK_BOX(hbox), align, FALSE, FALSE, 10);
  gtk_widget_show(align);
  frame = gtk_frame_new("Counter/gauge");
  gtk_frame_set_label_align(GTK_FRAME(frame), 0.5, 0.0);
  gtk_container_add(GTK_CONTAINER(align), frame);
  gtk_widget_show(frame);

  dtb->varlist = gtk_clist_new(1);
  clist = dtb->varlist;

//  hsize = gdk_string_width(font, "Total Packets Transmitted"); 
//  hsize += gdk_string_width(font, "w");
  gtk_clist_set_column_width(GTK_CLIST(clist), 0, 120);
  gtk_clist_set_column_justification(GTK_CLIST(clist), 0, GTK_JUSTIFY_CENTER);

  for (ii=0; ii<NUM_VARNAME; ii++) { 
    strcpy(ltext[0], varlongname[ii]);
    gtk_clist_append(GTK_CLIST(clist), ltext); 
  }
/*
  gtk_signal_connect(GTK_OBJECT(clist), "select-row",
                     GTK_SIGNAL_FUNC(choose_var), NULL);
  gtk_signal_connect(GTK_OBJECT(clist),
      "button_press_event",
      GTK_SIGNAL_FUNC(signal_handler_startvar),
      NULL);
*/
  gtk_container_add(GTK_CONTAINER(frame), dtb->varlist);
  gtk_widget_show(dtb->varlist);

  hbox = gtk_hbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(dtb), hbox, FALSE, FALSE, 10);
  gtk_widget_show(hbox);

  button = gtk_button_new_with_label(" Open window "); 
//  gtk_signal_connect(GTK_OBJECT(button), "clicked",
//                     GTK_SIGNAL_FUNC(launch_tool), NULL);
  gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, FALSE, 0);
  gtk_widget_show(button);
}















