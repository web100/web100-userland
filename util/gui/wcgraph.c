
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

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <gtk/gtk.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <math.h>

#include <gtk/gtkmain.h>
#include <gtk/gtksignal.h>

#include "wcgraph.h"

#define DEFAULT_SIZE 150


static void wc_graph_class_init(WcGraphClass *klass);
static void wc_graph_init(WcGraph *graph);
static void wc_graph_destroy(GtkObject *object);
static void wc_graph_realize(GtkWidget *widget);
static void wc_graph_size_request(GtkWidget *widget,
					GtkRequisition *requisition);
static void wc_graph_size_allocate(GtkWidget *widget,
					GtkAllocation *allocation);
static gint wc_graph_expose(GtkWidget *widget, GdkEventExpose *event);
static gint wc_graph_configure(GtkWidget *widget, GdkEventConfigure *event);
static gint wc_graph_button_press(GtkWidget *widget, GdkEventButton *event);
static gint wc_graph_button_release(GtkWidget *widget, GdkEventButton *event);
static gint wc_graph_motion_notify(GtkWidget *widget, GdkEventMotion *event);
static gint wc_graph_timer(WcGraph *graph); 
static void wc_graph_update_mouse(WcGraph *graph, gint x, gint y);
static void wc_graph_update(WcGraph *graph);
static void wc_graph_adjustment_changed(GtkAdjustment *adjustment,
					gpointer data);
static void wc_graph_adjustment_value_changed (GtkAdjustment *adjustment,
						gpointer data);

static GtkWidgetClass *parent_class = NULL;
static GdkPixmap *pixmap = NULL;
char temptext[60];

guint wc_graph_get_type ()
{
	static guint graph_type = 0;

	if (!graph_type){
		GtkTypeInfo graph_info = {
			"WcGraph",
			sizeof (WcGraph),
			sizeof (WcGraphClass),
			(GtkClassInitFunc) wc_graph_class_init,
			(GtkObjectInitFunc) wc_graph_init,
			(GtkArgSetFunc) NULL,
			(GtkArgGetFunc) NULL,
		};

		graph_type = gtk_type_unique (gtk_widget_get_type (),
						&graph_info);
	}
	return graph_type;
}

static void wc_graph_class_init (WcGraphClass *class)
{
	GtkObjectClass *object_class;
	GtkWidgetClass *widget_class;
	
	object_class = (GtkObjectClass*) class;
	widget_class = (GtkWidgetClass*) class;
	parent_class = gtk_type_class (gtk_widget_get_type ());
	object_class->destroy = wc_graph_destroy;
	widget_class->realize = wc_graph_realize;
	widget_class->expose_event = wc_graph_expose;
	widget_class->configure_event = wc_graph_configure;
	widget_class->size_request = wc_graph_size_request;
	widget_class->size_allocate = wc_graph_size_allocate;
}

static void wc_graph_init (WcGraph *graph)
{
	int ii;

	graph->button = 0;
	graph->policy = GTK_UPDATE_CONTINUOUS;
	graph->timer = 0;
	graph->radius = 0;
	graph->pointer_width = 0;
	graph->angle = 0.0;
	graph->old_value = 0.0;
	graph->old_lower = 0.0;
	graph->old_upper = 0.0;
	graph->adjustment = NULL;
	for(ii=0;ii<20;ii++)graph->nvalue[ii] = 0;
}

GtkWidget* wc_graph_new(GtkAdjustment *adjustment)
{
	WcGraph *graph;
	GdkFont *fixed_font;

	fixed_font =
		gdk_font_load ("-miwc-fixed-medium-r-*-*-*-140-*-*-*-*-*-*");

	graph = gtk_type_new (wc_graph_get_type ());
	graph->font = fixed_font;

	if (!adjustment)
		adjustment = (GtkAdjustment*) gtk_adjustment_new (0.0, 0.0, 10.0, 0.1, 0.5, 0.2);

	wc_graph_set_adjustment (graph, adjustment);

	return GTK_WIDGET (graph);
}

