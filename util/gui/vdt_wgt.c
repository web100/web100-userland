#include <stdlib.h>
#include <stdio.h> 
#include <math.h>
#include <gtk/gtk.h> 
#include "web100.h"
#include "web100obj.h"
#include "wcgraph.h"
#include "wcmeter.h"
#include "vdt_wgt.h"
#include "sockset_wgt.h"


static void vdt_class_init (VdtClass *class);
static void vdt_init (Vdt *vdt);
static void vdt_construct (Vdt *vdt, Web100Obj *web100obj, char *varname);
static void vdt_set_var (Vdt *vdt, char *varname);
static void vdt_web100obj_snap_update (GtkObject *object, gpointer *data);
static void vdt_destroy (GtkObject *object);

static GtkVBoxClass *parent_class = NULL;

#define GRAPH 0
#define METER 1 
static int mode = GRAPH;
static float ewma, oldewma, max, oldmax;
static float graphval[20];
static int smoothing_on=1, delta_on=1;
static char valtext[32];

float get_ewma(float value, float oldewma, float weight)
{
  float ewma;

  ewma = weight*value + (1-weight)*oldewma;
  return(ewma);
}

#define CHECKMAX(s) {if (val <= (m=(s)*decade)) { return(m); }}

float newmax(float val, float oldmax)
{
  float m, decade;

  if (val > oldmax){
    decade = pow(10, floor(log10(val)));
    CHECKMAX(1.0); 
    CHECKMAX(2.0);
    CHECKMAX(5.0);
    CHECKMAX(10.0); // should be redundant
  }
  else return oldmax;
}

GtkType vdt_get_type ()
{
  static guint vdt_type = 0;

  if(!vdt_type)
    {
      GtkTypeInfo vdt_info =
      {
	"Vdt",
	sizeof (Vdt),
	sizeof (VdtClass),
	(GtkClassInitFunc) vdt_class_init,
	(GtkObjectInitFunc) vdt_init,
        (GtkArgSetFunc) NULL,
        (GtkArgGetFunc) NULL
      };

      vdt_type = gtk_type_unique (gtk_vbox_get_type (), &vdt_info);
    }

  return vdt_type;
}

static void vdt_class_init (VdtClass *class)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;

  object_class = (GtkObjectClass*) class;
  widget_class = (GtkWidgetClass*) class;

  parent_class = gtk_type_class (gtk_vbox_get_type ());

  object_class->destroy = vdt_destroy;
}

GtkWidget* vdt_new (Web100Obj *web100obj, char *varname)
{
  Vdt *vdt = gtk_type_new (vdt_get_type ()); 

  vdt_construct (vdt, web100obj, varname); 

  return GTK_WIDGET (vdt); 
}

static void vdt_construct (Vdt *vdt, Web100Obj *web100obj, char *varname)
{ 
  vdt_set_web100obj (vdt, web100obj);

  vdt_set_var (vdt, varname); 
} 

void vdt_set_web100obj (Vdt *vdt, Web100Obj *web100obj)
{
  g_return_if_fail (vdt != NULL);
  g_return_if_fail (IS_VDT (vdt)); 

  if (!web100obj) {
    vdt->web100obj = NULL;
    return;
  }
  else
    g_return_if_fail (IS_WEB100_OBJ (web100obj));

  if (vdt->web100obj) { 
    gtk_signal_disconnect_by_data (GTK_OBJECT (vdt->web100obj),
                                   (gpointer) vdt);
    gtk_object_unref (GTK_OBJECT (vdt->web100obj));
  }

  vdt->web100obj = web100obj; 
  gtk_object_ref (GTK_OBJECT (web100obj)); 
  gtk_object_sink (GTK_OBJECT (web100obj));

  gtk_signal_connect (GTK_OBJECT(web100obj),
                      "snap_update",
                      GTK_SIGNAL_FUNC(vdt_web100obj_snap_update),
                      vdt);
}

Web100Obj* vdt_get_web100obj (Vdt *vdt)
{
  g_return_val_if_fail (vdt != NULL, NULL);
  g_return_val_if_fail (IS_VDT (vdt), NULL);

  return vdt->web100obj;
}

