#ifndef __AVD_TABLE_H__
#define __AVD_TABLE_H__


#include <gdk/gdk.h>
#include <gtk/gtkvbox.h> 
#include <gtk/gtkobject.h>
#include "web100obj.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define AVD_TABLE(obj)          GTK_CHECK_CAST (obj, avd_table_get_type (), Avd_table)
#define AVD_TABLE_CLASS(class)  GTK_CHECK_CLASS_CAST (class, avd_table_get_type (), Avd_tableClass)
#define IS_AVD_TABLE(obj)       GTK_CHECK_TYPE (obj, avd_table_get_type ())

typedef struct _Avd_table       Avd_table;
typedef struct _Avd_tableClass  Avd_tableClass; 

struct _Avd_table
{
  GtkVBox vbox;
  
  Web100Obj *web100obj;
  GtkWidget *scrollwin;
  GtkWidget *varlist;

  GtkWidget *reset_button;
};

struct _Avd_tableClass
{ 
  GtkVBoxClass parent_class; 
};

GtkType     avd_table_get_type       (void); 
GtkWidget*  avd_table_new            (Web100Obj *);
void        avd_table_update         (GtkObject *, gpointer *);
void        avd_table_set_web100obj (Avd_table *, Web100Obj *);
void       avd_table_sockset_listen (GtkObject *object, gpointer data);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __AVD_TABLE_H__ */ 
