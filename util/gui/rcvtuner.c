#include <stdlib.h>
#include <stdio.h> 
#include <math.h>
#include <gtk/gtk.h>

#include "rcvtuner.h"


static void rcv_tuner_class_init (RcvTunerClass *klass);
static void rcv_tuner_init (RcvTuner *rcvtuner);
static void rcv_tuner_destroy (GtkObject *object);
static void rcv_tuner_construct (RcvTuner *rcvtuner, Web100Object *web100obj); 
static void rcv_tuner_set_object (Web100Widget *widget, Web100Object *web100obj);
static void rcv_tuner_snap_update (GtkObject *object, gpointer data); 
static void rcv_tuner_init_vars (RcvTuner *rcvtuner);

static void rcv_tuner_write_limrwin_value  (GtkWidget *button, gpointer data);
static void rcv_tuner_write_rcvbuf_value  (GtkWidget *button, gpointer data); 
static void rcv_tuner_limrwin_value_changed  (GtkAdjustment *adj, gpointer data);
static void rcv_tuner_rcvbuf_value_changed  (GtkAdjustment *adj, gpointer data);
static float rcv_tuner_val_to_scale (u_int32_t val, u_int32_t max, u_int32_t min);
static u_int32_t rcv_tuner_scale_to_val (float scale, u_int32_t max, u_int32_t min);

static Web100WidgetClass *parent_class = NULL;


#ifdef GTK2
GType
rcv_tuner_get_type (void)
{
  static GType rcv_tuner_type = 0;

  if (!rcv_tuner_type)
  {
    static const GTypeInfo rcv_tuner_info =
    {
      sizeof (RcvTunerClass),
      NULL,
      NULL,
      (GClassInitFunc) rcv_tuner_class_init,
      NULL,
      NULL,
      sizeof (RcvTuner),
      0,
      (GInstanceInitFunc) rcv_tuner_init,
    };

    rcv_tuner_type = g_type_register_static (TYPE_WEB100_WIDGET, "RcvTuner", &rcv_tuner_info, 0);
  }

  return rcv_tuner_type;
}
#else
GtkType
rcv_tuner_get_type (void)
{
  static GtkType rcv_tuner_type = 0;

  if(!rcv_tuner_type)
    {
      GtkTypeInfo rcv_tuner_info =
      {
	"RcvTuner",
	sizeof (RcvTuner),
	sizeof (RcvTunerClass),
	(GtkClassInitFunc) rcv_tuner_class_init,
	(GtkObjectInitFunc) rcv_tuner_init,
        NULL,
        NULL
      };

      rcv_tuner_type = gtk_type_unique (web100_widget_get_type (), &rcv_tuner_info);
    }

  return rcv_tuner_type;
}
#endif

static void
rcv_tuner_class_init (RcvTunerClass *class)
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

  object_class->destroy = rcv_tuner_destroy;

  web100widget_class->set_object = rcv_tuner_set_object;
  web100widget_class->snap_update = rcv_tuner_snap_update;
}

static void
rcv_tuner_init (RcvTuner *rcvtuner)
{ 
  GTK_WIDGET_SET_FLAGS (rcvtuner, GTK_NO_WINDOW);
}

GtkWidget*
rcv_tuner_new (Web100Object *web100obj)
{ 
  RcvTuner *rcvtuner;
#ifdef GTK2
  rcvtuner = g_object_new (TYPE_RCV_TUNER, NULL);
#else
  rcvtuner = gtk_type_new (rcv_tuner_get_type ()); 
#endif
 
  web100_widget_set_object (WEB100_WIDGET (rcvtuner), web100obj);

  rcv_tuner_construct (rcvtuner, web100obj);

  rcv_tuner_init_vars (rcvtuner);

  return GTK_WIDGET (rcvtuner); 
}