static void wc_graph_destroy(GtkObject *object)
{
	WcGraph *graph;

	g_return_if_fail (object != NULL);
	g_return_if_fail (WC_IS_GRAPH (object));

	graph = WC_GRAPH (object);

	if (graph->adjustment)
		gtk_object_unref (GTK_OBJECT (graph->adjustment));

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

GtkAdjustment* wc_graph_get_adjustment(WcGraph *graph)
{
	g_return_val_if_fail (graph != NULL, NULL);
	g_return_val_if_fail (WC_IS_GRAPH (graph), NULL);

	return graph->adjustment;
}

void wc_graph_set_update_policy(WcGraph *graph,
				GtkUpdateType policy)
{
	g_return_if_fail (graph != NULL);
	g_return_if_fail (WC_IS_GRAPH (graph));

	graph->policy = policy;
}

void wc_graph_set_adjustment(WcGraph *graph, GtkAdjustment *adjustment)
{
	g_return_if_fail (graph != NULL);
	g_return_if_fail (WC_IS_GRAPH (graph));

	if (graph->adjustment)
	{
		gtk_signal_disconnect_by_data (GTK_OBJECT (graph->adjustment),
						(gpointer) graph);
		gtk_object_unref (GTK_OBJECT (graph->adjustment));
	}

	graph->adjustment = adjustment;
	gtk_object_ref (GTK_OBJECT (graph->adjustment));

	gtk_signal_connect (GTK_OBJECT (adjustment), "changed",
			(GtkSignalFunc) wc_graph_adjustment_changed,
			(gpointer) graph);
	gtk_signal_connect (GTK_OBJECT (adjustment), "value_changed",
			(GtkSignalFunc) wc_graph_adjustment_value_changed,
			(gpointer) graph);

	graph->old_value = adjustment->value;
	graph->old_lower = adjustment->lower;
	graph->old_upper = adjustment->upper;

	wc_graph_update (graph);
}

static void wc_graph_realize(GtkWidget *widget)
{ 
	WcGraph *graph;
	GdkWindowAttr attributes;
	gint attributes_mask;

	g_return_if_fail (widget != NULL);
	g_return_if_fail (WC_IS_GRAPH (widget));

	GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);
	graph = WC_GRAPH (widget);

	attributes.x = widget->allocation.x;
	attributes.y = widget->allocation.y;
	attributes.width = widget->allocation.width;
	attributes.height = widget->allocation.height;
	attributes.wclass = GDK_INPUT_OUTPUT;
	attributes.window_type = GDK_WINDOW_CHILD;
	attributes.event_mask = gtk_widget_get_events (widget) | 
		/* TEST */
/* 		GDK_CONFIGURE_MASK | */
		GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK | 
		GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK |
		GDK_POINTER_MOTION_HINT_MASK;
	attributes.visual = gtk_widget_get_visual (widget);
	attributes.colormap = gtk_widget_get_colormap (widget);

	attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
	widget->window = gdk_window_new (widget->parent->window, &attributes, attributes_mask);

	widget->style = gtk_style_attach (widget->style, widget->window);

	gdk_window_set_user_data (widget->window, widget);

	gtk_style_set_background (widget->style, widget->window, GTK_STATE_NORMAL);
}

static void wc_graph_size_request(GtkWidget *widget,
				GtkRequisition *requisition)
{
	requisition->width = 1.5*DEFAULT_SIZE;
	requisition->height = DEFAULT_SIZE;
}

static void wc_graph_size_allocate(GtkWidget *widget, GtkAllocation *allocation)
{
	WcGraph *graph;

	g_return_if_fail (widget != NULL);
	g_return_if_fail (WC_IS_GRAPH (widget));
	g_return_if_fail (allocation != NULL);

	widget->allocation = *allocation;
	graph = WC_GRAPH (widget);

	if (GTK_WIDGET_REALIZED (widget))
	{ 
		gdk_window_move_resize (widget->window,
				allocation->x, allocation->y,
				allocation->width, allocation->height);

	}
	graph->radius = MIN(allocation->width,allocation->height) * 0.45;
	graph->pointer_width = graph->radius / 5;
}

gint wc_graph_repaint(GtkWidget *widget)
{
	WcGraph *graph;
	GdkPoint points[6];
	gdouble s,c;
	gdouble theta, last, increment; 
	gint xc, yc, width, height, heightb, bar_width, bar_height, upper_bound;
	gint upper, lower;
	gint tick_length;
	gint i, inc;
	gint xfudge=50, yfudge=20;
	int ii;

/* 	char *temptext = malloc(60); */
/* 	strcpy(temptext, "what"); */ 

	g_return_val_if_fail (widget != NULL, FALSE);
	g_return_val_if_fail (WC_IS_GRAPH (widget), FALSE);

	graph = WC_GRAPH (widget);

	if((graph->adjustment->upper)/1000000 > 1){
	  sprintf(temptext, "%d", (int) (graph->adjustment->upper)/1000000);
	  strcat(temptext, " M");
	} else if ((graph->adjustment->upper)/1000 > 1){
	  sprintf(temptext, "%d", (int) (graph->adjustment->upper)/1000);
	  strcat(temptext, " k");
	} else sprintf(temptext, "%d", (int) graph->adjustment->upper);

	/*  gdk_window_clear_area (widget->window,
	 *  0, 0,
	 *  widget->allocation.width,
	 *  widget->allocation.height); */

	gdk_draw_rectangle(pixmap,
			widget->style->base_gc[widget->state],
			TRUE, 0, 0,
			widget->allocation.width,
			widget->allocation.height);

	width = widget->allocation.width-xfudge;
	height = widget->allocation.height;
	heightb = height - yfudge;

	bar_width = width/20;
	upper_bound = graph->adjustment->upper;

/* 	upper = graph->adjustment->upper; */
/* 	lower = graph->adjustment->lower;  */

	gdk_gc_set_line_attributes(widget->style->black_gc, 
			2,
			GDK_LINE_SOLID,
			GDK_CAP_ROUND,
			GDK_JOIN_ROUND);
	gdk_draw_line(pixmap,
			widget->style->black_gc,
			xfudge, yfudge,
			width + xfudge, yfudge);
	gdk_draw_line(pixmap,
			widget->style->black_gc,
			0, yfudge + 0.5*heightb,
			width+xfudge, yfudge + 0.5*heightb);
	gdk_gc_set_line_attributes(widget->style->black_gc,
			1,
			GDK_LINE_SOLID,
			GDK_CAP_ROUND,
			GDK_JOIN_ROUND);

	gdk_draw_text(pixmap, widget->style->font,
			widget->style->fg_gc[widget->state],
			5,
			yfudge,
			temptext, strlen(temptext));

 	for(ii=0;ii<20;ii++){
		if(upper_bound) bar_height = (graph->nvalue[ii])*heightb/upper_bound;
		else bar_height = 0;
		gdk_draw_rectangle(pixmap, 
				widget->style->bg_gc[widget->state],
				TRUE,
				(ii*bar_width + xfudge),
				(height-bar_height ),
				(bar_width-1),
				(bar_height ));
/* 		printf("%d, ", bar_height); */
	}
/* 	printf("\n"); */
	


	return FALSE;
}

