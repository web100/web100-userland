
#include <stdlib.h>
#include <stdio.h> 
#include <gtk/gtk.h>

#include "web100.h"
#include "web100obj.h" 
#include "triage_wgt.h"


#define TRIAGE_DEFAULT_SIZE 300

static void triage_class_init (TriageClass *class);
static void triage_init (Triage *triage);
static void triage_destroy (GtkObject *object);

static GtkVBoxClass *parent_class = NULL;


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

static void triage_init (Triage *triage)
{
  GTK_WIDGET_SET_FLAGS (triage, GTK_NO_WINDOW);
  triage->web100obj = NULL;
}


GtkWidget* triage_new (Web100Obj *web100obj)
{
  Triage *triage = gtk_type_new (triage_get_type ()); 

//  triage_construct (triage, web100obj);
  triage_set_web100obj (triage, web100obj);

  return GTK_WIDGET (triage); 
}

static void triage_construct (Triage *triage, Web100Obj *web100obj)
{ 
 
  triage_set_web100obj (triage, web100obj);

} 

void triage_set_web100obj (Triage *triage, Web100Obj *web100obj)
{
  g_return_if_fail (triage != NULL);
  g_return_if_fail (IS_TRIAGE (triage)); 

  if (!web100obj) {
    triage->web100obj = NULL;
    return;
  }
  else
    g_return_if_fail (IS_WEB100_OBJ (web100obj));

  if (triage->web100obj) { 
    gtk_signal_disconnect_by_data (GTK_OBJECT (triage->web100obj),
                                   (gpointer) triage);
    gtk_object_unref (GTK_OBJECT (triage->web100obj));
  }

  triage->web100obj = web100obj; 
  gtk_object_ref (GTK_OBJECT (web100obj)); 

  gtk_signal_connect (GTK_OBJECT(web100obj),
                      "changed",
                      GTK_SIGNAL_FUNC(triage_web100obj_changed),
                      triage);

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

