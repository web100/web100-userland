#ifndef __VDT_H__
#define __VDT_H__


#include <gdk/gdk.h>
#include <gtk/gtkvbox.h> 
#include <gtk/gtkobject.h>
#include "web100.h"
#include "web100obj.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define VDT(obj)          GTK_CHECK_CAST (obj, vdt_get_type (), Vdt)
#define VDT_CLASS(class)  GTK_CHECK_CLASS_CAST (class, vdt_get_type (), VdtClass)
#define IS_VDT(obj)       GTK_CHECK_TYPE (obj, vdt_get_type ())

typedef struct _Vdt       Vdt;
typedef struct _VdtClass  VdtClass;

struct _Vdt
{
  GtkVBox vbox;
  
  Web100Obj *web100obj;

  web100_var *var;
  web100_group *group;

  GtkWidget *graph;
  GtkWidget *clist, *entry;
  float ewma, oldewma, max, oldmax;
  float graphval[20];
};

struct _VdtClass
{ 
  GtkVBoxClass parent_class; 
};

GtkType     vdt_get_type       (void); 
GtkWidget*  vdt_new            (Web100Obj *, char *);
void        vdt_update         (GtkObject *, gpointer *);
void        vdt_set_web100obj  (Vdt *, Web100Obj *);
void        vdt_sockset_listen (GtkObject *, gpointer);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __VDT_H__ */ 
