#ifndef __STUNER_H__
#define __STUNER_H__


#include <gdk/gdk.h>
#include <gtk/gtkvbox.h> 
#include <gtk/gtkobject.h>
#include "web100obj.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define STUNER(obj)          GTK_CHECK_CAST (obj, stuner_get_type (), Stuner)
#define STUNER_CLASS(class)  GTK_CHECK_CLASS_CAST (class, stuner_get_type (), StunerClass)
#define IS_STUNER(obj)       GTK_CHECK_TYPE (obj, stuner_get_type ())

enum web100_stuning_mode {
  STUNEMODE_DEFAULT = 0,
  STUNEMODE_SETSOCKOPT,
  STUNEMODE_FIXED,
  STUNEMODE_AUTO,
  STUNEMODE_EXP1,
  STUNEMODE_EXP2
};

typedef struct _Stuner       Stuner;
typedef struct _StunerClass  StunerClass;

struct _Stuner
{
  GtkVBox vbox;
  
  Web100Obj *web100obj;

  web100_var *bufset, *tunemode;

  GtkAdjustment *adjustment;
  GtkWidget *scale, *label, *write_button;

  float scale_val;
  unsigned int buf_val; 
};

struct _StunerClass
{ 
  GtkVBoxClass parent_class; 
};

GtkType     stuner_get_type       (void); 
GtkWidget*  stuner_new            (Web100Obj *);
void        stuner_update         (GtkObject *, gpointer *);
void        stuner_set_web100obj  (Stuner *, Web100Obj *);
void        stuner_sockset_listen (GtkObject *, gpointer);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __STUNER_H__ */ 
