#include <stdlib.h>
#include <stdio.h> 
#include <math.h>
#include <gtk/gtk.h>

#include "sndtuner.h"


static void snd_tuner_class_init (SndTunerClass *klass);
static void snd_tuner_init (SndTuner *sndtuner);
static void snd_tuner_destroy (GtkObject *object);
static void snd_tuner_construct (SndTuner *sndtuner, Web100Object *web100obj); 
static void snd_tuner_set_object (Web100Widget *widget, Web100Object *web100obj);
static void snd_tuner_snap_update (GtkObject *object, gpointer data); 
static void snd_tuner_init_vars (SndTuner *sndtuner);

static void snd_tuner_write_limcwnd_value  (GtkWidget *button, gpointer data);
static void snd_tuner_write_sndbuf_value  (GtkWidget *button, gpointer data); 
static void snd_tuner_limcwnd_value_changed  (GtkAdjustment *adj, gpointer data);
static void snd_tuner_sndbuf_value_changed  (GtkAdjustment *adj, gpointer data);
static float snd_tuner_val_to_scale (u_int32_t val, u_int32_t max, u_int32_t min);
static u_int32_t snd_tuner_scale_to_val (float scale, u_int32_t max, u_int32_t min);

static Web100WidgetClass *parent_class = NULL;


#ifdef GTK2
GType
snd_tuner_get_type (void)
{
  static GType snd_tuner_type = 0;

  if (!snd_tuner_type)
  {
    static const GTypeInfo snd_tuner_info =
    {
      sizeof (SndTunerClass),
      NULL,
      NULL,
      (GClassInitFunc) snd_tuner_class_init,
      NULL,
      NULL,
      sizeof (SndTuner),
      0,
      (GInstanceInitFunc) snd_tuner_init,
    };

    snd_tuner_type = g_type_register_static (TYPE_WEB100_WIDGET, "SndTuner", &snd_tuner_info, 0);
  }

  return snd_tuner_type;
}
#else
GtkType
snd_tuner_get_type (void)
{
  static GtkType snd_tuner_type = 0;

  if(!snd_tuner_type)
    {
      GtkTypeInfo snd_tuner_info =
      {
	"SndTuner",
	sizeof (SndTuner),
	sizeof (SndTunerClass),
	(GtkClassInitFunc) snd_tuner_class_init,
	(GtkObjectInitFunc) snd_tuner_init,
        NULL,
        NULL
      };

      snd_tuner_type = gtk_type_unique (web100_widget_get_type (), &snd_tuner_info);
    }

  return snd_tuner_type;
}
#endif

static void
snd_tuner_class_init (SndTunerClass *class)
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

  object_class->destroy = snd_tuner_destroy;

  web100widget_class->set_object = snd_tuner_set_object;
  web100widget_class->snap_update = snd_tuner_snap_update;
}

static void
snd_tuner_init (SndTuner *sndtuner)
{ 
  GTK_WIDGET_SET_FLAGS (sndtuner, GTK_NO_WINDOW);
}

GtkWidget*
snd_tuner_new (Web100Object *web100obj)
{ 
  SndTuner *sndtuner;
#ifdef GTK2
  sndtuner = g_object_new (TYPE_SND_TUNER, NULL);
#else
  sndtuner = gtk_type_new (snd_tuner_get_type ()); 
#endif
 
  web100_widget_set_object (WEB100_WIDGET (sndtuner), web100obj);

  snd_tuner_construct (sndtuner, web100obj);

  snd_tuner_init_vars (sndtuner);

  return GTK_WIDGET (sndtuner); 
}

