
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
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>

#include "wcpie.h"

#define DEFAULT_PIE_WIDTH 200
#define DEFAULT_PIE_HEIGHT 200

#define DEFAULT_SND_COLOR 0x00ff00
#define DEFAULT_PATH_COLOR 0xffff00
#define DEFAULT_RCV_COLOR 0xff0000


static void wc_pie_class_init(WcPieClass *klass);
static void wc_pie_init(WcPie *pie);
static void wc_pie_destroy(GtkObject *object);
static void wc_pie_realize(GtkWidget *widget);
static void wc_pie_size_request(GtkWidget *widget, GtkRequisition *requisition);
static void wc_pie_size_allocate(GtkWidget *widget, GtkAllocation *allocation);
static gint wc_pie_expose(GtkWidget *widget, GdkEventExpose *event);
static gint wc_pie_configure(GtkWidget *widget, GdkEventConfigure *event);
static gint wc_pie_button_press(GtkWidget *widget, GdkEventButton *event);
static gint wc_pie_button_release(GtkWidget *widget, GdkEventButton *event);
static gint wc_pie_motion_notify(GtkWidget *widget, GdkEventMotion *event);
static gint wc_pie_timer(WcPie *pie); 
static void wc_pie_update(WcPie *pie);
static void wc_pie_adjustment_changed(GtkAdjustment *adjustment, gpointer data);
static void wc_pie_adjustment_value_changed(GtkAdjustment *adjustment, gpointer data);

static GtkWidgetClass *parent_class = NULL;
static GdkPixmap *pixmap = NULL; 

GdkColor red = {0, 0xaaaa, 0x0000, 0x0000};
GdkColor green = {0, 0x0000, 0xaaaa, 0x0000};
GdkColor yellow = {0, 0xcccc, 0xcccc, 0x0000};
GdkColor black = {0, 0x0000, 0x0000, 0x0000};

GdkColor pie_color[3];

char temptext[60];

#ifdef GTK2
GType
wc_pie_get_type (void)
{
  static GType wc_pie_type = 0;

  if (!wc_pie_type)
  {
    static const GTypeInfo wc_pie_info =
    {
      sizeof (WcPieClass),
      NULL,
      NULL,
      (GClassInitFunc) wc_pie_class_init,
      NULL,
      NULL,
      sizeof (WcPie),
      0,
      (GInstanceInitFunc) wc_pie_init,
    };

    wc_pie_type = g_type_register_static (GTK_TYPE_WIDGET, "WcPie", &wc_pie_info, 0); 
  }

  return wc_pie_type;
}
#else
GtkType
wc_pie_get_type (void)
{
  static GtkType wc_pie_type = 0;

  if(!wc_pie_type)
    {
      static const GtkTypeInfo wc_pie_info =
      {
	"WcPie",
	sizeof (WcPie),
	sizeof (WcPieClass),
	(GtkClassInitFunc) wc_pie_class_init,
	(GtkObjectInitFunc) wc_pie_init,
        NULL,
        NULL,
	(GtkClassInitFunc) NULL,
      };

      wc_pie_type = gtk_type_unique (GTK_TYPE_WIDGET, &wc_pie_info);
    }

  return wc_pie_type;
}
#endif

static void
wc_pie_class_init(WcPieClass *class)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;

  object_class = (GtkObjectClass*) class;
  widget_class = (GtkWidgetClass*) class;

#ifdef GTK2
  parent_class = g_type_class_peek_parent (class);
#else
  parent_class = gtk_type_class (gtk_widget_get_type ());
#endif

  object_class->destroy = wc_pie_destroy;

  widget_class->realize = wc_pie_realize;
  widget_class->expose_event = wc_pie_expose;
  widget_class->configure_event = wc_pie_configure;
  widget_class->size_request = wc_pie_size_request;
  widget_class->size_allocate = wc_pie_size_allocate; 

#ifdef GTK2
  gtk_widget_class_install_style_property (widget_class,
      g_param_spec_int ("snd_color",
       	"Snd color",
       	"Color of Sender segment",
       	0,
       	G_MAXINT,
       	DEFAULT_SND_COLOR,
       	G_PARAM_READABLE));
  gtk_widget_class_install_style_property (widget_class,
      g_param_spec_int ("path_color",
       	"Path color",
       	"Color of Congestion segment",
       	0,
       	G_MAXINT,
       	DEFAULT_PATH_COLOR,
       	G_PARAM_READABLE));
  gtk_widget_class_install_style_property (widget_class,
      g_param_spec_int ("rcv_color",
       	"Rcv color",
       	"Color of Receiver segment",
       	0,
       	G_MAXINT,
       	DEFAULT_RCV_COLOR,
       	G_PARAM_READABLE));
  gtk_widget_class_install_style_property (widget_class,
      g_param_spec_string ("test",
       	"Test",
       	"test",
	NULL,
       	G_PARAM_READABLE));
