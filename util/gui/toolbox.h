#ifndef __TOOLBOX_H__
#define __TOOLBOX_H__


#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include "config.h"
#include "web100object.h"
#include "web100widget.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define TYPE_TOOLBOX               (toolbox_get_type ())
#define TOOLBOX(obj)               (GTK_CHECK_CAST (obj, TYPE_TOOLBOX, Toolbox))
#define TOOLBOX_CLASS(klass)       (GTK_CHECK_CLASS_CAST (klass, TYPE_TOOLBOX, ToolboxClass))
#define IS_TOOLBOX(obj)            (GTK_CHECK_TYPE (obj, TYPE_TOOLBOX))
#define IS_TOOLBOX_CLASS(klass)    (GTK_CHECK_CLASS_TYPE ((klass), TYPE_TOOLBOX))

#ifdef GTK2
#define TOOLBOX_GET_CLASS(obj)     (GTK_CHECK_GET_CLASS ((obj), TYPE_TOOLBOX, ToolboxClass))
#else
#define TOOLBOX_GET_CLASS(obj)     (TOOLBOX_CLASS (obj->klass))
#endif

typedef struct _Toolbox       Toolbox;
typedef struct _ToolboxClass  ToolboxClass;

struct _Toolbox
{
  Web100Widget web100widget;
};

struct _ToolboxClass
{ 
  Web100WidgetClass parent_class;
};

#ifdef GTK2
GType          toolbox_get_type    (void); 
#else
GtkType        toolbox_get_type    (void);
#endif
GtkWidget*  toolbox_new         (Web100Object *);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __TOOLBOX_H__ */ 
