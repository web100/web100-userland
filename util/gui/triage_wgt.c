
#include <stdlib.h>
#include <stdio.h> 
#include <gtk/gtk.h>

#include "web100.h"
#include "web100obj.h" 
#include "sockset_wgt.h"
#include "wcpie.h"
#include "triage_wgt.h"


#define SND 0
#define CNG 1
#define RCV 2

static void triage_class_init (TriageClass *class);
static void triage_init (Triage *triage);
static void triage_destroy (GtkObject *object);
static void triage_web100obj_snap_update (GtkObject *object, gpointer *data);

static GtkVBoxClass *parent_class = NULL;

unsigned long long int val[3], delta[3];

float new_ewma[3], old_ewma[3], total;
char valtext[40];

float ewma(float new_val, float old_ewma, float weight)
{
  float new_ewma;

  new_ewma = weight*new_val + (1-weight)*old_ewma;
  return(new_ewma);
}


GtkType triage_get_type ()
{
  static guint triage_type = 0;

  if(!triage_type)
    {
      GtkTypeInfo triage_info =
      {
	"Triage",
	sizeof (Triage),
	sizeof (TriageClass),
	(GtkClassInitFunc) triage_class_init,
	(GtkObjectInitFunc) triage_init,
        (GtkArgSetFunc) NULL,
        (GtkArgGetFunc) NULL
      };

      triage_type = gtk_type_unique (gtk_vbox_get_type (), &triage_info);
    }

  return triage_type;
}

static void triage_class_init (TriageClass *class)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;

  object_class = (GtkObjectClass*) class;
  widget_class = (GtkWidgetClass*) class;

  parent_class = gtk_type_class (gtk_vbox_get_type ());

  object_class->destroy = triage_destroy;
}

GtkWidget* triage_new (Web100Obj *web100obj)
{
  Triage *triage = gtk_type_new (triage_get_type ()); 

  triage_set_web100obj (triage, web100obj);

  return GTK_WIDGET (triage); 
}

void triage_set_web100obj (Triage *triage, Web100Obj *web100obj)
{
  web100_group *group;
  int ii;

  g_return_if_fail (triage != NULL);
  g_return_if_fail (IS_TRIAGE (triage)); 

  g_return_if_fail (web100obj != NULL);
  g_return_if_fail (IS_WEB100_OBJ (web100obj));

  if (triage->web100obj) { 
    gtk_signal_disconnect_by_data (GTK_OBJECT (triage->web100obj),
                                   (gpointer) triage);
    gtk_object_unref (GTK_OBJECT (triage->web100obj));
  }

  triage->web100obj = web100obj; 
  gtk_object_ref (GTK_OBJECT (web100obj)); 
  gtk_object_sink (GTK_OBJECT (web100obj));

  group = web100_group_find(web100obj->agent, "read");

  triage->wv[SND] = web100_var_find(group, "SndLimTimeSender");
  triage->wv[CNG] = web100_var_find(group, "SndLimTimeCwnd");
  triage->wv[RCV] = web100_var_find(group, "SndLimTimeRwin");

  gtk_signal_connect (GTK_OBJECT(web100obj),
                      "snap_update",
                      GTK_SIGNAL_FUNC(triage_web100obj_snap_update),
                      triage); 
	
}

Web100Obj* triage_get_web100obj (Triage *triage)
{
  g_return_val_if_fail (triage != NULL, NULL);
  g_return_val_if_fail (IS_TRIAGE (triage), NULL);

  return triage->web100obj;
}

void triage_update (GtkObject *object, gpointer *data)
{
// do something
} 

void triage_web100obj_snap_update (GtkObject *object, gpointer *data)
{ 
  Web100Obj *web100obj; 
  Triage *triage;
  struct snapshot_data *snap;
  float per_snd, per_rcv, per_cng;
  int ii, jj; 

  web100obj = WEB100_OBJ (object);
  triage = TRIAGE (data);

  if(WEB100_OBJ (object)->connection) { 
    snap = WEB100_OBJ (object)->snapshot_head;
    while(snap) {
      if(!strcmp(snap->name, "read")) 
       	break; 
      snap = snap->next;
    } 

    if(snap->prior) {
      for(ii=0;ii<3;ii++)
	web100_delta_any(triage->wv[ii], snap->last, snap->prior, &delta[ii]); 
    }
    for(ii=0;ii<3;ii++){ 
      old_ewma[ii] = new_ewma[ii]; 
      new_ewma[ii] = ewma(delta[ii], old_ewma[ii], 0.4); 
    }
    total = new_ewma[0] + new_ewma[1] + new_ewma[2];

    if(total){ 
      per_snd = new_ewma[0]/total;
      per_cng = new_ewma[1]/total;
      per_rcv = new_ewma[2]/total;

      gtk_adjustment_set_value(WC_PIE(triage->pie)->adjustment[0], new_ewma[0]/total);
      gtk_adjustment_set_value(WC_PIE(triage->pie)->adjustment[1], new_ewma[1]/total);
      gtk_adjustment_set_value(WC_PIE(triage->pie)->adjustment[2], new_ewma[2]/total);

      sprintf(valtext, "%.3f", per_snd);
      gtk_clist_set_text(GTK_CLIST(triage->clist), 0, 1, valtext);
      sprintf(valtext, "%.3f", per_rcv);
      gtk_clist_set_text(GTK_CLIST(triage->clist), 1, 1, valtext);
      sprintf(valtext, "%.3f", per_cng);
      gtk_clist_set_text(GTK_CLIST(triage->clist), 2, 1, valtext);
    }

//    gtk_widget_show(triage->pie); 

  }
}

