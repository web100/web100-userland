#include <stdlib.h>
#include <stdio.h> 
#include <gtk/gtk.h>
#include "web100object.h"
#include "web100widget.h"
#include "utillaunch.h"
#include "toolbox.h"


static void toolbox_class_init (ToolboxClass *klass);
static void toolbox_init (Toolbox *toolbox);
static void toolbox_destroy (GtkObject *object);
static void toolbox_snap_update (GtkObject *object, gpointer data);
static void toolbox_construct (Toolbox *toolbox, Web100Object *web100obj);

static void state_callback (GtkButton *button, gpointer data);
static void display_callback (GtkButton *button, gpointer data);
static void triage_callback (GtkButton *button, gpointer data);
static void list_callback (GtkButton *button, gpointer data);
static void rcvtuner_callback (GtkButton *button, gpointer data);
static void sndtuner_callback (GtkButton *button, gpointer data);

static Web100WidgetClass *parent_class = NULL;

#ifdef GTK2
GType
toolbox_get_type (void)
{
  static GType toolbox_type = 0;

  if (!toolbox_type)
  {
    static const GTypeInfo toolbox_info =
    {
      sizeof (ToolboxClass),
      NULL,
      NULL,
      (GClassInitFunc) toolbox_class_init,
      NULL,
      NULL,
      sizeof (Toolbox),
      0,
      (GInstanceInitFunc) toolbox_init,
    };

    toolbox_type = g_type_register_static (TYPE_WEB100_WIDGET, "Toolbox", &toolbox_info, 0);
  }

  return toolbox_type;
}
#else
GtkType
toolbox_get_type (void)
{
  static GtkType toolbox_type = 0;

  if(!toolbox_type)
    {
      GtkTypeInfo toolbox_info =
      {
	"Toolbox",
	sizeof (Toolbox),
	sizeof (ToolboxClass),
	(GtkClassInitFunc) toolbox_class_init,
	(GtkObjectInitFunc) toolbox_init,
        NULL,
        NULL
      };

      toolbox_type = gtk_type_unique (web100_widget_get_type (), &toolbox_info);
    }

  return toolbox_type;
}
#endif

static void
toolbox_class_init (ToolboxClass *class)
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

  object_class->destroy = toolbox_destroy;

  web100widget_class->snap_update = toolbox_snap_update;
}

static void
toolbox_init (Toolbox *toolbox)
{
  GTK_WIDGET_SET_FLAGS (toolbox, GTK_NO_WINDOW);
}

GtkWidget*
toolbox_new (Web100Object *web100obj)
{ 
  Toolbox *toolbox;
#ifdef GTK2
  toolbox = g_object_new (TYPE_TOOLBOX, NULL);
#else
  toolbox = gtk_type_new (toolbox_get_type ()); 
#endif

  web100_widget_set_object (WEB100_WIDGET (toolbox), web100obj);
 
  toolbox_construct (toolbox, web100obj);

  return GTK_WIDGET (toolbox); 
}

