#ifndef __WEB100_POLL_H__
#define __WEB100_POLL_H__

#include <gdk/gdk.h>
#include <gtk/gtk.h> 
#include "config.h"
#include "web100.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define WEB100_POLL(sync)          GTK_CHECK_CAST (sync, web100_poll_get_type (), Web100Poll)
#define WEB100_POLL_CLASS(klass)   GTK_CHECK_CLASS_CAST (klass, web100_poll_get_type (), Web100PollClass)
#define IS_WEB100_POLL(sync)       GTK_CHECK_TYPE (sync, web100_poll_get_type ())


typedef struct _Web100Poll       Web100Poll;
typedef struct _Web100PollClass  Web100PollClass;

struct _Web100Poll
{
  GtkObject object;

  web100_agent    *agent;

  GList           *objects;

  GtkAdjustment   *adjustment;
  guint            timeout_id;
};

struct _Web100PollClass
{ 
  GtkObjectClass parent_class; 
};

GtkType     web100_poll_get_type       (void); 
GtkObject*  web100_poll_new            (web100_agent *agent);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __WEB100_POLL_H__ */ 
