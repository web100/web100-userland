#include <stdio.h>
#include <gtk/gtk.h>
#include "web100.h"
#include "web100sync.h"
#include "web100obj.h"
#include "sockset_wgt.h"
#include "avd_list_wgt.h"
#include "avd_table_wgt.h"
#include "dtb_wgt.h"
#include "ur_launch.h"
//#include "cpr_wgt.h"

web100_agent *agent;
Web100Sync *web100sync;
Web100Obj *web100obj;

GtkWidget *window, *vbox, *sockset, *avd_list, *avd, *cpr, *dtb;
int main(int argc, char *argv[] )
{ 
  gtk_init (&argc, &argv);
  gtk_rc_parse(WEB100_CONF_DIR "/web100.rc");

  if (argc < 2) {
    printf("Usage: util Widget_name [ConnectionID] [-l(ocal)|-r(emote)]\n");
    exit(2);
  }

  if ((web100sync = (Web100Sync *) web100sync_new (NULL)) == NULL) { 
    exit(1);
  }

  if(argc >= 3) 
  web100obj = WEB100_OBJ (web100obj_new(web100sync, argv[2])); 
  else web100obj = WEB100_OBJ (web100obj_new(web100sync, NULL));

  ur_launch_util (argv[1], web100obj, TRUE, TRUE);

  gtk_main ();
  
  return 0;
} 
