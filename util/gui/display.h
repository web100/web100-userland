#ifndef __DISPLAY_H__
#define __DISPLAY_H__


#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include "web100object.h"
#include "web100widget.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define TYPE_DISPLAY               (display_get_type ())
#define DISPLAY(obj)               (GTK_CHECK_CAST (obj, TYPE_DISPLAY, Display))
#define DISPLAY_CLASS(klass)       (GTK_CHECK_CLASS_CAST (klass, TYPE_DISPLAY, DisplayClass))
#define IS_DISPLAY(obj)            (GTK_CHECK_TYPE (obj, TYPE_DISPLAY))
#define IS_DISPLAY_CLASS(klass)    (GTK_CHECK_CLASS_TYPE ((klass), TYPE_DISPLAY))

#ifdef GTK2
#define DISPLAY_GET_CLASS(obj)     (GTK_CHECK_GET_CLASS ((obj), TYPE_DISPLAY, DisplayClass))
#else
#define DISPLAY_GET_CLASS(obj)     (DISPLAY_CLASS (obj->klass))
#endif

typedef struct _Display       Display;
typedef struct _DisplayClass  DisplayClass;

struct _Display
{
  Web100Widget web100widget;

  web100_var *var;
  web100_group *group;

  GtkWidget *graph;
  GtkWidget *clist;
  GtkWidget *entry;
  GtkWidget *combo;
  GtkWidget *smooth_toggle;

  gfloat ewma, oldewma, max, oldmax;
  gfloat graphval[32];
  int delta_on, smooth_on;
  int step;
};

struct _DisplayClass
{ 
  Web100WidgetClass parent_class;
};

#ifdef GTK2
GType       display_get_type    (void); 
#else
GtkType     display_get_type    (void);
#endif
GtkWidget*  display_new         (Web100Object *, char *);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __DISPLAY_H__ */ 
