
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

#include "config.h"
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
static void wc_graph_update(WcGraph *graph);
static void wc_graph_adjustment_changed(GtkAdjustment *adjustment,
					gpointer data);
static void wc_graph_adjustment_value_changed (GtkAdjustment *adjustment,
						gpointer data);

static GtkWidgetClass *parent_class = NULL;
static GdkPixmap *pixmap = NULL;
GdkColor graph_color = {0, 0xaaaa, 0x0000, 0x0000};

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
			NULL,
			NULL,
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

  gdk_color_alloc(gdk_colormap_get_system(), &graph_color);

  graph->button = 0;
  graph->policy = GTK_UPDATE_CONTINUOUS; 
  graph->old_value = 0.0;
  graph->old_lower = 0.0;
  graph->old_upper = 0.0;
  graph->adjustment = NULL;
  for(ii=0;ii<32;ii++)graph->nvalue[ii] = 0.;
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

	if (graph->adjustment) {
#ifdef GTK2
	  g_object_unref (G_OBJECT (graph->adjustment));
#else
	  gtk_object_unref (GTK_OBJECT (graph->adjustment));
#endif
	  graph->adjustment = NULL;
	}

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
#ifdef GTK2 

		g_object_unref (G_OBJECT (graph->adjustment));
#else
		gtk_signal_disconnect_by_data (GTK_OBJECT (graph->adjustment), (gpointer) graph);
		gtk_object_unref (GTK_OBJECT (graph->adjustment));
#endif
	}

	graph->adjustment = adjustment;
#ifdef GTK2
	g_object_ref (G_OBJECT (graph->adjustment));
#else
	gtk_object_ref (GTK_OBJECT (graph->adjustment));
#endif
	gtk_object_sink (GTK_OBJECT (graph->adjustment));

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
	requisition->width = 242;
	requisition->height = 172;
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
}

gint wc_graph_repaint(GtkWidget *widget)
{
  WcGraph *graph;
  GdkPoint points[6];
  char temptext[16];
  char units[8];
  gdouble s,c;
  gfloat theta, last, increment, upper_bound; 
  guint xc, yc;
  gint width, height, heightb, bar_width, bar_height, tmp;
  gint denom;
  gint upper, lower; 
  gint i, inc;
  gint lmargin=2, rmargin=48, bmargin=20, tmargin=20;
  int ii;
  int tmax;

  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (WC_IS_GRAPH (widget), FALSE);

  graph = WC_GRAPH (widget);

  if((graph->adjustment->upper)/1000000 >= 1){
    denom = 1000000;
    sprintf(temptext, "%.0f", (graph->adjustment->upper)/1000000);
    strcpy(units, " M");
    strcat(temptext, units);
  } else if ((graph->adjustment->upper)/1000 >= 1){
    denom = 1000;
    sprintf(temptext, "%.0f", (graph->adjustment->upper)/1000);
    strcpy(units, " K");
    strcat(temptext, units);
  } else {
    denom = 1;
    sprintf(temptext, "%.0f", graph->adjustment->upper);
    strcpy(units, ""); 
  }

  gdk_draw_rectangle(pixmap,
      widget->style->base_gc[widget->state],
      TRUE, 0, 0,
      widget->allocation.width,
      widget->allocation.height);

  width = widget->allocation.width;
  height = widget->allocation.height;
  heightb = height - bmargin - tmargin;

  bar_width = width/32 - 1;
  upper_bound = graph->adjustment->upper;

  gdk_gc_set_line_attributes(widget->style->black_gc, 
      1,
      GDK_LINE_SOLID,
      GDK_CAP_ROUND,
      GDK_JOIN_ROUND);
  gdk_draw_line(pixmap,
      widget->style->black_gc,
      lmargin, height - bmargin,
      width - rmargin, height - bmargin);
  gdk_draw_line(pixmap,
      widget->style->black_gc,
      width - rmargin, tmargin,
      width - rmargin, height - bmargin);

  if ((tmax = atoi(temptext)) != 0 ) {
    while (tmax > 10)
      tmax /= 10;
    for (ii=0;ii<tmax;ii++) {
      gdk_draw_line(pixmap,
	  widget->style->black_gc,
	  width - rmargin, tmargin + (ii*heightb)/tmax,
	  width - rmargin + 3, tmargin + (ii*heightb)/tmax);
    }
  }

#ifdef GTK2
  gdk_draw_text(pixmap,
// XXX fix Pango
      gdk_font_from_description (widget->style->font_desc), 
      widget->style->fg_gc[widget->state],
      width - rmargin,
      16,
      temptext, strlen(temptext));
#else
  gdk_draw_text(pixmap, 
      widget->style->font, 
      widget->style->fg_gc[widget->state],
      width - rmargin,
      16,
      temptext, strlen(temptext));
#endif

  for(ii=0;ii<32;ii++){
    if(upper_bound) {
      bar_height = (gint) ((graph->nvalue[ii])*heightb/upper_bound);
    }
    else bar_height = 0;
    if (bar_height) {
      tmp = height-bar_height-bmargin;
      if (tmp < 2) {
       	tmp = 2;
       	bar_height = height - bmargin - 2;
      }
      gdk_draw_rectangle(pixmap, 
	  graph->gc,
	  TRUE, 
	  (ii*bar_width + lmargin),
	  tmp,
	  (bar_width-1),
	  (bar_height-1)); 
    }
  }

  if (denom > 1)
    sprintf(temptext, "%.1f", (float) graph->nvalue[31]/denom);
  else if (denom == 1)
    sprintf(temptext, "%.0f", (float) graph->nvalue[31]);
  strcat(temptext, units);

#ifdef GTK2
  gdk_draw_text(pixmap,
// XXX fix Pango
  gdk_font_from_description (widget->style->font_desc), 
  widget->style->fg_gc[widget->state],
  width - rmargin + 8,
  (height-bar_height-bmargin),
  temptext, strlen(temptext));
#else
  gdk_draw_text(pixmap, 
      widget->style->font, 
      widget->style->fg_gc[widget->state],
      width - rmargin + 8,
      (height-bar_height-bmargin),
      temptext, strlen(temptext));
#endif

  strcpy(temptext, "- t");

#ifdef GTK2
  gdk_draw_text(pixmap,
// XXX fix Pango
  gdk_font_from_description (widget->style->font_desc), 
  widget->style->fg_gc[widget->state],
  8,
  height - 6,
  temptext, strlen(temptext));
#else
  gdk_draw_text(pixmap, 
      widget->style->font, 
      widget->style->fg_gc[widget->state],
      8,
      height - 6,
      temptext, strlen(temptext));
#endif

  return FALSE;
}

static gint wc_graph_expose(GtkWidget *widget, GdkEventExpose *event)
{ 
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
  WcGraph *graph = WC_GRAPH (widget);

  if(pixmap){ 
    gdk_pixmap_unref(pixmap);
  }
  pixmap = gdk_pixmap_new(widget->window,
      widget->allocation.width,
      widget->allocation.height,
      -1);

  if(graph->gc){
    gdk_gc_unref(graph->gc);
  }
  graph->gc = gdk_gc_new(pixmap);

  gdk_gc_set_foreground(graph->gc, &graph_color);

  return TRUE;
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

void wc_graph_set_value(WcGraph *graph, int which, gfloat value)
{
	graph->nvalue[which] = value;
}