static void
snd_tuner_destroy (GtkObject *object)
{ 
  g_return_if_fail (object != NULL);
  g_return_if_fail (IS_SND_TUNER (object));

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
snd_tuner_set_object (Web100Widget *widget, Web100Object *web100obj)
{ 
  SndTuner *sndtuner;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (IS_WEB100_WIDGET (widget)); 
  g_return_if_fail (web100obj != NULL);
  g_return_if_fail (IS_WEB100_OBJECT (web100obj)); 

  sndtuner = SND_TUNER (widget);

  web100_widget_set_object (widget, web100obj);

  snd_tuner_init_vars (sndtuner);
}

static void
snd_tuner_snap_update (GtkObject *object, gpointer data)
{
}

static void
snd_tuner_init_vars (SndTuner *sndtuner)
{
  web100_group       *tunegp;
  web100_connection  *conn;
  Web100Object *obj = WEB100_WIDGET (sndtuner)->web100object;
  gint mode;
  FILE *fp;
  u_int32_t val;

  if ((tunegp = web100_group_find (WEB100_WIDGET (sndtuner)->web100object->agent, "tune")) != NULL) { 
    sndtuner->Sndbuf = web100_var_find (tunegp, "X_Sndbuf"); 
    sndtuner->LimCwnd = web100_var_find (tunegp, "LimCwnd"); 
  }
  
  if ((conn = web100_connection_lookup (obj->agent, obj->cid)) != NULL) { 
    if (sndtuner->Sndbuf) {
      web100_raw_read (sndtuner->Sndbuf, conn, &sndtuner->sndbuf_val);

// Get upper bound on sndbuf from system-wide settings
// Set lower bound to something reasonable

      sndtuner->sndbuf_min = 8192;

      if (((fp = fopen ("/proc/sys/net/core/wmem_max", "r")) != NULL) &&
	  (fscanf (fp, "%u", &val) != 0))
       	sndtuner->sndbuf_max = val;
      else
       	sndtuner->sndbuf_max = sndtuner->sndbuf_min;

      if (fclose(fp))
       	perror("/proc/sys/net/core/wmem_max");

      sndtuner->sndbuf_scale_val = snd_tuner_val_to_scale (sndtuner->sndbuf_val, sndtuner->sndbuf_max, sndtuner->sndbuf_min); 

      gtk_adjustment_set_value (sndtuner->sndbuf_adjustment, sndtuner->sndbuf_scale_val);
    }
    if (sndtuner->LimCwnd) {
      web100_raw_read (sndtuner->LimCwnd, conn, &sndtuner->limcwnd_val);
//XXX 
      sndtuner->limcwnd_max = 2*sndtuner->limcwnd_val;

// Set lower bound on limcwnd to something reasonable
      sndtuner->limcwnd_min = 8192;

      sndtuner->limcwnd_scale_val = snd_tuner_val_to_scale (sndtuner->limcwnd_val, sndtuner->limcwnd_max, sndtuner->limcwnd_min);

      gtk_adjustment_set_value (sndtuner->limcwnd_adjustment, sndtuner->limcwnd_scale_val); 
    }
  }
} 

static void
snd_tuner_write_limcwnd_value (GtkWidget *button, gpointer data)
{ 
  SndTuner *sndtuner = SND_TUNER (data);
  web100_connection *conn;
  Web100Object *obj = WEB100_WIDGET (sndtuner)->web100object;

  if ((conn = web100_connection_lookup(obj->agent, obj->cid)) != NULL) { 
    if (sndtuner->LimCwnd)
      web100_raw_write (sndtuner->LimCwnd, conn, &sndtuner->limcwnd_val); 
  }
}

static void
snd_tuner_write_sndbuf_value  (GtkWidget *button, gpointer data)
{
  SndTuner *sndtuner = SND_TUNER (data);
  web100_connection *conn;
  Web100Object *obj = WEB100_WIDGET (sndtuner)->web100object;

  if ((conn = web100_connection_lookup(obj->agent, obj->cid)) != NULL) { 
    if (sndtuner->Sndbuf)
      web100_raw_write (sndtuner->Sndbuf, conn, &sndtuner->sndbuf_val); 
  }
}

static void
snd_tuner_limcwnd_value_changed (GtkAdjustment *adj, gpointer data)
{
  SndTuner *sndtuner = SND_TUNER (data);
  char strval[64]; 

  sndtuner->limcwnd_scale_val = adj->value;

  sndtuner->limcwnd_val = snd_tuner_scale_to_val (sndtuner->limcwnd_scale_val, sndtuner->limcwnd_max, sndtuner->limcwnd_min);

  sprintf(strval, "%u", (sndtuner->limcwnd_val));
  gtk_label_set_text (GTK_LABEL (sndtuner->limcwnd_label), strval);
}

static void
snd_tuner_sndbuf_value_changed  (GtkAdjustment *adj, gpointer data)
{
  SndTuner *sndtuner = SND_TUNER (data); 
  char strval[64];

  sndtuner->sndbuf_scale_val = adj->value;

  sndtuner->sndbuf_val = snd_tuner_scale_to_val (sndtuner->sndbuf_scale_val, sndtuner->sndbuf_max, sndtuner->sndbuf_min);

  sprintf(strval, "%u", (sndtuner->sndbuf_val));
  gtk_label_set_text (GTK_LABEL (sndtuner->sndbuf_label), strval);
}

static float
snd_tuner_val_to_scale (u_int32_t val, u_int32_t max, u_int32_t min)
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
snd_tuner_scale_to_val (float scale, u_int32_t max, u_int32_t min)
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
snd_tuner_construct (SndTuner *sndtuner, Web100Object *web100obj)
{ 
  GtkWidget *SB_label, *SB_scale, *SB_write_button;
  GtkWidget *LC_label, *LC_scale, *LC_write_button;
  GtkWidget *hbox;
  gint mode;

  sndtuner->sndbuf_adjustment = GTK_ADJUSTMENT (gtk_adjustment_new(0.5, 0.0, 1.0, 0.01, 0.1, 0.0)); 

#ifdef GTK2
  g_signal_connect (G_OBJECT (sndtuner->sndbuf_adjustment), "value_changed", G_CALLBACK (snd_tuner_sndbuf_value_changed), sndtuner);
#else
  gtk_signal_connect (GTK_OBJECT (sndtuner->sndbuf_adjustment), "value_changed", GTK_SIGNAL_FUNC (snd_tuner_sndbuf_value_changed), sndtuner);
#endif

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (sndtuner), hbox, FALSE, FALSE, 5);
  gtk_widget_show (hbox);

  SB_label = gtk_label_new ("Sndbuf");
  gtk_box_pack_start (GTK_BOX (hbox), SB_label, FALSE, FALSE, 0);
  gtk_widget_show (SB_label);

  SB_scale = gtk_hscale_new (sndtuner->sndbuf_adjustment);
  gtk_scale_set_draw_value (GTK_SCALE (SB_scale), FALSE); 
  gtk_scale_set_digits (GTK_SCALE (SB_scale), 10); 
  gtk_box_pack_start (GTK_BOX (hbox), SB_scale, TRUE, TRUE, 0);
  gtk_widget_show (SB_scale);

  SB_write_button = gtk_button_new_with_label ("Write");
#ifdef GTK2
  g_signal_connect (G_OBJECT (SB_write_button), "clicked", G_CALLBACK (snd_tuner_write_sndbuf_value), sndtuner);
#else 
  gtk_signal_connect (GTK_OBJECT (SB_write_button), "clicked", GTK_SIGNAL_FUNC (snd_tuner_write_sndbuf_value), sndtuner);
#endif
  gtk_box_pack_start (GTK_BOX (hbox), SB_write_button, FALSE, FALSE, 0);
  gtk_widget_show (SB_write_button);

  sndtuner->sndbuf_label = GTK_LABEL (gtk_label_new (NULL));
  gtk_box_pack_start (GTK_BOX (sndtuner), GTK_WIDGET (sndtuner->sndbuf_label), FALSE, FALSE, 5); 
  gtk_widget_show (GTK_WIDGET (sndtuner->sndbuf_label));

  sndtuner->limcwnd_adjustment = GTK_ADJUSTMENT (gtk_adjustment_new(0.5, 0.0, 1.0, 0.01, 0.1, 0.0)); 

#ifdef GTK2
  g_signal_connect (G_OBJECT (sndtuner->limcwnd_adjustment), "value_changed", G_CALLBACK (snd_tuner_limcwnd_value_changed), sndtuner);
#else
  gtk_signal_connect (GTK_OBJECT (sndtuner->limcwnd_adjustment), "value_changed", GTK_SIGNAL_FUNC (snd_tuner_limcwnd_value_changed), sndtuner);
#endif

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (sndtuner), hbox, FALSE, FALSE, 5);
  gtk_widget_show (hbox);

  LC_label = gtk_label_new ("LimCwnd");
  gtk_box_pack_start (GTK_BOX (hbox), LC_label, FALSE, FALSE, 0);
  gtk_widget_show (LC_label);

  LC_scale = gtk_hscale_new (sndtuner->limcwnd_adjustment);
  gtk_scale_set_draw_value (GTK_SCALE (LC_scale), FALSE); 
  gtk_scale_set_digits (GTK_SCALE (LC_scale), 10); 
  gtk_box_pack_start (GTK_BOX (hbox), LC_scale, TRUE, TRUE, 0);
  gtk_widget_show (LC_scale);

  LC_write_button = gtk_button_new_with_label ("Write");
#ifdef GTK2
  g_signal_connect (G_OBJECT (LC_write_button), "clicked", G_CALLBACK (snd_tuner_write_limcwnd_value), sndtuner);
#else
  gtk_signal_connect (GTK_OBJECT (LC_write_button), "clicked", GTK_SIGNAL_FUNC (snd_tuner_write_limcwnd_value), sndtuner);
#endif
  gtk_box_pack_start (GTK_BOX (hbox), LC_write_button, FALSE, FALSE, 0);
  gtk_widget_show (LC_write_button);

  sndtuner->limcwnd_label = GTK_LABEL (gtk_label_new (NULL));
  gtk_box_pack_start (GTK_BOX (sndtuner), GTK_WIDGET (sndtuner->limcwnd_label), FALSE, FALSE, 5); 
  gtk_widget_show (GTK_WIDGET (sndtuner->limcwnd_label));

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (sndtuner), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);
}
