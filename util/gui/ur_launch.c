#include <stdio.h>
#include <gtk/gtk.h>
#include "web100.h"
#include "web100sync.h"
#include "web100obj.h"
#include "sockset_wgt.h"
#include "avd_list_wgt.h"
#include "avd_table_wgt.h"
#include "cpr_wgt.h"
#include "dtb_wgt.h" 
#include "rtuner_wgt.h"
#include "stuner_wgt.h"
#include "triage_wgt.h"


void ur_launch_util (char *name, Web100Obj *web100obj,
                     gboolean malleable, gboolean master)
{
static  GtkWidget *window, *vbox, *ur_sockset, *avd_list, *avd, *cpr, *dtb, *rtuner, *stuner, *triage;

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_policy (GTK_WINDOW(window), TRUE, TRUE, FALSE); 
  gtk_window_set_title (GTK_WINDOW (window), name);

  if (master)
    gtk_signal_connect (GTK_OBJECT (window), "destroy",
                        GTK_SIGNAL_FUNC (gtk_main_quit), NULL);
  else // if slave
    gtk_signal_connect (GTK_OBJECT (window), "destroy",
                        GTK_SIGNAL_FUNC (gtk_widget_destroy), NULL); 
  

  gtk_container_set_border_width (GTK_CONTAINER (window), 10);

  vbox = gtk_vbox_new (FALSE, 0); 
  gtk_container_add (GTK_CONTAINER (window), vbox);
  gtk_widget_show (vbox);

  ur_sockset = sockset_new (web100obj, malleable);
  gtk_box_pack_start (GTK_BOX(vbox), ur_sockset, FALSE, FALSE, 0);
  gtk_widget_show (ur_sockset);

  if (!strncmp ("avd_list", name, 8)) {
    avd_list = avd_list_new (web100obj);
    gtk_box_pack_start (GTK_BOX(vbox), avd_list, TRUE, TRUE, 0);
    gtk_widget_show (avd_list);
// TODO: track widgets for closing all windows for a given connection
//    if (!master) g_list_prepend (web100obj->widgets, avd_list);

    if (malleable) { 
      gtk_signal_connect (GTK_OBJECT (ur_sockset),
                          "web100obj_changed",
                          GTK_SIGNAL_FUNC (avd_list_sockset_listen),
                          (gpointer) avd_list); 
    }

  }

  if (!strncmp ("avd_table", name, 9)) {
    avd = avd_table_new (web100obj);
    gtk_box_pack_start (GTK_BOX(vbox), avd, TRUE, TRUE, 0);
    gtk_widget_show (avd);
  
    if (malleable)
    gtk_signal_connect (GTK_OBJECT (ur_sockset),
                        "web100obj_changed",
                        GTK_SIGNAL_FUNC (avd_table_sockset_listen),
                        avd); 
  }

  if (!strncmp ("cpr", name, 3)) {
    cpr = cpr_new (web100obj);
    gtk_box_pack_start (GTK_BOX(vbox), cpr, TRUE, TRUE, 0);
    gtk_widget_show (cpr);

    if (malleable)
    gtk_signal_connect (GTK_OBJECT (ur_sockset),
                        "web100obj_changed",
                        GTK_SIGNAL_FUNC (cpr_sockset_listen),
                        cpr); 

  }

  if (!strncmp ("dtb", name, 3)) {
    dtb = dtb_new (web100obj);
    gtk_box_pack_start (GTK_BOX(vbox), dtb, TRUE, TRUE, 0);
    gtk_widget_show (dtb);

    if (malleable)
    gtk_signal_connect (GTK_OBJECT (ur_sockset),
                        "web100obj_changed",
                        GTK_SIGNAL_FUNC (dtb_sockset_listen),
                        dtb); 

  } 
  if (!strncmp ("rtuner", name, 6)) {
    rtuner = rtuner_new (web100obj);
    gtk_box_pack_start (GTK_BOX(vbox), rtuner, TRUE, TRUE, 0);
    gtk_widget_show (rtuner);

    if (malleable)
    gtk_signal_connect (GTK_OBJECT (ur_sockset),
                        "web100obj_changed",
                        GTK_SIGNAL_FUNC (rtuner_sockset_listen),
                        rtuner); 

  }
  if (!strncmp ("stuner", name, 6)) {
    stuner = stuner_new (web100obj);
    gtk_box_pack_start (GTK_BOX(vbox), stuner, TRUE, TRUE, 0);
    gtk_widget_show (stuner);

    if (malleable)
    gtk_signal_connect (GTK_OBJECT (ur_sockset),
                        "web100obj_changed",
                        GTK_SIGNAL_FUNC (stuner_sockset_listen),
                        stuner); 

  } 
  if (!strncmp ("triage", name, 6)) {
    triage = triage_new (web100obj);
    gtk_box_pack_start (GTK_BOX(vbox), triage, TRUE, TRUE, 0);
    gtk_widget_show (triage);

    if (malleable)
    gtk_signal_connect (GTK_OBJECT (ur_sockset),
                        "web100obj_changed",
                        GTK_SIGNAL_FUNC (triage_sockset_listen),
                        triage); 

  }
  gtk_widget_show (window);
}