static gint wc_graph_expose(GtkWidget *widget, GdkEventExpose *event)
{ 
/* 	printf("expose\n"); */
/* 	if(pixmap){
		gdk_pixmap_unref(pixmap);
	}
	pixmap = gdk_pixmap_new(widget->window,
			widget->allocation.width,
			widget->allocation.height,
			-1); */
  	wc_graph_configure(widget, NULL);   
	wc_graph_repaint(widget);

	gdk_draw_pixmap(widget->window,
			widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
			pixmap,
			event->area.x, event->area.y,
			event->area.x, event->area.y,
			event->area.width, event->area.height);
}

static gint wc_graph_configure(GtkWidget *widget, GdkEventConfigure *event)
{ 
 	if(pixmap){ 
		gdk_pixmap_unref(pixmap);
	}
	pixmap = gdk_pixmap_new(widget->window,
			widget->allocation.width,
			widget->allocation.height,
			-1);
/* printf("configure\n"); */
	return TRUE;
}

static gint wc_graph_timer(WcGraph *graph)
{
	g_return_val_if_fail (graph != NULL, FALSE);
	g_return_val_if_fail (WC_IS_GRAPH (graph), FALSE);

	if (graph->policy == GTK_UPDATE_DELAYED)
		gtk_signal_emit_by_name (GTK_OBJECT (graph->adjustment), "value_changed");

	return FALSE;
}

static void wc_graph_update(WcGraph *graph)
{
	gfloat new_value;

	g_return_if_fail (graph != NULL);
	g_return_if_fail (WC_IS_GRAPH (graph));

	new_value = graph->adjustment->value;

	if (new_value < graph->adjustment->lower)
		new_value = graph->adjustment->lower;

	if (new_value > graph->adjustment->upper)
		new_value = graph->adjustment->upper;

   if (new_value != graph->adjustment->value)
    {
      graph->adjustment->value = new_value;
      gtk_signal_emit_by_name (GTK_OBJECT (graph->adjustment), "value_changed");
    }

	graph->angle = 7.*M_PI/6. - (new_value - graph->adjustment->lower) * 4.*M_PI/3. /
		(graph->adjustment->upper - graph->adjustment->lower);

	gtk_widget_draw (GTK_WIDGET(graph), NULL);
}

static void wc_graph_adjustment_changed(GtkAdjustment *adjustment,
					gpointer data)
{
	WcGraph *graph;

	g_return_if_fail (adjustment != NULL);
	g_return_if_fail (data != NULL);

	graph = WC_GRAPH(data);

	if ((graph->old_value != adjustment->value) ||
			(graph->old_lower != adjustment->lower) ||
			(graph->old_upper != adjustment->upper))
	{
		wc_graph_update (graph);

		graph->old_value = adjustment->value;
		graph->old_lower = adjustment->lower;
		graph->old_upper = adjustment->upper;
	}
}

static void wc_graph_adjustment_value_changed(GtkAdjustment *adjustment,
						gpointer data)
{
	WcGraph *graph;

	g_return_if_fail (adjustment != NULL);
	g_return_if_fail (data != NULL);

	graph = WC_GRAPH (data);

	if (graph->old_value != adjustment->value)
	{
		wc_graph_update (graph);

		graph->old_value = adjustment->value;
	}
} 

void wc_graph_set_value(WcGraph *graph, int which, int value)
{
	graph->nvalue[which] = value;
}
