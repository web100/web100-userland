#include "config.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <gtk/gtk.h>
#include "web100poll.h"
#include "web100object.h"
#include "utillaunch.h"

Web100Poll *wpoll;
Web100Object *obj;

void
usage (void)
{
  printf ("Usage: gutil [tool [data]] [cid] [-l|-r]\n");
}

int main(int argc, char *argv[] )
{ 
  extern int optind;
  int option;
  int count;
  const char *rcfile;
  gboolean local;
  
  gtk_init(&argc, &argv);

  if (rcfile = getenv("WEB100_RC_FILE")) {
    gtk_rc_parse(rcfile);
  }
  else {
#ifdef GTK2
  gtk_rc_parse(WEB100_CONF_DIR "/web100.2.rc");
#else
  gtk_rc_parse(WEB100_CONF_DIR "/web100.1.rc");
#endif
  }

  while ((option = getopt(argc, argv, "lr")) != -1)
    switch (option)
    {
      case 'l':
       	if (rcfile = getenv("WEB100_RC_FILE_LOCAL")) {
	  gtk_rc_parse(rcfile);
       	}
	else {
	  gtk_rc_parse(WEB100_CONF_DIR "/lcl.rc"); 
	}
	local = TRUE;
	break;
      case 'r':
       	if (rcfile = getenv("WEB100_RC_FILE_REMOTE")) {
	  gtk_rc_parse(rcfile);
       	}
	else {
	  gtk_rc_parse(WEB100_CONF_DIR "/rmt.rc"); 
	}
	local = FALSE;
	break;
      default:
       	break;
    } 

  if ((wpoll = WEB100_POLL (web100_poll_new (NULL))) == NULL) { 
    exit(EXIT_FAILURE);
  }

  if (argc-optind > 3) { 
    usage();
    exit(EXIT_FAILURE);
  }

  if ((argc > 1) && isdigit(argv[argc-1][0])) {
    count = argc-optind-1;
    if ((obj = WEB100_OBJECT (web100_object_new (wpoll, atoi(argv[argc-1])))) == NULL)
      obj = WEB100_OBJECT (web100_object_new (wpoll, -1)); 
  }
  else {
    count = argc-optind; 
    obj = WEB100_OBJECT (web100_object_new (wpoll, -1));
  }

  switch (count)
  {
    case 2:
      util_launch(argv[optind], obj, TRUE, local, argv[optind+1]);
      break;
    case 1:
      util_launch(argv[optind], obj, TRUE, local, NULL);
      break;
    case 0:
    default:
      util_launch("toolbox", obj, TRUE, local, NULL);
      break;
  }

  gtk_main ();

  return(0);
} 
