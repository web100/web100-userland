#ifndef __WEB100_WIDGET_H__
#define __WEB100_WIDGET_H__


#include <gdk/gdk.h>
#include <gtk/gtkobject.h>
#include <gtk/gtkvbox.h>
#include "config.h"
#include "web100.h"
#include "web100poll.h"
#include "web100object.h" 


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define TYPE_WEB100_WIDGET               (web100_widget_get_type ())
#define WEB100_WIDGET(obj)               (GTK_CHECK_CAST (obj, TYPE_WEB100_WIDGET, Web100Widget))
#define WEB100_WIDGET_CLASS(klass)       (GTK_CHECK_CLASS_CAST (klass, TYPE_WEB100_WIDGET, Web100WidgetClass))
#define IS_WEB100_WIDGET(obj)            (GTK_CHECK_TYPE (obj, TYPE_WEB100_WIDGET))
#define IS_WEB100_WIDGET_CLASS(klass)    (GTK_CHECK_CLASS_TYPE ((klass), TYPE_WEB100_WIDGET))

#ifdef GTK2
#define WEB100_WIDGET_GET_CLASS(obj)     (GTK_CHECK_GET_CLASS ((obj), TYPE_WEB100_WIDGET, Web100WidgetClass))
#else
#define WEB100_WIDGET_GET_CLASS(obj)     (WEB100_WIDGET_CLASS (GTK_OBJECT (obj)->klass))
#endif


typedef struct _Web100Widget       Web100Widget;
typedef struct _Web100WidgetClass  Web100WidgetClass;

struct _Web100Widget
{
  GtkVBox vbox;
  
  Web100Object *web100object; 

  GtkWidget    *update_widget;
  gint          update_toggle;

  gboolean      local;
  
  GList        *widgets;
};

struct _Web100WidgetClass
{ 
  GtkVBoxClass parent_class; 

/** signal handlers **/
  void (* connection_closed) (GtkObject *object, gpointer data);
  void (* connection_init)   (GtkObject *object, gpointer data);
  void (* snap_update)       (GtkObject *object, gpointer data); 
/** signal emission **/
  void (* object_changed)    (Web100Widget *widget); 
/** function prototype **/
  void (* set_object)        (Web100Widget *widget, Web100Object *object);
};

#ifdef GTK2
GType          web100_widget_get_type       (void); 
#else
GtkType        web100_widget_get_type       (void);
#endif
void           web100_widget_set_object     (Web100Widget *, Web100Object *);
void           web100_widget_set_object_from_widget  (Web100Widget *, Web100Widget *); 
void           web100_widget_set_object_from_widget_reversed  (Web100Widget *, Web100Widget *); 
Web100Object*  web100_widget_get_object     (Web100Widget *);
void           web100_widget_share_object   (Web100Widget *, Web100Widget *);
void           web100_widget_connection_closed (GtkObject *object, gpointer data);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __WEB100_WIDGET_H__ */ 
