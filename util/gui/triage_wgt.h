#ifndef __TRIAGE_H__
#define __TRIAGE_H__


#include <gdk/gdk.h>
#include <gtk/gtkvbox.h> 
#include <gtk/gtkobject.h> 
#include "web100obj.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define TRIAGE(obj)          GTK_CHECK_CAST (obj, triage_get_type (), Triage)
#define TRIAGE_CLASS(class)  GTK_CHECK_CLASS_CAST (class, triage_get_type (), TriageClass)
#define IS_TRIAGE(obj)       GTK_CHECK_TYPE (obj, triage_get_type ())

typedef struct _Triage       Triage;
typedef struct _TriageClass  TriageClass;

struct _Triage
{
  GtkVBox vbox;
  
  Web100Obj *web100obj;

  web100_var *wv[3];
  GtkWidget  *pie, *clist;
};

struct _TriageClass
{ 
  GtkVBoxClass parent_class; 
};

GtkType     triage_get_type       (void); 
GtkWidget*  triage_new            (Web100Obj *);
void        triage_update         (GtkObject *, gpointer *);
void        triage_set_web100obj  (Triage *, Web100Obj *);
void        triage_sockset_listen (GtkObject *, gpointer);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __TRIAGE_H__ */ 
