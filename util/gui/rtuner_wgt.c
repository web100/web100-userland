#include <stdlib.h>
#include <stdio.h> 
#include <math.h>
#include <gtk/gtk.h>

#include "web100.h" 
#include "web100obj.h"

#include "sockset_wgt.h"
#include "rtuner_wgt.h"

#define TUNEMODE RTUNEMODE_FIXED

static void rtuner_class_init     (RtunerClass *class);
static void rtuner_init           (Rtuner *rtuner); 
static void rtuner_set_buf_value  (GtkWidget *button, gpointer data);
static void rtuner_value_changed  (GtkAdjustment *adj, gpointer data);
static void rtuner_destroy        (GtkObject *object);

static GtkVBoxClass *parent_class = NULL;

//static GtkWidget *label; 
static float factory_default = 0.313780;


GtkType rtuner_get_type ()
{
  static guint rtuner_type = 0;

  if(!rtuner_type)
    {
      GtkTypeInfo rtuner_info =
      {
	"Rtuner",
	sizeof (Rtuner),
	sizeof (RtunerClass),
	(GtkClassInitFunc) rtuner_class_init,
	(GtkObjectInitFunc) rtuner_init,
        (GtkArgSetFunc) NULL,
        (GtkArgGetFunc) NULL
      };

      rtuner_type = gtk_type_unique (gtk_vbox_get_type (), &rtuner_info);
    } 
  return rtuner_type;
}

static void rtuner_class_init (RtunerClass *class)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;

  object_class = (GtkObjectClass*) class;
  widget_class = (GtkWidgetClass*) class;

  parent_class = gtk_type_class (gtk_vbox_get_type ());

  object_class->destroy = rtuner_destroy;
}

GtkWidget* rtuner_new (Web100Obj *web100obj)
{
  Rtuner *rtuner = gtk_type_new (rtuner_get_type ()); 

  rtuner_set_web100obj (rtuner, web100obj);

  return GTK_WIDGET (rtuner); 
}

void rtuner_set_web100obj (Rtuner *rtuner, Web100Obj *web100obj)
{
  web100_group *gp;

  g_return_if_fail (rtuner != NULL);
  g_return_if_fail (IS_RTUNER (rtuner)); 
  g_return_if_fail (IS_WEB100_OBJ (web100obj));

  if (rtuner->web100obj)
    gtk_object_unref (GTK_OBJECT (rtuner->web100obj)); 

  rtuner->web100obj = web100obj; 
  gtk_object_ref (GTK_OBJECT (web100obj)); 
  gtk_object_sink (GTK_OBJECT (web100obj)); 

  if ((gp = web100_group_find (rtuner->web100obj->agent, "tune")) == NULL) {
    web100_perror ("rtuner_set_web100obj: web100_group_find");
    return;
  } 
  if ((rtuner->bufset = web100_var_find (gp, "RcvbufSet")) == NULL) {
    web100_perror ("rtuner_set_web100obj: web100_var_find");
    return;
  }
  if ((rtuner->tunemode = web100_var_find (gp, "RTuneMode")) == NULL) {
    web100_perror ("rtuner_set_web100obj: web100_var_find");
    return;
  }
}

Web100Obj* rtuner_get_web100obj (Rtuner *rtuner)
{
  g_return_val_if_fail (rtuner != NULL, NULL);
  g_return_val_if_fail (IS_RTUNER (rtuner), NULL);

  return rtuner->web100obj;
}

static void rtuner_set_buf_value (GtkWidget *button, gpointer data)
{ 
  Rtuner *rtuner = RTUNER (data);
  int tunemode = TUNEMODE;

  if (web100_raw_write (rtuner->bufset, rtuner->web100obj->connection, &rtuner->buf_val) < 0) {
    web100_perror ("set_buf_value: web100_raw_write");
  }
  if (web100_raw_write (rtuner->tunemode, rtuner->web100obj->connection, &tunemode) < 0) { 
    web100_perror ("set_buf_value: web100_raw_write");
  } 
}

static void rtuner_value_changed (GtkAdjustment *adj, gpointer data)
{
  Rtuner *rtuner;
  float ll = log(4000000)-log(10000), aa = log(10000);
  char buf_val_text[40]; 

  rtuner = RTUNER (data);

  rtuner->scale_val = adj->value;
  rtuner->buf_val = floor(exp(ll*(adj->value) + aa));
  sprintf(buf_val_text, "%u", (rtuner->buf_val));

  gtk_label_set_text (GTK_LABEL (rtuner->label), buf_val_text);
}

static void rtuner_destroy (GtkObject *object)
{
  g_return_if_fail (object != NULL);
  g_return_if_fail (IS_RTUNER(object));

  if (RTUNER (object)->web100obj != NULL)
    gtk_object_unref (GTK_OBJECT (RTUNER (object)->web100obj)); 

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

void rtuner_sockset_listen (GtkObject *object, gpointer data)
{ 
  g_return_if_fail (object != NULL);
  g_return_if_fail (GTK_IS_OBJECT (object));
  g_return_if_fail (data != NULL);

  rtuner_set_web100obj (RTUNER (data), SOCKSET (object)->web100obj);
}

static void rtuner_init (Rtuner *rtuner)
{
  GtkWidget *scale;
  GtkWidget *hbox;

  GTK_WIDGET_SET_FLAGS (rtuner, GTK_NO_WINDOW);
//  rtuner->web100obj = NULL; 

  rtuner->scale_val = factory_default;

  rtuner->adjustment = GTK_ADJUSTMENT (gtk_adjustment_new(rtuner->scale_val, 
                                       0.0, 1.0, 0.01, 0.1, 0.0)); 
  gtk_signal_connect (GTK_OBJECT (rtuner->adjustment), "value_changed",
                      GTK_SIGNAL_FUNC (rtuner_value_changed), rtuner);

  scale = gtk_hscale_new (rtuner->adjustment);
  gtk_scale_set_draw_value (GTK_SCALE (scale), FALSE); 
  gtk_scale_set_digits (GTK_SCALE (scale), 10); 
  gtk_box_pack_start (GTK_BOX (rtuner), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  rtuner->label = gtk_label_new (NULL);
  gtk_box_pack_start (GTK_BOX (rtuner), rtuner->label, FALSE, FALSE, 5); 
  gtk_widget_show (rtuner->label);

  gtk_signal_emit_by_name (GTK_OBJECT (rtuner->adjustment), "value_changed"); 

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (rtuner), hbox, FALSE, FALSE, 5);
  gtk_widget_show (hbox);

  rtuner->write_button = gtk_button_new_with_label (" Write to buf ");
  gtk_signal_connect (GTK_OBJECT (rtuner->write_button), "clicked",
                      GTK_SIGNAL_FUNC (rtuner_set_buf_value), rtuner);
  gtk_box_pack_start (GTK_BOX (hbox), rtuner->write_button, TRUE, FALSE, 0);
  gtk_widget_show (rtuner->write_button);
} 