#endif
}

static void
wc_pie_init(WcPie *pie)
{
  int ii;
  gint style_color[3];
  gfloat testfloat[3];
  char teststr[16];

  pie->button = 0;
  pie->policy = GTK_UPDATE_CONTINUOUS;
  pie->timer = 0;
  pie->radius = 0;

  for (ii=0;ii<3;ii++) pie->adjustment[ii] = NULL;

#ifdef GTK2
  gtk_widget_ensure_style (GTK_WIDGET (pie));
  gtk_widget_style_get (GTK_WIDGET (pie), "snd_color", &style_color[0], NULL);
  gtk_widget_style_get (GTK_WIDGET (pie), "path_color", &style_color[1], NULL);
  gtk_widget_style_get (GTK_WIDGET (pie), "rcv_color", &style_color[2], NULL); 
#else
  style_color[0] = DEFAULT_SND_COLOR; 
  style_color[1] = DEFAULT_PATH_COLOR; 
  style_color[2] = DEFAULT_RCV_COLOR; 
#endif

  for (ii=0;ii<3;ii++) { 
    pie_color[ii].red = (u_int16_t) ((style_color[ii] & 0xff0000) >> 16)*0x101;
    pie_color[ii].green = (u_int16_t) ((style_color[ii] & 0x00ff00) >> 8)*0x101;
    pie_color[ii].blue = (u_int16_t) (style_color[ii] & 0x0000ff)*0x101; 
  }
  for (ii=0;ii<3;ii++) gdk_color_alloc(gdk_colormap_get_system(), &pie_color[ii]);
}

GtkWidget*
wc_pie_new(GtkAdjustment *adjustment[3])
{
  WcPie *pie;
  GtkAdjustment *tempadj;
  GtkStyle *defstyle;
  int ii;
  gint style_color[3];

#ifdef GTK2
  pie = g_object_new (wc_pie_get_type (), NULL);
#else
  pie = gtk_type_new (wc_pie_get_type ());
#endif
  for(ii=0;ii<3;ii++)
  {
    if (!adjustment[ii]) 
      adjustment[ii] = (GtkAdjustment*) gtk_adjustment_new(0, 0, 0, 0, 0, 0); 
    wc_pie_set_adjustment (pie, adjustment[ii], ii);
  }

  return GTK_WIDGET (pie);
}

static void
wc_pie_destroy(GtkObject *object)
{
  WcPie *pie;
  int ii;

  g_return_if_fail (object != NULL);
  g_return_if_fail (WC_IS_PIE(object));

  pie = WC_PIE(object);

  for(ii=0;ii<3;ii++)
  {
    if (pie->adjustment[ii])
      gtk_object_unref(GTK_OBJECT(pie->adjustment[ii]));
    pie->adjustment[ii] = NULL;
  }

  if (GTK_OBJECT_CLASS(parent_class)->destroy)
    (* GTK_OBJECT_CLASS(parent_class)->destroy) (object);
}

GtkAdjustment*
wc_pie_get_adjustment(WcPie *pie, int ii)
{
  g_return_val_if_fail (pie != NULL, NULL);
  g_return_val_if_fail (WC_IS_PIE (pie), NULL);

  return pie->adjustment[ii];
}

void
wc_pie_set_update_policy(WcPie *pie, GtkUpdateType policy)
{
  g_return_if_fail (pie != NULL);
  g_return_if_fail (WC_IS_PIE (pie));

  pie->policy = policy;
}

void
wc_pie_set_adjustment(WcPie *pie, GtkAdjustment *adjustment, int ii)
{
  g_return_if_fail (pie != NULL);
  g_return_if_fail (WC_IS_PIE (pie));

  if (pie->adjustment[ii])
  { 
    gtk_signal_disconnect_by_data (GTK_OBJECT (pie->adjustment[ii]), (gpointer) pie);
    gtk_object_unref (GTK_OBJECT (pie->adjustment[ii]));
  }

  pie->adjustment[ii] = adjustment; 
  gtk_object_ref(GTK_OBJECT (pie->adjustment[ii])); 
  gtk_object_sink (GTK_OBJECT (pie->adjustment[ii]));
  gtk_signal_connect(GTK_OBJECT (adjustment), "changed",
      (GtkSignalFunc) wc_pie_adjustment_changed,
      (gpointer) pie);

  gtk_signal_connect(GTK_OBJECT (adjustment), "value_changed",
      (GtkSignalFunc) wc_pie_adjustment_value_changed,
      (gpointer) pie);

  wc_pie_update(pie); 
}