static void vdt_set_var (Vdt *vdt, char *varname)
{
  Web100Obj *web100obj;

  web100obj = WEB100_OBJ (vdt->web100obj);
  if (!varname) return;

  web100_agent_find_var_and_group(web100obj->agent, varname, 
                                  &vdt->group, &vdt->var); 
  gtk_entry_set_text(GTK_ENTRY(vdt->entry), varname);

  if (web100_get_var_type(vdt->var) != WEB100_TYPE_COUNTER32 &&
      web100_get_var_type(vdt->var) != WEB100_TYPE_COUNTER64) {
    delta_on = smoothing_on = 0;
  }
}

void vdt_web100obj_snap_update (GtkObject *object, gpointer *data)
{ 
  GtkAdjustment *adj[2]; 
  float result;
  int ii;
  char buf[8];
  u_int64_t val, del;
  Web100Obj *web100obj;
  Vdt *vdt;
  web100_var *var;
  struct snapshot_data *snap;

  web100obj = WEB100_OBJ (object);
  vdt = VDT (data);

  if (!vdt->var) return;
  var = vdt->var;

  for (snap = web100obj->snapshot_head; snap != NULL; snap = snap->next) {
    if (!strncmp(web100_get_group_name(snap->group), web100_get_group_name(vdt->group), WEB100_GROUPNAME_LEN_MAX))
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

  if (delta_on) {
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
    result = (float)del; 
  }
  else 
    result = (float)val;

  if(smoothing_on) {
    oldewma = ewma;
    result = ewma = get_ewma(result, oldewma, 0.4);
  }

  if(result > oldmax)
  { 
    max = newmax(result, oldmax); 
    oldmax = max;

//    adj[0] = GTK_ADJUSTMENT(gtk_adjustment_new(result, 0, max, 0.01, 0.1, 0));
//    wc_meter_set_adjustment(WC_METER(meter), GTK_ADJUSTMENT(adj[0]));

    adj[1] = GTK_ADJUSTMENT(gtk_adjustment_new(result, 0, max, 0.01, 0.1, 0));
    wc_graph_set_adjustment(WC_GRAPH(vdt->graph), GTK_ADJUSTMENT(adj[1]));
  }

//  gtk_adjustment_set_value(WC_METER(meter)->adjustment, result);
//  gtk_widget_draw(meter, NULL);

  for(ii=0;ii<19;ii++){ graphval[ii] = graphval[ii+1];}
  graphval[19] = result;
  for(ii=0;ii<20;ii++){
    wc_graph_set_value(WC_GRAPH(vdt->graph), ii, graphval[ii]);
  } 
  gtk_widget_draw(vdt->graph, NULL); 

  sprintf(valtext, "%llu", val);
  gtk_clist_set_text(GTK_CLIST(vdt->clist), 0, 1, valtext);
  if(delta_on) sprintf(valtext, "%llu", del);
  else strcpy(valtext, " ");
  gtk_clist_set_text(GTK_CLIST(vdt->clist), 1, 1, valtext);
  if(smoothing_on) sprintf(valtext, "%.1f", result);
  else strcpy(valtext, " ");
  gtk_clist_set_text(GTK_CLIST(vdt->clist), 2, 1, valtext);
}

void vdt_sockset_listen (GtkObject *object, gpointer data)
{ 
  Sockset *sockset;
  Vdt *vdt;
  Web100Obj *web100obj;

  g_return_if_fail (object != NULL);
  g_return_if_fail (GTK_IS_OBJECT (object));
  g_return_if_fail (data != NULL);

  sockset = SOCKSET (object);
  vdt = VDT (data);
  web100obj = sockset->web100obj;
  g_return_if_fail (IS_WEB100_OBJ (web100obj));

  vdt_set_web100obj (VDT (vdt), SOCKSET (sockset)->web100obj);
}

static void vdt_destroy (GtkObject *object)
{
  g_return_if_fail (object != NULL);
  g_return_if_fail (IS_VDT(object));

  if (VDT (object)->web100obj != NULL) { 
    gtk_signal_disconnect_by_data (GTK_OBJECT (VDT (object)->web100obj),
                                   (gpointer) object);
    gtk_object_unref (GTK_OBJECT (VDT (object)->web100obj));
  }

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void view_meter(GtkWidget *butt, gpointer data)
{
//  gtk_widget_hide(displaybox[GRAPH]);
//  gtk_widget_show(displaybox[METER]);
}

static void view_graph(GtkWidget *butt, gpointer data)
{
//  gtk_widget_hide(displaybox[METER]);
//  gtk_widget_show(displaybox[GRAPH]);
}

gint toggle_smooth(GtkWidget *butt, gpointer data)
{
//  smoothing_on = 1-smoothing_on;
}

gint lower_max(GtkWidget *butt, gpointer data)
{
//  oldmax = 1.;
//  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(mbutton), TRUE);
}


static void vdt_init (Vdt *vdt)
{
  GtkWidget *infobox, *vbox, *frame, *entry, *radiobox, *align, *clist,
            *button, *button2, *smoothbox, *displaybox, *hbox, *mbutton,
	    *sbutton, *meter, *graph;
  GtkAdjustment *adjustment;
  GSList *group;
  char *itext[2] = {NULL, NULL}; 
  int ii;

  GTK_WIDGET_SET_FLAGS (vdt, GTK_NO_WINDOW);

  ewma = oldewma = max = oldmax = 0.;
  for(ii=0;ii<20;ii++) graphval[ii] = 0.;

  infobox = gtk_hbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vdt), infobox, FALSE, FALSE, 5);
  gtk_widget_show(infobox);

  frame = gtk_frame_new(NULL);
  gtk_frame_set_label(GTK_FRAME(frame), "Variable name");
  gtk_frame_set_label_align(GTK_FRAME(frame), 0.5, 0.0); 
  gtk_box_pack_start(GTK_BOX(infobox), frame, FALSE, FALSE, 0);
  gtk_widget_show(frame);

  vdt->entry = gtk_entry_new();
  gtk_entry_set_editable(GTK_ENTRY(vdt->entry), FALSE); 
  gtk_container_add(GTK_CONTAINER(frame), vdt->entry);
  gtk_widget_show(vdt->entry);

  align = gtk_alignment_new(0.5,0.5,1.0,0.0);
  gtk_box_pack_start(GTK_BOX(infobox), align, TRUE, TRUE, 5); 
  gtk_widget_show(align);

  vdt->clist = gtk_clist_new(2); 

  for(ii=0;ii<3;ii++)
    gtk_clist_append(GTK_CLIST(vdt->clist), itext);

  gtk_clist_set_text(GTK_CLIST(vdt->clist), 0, 0, "val");
  gtk_clist_set_text(GTK_CLIST(vdt->clist), 1, 0, "delta");
  gtk_clist_set_text(GTK_CLIST(vdt->clist), 2, 0, "smoothed");
  gtk_clist_set_column_width(GTK_CLIST(vdt->clist), 0, 30);
  gtk_clist_set_column_width(GTK_CLIST(vdt->clist), 1, 60);


  gtk_container_add(GTK_CONTAINER(align), vdt->clist);
  gtk_widget_show(vdt->clist);

  displaybox = gtk_alignment_new(0.5,0.5,0.0,0.0);
  gtk_widget_show(displaybox);
  frame = gtk_frame_new(NULL);
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
  gtk_container_add(GTK_CONTAINER(displaybox), frame);
  gtk_widget_show(frame);

  adjustment = GTK_ADJUSTMENT(gtk_adjustment_new (0, 0.0, 10.0, 0.01, 0.1, 0));
  vdt->graph = wc_graph_new(GTK_ADJUSTMENT(adjustment)); 
  wc_graph_set_update_policy (WC_GRAPH(vdt->graph), GTK_UPDATE_CONTINUOUS);
  gtk_container_add(GTK_CONTAINER(frame), vdt->graph);    
  gtk_widget_show(vdt->graph); 
  gtk_box_pack_start(GTK_BOX(vdt), displaybox,
	FALSE, FALSE, 10); 
}

