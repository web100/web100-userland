
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

	/* update policy (GTK_UPDATE_[CONTINUOUS/DELAYED/DIWCONTINUOUS]) */
	guint policy : 2;

	/* Button currently pressed or 0 if none */
	guint8 button;

	/* Dimensions of graph components */
	gint radius;
	gint pointer_width;

	/* ID of update timer, or 0 if none */
	guint32 timer;

	/* Current angle */
	gfloat angle;
	gfloat last_angle;

	/* Old values from adjustment stored so we know when something changes */
	gfloat old_value;
	gfloat old_lower;
	gfloat old_upper;

	/* The adjustment object that stores the data for this graph */
	GtkAdjustment *adjustment; 

	/* Values for the 6 bars */
	gint nvalue[20];

	GdkFont *font;

};

struct _WcGraphClass
{
	GtkWidgetClass parent_class;
};

GtkWidget*     wc_graph_new                    (GtkAdjustment *adjustment);
guint          wc_graph_get_type               (void);
GtkAdjustment* wc_graph_get_adjustment         (WcGraph      *graph);
void           wc_graph_set_update_policy      (WcGraph      *graph,
						GtkUpdateType  policy);

void           wc_graph_set_adjustment         (WcGraph      *graph,
						GtkAdjustment *adjustment);
void wc_graph_set_value(WcGraph *graph, int which, int value);
#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __WCGRAPH_H__ */
/* example-end */
