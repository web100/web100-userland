#include <stdlib.h>
#include <stdio.h> 
#include <math.h>
#include <gtk/gtk.h>

#include "display.h"
#include "wcgraph.h"


static void display_class_init (DisplayClass *klass);
static void display_init (Display *display);
static void display_destroy (GtkObject *object);
static void display_snap_update (GtkObject *object, gpointer data);
static void display_set_object (Web100Widget *widget, Web100Object *object); 
static void display_set_var (Display *display, const char *varname);
static float display_get_ewma (float value, float oldewma, float weight);
static float display_newmax (float val, float oldmax);
static void display_entry_cb (GtkEntry *entry, gpointer data); 
static void display_construct (Display *display, Web100Object *web100obj, char *name);
static void display_zoom (GtkButton *, gpointer);
static void display_toggle_smooth (GtkToggleButton *, gpointer);

static Web100WidgetClass *parent_class = NULL;


#ifdef GTK2
GType
display_get_type (void)
{
  static GType display_type = 0;

  if (!display_type)
  {
    static const GTypeInfo display_info =
    {
      sizeof (DisplayClass),
      NULL,
      NULL,
      (GClassInitFunc) display_class_init,
      NULL,
      NULL,
      sizeof (Display),
      0,
      (GInstanceInitFunc) display_init,
    };

    display_type = g_type_register_static (TYPE_WEB100_WIDGET, "Display", &display_info, 0);
  }

  return display_type;
}
#else
GtkType
display_get_type (void)
{
  static GtkType display_type = 0;

  if(!display_type)
    {
      GtkTypeInfo display_info =
      {
	"Display",
	sizeof (Display),
	sizeof (DisplayClass),
	(GtkClassInitFunc) display_class_init,
	(GtkObjectInitFunc) display_init,
        NULL,
        NULL
      };

      display_type = gtk_type_unique (web100_widget_get_type (), &display_info);
    }

  return display_type;
}
#endif

static void
display_class_init (DisplayClass *class)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;
  Web100WidgetClass *web100widget_class;

  object_class = (GtkObjectClass*) class;
  widget_class = (GtkWidgetClass*) class;
  web100widget_class = (Web100WidgetClass*) class;

#ifdef GTK2 
  parent_class = g_type_class_peek_parent (class);
#else
  parent_class = gtk_type_class (TYPE_WEB100_WIDGET);
#endif

  object_class->destroy = display_destroy;

  web100widget_class->snap_update = display_snap_update;
  web100widget_class->set_object = display_set_object;
}

static void
display_init (Display *display)
{ 
  int ii;

  GTK_WIDGET_SET_FLAGS (display, GTK_NO_WINDOW); 

  display->var = NULL;
  display->group = NULL;
  display->graph = NULL;

  display->ewma = display->oldewma = display->max = display->oldmax = 0.;
  for(ii=0;ii<32;ii++) display->graphval[ii] = 0.;
  display->step = 0;

  display->delta_on = display->smooth_on = 0;
}

GtkWidget*
display_new (Web100Object *web100obj, char *name)
{ 
  Display *display;
#ifdef GTK2
  display = g_object_new (TYPE_DISPLAY, NULL);
#else
  display = gtk_type_new (display_get_type ()); 
#endif
 
  web100_widget_set_object (WEB100_WIDGET (display), web100obj);
  display_construct (display, web100obj, name);
  display_set_var (display, name);

  return GTK_WIDGET (display); 
}

