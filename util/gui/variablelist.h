#ifndef __VARIABLE_LIST_H__
#define __VARIABLE_LIST_H__


#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include "config.h"
#include "web100.h"
#include "web100object.h"
#include "web100widget.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define TYPE_VARIABLE_LIST               (variable_list_get_type ())
#define VARIABLE_LIST(obj)               (GTK_CHECK_CAST (obj, TYPE_VARIABLE_LIST, VariableList))
#define VARIABLE_LIST_CLASS(klass)       (GTK_CHECK_CLASS_CAST (klass, TYPE_VARIABLE_LIST, VariableListClass))
#define IS_VARIABLE_LIST(obj)            (GTK_CHECK_TYPE (obj, TYPE_VARIABLE_LIST))
#define IS_VARIABLE_LIST_CLASS(klass)    (GTK_CHECK_CLASS_TYPE ((klass), TYPE_VARIABLE_LIST))

#ifdef GTK2
#define VARIABLE_LIST_GET_CLASS(obj)     (GTK_CHECK_GET_CLASS ((obj), TYPE_VARIABLE_LIST, VariableListClass))
#else
#define VARIABLE_LIST_GET_CLASS(obj)     (VARIABLE_LIST_CLASS (obj->klass))
#endif

typedef struct _VariableList       VariableList;
typedef struct _VariableListClass  VariableListClass;

struct _VariableList
{
  Web100Widget web100widget;

  web100_var   **variable;
#ifdef GTK2
  GtkListStore     *list_store;
  GtkTreeSelection *list_select;
#else
  GtkCList         *clist;
#endif
};

struct _VariableListClass
{ 
  Web100WidgetClass parent_class;
};

#ifdef GTK2
GType          variable_list_get_type    (void); 
#else
GtkType        variable_list_get_type    (void);
#endif
GtkWidget*     variable_list_new         (Web100Object *);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __VARIABLE_LIST_H__ */ 
