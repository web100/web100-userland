#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <gtk/gtk.h>
#include "web100.h"
#include "web100sync.h"
#include "web100obj.h"
#include "ur_launch.h"


Web100Sync *web100sync;
Web100Obj *web100obj;

int main(int argc, char *argv[] )
{ 
  extern int optind;
  int option;

  gtk_init(&argc, &argv);

  if (argc < 2) {
    printf("Usage: util tool_name [ConnectionID] [-l(ocal)|-r(emote)]\n");
    exit(2);
  }

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

  if(argv[optind+1]) 
    web100obj = WEB100_OBJ (web100obj_new(web100sync, argv[optind+1])); 
  else
    web100obj = WEB100_OBJ (web100obj_new(web100sync, NULL));

  ur_launch_util(argv[optind], web100obj, TRUE, TRUE);

  gtk_main ();

  return 0;
} 
