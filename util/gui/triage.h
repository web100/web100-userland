#ifndef __TRIAGE_H__
#define __TRIAGE_H__


#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include "config.h"
#include "web100object.h"
#include "web100widget.h"

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
  Web100Widget web100widget;

  web100_var *wv[3];
  GtkWidget  *pie, *clist;

  unsigned long long int val[3], delta[3];

  float new_ewma[3], old_ewma[3];
};

struct _TriageClass
{ 
  Web100WidgetClass parent_class;
};

#ifdef GTK2
GType       triage_get_type    (void);
#else
GtkType     triage_get_type    (void);
#endif
GtkWidget*  triage_new          (Web100Object *);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __TRIAGE_H__ */ 