static void
display_destroy (GtkObject *object)
{ 
  g_return_if_fail (object != NULL);
  g_return_if_fail (IS_DISPLAY (object));

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
display_snap_update (GtkObject *object, gpointer data)
{ 
  GtkAdjustment *adj[2]; 
  float result;
  int ii;
  char buf[256];
  u_int64_t val, del;
  Web100Object *web100obj;
  Display *display;
  web100_var *var;
  struct snapshot_list *snap;
  char valtext[64];
  int step;

  web100obj = WEB100_OBJECT (object);
  display = DISPLAY (data);
  step = display->step;

  if (!display->var) return;
  var = display->var;

  for (snap = web100obj->snapshot_head; snap != NULL; snap = snap->next) {
    if (!strncmp(web100_get_group_name(snap->group), web100_get_group_name(display->group), WEB100_GROUPNAME_LEN_MAX))
      break;
  } 

  if (!snap->last) return;

  web100_snap_read(var, snap->last, &buf);

  switch (web100_get_var_type(var)) {
    case WEB100_TYPE_IP_ADDRESS:
      return;
    case WEB100_TYPE_UNSIGNED16:
      val = *(u_int16_t *)buf;
      break;
    case WEB100_TYPE_COUNTER64:
      val = *(u_int64_t *)buf;
      break;
    default:
      val = *(u_int32_t *)buf;
      break;
  }

  if (display->delta_on) {
    if (!snap->prior) return;
    web100_delta_any(var, snap->last, snap->prior, &buf);

    switch (web100_get_var_type(var)) {
      case WEB100_TYPE_IP_ADDRESS:
       	return;
      case WEB100_TYPE_UNSIGNED16:
       	del = *(u_int16_t *)buf;
       	break;
      case WEB100_TYPE_COUNTER64:
       	del = *(u_int64_t *)buf;
       	break;
      default:
       	del = *(u_int32_t *)buf;
       	break;
    } 
    result = (float) del; 
  }
  else 
    result = (float) val;

  if(display->smooth_on) {
    display->oldewma = display->ewma;
    if (display->oldewma)
      result = display->ewma = display_get_ewma(result, display->oldewma, 0.4);
    else display->ewma = result;
  }

  display->max = display_newmax(result, display->oldmax); 
  display->oldmax = display->max;

  adj[1] = GTK_ADJUSTMENT(gtk_adjustment_new(result, 0, display->max, 0.01, 0.1, 0));
  wc_graph_set_adjustment(WC_GRAPH(display->graph), GTK_ADJUSTMENT(adj[1]));

  display->graphval[step] = result;

  for(ii=0;ii<32;ii++){
    wc_graph_set_value(WC_GRAPH(display->graph), ii, display->graphval[(ii+step+1)%32]);
  } 
  display->step = (step+1)%32;

  gtk_widget_draw(display->graph, NULL); 

  sprintf(valtext, "%llu", val);
  gtk_clist_set_text(GTK_CLIST(display->clist), 0, 1, valtext);
  if(display->delta_on) sprintf(valtext, "%llu", del);
  else strcpy(valtext, " ");
  gtk_clist_set_text(GTK_CLIST(display->clist), 1, 1, valtext);
  if(display->smooth_on) sprintf(valtext, "%.1f", result);
  else strcpy(valtext, " ");
  gtk_clist_set_text(GTK_CLIST(display->clist), 2, 1, valtext);
}

static void
display_set_object (Web100Widget *widget, Web100Object *object)
{
  web100_widget_set_object (widget, object);
  
  if (DISPLAY (widget)->var)
    display_set_var (DISPLAY (widget), web100_get_var_name (DISPLAY (widget)->var));
}


static void
display_set_var (Display *display, const char *varname)
{ 
  Web100Object *web100obj;
  int ii;

  g_return_if_fail (display != NULL);
  g_return_if_fail (IS_DISPLAY (display));
 
  if (!varname) return;

  web100obj = WEB100_WIDGET (display)->web100object;

  if (web100_agent_find_var_and_group(web100obj->agent, varname, &display->group, &display->var))
   return;

  if ((web100_get_var_type(display->var) != WEB100_TYPE_COUNTER32) &&
      (web100_get_var_type(display->var) != WEB100_TYPE_COUNTER64)) {
    display->delta_on = display->smooth_on = 0;
  }
  else display->delta_on = display->smooth_on = 1;

  if (display->smooth_on) {
    gtk_signal_handler_block_by_func (GTK_OBJECT (display->smooth_toggle), GTK_SIGNAL_FUNC (display_toggle_smooth), display);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (display->smooth_toggle), TRUE);
    gtk_signal_handler_unblock_by_func (GTK_OBJECT (display->smooth_toggle), GTK_SIGNAL_FUNC (display_toggle_smooth), display); 
  }

  display->ewma = display->oldewma = display->max = 0.;
  display->oldmax = 10.;
  for(ii=0;ii<32;ii++) display->graphval[ii] = 0.;
  display->step = 0;
}

