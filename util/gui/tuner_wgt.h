#ifndef __FOO_H__
#define __FOO_H__


#include <gdk/gdk.h>
#include <gtk/gtkvbox.h> 
#include <gtk/gtkobject.h>
#include "web100obj.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define FOO(obj)          GTK_CHECK_CAST (obj, foo_get_type (), Foo)
#define FOO_CLASS(class)  GTK_CHECK_CLASS_CAST (class, foo_get_type (), FooClass)
#define IS_FOO(obj)       GTK_CHECK_TYPE (obj, foo_get_type ())

typedef struct _Foo       Foo;
typedef struct _FooClass  FooClass;

struct _Foo
{
  GtkVBox vbox;
  
  Web100Obj *web100obj;
};

struct _FooClass
{ 
  GtkVBoxClass parent_class; 
};

GtkType     foo_get_type       (void); 
GtkWidget*  foo_new            (Web100Obj *);
void        foo_update         (GtkObject *, gpointer *);
void        foo_set_web100obj (Foo *, Web100Obj *);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FOO_H__ */ 
