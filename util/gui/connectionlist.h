#ifndef __CONNECTION_LIST_H__
#define __CONNECTION_LIST_H__


#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include "config.h"
#include "web100object.h"
#include "web100widget.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define TYPE_CONNECTION_LIST               (connection_list_get_type ())
#define CONNECTION_LIST(obj)               (GTK_CHECK_CAST (obj, TYPE_CONNECTION_LIST, ConnectionList))
#define CONNECTION_LIST_CLASS(klass)       (GTK_CHECK_CLASS_CAST (klass, TYPE_CONNECTION_LIST, ConnectionListClass))
#define IS_CONNECTION_LIST(obj)            (GTK_CHECK_TYPE (obj, TYPE_CONNECTION_LIST))
#define IS_CONNECTION_LIST_CLASS(klass)    (GTK_CHECK_CLASS_TYPE ((klass), TYPE_CONNECTION_LIST))

#ifdef GTK2
#define CONNECTION_LIST_GET_CLASS(obj)     (GTK_CHECK_GET_CLASS ((obj), TYPE_CONNECTION_LIST, ConnectionListClass))
#else
#define CONNECTION_LIST_GET_CLASS(obj)     (CONNECTION_LIST_CLASS (obj->klass))
#endif

typedef struct _ConnectionList       ConnectionList;
typedef struct _ConnectionListClass  ConnectionListClass;

struct _ConnectionList
{
  Web100Widget web100widget;

#ifdef GTK2
  GtkListStore      *list_store;
  GtkWidget         *view;
#else
  GtkCList          *clist;
#endif
};

struct _ConnectionListClass
{ 
  Web100WidgetClass parent_class;
};

#ifdef GTK2
GType          connection_list_get_type    (void); 
#else
GtkType        connection_list_get_type    (void);
#endif
GtkWidget*     connection_list_new         (Web100Object *web100obj);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __CONNECTION_LIST_H__ */ 
