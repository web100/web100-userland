
#include <stdlib.h>
#include <stdio.h> 
#include <gtk/gtk.h>

#include "web100.h"
#include "web100obj.h"
#include "web100gui.h" 
#include "foo_wgt.h"


#define FOO_DEFAULT_SIZE 300

static void foo_class_init (FooClass *class);
static void foo_init (Foo *foo);
static void foo_destroy (GtkObject *object);

static GtkVBoxClass *parent_class = NULL;


GtkType foo_get_type ()
{
  static guint foo_type = 0;

  if(!foo_type)
    {
      GtkTypeInfo foo_info =
      {
	"Foo",
	sizeof (Foo),
	sizeof (FooClass),
	(GtkClassInitFunc) foo_class_init,
	(GtkObjectInitFunc) foo_init,
        (GtkArgSetFunc) NULL,
        (GtkArgGetFunc) NULL
      };

      foo_type = gtk_type_unique (gtk_vbox_get_type (), &foo_info);
    }

  return foo_type;
}

static void foo_class_init (FooClass *class)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;

  object_class = (GtkObjectClass*) class;
  widget_class = (GtkWidgetClass*) class;

  parent_class = gtk_type_class (gtk_vbox_get_type ());

  object_class->destroy = foo_destroy;
}

static void foo_init (Foo *foo)
{
  GTK_WIDGET_SET_FLAGS (foo, GTK_NO_WINDOW);
  foo->web100obj = NULL;
}


GtkWidget* foo_new (Web100Obj *web100obj)
{
  Foo *foo = gtk_type_new (foo_get_type ()); 

//  foo_construct (foo, web100obj);
  foo_set_web100obj (foo, web100obj);

  return GTK_WIDGET (foo); 
}

static void foo_construct (Foo *foo, Web100Obj *web100obj)
{ 
 
  foo_set_web100obj (foo, web100obj);

} 

void foo_set_web100obj (Foo *foo, Web100Obj *web100obj)
{
  g_return_if_fail (foo != NULL);
  g_return_if_fail (IS_FOO (foo)); 

  if (!web100obj) {
    foo->web100obj = NULL;
    return;
  }
  else
    g_return_if_fail (IS_WEB100_OBJ (web100obj));

  if (foo->web100obj) { 
    gtk_signal_disconnect_by_data (GTK_OBJECT (foo->web100obj),
                                   (gpointer) foo);
    gtk_object_unref (GTK_OBJECT (foo->web100obj));
  }

  foo->web100obj = web100obj; 
  gtk_object_ref (GTK_OBJECT (web100obj)); 

  gtk_signal_connect (GTK_OBJECT(web100obj),
                      "changed",
                      GTK_SIGNAL_FUNC(foo_web100obj_changed),
                      foo);

  gtk_signal_connect (GTK_OBJECT(web100obj),
                      "snap_update",
                      GTK_SIGNAL_FUNC(foo_web100obj_snap_update),
                      foo);
}

Web100Obj* foo_get_web100obj (Foo *foo)
{
  g_return_val_if_fail (foo != NULL, NULL);
  g_return_val_if_fail (IS_FOO (foo), NULL);

  return foo->web100obj;
}

void foo_update (GtkObject *object, gpointer *data)
{
// do something
}

static void foo_destroy (GtkObject *object)
{
  g_return_if_fail (object != NULL);
  g_return_if_fail (IS_FOO(object));

  if (FOO (object)->web100obj != NULL) { 
    gtk_signal_disconnect_by_data (GTK_OBJECT (FOO (object)->web100obj),
                                   (gpointer) object);
    gtk_object_unref (GTK_OBJECT (FOO (object)->web100obj));
  }

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

