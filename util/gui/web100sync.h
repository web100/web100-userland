#ifndef __WEB100_SYNC_H__
#define __WEB100_SYNC_H__

#include <gdk/gdk.h>
#include <gtk/gtkobject.h> 
#include "web100.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define WEB100_SYNC(sync)          GTK_CHECK_CAST (sync, web100sync_get_type (), Web100Sync)
#define WEB100_SYNC_CLASS(klass)   GTK_CHECK_CLASS_CAST (klass, web100sync_get_type (), Web100SyncClass)
#define IS_WEB100_SYNC(sync)       GTK_CHECK_TYPE (sync, web100sync_get_type ())


typedef struct _Web100Sync       Web100Sync;
typedef struct _Web100SyncClass  Web100SyncClass;

struct _Web100Sync
{
  GtkObject object;

  web100_agent    *agent;

  GList           *objects;

  GtkAdjustment   *adjustment;
  guint           timeout_id;
};

struct _Web100SyncClass
{ 
  GtkObjectClass parent_class; 
};

GtkType     web100sync_get_type       (void); 
GtkObject*  web100sync_new            (web100_agent *agent);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __WEB100_SYNC_H__ */ 
