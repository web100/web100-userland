#ifndef __CPR_H__
#define __CPR_H__


#include <gdk/gdk.h>
#include <gtk/gtkvbox.h> 
#include <gtk/gtkobject.h>
#include "web100obj.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define CPR(obj)          GTK_CHECK_CAST (obj, cpr_get_type (), Cpr)
#define CPR_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass, cpr_get_type (), CprClass)
#define IS_CPR(obj)       GTK_CHECK_TYPE (obj, cpr_get_type ())

typedef struct _Cpr       Cpr;
typedef struct _CprClass  CprClass;

struct _Cpr
{
  GtkVBox vbox;
  
  Web100Obj *web100obj;
  GtkWidget *scrollwin;
  GtkWidget *varlist;
};

struct _CprClass
{ 
  GtkVBoxClass parent_class; 
};

GtkType     cpr_get_type       (void); 
GtkWidget*  cpr_new            (Web100Obj *);
void        cpr_update         (GtkObject *, gpointer *);
void        cpr_set_web100obj  (Cpr *, Web100Obj *);
void        cpr_sockset_listen (GtkObject *object, gpointer data);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __CPR_H__ */ 