static float
display_get_ewma(float value, float oldewma, float weight)
{
  float ewma;

  ewma = weight*value + (1-weight)*oldewma;
  return(ewma);
}

static float
display_newmax(float val, float oldmax)
{
  float m, decade;

#define CHECKMAX(s) {if (val <= (m=(s)*decade)) { return(m); }}

  if (val > oldmax){
    decade = pow(10, floor(log10(val)));
    CHECKMAX(1.0); 
    CHECKMAX(2.0);
    CHECKMAX(5.0);
    CHECKMAX(10.0);
  }
  else return oldmax;
}

static void
display_entry_cb (GtkEntry *entry, gpointer data)
{
  const char *varname = NULL;

  varname = gtk_entry_get_text (entry);

  display_set_var (DISPLAY (data), varname);
}

static void
display_construct (Display *display, Web100Object *web100obj, char *varname)
{ 
  GtkWidget *infobox, *vbox, *frame, *entry, *radiobox, *align, *clist,
            *button2, *smoothbox, *displaybox, *hbox, *mbutton,
	    *sbutton, *meter, *graph; 
  GtkAdjustment *adjustment;
  GtkWidget *button;
#ifdef GTK2
  GtkStockItem stock_zoom;
  GdkPixbuf *pixbuf;
  GtkImage *stock;
#endif
  GList *items = NULL;
  web100_agent *agent;
  web100_group *group;
  web100_var   *var; 
  char *itext[2] = {NULL, NULL}; 
  char *name;
  int ii;

  display->ewma = display->oldewma = display->max = display->oldmax = 0.;
  for(ii=0;ii<32;ii++) display->graphval[ii] = 0.;

  infobox = gtk_hbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(display), infobox, FALSE, FALSE, 5);
  gtk_widget_show(infobox);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (infobox), vbox, TRUE, TRUE, 10);
  gtk_widget_show (vbox);

  frame = gtk_frame_new(NULL);
  gtk_frame_set_label(GTK_FRAME(frame), "Variable name");
  gtk_frame_set_label_align(GTK_FRAME(frame), 0.5, 0.5); 
  gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show(frame);

  display->combo = gtk_combo_new ();

  agent = WEB100_WIDGET (display)->web100object->agent;
  group = web100_group_find (agent, "read");

  var = web100_var_head (group);
  while (var) { 
    if ((name = malloc (WEB100_VARNAME_LEN_MAX)) == NULL) {
      perror ("Out of memory");
      exit (EXIT_FAILURE);
    }

    strncpy (name, web100_get_var_name (var), WEB100_VARNAME_LEN_MAX); 

    items = g_list_append (items, name); 

    var = web100_var_next (var);
  }

  gtk_combo_set_popdown_strings (GTK_COMBO (display->combo), items);
  gtk_combo_set_value_in_list (GTK_COMBO (display->combo), TRUE, TRUE);
  gtk_combo_set_case_sensitive (GTK_COMBO (display->combo), FALSE);
  gtk_combo_disable_activate (GTK_COMBO (display->combo));

  gtk_container_add(GTK_CONTAINER(frame), display->combo);

  gtk_widget_show(display->combo);

  if (varname)
    gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (display->combo)->entry), varname);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (infobox), vbox, TRUE, TRUE, 10);
  gtk_widget_show (vbox);

  display->clist = gtk_clist_new(2); 

  for(ii=0;ii<3;ii++)
    gtk_clist_append(GTK_CLIST(display->clist), itext);

  gtk_clist_set_text(GTK_CLIST(display->clist), 0, 0, "val");
  gtk_clist_set_text(GTK_CLIST(display->clist), 1, 0, "delta");
  gtk_clist_set_text(GTK_CLIST(display->clist), 2, 0, "smoothed");

  gtk_clist_set_column_auto_resize (GTK_CLIST (display->clist), 0, TRUE);
  gtk_clist_set_column_auto_resize (GTK_CLIST (display->clist), 1, TRUE);

  gtk_box_pack_start (GTK_BOX (vbox), display->clist, TRUE, TRUE, 0);
  gtk_widget_show(display->clist);

  hbox = gtk_hbox_new (FALSE, 0); 
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  display->smooth_toggle = gtk_check_button_new_with_label ("smooth"); 

  gtk_box_pack_start (GTK_BOX (hbox), display->smooth_toggle, FALSE, FALSE, 0);
  gtk_widget_show (display->smooth_toggle);
  gtk_signal_connect (GTK_OBJECT (display->smooth_toggle), "toggled", GTK_SIGNAL_FUNC (display_toggle_smooth), display);

  displaybox = gtk_alignment_new(0.93,0.5,0.0,0.0);
  gtk_widget_show(displaybox);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (displaybox), hbox);
  gtk_widget_show (hbox);

  frame = gtk_frame_new(NULL);
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 10);
  gtk_widget_show(frame);

  adjustment = GTK_ADJUSTMENT(gtk_adjustment_new (0, 0.0, 10.0, 0.01, 0.1, 0));
  display->graph = wc_graph_new(GTK_ADJUSTMENT(adjustment)); 
  wc_graph_set_update_policy (WC_GRAPH(display->graph), GTK_UPDATE_CONTINUOUS);
  gtk_container_add(GTK_CONTAINER(frame), display->graph);    
  gtk_widget_show(display->graph); 

  gtk_box_pack_start(GTK_BOX(display), displaybox, FALSE, FALSE, 10); 

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 4);
  gtk_widget_show (vbox);

