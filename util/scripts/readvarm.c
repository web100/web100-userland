/*
 * readvarm: reads the current values for 1 or more web100 variables in a 
 *           connection. Web100 variables are separated by spaces.
 *
 * Usage: readvarm connectionID <var name> [<var name> ...]
 * Example: readvarm 1359 LocalPort LocalAddress RemotePort RemoteAddress      
 *
 * Copyright (c) 2001
 *      Carnegie Mellon University, The Board of Trustees of the University
 *      of Illinois, and University Corporation for Atmospheric Research.
 *      All rights reserved.  This software comes with NO WARRANTY.
 *
 * Since our code is currently under active development we prefer that
 * everyone gets the it directly from us.  This will permit us to
 * collaborate with all of the users.  So for the time being, please refer
 * potential users to us instead of redistributing web100.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include "web100/web100.h"

static const char* argv0 = NULL;

//Display correct usage of script.
static void
usage(void)
{
    fprintf(stderr,
            "Usage: %s <connection id> <var name> [<var name> ...]\n",
            argv0);
}

int
main(int argc, char *argv[])
{
    web100_agent      *agent;
    web100_connection *conn;
    web100_group      *group;
    web100_var        *var;

    char buf[8];
    int cid;
    char **arg;

    argv0 = argv[0];

    //check that we have at least a connectionID and one variable. 
    if (argc < 3) {
        usage();
        exit(EXIT_FAILURE);
    }

    //get connection id from argv[1]
    cid = atoi(argv[1]);

    //Attach to a web100 agent on the local machine. see 'man web100_attach'
    if ((agent = web100_attach(WEB100_AGENT_TYPE_LOCAL, NULL)) == NULL) {
        web100_perror("web100_attach");
        exit(EXIT_FAILURE);
    }
    
    //Lookup the connection, if it does not exist spit out error.
    if ((conn = web100_connection_lookup(agent, cid)) == NULL) {
        web100_perror("web100_connection_lookup");
        exit(EXIT_FAILURE);
    }
    
    // for each of the web100 variables, find the var and group for each 
    // spit out error and exit if the variable doesnt exist.
    // Note: The script will stop once it hits a variable it doesnt know.
 
    for( arg=&argv[2]; *arg; arg++ ){
      if ((web100_agent_find_var_and_group(agent, *arg, &group, &var)) != 
	  WEB100_ERR_SUCCESS) {
        web100_perror("web100_agent_find_var_and_group");
        exit(EXIT_FAILURE);
      }

      //Read the variable.
      if ((web100_raw_read(var, conn, buf)) != WEB100_ERR_SUCCESS) {
        web100_perror("web100_raw_read");
        exit(EXIT_FAILURE);
      }

     //Print out variable name and value to user.
      printf("%-20s: %s\n", *arg, web100_value_to_text(web100_get_var_type(var), 
						    buf));
    }

   //detach from agent.
   web100_detach(agent);

    return 0;
}