void triage_sockset_listen (GtkObject *object, gpointer data)
{ 
  g_return_if_fail (object != NULL);
  g_return_if_fail (GTK_IS_OBJECT (object));
  g_return_if_fail (data != NULL); 

  triage_set_web100obj (TRIAGE (data), SOCKSET (object)->web100obj);
}

static void triage_destroy (GtkObject *object)
{
  g_return_if_fail (object != NULL);
  g_return_if_fail (IS_TRIAGE(object));

  if (TRIAGE (object)->web100obj != NULL) { 
    gtk_signal_disconnect_by_data (GTK_OBJECT (TRIAGE (object)->web100obj),
                                   (gpointer) object);
    gtk_object_unref (GTK_OBJECT (TRIAGE (object)->web100obj));
  }

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void triage_init (Triage *triage)
{
  GtkWidget *align, *frame, *pie;
  GtkAdjustment *adjarray[3];
  char *itext[2] = {NULL, NULL};
  int ii;

     
  GTK_WIDGET_SET_FLAGS (triage, GTK_NO_WINDOW);
  triage->web100obj = NULL;
/*
  hbox = gtk_hbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(triage), hbox, TRUE, TRUE, 10);
  gtk_widget_show(hbox);

  vbox = gtk_vbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
  gtk_widget_show(vbox); 
*/
  align = gtk_alignment_new(0.5,0.5,1.0,0.0);
  gtk_box_pack_start(GTK_BOX(triage), align, TRUE, FALSE, 10); 
  gtk_widget_show(align);

  triage->clist = gtk_clist_new(2); 
  gtk_clist_set_column_width(GTK_CLIST(triage->clist), 0, 40);
  gtk_clist_set_column_width(GTK_CLIST(triage->clist), 1, 40);
  gtk_clist_set_column_justification(GTK_CLIST(triage->clist), 0, GTK_JUSTIFY_RIGHT);
  gtk_clist_set_column_justification(GTK_CLIST(triage->clist), 1, GTK_JUSTIFY_RIGHT);

  for(ii=0;ii<3;ii++)
    gtk_clist_append(GTK_CLIST(triage->clist), itext);

  gtk_clist_set_text(GTK_CLIST(triage->clist), 0, 0, "%Snd");
  gtk_clist_set_text(GTK_CLIST(triage->clist), 1, 0, "%Rcv");
  gtk_clist_set_text(GTK_CLIST(triage->clist), 2, 0, "%Cng");

  gtk_container_add(GTK_CONTAINER(align), triage->clist);
  gtk_widget_show(triage->clist);

  align = gtk_alignment_new(0.5,0.5,0.0,0.0);

  frame = gtk_frame_new(NULL);   
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN); 
  gtk_container_add(GTK_CONTAINER(align), frame); 
  gtk_widget_show(frame); 

  adjarray[0] = GTK_ADJUSTMENT(gtk_adjustment_new(0.0, 0.0, 1.0, 0.01, 0.1, 0));
  adjarray[1] = GTK_ADJUSTMENT(gtk_adjustment_new(0.0, 0.0, 1.0, 0.01, 0.1, 0));
  adjarray[2] = GTK_ADJUSTMENT(gtk_adjustment_new(0.0, 0.0, 1.0, 0.01, 0.1, 0)); 
  triage->pie = wc_pie_new(adjarray);
  wc_pie_set_update_policy (WC_PIE(triage->pie), GTK_UPDATE_CONTINUOUS);
  gtk_container_add(GTK_CONTAINER(frame), triage->pie);
  gtk_widget_show(triage->pie); 
  gtk_box_pack_start(GTK_BOX(triage), align,
      FALSE, FALSE, 10);
  gtk_widget_show(align);

}

















