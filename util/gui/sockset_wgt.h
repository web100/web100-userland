#ifndef __SOCKSET_H__
#define __SOCKSET_H__


#include <gdk/gdk.h>
#include <gtk/gtkvbox.h> 
#include <gtk/gtkobject.h>
#include "web100obj.h"
#include "cnlst_wgt.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define SOCKSET(obj)          GTK_CHECK_CAST (obj, sockset_get_type (), Sockset)
#define SOCKSET_CLASS(class)  GTK_CHECK_CLASS_CAST (class, sockset_get_type (), SocksetClass)
#define IS_SOCKSET(obj)       GTK_CHECK_TYPE (obj, sockset_get_type ())

typedef struct _Sockset       Sockset;
typedef struct _SocksetClass  SocksetClass;

struct _Sockset
{
  GtkVBox vbox;
  
  Web100Obj *web100obj;
  Cnlst     *cnlst;
  GtkWidget *sessentry, *cidentry;
};

struct _SocksetClass
{ 
  GtkVBoxClass parent_class; 

  void (* web100obj_changed) (Sockset *sockset);
};

GtkType     sockset_get_type       (void); 
GtkWidget*  sockset_new            (Web100Obj *, gboolean malleable);
void        sockset_update         (GtkObject *, gpointer *);
void        sockset_set_web100obj  (Sockset *, Web100Obj *);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __SOCKSET_H__ */ 
