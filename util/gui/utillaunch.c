#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <gtk/gtk.h>
#include "config.h"
#include "utillaunch.h"
#include "connectionselect.h"
#include "display.h"
#include "rcvtuner.h"
#include "sndtuner.h"
#include "toolbox.h"
#include "triage.h"
#include "variablelist.h"
#include "web100object.h"


#define UTIL_NAME_LEN_MAX 32 

static void util_close (gpointer data, guint action, GtkWidget *widget)
{
  GtkWidget *window;

  window = gtk_widget_get_toplevel (GTK_WIDGET (data));

  if (window) gtk_widget_destroy (window);
}

static void update_show (gpointer data, guint action, GtkWidget *widget)
{
  gint *toggle;
 
  if (data != NULL && IS_WEB100_WIDGET (data)) {
    toggle = &WEB100_WIDGET (data)->update_toggle;

    *toggle = 1 - *toggle;
    if (*toggle) gtk_widget_show (WEB100_WIDGET (data)->update_widget);
    else gtk_widget_hide (WEB100_WIDGET (data)->update_widget);
  }
}

static void get_menu (GtkWidget *window, Web100Widget *w100widget, int master, GtkWidget **menubar)
{ 
  static GtkItemFactoryEntry master_menu_items[] = {
    { "/_File",         NULL,         NULL, 0, "<Branch>" }, 
    { "/File/Quit",     "<control>Q", gtk_main_quit, 0, NULL },
    { "/_View",         NULL,         NULL, 0, "<Branch>" },
    { "/View/Update interval", "<control>U", update_show, 0, "<ToggleItem>" }, 
  };

  static GtkItemFactoryEntry slave_menu_items[] = {
    { "/_File",         NULL,         NULL, 0, "<Branch>" }, 
    { "/File/Close",     "<control>W", util_close, 0, NULL }, 
    { "/_View",         NULL,         NULL, 0, "<Branch>" },
    { "/View/Update interval", "<control>U", update_show, 0, "<ToggleItem>" }, 
  };

  GtkItemFactory *item_factory;
  GtkAccelGroup *accel_group;

#define MENU_SIZE(x) ((gint) sizeof (x) / sizeof (x[0]))

  accel_group = gtk_accel_group_new ();

  item_factory = gtk_item_factory_new (GTK_TYPE_MENU_BAR, "<main>", accel_group);

  if (master) 
    gtk_item_factory_create_items (item_factory, MENU_SIZE(master_menu_items), master_menu_items, w100widget);
  else
    gtk_item_factory_create_items (item_factory, MENU_SIZE(slave_menu_items), slave_menu_items, w100widget);

  gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);

  if (menubar) 
    *menubar = gtk_item_factory_get_widget (item_factory, "<main>");
}

int
util_launch (char *name, Web100Object *web100obj, gboolean master, gboolean local, gpointer data)
{
  Web100Widget *util = NULL;
  ConnectionSelect *select; 
  GtkAdjustment *adjust;

  GtkWidget *window;
  GtkWidget *vbox;
  GtkWidget *frame;
  GtkWidget *menubar;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *hscale;

  static char hostname[64];
  size_t len;
  char titlebar[128];
  static gint xpos = 0, ypos = 0;
  GdkRectangle bbox;

  if (!strncmp ("display", name, UTIL_NAME_LEN_MAX)) { 
    util = WEB100_WIDGET (display_new (web100obj, (char *) data));
  }
  if (!strncmp ("rcvtuner", name, UTIL_NAME_LEN_MAX)) {
    util = WEB100_WIDGET (rcv_tuner_new (web100obj));
  }
  if (!strncmp ("sndtuner", name, UTIL_NAME_LEN_MAX)) {
    util = WEB100_WIDGET (snd_tuner_new (web100obj));
  }
  if (!strncmp ("toolbox", name, UTIL_NAME_LEN_MAX)) {
    util = WEB100_WIDGET (toolbox_new (web100obj));
  }
  if (!strncmp ("triage", name, UTIL_NAME_LEN_MAX)) {
    util = WEB100_WIDGET (triage_new (web100obj));
  }
  if (!strncmp ("variablelist", name, UTIL_NAME_LEN_MAX)) {
    util = WEB100_WIDGET (variable_list_new (web100obj));
  }

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gethostname (hostname, len);
  strncpy (titlebar, name, UTIL_NAME_LEN_MAX);
  strcat (titlebar, "@");
  strncat (titlebar, hostname, 64);
  gtk_window_set_title (GTK_WINDOW (window), titlebar);

  if (master) {
    gtk_signal_connect (GTK_OBJECT (window), "destroy", GTK_SIGNAL_FUNC (gtk_main_quit), NULL);
  }
  else {
    gtk_signal_connect (GTK_OBJECT (window), "delete_event", GTK_SIGNAL_FUNC (gtk_widget_destroy), NULL);
  }

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (window), vbox);
  gtk_widget_show (vbox);

  frame = gtk_frame_new (NULL); 
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  get_menu (window, util, master, &menubar);

  gtk_container_set_border_width (GTK_CONTAINER (menubar), 0);
#ifndef GTK2
  gtk_menu_bar_set_shadow_type (GTK_MENU_BAR (menubar), GTK_SHADOW_NONE);
#endif
  gtk_container_add (GTK_CONTAINER (frame), menubar);
  gtk_widget_show (menubar);

  frame = gtk_frame_new (NULL);
  gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  select = CONNECTION_SELECT (connection_select_new (web100obj, master));

  gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (select), FALSE, FALSE, 10);
  gtk_widget_show (GTK_WIDGET (select));
  
  if (util) {
    gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (util), TRUE, TRUE, 10);
    gtk_widget_show (GTK_WIDGET (util));

    web100_widget_share_object (util, WEB100_WIDGET (select));

    hbox = gtk_hbox_new (FALSE, 0);

    label = gtk_label_new ("Update interval (sec)");
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 5);
    gtk_widget_show (label);

    adjust = util->web100object->web100poll->adjustment; 
    hscale = gtk_hscale_new (adjust); 
    gtk_box_pack_end (GTK_BOX (hbox), hscale, TRUE, TRUE, 5);
    gtk_widget_show (hscale);

    util->update_widget = hbox;
    gtk_box_pack_end (GTK_BOX (vbox), util->update_widget, FALSE, FALSE, 0); 
  }
  else {
    gtk_widget_show (window);
    return 0;
  }

  gtk_widget_realize (window);

#ifdef GTK2
  if (local) {
    util->local = TRUE;

    gdk_window_get_frame_extents (window->window, &bbox);

    if (xpos + bbox.width > gdk_screen_width()) {
      xpos = 0;
      ypos += bbox.height;
    }
    gtk_window_move (GTK_WINDOW (window), xpos, ypos);

    xpos += bbox.width;
  }
  else { 
    util->local = FALSE;

    gdk_window_get_frame_extents (window->window, &bbox);

    if (xpos + bbox.width > gdk_screen_width()) {
      xpos = 0;
      ypos += bbox.height;
    }
    gtk_window_set_gravity (GTK_WINDOW (window), GDK_GRAVITY_SOUTH_EAST);
    gtk_window_move (GTK_WINDOW (window), gdk_screen_width() - bbox.width - xpos, gdk_screen_height() - bbox.height - ypos);

    xpos += bbox.width;
  }
#else 
  if (local) util->local = TRUE;
  else util->local = FALSE;
#endif
  gtk_widget_show (window);

  return 0;
}
