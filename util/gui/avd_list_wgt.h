#ifndef __AVD_LIST_H__
#define __AVD_LIST_H__


#include <gdk/gdk.h>
#include <gtk/gtkvbox.h> 
#include <gtk/gtkobject.h>
#include "web100obj.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define AVD_LIST(obj)          GTK_CHECK_CAST (obj, avd_list_get_type (), Avd_list)
#define AVD_LIST_CLASS(class)  GTK_CHECK_CLASS_CAST (class, avd_list_get_type (), Avd_listClass)
#define IS_AVD_LIST(obj)       GTK_CHECK_TYPE (obj, avd_list_get_type ())

typedef struct _Avd_list       Avd_list;
typedef struct _Avd_listClass  Avd_listClass; 

struct _Avd_list
{
  GtkVBox vbox;
  
  Web100Obj *web100obj;
  GtkWidget *scrollwin;
  GtkWidget *varlist;

  GtkWidget *reset_button;
};

struct _Avd_listClass
{ 
  GtkVBoxClass parent_class; 
};

GtkType     avd_list_get_type       (void); 
GtkWidget*  avd_list_new            (Web100Obj *);
void        avd_list_update         (GtkObject *, gpointer *);
void        avd_list_set_web100obj (Avd_list *, Web100Obj *);
void       avd_list_sockset_listen (GtkObject *object, gpointer data);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __AVD_LIST_H__ */ 
