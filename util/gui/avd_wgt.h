#ifndef __AVD_H__
#define __AVD_H__


#include <gdk/gdk.h>
#include <gtk/gtkvbox.h> 
#include <gtk/gtkobject.h>
#include "web100obj.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define AVD(obj)          GTK_CHECK_CAST (obj, avd_get_type (), Avd)
#define AVD_CLASS(class)  GTK_CHECK_CLASS_CAST (class, avd_get_type (), AvdClass)
#define IS_AVD(obj)       GTK_CHECK_TYPE (obj, avd_get_type ())

typedef struct _Avd       Avd;
typedef struct _AvdClass  AvdClass;

struct _Avd
{
  GtkVBox vbox;
  
  Web100Obj *web100obj;
  GtkWidget *scrollwin;
  GtkWidget *varlist;
};

struct _AvdClass
{ 
  GtkVBoxClass parent_class; 
};

GtkType     avd_get_type       (void); 
GtkWidget*  avd_new            (Web100Obj *);
void        avd_update         (GtkObject *, gpointer *);
void        avd_set_web100obj (Avd *, Web100Obj *);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __AVD_H__ */ 