static void
toolbox_destroy (GtkObject *object)
{ 
  g_return_if_fail (object != NULL);
  g_return_if_fail (IS_TOOLBOX (object));

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
toolbox_snap_update (GtkObject *object, gpointer data)
{
}

static void state_callback (GtkButton *button, gpointer data)
{
}

static void display_callback (GtkButton *button, gpointer data)
{ 
  Web100Widget *ww = WEB100_WIDGET (data);

  util_launch ("display", ww->web100object, FALSE, ww->local, NULL); 
}

static void triage_callback (GtkButton *button, gpointer data)
{
  Web100Widget *ww = WEB100_WIDGET (data);
  
  util_launch ("triage", ww->web100object, FALSE, ww->local, NULL);
}

static void list_callback (GtkButton *button, gpointer data)
{
  Web100Widget *ww = WEB100_WIDGET (data);

  util_launch ("variablelist", ww->web100object, FALSE, ww->local, NULL);
}

static void sndtuner_callback (GtkButton *button, gpointer data)
{
  Web100Widget *ww = WEB100_WIDGET (data);

  util_launch ("sndtuner", ww->web100object, FALSE, ww->local, NULL);
}

static void rcvtuner_callback (GtkButton *button, gpointer data)
{
  Web100Widget *ww = WEB100_WIDGET (data);

  util_launch ("rcvtuner", ww->web100object, FALSE, ww->local, NULL); 
}

static void
toolbox_construct (Toolbox *toolbox, Web100Object *web100obj)
{
  GtkWidget *hbox, *table, *frame;
  GtkWidget *state_button, *display_button, *triage_button, *list_button, *sndtuner_button, *rcvtuner_button;
#ifdef GTK2
  GtkWidget *state_image, *display_image, *triage_image, *list_image, *sndtuner_image, *rcvtuner_image;
#else 
#endif 

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (toolbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  table = gtk_table_new (1, 5, FALSE); 
  gtk_box_pack_start(GTK_BOX(hbox), table, TRUE, FALSE, 10); 
  gtk_widget_show (table); 

  frame = gtk_frame_new("Display");
  gtk_frame_set_label_align(GTK_FRAME(frame), 0.1, 0.5); 
  gtk_table_attach_defaults (GTK_TABLE (table), frame, 1, 2, 0, 1); 
  gtk_widget_show(frame);

  display_button = gtk_button_new (); 
#ifdef GTK2
  gtk_widget_set_size_request (display_button, 45, 45);

  g_signal_connect (G_OBJECT (display_button), "clicked", G_CALLBACK (display_callback), toolbox);
#else
  gtk_widget_set_usize (display_button, 45, 10);

  gtk_signal_connect (GTK_OBJECT (display_button), "clicked", GTK_SIGNAL_FUNC (display_callback), toolbox);
#endif
  gtk_container_add (GTK_CONTAINER (frame), display_button); 
  gtk_widget_show (display_button);

#ifdef GTK2
  display_image = gtk_image_new_from_file (WEB100_IMAGE_DIR "/display_w2.gif");
  gtk_container_add (GTK_CONTAINER (display_button), display_image);
  gtk_widget_show (display_image);
#else
#endif

  frame = gtk_frame_new("Triage");
  gtk_frame_set_label_align(GTK_FRAME(frame), 0.1, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), frame, 2, 3, 0, 1); 
  gtk_widget_show(frame);

  triage_button = gtk_button_new ();
#ifdef GTK2
  gtk_widget_set_size_request (triage_button, 45, 45);

  g_signal_connect (G_OBJECT (triage_button), "clicked", G_CALLBACK (triage_callback), toolbox);
#else
  gtk_widget_set_usize (triage_button, 45, 10);

  gtk_signal_connect (GTK_OBJECT (triage_button), "clicked", GTK_SIGNAL_FUNC (triage_callback), toolbox);
#endif
  gtk_container_add (GTK_CONTAINER (frame), triage_button); 
  gtk_widget_show (triage_button);

#ifdef GTK2
  triage_image = gtk_image_new_from_file (WEB100_IMAGE_DIR "/triage_w2.gif");
  gtk_container_add (GTK_CONTAINER (triage_button), triage_image);
  gtk_widget_show (triage_image); 
#else
#endif

  frame = gtk_frame_new("List");
  gtk_frame_set_label_align(GTK_FRAME(frame), 0.1, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), frame, 3, 4, 0, 1); 
  gtk_widget_show(frame);

  list_button = gtk_button_new ();
#ifdef GTK2
  gtk_widget_set_size_request (list_button, 45, 45);

  g_signal_connect (G_OBJECT (list_button), "clicked", G_CALLBACK (list_callback), toolbox);
#else
  gtk_widget_set_usize (list_button, 45, 10);

  gtk_signal_connect (GTK_OBJECT (list_button), "clicked", GTK_SIGNAL_FUNC (list_callback), toolbox);
#endif
  gtk_container_add (GTK_CONTAINER (frame), list_button);
  gtk_widget_show (list_button);

#ifdef GTK2
  list_image = gtk_image_new_from_file (WEB100_IMAGE_DIR "/list_w2.gif");
  gtk_container_add (GTK_CONTAINER (list_button), list_image);
  gtk_widget_show (list_image);
#else
#endif

  frame = gtk_frame_new("STuner");
  gtk_frame_set_label_align(GTK_FRAME(frame), 0.1, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), frame, 0, 1, 0, 1); 
  gtk_widget_show(frame);

  sndtuner_button = gtk_button_new ();
#ifdef GTK2
  gtk_widget_set_size_request (sndtuner_button, 45, 45);

  g_signal_connect (G_OBJECT (sndtuner_button), "clicked", G_CALLBACK (sndtuner_callback), toolbox);
#else
  gtk_widget_set_usize (sndtuner_button, 45, 10);

  gtk_signal_connect (GTK_OBJECT (sndtuner_button), "clicked", GTK_SIGNAL_FUNC (sndtuner_callback), toolbox);
#endif
  gtk_container_add (GTK_CONTAINER (frame), sndtuner_button);
  gtk_widget_show (sndtuner_button);

#ifdef GTK2
  sndtuner_image = gtk_image_new_from_file (WEB100_IMAGE_DIR "/sndtuner_w2.gif");
  gtk_container_add (GTK_CONTAINER (sndtuner_button), sndtuner_image);
  gtk_widget_show (sndtuner_image);
#else
#endif

  frame = gtk_frame_new("RTuner");
  gtk_frame_set_label_align(GTK_FRAME(frame), 0.1, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), frame, 4, 5, 0, 1); 
  gtk_widget_show(frame);

  rcvtuner_button = gtk_button_new ();
#ifdef GTK2
  gtk_widget_set_size_request (rcvtuner_button, 45, 45);

  g_signal_connect (G_OBJECT (rcvtuner_button), "clicked", G_CALLBACK (rcvtuner_callback), toolbox);
#else
  gtk_widget_set_usize (rcvtuner_button, 45, 10);

  gtk_signal_connect (GTK_OBJECT (rcvtuner_button), "clicked", GTK_SIGNAL_FUNC (rcvtuner_callback), toolbox);
#endif
  gtk_container_add (GTK_CONTAINER (frame), rcvtuner_button);
  gtk_widget_show (rcvtuner_button);

#ifdef GTK2
  rcvtuner_image = gtk_image_new_from_file (WEB100_IMAGE_DIR "/rcvtuner_w2.gif");
  gtk_container_add (GTK_CONTAINER (rcvtuner_button), rcvtuner_image);
  gtk_widget_show (rcvtuner_image);
#else
#endif
}