static void
wc_pie_realize(GtkWidget *widget)
{
  WcPie *pie;
  GdkWindowAttr attributes;
  gint attributes_mask;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (WC_IS_PIE (widget));

  GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);
  pie = WC_PIE (widget);

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

  widget->style = gtk_style_attach(widget->style, widget->window);

  gdk_window_set_user_data(widget->window, widget);

  gtk_style_set_background(widget->style, widget->window, GTK_STATE_ACTIVE);
}

static void
wc_pie_size_request(GtkWidget *widget, GtkRequisition *requisition)
{ 
  requisition->width = DEFAULT_PIE_WIDTH; 
  requisition->height = DEFAULT_PIE_HEIGHT;
}

static void
wc_pie_size_allocate(GtkWidget *widget, GtkAllocation *allocation)
{
  WcPie *pie;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (WC_IS_PIE (widget));
  g_return_if_fail (allocation != NULL);

  widget->allocation = *allocation;
  pie = WC_PIE (widget);

  if (GTK_WIDGET_REALIZED (widget))
  { 
    gdk_window_move_resize (widget->window,
       	allocation->x, allocation->y,
       	allocation->width, allocation->height); 
  }
  pie->radius = MIN(allocation->width,allocation->height) * 0.33; 
}

gint
wc_pie_repaint(GtkWidget *widget) 
{
  WcPie *pie;
  GdkPoint points[6];
  gdouble s,c;
  gdouble theta, last, increment; 
  gint xc, yc, temp, temp2=0;
  gint upper, lower;
  gint tick_length;
  gint i, inc;
  int ii, jj;

  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (WC_IS_PIE (widget), FALSE);

  pie = WC_PIE (widget);

  gdk_draw_rectangle(pixmap,
      widget->style->base_gc[widget->state],
      TRUE,
      0, 0,
      widget->allocation.width,
      widget->allocation.height);

  xc = widget->allocation.width/2;
  yc = widget->allocation.height/2;

  for(jj=0;jj<3;jj++){
    for(ii=0;ii<(pie->npts[jj]);ii++){
      pie->pts[jj][ii].x = xc + (pie->radius)*cos((ii+temp2)*M_PI/90 + M_PI/2);
      pie->pts[jj][ii].y = yc - (pie->radius)*sin((ii+temp2)*M_PI/90 + M_PI/2); 
    }
    temp = pie->npts[jj];

    pie->pts[jj][temp].x = xc; 
    pie->pts[jj][temp].y = yc;

    temp2 += temp - 1; 
  }

/* fudge for correct drawing of polygons */
  if(pie->npts[2]){
    if(pie->npts[0] || pie->npts[1]){
      pie->pts[2][pie->npts[2]-1].x = xc;
      pie->pts[2][pie->npts[2]-1].y = yc - (pie->radius); 
    } else { 
      pie->pts[2][pie->npts[2]].x = xc;
      pie->pts[2][pie->npts[2]].y = yc - (pie->radius);
    }
  } else if(pie->npts[1]){
    if(pie->npts[0]){
      pie->pts[1][pie->npts[1]-1].x = xc;
      pie->pts[1][pie->npts[1]-1].y = yc - (pie->radius); 
    } else {
      pie->pts[1][pie->npts[1]].x = xc;
      pie->pts[1][pie->npts[1]].y = yc - (pie->radius);
    }
  } else if(pie->npts[0]){
    pie->pts[0][pie->npts[0]].x = xc;
    pie->pts[0][pie->npts[0]].y = yc - (pie->radius);
  }

  for(jj=0;jj<3;jj++){
    gdk_draw_polygon(pixmap,
       	pie->gc[jj],
       	TRUE,
       	pie->pts[jj],
       	pie->npts[jj]+1); 
  }

  gdk_gc_set_line_attributes(widget->style->black_gc,
      3,
      GDK_LINE_SOLID,
      GDK_CAP_ROUND,
      GDK_JOIN_ROUND);

  gdk_draw_arc(pixmap,
      widget->style->black_gc,
      0, xc-(pie->radius), yc-(pie->radius),
      2*(pie->radius), 2*(pie->radius), 0, 360*64);

  gdk_draw_line(pixmap,
      widget->style->black_gc,
      xc, yc,
      xc, yc - (pie->radius));
  if(pie->npts[0] && pie->npts[1])
    gdk_draw_line(pixmap,
       	widget->style->black_gc,
       	xc, yc, 
	xc + (pie->radius)*cos((pie->npts[0]-1)*M_PI/90+M_PI/2), 
	yc - (pie->radius)*sin((pie->npts[0]-1)*M_PI/90+M_PI/2));
  if((pie->npts[0] || pie->npts[1]) && pie->npts[2])
    gdk_draw_line(pixmap,
       	widget->style->black_gc,
       	xc, yc, 
	xc + (pie->radius)*cos((pie->npts[0]+pie->npts[1]-2)*M_PI/90+M_PI/2), 
	yc - (pie->radius)*sin((pie->npts[0]+pie->npts[1]-2)*M_PI/90+M_PI/2));

  gdk_gc_set_line_attributes(widget->style->black_gc,
      1,
      GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_ROUND);
  strcpy(temptext, "Send");
  gdk_draw_text(pixmap,
#ifdef GTK2 
      gdk_font_from_description (widget->style->font_desc), 
#else
      widget->style->font,
#endif
      pie->gc[0],
      xc - 70, 
      yc - (pie->radius) - 8,
      temptext, strlen(temptext));
  strcpy(temptext, "Receive");
  gdk_draw_text(pixmap,
#ifdef GTK2 
      gdk_font_from_description (widget->style->font_desc),
#else
      widget->style->font,
#endif

      pie->gc[2],
      xc + 30,
      yc - (pie->radius) - 8,
      temptext, strlen(temptext)); 
  strcpy(temptext, "Path");
  gdk_draw_text(pixmap,
#ifdef GTK2 
      gdk_font_from_description (widget->style->font_desc),
#else
      widget->style->font,
#endif 
      pie->gc[1],
      xc - 16, 
      DEFAULT_PIE_HEIGHT*17/18,
      temptext, strlen(temptext));

  return FALSE;
}

