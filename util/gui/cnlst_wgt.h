#ifndef __CNLST_H__
#define __CNLST_H__


#include <gdk/gdk.h>
#include <gtk/gtkvbox.h> 
#include <gtk/gtkobject.h>
#include "web100obj.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define CNLST(obj)          GTK_CHECK_CAST (obj, cnlst_get_type (), Cnlst)
#define CNLST_CLASS(class)  GTK_CHECK_CLASS_CAST (class, cnlst_get_type (), CnlstClass)
#define IS_CNLST(obj)       GTK_CHECK_TYPE (obj, cnlst_get_type ())

typedef struct _Cnlst       Cnlst;
typedef struct _CnlstClass  CnlstClass;

struct _Cnlst
{
  GtkVBox vbox;
  
  GtkWidget *clist;
};

struct _CnlstClass
{ 
  GtkVBoxClass parent_class; 
};

GtkType     cnlst_get_type       (void); 
GtkWidget*  cnlst_new            (void);
gint        cnlst_update         (gpointer);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __CNLST_H__ */ 
