
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

#ifndef __WCPIE_H__
#define __WCPIE_H__


#include <gdk/gdk.h>
#include <gtk/gtkadjustment.h>
#include <gtk/gtkwidget.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define WC_PIE(obj)          GTK_CHECK_CAST (obj, wc_pie_get_type (), WcPie)
#define WC_PIE_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass, wc_pie_get_type (), WcPieClass)
#define WC_IS_PIE(obj)       GTK_CHECK_TYPE (obj, wc_pie_get_type ())


typedef struct _WcPie        WcPie;
typedef struct _WcPieClass   WcPieClass;

struct _WcPie
{
	GtkWidget widget;

	guint policy : 2; 
	guint8 button; 
	gint radius; 
	guint32 timer;

	GdkPoint pts[3][181];
	gint npts[3];
	GdkGC *gc[3];

	GtkAdjustment *adjustment[3]; 
};

struct _WcPieClass
{
	GtkWidgetClass parent_class;
};


GtkWidget*     wc_pie_new                  (GtkAdjustment *adjustment[3]);
#ifdef GTK2
GType          wc_pie_get_type             (void);
#else
GtkType        wc_pie_get_type             (void);
#endif
GtkAdjustment* wc_pie_get_adjustment       (WcPie *dial, int ii);
void           wc_pie_set_update_policy    (WcPie *dial, GtkUpdateType policy);

void           wc_pie_set_adjustment       (WcPie *dial, GtkAdjustment *adjustment, int ii);

#ifdef __cplusplus
}
#endif /* __cplusplus */ 

#endif /* __WCPIE_H__ */ 
