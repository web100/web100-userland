#ifndef __RTUNER_H__
#define __RTUNER_H__


#include <gdk/gdk.h>
#include <gtk/gtkvbox.h> 
#include <gtk/gtkobject.h>
#include "web100obj.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define RTUNER(obj)          GTK_CHECK_CAST (obj, rtuner_get_type (), Rtuner)
#define RTUNER_CLASS(class)  GTK_CHECK_CLASS_CAST (class, rtuner_get_type (), RtunerClass)
#define IS_RTUNER(obj)       GTK_CHECK_TYPE (obj, rtuner_get_type ())

enum web100_rtuning_mode {
  RTUNEMODE_DEFAULT = 0,
  RTUNEMODE_SETSOCKOPT,
  RTUNEMODE_FIXED,
  RTUNEMODE_AUTO,
  RTUNEMODE_EXP1,
  RTUNEMODE_EXP2
};

typedef struct _Rtuner       Rtuner;
typedef struct _RtunerClass  RtunerClass;

struct _Rtuner
{
  GtkVBox vbox;
  
  Web100Obj *web100obj;

  web100_var *bufset, *tunemode;

  GtkAdjustment *adjustment;
  GtkWidget *label;
  float scale_val;
  unsigned int buf_val;

  GtkWidget *write_button;
};

struct _RtunerClass
{ 
  GtkVBoxClass parent_class; 
};

GtkType     rtuner_get_type       (void); 
GtkWidget*  rtuner_new            (Web100Obj *);
void        rtuner_update         (GtkObject *, gpointer *);
void        rtuner_set_web100obj  (Rtuner *, Web100Obj *);
void        rtuner_sockset_listen (GtkObject *, gpointer);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __RTUNER_H__ */ 
