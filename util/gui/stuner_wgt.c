#include <stdlib.h>
#include <stdio.h> 
#include <math.h>
#include <gtk/gtk.h>

#include "web100.h" 
#include "web100obj.h"

#include "sockset_wgt.h"
#include "stuner_wgt.h"

#define TUNEMODE STUNEMODE_FIXED

static void stuner_class_init     (StunerClass *class);
static void stuner_init           (Stuner *stuner); 
static void stuner_set_buf_value  (GtkWidget *button, gpointer data);
static void stuner_value_changed  (GtkAdjustment *adj, gpointer data);
static void stuner_destroy        (GtkObject *object);

static GtkVBoxClass *parent_class = NULL;

static float factory_default = 0.313780;


GtkType stuner_get_type ()
{
  static guint stuner_type = 0;

  if(!stuner_type)
    {
      GtkTypeInfo stuner_info =
      {
	"Stuner",
	sizeof (Stuner),
	sizeof (StunerClass),
	(GtkClassInitFunc) stuner_class_init,
	(GtkObjectInitFunc) stuner_init,
        (GtkArgSetFunc) NULL,
        (GtkArgGetFunc) NULL
      };

      stuner_type = gtk_type_unique (gtk_vbox_get_type (), &stuner_info);
    } 
  return stuner_type;
}

static void stuner_class_init (StunerClass *class)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;

  object_class = (GtkObjectClass*) class;
  widget_class = (GtkWidgetClass*) class;

  parent_class = gtk_type_class (gtk_vbox_get_type ());

  object_class->destroy = stuner_destroy;
}

GtkWidget* stuner_new (Web100Obj *web100obj)
{
  Stuner *stuner = gtk_type_new (stuner_get_type ()); 

  stuner_set_web100obj (stuner, web100obj);

  return GTK_WIDGET (stuner); 
}

void stuner_set_web100obj (Stuner *stuner, Web100Obj *web100obj)
{
  web100_group *gp;

  g_return_if_fail (stuner != NULL);
  g_return_if_fail (IS_STUNER (stuner)); 
  g_return_if_fail (IS_WEB100_OBJ (web100obj));

  if (stuner->web100obj)
    gtk_object_unref (GTK_OBJECT (stuner->web100obj)); 

  stuner->web100obj = web100obj; 
  gtk_object_ref (GTK_OBJECT (web100obj)); 
  gtk_object_sink (GTK_OBJECT (web100obj)); 

  if ((gp = web100_group_find (stuner->web100obj->agent, "tune")) == NULL) {
    web100_perror ("stuner_set_web100obj: web100_group_find");
    return;
  } 
  if ((stuner->bufset = web100_var_find (gp, "SndbufSet")) == NULL) {
    web100_perror ("stuner_set_web100obj: web100_var_find");
    return;
  }
  if ((stuner->tunemode = web100_var_find (gp, "STuneMode")) == NULL) {
    web100_perror ("stuner_set_web100obj: web100_var_find");
    return;
  }
}

Web100Obj* stuner_get_web100obj (Stuner *stuner)
{
  g_return_val_if_fail (stuner != NULL, NULL);
  g_return_val_if_fail (IS_STUNER (stuner), NULL);

  return stuner->web100obj;
}

static void stuner_set_buf_value (GtkWidget *button, gpointer data)
{ 
  Stuner *stuner = STUNER (data);
  int tunemode = TUNEMODE;

  if (web100_raw_write (stuner->bufset, stuner->web100obj->connection, &stuner->buf_val) < 0) {
    web100_perror ("set_buf_value: web100_raw_write");
  }
  if (web100_raw_write (stuner->tunemode, stuner->web100obj->connection, &tunemode) < 0) { 
    web100_perror ("set_buf_value: web100_raw_write");
  } 
}

static void stuner_value_changed (GtkAdjustment *adj, gpointer data)
{
  Stuner *stuner;
  float ll = log(4000000)-log(10000), aa = log(10000);
  char buf_val_text[40]; 

  stuner = STUNER (data);

  stuner->scale_val = adj->value;
  stuner->buf_val = floor(exp(ll*(adj->value) + aa));
  sprintf(buf_val_text, "%u", (stuner->buf_val));

  gtk_label_set_text (GTK_LABEL (stuner->label), buf_val_text);
}

static void stuner_destroy (GtkObject *object)
{
  g_return_if_fail (object != NULL);
  g_return_if_fail (IS_STUNER(object));

  if (STUNER (object)->web100obj != NULL)
    gtk_object_unref (GTK_OBJECT (STUNER (object)->web100obj)); 

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

void stuner_sockset_listen (GtkObject *object, gpointer data)
{ 
  g_return_if_fail (object != NULL);
  g_return_if_fail (GTK_IS_OBJECT (object));
  g_return_if_fail (data != NULL);

  stuner_set_web100obj (STUNER (data), SOCKSET (object)->web100obj);
}

static void stuner_init (Stuner *stuner)
{
  GtkWidget *scale, *hbox;

  GTK_WIDGET_SET_FLAGS (stuner, GTK_NO_WINDOW);
//  stuner->web100obj = NULL; 

  stuner->scale_val = factory_default;

  stuner->adjustment = GTK_ADJUSTMENT (gtk_adjustment_new(stuner->scale_val, 
                                       0.0, 1.0, 0.01, 0.1, 0.0)); 
  gtk_signal_connect (GTK_OBJECT (stuner->adjustment), "value_changed",
                      GTK_SIGNAL_FUNC (stuner_value_changed), stuner);

  scale = gtk_hscale_new (stuner->adjustment);
  gtk_scale_set_draw_value (GTK_SCALE (scale), FALSE); 
  gtk_scale_set_digits (GTK_SCALE (scale), 10); 
  gtk_box_pack_start (GTK_BOX (stuner), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  stuner->label = gtk_label_new (NULL);
  gtk_box_pack_start (GTK_BOX (stuner), stuner->label, FALSE, FALSE, 5); 
  gtk_widget_show (stuner->label);

  gtk_signal_emit_by_name (GTK_OBJECT (stuner->adjustment), "value_changed"); 

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (stuner), hbox, FALSE, FALSE, 5);
  gtk_widget_show (hbox);

  stuner->write_button = gtk_button_new_with_label (" Write to buf ");
  gtk_signal_connect (GTK_OBJECT (stuner->write_button), "clicked",
                      GTK_SIGNAL_FUNC (stuner_set_buf_value), stuner);
  gtk_box_pack_start (GTK_BOX (hbox), stuner->write_button, TRUE, FALSE, 0);
  gtk_widget_show (stuner->write_button);
} 