static void
rcv_tuner_destroy (GtkObject *object)
{ 
  g_return_if_fail (object != NULL);
  g_return_if_fail (IS_RCV_TUNER (object));

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
rcv_tuner_set_object (Web100Widget *widget, Web100Object *web100obj)
{ 
  RcvTuner *rcvtuner;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (IS_WEB100_WIDGET (widget)); 
  g_return_if_fail (web100obj != NULL);
  g_return_if_fail (IS_WEB100_OBJECT (web100obj)); 

  rcvtuner = RCV_TUNER (widget);

  web100_widget_set_object (widget, web100obj);

  rcv_tuner_init_vars (rcvtuner);
}

static void
rcv_tuner_snap_update (GtkObject *object, gpointer data)
{
}

static void
rcv_tuner_init_vars (RcvTuner *rcvtuner)
{
  web100_group       *tunegp;
  web100_connection  *conn;
  Web100Object *obj = WEB100_WIDGET (rcvtuner)->web100object;
  gint mode;
  FILE *fp;
  u_int32_t val;

  if ((tunegp = web100_group_find (WEB100_WIDGET (rcvtuner)->web100object->agent, "tune")) != NULL) { 
    rcvtuner->Rcvbuf = web100_var_find (tunegp, "X_Rcvbuf"); 
    rcvtuner->LimRwin = web100_var_find (tunegp, "LimRwin"); 
  }
  
  if ((conn = web100_connection_lookup (obj->agent, obj->cid)) != NULL) { 
    if (rcvtuner->Rcvbuf) {
      web100_raw_read (rcvtuner->Rcvbuf, conn, &rcvtuner->rcvbuf_val);

// Get upper bound on rcvbuf from system-wide settings
// Set lower bound to something reasonable

      rcvtuner->rcvbuf_min = 8192;

      if (((fp = fopen ("/proc/sys/net/core/rmem_max", "r")) != NULL) &&
	  (fscanf (fp, "%u", &val) != 0))
       	rcvtuner->rcvbuf_max = val;
      else
       	rcvtuner->rcvbuf_max = rcvtuner->rcvbuf_min;

      if (fclose(fp))
       	perror("/proc/sys/net/core/wmem_max");

      rcvtuner->rcvbuf_scale_val = rcv_tuner_val_to_scale (rcvtuner->rcvbuf_val, rcvtuner->rcvbuf_max, rcvtuner->rcvbuf_min); 

      gtk_adjustment_set_value (rcvtuner->rcvbuf_adjustment, rcvtuner->rcvbuf_scale_val);
    }
    if (rcvtuner->LimRwin) {
      web100_raw_read (rcvtuner->LimRwin, conn, &rcvtuner->limrwin_val);

      rcvtuner->limrwin_max = (u_int32_t) -1;

// Set lower bound on limrwin to something reasonable
      rcvtuner->limrwin_min = 8192;

      rcvtuner->limrwin_scale_val = rcv_tuner_val_to_scale (rcvtuner->limrwin_val, rcvtuner->limrwin_max, rcvtuner->limrwin_min);

      gtk_adjustment_set_value (rcvtuner->limrwin_adjustment, rcvtuner->limrwin_scale_val); 
    }
  }
} 

static void
rcv_tuner_write_limrwin_value (GtkWidget *button, gpointer data)
{ 
  RcvTuner *rcvtuner = RCV_TUNER (data);
  web100_connection *conn;
  Web100Object *obj = WEB100_WIDGET (rcvtuner)->web100object;

  if ((conn = web100_connection_lookup(obj->agent, obj->cid)) != NULL) { 
    if (rcvtuner->LimRwin)
      web100_raw_write (rcvtuner->LimRwin, conn, &rcvtuner->limrwin_val); 
  }
}

static void
rcv_tuner_write_rcvbuf_value  (GtkWidget *button, gpointer data)
{
  RcvTuner *rcvtuner = RCV_TUNER (data);
  web100_connection *conn;
  Web100Object *obj = WEB100_WIDGET (rcvtuner)->web100object;

  if ((conn = web100_connection_lookup(obj->agent, obj->cid)) != NULL) { 
    if (rcvtuner->Rcvbuf)
      web100_raw_write (rcvtuner->Rcvbuf, conn, &rcvtuner->rcvbuf_val); 
  }
}

static void
rcv_tuner_limrwin_value_changed (GtkAdjustment *adj, gpointer data)
{
  RcvTuner *rcvtuner = RCV_TUNER (data);
  char strval[64]; 

  rcvtuner->limrwin_scale_val = adj->value;

  rcvtuner->limrwin_val = rcv_tuner_scale_to_val (rcvtuner->limrwin_scale_val, rcvtuner->limrwin_max, rcvtuner->limrwin_min);

  sprintf(strval, "%u", (rcvtuner->limrwin_val));
  gtk_label_set_text (GTK_LABEL (rcvtuner->limrwin_label), strval);
}

static void
rcv_tuner_rcvbuf_value_changed  (GtkAdjustment *adj, gpointer data)
{
  RcvTuner *rcvtuner = RCV_TUNER (data); 
  char strval[64];

  rcvtuner->rcvbuf_scale_val = adj->value;

  rcvtuner->rcvbuf_val = rcv_tuner_scale_to_val (rcvtuner->rcvbuf_scale_val, rcvtuner->rcvbuf_max, rcvtuner->rcvbuf_min);

  sprintf(strval, "%u", (rcvtuner->rcvbuf_val));
  gtk_label_set_text (GTK_LABEL (rcvtuner->rcvbuf_label), strval);
}

static float
rcv_tuner_val_to_scale (u_int32_t val, u_int32_t max, u_int32_t min)
{
  float bb, aa;

  if (!min) min = 1;
  if (max <= min) {
    max = min;
    return max;
  }

  aa = log(min); bb = log(max) - aa;

  return (log(val) - log(min))/bb;
}

static u_int32_t
rcv_tuner_scale_to_val (float scale, u_int32_t max, u_int32_t min)
{
  float bb, aa, res;
  u_int32_t val;

  if (!min) min = 1;
  if (max < min) max = min;

  res = exp((scale)*(log(max)-log(min)) + log(min));

  val = (res < (u_int32_t) -1) ? (u_int32_t) floor(res) : (u_int32_t) -1;

  return val;
}

static void
rcv_tuner_construct (RcvTuner *rcvtuner, Web100Object *web100obj)
{ 
  GtkWidget *RB_label, *RB_scale, *RB_write_button;
  GtkWidget *LR_label, *LR_scale, *LR_write_button;
  GtkWidget *hbox;
  gint mode;

  rcvtuner->rcvbuf_adjustment = GTK_ADJUSTMENT (gtk_adjustment_new(0.5, 0.0, 1.0, 0.01, 0.1, 0.0)); 

#ifdef GTK2
  g_signal_connect (G_OBJECT (rcvtuner->rcvbuf_adjustment), "value_changed", G_CALLBACK (rcv_tuner_rcvbuf_value_changed), rcvtuner);
#else
  gtk_signal_connect (GTK_OBJECT (rcvtuner->rcvbuf_adjustment), "value_changed", GTK_SIGNAL_FUNC (rcv_tuner_rcvbuf_value_changed), rcvtuner);
#endif

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (rcvtuner), hbox, FALSE, FALSE, 5);
  gtk_widget_show (hbox);

  RB_label = gtk_label_new ("Rcvbuf");
  gtk_box_pack_start (GTK_BOX (hbox), RB_label, FALSE, FALSE, 0);
  gtk_widget_show (RB_label);

  RB_scale = gtk_hscale_new (rcvtuner->rcvbuf_adjustment);
  gtk_scale_set_draw_value (GTK_SCALE (RB_scale), FALSE); 
  gtk_scale_set_digits (GTK_SCALE (RB_scale), 10); 
  gtk_box_pack_start (GTK_BOX (hbox), RB_scale, TRUE, TRUE, 0);
  gtk_widget_show (RB_scale);

  RB_write_button = gtk_button_new_with_label ("Write");
#ifdef GTK2
  g_signal_connect (G_OBJECT (RB_write_button), "clicked", G_CALLBACK (rcv_tuner_write_rcvbuf_value), rcvtuner);
#else 
  gtk_signal_connect (GTK_OBJECT (RB_write_button), "clicked", GTK_SIGNAL_FUNC (rcv_tuner_write_rcvbuf_value), rcvtuner);
#endif
  gtk_box_pack_start (GTK_BOX (hbox), RB_write_button, FALSE, FALSE, 0);
  gtk_widget_show (RB_write_button);

  rcvtuner->rcvbuf_label = GTK_LABEL (gtk_label_new (NULL));
  gtk_box_pack_start (GTK_BOX (rcvtuner), GTK_WIDGET (rcvtuner->rcvbuf_label), FALSE, FALSE, 5); 
  gtk_widget_show (GTK_WIDGET (rcvtuner->rcvbuf_label));

  rcvtuner->limrwin_adjustment = GTK_ADJUSTMENT (gtk_adjustment_new(0.5, 0.0, 1.0, 0.01, 0.1, 0.0)); 

#ifdef GTK2
  g_signal_connect (G_OBJECT (rcvtuner->limrwin_adjustment), "value_changed", G_CALLBACK (rcv_tuner_limrwin_value_changed), rcvtuner);
#else
  gtk_signal_connect (GTK_OBJECT (rcvtuner->limrwin_adjustment), "value_changed", GTK_SIGNAL_FUNC (rcv_tuner_limrwin_value_changed), rcvtuner);
#endif

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (rcvtuner), hbox, FALSE, FALSE, 5);
  gtk_widget_show (hbox);

  LR_label = gtk_label_new ("LimRwin");
  gtk_box_pack_start (GTK_BOX (hbox), LR_label, FALSE, FALSE, 0);
  gtk_widget_show (LR_label);

  LR_scale = gtk_hscale_new (rcvtuner->limrwin_adjustment);
  gtk_scale_set_draw_value (GTK_SCALE (LR_scale), FALSE); 
  gtk_scale_set_digits (GTK_SCALE (LR_scale), 10); 
  gtk_box_pack_start (GTK_BOX (hbox), LR_scale, TRUE, TRUE, 0);
  gtk_widget_show (LR_scale);

  LR_write_button = gtk_button_new_with_label ("Write");
#ifdef GTK2
  g_signal_connect (G_OBJECT (LR_write_button), "clicked", G_CALLBACK (rcv_tuner_write_limrwin_value), rcvtuner);
#else
  gtk_signal_connect (GTK_OBJECT (LR_write_button), "clicked", GTK_SIGNAL_FUNC (rcv_tuner_write_limrwin_value), rcvtuner);
#endif
  gtk_box_pack_start (GTK_BOX (hbox), LR_write_button, FALSE, FALSE, 0);
  gtk_widget_show (LR_write_button);

  rcvtuner->limrwin_label = GTK_LABEL (gtk_label_new (NULL));
  gtk_box_pack_start (GTK_BOX (rcvtuner), GTK_WIDGET (rcvtuner->limrwin_label), FALSE, FALSE, 5); 
  gtk_widget_show (GTK_WIDGET (rcvtuner->limrwin_label));

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (rcvtuner), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);
}
