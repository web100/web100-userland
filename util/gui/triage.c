
#include <stdlib.h>
#include <stdio.h> 
#include <gtk/gtk.h>

#include "web100.h"
#include "web100object.h" 
#include "web100widget.h"
#include "wcpie.h"
#include "triage.h"


#define SND 0
#define CNG 1
#define RCV 2

static void triage_class_init (TriageClass *class);
static void triage_init (Triage *triage);
static void triage_destroy (GtkObject *object);
static void triage_construct (Triage *triage, Web100Object *object);

static void triage_snap_update (GtkObject *object, gpointer data);
static void triage_set_object (Web100Widget *widget, Web100Object *object);
static void triage_connection_init (Triage *triage, Web100Object *object);

static GtkVBoxClass *parent_class = NULL;

static float ewma(float new_val, float old_ewma, float weight)
{
  float new_ewma;

  new_ewma = weight*new_val + (1-weight)*old_ewma;
  return(new_ewma);
}


#ifdef GTK2
GType
triage_get_type (void)
{
  static GType triage_type = 0;

  if (!triage_type)
  {
    static const GTypeInfo triage_info =
    {
      sizeof (TriageClass),
      NULL,
      NULL,
      (GClassInitFunc) triage_class_init,
      NULL,
      NULL,
      sizeof (Triage),
      0,
      (GInstanceInitFunc) triage_init,
    };

    triage_type = g_type_register_static (TYPE_WEB100_WIDGET, "Triage", &triage_info, 0);
  }

  return triage_type;
}
#else
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
        NULL,
        NULL
      };

      triage_type = gtk_type_unique (web100_widget_get_type (), &triage_info);
    }

  return triage_type;
}
#endif

static void triage_class_init (TriageClass *class)
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

  object_class->destroy = triage_destroy;

  web100widget_class->snap_update = triage_snap_update;

  web100widget_class->set_object = triage_set_object;
}

static void triage_init (Triage *triage)
{ 
  GTK_WIDGET_SET_FLAGS (triage, GTK_NO_WINDOW);
} 

GtkWidget* triage_new (Web100Object *web100obj)
{
  Triage *triage = gtk_type_new (triage_get_type ()); 

  web100_widget_set_object (WEB100_WIDGET (triage), web100obj);

  triage_construct (triage, web100obj);

  triage_connection_init (triage, web100obj);

  return GTK_WIDGET (triage); 
}

static void
triage_snap_update (GtkObject *object, gpointer data)
{ 
  Web100Object *web100obj; 
  Triage *triage;
  struct snapshot_list *snap;
  float per_snd, per_rcv, per_cng, total;
  char valtext[64];
  int ii, jj;

  web100obj = WEB100_OBJECT (object);
  triage = TRIAGE (data);

  if(WEB100_OBJECT (object)->cid != -1) { 
    snap = WEB100_OBJECT (object)->snapshot_head;
    while(snap) {
      if(!strcmp(snap->name, "read")) 
       	break; 
      snap = snap->next;
    } 

    if(snap->prior) {
      for(ii=0;ii<3;ii++)
	web100_delta_any(triage->wv[ii], snap->last, snap->prior, &triage->delta[ii]); 
    }
    for(ii=0;ii<3;ii++){ 
      triage->old_ewma[ii] = triage->new_ewma[ii]; 
      triage->new_ewma[ii] = ewma(triage->delta[ii], triage->old_ewma[ii], 0.4); 
    }
    total = triage->new_ewma[0] + triage->new_ewma[1] + triage->new_ewma[2];

    if(total){ 
      per_snd = triage->new_ewma[0]/total;
      per_cng = triage->new_ewma[1]/total;
      per_rcv = triage->new_ewma[2]/total;

      gtk_adjustment_set_value(WC_PIE(triage->pie)->adjustment[0], per_snd);
      gtk_adjustment_set_value(WC_PIE(triage->pie)->adjustment[1], per_cng);
      gtk_adjustment_set_value(WC_PIE(triage->pie)->adjustment[2], per_rcv);

      sprintf(valtext, "%.3f", 100*per_snd);
      gtk_clist_set_text(GTK_CLIST(triage->clist), 0, 1, valtext);
      sprintf(valtext, "%.3f", 100*per_rcv);
      gtk_clist_set_text(GTK_CLIST(triage->clist), 1, 1, valtext);
      sprintf(valtext, "%.3f", 100*per_cng);
      gtk_clist_set_text(GTK_CLIST(triage->clist), 2, 1, valtext);
    } 
  }
}

static void triage_destroy (GtkObject *object)
{
  g_return_if_fail (object != NULL);
  g_return_if_fail (IS_TRIAGE(object));

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void triage_set_object (Web100Widget *widget, Web100Object *object)
{
  Triage *triage;

  triage = TRIAGE (widget);

  web100_widget_set_object (widget, object);
  
  triage_connection_init (triage, object);
}

static void triage_connection_init (Triage *triage, Web100Object *object)
{
  web100_group *group;

  group = web100_group_find(WEB100_WIDGET (triage)->web100object->agent, "read");

  triage->wv[SND] = web100_var_find(group, "SndLimTimeSender");
  triage->wv[CNG] = web100_var_find(group, "SndLimTimeCwnd");
  triage->wv[RCV] = web100_var_find(group, "SndLimTimeRwin");
}

static void triage_construct (Triage *triage, Web100Object *web100obj)
{
  GtkWidget *hbox, *frame, *pie;
  GtkAdjustment *adjarray[3];
  char *itext[2] = {NULL, NULL};
  int ii;

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (triage), hbox, FALSE, FALSE, 10);
  gtk_widget_show (hbox);

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

  gtk_box_pack_start (GTK_BOX (hbox), triage->clist, TRUE, TRUE, 20);
  gtk_widget_show(triage->clist);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (triage), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  frame = gtk_frame_new(NULL);   
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN); 
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 20);
  gtk_widget_show(frame); 

  adjarray[0] = GTK_ADJUSTMENT(gtk_adjustment_new(0.0, 0.0, 1.0, 0.01, 0.1, 0));
  adjarray[1] = GTK_ADJUSTMENT(gtk_adjustment_new(0.0, 0.0, 1.0, 0.01, 0.1, 0));
  adjarray[2] = GTK_ADJUSTMENT(gtk_adjustment_new(0.0, 0.0, 1.0, 0.01, 0.1, 0)); 
  triage->pie = wc_pie_new(adjarray);
  wc_pie_set_update_policy (WC_PIE(triage->pie), GTK_UPDATE_CONTINUOUS);
  gtk_container_add(GTK_CONTAINER(frame), triage->pie);
  gtk_widget_show(triage->pie); 
}

















