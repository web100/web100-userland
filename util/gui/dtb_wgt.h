#ifndef __DTB_H__
#define __DTB_H__


#include <gdk/gdk.h>
#include <gtk/gtkvbox.h> 
#include <gtk/gtkobject.h>
#include "web100obj.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define DTB(obj)          GTK_CHECK_CAST (obj, dtb_get_type (), Dtb)
#define DTB_CLASS(class)  GTK_CHECK_CLASS_CAST (class, dtb_get_type (), DtbClass)
#define IS_DTB(obj)       GTK_CHECK_TYPE (obj, dtb_get_type ())

typedef struct _Dtb       Dtb;
typedef struct _DtbClass  DtbClass;

struct _Dtb
{
  GtkVBox vbox;
  
  Web100Obj *web100obj;

  GtkWidget *toollist, *varlist;
};

struct _DtbClass
{ 
  GtkVBoxClass parent_class; 
};

GtkType     dtb_get_type       (void); 
GtkWidget*  dtb_new            (Web100Obj *);
void        dtb_update         (GtkObject *, gpointer *);
void        dtb_set_web100obj (Dtb *, Web100Obj *);
void dtb_sockset_listen (GtkObject *object, gpointer data);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __DTB_H__ */ 
