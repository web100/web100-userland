#ifndef __CONNECTION_SELECT_H__
#define __CONNECTION_SELECT_H__


#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include "config.h"
#include "web100object.h"
#include "web100widget.h"
#include "connectionlist.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define TYPE_CONNECTION_SELECT               (connection_select_get_type ())
#define CONNECTION_SELECT(obj)               (GTK_CHECK_CAST (obj, TYPE_CONNECTION_SELECT, ConnectionSelect))
#define CONNECTION_SELECT_CLASS(klass)       (GTK_CHECK_CLASS_CAST (klass, TYPE_CONNECTION_SELECT, ConnectionSelectClass))
#define IS_CONNECTION_SELECT(obj)            (GTK_CHECK_TYPE (obj, TYPE_CONNECTION_SELECT))
#define IS_CONNECTION_SELECT_CLASS(klass)    (GTK_CHECK_CLASS_TYPE ((klass), TYPE_CONNECTION_SELECT))

#ifdef GTK2
#define CONNECTION_SELECT_GET_CLASS(obj)     (GTK_CHECK_GET_CLASS ((obj), TYPE_CONNECTION_SELECT, ConnectionSelectClass))
#else
#define CONNECTION_SELECT_GET_CLASS(obj)     (CONNECTION_SELECT_CLASS (obj->klass))
#endif

typedef struct _ConnectionSelect       ConnectionSelect;
typedef struct _ConnectionSelectClass  ConnectionSelectClass;

struct _ConnectionSelect
{
  Web100Widget web100widget;

  ConnectionList *connection_list;
  GtkEntry       *addr_entry, *cid_entry;

  gboolean     selectable;
};

struct _ConnectionSelectClass
{ 
  Web100WidgetClass parent_class;
};

#ifdef GTK2
GType          connection_select_get_type    (void); 
#else
GtkType        connection_select_get_type    (void);
#endif
GtkWidget*     connection_select_new         (Web100Object *web100obj, int selectable);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __CONNECTION_SELECT_H__ */ 
