#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
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
#include "vdt_wgt.h"


Web100Sync *web100sync;
Web100Obj *web100obj;

void launch_util (char *name, Web100Obj *web100obj, char varname[WEB100_VARNAME_LEN_MAX], gboolean master)
{
static  GtkWidget *window, *vbox, *ur_sockset, *avd_list, *avd, *cpr, *dtb, *rtuner, *stuner, *triage, *vdt;

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

  ur_sockset = sockset_new (web100obj, TRUE);
  gtk_box_pack_start (GTK_BOX(vbox), ur_sockset, FALSE, FALSE, 0);
  gtk_widget_show (ur_sockset);

  if (!strncmp ("avd_list", name, 8)) {
    avd_list = avd_list_new (web100obj);
    gtk_box_pack_start (GTK_BOX(vbox), avd_list, TRUE, TRUE, 0);
    gtk_widget_show (avd_list);

    gtk_signal_connect (GTK_OBJECT (ur_sockset),
       	"web100obj_changed",
       	GTK_SIGNAL_FUNC (avd_list_sockset_listen),
       	(gpointer) avd_list); 
  } 
  if (!strncmp ("avd_table", name, 9)) {
    avd = avd_table_new (web100obj);
    gtk_box_pack_start (GTK_BOX(vbox), avd, TRUE, TRUE, 0);
    gtk_widget_show (avd);
  
    gtk_signal_connect (GTK_OBJECT (ur_sockset),
                        "web100obj_changed",
                        GTK_SIGNAL_FUNC (avd_table_sockset_listen),
                        avd); 
  } 
  if (!strncmp ("cpr", name, 3)) {
    cpr = cpr_new (web100obj);
    gtk_box_pack_start (GTK_BOX(vbox), cpr, TRUE, TRUE, 0);
    gtk_widget_show (cpr);

    gtk_signal_connect (GTK_OBJECT (ur_sockset),
                        "web100obj_changed",
                        GTK_SIGNAL_FUNC (cpr_sockset_listen),
                        cpr); 
  } 
  if (!strncmp ("dtb", name, 3)) {
    dtb = dtb_new (web100obj);
    gtk_box_pack_start (GTK_BOX(vbox), dtb, TRUE, TRUE, 0);
    gtk_widget_show (dtb);

    gtk_signal_connect (GTK_OBJECT (ur_sockset),
                        "web100obj_changed",
                        GTK_SIGNAL_FUNC (dtb_sockset_listen),
                        dtb); 
  } 
  if (!strncmp ("rtuner", name, 6)) {
    rtuner = rtuner_new (web100obj);
    gtk_box_pack_start (GTK_BOX(vbox), rtuner, TRUE, TRUE, 0);
    gtk_widget_show (rtuner);

    gtk_signal_connect (GTK_OBJECT (ur_sockset),
                        "web100obj_changed",
                        GTK_SIGNAL_FUNC (rtuner_sockset_listen),
                        rtuner); 
  }
  if (!strncmp ("stuner", name, 6)) {
    stuner = stuner_new (web100obj);
    gtk_box_pack_start (GTK_BOX(vbox), stuner, TRUE, TRUE, 0);
    gtk_widget_show (stuner);

    gtk_signal_connect (GTK_OBJECT (ur_sockset),
                        "web100obj_changed",
                        GTK_SIGNAL_FUNC (stuner_sockset_listen),
                        stuner); 
  } 
  if (!strncmp ("triage", name, 6)) {
    triage = triage_new (web100obj);
    gtk_box_pack_start (GTK_BOX(vbox), triage, TRUE, TRUE, 0);
    gtk_widget_show (triage);

    gtk_signal_connect (GTK_OBJECT (ur_sockset),
                        "web100obj_changed",
                        GTK_SIGNAL_FUNC (triage_sockset_listen),
                        triage); 
  }
  if (!strncmp ("vdt", name, 3)) {
    vdt = vdt_new (web100obj, varname);
    gtk_box_pack_start (GTK_BOX(vbox), vdt, TRUE, TRUE, 0);
    gtk_widget_show (vdt);

    gtk_signal_connect (GTK_OBJECT (ur_sockset),
       	"web100obj_changed",
       	GTK_SIGNAL_FUNC (vdt_sockset_listen),
       	vdt); 
  }
  gtk_widget_show (window);
}

int main(int argc, char *argv[] )
{ 
  extern int optind;
  int option;

  gtk_init(&argc, &argv);

  gtk_rc_parse(WEB100_CONF_DIR "/web100.rc");
  while ((option = getopt(argc, argv, "lr")) != -1)
    switch(option)
    {
      case 'l':
       	gtk_rc_parse(WEB100_CONF_DIR "/lcl.rc"); 
	break;
      case 'r':
       	gtk_rc_parse(WEB100_CONF_DIR "/rmt.rc"); 
	break;
      default: 
       	break;
    } 

  if ((web100sync = (Web100Sync *) web100sync_new (NULL)) == NULL) { 
    exit(1);
  }

  if(argv[optind]) {
    if (argv[optind+1]) { 
      if (argv[optind+2]) {
       	web100obj = WEB100_OBJ (web100obj_new(web100sync, argv[optind+2]));
       	launch_util(argv[optind], web100obj, argv[optind+1], TRUE);
      }
      else {
       	web100obj = WEB100_OBJ (web100obj_new(web100sync, argv[optind+1]));
       	launch_util(argv[optind], web100obj, NULL, TRUE); 
      }
    }
    else {
      if (isdigit(argv[optind][0])) {
	web100obj = WEB100_OBJ (web100obj_new(web100sync, argv[optind]));
       	launch_util("dtb", web100obj, NULL, TRUE);
      }
      else {
	web100obj = WEB100_OBJ (web100obj_new(web100sync, NULL));
       	launch_util(argv[optind], web100obj, NULL, TRUE);
      }
    }
  }
  else {
    web100obj = WEB100_OBJ (web100obj_new(web100sync, NULL));
    launch_util("dtb", web100obj, NULL, TRUE);
  }

  gtk_main ();

  return 0;
} 