#ifdef GTK2
  button = gtk_button_new ();
  g_signal_connect (G_OBJECT (button), "pressed", G_CALLBACK (display_zoom), display);
#else
  button = gtk_button_new_with_label ("Zoom");
  gtk_signal_connect (GTK_OBJECT (button), "pressed", GTK_SIGNAL_FUNC (display_zoom), display);
#endif 
  gtk_box_pack_end(GTK_BOX(vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);
   
#if GTK2
  pixbuf = gtk_widget_render_icon (button, GTK_STOCK_ZOOM_IN, GTK_ICON_SIZE_BUTTON, NULL);
  stock = GTK_IMAGE (gtk_image_new_from_pixbuf (pixbuf));
  gtk_container_add (GTK_CONTAINER (button), GTK_WIDGET (stock));
  gtk_widget_show (GTK_WIDGET (stock)); 
#endif
#ifdef GTK2
  g_signal_connect (G_OBJECT (GTK_COMBO (display->combo)->entry), "changed", G_CALLBACK (display_entry_cb), display);
#else
  gtk_signal_connect (GTK_OBJECT (GTK_COMBO (display->combo)->entry), "changed", GTK_SIGNAL_FUNC (display_entry_cb), display); 
#endif
}

static void
display_zoom (GtkButton *butt, gpointer data)
{
  Display *display = DISPLAY (data);

  display->oldmax = 1;
} 

static void
display_toggle_smooth (GtkToggleButton *button, gpointer data)
{
  Display *display = DISPLAY (data);

  printf("signal\n");
  display->smooth_on = 1 - display->smooth_on;
}






