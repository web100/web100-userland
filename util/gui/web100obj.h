#ifndef __WEB100_OBJ_H__
#define __WEB100_OBJ_H__

#include <gdk/gdk.h>
#include <gtk/gtkobject.h> 
#include "web100.h"
#include "web100sync.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define WEB100_OBJ(obj)          GTK_CHECK_CAST (obj, web100obj_get_type (), Web100Obj)
#define WEB100_OBJ_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass, web100obj_get_type (), Web100ObjClass)
#define IS_WEB100_OBJ(obj)       GTK_CHECK_TYPE (obj, web100obj_get_type ())


typedef struct _Web100Obj       Web100Obj;
typedef struct _Web100ObjClass  Web100ObjClass;

struct snapshot_data
{
  char name[WEB100_GROUPNAME_LEN_MAX];
  web100_group    *group; // redundency, for convenience

  web100_snapshot *last, *prior, *set, *alt;
  struct snapshot_data *next;
};

struct _Web100Obj
{
  GtkObject object;

  int cid;
  struct web100_connection_spec spec;
  struct web100_connection_spec_v6 spec_v6;
 
  web100_agent       *agent;

  web100_connection  *connection;

  web100_snapshot    *snap, *snap_read_prior, *snap_read_last,
                     *snap_tune, *snap_tune_last, *snap_tune_prior,
		     *set_snap_read, *set_snap_tune;

  struct snapshot_data *snapshot_head;		       
  guint timeout_id;

  Web100Sync           *web100sync; 
  GList                *widgets; // keep track of calling widgets
};

struct _Web100ObjClass
{ 
  GtkObjectClass parent_class; 

  void (* connection_changed) (Web100Obj *web100obj);
  void (* snap_update) (Web100Obj *web100obj); 
};

GtkType     web100obj_get_type       (void); 
GtkObject*  web100obj_new            (Web100Sync *web100sync,
                                      char *ascii_cid); 
void        web100obj_changed        (Web100Obj *);
void        web100obj_snap_update    (Web100Obj *);
void        web100obj_set_connection (Web100Obj *web100obj,
                                      int cid);
void        web100obj_refresh        (Web100Obj *); 
gint        web100obj_compar         (web100_connection *aa,
                                      web100_connection *bb);
void        sprinttype               (char *text, int type, void *buf);
// static void web100obj_destroy   (Web100Obj *);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __WEB100_OBJ_H__ */ 
