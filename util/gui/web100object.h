#ifndef __WEB100_OBJECT_H__
#define __WEB100_OBJECT_H__

#include <gdk/gdk.h>
#include <gtk/gtkobject.h> 
#include "web100.h"
#include "web100poll.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define WEB100_OBJECT(obj)          GTK_CHECK_CAST (obj, web100_object_get_type (), Web100Object)
#define WEB100_OBJECT_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass, web100_object_get_type (), Web100ObjectClass)
#define IS_WEB100_OBJECT(obj)       GTK_CHECK_TYPE (obj, web100_object_get_type ())

#define WEB100_OBJECT_CONNECTION_TYPE_NONE -1
#define WEB100_OBJECT_CONNECTION_TYPE_LOG  -2

typedef struct _Web100Object       Web100Object;
typedef struct _Web100ObjectClass  Web100ObjectClass;

struct snapshot_list
{
  char name[WEB100_GROUPNAME_LEN_MAX];
  web100_group    *group;

  web100_snapshot *last, *prior, *set, *alt;
  struct snapshot_list *next;
};

struct _Web100Object
{
  GtkObject object;

  int                               cid;
  WEB100_ADDRTYPE                   addrtype;
  struct web100_connection_spec     spec;
  struct web100_connection_spec_v6  spec_v6; 
  web100_agent                     *agent;

  struct snapshot_list *snapshot_head;		       

  Web100Poll           *web100poll; 
  GList                *widgets; // keep track of calling widgets
};

struct _Web100ObjectClass
{ 
  GtkObjectClass parent_class; 

  void (* connection_init)   (Web100Object *web100_object);
  void (* connection_closed) (Web100Object *web100_object);
  void (* snap_update)       (Web100Object *web100_object); 
};

#ifdef GTK2
GType      web100_object_get_type           (void);
#else
GtkType    web100_object_get_type           (void);
#endif
GtkObject* web100_object_new                (Web100Poll *web100co, int cid);

void       web100_object_connection_closed  (Web100Object *);
void       web100_object_connection_init    (Web100Object *);
void       web100_object_snap_update        (Web100Object *);

int        web100_object_set_connection     (Web100Object *web100_object, int cid);
void       web100_object_refresh            (Web100Object *); 


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __WEB100_OBJECT_H__ */ 
