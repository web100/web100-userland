
/*
 *  Copyrights
 *
 *   All documentation and programs in this release is copyright (c)
 *   Carnegie Mellon University, The Board of Trustees of the University of
 *   Illinois, and University Corporation for Atmospheric Research, 2001.
 *   This software comes with NO WARRANTY.
 *
 *   The kernel changes and additions are also covered by the GPL version 2.
 *
 *   Since our code is currently under active development we prefer that
 *   everyone gets the it directly from us.  This will permit us to
 *   collaborate with all of the users.  So for the time being, please refer
 *   potential users to us instead of redistributing web100.
 */

#ifndef __WCGRAPH_H__
#define __WCGRAPH_H__

#include <gdk/gdk.h>
#include <gtk/gtkadjustment.h>
#include <gtk/gtkwidget.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define WC_GRAPH(obj)          GTK_CHECK_CAST (obj, wc_graph_get_type (), WcGraph)
#define WC_GRAPH_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass, wc_graph_get_type (), WcGraphClass)
#define WC_IS_GRAPH(obj)       GTK_CHECK_TYPE (obj, wc_graph_get_type ())

typedef struct _WcGraph        WcGraph;
typedef struct _WcGraphClass   WcGraphClass;

struct _WcGraph
{
  GtkWidget widget;

  guint policy : 2;

  guint8 button;

  gfloat old_value;
  gfloat old_lower;
  gfloat old_upper;

  GtkAdjustment *adjustment;

  gfloat nvalue[32];

  GdkGC  *gc;
  GdkFont *font; 
};

struct _WcGraphClass
{
  GtkWidgetClass parent_class;
};

GtkWidget*     wc_graph_new                (GtkAdjustment *adjustment);
guint          wc_graph_get_type           (void);
GtkAdjustment* wc_graph_get_adjustment     (WcGraph *graph);
void           wc_graph_set_update_policy  (WcGraph *graph, GtkUpdateType policy); 
void           wc_graph_set_adjustment     (WcGraph *graph, GtkAdjustment *adjustment);
void           wc_graph_set_value          (WcGraph *graph, int which, gfloat value);
#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __WCGRAPH_H__ */ 