static
gint wc_pie_expose(GtkWidget *widget, GdkEventExpose *event)
{ 
  wc_pie_configure(widget, NULL);
  wc_pie_repaint(widget);

  gdk_draw_pixmap(widget->window,
      widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
      pixmap,
      event->area.x, event->area.y,
      event->area.x, event->area.y,
      event->area.width, event->area.height);
}

static
gint wc_pie_configure(GtkWidget *widget, GdkEventConfigure *event)
{ 
  WcPie *pie;
  int ii;

  pie = WC_PIE (widget);

  if(pixmap){ 
    gdk_pixmap_unref(pixmap);
  }
  pixmap = gdk_pixmap_new(widget->window,
      widget->allocation.width,
      widget->allocation.height,
      -1);
  for(ii=0;ii<3;ii++){
    if(pie->gc[ii]){
      gdk_gc_unref(pie->gc[ii]);
    }
    pie->gc[ii] = gdk_gc_new(pixmap);
  }

  for (ii=0;ii<3;ii++) { 
    gdk_gc_set_foreground(pie->gc[ii], (GdkColor *) &pie_color[ii]); 
  }

  return TRUE;
}

static gint
wc_pie_timer (WcPie *pie)
{
  g_return_val_if_fail (pie != NULL, FALSE);
  g_return_val_if_fail (WC_IS_PIE (pie), FALSE);

  if (pie->policy == GTK_UPDATE_DELAYED)
    gtk_signal_emit_by_name (GTK_OBJECT (pie->adjustment[0]), "value_changed");

  return FALSE;
}

static void
wc_pie_update (WcPie *pie)
{
  gfloat new_value;
  gint xc, yc;
  int ii, jj, total = 0, temp = 0; 

  g_return_if_fail(pie != NULL);
  g_return_if_fail(WC_IS_PIE(pie));

  for(ii=0;ii<3;ii++)
  {
    if(pie->adjustment[ii]){ 
      new_value = pie->adjustment[ii]->value;
      if (new_value < pie->adjustment[ii]->lower)
       	new_value = pie->adjustment[ii]->lower;
      if (new_value > pie->adjustment[ii]->upper)
       	new_value = pie->adjustment[ii]->upper;
      if (new_value != pie->adjustment[ii]->value)
      {
       	pie->adjustment[ii]->value = new_value;
       	gtk_signal_emit_by_name (GTK_OBJECT (pie->adjustment[ii]), "value_changed");
      }
      if(pie->adjustment[ii]->upper - pie->adjustment[ii]->lower)
      { 
	pie->npts[ii] = rint((new_value - (pie->adjustment[ii]->lower))*180 / (pie->adjustment[ii]->upper - pie->adjustment[ii]->lower)); 
      }
      else pie->npts[ii] = 0; 
      total += pie->npts[ii]; temp += pie->npts[ii]-1;
    }
  }

  if(total) for(ii=0;ii<3;ii++) pie->npts[ii] = pie->npts[ii]*180/total;

  gtk_widget_draw (GTK_WIDGET(pie), NULL);
}

static void
wc_pie_adjustment_changed(GtkAdjustment *adjustment, gpointer data)
{
  WcPie *pie;

  g_return_if_fail(adjustment != NULL);
  g_return_if_fail(data != NULL);

  pie = WC_PIE(data);

  wc_pie_update(pie);
}

static void
wc_pie_adjustment_value_changed(GtkAdjustment *adjustment, gpointer data)
{
  WcPie *pie;

  g_return_if_fail(adjustment != NULL);
  g_return_if_fail(data != NULL);

  pie = WC_PIE(data);

  wc_pie_update(pie);
}

